// wallpaper.c - 32-bit framebuffer wallpaper generator
// Compile: gcc -o wallpaper wallpaper.c -lm
// Linux:   ./wallpaper /dev/fb0
// 自研OS:  直接调用 background((char*)0xB8000) 或你的显存地址

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============ 核心函数 ============
void background(char *buf)
{
    int x, y;
    for (y = 0; y < HEIGHT; y++) {
        for (x = 0; x < WIDTH; x++) {
            float nx = (float)x / (float)WIDTH;
            float ny = (float)y / (float)HEIGHT;
            float cx = (nx - 0.5f) * 2.0f;
            float cy = (ny - 0.5f) * 2.0f;
            float dist;

            // 计算到中心距离
            {
                float dx = cx, dy = cy;
                // 用整数运算近似 sqrt，避免链接 -lm
                // 或者直接用: dist = sqrtf(cx*cx + cy*cy);
                int i;
                float s = cx * cx + cy * cy;
                dist = s;
                for (i = 0; i < 8; i++) {
                    dist = (dist + s / dist) * 0.5f;
                }
                // 此时 dist ≈ sqrt(s)
            }

            /* Layer 1: 深蓝紫渐变背景 */
            int r = (int)(10 + 20 * ny);
            int g = (int)(10 + 15 * ny);
            int b = (int)(40 + 60 * (1.0f - ny));

            /* Layer 2: 中心径向发光 */
            {
                float glow = 1.0f - dist * 1.2f;
                if (glow > 0.0f) {
                    r += (int)(30 * glow);
                    g += (int)(20 * glow);
                    b += (int)(80 * glow);
                }
            }

            /* Layer 3: 对角线光带 */
            {
                float diag = (cx + cy) * 0.5f;
                float d = diag - 0.3f;
                if (d < 0) d = -d;
                float streak = 1.0f - d * 4.0f;
                if (streak > 0.0f) {
                    streak *= 0.3f;
                    r += (int)(60 * streak);
                    g += (int)(40 * streak);
                    b += (int)(120 * streak);
                }
            }

            /* Layer 4: 同心圆环 */
            {
                float t = dist * 12.0f - ny * 3.0f;
                float ring;
                // 简单的 sin 近似（Taylor 展开）
                ring = t;
                {
                    float t2 = t * t;
                    ring = t - t2 * t / 6.0f + t2 * t2 * t / 120.0f;
                }
                if (ring < 0) ring = -ring;
                ring *= 0.15f;
                r += (int)(40 * ring);
                g += (int)(50 * ring);
                b += (int)(100 * ring);
            }

            /* Layer 5: 暗角（边缘变暗） */
            {
                float vignette = 1.0f - dist * 0.6f;
                if (vignette < 0.0f) vignette = 0.0f;
                if (vignette > 1.0f) vignette = 1.0f;
                r = (int)(r * vignette);
                g = (int)(g * vignette);
                b = (int)(b * vignette);
            }

            /* 颜色限幅 */
            if (r < 0) r = 0; if (r > 255) r = 255;
            if (g < 0) g = 0; if (g > 255) g = 255;
            if (b < 0) b = 0; if (b > 255) b = 255;

            /* 写入显存（BGRA 32-bit，适配大多数 framebuffer） */
            int idx = (y * WIDTH + x) * 4;
            buf[idx + 0] = (char)b;       /* Blue  */
            buf[idx + 1] = (char)g;       /* Green */
            buf[idx + 2] = (char)r;       /* Red   */
            buf[idx + 3] = (char)0xFF;    /* Alpha */
        }
    }
}