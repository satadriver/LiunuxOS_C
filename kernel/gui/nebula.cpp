// nebula.c - 星云/银河
// 依赖: 无
// 调用: background_nebula((char*)显存地址);

#include "../def.h"
#include "../video.h"

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