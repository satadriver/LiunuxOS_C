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

// 数学常量
#define PI 3.14159265358979323846f

#define MAX_COORDINATE   2.0f
#define MIN_COORDINATE   0.5f
     

// 3D点结构
typedef struct {
    float x, y, z;
} point3d_t;

// 2D投影点结构
typedef struct {
    int x, y;
    int valid;  // 新增：点是否在视锥内
} point2d_t;

// 8个立方体顶点
static point3d_t cube_vertices[] = {
    {-1.0f, -1.0f, -1.0f},  // 0: 前下左
    { 1.0f, -1.0f, -1.0f},  // 1: 前下右
    { 1.0f,  1.0f, -1.0f},  // 2: 前上右
    {-1.0f,  1.0f, -1.0f},  // 3: 前上左
    {-1.0f, -1.0f,  1.0f},  // 4: 后下左
    { 1.0f, -1.0f,  1.0f},  // 5: 后下右
    { 1.0f,  1.0f,  1.0f},  // 6: 后上右
    {-1.0f,  1.0f,  1.0f}   // 7: 后上左
};


static point3d_t cube_vertices2[] = {
    {-2.0f, -2.0f, -2.0f},  // 0: 前下左
    { 2.0f, -2.0f, -2.0f},  // 1: 前下右
    { 2.0f,  2.0f, -2.0f},  // 2: 前上右
    {-2.0f,  2.0f, -2.0f},  // 3: 前上左
    {-2.0f, -2.0f,  2.0f},  // 4: 后下左
    { 2.0f, -2.0f,  2.0f},  // 5: 后下右
    { 2.0f,  2.0f,  2.0f},  // 6: 后上右
    {-2.0f,  2.0f,  2.0f}   // 7: 后上左
};

/*
        后上左(7)--------后上右(6)
        /|              /|
       / |             / |
  前上左(3)--------前上右(2)
      |  |           |  |
      | 后下左(4)----|--后下右(5)
      | /            | /
      |/             |/
  前下左(0)--------前下右(1)
*/
// 12条棱的索引
static const int g_edges[12][2] = {
    {0,1}, {1,2}, {2,3}, {3,0},  // 前面4条边
    {4,5}, {5,6}, {6,7}, {7,4},  // 后面4条边
    {0,4}, {1,5}, {2,6}, {3,7}   // 连接前后的4条边
};

// 颜色表
static const uint32_t g_colors[] = {
    0xFF0000, 0x00FF00, 0x0000FF, 0xFFFF00,
    0xFF00FF, 0x00FFFF, 0xFF8800, 0x88FF00,
    0xFF0088, 0x0088FF, 0x88FF88, 0xFF88FF
};

// 全局变量
static float g_rotation_x = 0.0f;
static float g_rotation_y = 0.0f;
static float g_rotation_z = 0.0f;
static point2d_t g_project[8];

float g_x_speed = 1.0;
float g_y_speed = 1.0;
float g_z_speed = 1.0;


static float g_rotation_x2 = 0.0f;
static float g_rotation_y2 = 0.0f;
static float g_rotation_z2 = 0.0f;
static point2d_t g_project2[8];
float g_x_speed2 = 1.0;
float g_y_speed2 = 1.0;
float g_z_speed2 = 1.0;

float* g_zbuffer = NULL;

// 改进的正弦函数（使用更多项，精度更高）
float mysin(float x) {
    // 角度归一化到 [-PI, PI]
    while (x > PI) x -= 2 * PI;
    while (x < -PI) x += 2 * PI;

    float result = 0.0f;
    float term = x;

    // 使用更多项提高精度
    result = term;
    term = term * x * x / 6.0f;
    result -= term;
    term = term * x * x / 20.0f;
    result += term;
    term = term * x * x / 42.0f;
    result -= term;
    term = term * x * x / 72.0f;
    result += term;
    term = term * x * x / 110.0f;
    result -= term;

    return result;
}

float mycos(float x) {
    // 角度归一化
    while (x > PI) x -= 2 * PI;
    while (x < -PI) x += 2 * PI;

    float result = 0.0f;
    float term = 1.0f;

    result = term;
    term = term * x * x / 2.0f;
    result -= term;
    term = term * x * x / 12.0f;
    result += term;
    term = term * x * x / 30.0f;
    result -= term;
    term = term * x * x / 56.0f;
    result += term;
    term = term * x * x / 90.0f;
    result -= term;

    return result;
}

