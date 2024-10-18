#include "screenProtect.h"
#include "video.h"
#include "Utils.h"
#include "file.h"
#include "window.h"
#include "keyboard.h"
#include "mouse.h"
#include "gdi.h"
#include "malloc.h"
#include "page.h"
#include "servicesProc.h"
#include "cmosExactTimer.h"
#include "hardware.h"
#include "device.h"
#include "math.h"
#include "timer8254.h"

#define SCREENPROTECT_BACKGROUND_COLOR 0		//0XBBFFFF		0X87CEEB

//#define USE_CMOS_EXACT_TIMER

#define OUTPUT_INFO_COLOR	0Xff0000





int gCircleColor = 0xffffff;
int gCircleCenterX = 0;
int gCircleCenterY = 0;
int gRadius = 64;
int gDeltaX = 3;
int gDeltaY = 3;

int gScreenProtectWindowID = 0;

int gTimerID = 0;


int initScreenProtect() {
	int ret = 0;

	unsigned int r = __random(0);

	gCircleCenterX = r % gVideoWidth;
	if (gCircleCenterX + gRadius >= gVideoWidth)
	{
		gCircleCenterX = gVideoWidth - gRadius;
	}
	else if (gCircleCenterX <= gRadius)
	{
		gCircleCenterX = gRadius;
	}

	gCircleCenterY = r % gVideoHeight;
	if (gCircleCenterY + gRadius >= gVideoHeight)
	{
		gCircleCenterY = gVideoHeight - gRadius;
	}
	else if (gCircleCenterY <= gRadius)
	{
		gCircleCenterY = gRadius;
	}

	int screensize = gVideoHeight * gVideoWidth * gBytesPerPixel;

	unsigned char* dst = (unsigned char*)gGraphBase + screensize;

	unsigned char* src = (unsigned char*)gGraphBase;

	POINT p;
	p.x = 0;
	p.y = 0;
	__drawRectWindow(&p, gVideoWidth, gVideoHeight, SCREENPROTECT_BACKGROUND_COLOR, dst);

	//sphere7(gCircleCenterX, gCircleCenterY, gRadius, SCREENPROTECT_BACKGROUND_COLOR, (unsigned char*)gGraphBase + screensize * 2);
	ret = __drawCircle(gCircleCenterX, gCircleCenterY, 
		gRadius|0x0000000, (gRadius/2)|0x0000000, gCircleColor, (unsigned char*)gGraphBase + screensize * 2);

	gScreenProtectWindowID = addWindow(0, 0, 0, 0, "__screenProtect");

	gTimerID = __kAddExactTimer((DWORD)__kScreenProtect, CMOS_EXACT_INTERVAL * 2, 0, 0, 0, 0);

	return TRUE;
}



int stopScreenProtect() {
	int ret = 0;

	__kRemoveExactTimer(gTimerID);

	removeWindow(gScreenProtectWindowID);

	gScreenProtectWindowID = 0;

	int screensize = gVideoHeight * gVideoWidth * gBytesPerPixel;

	ret = __restoreCircle(gCircleCenterX, gCircleCenterY, gRadius, gRadius / 2, (unsigned char*)gGraphBase + screensize * 2);

	unsigned char* src = (unsigned char*)gGraphBase + screensize;

	unsigned char* dst = (unsigned char*)gGraphBase;

	POINT p;
	p.x = 0;
	p.y = 0;
	__DestroyRectWindow(&p, gVideoWidth, gVideoHeight, (unsigned char*)src);

	return TRUE;
}



