



#include "main.h"
#include "video.h"
#include "Utils.h"
#include "mouse.h"
#include "keyboard.h"
#include "libc.h"
#include "math.h"
#include "window.h"
#include "guiHelper.h"
#include "cmosPeriodTimer.h"
#include "memory.h"
#include "systemService.h"
#include "apic.h"
#include "task.h"
#include "math.h"

#define sqrt __sqrt
#define sqrtf __sqrtf
#define exp __exp
#define expf __expf
#define log __log
#define sin __sin
#define cos __cos
#define sinf __sinf
#define cosf __cosf
#define fabs __fabs
#define fabsf __fabsf
#define logf __logf

#define malloc mymalloc
#define free myfree
#define realloc __realloc
#define calloc __calloc

#define abort __abort
#define memset __memset

#define memcpy __memcpy

// --- 硬件定义 ---
#define SCREEN_WIDTH gVideoWidth
#define SCREEN_HEIGHT gVideoHeight
//#define BYTES_PER_PIXEL 4 // 32-bit (XRGB)
// 注意：在实模式DOS下，线性显存地址通常是 0xA0000 (需分段) 或通过 DPMI 映射到 0x4000000
// 这里假设使用 DJGPP 或类似环境，可以直接访问线性地址
//#define LFB_ADDRESS 0x4000000 

// --- 数据结构 ---
typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    float r, g, b;
} Color3;

// --- 全局显存指针 ---
uint32_t* framebuffer = (uint32_t*)gGraphBase;
float * g_my_zbuffer = 0;

// --- 纯 C 数学库 (指针传参版) ---

// 向量加法: out = v1 + v2
void vec3_add(Vec3* out, const Vec3* v1, const Vec3* v2) {
    out->x = v1->x + v2->x;
    out->y = v1->y + v2->y;
    out->z = v1->z + v2->z;
}

// 向量减法: out = v1 - v2
void vec3_sub(Vec3* out, const Vec3* v1, const Vec3* v2) {
    out->x = v1->x - v2->x;
    out->y = v1->y - v2->y;
    out->z = v1->z - v2->z;
}

// 向量点积
float vec3_dot(const Vec3* v1, const Vec3* v2) {
    return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
}

// 向量归一化
void vec3_normalize(Vec3* out, const Vec3* v) {
    float len_sq = v->x * v->x + v->y * v->y + v->z * v->z;
    if (len_sq > 0.0f) {
        float inv_len = 1.0f / sqrtf(len_sq);
        out->x = v->x * inv_len;
        out->y = v->y * inv_len;
        out->z = v->z * inv_len;
    }
}

// 旋转向量 (X轴): out = rotate(v, angle)
void vec3_rotate_x(Vec3* out, const Vec3* v, float angle) {
    float cos_a = cosf(angle);
    float sin_a = sinf(angle);
    out->x = v->x;
    out->y = v->y * cos_a - v->z * sin_a;
    out->z = v->y * sin_a + v->z * cos_a;
}

// 旋转向量 (Y轴): out = rotate(v, angle)
void vec3_rotate_y(Vec3* out, const Vec3* v, float angle) {
    float cos_a = cosf(angle);
    float sin_a = sinf(angle);
    out->x = v->x * cos_a + v->z * sin_a;
    out->y = v->y;
    out->z = -v->x * sin_a + v->z * cos_a;
}

// --- 渲染辅助函数 ---

// 设置像素 (32-bit)
void put_pixel(int x, int y, uint32_t color) {
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        framebuffer[y * SCREEN_WIDTH + x] = color;
    }
}

