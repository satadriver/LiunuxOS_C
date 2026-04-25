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
#define fmaxf __fmaxf

#define malloc my_malloc
#define free my_free
#define realloc my_realloc
#define calloc my_calloc

#define memcpy my_memcpy
#define memset	my_memset

#define abort my_abort

#define printf my_printf
#define fprintf my_fprintf

#define fread my_fread
#define fopen my_fopen
#define fwrite my_fwrite
#define fclose my_fclose
#define strcmp my_strcmp
#define strcat my_strcat
#define strlen my_strlen
#define strcpy my_strcpy
#define strncmp my_strncmp

#define wcslen my_wcslen
#define wcscmp my_wcscmp
#define wcscat my_wcscat
#define wcsstr my_wcsstr
#define wcscpy my_wcscpy

#define fputc my_fputc
#define fgetc my_fgetc
#define fgets my_fgets
#define fputs my_fputs

#define M_PI PI




/**
 * 3D 软渲染动画 (torus)
 * 适用于 32 位保护模式，线性帧缓冲，32 位色 (RGBA)
 * 编译命令: i686-elf-gcc -std=c99 -ffreestanding -O2 -m32 -c main.c -o main.o
 * 链接命令: i686-elf-ld -T your_linker.ld -o kernel.elf main.o -lm
 *
 * 注意: 请根据你的显存地址和分辨率修改 FB_ADDR, FB_WIDTH, FB_HEIGHT
 */



 /* ========== 硬件配置（请根据你的 OS 修改） ========== */
//#define FB_ADDR         0xE0000000      // 线性帧缓冲物理地址
//#define FB_WIDTH        1024            // 屏幕宽度（像素）
//#define FB_HEIGHT       768             // 屏幕高度（像素）
//#define FB_STRIDE       (FB_WIDTH * 4)  // 每行字节数（32 位色）

typedef uint32_t color_t;               // RGBA 格式 (0xAARRGGBB)
static color_t* fb = (color_t*)gGraphBase; // 直接写显存
static float* zbuffer = NULL;           // 深度缓冲（动态分配）

/* ========== 简单数学库（使用标准 C 函数） ========== */
typedef struct { float x, y, z; } vec3;
typedef struct { float x, y, z, w; } vec4;
typedef struct { float m[4][4]; } mat4;