extern "C" __declspec(dllexport) void __kScreenProtect(int p1, int p2, int p3, int p4) {

	unsigned int asc = __kGetKbd(gScreenProtectWindowID) & 0xff;
	//unsigned int asc = __getchar(gScreenProtectWindowID);
	if (asc == 0x1b)
	{
		stopScreenProtect();
		return;
	}

	MOUSEINFO mouseinfo;
	mouseinfo.status = 0;
	mouseinfo.x = 0;
	mouseinfo.y = 0;
	__kGetMouse(&mouseinfo, gScreenProtectWindowID);
	if (mouseinfo.status || mouseinfo.x || mouseinfo.y)
	{
		//stopScreenProtect();
		//return;
	}

	int ret = 0;

	int screensize = gVideoHeight * gVideoWidth * gBytesPerPixel;

	DWORD oldx = gCircleCenterX;
	DWORD oldy = gCircleCenterY;

	gCircleCenterX += gDeltaX;
	if (gCircleCenterX + gRadius >= gVideoWidth)
	{
		gCircleCenterX = gVideoWidth - gRadius;
		gDeltaX = -gDeltaX;
	}
	else if (gCircleCenterX <= gRadius)
	{
		gCircleCenterX = gRadius;
		gDeltaX = -gDeltaX;
	}

	gCircleCenterY += gDeltaY;
	if (gCircleCenterY + gRadius >= gVideoHeight)
	{
		gCircleCenterY = gVideoHeight - gRadius;
		gDeltaY = -gDeltaY;
	}
	else if (gCircleCenterY <= gRadius)
	{
		gCircleCenterY = gRadius;
		gDeltaY = -gDeltaY;
	}

	ret = __restoreCircle(oldx, oldy, gRadius, gRadius / 2, (unsigned char*)gGraphBase + screensize * 2);

	//sphere7(gCircleCenterX, gCircleCenterY, gRadius, SCREENPROTECT_BACKGROUND_COLOR, (unsigned char*)gGraphBase + screensize * 2);
	ret = __drawCircle(gCircleCenterX, gCircleCenterY, 
		gRadius|0x0000000, (gRadius / 2)| 0x0000000, gCircleColor, (unsigned char*)gGraphBase + screensize * 2);
	return;
}











int gVectorGraphWid = 0;
int gBaseColor = 0;
int gVectorGraphTid = 0;
char* gVectorGraphBuf = 0;

void stopVectorGraph() {
	int ret = 0;

	__kRemoveExactTimer(gVectorGraphTid);

	removeWindow(gVectorGraphWid);

	//DWORD backsize = gBytesPerPixel * (gVideoWidth) * (gVideoHeight);
	//__memcpy((char*)gGraphBase,(char*) gVectorGraphBuf, backsize);
	POINT p;
	p.x = 0;
	p.y = 0;
	int color = 0;
	__DestroyRectWindow(&p, gVideoWidth, gVideoHeight, (unsigned char*)gVectorGraphBuf);

	__kFree((DWORD)gVectorGraphBuf);
	return;
}


void VectorGraph(DWORD p1, DWORD p2, DWORD p3, DWORD p4) {

	unsigned int asc = __kGetKbd(gVectorGraphWid) & 0xff;
	if (asc == 0x1b)
	{
		stopVectorGraph();
		return;
	}

	MOUSEINFO mouseinfo;
	mouseinfo.status = 0;
	mouseinfo.x = 0;
	mouseinfo.y = 0;
	__kGetMouse(&mouseinfo, gVectorGraphWid);
	if (mouseinfo.status || mouseinfo.x || mouseinfo.y)
	{
		//stopVectorGraph();
		//return;
	}

	int cx = gVideoWidth / 2;
	int cy = gVideoHeight / 2;

	for (int y = 0; y < gVideoHeight; y++) {
		for (int x = 0; x < gVideoWidth; x++) {

			//#define VECTOR_GRAPH_VIDEO_3
#define VECTOR_GRAPH_VIDEO_2

#ifdef VECTOR_GRAPH_VIDEO_3
			DWORD c = ((x - cx) * (x - cx) * (x - cx)) + ((y - cy) * (y - cy) * (y - cy)) - gBaseColor * gBaseColor * gBaseColor;

#elif defined VECTOR_GRAPH_VIDEO_2
			DWORD c = ((x - cx) * (x - cx)*0x1) + ((y - cy)* (y - cy)*0x1) + gBaseColor * gBaseColor;
			DWORD high = (c >> 24);
			c = c + high;
#else

#endif
			unsigned char* ptr = (unsigned char*)__getpos(x, y) + gGraphBase;
			for (int k = 0; k < gBytesPerPixel; k++) {
				*ptr = c & 0xff;
				c = c >> 8;
				ptr++;
			}
		}
	}

	gBaseColor = (gBaseColor + 1) % 0x1000000;
	return;
}

void initVectorGraph() {
	DWORD backsize = gBytesPerPixel * (gVideoWidth) * (gVideoHeight);

	gVectorGraphBuf = (char*)__kMalloc(backsize);

	POINT p;
	p.x = 0;
	p.y = 0;
	int color = 0;
	__drawRectWindow(&p, gVideoWidth, gVideoHeight, color, (unsigned char*)gVectorGraphBuf);

	gVectorGraphWid = addWindow(FALSE, 0, 0, 0, "VectorGraph");

	gVectorGraphTid = __kAddExactTimer((DWORD)VectorGraph, CMOS_EXACT_INTERVAL * 2, 0, 0, 0, 0);

	gBaseColor = 0;
}



