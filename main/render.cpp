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

#define tanf __tanf

#define malloc mymalloc
#define free myfree
#define realloc __realloc
#define calloc __calloc

#define abort __abort


// ================= 1. 硬件抽象层配置 (请根据内核修改这里) =================

// 屏幕分辨率
#define WIDTH  gVideoWidth
#define HEIGHT gVideoHeight

// 显存地址：请修改为您内核分配的实际物理地址
// 例如：0xE0000000 (线性帧缓冲) 或 0x000B8000 (实模式 VGA)
//#define FRAMEBUFFER_ADDR 0xE0000000 

// 定义颜色格式宏 (假设是 32位色，格式为 0x00RRGGBB)
// 如果是 24位色，您可能需要按字节写入，这里统一按 32位处理以简化逻辑
typedef uint32_t color_t;

// 像素绘制函数：直接向显存写入
void put_pixel_old(int x, int y, color_t color) {
    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
        // 将显存地址强转为 uint32_t 指针
        volatile color_t* fb = (color_t*)gGraphBase;
        fb[y * WIDTH + x] = color;
    }
}

void put_pixel(int x, int y, uint32_t color) {
    PROCESS_INFO* process = (PROCESS_INFO*)GetCurrentTaskTssBase();
    uint8_t* video_mem = (uint8_t*)process->videoBase;
    int offset;

    // 边界检查
    if (x < 0 || x >= gVideoWidth || y < 0 || y >= gVideoHeight) {
        return;
    }

    // 计算显存偏移（24位RGB，每个像素3字节）
    offset = (y * gVideoWidth + x) * gBytesPerPixel;

    // 写入RGB分量（小端序：BGR顺序）
    for (int i = 0; i < gBytesPerPixel; i++) {
        video_mem[offset + i] = (color) & 0xFF;  // 蓝色
        color >>= 8;
    }
}

// ================= 2. 数学库 (纯 C 指针风格，无结构体返回值) =================

typedef struct { float x, y, z; } Vec3;
typedef struct { float m[4][4]; } Mat4;

// --- 向量运算 ---

// 设置向量值
void vec3_set(Vec3* out, float x, float y, float z) {
    out->x = x; out->y = y; out->z = z;
}

// 向量减法: out = a - b
void vec3_sub(Vec3* out, const Vec3* a, const Vec3* b) {
    out->x = a->x - b->x;
    out->y = a->y - b->y;
    out->z = a->z - b->z;
}

// 向量叉乘: out = a x b
void vec3_cross(Vec3* out, const Vec3* a, const Vec3* b) {
    out->x = a->y * b->z - a->z * b->y;
    out->y = a->z * b->x - a->x * b->z;
    out->z = a->x * b->y - a->y * b->x;
}

// 向量点乘: 返回标量
float vec3_dot(const Vec3* a, const Vec3* b) {
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

// 向量归一化: out = normalize(v)
void vec3_norm(Vec3* out, const Vec3* v) {
    float len_sq = v->x * v->x + v->y * v->y + v->z * v->z;
    if (len_sq == 0.0f) { vec3_set(out, 0, 0, 0); return; }
    float inv_len = 1.0f / sqrtf(len_sq);
    out->x = v->x * inv_len;
    out->y = v->y * inv_len;
    out->z = v->z * inv_len;
}

// --- 矩阵运算 ---

// 创建单位矩阵
void mat4_identity(Mat4* out) {
    __memset((char*)out, 0, sizeof(Mat4));
    out->m[0][0] = 1.0f; out->m[1][1] = 1.0f;
    out->m[2][2] = 1.0f; out->m[3][3] = 1.0f;
}

// 矩阵乘法: out = a * b
void mat4_multiply(Mat4* out, const Mat4* a, const Mat4* b) {
    Mat4 temp;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            temp.m[i][j] = 0;
            for (int k = 0; k < 4; k++) {
                temp.m[i][j] += a->m[i][k] * b->m[k][j];
            }
        }
    }
    __memcpy((char*)out, (char*)&temp, sizeof(Mat4));
}

// 矩阵变换向量: out = m * v
void mat4_transform(Vec3* out, const Mat4* m, const Vec3* v) {
    // 齐次坐标变换
    out->x = v->x * m->m[0][0] + v->y * m->m[1][0] + v->z * m->m[2][0] + m->m[3][0];
    out->y = v->x * m->m[0][1] + v->y * m->m[1][1] + v->z * m->m[2][1] + m->m[3][1];
    out->z = v->x * m->m[0][2] + v->y * m->m[1][2] + v->z * m->m[2][2] + m->m[3][2];
}

