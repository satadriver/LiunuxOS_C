


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




void CubeAnimation(WINDOWCLASS* window) {
	//DWORD backsize = gBytesPerPixel * (gVideoWidth) * (gVideoHeight);
	//DWORD backGround = __kMalloc(backsize);
	//POINT p;
	//p.x = 0;
	//p.y = 0;

	DWORD color = 0;

	//__drawRectWindow(&p, gVideoWidth, gVideoHeight, color, (unsigned char*)backGround);
	//DWORD windowid = addWindow(FALSE, 0, 0, 0, "CubeVectorGraph");

	while (1)
	{
		unsigned int ck = __kGetKbd(window->id);
		//unsigned int ck = __getchar(windowid);
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

		for (int y = 0; y < gVideoHeight; y++) {
			for (int x = 0; x < gVideoWidth; x++) {

#define VECTOR_GRAPH_VIDEO_3
				//#define VECTOR_GRAPH_VIDEO_2

#ifdef VECTOR_GRAPH_VIDEO_3
				DWORD c = ((x - cx) * (x - cx) * (x - cx) * 0x1) + ((y - cy) * (y - cy) * (y - cy) * 0x1) + color * color * color;
				DWORD high = c >> 24;
				c = c + high;
#elif defined VECTOR_GRAPH_VIDEO_2
				DWORD c = ((x - cx) * (x - cx)) + ((y - cy) * (y - cy)) - color * color;
#else

#endif
				unsigned char* ptr = (unsigned char*)__getpos(x, y) + (DWORD)videoBase;
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





extern "C" __declspec(dllexport)int CubeVector(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD runparam) {

	char szout[1024];

	WINDOWCLASS window;

	initFullWindow(&window, filename, tid,1);

	CubeAnimation(&window);

	__DestroyWindow(&window);

	return 0;
}