void EllipseScreenColor() {
	DWORD backsize = gBytesPerPixel * (gVideoWidth) * (gVideoHeight);

	DWORD backGround = __kMalloc(backsize);

	POINT p;
	p.x = 0;
	p.y = 0;

	int color = 0;

	int baseColor = 0;

	__drawRectWindow(&p, gVideoWidth, gVideoHeight, color, (unsigned char*)backGround);

	DWORD windowid = addWindow(FALSE, 0, 0, 0, "EllipseScreenColor");

	int A = 13;
	int B = 7;

	while (1)
	{
		unsigned int ck = __kGetKbd(windowid);
		//unsigned int ck = __getchar(windowid);
		unsigned int asc = ck & 0xff;
		if (asc == 0x1b)
		{
			__DestroyRectWindow(&p, gVideoWidth, gVideoHeight, (unsigned char*)backGround);
			removeWindow(windowid);

			__kFree(backGround);

			//__terminatePid(pid);
			return;
		}

		__sleep(0);

		int cx = gVideoWidth / 2;
		int cy = gVideoHeight / 2;

		int cx2 = gVideoWidth / 2 + 100;
		int cy2 = gVideoHeight / 2 + 100;
		for (int y = 0; y < gVideoHeight; y++) {
			for (int x = 0; x < gVideoWidth; x++) {
				DWORD c = (A * A * (x - cx) * (x - cx)*0x1) + (B * B * (y - cy) * (y - cy) * 0x1) + baseColor * baseColor * A * A * B * B ;
				if (c == A * A * B * B*0x1000) {

				}
				DWORD high = c >> 24;
				c = c + high;
				unsigned char* ptr = (unsigned char*)__getpos(x, y) + gGraphBase;
				for (int k = 0; k < gBytesPerPixel; k++) {
					*ptr = c & 0xff;
					c = c >> 8;
					ptr++;
				}
			}
		}

		baseColor = (baseColor + 1) % 0x1000000;

		//int tmp = A;
		//A = B;
		//B = tmp;
	}
}


#define SPIRAL_SMALL_CIRCLE_SIZE2	24
#define SPIRAL_SMALL_CIRCLE_SIZE3	16
#define SPIRAL_SMALL_CIRCLE_SIZE	32

#define AXIS_COLOR					0