// 画点函数（直接写显存）
void draw_pixel(int x, int y, uint32_t color) {
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
// 3D旋转变换（保持不变）
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

// 改进的透视投影（带视锥体裁剪）
void project_point_old(point3d_t* p, point2d_t* out) {
    float scale = 400.0f;
    float near_plane = 1.0f;    // 近裁剪平面
    float far_plane = 100.0f;    // 远裁剪平面
    float distance = 5.0f;       // 视距

    // 检查点是否在视锥体内
    // 如果z值太负（在相机后面）或太远，标记为无效
    if (p->z <= -near_plane || p->z >= far_plane) {
        out->valid = 0;
        out->x = 0;
        out->y = 0;
        return;
    }

    // 安全计算透视投影
    float denominator = distance + p->z;
    if (denominator <= 0.001f) {
        // 点太靠近相机或在相机后面
        out->valid = 0;
        out->x = 0;
        out->y = 0;
        return;
    }

    float factor = scale / denominator;

    out->x = gVideoWidth / 2 + (int)(p->x * factor);
    out->y = gVideoHeight / 2 - (int)(p->y * factor);
    out->valid = 1;
}

// 修改 project_point 函数，使用更宽松的裁剪
void project_point(point3d_t* p, point2d_t* out) {
    float scale = 80.0;
    float distance = 5.0f;  // 增加距离，让立方体远离相机
    float near_plane = 1.0f;  // 更宽松的近平面

    // 扩大有效范围，只剔除真正在相机后面的点
    if (p->z < -distance + 1.0) {
        out->valid = 0;
        out->x = 0;
        out->y = 0;
        return;
    }

	int pos = p->y * gVideoWidth + p->x;
    if (g_zbuffer != NULL && p->z > g_zbuffer[pos]) {
        g_zbuffer[pos] = p->z;  // 更新Z-buffer
    }

    // 安全计算
    float denominator = distance + p->z;
    if (denominator < 1.0) {
        // 对于非常接近相机的点，使用一个小的正值
        denominator = 1.0;
    }

    float factor = scale * distance / denominator;

    out->x = gVideoWidth / 2 + (int)(p->x * factor);
    out->y = gVideoHeight / 2 - (int)(p->y * factor);
    out->valid = 1;  // 总是标记为有效
}

// 画线函数（改进：只画两个点都有效的线段）
void draw_line_clipped(int x0, int y0, int x1, int y1, uint32_t color, int valid0, int valid1) {
    // 只有当两个端点都有效时才画线
    if (valid0 && valid1) {
        draw_line(x0, y0, x1, y1, color);
    }
}



// 主渲染函数（改进版）
void render_frame(void) {
    point3d_t rotated[8];
    int i;

    // 更新旋转角度（减小增量，使运动更平滑）
    g_rotation_x += g_x_speed;  // 0.015f;
    g_rotation_y += g_y_speed;  // 0.02f;
    g_rotation_z += g_z_speed;  // 0.01f;
    //g_rotation_x += 0.015f;
    //g_rotation_y += 0.02f;
    //g_rotation_z += 0.01f;

    // 归一化角度到0-2PI
    if (g_rotation_x > 2 * PI) {
        //g_rotation_x -= 2 * PI;
		g_x_speed = -g_x_speed;  // 反转X轴旋转方向
    }
    if (g_rotation_y > 2 * PI) {
        //g_rotation_y -= 2 * PI;
        g_y_speed = -g_y_speed;  // 反转Y轴旋转方向
    }
    if (g_rotation_z > 2 * PI) {
        //g_rotation_z -= 2 * PI;
        g_z_speed = -g_z_speed;  // 反转Z轴旋转方向
    }

    if (g_rotation_x < 0)
    {
        //g_rotation_x += 2 * PI;
        g_x_speed = -g_x_speed;  // 反转X轴旋转方向
    }
    if (g_rotation_y < 0) {
        //g_rotation_y += 2 * PI;
        g_y_speed = -g_y_speed;  // 反转Y轴旋转方向
    }
    if (g_rotation_z < 0) {
        //g_rotation_z += 2 * PI;
        g_z_speed = -g_z_speed;  // 反转Z轴旋转方向
    }

    // 旋转所有顶点并投影
    for (i = 0; i < 8; i++) {
        rotate_point((point3d_t*)&cube_vertices[i], g_rotation_x, g_rotation_y, g_rotation_z, &rotated[i]);
        project_point(&rotated[i], &g_project[i]);
    }

    // 绘制所有边（只绘制有效线段）
    for (i = 0; i < 12; i++) {
        int idx1 = g_edges[i][0];
        int idx2 = g_edges[i][1];
        draw_line_clipped(
            g_project[idx1].x, g_project[idx1].y,
            g_project[idx2].x, g_project[idx2].y,
            g_colors[i % 12],
            g_project[idx1].valid,
            g_project[idx2].valid
        );
    }
}

// 主渲染函数（改进版）
void render_frame2(void) {
    point3d_t rotated[8];
    int i;

    // 更新旋转角度（减小增量，使运动更平滑）
    //g_rotation_x += g_x_speed;  // 0.015f;
    //g_rotation_y += g_y_speed;  // 0.02f;
    //g_rotation_z += g_z_speed;  // 0.01f;
    g_rotation_x2 += 0.015f;
    g_rotation_y2 += 0.02f;
    g_rotation_z2 += 0.01f;

    // 归一化角度到0-2PI
    if (g_rotation_x2 > 2 * PI) {
        //g_rotation_x -= 2 * PI;
        g_x_speed2 = -g_x_speed2;  // 反转X轴旋转方向
    }
    if (g_rotation_y2 > 2 * PI) {
        //g_rotation_y -= 2 * PI;
        g_y_speed2 = -g_y_speed2;  // 反转Y轴旋转方向
    }
    if (g_rotation_z2 > 2 * PI) {
        //g_rotation_z -= 2 * PI;
        g_z_speed2 = -g_z_speed2;  // 反转Z轴旋转方向
    }

    if (g_rotation_x2 < 0)
    {
        //g_rotation_x += 2 * PI;
        g_x_speed2 = -g_x_speed2;  // 反转X轴旋转方向
    }
    if (g_rotation_y2 < 0) {
        //g_rotation_y += 2 * PI;
        g_y_speed2 = -g_y_speed2;  // 反转Y轴旋转方向
    }
    if (g_rotation_z2 < 0) {
        //g_rotation_z += 2 * PI;
        g_z_speed2 = -g_z_speed2;  // 反转Z轴旋转方向
    }

    // 旋转所有顶点并投影
    for (i = 0; i < 8; i++) {
        rotate_point((point3d_t*)&cube_vertices2[i], g_rotation_x2, g_rotation_y2, g_rotation_z2, &rotated[i]);
        project_point(&rotated[i], &g_project2[i]);
    }

    // 绘制所有边（只绘制有效线段）
    for (i = 0; i < 12; i++) {
        int idx1 = g_edges[i][0];
        int idx2 = g_edges[i][1];
        draw_line_clipped(
            g_project2[idx1].x, g_project2[idx1].y,
            g_project2[idx2].x, g_project2[idx2].y,
            g_colors[i % 12],
            //0xffffffff,
            g_project2[idx1].valid,
            g_project2[idx2].valid
        );
    }
}

// 可选：添加深度测试和背面剔除
int is_backface(int v0, int v1, int v2, point3d_t* vertices) {
    // 计算面法向量
    float ax = vertices[v1].x - vertices[v0].x;
    float ay = vertices[v1].y - vertices[v0].y;
    float az = vertices[v1].z - vertices[v0].z;

    float bx = vertices[v2].x - vertices[v0].x;
    float by = vertices[v2].y - vertices[v0].y;
    float bz = vertices[v2].z - vertices[v0].z;

    // 法向量
    float nx = ay * bz - az * by;
    float ny = az * bx - ax * bz;
    float nz = ax * by - ay * bx;

    // 视线向量（假设相机在(0,0,-distance)）
    float vx = 0 - vertices[v0].x;
    float vy = 0 - vertices[v0].y;
    float vz = -5 - vertices[v0].z;

    // 点积：背面如果 > 0
    return (nx * vx + ny * vy + nz * vz) > 0;
}

// 或者使用更简单的解决方案：增加距离值
void project_point_simple(point3d_t* p, point2d_t* out) {
    float scale = 400.0f;
    float distance = 8.0f;  // 增加视距，让立方体远离相机

    // 限制z值范围，避免除零
    float z_clamped = p->z;
    if (z_clamped <= -distance + 0.1f) {
        z_clamped = -distance + 0.1f;
    }

    float factor = scale / (distance + z_clamped);
    out->x = gVideoWidth / 2 + (int)(p->x * factor);
    out->y = gVideoHeight / 2 - (int)(p->y * factor);
    out->valid = 1;
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
        for (int j = 0; j < gBytesPerPixel; j++) {
            unsigned char c = (cc) & 0xFF;
            if (video_mem[i * gBytesPerPixel + j] != c) {
                video_mem[i * gBytesPerPixel + j] = c;
            }
            
            cc >>= 8;
        }

        g_zbuffer[i] = -99999.0;
    }

}

