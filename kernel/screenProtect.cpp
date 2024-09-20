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


#define SCREENPROTECT_BACKGROUND_COLOR 0		//0XBBFFFF		0X87CEEB



int gCircleCenterX = 0;
int gCircleCenterY = 0;
int gRadius = 128;
int gDeltaX = 6;
int gDeltaY = 6;

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

	__kRestoreMouse();

	disableMouse();

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

	sphere7(gCircleCenterX, gCircleCenterY, gRadius, SCREENPROTECT_BACKGROUND_COLOR, (unsigned char*)gGraphBase + screensize * 2);
	//ret = __drawColorCircle(gCircleCenterX, gCircleCenterY, gRadius, gCircleColor, (unsigned char*)gGraphBase + screensize*2);

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
	__kDrawMouse();

	return TRUE;
}



extern "C" __declspec(dllexport) void __kScreenProtect(int p1,int p2,int p3,int p4) {

	unsigned int ck = __kGetKbd(gScreenProtectWindowID);
	//unsigned int ck = __getchar(gScreenProtectWindowID);
	unsigned int asc = ck & 0xff;
	if(asc)
	//if (asc == 0x1b || asc == 0x0a)
	{
		stopScreenProtect();
		return;
	}

	MOUSEINFO mouseinfo;
	mouseinfo.status = 0;
	getmouse(&mouseinfo, gScreenProtectWindowID);
	if (mouseinfo.status )	
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

	sphere7(gCircleCenterX, gCircleCenterY, gRadius, SCREENPROTECT_BACKGROUND_COLOR, (unsigned char*)gGraphBase + screensize * 2);
	//ret = __drawColorCircle(gCircleCenterX, gCircleCenterY, gRadius, gCircleColor, (unsigned char*)gGraphBase + screensize * 2);
	return ;
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