


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


void EllipseAnimation(WINDOWCLASS * window) {


	//DWORD backsize = gBytesPerPixel * (gVideoWidth) * (gVideoHeight);
	//DWORD backGround = __kMalloc(backsize);
	//POINT p;
	//p.x = 0;
	//p.y = 0;

	int baseColor = 0;

	//__drawRectWindow(&p, gVideoWidth, gVideoHeight, color, (unsigned char*)backGround);
	//DWORD windowid = addWindow(FALSE, 0, 0, 0, "EllipseScreenColor");

	int A = 13;
	int B = 7;

	while (1)
	{
		unsigned int ck = __kGetKbd(window->id);
		unsigned int asc = ck & 0xff;
		if (asc == 0x1b)
		{
			//__terminatePid(pid);
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
		}

		__sleep(0);

		int cx = gVideoWidth / 2;
		int cy = gVideoHeight / 2;

		unsigned char* videoBase = (unsigned char*)GetVideoBase();

		int cx2 = gVideoWidth / 2 + 100;
		int cy2 = gVideoHeight / 2 + 100;
		for (int y = 0; y < gVideoHeight; y++) {
			for (int x = 0; x < gVideoWidth; x++) {
				DWORD c = (A * A * (x - cx) * (x - cx) * 0x1) + (B * B * (y - cy) * (y - cy) * 0x1) + baseColor * baseColor * A * A * B * B;
				if (c == A * A * B * B * 0x1000) {

				}
				DWORD high = c >> 24;
				c = c + high;
				unsigned char* ptr = (unsigned char*)__getpos(x, y) + (DWORD)videoBase;
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



int EllipseVector(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD runparam) {

	char szout[1024];

	WINDOWCLASS window;

	initFullWindow(&window, filename, tid);

	EllipseAnimation(&window);

	__DestroyWindow(&window);

	return 0;
}