void SpiralVectorGraph() {
	DWORD backsize = gBytesPerPixel * (gVideoWidth) * (gVideoHeight);

	DWORD backGround = __kMalloc(backsize);

	DWORD bufsize = gBytesPerPixel * (SPIRAL_SMALL_CIRCLE_SIZE) * (SPIRAL_SMALL_CIRCLE_SIZE);

	DWORD buf = __kMalloc(bufsize);

	DWORD buf2 = __kMalloc(bufsize);

	DWORD buf3 = __kMalloc(bufsize);

	POINT p;
	p.x = 0;
	p.y = 0;

	__drawRectWindow(&p, gVideoWidth, gVideoHeight, 0xffffff, (unsigned char*)backGround);

	DWORD windowid = addWindow(FALSE, 0, 0, 0, "SpiralVectorGraph");
	
	int cx = gVideoWidth / 2;
	int cy = gVideoHeight / 2;

	__drawLine(0, cy, gVideoWidth - 1, cy,0, AXIS_COLOR,0);

	__drawLine(cx, 0, cx, gVideoHeight -1, 0, AXIS_COLOR, 0);

	int color_cos = 0xff00;
	for (int x = 0; x < gVideoWidth; x++)
	{
		int y = cy - (int)( __cos(1.0*((int)x - (int)cx)/100.0) * 100.0);
		int c = color_cos;
		unsigned char* p = (unsigned char*)__getpos((int)x, y) + gGraphBase;
		for (int k = 0; k < gBytesPerPixel; k++) {
				
			p[k] = c &0xff;
			c = c >> 8;
		}

		if (x - cx == 100) {
			int pos = __getpos((int)x, y);
			__drawGraphChar("y=cos(x)", AXIS_COLOR, pos, 0);
		}	
	}

	int color_sin = 0xff0000;
	for (int x = 0; x < gVideoWidth; x++)
	{
		int c = color_sin;
		int y = cy -  (int)(__sin(1.0 * ((int)x - (int)cx)/100.0)  * 100.0);

		unsigned char* p = (unsigned char*)__getpos((int)x, y) + gGraphBase;
		for (int k = 0; k < gBytesPerPixel; k++) {

			p[k] = c & 0xff;
			c = c >> 8;
		}

		if (x - cx == 100) {
			int pos = __getpos((int)x, y);
			__drawGraphChar("y=sin(x)", AXIS_COLOR, pos, 0);
		}
	}

	for (int x = 0; x < gVideoWidth; x++) {
		DWORD y = cy - 100.0 * ((float)(x - cx)*1.0 / 100.0) * (((float)(x - cx)*1.0) / 100.0);
		if (y >= 0) {
			DWORD c = 0xff0000;
			unsigned char* ptr = (unsigned char*)__getpos(x, y) + gGraphBase;
			for (int k = 0; k < gBytesPerPixel; k++) {
				*ptr = c & 0xff;
				c = c >> 8;
				ptr++;
			}
		}

		if (x  == 100) {
			int pos = __getpos((int)x, y);
			__drawGraphChar("y=x*x", AXIS_COLOR, pos, 0);
		}
	}
	
	double A = 1.0;
	double B = 1.0;
	double A2 = 2.0;
	double B2 = 2.0;
	double A3 = 3.0;
	double B3 = 3.0;

	double theta = 0.0;
	double theta2 = 0.0;
	double theta3 = 0.0;

	int color = 0x8000;
	int color2 = 0x800000;
	int color3 = 0x80;

	int oldx = cx + SPIRAL_SMALL_CIRCLE_SIZE*2;
	int oldy = cy + SPIRAL_SMALL_CIRCLE_SIZE*2;
	int oldx2 = cx - SPIRAL_SMALL_CIRCLE_SIZE2*2;
	int oldy2 = cy - SPIRAL_SMALL_CIRCLE_SIZE2*2;
	int oldx3 = cx;
	int oldy3 = cy;
	__drawCircle(oldx, oldy, SPIRAL_SMALL_CIRCLE_SIZE, 0, color, (unsigned char*)buf);

	__drawCircle(oldx2, oldy2, SPIRAL_SMALL_CIRCLE_SIZE2, 0, color2, (unsigned char*)buf2);

	__drawCircle(oldx3, oldy3, SPIRAL_SMALL_CIRCLE_SIZE3, 0, color3, (unsigned char*)buf3);
	
	while (1)
	{
		unsigned int ck = __kGetKbd(windowid);
		unsigned int asc = ck & 0xff;
		if (asc == 0x1b)
		{
			__DestroyRectWindow(&p, gVideoWidth, gVideoHeight, (unsigned char*)backGround);
			removeWindow(windowid);

			__kFree(backGround);

			__kFree(buf);
			__kFree(buf2);
			__kFree(buf3);

			return;
		}

		__sleep(0);
		if (theta >= 64) {
			theta += 0.01;
		}
		else if (theta >= 32) {
			theta += 0.05;
		}
		else {
			theta += 0.1;
		}

		if (theta2 >= 64) {
			theta2 += 0.015;
		}
		else if (theta2 >= 32) {
			theta2 += 0.10;
		}
		else {
			theta2 += 0.15;
		}

		if (theta3 >= 64) {
			theta3 += 0.02;
		}
		else if (theta3 >= 32) {
			theta3 += 0.15;
		}
		else {
			theta3 += 0.2;
		}
		color += 1;
		color2 += 1;
		color3 += 1;

		int px = cx + (int)((A + B * theta) * __cos(theta));
		int py = cy - (int)( (A + B * theta) * __sin(theta));

		int px2 = cx + (int)((A2 + B2 * theta2) * __cos(theta2));
		int py2 = cy - (int)((A2 + B2 * theta2) * __sin(theta2));

		int px3 = cx + (int)((A3 + B3 * theta3) * __cos(theta3));
		int py3 = cy - (int)((A3 + B3 * theta3) * __sin(theta3));
		
		__restoreCircle(oldx3, oldy3, SPIRAL_SMALL_CIRCLE_SIZE3, 0, (unsigned char*)buf3);
		__restoreCircle(oldx2, oldy2, SPIRAL_SMALL_CIRCLE_SIZE2, 0, (unsigned char*)buf2);
		__restoreCircle(oldx, oldy, SPIRAL_SMALL_CIRCLE_SIZE, 0, (unsigned char*)buf);

		if ((px >= SPIRAL_SMALL_CIRCLE_SIZE && px <= gVideoWidth- SPIRAL_SMALL_CIRCLE_SIZE) &&
			(py >= SPIRAL_SMALL_CIRCLE_SIZE && py <= gVideoHeight- SPIRAL_SMALL_CIRCLE_SIZE) ) {

		}
		else {
			px = cx;
			py = cy;

			theta = 0;
		}

		if ((px2 >= SPIRAL_SMALL_CIRCLE_SIZE2 && px2 <= gVideoWidth - SPIRAL_SMALL_CIRCLE_SIZE2) &&
			(py2 >= SPIRAL_SMALL_CIRCLE_SIZE2 && py2 <= gVideoHeight - SPIRAL_SMALL_CIRCLE_SIZE2)) {

		}
		else {
			px2 = cx;
			py2 = cy;

			theta2 = 0;
		}


		if ((px3 >= SPIRAL_SMALL_CIRCLE_SIZE3 && px3 <= gVideoWidth - SPIRAL_SMALL_CIRCLE_SIZE3) &&
			(py3 >= SPIRAL_SMALL_CIRCLE_SIZE3 && py3 <= gVideoHeight - SPIRAL_SMALL_CIRCLE_SIZE3)) {

		}
		else {
			px3 = cx;
			py3 = cy;

			theta3 = 0;
		}

		__drawCircle(px, py, SPIRAL_SMALL_CIRCLE_SIZE, 0, color, (unsigned char*)buf);

		__drawCircle(px2, py2, SPIRAL_SMALL_CIRCLE_SIZE2, 0, color2, (unsigned char*)buf2);

		__drawCircle(px3, py3, SPIRAL_SMALL_CIRCLE_SIZE3, 0, color3, (unsigned char*)buf3);

		oldx = px;
		oldy = py;	
		oldx2 = px2;
		oldy2 = py2;
		oldx3 = px3;
		oldy3 = py3;
	}
}





