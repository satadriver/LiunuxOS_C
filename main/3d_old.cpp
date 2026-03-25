

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


// 数学常量
#define PI 3.14159265358979323846

// 3D点结构
typedef struct {
    float x, y, z;
} point3d_t;

// 2D投影点结构
typedef struct {
    int x, y;
} point2d_t;

// 8个立方体顶点
static const point3d_t cube_vertices[] = {
    {-1.0f, -1.0f, -1.0f},  // 0: 前下左
    { 1.0f, -1.0f, -1.0f},  // 1: 前下右
    { 1.0f,  1.0f, -1.0f},  // 2: 前上右
    {-1.0f,  1.0f, -1.0f},  // 3: 前上左
    {-1.0f, -1.0f,  1.0f},  // 4: 后下左
    { 1.0f, -1.0f,  1.0f},  // 5: 后下右
    { 1.0f,  1.0f,  1.0f},  // 6: 后上右
    {-1.0f,  1.0f,  1.0f}   // 7: 后上左
};

// 12条棱的索引（每两个顶点一条线）
static const int edges[12][2] = {
    {0,1}, {1,2}, {2,3}, {3,0},  // 前面4条边
    {4,5}, {5,6}, {6,7}, {7,4},  // 后面4条边
    {0,4}, {1,5}, {2,6}, {3,7}   // 连接前后的4条边
};

// 颜色表（RGB 24位）
static const uint32_t colors[] = {
    0xFF0000,  // 红色
    0x00FF00,  // 绿色
    0x0000FF,  // 蓝色
    0xFFFF00,  // 黄色
    0xFF00FF,  // 紫色
    0x00FFFF,  // 青色
    0xFF8800,  // 橙色
    0x88FF00,  // 黄绿
    0xFF0088,  // 粉红
    0x0088FF,  // 天蓝
    0x88FF88,  // 浅绿
    0xFF88FF   // 浅紫
};

// 全局变量
static float rotation_x = 0.0f;
static float rotation_y = 0.0f;
static float rotation_z = 0.0f;
static point2d_t projected[8];

// 数学函数实现（因为裸机环境可能没有标准库）
float mysin(float x) {
    float result = 0.0f;
    float term = x;
    int sign = 1;

    // 泰勒级数展开 sin(x) = x - x^3/6 + x^5/120 - x^7/5040 + ...
    result = term;
    term = term * x * x / 6.0f;
    result -= term;
    term = term * x * x / 20.0f;
    result += term;
    term = term * x * x / 42.0f;
    result -= term;

    return result;
}

float mycos(float x) {
    float result = 0.0f;
    float term = 1.0f;
    int sign = 1;

    // 泰勒级数展开 cos(x) = 1 - x^2/2 + x^4/24 - x^6/720 + ...
    result = term;
    term = term * x * x / 2.0f;
    result -= term;
    term = term * x * x / 12.0f;
    result += term;
    term = term * x * x / 30.0f;
    result -= term;

    return result;
}

// 3D旋转变换
void rotate_point(point3d_t* p, float rx, float ry, float rz, point3d_t* out) {
    float x = p->x;
    float y = p->y;
    float z = p->z;
    float sx = mysin(rx);
    float cx = mycos(rx);
    float sy = mysin(ry);
    float cy = mycos(ry);
    float sz = mysin(rz);
    float cz = mycos(rz);
    float x1, y1, z1;

    // 绕X轴旋转
    y1 = y * cx - z * sx;
    z1 = y * sx + z * cx;
    y = y1;
    z = z1;

    // 绕Y轴旋转
    x1 = x * cy + z * sy;
    z1 = -x * sy + z * cy;
    x = x1;
    z = z1;

    // 绕Z轴旋转
    x1 = x * cz - y * sz;
    y1 = x * sz + y * cz;
    x = x1;
    y = y1;

    out->x = x;
    out->y = y;
    out->z = z;
}

// 3D到2D透视投影
void project_point(point3d_t* p, point2d_t* out) {

    float scale = 400.0f;  // 投影缩放系数
    float distance = 5.0f;  // 视距

    if (p->z <= -distance) {
        p->z = -distance + 1.0; // 避免z过大导致factor为负
    }

    if(p->z >= distance) {
        p->z = distance - 1.0; // 避免z过小导致factor过大
	}

    // 透视投影公式
    float factor = scale / (distance + p->z);

    out->x = gVideoWidth / 2 + (int)(p->x * factor);
    out->y = gVideoHeight / 2 - (int)(p->y * factor);
}

