#pragma once

#include "video.h"
#include "Utils.h"
#include "mouse.h"
#include "keyboard.h"
#include "task.h"
#include "math.h"
#include "window.h"
#include "guiHelper.h"



#define CLOCK_RADIUS_VALUE		0x100

#define CLOCK_COLOR_VALUE		0xffff00



double getHourAngle(int h,int m) {
	return PI * ( 1 / 2 -  (2 * h / 12 + 2*m/60) );
}

double getMinuteAngle(int m) {
	return PI * ( 1/ 2 - 2*m/60);
}

double getSecondAngle(int s) {
	return PI * (1 / 2 - 2 * s / 60);
}

int clockMainProc() {
	int mx = gVideoWidth / 2;
	int my = gVideoHeight / 2;
	__drawCircle(mx, my, CLOCK_RADIUS_VALUE, CLOCK_COLOR_VALUE, 0);

	for (int i = 0; i < 12; i++) {
		double angle = PI * (1 / 2 - 2 * i / 12);

	}
	return 0;
}



int __kClock(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD runparam) {
	char szout[1024];
	__printf(szout, "%s task tid:%x,filename:%s,funcname:%s,param:%x\n",__FUNCTION__, tid, filename,funcname,runparam);

	int retvalue = 0;

	LPTASKCMDPARAMS taskcmd = (LPTASKCMDPARAMS)runparam;

	WINDOWCLASS window;
	__memset((char*)&window, 0, sizeof(WINDOWCLASS));
	__strcpy(window.caption, filename);
	initFullWindow(&window, funcname, tid);

	while (1)
	{
		//unsigned int ck = __getchar(window.id);
		unsigned int ck = __kGetKbd(window.id);
		unsigned int asc = ck & 0xff;
		if (asc == 0x1b)
		{
			__DestroyWindow(&window);
			return 0;
		}

		MOUSEINFO mouseinfo;
		__memset((char*)&mouseinfo, 0, sizeof(MOUSEINFO));
		//retvalue = getmouse(&mouseinfo,window.id);
		retvalue = __kGetMouse(&mouseinfo, window.id);
		if (mouseinfo.status & 1)	//left click
		{
			if (mouseinfo.x >= window.shutdownx && mouseinfo.x <= window.shutdownx + window.capHeight)
			{
				if (mouseinfo.y >= window.shutdowny && mouseinfo.y <= window.shutdowny + window.capHeight)
				{
					__DestroyWindow(&window);
					return 0;
				}
			}
		}

		__sleep(0);
	}
	return 0;
}