void CubeVectorGraph() {
	DWORD backsize = gBytesPerPixel * (gVideoWidth) * (gVideoHeight);

	DWORD backGround = __kMalloc(backsize);

	POINT p;
	p.x = 0;
	p.y = 0;

	DWORD color = 0;

	__drawRectWindow(&p, gVideoWidth, gVideoHeight, color, (unsigned char*)backGround);

	DWORD windowid = addWindow(FALSE, 0, 0, 0, "CubeVectorGraph");

	while (1)
	{
		unsigned int ck = __kGetKbd(windowid);
		//unsigned int ck = __getchar(windowid);
		unsigned int asc = ck & 0xff;
		if (asc == 0x1b)
		{
			__DestroyRectWindow(&p, gVideoWidth, gVideoHeight, (unsigned char*)backGround);
			removeWindow(windowid);

			__kFree(backGround);

			//__terminatePid(pid);
			return;
		}

		__sleep(0);

		int cx = gVideoWidth / 2;
		int cy = gVideoHeight / 2;

		for (int y = 0; y < gVideoHeight; y++) {
			for (int x = 0; x < gVideoWidth; x++) {

#define VECTOR_GRAPH_VIDEO_3
				//#define VECTOR_GRAPH_VIDEO_2

#ifdef VECTOR_GRAPH_VIDEO_3
				DWORD c = ((x - cx) * (x - cx) * (x - cx)*0x1) + ((y - cy) * (y - cy) * (y - cy) * 0x1) + color * color * color;
				DWORD high = c >> 24;
				c = c + high;
#elif defined VECTOR_GRAPH_VIDEO_2
				DWORD c = ((x - cx) * (x - cx)) + ((y - cy) * (y - cy)) - color * color;
#else

#endif
				unsigned char* ptr = (unsigned char*)__getpos(x, y) + gGraphBase;
				for (int k = 0; k < gBytesPerPixel; k++) {
					*ptr = c & 0xff;
					c = c >> 8;
					ptr++;
				}
			}
		}

		color = (color + 1) % 0x1000000;
	}
}





