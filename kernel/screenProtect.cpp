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

#define SCREENPROTECT_BACKGROUND_COLOR 0		//0XBBFFFF		0X87CEEB


int gCircleColor = 0xffffff;
int gCircleCenterX = 0;
int gCircleCenterY = 0;
int gRadius = 64;
int gDeltaX = 1;
int gDeltaY = 1;

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

	//__kRestoreMouse();
	//disableMouse();

	int screensize = gVideoHeight*gVideoWidth*gBytesPerPixel;

	unsigned char * dst = (unsigned char*)gGraphBase + screensize;

	unsigned char * src = (unsigned char*)gGraphBase;

	__memcpy((char*)dst, (char*)src, screensize);

	//__memset4((char*)src, SREENPROTECT_COLOR, screensize);
	//__memset((char*)src, SREENPROTECT_COLOR, screensize);

	POINT p;
	p.x = 0;
	p.y = 0;
	__drawRectWindow(&p, gVideoWidth, gVideoHeight, SCREENPROTECT_BACKGROUND_COLOR, 0);

	//sphere7(gCircleCenterX, gCircleCenterY, gRadius, SCREENPROTECT_BACKGROUND_COLOR, (unsigned char*)gGraphBase + screensize * 2);
	ret = __drawColorCircle(gCircleCenterX, gCircleCenterY, gRadius, gCircleColor, (unsigned char*)gGraphBase + screensize*2);

	gScreenProtectWindowID = addWindow(0, 0, 0, 0,"__screenProtect");

	gTimerID = __kAddExactTimer((DWORD)__kScreenProtect, CMOS_EXACT_INTERVAL, 0, 0, 0, 0);
	
	return TRUE;
}



int stopScreenProtect() {
	int ret = 0;

	__kRemoveExactTimer(gTimerID);

	removeWindow(gScreenProtectWindowID);

	gScreenProtectWindowID = 0;
	
	int screensize = gVideoHeight*gVideoWidth*gBytesPerPixel;

	ret = __restoreCircle(gCircleCenterX, gCircleCenterY, gRadius, (unsigned char*)gGraphBase + screensize * 2);

	unsigned char * src = (unsigned char*)gGraphBase + screensize;

	unsigned char * dst = (unsigned char*)gGraphBase;

	__memcpy((char*)dst, (char*)src, screensize);

	//enableMouse();
	//setMouseRate(200);
	//__kDrawMouse();

	return TRUE;
}



extern "C" __declspec(dllexport) void __kScreenProtect(int p1,int p2,int p3,int p4) {

	unsigned int asc = __kGetKbd(gScreenProtectWindowID) & 0xff;
	//unsigned int asc = __getchar(gScreenProtectWindowID);
	if(asc)
	if (asc == 0x1b || asc == 0x0a || asc == 0x0d)
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
		stopScreenProtect();
		return;
	}
	
	int ret = 0;

	int screensize = gVideoHeight*gVideoWidth*gBytesPerPixel;

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
	
	ret = __restoreCircle(oldx, oldy, gRadius, (unsigned char*)gGraphBase + screensize * 2);

	//sphere7(gCircleCenterX, gCircleCenterY, gRadius, SCREENPROTECT_BACKGROUND_COLOR, (unsigned char*)gGraphBase + screensize * 2);
	ret = __drawColorCircle(gCircleCenterX, gCircleCenterY, gRadius, gCircleColor, (unsigned char*)gGraphBase + screensize * 2);
	return ;
}











int gVectorGraphWid = 0;
int gBaseColor = 0;
int gVectorGraphTiD = 0;
char *gVectorGraphBuf = 0;

