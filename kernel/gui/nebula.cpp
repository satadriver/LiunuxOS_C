// nebula.c - 星云/银河
// 依赖: 无
// 调用: background_nebula((char*)显存地址);

#include "../def.h"
#include "../video.h"
#include "../Utils.h"

#define _OLD_NEBULA


#ifdef _OLD_NEBULA
static float my_sqrt(float x) {
    if (x <= 0)return 0; float s = x; int i;
    for (i = 0; i < 10; i++)s = (s + x / s) * 0.5f; return s;
}
static float my_sin(float x) {
    while (x > 3.14159265f)x -= 6.28318530f;
    while (x < -3.14159265f)x += 6.28318530f;
    float x2 = x * x; return x - x2 * x / 6.0f + x2 * x2 * x / 120.0f - x2 * x2 * x2 * x / 5040.0f;
}
static float my_fabs(float x) { return x < 0 ? -x : x; }
static unsigned int hash_u(unsigned int n) { n = (n ^ (n >> 13)) * 1274126177u; return n ^ (n >> 16); }
static float hash2d(int ix, int iy) {
    return(float)(hash_u((unsigned)(ix * 374761393 + iy * 668265263)) & 0xFFFF) / 65535.0f;
}
static float value_noise(float x, float y, float sc) {
    float sx = x / sc, sy = y / sc; int ix = (int)sx, iy = (int)sy;
    float fx = sx - ix, fy = sy - iy;
    fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy);
    return(hash2d(ix, iy) * (1 - fx) + hash2d(ix + 1, iy) * fx) * (1 - fy) +
        (hash2d(ix, iy + 1) * (1 - fx) + hash2d(ix + 1, iy + 1) * fx) * fy;
}
static float fbm(float x, float y) {
    float val = 0, amp = 0.5f; int i;
    for (i = 0; i < 5; i++) { val += amp * value_noise(x, y, 80.0f); x *= 2; y *= 2; amp *= 0.5f; }
    return val;
}
static void px(char* buf, int x, int y, int r, int g, int b) {
    if (r < 0)r = 0; if (r > 255)r = 255;
    if (g < 0)g = 0; if (g > 255)g = 255;
    if (b < 0)b = 0; if (b > 255)b = 255;
    int i = (y * gVideoWidth + x) * gBytesPerPixel;
    buf[i] = b; buf[i + 1] = g; buf[i + 2] = r;
}

extern "C" __declspec(dllexport) void background_nebula(char* buf) {
    int x, y;
    for (y = 0; y < gVideoHeight; y++) {
        for (x = 0; x < gVideoWidth; x++) {
            float nx = (float)x / gVideoWidth, ny = (float)y / gVideoHeight;
            float cx = (nx - 0.5f) * 2, cy = (ny - 0.5f) * 2;
            float dist = my_sqrt(cx * cx + cy * cy);

            /* 修正的 atan2 近似: 第一象限映射到 [0, π/2] */
            float angle;
            {
                float ax = my_fabs(cx), ay = my_fabs(cy);
                float a;
                if (ax > ay)
                    a = (ay / (ax + 0.0001f)) * 0.785398f;       /* [0, π/4] */
                else
                    a = 1.5707963f - (ax / (ay + 0.0001f)) * 0.785398f; /* [π/4, π/2] */
                if (cy < 0) a = -a;
                if (cx < 0) a = 3.14159265f - a;
                angle = a;
            }

            int r = 1, g = 1, b = 5;

            /* 螺旋星臂 —— 角度现在覆盖完整 2π，螺旋可见 */
            float a1 = my_sin(angle - dist * 8.0f);
            float a2 = my_sin(angle - dist * 8.0f + 3.14159265f);
            /* 取正半波并平方，让星臂更锐利 */
            if (a1 < 0) a1 = 0; a1 *= a1;
            if (a2 < 0) a2 = 0; a2 *= a2;
            float arm = a1 > a2 ? a1 : a2;
            {float f = 1.0f - dist * 0.8f; if (f < 0) f = 0; arm *= f; }

            float n1 = fbm((float)x, (float)y);
            float n2 = fbm((float)x + 500, (float)y + 500);

            float core = 1 - dist * 2.5f; if (core < 0) core = 0; core *= core;

            /* 繁星 */
            if (hash2d((int)(x * 0.7f), (int)(y * 0.7f)) > 0.998f) {
                float sb = hash2d((int)(x * 3.1f), (int)(y * 3.1f));
                r = 180 + (int)(75 * sb); g = 180 + (int)(75 * sb); b = 200 + (int)(55 * sb);
            }
            else {
                r += (int)(180 * core + 120 * arm * n1);
                g += (int)(60 * core + 30 * arm * n1 + 60 * arm * n2);
                b += (int)(100 * core + 200 * arm * n2 + 40 * arm * n1);
            }
            px(buf, x, y, r, g, b);
        }
    }
}