// 主函数
extern "C" __declspec(dllexport) int Rotate3DCube(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD runparam) {
    WINDOWCLASS window;
    __memset((char*)&window, 0, sizeof(WINDOWCLASS));
    __strcpy(window.caption, funcname);
    initFullWindow(&window, funcname, tid, 1);

    // 清屏为黑色
    //clear_screen(0x000000);

    const point3d_t std_vertices[8] = {
    {-1.0f, -1.0f, -1.0f},  // 0: 前下左
    { 1.0f, -1.0f, -1.0f},  // 1: 前下右
    { 1.0f,  1.0f, -1.0f},  // 2: 前上右
    {-1.0f,  1.0f, -1.0f},  // 3: 前上左
    {-1.0f, -1.0f,  1.0f},  // 4: 后下左
    { 1.0f, -1.0f,  1.0f},  // 5: 后下右
    { 1.0f,  1.0f,  1.0f},  // 6: 后上右
    {-1.0f,  1.0f,  1.0f}   // 7: 后上左
    };
	__memcpy((char*)cube_vertices, (char*)std_vertices, sizeof(std_vertices));

    int frameCnt = 0;
	float direction = 1.0;  // 1: 正向，-1: 反向

	g_zbuffer = (float*)malloc(gVideoWidth * gVideoHeight * sizeof(float));

    // 主循环
    while (1) {
        unsigned int ck = __kGetKbd(window.id);
        unsigned int asc = ck & 0xff;
        if (asc == 0x1b) {
            __DestroyWindow(&window);
            free(g_zbuffer);
            return 0;
        }

        MOUSEINFO mouseinfo;
        __memset((char*)&mouseinfo, 0, sizeof(MOUSEINFO));
        int ret = __kGetMouse(&mouseinfo, window.id);

        if (mouseinfo.status & 1) {  // left click
            if (mouseinfo.x >= window.shutdownx && mouseinfo.x <= window.shutdownx + window.capHeight &&
                mouseinfo.y >= window.shutdowny && mouseinfo.y <= window.shutdowny + window.capHeight) {
                __DestroyWindow(&window);
                free(g_zbuffer);
                return 0;
            }
            if (mouseinfo.x >= window.minx && mouseinfo.x <= window.minx + window.capHeight &&
                mouseinfo.y >= window.miny && mouseinfo.y <= window.miny + window.capHeight) {
                MinimizeWindow(&window);
            }
        }

        for(int i = 0;i < 8; i++) {
			float delta = 1.0f;
            if (cube_vertices[i].x * delta > 0) {
                cube_vertices[i].x += direction * 0.01;
            }
            else {
                cube_vertices[i].x -= direction * 0.01;
            }
            
            if (cube_vertices[i].y * delta > 0) {
                cube_vertices[i].y += direction * 0.01;
            }
            else {
                cube_vertices[i].y -= direction * 0.01;
            }

            if (cube_vertices[i].z * delta > 0) {
                cube_vertices[i].z += direction * 0.01;
            }
            else {
                cube_vertices[i].z -= direction * 0.01;
            }  
		}
        if(cube_vertices[0].x <= -MAX_COORDINATE || cube_vertices[0].x >= - MIN_COORDINATE) {
            direction = -direction;
		}
        
        if (frameCnt % 1024 == 0) {
            DWORD tmp = __random(0) % 100;
            g_x_speed = ((double)tmp);
            g_x_speed = g_x_speed / 1000.0;
            if (g_x_speed < 0.005) {
                g_x_speed = 0.005;
            }
            if (tmp % 2 == 0) {
                //g_x_speed = -g_x_speed;
            }

            tmp = __random(0) % 100;
            g_y_speed = ((double)tmp);
            g_y_speed = g_y_speed / 1000.0;
            if (g_y_speed < 0.005) {
                g_y_speed = 0.005;
            }
            if (tmp % 2 == 0) {
                //g_y_speed = -g_y_speed;
            }

            tmp = __random(0) % 100;
            g_z_speed = ((double)tmp);
            g_z_speed = g_z_speed / 1000.0;
            if (g_z_speed < 0.005) {
                g_z_speed = 0.005;
            }
            if (tmp % 2 == 0) {
                //g_z_speed = -g_z_speed;
            }
        }
        frameCnt++;

        // 清屏并渲染
        clear_screen(0x000000);

        render_frame();

        //render_frame2();

        // 简单的帧率控制
        __sleep(0);
    }

    return 0;
}