void SnowScreenShow() {
	DWORD backsize = gBytesPerPixel * (gVideoWidth) * (gVideoHeight);

	DWORD backGround = __kMalloc(backsize);

	POINT p;
	p.x = 0;
	p.y = 0;
	int color = 0;
	__drawRectWindow(&p, gVideoWidth, gVideoHeight, color, (unsigned char*)backGround);

	DWORD windowid = addWindow(FALSE, 0, 0, 0, "SnowScreenShow");

	while (1)
	{
		unsigned int ck = __kGetKbd(windowid);
		//unsigned int ck = __getchar(windowid);
		unsigned int asc = ck & 0xff;
		if (asc == 0x1b)
		{
			__DestroyRectWindow(&p, gVideoWidth, gVideoHeight, (unsigned char*)backGround);
			removeWindow(windowid);

			__kFree(backGround);

			//__terminatePid(pid);
			return;
		}

		__sleep(0);


		for (int y = 0; y < gVideoHeight; y++) {
			for (int x = 0; x < gVideoWidth; x++) {

				int color = 0xffffff;
				__int64 v = __krdtsc();
				if (v == 0) {
					v = __random(0);
				}
				if (v & 1) {
					color = 0;
				}

				unsigned char* ptr = (unsigned char*)__getpos(x, y) + gGraphBase;
				for (int k = 0; k < gBytesPerPixel; k++) {
					*ptr = color & 0xff;
					color = color >> 8;
					ptr++;
				}
			}
		}

		__diamond(gVideoWidth/2, gVideoHeight/2, 0x100, 0x100, 0);
	}
}














int g_PauseBreakFlag = 0;

void pauseBreak() {
	g_PauseBreakFlag ^= g_PauseBreakFlag;
	return;
}


extern "C" __declspec(dllexport) int __kPrintScreen() {

	int screensize = gVideoHeight * gVideoWidth * gBytesPerPixel;

	char* data = (char*)__kMalloc(gWindowSize);
	BITMAPFILEHEADER* hdr = (BITMAPFILEHEADER*)data;
	hdr->bfType = 0x4d42;
	hdr->bfSize = screensize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	hdr->bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	hdr->bfReserved1 = 0;
	hdr->bfReserved2 = 0;

	BITMAPINFOHEADER* info = (BITMAPINFOHEADER*)(data + sizeof(BITMAPFILEHEADER));
	info->biBitCount = gBytesPerPixel * 8;
	info->biHeight = -gVideoHeight;
	info->biWidth = -gVideoWidth;
	info->biSize = 40;
	info->biSizeImage = gBytesPerPixel * gVideoWidth * gVideoHeight;
	info->biClrImportant = 0;
	info->biClrUsed = 0;
	info->biCompression = 0;
	info->biXPelsPerMeter = 0;
	info->biYPelsPerMeter = 0;

	__memcpy((char*)(data + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)),
		(char*)gGraphBase, screensize);

	char filename[256];
	__printf(filename, "c:\\%x.bmp", *(unsigned int*)TIMER0_TICK_COUNT);
	int ret = writeFile(filename, (char*)data, screensize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER), FILE_WRITE_APPEND);

	__kFree((DWORD)data);
	return 0;
}



#define ANGLE_DIVISION  30

char* gTrajectBuf = 0;
char* g_circle_buf = 0;
int gTrajectWid = 0;
int gTrajectTid = 0;

double g_x_s = 0;
double g_y_s = 0;
double GRAVITY_ACC = 9.80 * 4;
double g_centerX = 0;
double g_centerY = 0;
double g_radius = 32.0;

double g_frame_delay = 0;

int g_counter = 0;
int g_circle_color = 0xffffff;




void stopTrajectory() {
	int ret = 0;
#ifdef USE_CMOS_EXACT_TIMER
	__kRemoveExactTimer(gTrajectTid);
#else
	__kRemove8254Timer(gTrajectTid);
#endif

	removeWindow(gTrajectWid);

	POINT p;
	p.x = 0;
	p.y = 0;
	int color = 0;
	__DestroyRectWindow(&p, gVideoWidth, gVideoHeight, (unsigned char*)gTrajectBuf);

	__kFree((DWORD)gTrajectBuf);

	__kFree((DWORD)g_circle_buf);
	return;
}


//F = 1/2 * ro * v*v * s * 1300*c
double resist_air(double v, double radius) {
	double t = __abs((v * v * 0.67 * __sqrt(radius) / 1226.0 / 2.0));
#if 0
	double min = 0.5 * 1000.0 / g_frame_delay;
	if (t < min) {
		t = min;
	}
#endif
	return t;
}

double resist_bounce(double v, double radius) {
	double r = -v / 2.0;
#if 0
	if (r < -1) {
		r = r - 1;
	}
	else if (r > 1) {
		r = r + 1;
	}
	else {
		if (r >= 0 && r <= 1) {
			if (r >= 0.5) {
				r += 0.5;
			}
			else {
				r = 0;
			}
		}
		else if (r < 0 && r >= -1) {
			if (r <= -0.5) {
				r -= 0.5;
			}
			else {
				r = 0;
			}
		}
		else {
			r = 0;
		}
	}
#endif
	return r;
}


