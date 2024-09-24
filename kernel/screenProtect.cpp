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
int gRadius = 64;
int gDeltaX = 6;
int gDeltaY = 6;

int gScreenProtectWindowID = 0;

int gTimerID = 0;

int gCircleColor = 0xffffff;



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
char gVectorGraphBuf = 0;

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
	__restoreRectWindow(&p, gVideoWidth, gVideoHeight, (unsigned char*)gVectorGraphBuf);

	__kFree(gVectorGraphBuf);
	return;
}


void VectorGraph() {

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

	DWORD cx = gVideoWidth / 2;
	DWORD cy = gVideoHeight / 2;

	for (DWORD y = 0; y < gVideoHeight; y++) {
		for (DWORD x = 0; x < gVideoWidth; x++) {
			//DWORD r = ((x - cx) ^ 2) + ((y - cy) ^ 2);
			DWORD r = ((x - cx)*(x - cx)) + ((y - cy) * (y - cy));
			DWORD c = 0;
			//DWORD c = r + (r << 8) + (r << 16);
			c = r + gBaseColor* gBaseColor;
			//__asm {
			//	mov eax,r
			//	mov ecx, gBaseColor
			//	rol eax, cl
			//	mov c,eax
			//}
			//DWORD c = r << gBaseColor;
			unsigned char* ptr = (unsigned char*)__getpos(x, y) + gGraphBase;
			for (int k = 0; k < gBytesPerPixel; k++) {


				*ptr = c&0xff;
				c = c >> 8;

				ptr++;
			}
		}
	}

	gBaseColor = gBaseColor+1;
	//gBaseColor = (gBaseColor + 1)%32;
}

void initVectorGraph() {
	DWORD backsize = gBytesPerPixel * (gVideoWidth) * (gVideoHeight);

	gVectorGraphBuf = __kMalloc(backsize);

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
			__restoreRectWindow(&p, gVideoWidth, gVideoHeight, (unsigned char*)backGround);
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