// 画点函数（直接写显存）
void draw_pixel(int x, int y, uint32_t color) {
    PROCESS_INFO* process = (PROCESS_INFO*)GetCurrentTaskTssBase();
    unsigned char* framebuffer = (unsigned char*)process->videoBase;

    uint8_t* video_mem = (uint8_t*)framebuffer;
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

// Bresenham画线算法
void draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = __abs(x1 - x0);
    int dy = __abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    int e2;

    while (1) {
        draw_pixel(x0, y0, color);

        if (x0 == x1 && y0 == y1) break;

        e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

// 清屏
void clear_screen(uint32_t color) {
    PROCESS_INFO* process = (PROCESS_INFO*)GetCurrentTaskTssBase();
    unsigned char* framebuffer = (unsigned char*)process->videoBase;

    uint8_t* video_mem = (uint8_t*)framebuffer;
    int total_pixels = gVideoHeight * gVideoWidth;
    int i;

    for (i = 0; i < total_pixels; i++) {
        DWORD cc = color;
        for(int j = 0; j < gBytesPerPixel; j++) {
            video_mem[i * gBytesPerPixel + j] = (cc) & 0xFF;
            cc >>= 8;
		}
    }
}



// 主渲染函数
void render_frame(void) {
    point3d_t rotated[8];
    int i;

    // 更新旋转角度（每帧增加一点）
    rotation_x += 0.02f;
    rotation_y += 0.03f;
    rotation_z += 0.01f;

    // 归一化角度到0-2PI
    if (rotation_x > 2 * PI) rotation_x -= 2 * PI;
    if (rotation_y > 2 * PI) rotation_y -= 2 * PI;
    if (rotation_z > 2 * PI) rotation_z -= 2 * PI;

    // 旋转所有顶点并投影
    for (i = 0; i < 8; i++) {
        rotate_point((point3d_t*)&cube_vertices[i], rotation_x, rotation_y, rotation_z, &rotated[i]);
        project_point(&rotated[i], &projected[i]);
    }

    // 绘制所有边
    for (i = 0; i < 12; i++) {
        int idx1 = edges[i][0];
        int idx2 = edges[i][1];
        draw_line(projected[idx1].x, projected[idx1].y,
            projected[idx2].x, projected[idx2].y,
            colors[i % 12]);
    }
}





extern "C" __declspec(dllexport) int Rotate3DBall_old(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD runparam) {
   
    WINDOWCLASS window;
    __memset((char*)&window, 0, sizeof(WINDOWCLASS));
    __strcpy(window.caption, funcname);
    initFullWindow(&window, funcname, tid, 1);

    int frame = 0;

    // 初始化显存（可能需要先设置VESA模式）
    // 注意：在实际裸机环境中，需要先通过VESA BIOS调用设置视频模式

    // 清屏为黑色
    clear_screen(0x000000);

    // 主循环
    while (1)
    {
        //unsigned int ck = __getchar(window.id);
        unsigned int ck = __kGetKbd(window.id);
        unsigned int asc = ck & 0xff;
        if (asc == 0x1b)
        {
            __DestroyWindow(&window);

            return 0;
        }

        MOUSEINFO mouseinfo;
        __memset((char*)&mouseinfo, 0, sizeof(MOUSEINFO));
        //retvalue = getmouse(&mouseinfo,window.id);
        int ret = __kGetMouse(&mouseinfo, window.id);
        if (mouseinfo.status & 1)	//left click
        {
            if (mouseinfo.x >= window.shutdownx && mouseinfo.x <= window.shutdownx + window.capHeight)
            {
                if (mouseinfo.y >= window.shutdowny && mouseinfo.y <= window.shutdowny + window.capHeight)
                {
                    __DestroyWindow(&window);

                    return 0;
                }
            }
            if (mouseinfo.x >= window.minx && mouseinfo.x <= window.minx + window.capHeight)
            {
                if (mouseinfo.y >= window.miny && mouseinfo.y <= window.miny + window.capHeight)
                {
                    MinimizeWindow(&window);
                }
            }
        }

        // 清屏（只清除绘制区域，或者使用双缓冲）
        clear_screen(0x000000);

        // 渲染当前帧
        render_frame();

        // 简单的帧率控制（约60 FPS）
        __sleep(0);

        frame++;
    }


    return 0;
}