// 绘制光纤 (带简单辉光效果)
void draw_fiber(Vec3* p1, Vec3* p2, uint32_t color) {
    int x0 = (int)p1->x;
    int y0 = (int)p1->y;
    int x1 = (int)p2->x;
    int y1 = (int)p2->y;

    int dx = abs(x1 - x0);
    int dy = -abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;
    int e2;

    while (1) {
        // 核心光束
        put_pixel(x0, y0, color);

        // 辉光 (简单的加法混合模拟：在显存上直接画较暗的边缘)
        // 注意：真加法混合需要读取显存当前值，这里简化处理
        if (x0 > 0) put_pixel(x0 - 1, y0, 0x00444444);
        if (y0 > 0) put_pixel(x0, y0 - 1, 0x00444444);

        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// --- 3D 场景数据 ---
Vec3 sphere_center = { 0.0f, 0.0f, 5.0f }; // 球心位置
float sphere_radius = 2.0f;
Vec3 light_dir = { -0.5f, -0.5f, -1.0f }; // 光源方向

// --- 主渲染逻辑 ---
void render_scene(float angle) {
    // 1. 清空 Z-Buffer
    // 使用 memset 设置最大深度
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        g_my_zbuffer[i] = 1000.0f;
    }

    // 2. 清空屏幕
    __memset((char*)framebuffer, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(int));

    Vec3 v_rotated, v_proj;
    Vec3 normal, light_norm;
    Color3 color;

    // 归一化光照向量
    vec3_normalize(&light_norm, &light_dir);

    // 3. 光线投射球体 (Ray Casting Sphere)
    // 遍历屏幕每个像素
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            // 将屏幕坐标映射到 -1 ~ 1 的空间
            float ray_x = (x - SCREEN_WIDTH / 2) * 0.01f;
            float ray_y = (y - SCREEN_HEIGHT / 2) * 0.01f;

            // 简单的正交投影射线
            Vec3 ray_origin = { ray_x, ray_y, 0.0f };
            Vec3 ray_dir = { 0.0f, 0.0f, 1.0f };

            // 计算球体变换后的位置
            vec3_rotate_y(&v_rotated, &sphere_center, angle);
            vec3_rotate_x(&v_rotated, &v_rotated, angle * 0.5f);

            // 射线与球体相交检测
            // |ray + t*dir - center|^2 = r^2
            // 简化为求解 t 的二次方程
            Vec3 oc;
            vec3_sub(&oc, &ray_origin, &v_rotated);

            float a = vec3_dot(&ray_dir, &ray_dir);
            float b = 2.0f * vec3_dot(&oc, &ray_dir);
            float c = vec3_dot(&oc, &oc) - (sphere_radius * sphere_radius);
            float discriminant = b * b - 4 * a * c;

            if (discriminant >= 0) {
                float t = (-b - sqrtf(discriminant)) / (2 * a);

                // Z-Buffer 测试
                if (t < g_my_zbuffer[y * SCREEN_WIDTH + x]) {
                    g_my_zbuffer[y * SCREEN_WIDTH + x] = t;

                    // 计算交点位置
                    Vec3 hit_point;
                    hit_point.x = ray_origin.x + t * ray_dir.x;
                    hit_point.y = ray_origin.y + t * ray_dir.y;
                    hit_point.z = ray_origin.z + t * ray_dir.z;

                    // 计算法向量 (交点 - 球心)
                    vec3_sub(&normal, &hit_point, &v_rotated);
                    vec3_normalize(&normal, &normal);

                    // --- 光照计算 (Lambertian Diffuse) ---
                    float diff = vec3_dot(&normal, &light_norm);
                    if (diff < 0) diff = 0;

                    // 基础颜色 (蓝色) + 光照
                    float final_r = diff * 0.2f;
                    float final_g = diff * 0.4f;
                    float final_b = 0.2f + diff * 0.8f;

                    // 转换为 32-bit 颜色 (0x00RRGGBB)
                    uint32_t pixel_color = ((int)(final_r * 255) << 16) |
                        ((int)(final_g * 255) << 8) |
                        ((int)(final_b * 255));

                    put_pixel(x, y, pixel_color);
                }
            }
        }
    }

    // 4. 绘制光纤特效
    // 从屏幕四角向球体中心发射激光
    Vec3 screen_corners[4] = {
        {0, 0, 0}, {SCREEN_WIDTH, 0, 0},
        {SCREEN_WIDTH, SCREEN_HEIGHT, 0}, {0, SCREEN_HEIGHT, 0}
    };

    // 投影球心到屏幕空间
    Vec3 projected_center;
    // 简单的投影公式
    projected_center.x = (v_rotated.x * 100) + SCREEN_WIDTH / 2;
    projected_center.y = (v_rotated.y * 100) + SCREEN_HEIGHT / 2;

    for (int i = 0; i < 4; i++) {
        // 动态颜色：随时间变化的青色/紫色
        uint32_t fiber_color = (int)(sinf(angle * 5 + i) * 127 + 128) << 16 | 0x00FFFF;
        draw_fiber(&screen_corners[i], &projected_center, fiber_color);
    }
}

// --- 入口函数 ---
extern "C" __declspec(dllexport) int Render3D(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD runparam) {
    WINDOWCLASS window;
    __memset((char*)&window, 0, sizeof(WINDOWCLASS));
    __strcpy(window.caption, funcname);
    initFullWindow(&window, funcname, tid, 1);
    float angle = 0.0f;

    // 初始化 VESA 模式 (伪代码，实际需调用 INT 0x10)
    // set_vesa_mode(320, 240, 32);

    g_my_zbuffer = (float*)malloc(gVideoWidth * gVideoHeight * sizeof(float));

    // 主循环
    while (1) {
        unsigned int ck = __kGetKbd(window.id);
        unsigned int asc = ck & 0xff;
        if (asc == 0x1b) {
            __DestroyWindow(&window);
            free(g_my_zbuffer);
            return 0;
        }

        MOUSEINFO mouseinfo;
        __memset((char*)&mouseinfo, 0, sizeof(MOUSEINFO));
        int ret = __kGetMouse(&mouseinfo, window.id);

        if (mouseinfo.status & 1) {  // left click
            if (mouseinfo.x >= window.shutdownx && mouseinfo.x <= window.shutdownx + window.capHeight &&
                mouseinfo.y >= window.shutdowny && mouseinfo.y <= window.shutdowny + window.capHeight) {
                __DestroyWindow(&window);
                free(g_my_zbuffer);
                return 0;
            }
            if (mouseinfo.x >= window.minx && mouseinfo.x <= window.minx + window.capHeight &&
                mouseinfo.y >= window.miny && mouseinfo.y <= window.miny + window.capHeight) {
                MinimizeWindow(&window);
            }
        }

        render_scene(angle);
        angle += 0.02f;

        __sleep(0);
    }
}