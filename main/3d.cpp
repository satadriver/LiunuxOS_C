

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

#pragma pack(1)
// 三维点结构
typedef struct {
    float x, y, z;
} Point3D;

// 二维点结构
typedef struct {
    int x, y;
} Point2D;

// 颜色结构
typedef struct {
    unsigned char r, g, b, a;
} Color;

#pragma pack()

// 深度缓冲
float* zbuffer = 0;

// 屏幕分辨率
//#define SCREEN_WIDTH 1024
//#define SCREEN_HEIGHT 768
//#define SCREEN_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT)

// 显存地址（假设使用线性帧缓冲）
//#define FRAMEBUFFER_ADDR 0xFD000000

// 帧缓冲指针（模拟显存）
//unsigned int* framebuffer = (unsigned int*)FRAMEBUFFER_ADDR;

// 球体参数
#define SPHERE_RADIUS       200
#define SPHERE_SEGMENTS     20
#define SPHERE_POINTS       ((SPHERE_SEGMENTS + 1) * (SPHERE_SEGMENTS + 1))

Point3D spherePoints[SPHERE_POINTS];
int sphereIndices[SPHERE_SEGMENTS * SPHERE_SEGMENTS * 6];


// 画点函数（直接写显存）
void drawDot(unsigned char* framebuffer,int x, int y, Color color) {
    if (x >= 0 && x < gVideoWidth && y >= 0 && y < gVideoHeight) {
        // 直接写显存（假设32位色：ARGB格式）
		int idx = y * gBytesPerLine + x * gBytesPerPixel;

        framebuffer[idx + 0] = color.b;
        framebuffer[idx + 1] = color.g;
        framebuffer[idx + 2] = color.b;     
    }
}

// 带深度测试的画点
void drawDotWithZ(unsigned char* framebuffer,int x, int y, float z, Color color) {
    if (x >= 0 && x < gVideoWidth && y >= 0 && y < gVideoHeight) {
        int idx = y * gBytesPerLine + x * gBytesPerPixel;
        if (z < zbuffer[idx]) {
            framebuffer[idx + 0] = color.b;
            framebuffer[idx + 1] = color.g;
            framebuffer[idx + 2] = color.b;

            zbuffer[idx/ gBytesPerPixel] = z;
        }
    }
}

// 清屏
void clearScreen(unsigned char* framebuffer) {
    int screenSize = gVideoHeight * gVideoWidth*gBytesPerPixel ;
    for (int i = 0; i < screenSize; i++) {
        framebuffer[i] = 0;  // 黑色
        zbuffer[i/ gBytesPerPixel] = 999999.0; // 重置深度缓冲
    }
}

