




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
#include "cmosExactTimer.h"
#include "timer8254.h"



int gVectorGraphWid = 0;
int gBaseColor = 0;
int gVectorGraphTid = 0;
char* gVectorGraphBuf = 0;

WINDOWCLASS g_sqaure_window;

void stopSqaureAnimation() {
	int ret = 0;

	__kRemoveExactTimer(gVectorGraphTid);

	__DestroyWindow(&g_sqaure_window);

	//POINT p;
	//p.x = 0;
	//p.y = 0;
	//int color = 0;
	//__DestroyRectWindow(&p, gVideoWidth, gVideoHeight, (unsigned char*)gVectorGraphBuf);
	//__kFree((DWORD)gVectorGraphBuf);
	return;
}


void SqaureAnimation() {



	int cx = gVideoWidth / 2;
	int cy = gVideoHeight / 2;

	for (int y = 0; y < gVideoHeight; y++) {
		for (int x = 0; x < gVideoWidth; x++) {

			//#define VECTOR_GRAPH_VIDEO_3
#define VECTOR_GRAPH_VIDEO_2

#ifdef VECTOR_GRAPH_VIDEO_3
			DWORD c = ((x - cx) * (x - cx) * (x - cx)) + ((y - cy) * (y - cy) * (y - cy)) - gBaseColor * gBaseColor * gBaseColor;

#elif defined VECTOR_GRAPH_VIDEO_2
			DWORD c = ((x - cx) * (x - cx) * 0x1) + ((y - cy) * (y - cy) * 0x1) + gBaseColor * gBaseColor;
			DWORD high = (c >> 24);
			c = c + high;
#else

#endif
			unsigned char* videoBase = (unsigned char*)GetVideoBase();
			unsigned char* ptr = (unsigned char*)__getpos(x, y) + (DWORD)videoBase;
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

void initSquareVector() {
	DWORD backsize = gBytesPerPixel * (gVideoWidth) * (gVideoHeight);

	//gVectorGraphBuf = (char*)__kMalloc(backsize);
	//POINT p;
	//p.x = 0;
	//p.y = 0;
	//int color = 0;
	//__drawRectWindow(&p, gVideoWidth, gVideoHeight, color, (unsigned char*)gVectorGraphBuf);
	//gVectorGraphWid = addWindow(FALSE, 0, 0, 0, "VectorGraph");

	gVectorGraphBuf = (char*)g_sqaure_window.backBuf;
	gVectorGraphWid = g_sqaure_window.id;
	gVectorGraphTid = __kAddExactTimer((DWORD)SqaureAnimation, CMOS_EXACT_INTERVAL * 2, 0, 0, 0, 0);

	gBaseColor = 0;
}



extern "C" __declspec(dllexport) int SquareVector(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD runparam) {

	char szout[1024];

	initFullWindow(&g_sqaure_window, funcname, tid,1);

	initSquareVector();

	while (1) {
		unsigned int ck = __kGetKbd(g_sqaure_window.id);
		unsigned int asc = ck & 0xff;
		if (asc == 0x1b)
		{
			
			break;
		}

		MOUSEINFO mouseinfo;
		__memset((char*)&mouseinfo, 0, sizeof(MOUSEINFO));
		int ret = __kGetMouse(&mouseinfo, g_sqaure_window.id);
		if (mouseinfo.status & 1)
		{
			if (mouseinfo.x >= g_sqaure_window.shutdownx && mouseinfo.x <= g_sqaure_window.shutdownx + g_sqaure_window.capHeight)
			{
				if (mouseinfo.y >= g_sqaure_window.shutdowny && mouseinfo.y <= g_sqaure_window.shutdowny + g_sqaure_window.capHeight)
				{
					break;
				}
			}
			if (mouseinfo.x >= g_sqaure_window.minx && mouseinfo.x <= g_sqaure_window.minx + g_sqaure_window.capHeight)
			{
				if (mouseinfo.y >= g_sqaure_window.miny && mouseinfo.y <= g_sqaure_window.miny + g_sqaure_window.capHeight)
				{
					MinimizeWindow(&g_sqaure_window);
				}
			}
		}

		__sleep(0);
	}

	stopSqaureAnimation();

	return 0;
}