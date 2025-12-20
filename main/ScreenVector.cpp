
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

#include "guiHelper.h"


#define SCREENPROTECT_BACKGROUND_COLOR 0		//0XBBFFFF		0X87CEEB


int gCircleColor = 0xffffff;
int gCircleCenterX = 0;
int gCircleCenterY = 0;
int gRadius = 64;
int gDeltaX = 3;
int gDeltaY = 3;

int gTimerID = 0;

WINDOWCLASS g_sv_window;



int stopScreenVector() {
	int ret = 0;

	__kRemoveExactTimer(gTimerID);

	unsigned char* videoBase = (unsigned char*)GetVideoBase();

	int screensize = gVideoHeight * gVideoWidth * gBytesPerPixel;

	ret = __restoreCircle(gCircleCenterX, gCircleCenterY, gRadius, gRadius / 2, (unsigned char*)videoBase + screensize * 2);

	//POINT p;
	//p.x = 0;
	//p.y = 0;
	//__DestroyRectWindow(&p, gVideoWidth, gVideoHeight, (unsigned char*)src);
	//removeWindow(gScrnWindowID);
	//gScrnWindowID = 0;
	__DestroyWindow(&g_sv_window);

	return TRUE;
}

extern "C" __declspec(dllexport) void ScreenAnimation() {
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

	unsigned char* videoBase = (unsigned char*)GetVideoBase();

	ret = __restoreCircle(oldx, oldy, gRadius, gRadius / 2, (unsigned char*)videoBase + screensize * 2);

	//sphere7(gCircleCenterX, gCircleCenterY, gRadius, SCREENPROTECT_BACKGROUND_COLOR, (unsigned char*)gGraphBase + screensize * 2);
	ret = __drawCircle(gCircleCenterX, gCircleCenterY,
		gRadius | 0x0000000, (gRadius / 2) | 0x0000000, gCircleColor, (unsigned char*)videoBase + screensize * 2);
	return;
}


int initScreenVector() {
	int ret = 0;

	unsigned int r = __random(0);

	gCircleColor = ~g_sv_window.color;

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

	unsigned char* videoBase = (unsigned char*)GetVideoBase();

	unsigned char* dst = (unsigned char*)videoBase + screensize;

	unsigned char* src = (unsigned char*)videoBase;

	//POINT p;
	//p.x = 0;
	//p.y = 0;
	//__drawRectWindow(&p, gVideoWidth, gVideoHeight, SCREENPROTECT_BACKGROUND_COLOR, dst);
	//gScrnWindowID = addWindow(0, 0, 0, 0, "__screenProtect");

	ret = __drawCircle(gCircleCenterX, gCircleCenterY,gRadius | 0x0000000, (gRadius / 2) | 0x0000000, gCircleColor,
		(unsigned char*)videoBase + screensize * 2);

	gTimerID = __kAddExactTimer((DWORD)ScreenAnimation, CMOS_EXACT_INTERVAL * 2, 0, 0, 0, 0);

	return TRUE;
}










extern "C" __declspec(dllexport)int ScreenVector(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD runparam) {

	char szout[256];
	int ret = 0;

	initFullWindow(&g_sv_window, funcname, tid,1);

	initScreenVector();

	while (1) {

		unsigned int ck = __kGetKbd(g_sv_window.id);
		unsigned int asc = ck & 0xff;
		if (asc == 0x1b)
		{		
			break;
		}

		MOUSEINFO mouseinfo;
		__memset((char*)&mouseinfo, 0, sizeof(MOUSEINFO));
		ret = __kGetMouse(&mouseinfo, g_sv_window.id);
		if (mouseinfo.status & 1)
		{
			if (mouseinfo.x >= g_sv_window.shutdownx && mouseinfo.x <= g_sv_window.shutdownx + g_sv_window.capHeight)
			{
				if (mouseinfo.y >= g_sv_window.shutdowny && mouseinfo.y <= g_sv_window.shutdowny + g_sv_window.capHeight)
				{
					break;
				}
			}
			if (mouseinfo.x >= g_sv_window.minx && mouseinfo.x <= g_sv_window.minx + g_sv_window.capHeight)
			{
				if (mouseinfo.y >= g_sv_window.miny && mouseinfo.y <= g_sv_window.miny + g_sv_window.capHeight)
				{
					MinimizeWindow(&g_sv_window);
				}
			}
		}

		__sleep(0);
	}

	stopScreenVector();

	return 0;
}