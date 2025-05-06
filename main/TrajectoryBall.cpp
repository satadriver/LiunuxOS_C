

#pragma once

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
#include "trajectoryBall.h"



#define ANGLE_DIVISION  30

char* gTrajectBuf = 0;
char* g_circle_buf = 0;
int gTrajectWid = 0;
int gTrajectTid = 0;

double g_x_s = 0;
double g_y_s = 0;
double GRAVITY_ACC = 9.80 * 4;
double g_centerX = 0;
double g_centerY = 0;
double g_radius = 32.0;

double g_frame_delay = 0;

int g_counter = 0;
int g_circle_color = 0xffffff;

WINDOWCLASS g_tb_window;





//F = 1/2 * ro * v*v * s * 1300*c
double resist_air(double v, double radius) {
	double t = __abs((v * v * 0.67 * __sqrt(radius) / 1226.0 / 2.0));
#if 0
	double min = 0.5 * 1000.0 / g_frame_delay;
	if (t < min) {
		t = min;
	}
#endif
	return t;
}

double resist_bounce(double v, double radius) {
	double r = -v / 2.0;
#if 0
	if (r < -1) {
		r = r - 1;
	}
	else if (r > 1) {
		r = r + 1;
	}
	else {
		if (r >= 0 && r <= 1) {
			if (r >= 0.5) {
				r += 0.5;
			}
			else {
				r = 0;
			}
		}
		else if (r < 0 && r >= -1) {
			if (r <= -0.5) {
				r -= 0.5;
			}
			else {
				r = 0;
			}
		}
		else {
			r = 0;
		}
	}
#endif
	return r;
}


double friction(double v, double radius) {
	double r = __abs(v / 4.0);
#if 0
	double min = 0.1 * 1000.0 / g_frame_delay;
	if (r > min) {
		r += min;
	}
	else {
		r += min;
		//r = r*2;
		//if (r > 0.5) {
		//	r += 0.5;
		//}
		//else {
		//	r = 0;
		//}
	}
#endif

	return r;
}


void stopTrajectoryBall() {
	int ret = 0;
#ifdef USE_CMOS_EXACT_TIMER
	__kRemoveExactTimer(gTrajectTid);
#else
	__kRemove8254Timer(gTrajectTid);
#endif

	//removeWindow(gTrajectWid);
	//POINT p;
	//p.x = 0;
	//p.y = 0;
	//int color = 0;
	//__DestroyRectWindow(&p, gVideoWidth, gVideoHeight, (unsigned char*)gTrajectBuf);
	//__kFree((DWORD)gTrajectBuf);

	__DestroyWindow(&g_tb_window);

	__kFree((DWORD)g_circle_buf);
	return;
}