// 创建透视投影矩阵
void mat4_perspective(Mat4* out, float fov, float aspect, float near, float far) {
    __memset((char*)out, 0, sizeof(Mat4));
    float tan_half_fov = tanf(fov / 2.0f);
    out->m[0][0] = 1.0f / (aspect * tan_half_fov);
    out->m[1][1] = 1.0f / tan_half_fov;
    out->m[2][2] = -(far + near) / (far - near);
    out->m[2][3] = -1.0f;
    out->m[3][2] = -(2.0f * far * near) / (far - near);
}

// ================= 3. 3D 渲染管线 =================

// 全局 Z-Buffer (深度缓冲)
float * z_buffer = 0;

// 模型数据：一个四面体 (金字塔)
Vec3 cube_vertices[] = {
    {0.0f, 1.0f, 0.0f},   // 顶点
    {-1.0f, -1.0f, -1.0f},// 左下后
    {1.0f, -1.0f, -1.0f}, // 右下后
    {0.0f, -1.0f, 1.0f}   // 下前
};

// 面索引与颜色
int faces[][3] = { {0,1,2}, {0,2,3}, {0,3,1}, {1,3,2} };
color_t face_colors[] = { 0xFF0000, 0x00FF00, 0x0000FF, 0xFFFF00 }; // 红绿蓝黄

// 绘制单个三角形 (带 Z-Buffer 测试)
void draw_triangle(Vec3* v0, Vec3* v1, Vec3* v2, color_t color) {
    // 1. 计算包围盒
    int min_x = (int)((v0->x < v1->x ? (v0->x < v2->x ? v0->x : v2->x) : (v1->x < v2->x ? v1->x : v2->x)));
    int max_x = (int)((v0->x > v1->x ? (v0->x > v2->x ? v0->x : v2->x) : (v1->x > v2->x ? v1->x : v2->x)));
    int min_y = (int)((v0->y < v1->y ? (v0->y < v2->y ? v0->y : v2->y) : (v1->y < v2->y ? v1->y : v2->y)));
    int max_y = (int)((v0->y > v1->y ? (v0->y > v2->y ? v0->y : v2->y) : (v1->y > v2->y ? v1->y : v2->y)));

    // 屏幕边界裁剪
    if (min_x < 0) min_x = 0;
    if (min_y < 0) min_y = 0;
    if (max_x >= WIDTH) max_x = WIDTH - 1;
    if (max_y >= HEIGHT) max_y = HEIGHT - 1;

    // 2. 扫描线光栅化
    for (int y = min_y; y <= max_y; y++) {
        for (int x = min_x; x <= max_x; x++) {
            float px = (float)x;
            float py = (float)y;

            // 重心坐标判断点是否在三角形内 (2D 叉乘)
            float w0 = (v1->x - v0->x) * (py - v0->y) - (v1->y - v0->y) * (px - v0->x);
            float w1 = (v2->x - v1->x) * (py - v1->y) - (v2->y - v1->y) * (px - v1->x);
            float w2 = (v0->x - v2->x) * (py - v2->y) - (v0->y - v2->y) * (px - v2->x);

            // 判断 winding order (假设逆时针为正)
            if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                // 3. 深度插值与测试
                // 简单的线性插值 (忽略透视校正以节省性能)
                float area = w0 + w1 + w2;
                if (area > 0) {
                    float alpha = w0 / area;
                    float beta = w1 / area;
                    float gamma = w2 / area;

                    // 插值深度 Z
                    float z = v0->z * alpha + v1->z * beta + v2->z * gamma;

                    int idx = y * WIDTH + x;
                    if (z < z_buffer[idx]) {
                        z_buffer[idx] = z;
                        put_pixel(x, y, color);
                    }
                }
            }
        }
    }
}