double friction(double v, double radius) {
	double r = __abs(v / 4.0);
#if 0
	double min = 0.1 * 1000.0 / g_frame_delay;
	if (r > min) {
		r += min;
	}
	else {
		r += min;
		//r = r*2;
		//if (r > 0.5) {
		//	r += 0.5;
		//}
		//else {
		//	r = 0;
		//}
	}
#endif

	return r;
}

void TrajectoryProc(DWORD p1, DWORD p2, DWORD p3, DWORD p4) {
	int ret = 0;

	char szout[1024];

	unsigned int asc = __kGetKbd(gTrajectWid) & 0xff;
	if (asc == 0x1b)
	{
		stopTrajectory();
		return;
	}

	MOUSEINFO mouseinfo;
	mouseinfo.status = 0;
	mouseinfo.x = 0;
	mouseinfo.y = 0;
	__kGetMouse(&mouseinfo, gTrajectWid);
	if (mouseinfo.status || mouseinfo.x || mouseinfo.y)
	{
		//stopTrajectory();
		//return;
	}

	double dx = resist_air(g_x_s, g_radius) * g_frame_delay / 1000.0;
	if (__abs(g_centerY - ((double)gVideoHeight - (double)g_radius)) <= 1.0) {
		dx += friction(g_x_s, g_radius) * g_frame_delay / 1000.0;
	}

	double gy = GRAVITY_ACC * g_frame_delay / 1000.0;
	double ry = resist_air(g_y_s, g_radius) * g_frame_delay / 1000.0;
	if (g_y_s > 0)
	{
		g_y_s = g_y_s - gy - ry;
		if (g_y_s < 0)
		{
			//g_y_s = 0;
		}
	}
	else {
		g_y_s = g_y_s - gy + ry;
		if (g_y_s < 0)
		{
			//g_y_s = 0;
		}
	}

	__int64 max_y = (__int64)((double)gVideoHeight - g_radius);
	if (__abs(g_y_s) < 0.1) {
		//g_y_s = 0;
	}

	if (g_x_s < 0) {
		g_x_s = g_x_s + dx;
		if (g_x_s > 0) {
			g_x_s = 0;
		}
	}
	else {
		g_x_s = g_x_s - dx;
		if (g_x_s < 0) {
			g_x_s = 0;
		}
	}

	double x = g_centerX + g_x_s;

	double y = g_centerY - g_y_s;

	if (x + g_radius >= gVideoWidth) {

		g_x_s = resist_bounce(g_x_s, g_radius);
		x = (double)gVideoWidth - g_radius;
		if (g_x_s > 0) {
			g_x_s = -g_x_s;
		}
	}
	else if (x - g_radius <= 0) {

		g_x_s = resist_bounce(g_x_s, g_radius);
		x = g_radius;
		if (g_x_s < 0) {
			g_x_s = -g_x_s;
		}
	}

	if (y + g_radius >= gVideoHeight) {

		g_y_s = resist_bounce(g_y_s, g_radius);
		y = (double)gVideoHeight - g_radius;
		if (g_y_s < 0) {
			g_y_s = -g_y_s;
		}
	}
	else if (y - g_radius <= 0) {

		g_y_s = resist_bounce(g_y_s, g_radius);
		y = g_radius;
		if (g_y_s > 0) {
			g_y_s = -g_y_s;
		}
	}

	if (x == g_centerX && y == g_centerY)
	{

	}
	else {

	}

	ret = __restoreCircle((int)g_centerX, (int)g_centerY, (int)g_radius, (int)g_radius/2, (unsigned char*)g_circle_buf);
	g_centerX = x;
	g_centerY = y;
	ret = __drawCircle((int)g_centerX, (int)g_centerY, (int)g_radius, (int)g_radius / 2, g_circle_color, (unsigned char*)g_circle_buf);

	if (__abs(g_y_s) <= 0.5 && __abs(g_x_s) <= 0.5 && (__abs(y - max_y) < 0.1 || __abs(g_centerY - max_y) < 0.1)) {
		g_counter++;
		if (g_counter > 2000.0 / g_frame_delay) {
			g_counter = 0;
			ret = __restoreCircle((int)g_centerX, (int)g_centerY, (int)g_radius, (int)g_radius / 2, (unsigned char*)g_circle_buf);

			double max_speed = ((double)1000.0 / (double)g_frame_delay) * GRAVITY_ACC * 10.0;

			__int64 ims = (__int64)max_speed;
			if (ims == 0) {
				__printf(szout, "max speed:%x,%lf\r\n", ims, max_speed);
				ims = 3000;
			}

			double velocity = (double)(__random(0) % (__int64)ims) + (1000.0 / (double)g_frame_delay) * 10.0;

			velocity = velocity * g_frame_delay / 1000.0;

			double angle = __random(0) % ANGLE_DIVISION;
			angle = PI / (angle + 1);
			if (angle <= PI / 12) {
				angle += PI / 12;
			}
			else if (angle >= PI * 11 / 12) {
				angle -= PI / 12;
			}

			g_x_s = __cos(angle) * velocity;
			g_y_s = __sin(angle) * velocity;

			//g_centerY = (double)((__int64)gVideoHeight - (__int64)g_radius - (__int64)TASKBAR_HEIGHT * 2);

			//g_centerX = (double)g_radius + TASKBAR_HEIGHT;

			ret = __drawCircle((int)g_centerX, (int)g_centerY, (int)g_radius, (int)g_radius / 2, g_circle_color, (unsigned char*)g_circle_buf);
		}
	}
	else {

	}

	__sprintf(szout, "(X:%f,Y:%f) (XS:%f,YS:%f)        ", g_centerX, g_centerY, g_x_s, g_y_s);
	int showPos = __getpos(0 , gVideoHeight - TASKBAR_HEIGHT*2);
	__drawGraphChar(szout, OUTPUT_INFO_COLOR, showPos, g_circle_color);

}




