#pragma once

#include "video.h"
#include "Utils.h"
#include "mouse.h"
#include "keyboard.h"
#include "task.h"
#include "math.h"
#include "window.h"
#include "guiHelper.h"
#include "math.h"
#include "cmosPeriodTimer.h"

#define CLOCK_RADIUS_SIZE				0x100

#define CLOCK_CIRCLE_COLOR				0x00ffff

#define CLOCK_NUMBER_COLOR				0

#define CLOCK_HOUR_COLOR				0xff0000

#define CLOCK_MINUTE_COLOR				0xff00

#define CLOCK_SECOND_COLOR				0x00FF

#define HOUR_HAND_SIZE				(CLOCK_RADIUS_SIZE*3/4)

#define MINUTE_HAND_SIZE			(CLOCK_RADIUS_SIZE*4/5)

#define SECOND_HAND_SIZE			(CLOCK_RADIUS_SIZE*9/10)


double getHourAngle(int h,int m) {
	return PI * ( 1.0 / 2.0 -  (2.0 * h / 12.0 + (m/60.0)*(2.0 /12.0) ) );
}

double getMinuteAngle(int m) {
	return PI * ( 1.0/ 2.0 - 2.0 * m / 60.0);
}

double getSecondAngle(int s) {
	return PI * (1.0 / 2.0 - 2.0 * s / 60.0);
}




extern "C" __declspec(dllexport)int __kClock(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD runparam) {
	char szout[1024];
	__printf(szout, "%s task tid:%x,filename:%s,funcname:%s,param:%x\n",__FUNCTION__, tid, filename,funcname,runparam);

	int retvalue = 0;

	LPTASKCMDPARAMS taskcmd = (LPTASKCMDPARAMS)runparam;

	WINDOWCLASS window;
	__memset((char*)&window, 0, sizeof(WINDOWCLASS));
	__strcpy(window.caption, filename);
	initFullWindow(&window, funcname, tid);


	int mx = window.width / 2;
	int my = window.height / 2;

	int r1 = CLOCK_RADIUS_SIZE;
	int r2 = CLOCK_RADIUS_SIZE - 8;
	__drawCircle(mx, my, r1, r2, CLOCK_CIRCLE_COLOR, 0);

	for (int i = 1; i <= 12; i++) {

		double angle = PI * (1.0 / 2.0 - (2.0 * i) / 12.0);
		int dx = (int)(1.0 * r1 * __cos(angle));
		int dy = -(int)(1.0 * r1 * __sin(angle));

		char strnum[16];
		int len = __sprintf(strnum, "%d", i);
		for (int j = 0; j < len; j++) {

			int fx = mx + dx - (len - 1 - j) * GRAPHCHAR_WIDTH - GRAPHCHAR_WIDTH / 2;
			int fy = my + dy - GRAPHCHAR_WIDTH / 2;
			int pos = __getpos(fx, fy);
			char sznum[2];
			sznum[0] = strnum[j];
			sznum[1] = 0;
			__drawGraphChar(sznum, CLOCK_NUMBER_COLOR, pos, window.color);
		}
	}

	char hourBak[0x1000];
	char minuteBak[0x1000];
	char secondBak[0x1000];

	DATETIME dt_old;
	__getDateTime(&dt_old);
	if (dt_old.hour >= 12) {
		dt_old.hour -= 12;
	}

	double ha_old = getHourAngle(dt_old.hour, dt_old.minute);
	double hm_old = getMinuteAngle(dt_old.minute);
	double hs_old = getSecondAngle(dt_old.second);

	int hourx_old = mx + (int)((1.0 * HOUR_HAND_SIZE) * __cos(ha_old));
	int houry_old = my - (int)((1.0 * HOUR_HAND_SIZE) * __sin(ha_old));
	int minutex_old = mx + (int)((1.0 * MINUTE_HAND_SIZE) * __cos(hm_old));
	int minutey_old = my - (int)((1.0 * MINUTE_HAND_SIZE) * __sin(hm_old));
	int secondx_old = mx + (int)(1.0 * SECOND_HAND_SIZE * __cos(hs_old));
	int secondy_old = my - (int)(1.0 * SECOND_HAND_SIZE * __sin(hs_old));

	__drawLine(mx, my, hourx_old, houry_old, 0, CLOCK_HOUR_COLOR, hourBak);
	__drawLine(mx, my, minutex_old, minutey_old, 0, CLOCK_MINUTE_COLOR, minuteBak);
	__drawLine(mx, my, secondx_old, secondy_old, 0, CLOCK_SECOND_COLOR, secondBak);

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

		__sleep(500);

		DATETIME dt;		
		__getDateTime(&dt);
		if (dt.hour >= 12) {
			dt.hour -= 12;
		}

		if (dt.hour != dt_old.hour || dt.minute != dt_old.minute || dt.second != dt_old.second) {

			
			char szdateTime[256];
			char* szdayInWeek = dayOfWeek2str(dt.dayInWeek);
			int datalen = __sprintf(szdateTime, "%d/%d/%d %d:%d:%d %s. Having a good time!", 
				dt.year, dt.month, dt.dayInMonth, dt.hour, dt.minute, dt.second, szdayInWeek);
			int pos = __getpos(mx - datalen*GRAPHCHAR_HEIGHT/2, my - CLOCK_RADIUS_SIZE  - GRAPHCHAR_HEIGHT*8);
			__drawGraphChar(szdateTime, CLOCK_NUMBER_COLOR, pos, window.color);

			double ha = getHourAngle(dt.hour, dt.minute);
			double hm = getMinuteAngle(dt.minute);
			double hs = getSecondAngle(dt.second);

			int hourx = mx + (int)((1.0 * HOUR_HAND_SIZE) * __cos(ha));
			int houry = my - (int)((1.0 * HOUR_HAND_SIZE) * __sin(ha));
			int minutex = mx + (int)((1.0 * MINUTE_HAND_SIZE) * __cos(hm));
			int minutey = my - (int)((1.0 * MINUTE_HAND_SIZE) * __sin(hm));
			int secondx = mx + (int)(1.0 * SECOND_HAND_SIZE * __cos(hs));
			int secondy = my - (int)(1.0 * SECOND_HAND_SIZE * __sin(hs));

			__drawLine(mx, my, secondx_old, secondy_old, 1, (DWORD)secondBak, 0);
			__drawLine(mx, my, minutex_old, minutey_old, 1, (DWORD)minuteBak, 0);
			__drawLine(mx, my, hourx_old, houry_old, 1, (DWORD)hourBak, 0);

			__drawLine(mx, my, hourx, houry, 0, CLOCK_HOUR_COLOR, hourBak);
			__drawLine(mx, my, minutex, minutey, 0, CLOCK_MINUTE_COLOR, minuteBak);
			__drawLine(mx, my, secondx, secondy, 0, CLOCK_SECOND_COLOR, secondBak);

			secondx_old = secondx;
			secondy_old = secondy;
			minutex_old = minutex;
			minutey_old = minutey;
			hourx_old = hourx;
			houry_old = houry;
		}
	}

	return 0;
}