#else

// nebula.c - 动画星云/银河
// 调用: background_nebula((char*)显存地址, 时间值);
//       每帧时间递增 0.016 左右(约60fps节奏，实际帧率取决于性能)

#include "../def.h"
#include "../video.h"

/* ---------- 渲染分辨率倍率 ----------
 * 1 = 原始分辨率（最慢，全屏fbm每帧可能数秒）
 * 2 = 半分辨率（快4倍，星云本身模糊，2x2块几乎看不出）
 * 3 = 1/3分辨率（快9倍，适合做动态壁纸）
 */
#define NEBULA_SCALE 2

static float my_sqrt(float x) {
    if (x <= 0)return 0; float s = x; int i;
    for (i = 0; i < 10; i++)s = (s + x / s) * 0.5f; return s;
}
static float my_sin(float x) {
    while (x > 3.14159265f)x -= 6.28318530f;
    while (x < -3.14159265f)x += 6.28318530f;
    float x2 = x * x; return x - x2 * x / 6.0f + x2 * x2 * x / 120.0f - x2 * x2 * x2 * x / 5040.0f;
}
static float my_fabs(float x) { return x < 0 ? -x : x; }
static unsigned int hash_u(unsigned int n) { n = (n ^ (n >> 13)) * 1274126177u; return n ^ (n >> 16); }
static float hash2d(int ix, int iy) {
    return(float)(hash_u((unsigned)(ix * 374761393 + iy * 668265263)) & 0xFFFF) / 65535.0f;
}
static float value_noise(float x, float y, float sc) {
    float sx = x / sc, sy = y / sc; int ix = (int)sx, iy = (int)sy;
    float fx = sx - ix, fy = sy - iy;
    fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy);
    return(hash2d(ix, iy) * (1 - fx) + hash2d(ix + 1, iy) * fx) * (1 - fy) +
        (hash2d(ix, iy + 1) * (1 - fx) + hash2d(ix + 1, iy + 1) * fx) * fy;
}
static float fbm(float x, float y) {
    float val = 0, amp = 0.5f; int i;
    for (i = 0; i < 5; i++) { val += amp * value_noise(x, y, 80.0f); x *= 2; y *= 2; amp *= 0.5f; }
    return val;
}
static void px(char* buf, int x, int y, int r, int g, int b) {
    if (r < 0)r = 0; if (r > 255)r = 255;
    if (g < 0)g = 0; if (g > 255)g = 255;
    if (b < 0)b = 0; if (b > 255)b = 255;
    int i = (y * gVideoWidth + x) * gBytesPerPixel;
    buf[i] = b; buf[i + 1] = g; buf[i + 2] = r;
}