void TrajectoryAnimation(DWORD p1, DWORD p2, DWORD p3, DWORD p4) {
	int ret = 0;

	char szout[1024];

	double dx = resist_air(g_x_s, g_radius) * g_frame_delay / 1000.0;
	if (__abs(g_centerY - ((double)gVideoHeight - (double)g_radius)) <= 1.0) {
		dx += friction(g_x_s, g_radius) * g_frame_delay / 1000.0;
	}

	double gy = GRAVITY_ACC * g_frame_delay / 1000.0;
	double ry = resist_air(g_y_s, g_radius) * g_frame_delay / 1000.0;
	if (g_y_s > 0)
	{
		g_y_s = g_y_s - gy - ry;
		if (g_y_s < 0)
		{
			//g_y_s = 0;
		}
	}
	else {
		g_y_s = g_y_s - gy + ry;
		if (g_y_s < 0)
		{
			//g_y_s = 0;
		}
	}

	__int64 max_y = (__int64)((double)gVideoHeight - g_radius);
	if (__abs(g_y_s) < 0.1) {
		//g_y_s = 0;
	}

	if (g_x_s < 0) {
		g_x_s = g_x_s + dx;
		if (g_x_s > 0) {
			g_x_s = 0;
		}
	}
	else {
		g_x_s = g_x_s - dx;
		if (g_x_s < 0) {
			g_x_s = 0;
		}
	}

	double x = g_centerX + g_x_s;

	double y = g_centerY - g_y_s;

	if (x + g_radius >= gVideoWidth) {

		g_x_s = resist_bounce(g_x_s, g_radius);
		x = (double)gVideoWidth - g_radius;
		if (g_x_s > 0) {
			g_x_s = -g_x_s;
		}
	}
	else if (x - g_radius <= 0) {

		g_x_s = resist_bounce(g_x_s, g_radius);
		x = g_radius;
		if (g_x_s < 0) {
			g_x_s = -g_x_s;
		}
	}

	if (y + g_radius >= gVideoHeight) {

		g_y_s = resist_bounce(g_y_s, g_radius);
		y = (double)gVideoHeight - g_radius;
		if (g_y_s < 0) {
			g_y_s = -g_y_s;
		}
	}
	else if (y - g_radius <= 0) {

		g_y_s = resist_bounce(g_y_s, g_radius);
		y = g_radius;
		if (g_y_s > 0) {
			g_y_s = -g_y_s;
		}
	}

	if (x == g_centerX && y == g_centerY)
	{

	}
	else {

	}

	ret = __restoreCircle((int)g_centerX, (int)g_centerY, (int)g_radius, (int)g_radius / 2, (unsigned char*)g_circle_buf);
	g_centerX = x;
	g_centerY = y;
	ret = __drawCircle((int)g_centerX, (int)g_centerY, (int)g_radius, (int)g_radius / 2, g_circle_color, (unsigned char*)g_circle_buf);

	if (__abs(g_y_s) <= 0.5 && __abs(g_x_s) <= 0.5 && (__abs(y - max_y) < 0.1 || __abs(g_centerY - max_y) < 0.1)) {
		g_counter++;
		if (g_counter > 2000.0 / g_frame_delay) {
			g_counter = 0;
			ret = __restoreCircle((int)g_centerX, (int)g_centerY, (int)g_radius, (int)g_radius / 2, (unsigned char*)g_circle_buf);

			double max_speed = ((double)1000.0 / (double)g_frame_delay) * GRAVITY_ACC * 10.0;

			__int64 ims = (__int64)max_speed;
			if (ims == 0) {
				__printf(szout, "max speed:%x,%lf\r\n", ims, max_speed);
				ims = 3000;
			}

			double velocity = (double)(__random(0) % (__int64)ims) + (1000.0 / (double)g_frame_delay) * 10.0;

			velocity = velocity * g_frame_delay / 1000.0;

			double angle = __random(0) % ANGLE_DIVISION;
			angle = PI / (angle + 1);
			if (angle <= PI / 12) {
				angle += PI / 12;
			}
			else if (angle >= PI * 11 / 12) {
				angle -= PI / 12;
			}

			g_x_s = __cos(angle) * velocity;
			g_y_s = __sin(angle) * velocity;

			//g_centerY = (double)((__int64)gVideoHeight - (__int64)g_radius - (__int64)TASKBAR_HEIGHT * 2);

			//g_centerX = (double)g_radius + TASKBAR_HEIGHT;

			ret = __drawCircle((int)g_centerX, (int)g_centerY, (int)g_radius, (int)g_radius / 2, g_circle_color, (unsigned char*)g_circle_buf);
		}
	}
	else {

	}

	__sprintf(szout, "(X:%f,Y:%f) (XS:%f,YS:%f)        ", g_centerX, g_centerY, g_x_s, g_y_s);
	int showPos = __getpos(0, gVideoHeight - TASKBAR_HEIGHT * 2);
	__drawGraphChar(szout, OUTPUT_INFO_COLOR, showPos, g_tb_window.color);

}