// Bresenham画线算法
void drawLine(unsigned char* framebuffer,Point2D p1, Point2D p2, Color color) {
    int x1 = p1.x, y1 = p1.y;
    int x2 = p2.x, y2 = p2.y;

    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        drawDot(framebuffer,x1, y1, color);

        if (x1 == x2 && y1 == y2) break;

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

// 3D点投影到2D屏幕
Point2D projectPoint(Point3D p, float distance) {
    Point2D result;

    if (p.z + distance > 0) {
        float factor = distance / (p.z + distance);
        result.x = (int)(p.x * factor + gVideoWidth / 2);
        result.y = (int)(p.y * factor + gVideoHeight / 2);
    }
    else {
        result.x = gVideoWidth / 2;
        result.y = gVideoHeight / 2;
    }

    return result;
}

// 获取投影点的z值（用于深度测试）
float getProjectedZ(Point3D p, float distance) {
    return p.z;
}

// 旋转3D点
Point3D rotatePoint(Point3D p, float angleX, float angleY, float angleZ) {
    Point3D result;
    float tempX, tempY, tempZ;

    // Z轴旋转
    tempX = p.x * cos(angleZ) - p.y * sin(angleZ);
    tempY = p.x * sin(angleZ) + p.y * cos(angleZ);
    tempZ = p.z;

    // Y轴旋转
    float x1 = tempX * cos(angleY) + tempZ * sin(angleY);
    float z1 = -tempX * sin(angleY) + tempZ * cos(angleY);
    float y1 = tempY;

    // X轴旋转
    result.x = x1;
    result.y = y1 * cos(angleX) - z1 * sin(angleX);
    result.z = y1 * sin(angleX) + z1 * cos(angleX);

    return result;
}

// 生成球体顶点和索引
void generateSphere() {
    int i, j, index = 0;

    // 生成顶点
    for (i = 0; i <= SPHERE_SEGMENTS; i++) {
        float theta = i * PI / SPHERE_SEGMENTS; // 极角 (0 to PI)

        for (j = 0; j <= SPHERE_SEGMENTS; j++) {
            float phi = j * 2 * PI / SPHERE_SEGMENTS; // 方位角 (0 to 2PI)

            float x = SPHERE_RADIUS * sin(theta) * cos(phi);
            float y = SPHERE_RADIUS * sin(theta) * sin(phi);
            float z = SPHERE_RADIUS * cos(theta);

            spherePoints[index].x = x;
            spherePoints[index].y = y;
            spherePoints[index].z = z;
            index++;
            //spherePoints[index++] = (Point3D){ x, y, z };
        }
    }

    // 生成三角形索引
    index = 0;
    for (i = 0; i < SPHERE_SEGMENTS; i++) {
        for (j = 0; j < SPHERE_SEGMENTS; j++) {
            int p1 = i * (SPHERE_SEGMENTS + 1) + j;
            int p2 = p1 + 1;
            int p3 = (i + 1) * (SPHERE_SEGMENTS + 1) + j;
            int p4 = p3 + 1;

            // 两个三角形组成一个四边形
            sphereIndices[index++] = p1;
            sphereIndices[index++] = p2;
            sphereIndices[index++] = p3;

            sphereIndices[index++] = p2;
            sphereIndices[index++] = p4;
            sphereIndices[index++] = p3;
        }
    }
}

// 画三角形（线框模式）
void drawTriangle(unsigned char* framebuffer,Point2D p1, Point2D p2, Point2D p3, Color color) {
    drawLine(framebuffer,p1, p2, color);
    drawLine(framebuffer,p2, p3, color);
    drawLine(framebuffer,p3, p1, color);
}

// 画填充三角形（带深度测试）
void drawFilledTriangle(unsigned char* framebuffer,Point2D p1, Point2D p2, Point2D p3,
    float z1, float z2, float z3, Color color) {
    // 简单的扫描线填充算法
    // 这里为了简化，使用线框模式
    drawTriangle(framebuffer,p1, p2, p3, color);
}

// 创建颜色渐变
Color getColorByHeight(float y) {
    Color color;
    float t = (y / SPHERE_RADIUS + 1) / 2; // 映射到0-1

    color.r = (unsigned char)(255 * (1 - t));
    color.g = (unsigned char)(255 * t);
    color.b = (unsigned char)(255 * (0.5 + 0.5 * sin(t * PI)));
    color.a = 255;

    return color;
}

// 等待垂直同步（模拟）
void waitVSync() {
    // 在实际硬件中，这里需要读取显卡寄存器
    // 这里用简单延时模拟
    for (int i = 0; i < 100000; i++);
}

extern "C" __declspec(dllexport) int Rotate3DBall(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD runparam) {
   
    WINDOWCLASS window;
    __memset((char*)&window, 0, sizeof(WINDOWCLASS));
    __strcpy(window.caption, funcname);
    initFullWindow(&window, funcname, tid, 1);

    int i;
    float angleX = 0, angleY = 0, angleZ = 0;

	int screenSize = gVideoHeight * gVideoWidth;
    // 分配深度缓冲
    zbuffer = (float*)__kMalloc(screenSize * sizeof(float));
    if (!zbuffer) {
        printf("Failed to allocate Z-buffer\n");
        return 1;
    }

    // 生成球体数据
    generateSphere();

    printf("3D Animation Started (Press Ctrl+C to exit)\n");
    printf("Resolution: %dx%d\n", gVideoWidth, gVideoHeight);
    printf("Sphere radius: %d, Segments: %d\n", SPHERE_RADIUS, SPHERE_SEGMENTS);

    // 主循环
    while (1)
    {
        //unsigned int ck = __getchar(window.id);
        unsigned int ck = __kGetKbd(window.id);
        unsigned int asc = ck & 0xff;
        if (asc == 0x1b)
        {
            __DestroyWindow(&window);
            __kFree((DWORD)zbuffer);
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
                    __kFree((DWORD)zbuffer);
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
        //waitVSync();
        __sleep(0);
		PROCESS_INFO* process = (PROCESS_INFO*)GetCurrentTaskTssBase();
        unsigned char* framebuffer = (unsigned char*)process->videoBase;
        clearScreen(framebuffer);

        Point3D rotated[SPHERE_POINTS];
        Point2D projected[SPHERE_POINTS];
        float projectedZ[SPHERE_POINTS];

        // 旋转所有顶点
        for (i = 0; i < SPHERE_POINTS; i++) {
            rotated[i] = rotatePoint(spherePoints[i], angleX, angleY, angleZ);
            projected[i] = projectPoint(rotated[i], 800);
            projectedZ[i] = getProjectedZ(rotated[i], 800);
        }

        // 绘制球体
        for (i = 0; i < SPHERE_SEGMENTS * SPHERE_SEGMENTS * 6; i += 3) {
            int idx1 = sphereIndices[i];
            int idx2 = sphereIndices[i + 1];
            int idx3 = sphereIndices[i + 2];

            // 简单的背面剔除
            Point3D v1 = rotated[idx1];
            Point3D v2 = rotated[idx2];
            Point3D v3 = rotated[idx3];

            // 计算法线
            float nx = (v2.y - v1.y) * (v3.z - v1.z) - (v2.z - v1.z) * (v3.y - v1.y);
            float ny = (v2.z - v1.z) * (v3.x - v1.x) - (v2.x - v1.x) * (v3.z - v1.z);
            float nz = (v2.x - v1.x) * (v3.y - v1.y) - (v2.y - v1.y) * (v3.x - v1.x);

            // 视线向量
            float viewZ = -1;

            // 背面剔除（点积大于0表示面向摄像机）
            if (nx * 0 + ny * 0 + nz * viewZ < 0) {
                Color color = getColorByHeight(rotated[idx1].y);
                drawFilledTriangle(framebuffer,projected[idx1], projected[idx2], projected[idx3],
                    projectedZ[idx1], projectedZ[idx2], projectedZ[idx3],
                    color);
            }
        }

        // 添加一些星空效果
        Color starColor = { 255, 255, 255, 255 };
        for (i = 0; i < 100; i++) {
            int x = __random(0) % gVideoWidth;
            int y = __random(0) % gVideoHeight;
            if (__random(0) % 100 < 5) { // 5%的星星闪R
                drawDot(framebuffer,x, y, starColor);
            }
        }

        // 更新旋转角度
        angleY += 0.02;  // Y轴旋转
        angleX += 0.01;  // X轴旋转
        angleZ += 0.005; // Z轴旋转
    }

    __kFree((DWORD)zbuffer);
    return 0;
}