// 主渲染函数
void render() {
    // 1. 清空 Z-Buffer
    for (int i = 0; i < WIDTH * HEIGHT; i++) z_buffer[i] = 99999.0f; // 设为极大值

    // 2. 准备矩阵
    static float angle = 0;
    angle += 0.02f; // 旋转速度

    Mat4 rot, trans, proj, mvp;

    // 旋转矩阵 (绕 Y 轴)
    mat4_identity(&rot);
    rot.m[0][0] = cosf(angle); rot.m[0][2] = sinf(angle);
    rot.m[2][0] = -sinf(angle); rot.m[2][2] = cosf(angle);

    // 平移矩阵 (向后移)
    mat4_identity(&trans);
    trans.m[3][2] = -5.0f;

    // 投影矩阵
    mat4_perspective(&proj, 3.14159f / 4.0f, (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);

    // 组合矩阵: MVP = Projection * Translation * Rotation
    Mat4 temp_mat;
    mat4_multiply(&temp_mat, &trans, &rot);
    mat4_multiply(&mvp, &proj, &temp_mat);

    // 光照方向
    Vec3 light_dir; vec3_set(&light_dir, 0.5f, 1.0f, -1.0f);
    vec3_norm(&light_dir, &light_dir);

    // 3. 渲染循环
    for (int i = 0; i < 4; i++) {
        int i0 = faces[i][0];
        int i1 = faces[i][1];
        int i2 = faces[i][2];

        // 变换顶点
        Vec3 t0, t1, t2;
        mat4_transform(&t0, &mvp, &cube_vertices[i0]);
        mat4_transform(&t1, &mvp, &cube_vertices[i1]);
        mat4_transform(&t2, &mvp, &cube_vertices[i2]);

        // 背面剔除 (Backface Culling)
        Vec3 edge1, edge2, normal;
        vec3_sub(&edge1, &t1, &t0);
        vec3_sub(&edge2, &t2, &t0);
        vec3_cross(&normal, &edge1, &edge2);

        // 如果法线 Z > 0，说明面朝后，不绘制
        if (normal.z > 0) continue;

        // 简单光照计算
        Vec3 world_n; // 这里简化处理，直接用变换后的法线近似
        vec3_norm(&world_n, &normal);
        float intensity = vec3_dot(&world_n, &light_dir);
        if (intensity < 0.1f) intensity = 0.1f; // 环境光

        // 颜色混合
        color_t base_color = face_colors[i];
        uint8_t r = (base_color >> 16) & 0xFF;
        uint8_t g = (base_color >> 8) & 0xFF;
        uint8_t b = base_color & 0xFF;
        color_t final_color = ((int)(r * intensity) << 16) | ((int)(g * intensity) << 8) | (int)(b * intensity);

        // 透视除法 (将齐次坐标转为屏幕坐标)
        // 注意：这里我们直接修改 t0, t1, t2 的 x,y 用于光栅化
        if (t0.z != 0) { t0.x /= t0.z; t0.y /= t0.z; }
        if (t1.z != 0) { t1.x /= t1.z; t1.y /= t1.z; }
        if (t2.z != 0) { t2.x /= t2.z; t2.y /= t2.z; }

        // 映射到屏幕空间 (-1~1 转 0~Width/Height)
        t0.x = (t0.x + 1) * WIDTH / 2;  t0.y = (1 - t0.y) * HEIGHT / 2;
        t1.x = (t1.x + 1) * WIDTH / 2;  t1.y = (1 - t1.y) * HEIGHT / 2;
        t2.x = (t2.x + 1) * WIDTH / 2;  t2.y = (1 - t2.y) * HEIGHT / 2;

        draw_triangle(&t0, &t1, &t2, final_color);
    }
}


void __clear_screen(uint32_t color) {
    PROCESS_INFO* process = (PROCESS_INFO*)GetCurrentTaskTssBase();
    unsigned char* framebuffer = (unsigned char*)process->videoBase;

    uint8_t* video_mem = (uint8_t*)framebuffer;
    int total_pixels = gVideoHeight * gVideoWidth;
    int i;

    for (i = 0; i < total_pixels; i++) {
        DWORD cc = color;
        for (int j = 0; j < gBytesPerPixel; j++) {
            unsigned char c = (cc) & 0xFF;
            if (video_mem[i * gBytesPerPixel + j] != c) {
                video_mem[i * gBytesPerPixel + j] = c;
            }

            cc >>= 8;
        }


    }

}


// 主循环入口 (由你的 OS 内核调用)
extern "C" __declspec(dllexport) int Render3DCube(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD runparam) {
    WINDOWCLASS window;
    __memset((char*)&window, 0, sizeof(WINDOWCLASS));
    __strcpy(window.caption, funcname);
    initFullWindow(&window, funcname, tid, 1);

    z_buffer = (float*)malloc(gVideoWidth * gVideoHeight * sizeof(float));

    // 主循环
    while (1) {
        unsigned int ck = __kGetKbd(window.id);
        unsigned int asc = ck & 0xff;
        if (asc == 0x1b) {
            __DestroyWindow(&window);
            free(z_buffer);
            return 0;
        }

        MOUSEINFO mouseinfo;
        __memset((char*)&mouseinfo, 0, sizeof(MOUSEINFO));
        int ret = __kGetMouse(&mouseinfo, window.id);

        if (mouseinfo.status & 1) {  // left click
            if (mouseinfo.x >= window.shutdownx && mouseinfo.x <= window.shutdownx + window.capHeight &&
                mouseinfo.y >= window.shutdowny && mouseinfo.y <= window.shutdowny + window.capHeight) {
                __DestroyWindow(&window);
                free(z_buffer);
                return 0;
            }
            if (mouseinfo.x >= window.minx && mouseinfo.x <= window.minx + window.capHeight &&
                mouseinfo.y >= window.miny && mouseinfo.y <= window.miny + window.capHeight) {
                MinimizeWindow(&window);
            }
        }
        __clear_screen(0);

        render();
        // 如果需要双缓冲，这里交换缓冲区

        __sleep(0);
    }
}