void TrajectoryBallInit() {
	int ret = 0;
	char szout[1024];

	DWORD backsize = gBytesPerPixel * (gVideoWidth) * (gVideoHeight);

	//gTrajectBuf = (char*)__kMalloc(backsize);
	gTrajectBuf = (char*)g_tb_window.backBuf;

	g_circle_color = ~g_tb_window.color;

	g_circle_buf = (char*)__kMalloc((int)(g_radius + 4) * 2 * 2 * (int)(g_radius + 4) * gBytesPerPixel);

	//gTrajectWid = addWindow(FALSE, 0, 0, 0, "Trajectory");
	gTrajectWid = g_tb_window.id;

	//POINT p;
	//p.x = 0;
	//p.y = 0;
	//int color = 0;
	//__drawRectWindow(&p, gVideoWidth, gVideoHeight, color, (unsigned char*)gTrajectBuf);

#ifdef USE_CMOS_EXACT_TIMER
	g_frame_delay = (double)CMOS_EXACT_INTERVAL * 2.0;
	gTrajectTid = __kAddExactTimer((DWORD)TrajectoryProc, (int)g_frame_delay, 0, 0, 0, 0);
#else
	g_frame_delay = (double)CMOS_EXACT_INTERVAL * 2.0;
	gTrajectTid = __kAdd8254Timer((DWORD)TrajectoryAnimation, (int)g_frame_delay, 0, 0, 0, 0);
#endif

	double max_speed = ((double)1000.0 / (double)g_frame_delay) * GRAVITY_ACC * 10.0;

	__int64 ims = (__int64)max_speed;
	if (ims == 0) {
		__printf(szout, "max speed:%x,%lf\r\n", ims, max_speed);
		ims = 3000;
	}

	double velocity = (double)(__random(0) % (__int64)ims) + ((double)1000.0 / (double)g_frame_delay) * 10.0;

	velocity = velocity * g_frame_delay / 1000.0;

	double angle = __random(0) % ANGLE_DIVISION;
	angle = PI / (angle + 1);
	if (angle <= PI / 12) {
		angle += PI / 12;
	}
	else if (angle >= PI * 11 / 12) {
		angle -= PI / 12;
	}

	//g_x_s = GetCos(angle) * velocity / 256;
	//g_y_s = GetSin(angle) * velocity/256;

	g_x_s = __cos(angle) * velocity;
	g_y_s = __sin(angle) * velocity;

	g_centerY = (double)((__int64)gVideoHeight - (__int64)g_radius - (__int64)TASKBAR_HEIGHT * 4);

	g_centerX = (double)g_radius + (__int64)gVideoWidth / 4;

	g_counter = 0;

	ret = __drawCircle((int)g_centerX, (int)g_centerY, (int)g_radius, (int)g_radius / 2, g_circle_color, (unsigned char*)g_circle_buf);

	__sprintf(szout, "(X:%f,Y:%f) (XS:%f,YS:%f)        ", g_centerX, g_centerY, g_x_s, g_y_s);
	int showPos = __getpos(0, gVideoHeight - TASKBAR_HEIGHT * 2);
	__drawGraphChar(szout, OUTPUT_INFO_COLOR, showPos, g_tb_window.color);
}





extern "C" __declspec(dllexport)int TrajectoryBall(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD runparam) {

	char szout[1024];

	int retvalue = 0;	

	initFullWindow(&g_tb_window, funcname, tid,1);

	TrajectoryBallInit();

	while (1) {
		unsigned int asc = __kGetKbd(gTrajectWid) & 0xff;
		if (asc == 0x1b)
		{
			break;
		}

		MOUSEINFO mouseinfo;
		__memset((char*)&mouseinfo, 0, sizeof(MOUSEINFO));
		__kGetMouse(&mouseinfo, gTrajectWid);
		if (mouseinfo.status & 1)	//left click
		{
			if (mouseinfo.x >= g_tb_window.shutdownx && mouseinfo.x <= g_tb_window.shutdownx + g_tb_window.capHeight)
			{
				if (mouseinfo.y >= g_tb_window.shutdowny && mouseinfo.y <= g_tb_window.shutdowny + g_tb_window.capHeight)
				{
					break;
				}
			}

			if (mouseinfo.x >= g_tb_window.minx && mouseinfo.x <= g_tb_window.minx + g_tb_window.capHeight)
			{
				if (mouseinfo.y >= g_tb_window.miny && mouseinfo.y <= g_tb_window.miny + g_tb_window.capHeight)
				{
					MinimizeWindow(&g_tb_window);
				}
			}
		}

		__sleep(0);
	}

	stopTrajectoryBall();

	return 0;
}