void stopVectorGraph() {
	int ret = 0;

	__kRemoveExactTimer(gVectorGraphTiD);

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
	if (asc == 0x1b || asc == 0x0a || asc == 0x0d)
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
		stopVectorGraph();
		return;
	}

	int cx = gVideoWidth / 2;
	int cy = gVideoHeight / 2;

	for (int y = 0; y < gVideoHeight; y++) {
		for (int x = 0; x < gVideoWidth; x++) {

//#define VECTOR_GRAPH_VIDEO_1
#define VECTOR_GRAPH_VIDEO_2

#ifdef VECTOR_GRAPH_VIDEO_1
			DWORD c = ((x - cx) * (x - cx) * (x - cx)) + ((y - cy) * (y - cy) * (y - cy)) + gBaseColor * gBaseColor * gBaseColor;
			
#elif defined VECTOR_GRAPH_VIDEO_2
			DWORD c = ((x - cx)* (x - cx)) + ((y - cy) * (y - cy)) + gBaseColor * gBaseColor;
			
#else

#endif
			unsigned char* ptr = (unsigned char*)__getpos(x, y) + gGraphBase;
			for (int k = 0; k < gBytesPerPixel; k++) {
				*ptr = c&0xff;
				c = c >> 8;
				ptr++;
			}
		}
	}
	//gBaseColor = gBaseColor + 1;

	gBaseColor = (gBaseColor + 3) % 0x1000000;
}

void initVectorGraph() {
	DWORD backsize = gBytesPerPixel * (gVideoWidth) * (gVideoHeight);

	gVectorGraphBuf = (char*)__kMalloc(backsize);

	gVectorGraphWid = addWindow(FALSE, 0, 0, 0, "VectorGraph");

	POINT p;
	p.x = 0;
	p.y = 0;
	int color = 0;
	__drawRectWindow(&p, gVideoWidth, gVideoHeight, color, (unsigned char*)gVectorGraphBuf);

	gVectorGraphTiD = __kAddExactTimer((DWORD)VectorGraph, CMOS_EXACT_INTERVAL, 0, 0, 0, 0);
}



void refreshScreenColor() {
	DWORD backsize = gBytesPerPixel * (gVideoWidth) * (gVideoHeight);

	DWORD backGround = __kMalloc(backsize);

	POINT p;
	p.x = 0;
	p.y = 0;

	int color = 0;

	__drawRectWindow(&p, gVideoWidth, gVideoHeight, color, (unsigned char*)backGround);

	DWORD windowid = addWindow(FALSE, 0, 0, 0, "refreshScreen");

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

		/*
		for (int y = 0; y < gVideoHeight; y++) {

			for (int x = 0; x < gVideoWidth; x++) {

				unsigned char* ptr = (unsigned char*)__getpos(x, y) + gGraphBase;
				int c = color;
				for (int k = 0; k < gBytesPerPixel; k++) {
					*ptr = c & 0xff;
					c = c >> 8;
					ptr++;
				}		
				color = (color + 1);
			}	
		}*/

		int cx = gVideoWidth / 2;
		int cy = gVideoHeight / 2;

		for (int y = 0; y < gVideoHeight; y++) {
			for (int x = 0; x < gVideoWidth; x++) {
				DWORD c = ((x - cx) * (x - cx) * (x - cx)* (x - cx)) + ((y - cy) * (y - cy) * (y - cy) * (y - cy)) + 
					gBaseColor * gBaseColor * gBaseColor* gBaseColor;
				unsigned char* ptr = (unsigned char*)__getpos(x, y) + gGraphBase;
				for (int k = 0; k < gBytesPerPixel; k++) {
					*ptr = c & 0xff;
					c = c >> 8;
					ptr++;
				}
			}
		}
		//gBaseColor = gBaseColor + 1;

		gBaseColor = (gBaseColor + 1) ;
	}
}




