
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
#include "systemService.h"
#include "cmosExactTimer.h"
#include "hardware.h"
#include "device.h"
#include "math.h"
#include "timer8254.h"
#include "guiHelper.h"
#include "SpiralBall.h"



void SpiralAnimation(WINDOWCLASS * window) {
	unsigned char* videoBase = (unsigned char*)GetVideoBase();

	DWORD backsize = gBytesPerPixel * (gVideoWidth) * (gVideoHeight);

	DWORD backGround = __kMalloc(backsize);

	DWORD bufsize = gBytesPerPixel * (SPIRAL_SMALL_CIRCLE_SIZE) * (SPIRAL_SMALL_CIRCLE_SIZE);

	DWORD buf = __kMalloc(bufsize);

	DWORD buf2 = __kMalloc(bufsize);

	DWORD buf3 = __kMalloc(bufsize);

	POINT p;
	p.x = 0;
	p.y = 0;

	//__drawRectWindow(&p, gVideoWidth, gVideoHeight, 0xffffff, (unsigned char*)backGround);
	//DWORD windowid = addWindow(FALSE, 0, 0, 0, "SpiralVectorGraph");

	int cx = gVideoWidth / 2;
	int cy = gVideoHeight / 2;

	__drawLine(0, cy, gVideoWidth - 1, cy, 0, AXIS_COLOR, 0);

	__drawLine(cx, 0, cx, gVideoHeight - 1, 0, AXIS_COLOR, 0);

	int color_cos = 0xff00;
	for (int x = 0; x < gVideoWidth; x++)
	{
		int y = cy - (int)(__cos(1.0 * ((int)x - (int)cx) / 100.0) * 100.0);
		int c = color_cos;
		unsigned char* p = (unsigned char*)__getpos((int)x, y) + (DWORD)videoBase;
		for (int k = 0; k < gBytesPerPixel; k++) {

			p[k] = c & 0xff;
			c = c >> 8;
		}

		if (x - cx == 100) {
			int pos = __getpos((int)x, y);
			__drawGraphChar("y=cos(x)", AXIS_COLOR, pos, 0);
		}
	}


	int color_sin = 0xff0000;
	for (int x = 0; x < gVideoWidth; x++)
	{
		int c = color_sin;
		int y = cy - (int)(__sin(1.0 * ((int)x - (int)cx) / 100.0) * 100.0);

		unsigned char* p = (unsigned char*)__getpos((int)x, y) + (DWORD)videoBase;
		for (int k = 0; k < gBytesPerPixel; k++) {

			p[k] = c & 0xff;
			c = c >> 8;
		}

		if (x - cx == 100) {
			int pos = __getpos((int)x, y);
			__drawGraphChar("y=sin(x)", AXIS_COLOR, pos, 0);
		}
	}

	for (int x = 0; x < gVideoWidth; x++) {
		DWORD y = cy - 100.0 * ((float)(x - cx) * 1.0 / 100.0) * (((float)(x - cx) * 1.0) / 100.0);
		if (y >= 0) {
			DWORD c = 0xff0000;
			unsigned char* ptr = (unsigned char*)__getpos(x, y) + (DWORD)videoBase;
			for (int k = 0; k < gBytesPerPixel; k++) {
				*ptr = c & 0xff;
				c = c >> 8;
				ptr++;
			}
		}

		if (x == 100) {
			int pos = __getpos((int)x, y);
			__drawGraphChar("y=x*x", AXIS_COLOR, pos, 0);
		}
	}

	double A = 1.0;
	double B = 1.0;
	double A2 = 1.5;
	double B2 = 1.5;
	double A3 = 2.0;
	double B3 = 2.0;

	double theta = 0.0;
	double theta2 = 0.0;
	double theta3 = 0.0;

	int color = 0x8000;
	int color2 = 0x800000;
	int color3 = 0x80;

	int oldx = cx + SPIRAL_SMALL_CIRCLE_SIZE * 4;
	int oldy = cy + SPIRAL_SMALL_CIRCLE_SIZE * 4;
	int oldx2 = cx - SPIRAL_SMALL_CIRCLE_SIZE2 * 4;
	int oldy2 = cy - SPIRAL_SMALL_CIRCLE_SIZE2 * 4;
	int oldx3 = cx;
	int oldy3 = cy;
	__drawCircle(oldx, oldy, SPIRAL_SMALL_CIRCLE_SIZE, 0, color, (unsigned char*)buf);

	__drawCircle(oldx2, oldy2, SPIRAL_SMALL_CIRCLE_SIZE2, 0, color2, (unsigned char*)buf2);

	__drawCircle(oldx3, oldy3, SPIRAL_SMALL_CIRCLE_SIZE3, 0, color3, (unsigned char*)buf3);

	while (1)
	{
		unsigned int ck = __kGetKbd(window->id);
		unsigned int asc = ck & 0xff;
		if (asc == 0x1b)
		{
			__kFree(backGround);
			__kFree((DWORD)buf);
			__kFree((DWORD)buf2);
			__kFree((DWORD)buf3);

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
					__kFree(backGround);
					__kFree((DWORD)buf);
					__kFree((DWORD)buf2);
					__kFree((DWORD)buf3);
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

		if (theta >= 64) {
			theta += 0.01;
		}
		else if (theta >= 32) {
			theta += 0.05;
		}
		else {
			theta += 0.1;
		}

		if (theta2 >= 64) {
			theta2 += 0.015;
		}
		else if (theta2 >= 32) {
			theta2 += 0.10;
		}
		else {
			theta2 += 0.15;
		}

		if (theta3 >= 64) {
			theta3 += 0.02;
		}
		else if (theta3 >= 32) {
			theta3 += 0.15;
		}
		else {
			theta3 += 0.2;
		}
		color += 1;
		color2 += 1;
		color3 += 1;

		int px = cx + (int)((A + B * theta) * __cos(theta));
		int py = cy - (int)((A + B * theta) * __sin(theta));

		int px2 = cx + (int)((A2 + B2 * theta2) * __cos(theta2));
		int py2 = cy - (int)((A2 + B2 * theta2) * __sin(theta2));

		int px3 = cx + (int)((A3 + B3 * theta3) * __cos(theta3));
		int py3 = cy - (int)((A3 + B3 * theta3) * __sin(theta3));

		__restoreCircle(oldx3, oldy3, SPIRAL_SMALL_CIRCLE_SIZE3, 0, (unsigned char*)buf3);
		__restoreCircle(oldx2, oldy2, SPIRAL_SMALL_CIRCLE_SIZE2, 0, (unsigned char*)buf2);
		__restoreCircle(oldx, oldy, SPIRAL_SMALL_CIRCLE_SIZE, 0, (unsigned char*)buf);

		if ((px >= SPIRAL_SMALL_CIRCLE_SIZE && px <= gVideoWidth - SPIRAL_SMALL_CIRCLE_SIZE) &&
			(py >= SPIRAL_SMALL_CIRCLE_SIZE && py <= gVideoHeight - SPIRAL_SMALL_CIRCLE_SIZE)) {

		}
		else {
			px = cx;
			py = cy;

			theta = 0;
		}

		if ((px2 >= SPIRAL_SMALL_CIRCLE_SIZE2 && px2 <= gVideoWidth - SPIRAL_SMALL_CIRCLE_SIZE2) &&
			(py2 >= SPIRAL_SMALL_CIRCLE_SIZE2 && py2 <= gVideoHeight - SPIRAL_SMALL_CIRCLE_SIZE2)) {

		}
		else {
			px2 = cx;
			py2 = cy;

			theta2 = 0;
		}


		if ((px3 >= SPIRAL_SMALL_CIRCLE_SIZE3 && px3 <= gVideoWidth - SPIRAL_SMALL_CIRCLE_SIZE3) &&
			(py3 >= SPIRAL_SMALL_CIRCLE_SIZE3 && py3 <= gVideoHeight - SPIRAL_SMALL_CIRCLE_SIZE3)) {

		}
		else {
			px3 = cx;
			py3 = cy;

			theta3 = 0;
		}

		__drawCircle(px, py, SPIRAL_SMALL_CIRCLE_SIZE, 0, color, (unsigned char*)buf);

		__drawCircle(px2, py2, SPIRAL_SMALL_CIRCLE_SIZE2, 0, color2, (unsigned char*)buf2);

		__drawCircle(px3, py3, SPIRAL_SMALL_CIRCLE_SIZE3, 0, color3, (unsigned char*)buf3);

		oldx = px;
		oldy = py;
		oldx2 = px2;
		oldy2 = py2;
		oldx3 = px3;
		oldy3 = py3;
	}
	return ;
}


extern "C" __declspec(dllexport)int SpiralBall(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD runparam) {

	char szout[256];

	int retvalue = 0;

	WINDOWCLASS window;

	initFullWindow(&window, funcname, tid,1);

	SpiralAnimation(&window);

	__DestroyWindow(&window);

	return 0;
}