static vec3 vec3_new(float x, float y, float z) {
    vec3 v = { x, y, z }; return v;
}
static vec4 vec4_new(float x, float y, float z, float w) {
    vec4 v = { x, y, z, w }; return v;
}
static vec3 vec3_add(vec3 a, vec3 b) {
    return vec3_new(a.x + b.x, a.y + b.y, a.z + b.z);
}
static vec3 vec3_sub(vec3 a, vec3 b) {
    return vec3_new(a.x - b.x, a.y - b.y, a.z - b.z);
}
static vec3 vec3_mul(vec3 v, float s) {
    return vec3_new(v.x * s, v.y * s, v.z * s);
}
static float vec3_dot(vec3 a, vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
static vec3 vec3_cross(vec3 a, vec3 b) {
    return vec3_new(a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x);
}
static float vec3_len(vec3 v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}
static vec3 vec3_norm(vec3 v) {
    float l = vec3_len(v);
    if (l < 1e-6f) return v;
    return vec3_new(v.x / l, v.y / l, v.z / l);
}

static mat4 mat4_identity(void) {
    mat4 m = { 0 };
    for (int i = 0; i < 4; i++) m.m[i][i] = 1.0f;
    return m;
}
static mat4 mat4_mul(mat4 a, mat4 b) {
    mat4 r = { 0 };
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            for (int k = 0; k < 4; k++)
                r.m[i][j] += a.m[i][k] * b.m[k][j];
    return r;
}
static vec4 mat4_mul_vec4(mat4 m, vec4 v) {
    vec4 r;
    r.x = m.m[0][0] * v.x + m.m[0][1] * v.y + m.m[0][2] * v.z + m.m[0][3] * v.w;
    r.y = m.m[1][0] * v.x + m.m[1][1] * v.y + m.m[1][2] * v.z + m.m[1][3] * v.w;
    r.z = m.m[2][0] * v.x + m.m[2][1] * v.y + m.m[2][2] * v.z + m.m[2][3] * v.w;
    r.w = m.m[3][0] * v.x + m.m[3][1] * v.y + m.m[3][2] * v.z + m.m[3][3] * v.w;
    return r;
}
static mat4 mat4_translate(float x, float y, float z) {
    mat4 m = mat4_identity();
    m.m[0][3] = x; m.m[1][3] = y; m.m[2][3] = z;
    return m;
}
static mat4 mat4_rotate_x(float a) {
    float c = cosf(a), s = sinf(a);
    mat4 m = mat4_identity();
    m.m[1][1] = c; m.m[1][2] = -s;
    m.m[2][1] = s; m.m[2][2] = c;
    return m;
}
static mat4 mat4_rotate_y(float a) {
    float c = cosf(a), s = sinf(a);
    mat4 m = mat4_identity();
    m.m[0][0] = c; m.m[0][2] = s;
    m.m[2][0] = -s; m.m[2][2] = c;
    return m;
}
static mat4 mat4_rotate_z(float a) {
    float c = cosf(a), s = sinf(a);
    mat4 m = mat4_identity();
    m.m[0][0] = c; m.m[0][1] = -s;
    m.m[1][0] = s; m.m[1][1] = c;
    return m;
}
static mat4 mat4_perspective(float fov, float aspect, float near, float far) {
    float tan_half = tanf(fov * 0.5f);
    mat4 m = { 0 };
    m.m[0][0] = 1.0f / (aspect * tan_half);
    m.m[1][1] = 1.0f / tan_half;
    m.m[2][2] = -(far + near) / (far - near);
    m.m[2][3] = -(2.0f * far * near) / (far - near);
    m.m[3][2] = -1.0f;
    return m;
}

/* ========== 网格数据结构 ========== */
typedef struct {
    vec3 pos;
    vec3 norm;
} vertex;
typedef struct {
    int i0, i1, i2;
} tri;
typedef struct {
    vertex* verts;
    tri* tris;
    int nvert, ntri;
} mesh;

/* 生成环面 (R: 主半径, r: 小半径, us: 环向分段, vs: 径向分段) */
static mesh* create_torus(float R, float r, int us, int vs) {
    int nv = us * vs;
    int nt = us * vs * 2;
    vertex* verts = (vertex*)malloc(nv * sizeof(vertex));
    tri* tris = (tri*)malloc(nt * sizeof(tri));

    for (int u = 0; u < us; u++) {
        float ua = 2.0f * M_PI * u / us;
        float cu = cosf(ua), su = sinf(ua);
        for (int v = 0; v < vs; v++) {
            float va = 2.0f * M_PI * v / vs;
            float cv = cosf(va), sv = sinf(va);
            float x = (R + r * cv) * cu;
            float y = (R + r * cv) * su;
            float z = r * sv;
            verts[u * vs + v].pos = vec3_new(x, y, z);
            // 法线：从环中心圆指向表面
            vec3 center_dir = vec3_new(R * cu, R * su, 0);
            vec3 to_surface = vec3_sub(verts[u * vs + v].pos, center_dir);
            verts[u * vs + v].norm = vec3_norm(to_surface);
        }
    }

    int idx = 0;
    for (int u = 0; u < us; u++) {
        int nu = (u + 1) % us;
        for (int v = 0; v < vs; v++) {
            int nv = (v + 1) % vs;
            int i00 = u * vs + v;
            int i10 = nu * vs + v;
            int i01 = u * vs + nv;
            int i11 = nu * vs + nv;
            tris[idx].i0 = i00;
            tris[idx].i1 = i10;
            tris[idx].i2 = i01;
            idx++;
            tris[idx].i0 = i10;
            tris[idx].i1 = i11;
            tris[idx].i2 = i01;
            idx++;
            //tris[idx++] = (tri){ i00, i10, i01 };
            //tris[idx++] = (tri){ i10, i11, i01 };
        }
    }

    mesh* m = (mesh*)malloc(sizeof(mesh));
    m->verts = verts; m->tris = tris;
    m->nvert = nv; m->ntri = nt;
    return m;
}

static void free_mesh(mesh* m) {
    if (m) { free(m->verts); free(m->tris); free(m); }
}

/* ========== 渲染器 ========== */
static vec3 light_dir = { 0.6f, 0.8f, 0.5f };
static color_t ambient = 0xFF202020;
static color_t diffuse = 0xFFE0A080;

static color_t shade(vec3 normal) {
    vec3 n = vec3_norm(normal);
    vec3 l = vec3_norm(light_dir);
    float diff = vec3_dot(n, l);
    if (diff < 0.2f) diff = 0.2f;  // 环境光保底
    int r = ((diffuse >> 16) & 0xFF) * diff;
    int g = ((diffuse >> 8) & 0xFF) * diff;
    int b = (diffuse & 0xFF) * diff;
    r += (ambient >> 16) & 0xFF;
    g += (ambient >> 8) & 0xFF;
    b += ambient & 0xFF;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    return (0xFF << 24) | (r << 16) | (g << 8) | b;
}

static void draw_pixel(int x, int y, float z, color_t c) {
    if (x < 0 || x >= gVideoWidth || y < 0 || y >= gVideoHeight) return;
    int idx = y * gVideoWidth + x;
    if (z < zbuffer[idx]) {
        zbuffer[idx] = z;
        fb[idx] = c;
    }
}

/* 三角形光栅化（重心坐标 + 透视修正） */
static void draw_triangle(vec4 clip[3], color_t col[3]) {
    // 透视除法得到 NDC
    float ndc_x[3], ndc_y[3];
    for (int i = 0; i < 3; i++) {
        ndc_x[i] = clip[i].x / clip[i].w;
        ndc_y[i] = clip[i].y / clip[i].w;
    }
    // 屏幕坐标
    int sx[3], sy[3];
    for (int i = 0; i < 3; i++) {
        sx[i] = (int)((ndc_x[i] * 0.5f + 0.5f) * gVideoWidth);
        sy[i] = (int)((-ndc_y[i] * 0.5f + 0.5f) * gVideoHeight);
    }

    // 包围盒
    int minx = sx[0], maxx = sx[0], miny = sy[0], maxy = sy[0];
    for (int i = 1; i < 3; i++) {
        if (sx[i] < minx) minx = sx[i];
        if (sx[i] > maxx) maxx = sx[i];
        if (sy[i] < miny) miny = sy[i];
        if (sy[i] > maxy) maxy = sy[i];
    }
    if (minx < 0) minx = 0; if (maxx >= gVideoWidth) maxx = gVideoWidth - 1;
    if (miny < 0) miny = 0; if (maxy >= gVideoHeight) maxy = gVideoHeight - 1;
    if (minx > maxx || miny > maxy) return;

    float inv_w[3];
    for (int i = 0; i < 3; i++) inv_w[i] = 1.0f / clip[i].w;

    for (int y = miny; y <= maxy; y++) {
        for (int x = minx; x <= maxx; x++) {
            // 重心坐标 (整数坐标)
            int denom = (sx[1] - sx[0]) * (sy[2] - sy[0]) - (sy[1] - sy[0]) * (sx[2] - sx[0]);
            if (denom == 0) continue;
            float w0 = ((sx[1] - sx[0]) * (y - sy[0]) - (sy[1] - sy[0]) * (x - sx[0])) / (float)denom;
            float w1 = ((sx[2] - sx[1]) * (y - sy[1]) - (sy[2] - sy[1]) * (x - sx[1])) / (float)denom;
            float w2 = 1.0f - w0 - w1;
            if (w0 < 0 || w1 < 0 || w2 < 0) continue;

            // 透视修正深度
            float z = 1.0f / (w0 * inv_w[0] + w1 * inv_w[1] + w2 * inv_w[2]);
            // 透视修正颜色
            float r = (w0 * ((col[0] >> 16) & 0xFF) * inv_w[0] +
                w1 * ((col[1] >> 16) & 0xFF) * inv_w[1] +
                w2 * ((col[2] >> 16) & 0xFF) * inv_w[2]) * z;
            float g = (w0 * ((col[0] >> 8) & 0xFF) * inv_w[0] +
                w1 * ((col[1] >> 8) & 0xFF) * inv_w[1] +
                w2 * ((col[2] >> 8) & 0xFF) * inv_w[2]) * z;
            float b = (w0 * (col[0] & 0xFF) * inv_w[0] +
                w1 * (col[1] & 0xFF) * inv_w[1] +
                w2 * (col[2] & 0xFF) * inv_w[2]) * z;
            if (r < 0) r = 0; if (r > 255) r = 255;
            if (g < 0) g = 0; if (g > 255) g = 255;
            if (b < 0) b = 0; if (b > 255) b = 255;
            color_t c = (0xFF << 24) | ((int)r << 16) | ((int)g << 8) | (int)b;
            draw_pixel(x, y, z, c);
        }
    }
}

static void render_mesh(mesh* m, mat4 model, mat4 view, mat4 proj) {
    mat4 mvp = mat4_mul(proj, mat4_mul(view, model));
    mat4 normal_mat = mat4_mul(view, model);  // 用于法线变换

    vec4* clip = (vec4*)malloc(m->nvert * sizeof(vec4));
    color_t* vcol = (color_t*)malloc(m->nvert * sizeof(color_t));
    if (!clip || !vcol) { free(clip); free(vcol); return; }

    // 顶点着色
    for (int i = 0; i < m->nvert; i++) {
        vec4 world = mat4_mul_vec4(model, vec4_new(m->verts[i].pos.x, m->verts[i].pos.y, m->verts[i].pos.z, 1.0f));
        clip[i] = mat4_mul_vec4(mvp, world);
        // 变换法线到相机空间
        vec3 n = m->verts[i].norm;
        vec3 n_cam;
        n_cam.x = normal_mat.m[0][0] * n.x + normal_mat.m[1][0] * n.y + normal_mat.m[2][0] * n.z;
        n_cam.y = normal_mat.m[0][1] * n.x + normal_mat.m[1][1] * n.y + normal_mat.m[2][1] * n.z;
        n_cam.z = normal_mat.m[0][2] * n.x + normal_mat.m[1][2] * n.y + normal_mat.m[2][2] * n.z;
        vcol[i] = shade(n_cam);
    }

    // 光栅化所有三角形
    for (int i = 0; i < m->ntri; i++) {
        tri t = m->tris[i];
        vec4 tri_clip[3] = { clip[t.i0], clip[t.i1], clip[t.i2] };
        color_t tri_col[3] = { vcol[t.i0], vcol[t.i1], vcol[t.i2] };
        draw_triangle(tri_clip, tri_col);
    }

    free(clip); free(vcol);
}



// 主循环入口 (由你的 OS 内核调用)
extern "C" __declspec(dllexport) int Render3DCube(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD runparam) {
    WINDOWCLASS window;
    __memset((char*)&window, 0, sizeof(WINDOWCLASS));
    __strcpy(window.caption, funcname);
    initFullWindow(&window, funcname, tid, 1);

    // 分配深度缓冲
    zbuffer = (float*)malloc(gVideoWidth * gVideoHeight * sizeof(float));
    if (!zbuffer) return 0;

    // 创建环面网格
    mesh* torus = create_torus(1.6f, 0.5f, 72, 36);
    if (!torus) { free(zbuffer); return 0; }

    mat4 proj = mat4_perspective(M_PI / 2.2f, (float)gVideoWidth / gVideoHeight, 0.3f, 8.0f);
    float angle = 0.0f;

    // 主循环
    while (1) {
        unsigned int ck = __kGetKbd(window.id);
        unsigned int asc = ck & 0xff;
        if (asc == 0x1b) {
            __DestroyWindow(&window);
            free_mesh(torus);
            free(zbuffer);
            return 0;
        }

        MOUSEINFO mouseinfo;
        __memset((char*)&mouseinfo, 0, sizeof(MOUSEINFO));
        int ret = __kGetMouse(&mouseinfo, window.id);

        if (mouseinfo.status & 1) {  // left click
            if (mouseinfo.x >= window.shutdownx && mouseinfo.x <= window.shutdownx + window.capHeight &&
                mouseinfo.y >= window.shutdowny && mouseinfo.y <= window.shutdowny + window.capHeight) {
                __DestroyWindow(&window);
                free_mesh(torus);
                free(zbuffer);
                return 0;
            }
            if (mouseinfo.x >= window.minx && mouseinfo.x <= window.minx + window.capHeight &&
                mouseinfo.y >= window.miny && mouseinfo.y <= window.miny + window.capHeight) {
                MinimizeWindow(&window);
            }
        }

        // 清空深度缓冲和帧缓冲
        for (int i = 0; i < gVideoWidth * gVideoHeight; i++) {
            zbuffer[i] = 1e30f;
            fb[i] = 0xFF000000;  // 黑色
        }

        // 模型矩阵：环面自转
        mat4 model = mat4_mul(mat4_rotate_y(angle), mat4_rotate_x(angle * 0.5f));

        // 视图矩阵：相机绕 Y 轴旋转，同时上下浮动
        float cam_r = 4.2f;
        float cam_a = angle * 0.3f;
        float cam_x = sinf(cam_a) * cam_r;
        float cam_z = cosf(cam_a) * cam_r;
        float cam_y = 0.8f + sinf(angle * 0.7f) * 0.6f;
        vec3 eye = vec3_new(cam_x, cam_y, cam_z);
        vec3 center = vec3_new(0, 0, 0);
        vec3 up = vec3_new(0, 1, 0);
        // 构造 LookAt 矩阵
        vec3 f = vec3_norm(vec3_sub(center, eye));
        vec3 r = vec3_norm(vec3_cross(up, f));
        vec3 u = vec3_cross(f, r);
        mat4 view = { 0 };
        view.m[0][0] = r.x; view.m[0][1] = r.y; view.m[0][2] = r.z; view.m[0][3] = -vec3_dot(r, eye);
        view.m[1][0] = u.x; view.m[1][1] = u.y; view.m[1][2] = u.z; view.m[1][3] = -vec3_dot(u, eye);
        view.m[2][0] = f.x; view.m[2][1] = f.y; view.m[2][2] = f.z; view.m[2][3] = -vec3_dot(f, eye);
        view.m[3][3] = 1.0f;

        render_mesh(torus, model, view, proj);

        angle += 0.025f;
        if (angle > 2 * M_PI) 
            angle -= 2 * M_PI;

        __sleep(0);
    }
    return 0;
}

