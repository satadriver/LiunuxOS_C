#pragma once
#include "paint.h"
#include "video.h"
#include "Utils.h"
#include "guiHelper.h"
#include "mouse.h"
#include "keyboard.h"
#include "task.h"
#include "file.h"
#include "graph.h"
#include "Pe.h"
#include "window.h"
#include "servicesProc.h"

#define POLYGON_SIZE	0x10

#define POLYGON_RADIUS	0x100


void DiamondAnimation(WINDOWCLASS * window) {
	//DWORD backsize = gBytesPerPixel * (gVideoWidth) * (gVideoHeight);
	//DWORD backGround = __kMalloc(backsize);
	//POINT p;
	//p.x = 0;
	//p.y = 0;
	//int color = 0;
	//__drawRectWindow(&p, gVideoWidth, gVideoHeight, color, (unsigned char*)backGround);
	//DWORD windowid = addWindow(FALSE, 0, 0, 0, "SnowScreenShow");

	unsigned char* videoBase = (unsigned char*)GetVideoBase();

	while (1)
	{
		unsigned int ck = __kGetKbd(window->id);
		unsigned int asc = ck & 0xff;
		if (asc == 0x1b)
		{
			return;
		}

		MOUSEINFO mouseinfo;
		__memset((char*)&mouseinfo, 0, sizeof(MOUSEINFO));
		int ret = __kGetMouse(&mouseinfo, window->id);
		if (mouseinfo.status & 1)	//left click
		{
			if (mouseinfo.x >= window->shutdownx && mouseinfo.x <= window->shutdownx + window->capHeight)
			{
				if (mouseinfo.y >= window->shutdowny && mouseinfo.y <= window->shutdowny + window->capHeight)
				{
					return;
				}
			}
			if (mouseinfo.x >= window->minx && mouseinfo.x <= window->minx + window->capHeight)
			{
				if (mouseinfo.y >= window->miny && mouseinfo.y <= window->miny + window->capHeight)
				{
					MinimizeWindow(window);
				}
			}
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

				unsigned char* ptr = (unsigned char*)__getpos(x, y) + (DWORD)videoBase;
				for (int k = 0; k < gBytesPerPixel; k++) {
					*ptr = color & 0xff;
					color = color >> 8;
					ptr++;
				}
			}
		}

		__diamond(gVideoWidth / 2, gVideoHeight / 2, POLYGON_RADIUS, POLYGON_SIZE, 0);
	}
}



extern "C" __declspec(dllexport)int DiamondVector(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD runparam) {

	char szout[1024];

	WINDOWCLASS window;

	initFullWindow(&window, funcname, tid,1);

	DiamondAnimation(&window);

	__DestroyWindow(&window);

	return 0;
}