void refreshScreenColor2() {
	DWORD backsize = gBytesPerPixel * (gVideoWidth) * (gVideoHeight);

	DWORD backGround = __kMalloc(backsize);

	POINT p;
	p.x = 0;
	p.y = 0;

	int color = 0;

	__drawRectWindow(&p, gVideoWidth, gVideoHeight, color, (unsigned char*)backGround);

	DWORD windowid = addWindow(FALSE, 0, 0, 0, "refreshScreen2");

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

		color += 0x00010f;
		__drawRectWindow(&p, gVideoWidth, gVideoHeight, color, 0);
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




#define  GRAVITY_ACC  9.8

char * gTrajectBuf = 0;
char* g_circle_buf = 0;
int gTrajectWid = 0;
int gTrajectTid = 0;
int g_x_s;
int g_y_s;

int g_circle_color = 0xffffff;
int g_centerX = 0;
int g_centerY = 0;
int g_radius = 32;

void stopTrajectory() {
	int ret = 0;

	__kRemoveExactTimer(gTrajectTid);

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

int resist_air(int v, int radius) {
	return (v * v + radius*radius) / 10000;
}

int resist_bounce(int v, int radius) {
	return -(v*v/radius);
}

void TrajectoryProc() {
	int ret = 0;

	unsigned int asc = __kGetKbd(gTrajectWid) & 0xff;
	if (asc == 0x1b || asc == 0x0a || asc == 0x0d)
	{
		stopTrajectory();
		return;
	}

	MOUSEINFO mouseinfo;
	mouseinfo.status = 0;
	mouseinfo.x = 0;
	mouseinfo.y = 0;
	__kGetMouse(&mouseinfo, gVectorGraphWid);
	if (mouseinfo.status || mouseinfo.x || mouseinfo.y)
	{
		stopTrajectory();
		return;
	}

	int xa = 0;

	g_x_s = g_x_s - resist_air(g_x_s,g_radius);
	if (g_x_s < 0) {
		g_x_s = 0;
	}

	g_y_s = g_y_s - GRAVITY_ACC * CMOS_EXACT_INTERVAL / 1000;


	int x = 0;
	int y = 0;
	x = g_centerX + g_x_s ;

	y = g_centerY - g_y_s ;

	if (x + g_radius > gVideoWidth) {

		g_x_s = resist_bounce(g_x_s, g_radius);
	}
	else if (y + g_radius > gVideoHeight) {

		g_y_s = resist_bounce(g_y_s, g_radius);
	}
	else if (x - g_radius < 0) {

		g_x_s = resist_bounce(g_x_s, g_radius);
	}
	else if (y - g_radius < 0) {

		g_y_s = resist_bounce(g_y_s, g_radius);
	}


	ret = __restoreCircle(g_centerX, g_centerY, g_radius, (unsigned char*)g_circle_buf);

	g_centerX = (int)x;
	g_centerY = (int)y;

	ret = __drawColorCircle(g_centerX, g_centerY, g_radius, g_circle_color, (unsigned char*)g_circle_buf);

}




void initTrajectory() {
	DWORD backsize = gBytesPerPixel * (gVideoWidth) * (gVideoHeight);

	gTrajectBuf = (char*)__kMalloc(backsize);

	g_circle_buf = (char*)__kMalloc( g_radius * 2 * 2 * g_radius * gBytesPerPixel);

	gTrajectWid = addWindow(FALSE, 0, 0, 0, "Trajectory");

	POINT p;
	p.x = 0;
	p.y = 0;
	int color = 0x00ff00;
	__drawRectWindow(&p, gVideoWidth, gVideoHeight, color, (unsigned char*)gTrajectBuf);

	gTrajectTid = __kAddExactTimer((DWORD)TrajectoryProc, CMOS_EXACT_INTERVAL, 0, 0, 0, 0);

	int velocity = __random(TIMER0_TICK_COUNT) % 100;

	int angle = __random(TIMER0_TICK_COUNT) % 64;

	g_x_s = GetCos(angle) * velocity / 256;

	g_y_s = GetSin(angle) * velocity/256;

	g_centerY = gVideoHeight - g_radius - 20;

	g_centerX = g_radius + 20;

	int ret = __drawColorCircle(g_centerX, g_centerY, g_radius, g_circle_color, (unsigned char*)g_circle_buf);
}