extern "C" __declspec(dllexport) void background_nebula(char* buf, float t)
{
    int x, y;

    /* 气体漂移速度 —— 不同层用不同速度，产生视差 */
    float drift_x1 = t * 22.0f;
    float drift_y1 = t * 18.0f;
    float drift_x2 = t * 18.4f;
    float drift_y2 = t * 15.6f;

    /* 螺旋旋转速度 */
    float rot = t * 2.3f;

    /* 核心脉动 */
    float core_pulse = 0.85f + 0.15f * my_sin(t * 1.5f);

    for (y = 0; y < gVideoHeight; y += NEBULA_SCALE) {
        for (x = 0; x < gVideoWidth; x += NEBULA_SCALE) {

            float nx = (float)x / gVideoWidth;
            float ny = (float)y / gVideoHeight;
            float cx = (nx - 0.5f) * 2;
            float cy = (ny - 0.5f) * 2;
            float dist = my_sqrt(cx * cx + cy * cy);

            /* atan2 近似 */
            float angle;
            {
                float ax = my_fabs(cx), ay = my_fabs(cy);
                float a;
                if (ax > ay)
                    a = (ay / (ax + 0.0001f)) * 0.785398f;
                else
                    a = 1.5707963f - (ax / (ay + 0.0001f)) * 0.785398f;
                if (cy < 0) a = -a;
                if (cx < 0) a = 3.14159265f - a;
                angle = a;
            }

            int r = 1, g = 1, b = 5;

            /* ---- 螺旋星臂（旋转） ---- */
            float spiral = angle - dist * 8.0f + rot;
            float a1 = my_sin(spiral);
            float a2 = my_sin(spiral + 3.14159265f);
            if (a1 < 0) a1 = 0; a1 *= a1;
            if (a2 < 0) a2 = 0; a2 *= a2;
            float arm = a1 > a2 ? a1 : a2;
            {float f = 1.0f - dist * 0.8f; if (f < 0) f = 0; arm *= f; }

            /* 星臂亮度波 —— 从中心向外传播的亮度脉冲 */
            arm *= (0.88f + 0.12f * my_sin(t * 0.7f - dist * 4.0f));

            /* ---- 噪声（漂移流动） ---- */
            float n1 = fbm((float)x + drift_x1, (float)y + drift_y1);
            float n2 = fbm((float)x + 500.0f + drift_x2,
                (float)y + 500.0f + drift_y2);

            /* ---- 核心（脉动） ---- */
            float core = 1.0f - dist * 2.5f;
            if (core < 0) core = 0;
            core *= core * core_pulse;

            /* ---- 繁星（闪烁） ---- */
            float star_hash = hash2d((int)(x * 0.7f), (int)(y * 0.7f));
            if (star_hash > 0.998f) {
                float sb = hash2d((int)(x * 3.1f), (int)(y * 3.1f));
                /* 每颗星用不同频率和相位闪烁 */
                float twinkle = my_sin(t * (2.0f + sb * 6.0f) + sb * 200.0f)
                    * 0.35f + 0.65f;
                r = (int)((180 + 75 * sb) * twinkle);
                g = (int)((180 + 75 * sb) * twinkle);
                b = (int)((200 + 55 * sb) * twinkle);
            }
            else {
                r += (int)(180 * core + 120 * arm * n1);
                g += (int)(60 * core + 30 * arm * n1 + 60 * arm * n2);
                b += (int)(100 * core + 200 * arm * n2 + 40 * arm * n1);
            }

            /* 填充 NEBULA_SCALE × NEBULA_SCALE 块 */
            {
                int sx, sy;
                for (sy = 0; sy < NEBULA_SCALE && y + sy < gVideoHeight; sy++) {
                    for (sx = 0; sx < NEBULA_SCALE && x + sx < gVideoWidth; sx++) {
                        px(buf, x + sx, y + sy, r, g, b);
                    }
                }
            }
        }
    }
}


static float gNebulaTime = 0.0f;

extern "C" __declspec(dllexport) void nebulaLoop(void)
{
    while (1) {
        background_nebula((char*)gGraphBase, gNebulaTime);
        gNebulaTime += 0.16f;   // 每帧递增，实际帧率由渲染速度决定

        //__sleep(0);
        // 翻页 / vsync ...
    }
}



#endif