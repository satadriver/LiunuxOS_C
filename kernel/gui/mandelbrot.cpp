// mandelbrot.c - 分形（修正版）
// 依赖: 无
// 调用: background_mandelbrot((char*)显存地址);
#define WIDTH  1024
#define HEIGHT 768
#define MAX_ITER 80

static void set_pixel(char *buf, int x, int y, int r, int g, int b) {
    if (r < 0) r = 0; if (r > 255) r = 255;
    if (g < 0) g = 0; if (g > 255) g = 255;
    if (b < 0) b = 0; if (b > 255) b = 255;
    int idx = (y * WIDTH + x) * 4;
    buf[idx]   = (char)b;
    buf[idx+1] = (char)g;
    buf[idx+2] = (char)r;
    buf[idx+3] = 0xFF;
}

void background_mandelbrot(char *buf) {
    int x, y;
    float x0_min = -2.2f, x0_max = 0.8f;
    float y0_min = -1.2f, y0_max = 1.2f;
    float dx = (x0_max - x0_min) / WIDTH;
    float dy = (y0_max - y0_min) / HEIGHT;

    for (y = 0; y < HEIGHT; y++) {
        for (x = 0; x < WIDTH; x++) {
            float cx = x0_min + x * dx;
            float cy = y0_min + y * dy;
            float zx = 0.0f, zy = 0.0f;
            int i = 0;
            while (zx * zx + zy * zy < 4.0f && i < MAX_ITER) {
                float tmp = zx * zx - zy * zy + cx;
                zy = 2.0f * zx * zy + cy;
                zx = tmp;
                i++;
            }
            int r = 0, g = 0, b = 0;
            if (i < MAX_ITER) {
                float t = (float)i / MAX_ITER;
                float t1 = 1.0f - t;
                r = (int)(9.0f * t1 * t * t * t * 255.0f);
                g = (int)(15.0f * t1 * t1 * t * t * 255.0f);
                b = (int)(8.5f * t1 * t1 * t1 * t * 255.0f);
                if (i > MAX_ITER * 7 / 10) {
                    float glow = ((float)i - MAX_ITER * 0.7f) / (MAX_ITER * 0.3f);
                    r += (int)(20 * glow);
                    g += (int)(40 * glow);
                    b += (int)(100 * glow);
                }
            }
            set_pixel(buf, x, y, r, g, b);
        }
    }
}