void initTrajectory() {
	int ret = 0;
	char szout[1024];

	DWORD backsize = gBytesPerPixel * (gVideoWidth) * (gVideoHeight);

	gTrajectBuf = (char*)__kMalloc(backsize);

	g_circle_buf = (char*)__kMalloc((int)(g_radius + 4) * 2 * 2 * (int)(g_radius + 4) * gBytesPerPixel);

	gTrajectWid = addWindow(FALSE, 0, 0, 0, "Trajectory");

	POINT p;
	p.x = 0;
	p.y = 0;
	int color = 0;
	__drawRectWindow(&p, gVideoWidth, gVideoHeight, color, (unsigned char*)gTrajectBuf);
#ifdef USE_CMOS_EXACT_TIMER
	g_frame_delay = (double)CMOS_EXACT_INTERVAL * 2.0;
	gTrajectTid = __kAddExactTimer((DWORD)TrajectoryProc, (int)g_frame_delay, 0, 0, 0, 0);
#else
	g_frame_delay = (double)CMOS_EXACT_INTERVAL * 2.0;
	gTrajectTid = __kAdd8254Timer((DWORD)TrajectoryProc, (int)g_frame_delay, 0, 0, 0, 0);
#endif

	double max_speed = ((double)1000.0 / (double)g_frame_delay) * GRAVITY_ACC * 10.0;

	__int64 ims = (__int64)max_speed;
	if (ims == 0) {
		__printf(szout, "max speed:%x,%lf\r\n", ims, max_speed);
		ims = 3000;
	}

	double velocity = (double)(__random(0) % (__int64)ims) + ((double)1000.0 / (double)g_frame_delay) * 10.0;

	velocity = velocity * g_frame_delay / 1000.0;

	double angle = __random(0) % ANGLE_DIVISION;
	angle = PI / (angle + 1);
	if (angle <= PI / 12) {
		angle += PI / 12;
	}
	else if (angle >= PI * 11 / 12) {
		angle -= PI / 12;
	}

	//g_x_s = GetCos(angle) * velocity / 256;
	//g_y_s = GetSin(angle) * velocity/256;

	g_x_s = __cos(angle) * velocity;
	g_y_s = __sin(angle) * velocity;

	g_centerY = (double)((__int64)gVideoHeight - (__int64)g_radius - (__int64)TASKBAR_HEIGHT * 4);

	g_centerX = (double)g_radius + (__int64)gVideoWidth / 4;

	g_counter = 0;

	ret = __drawCircle((int)g_centerX, (int)g_centerY, (int)g_radius, (int)g_radius / 2, g_circle_color, (unsigned char*)g_circle_buf);

	__sprintf(szout, "(X:%f,Y:%f) (XS:%f,YS:%f)        ", g_centerX, g_centerY, g_x_s, g_y_s);
	int showPos = __getpos(0 , gVideoHeight - TASKBAR_HEIGHT*2);
	__drawGraphChar(szout, OUTPUT_INFO_COLOR, showPos, g_circle_color);
}










