#include "console.h"
#include "video.h"
#include "mouse.h"
#include "keyboard.h"
#include "task.h"
#include "graph.h"
#include "soundBlaster/sbPlay.h"
#include "core.h"
#include "Utils.h"
#include "menu.h"
#include "guihelper.h"
#include "Pe.h"
#include "window.h"
#include "cmosExactTimer.h"
#include "ata.h"

#include "Kernel.h"
#include "mainUtils.h"

#include "Utils.h"
#include "paint.h"
#include "malloc.h"
#include "Thread.h"
#include "servicesProc.h"
#include "pci.h"
#include "cmd.h"








int __kConsole(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	int ret = 0;

	//	char szout[1024];
	// 	__printf(szout, "__kConsole task retaddr:%x,pid:%x,name:%s,funcname:%s,param:%x\n",retaddr, pid, filename,funcname,param);

	unsigned char szcmd[MAX_PATH_SIZE];
	__memset((char*)szcmd, 0, MAX_PATH_SIZE);
	int cmdptr = 0;

	WINDOWCLASS window;
	initConsoleWindow(&window, __FUNCTION__, tid);

	TASKCMDPARAMS taskcmd;
	__memset((char*)&taskcmd, 0, sizeof(TASKCMDPARAMS));

	setCursor( &window.showX, &window.showY, ~window.color);

	while (1)
	{
		unsigned int asc = __kGetKbd(window.id)&0xff;
		//unsigned int ck = __getchar(window.id);
		if (asc == 8)
		{
			ret = __clearChar(&window);
			cmdptr--;
			if (cmdptr <= 0)
			{
				cmdptr = 0;
			}
			szcmd[cmdptr] = 0;
		}
		else if (asc == 9)
		{
			char* sztab = "    ";
			__outputConsole((unsigned char*)sztab, DEFAULT_FONT_COLOR, &window);
		}
		else if (asc == 0x0a)
		{
			window.showX = (window.pos.x + (window.frameSize >> 1));

			window.showY = (window.showY + GRAPHCHAR_HEIGHT * window.zoomin);
			if (window.showY >= window.pos.y + window.height + window.capHeight + (window.frameSize >> 1))
			{
				window.showY = window.pos.y + window.capHeight + (window.frameSize >> 1);
			}

			__cmd((char*)szcmd, &window, filename, tid);

			cmdptr = 0;
			szcmd[cmdptr] = 0;
		}
		else if (asc == 0x1b)
		{
			removeCursor();
			__restoreWindow(&window);
			return 0;
		}
		else if (asc)
		{
			szcmd[cmdptr] = (unsigned char)asc;
			cmdptr++;
			if (cmdptr >= 1024)
			{
				cmdptr = 0;
			}
			szcmd[cmdptr] = 0;

			//ret = putchar((char*)&asc);
			ret = __outputConsole((unsigned char*)&asc, CONSOLE_FONT_COLOR, &window);
		}

		MOUSEINFO mouseinfo;
		__memset((char*)&mouseinfo, 0, sizeof(MOUSEINFO));
		ret = __kGetMouse(&mouseinfo, window.id);
		if (mouseinfo.status & 1)	//left click
		{
			if (mouseinfo.x >= window.shutdownx && mouseinfo.x <= window.shutdownx + window.capHeight)
			{
				if (mouseinfo.y >= window.shutdowny && mouseinfo.y <= window.shutdowny + window.capHeight)
				{
					removeCursor();
					__restoreWindow(&window);
					return 0;

					//__terminatePid(pid);
					//__sleep(-1);
				}
			}
		}
		else if (mouseinfo.status & 4)	//middle click
		{
 			//menu.pos.x = mouseinfo.x;
 			//menu.pos.y = mouseinfo.y;
 			//menu.action = mouseinfo.status;
		}

		__sleep(0);
	}
	return 0;
}



int __outputConsole(unsigned char* font, int color, WINDOWCLASS* window) {

	int resultpos = __outputConsoleStr(font, color, DEFAULT_FONT_COLOR, window);

	window->showX = (resultpos % gBytesPerLine) / gBytesPerPixel;
	window->showY = (resultpos / gBytesPerLine);

	return 0;
}


int __clearChar(WINDOWCLASS* window) {

	window->showX -= GRAPHCHAR_WIDTH * window->zoomin;
	if ((window->showX < window->pos.x + (window->frameSize >> 1)) &&
		(window->showY > window->pos.y + window->capHeight + (window->frameSize >> 1)))
	{
		window->showX = window->pos.x + (window->frameSize >> 1) + window->width - GRAPHCHAR_WIDTH * window->zoomin;

		window->showY -= (GRAPHCHAR_HEIGHT * window->zoomin);
		if (window->showY < window->pos.y + window->capHeight + (window->frameSize >> 1))
		{
			window->showY = window->pos.y + (window->frameSize >> 1) + window->capHeight;
		}
	}
	else if ((window->showX < window->pos.x + (window->frameSize >> 1)) &&
		(window->showY <= window->pos.y + window->capHeight + (window->frameSize >> 1)))
	{
		window->showX = window->pos.x + (window->frameSize >> 1);
		window->showY = window->pos.y + (window->frameSize >> 1) + window->capHeight;
	}

	int showpos = __outputConsoleStr((unsigned char*)" ", DEFAULT_FONT_COLOR, DEFAULT_FONT_COLOR, window);

	return showpos;
}


int __outputConsoleStr(unsigned char* font, int color, int bgcolor, WINDOWCLASS* window) {

	int len = __strlen((char*)font);

	unsigned int pos = __getpos(window->showX, window->showY) + gGraphBase;

	unsigned char* showpos = (unsigned char*)pos;
	unsigned char* keepy = showpos;
	unsigned char* keepx = keepy;

	for (int i = 0; i < len; i++)
	{
		unsigned int ch = font[i];
		if (ch == '\n')
		{
			int posy = (unsigned int)(showpos - gGraphBase) / gBytesPerLine;
			int posx = window->pos.x + (window->frameSize >> 1);

			posy += (GRAPHCHAR_HEIGHT * window->zoomin);
			if (posy >= window->pos.y + window->height + window->capHeight + (window->frameSize >> 1))
			{
				posy = window->pos.y + window->capHeight + (window->frameSize >> 1);
			}
			showpos = (unsigned char*)__getpos(posx, posy) + gGraphBase;

			keepx = showpos;
			keepy = showpos;
			continue;
		}
		else if (ch == '\r')
		{
			int posy = (unsigned int)(showpos - gGraphBase) / gBytesPerLine;
			int posx = window->pos.x + (window->frameSize >> 1);
			showpos = (unsigned char*)__getpos(posx, posy) + gGraphBase;
			keepx = showpos;
			keepy = showpos;
			continue;
		}

		int idx = ch << 3;
		unsigned char* p = (unsigned char*)gFontBase + idx;
		for (int j = 0; j < GRAPHCHAR_HEIGHT; j++)
		{
			unsigned char f = p[j];
			int m = 128;
			for (int k = 0; k < GRAPHCHAR_WIDTH; k++)
			{
				unsigned int c = 0;
				if (f & m)
				{
					c = color;
					for (int n = 0; n < gBytesPerPixel * window->zoomin; n++)
					{
						*showpos = c;
						c = c >> 8;
						showpos++;
					}
				}
				else {
					c = bgcolor;
					for (int n = 0; n < gBytesPerPixel * window->zoomin; n++)
					{
						*showpos = c;
						c = c >> 8;
						showpos++;
					}
				}
				//else {
				//	showpos += gBytesPerPixel*zoomin;
				//}

				m = m >> 1;
			}

			keepx += gBytesPerLine * window->zoomin;
			showpos = keepx;
		}

		keepy = keepy + GRAPHCHAR_WIDTH * gBytesPerPixel * window->zoomin;

		int posx = (((unsigned int)keepy - gGraphBase) % gBytesPerLine) / gBytesPerPixel;
		if (posx >= window->width + window->pos.x + (window->frameSize >> 1))
		{
			posx = (window->pos.x + (window->frameSize >> 1));
			int posy = (unsigned int)(keepy - gGraphBase) / gBytesPerLine;
			posy += (GRAPHCHAR_HEIGHT * window->zoomin);
			if (posy >= window->pos.y + window->height + window->capHeight + (window->frameSize >> 1))
			{
				posy = window->pos.y + window->capHeight + (window->frameSize >> 1);
			}
			keepy = (unsigned char*)__getpos(posx, posy) + gGraphBase;
		}

		keepx = keepy;
		showpos = keepy;
	}
	return (int)(showpos - gGraphBase);
}








int gPrevX = 0;
int gPrevY = 0;

int * gCursorX = 0;

int * gCursorY = 0;

int gCursorColor = 0;

unsigned char *gCursorBackup = 0;

int g_cursorID = 0;

int gTag = 0;


void setCursor( int* x, int* y, unsigned int color) {

	gCursorX = x;
	gCursorY = y;
	gCursorColor = color;
	gCursorBackup = (unsigned char*)CURSOR_GRAPH_BASE;

	//int ch = GRAPHCHAR_HEIGHT / 2;
	//int cw = GRAPHCHAR_WIDTH;
	//POINT p;
	//p.x = *gCursorX ;
	//p.y = *gCursorY + GRAPHCHAR_HEIGHT ;
	//int ret = __drawRectangle(&p, cw, ch, gCursorColor, (unsigned char*)gCursorBackup);
	//gTag = TRUE;

	gPrevX = *gCursorX;
	gPrevY = *gCursorY;

	g_cursorID = __kAddExactTimer((DWORD)drawCursor, CURSOR_REFRESH_MILLISECONDS, 0, 0, 0, 0);
}


int removeCursor() {

	__kRemoveExactTimer(g_cursorID);
	
	return 0;
}

int drawCursor(int p1, int p2, int p3, int p4) {

	int ret = 0;

	int ch = GRAPHCHAR_HEIGHT / 2;
	int cw = GRAPHCHAR_WIDTH;

	POINT p;
	
	if (gTag) {
		if (gPrevX != *gCursorX || gPrevY != *gCursorY) {
			p.x = gPrevX;
			p.y = gPrevY + GRAPHCHAR_HEIGHT;
		}
		else {
			p.x = *gCursorX;
			p.y = *gCursorY + GRAPHCHAR_HEIGHT;
		}
		
		ret = __restoreRectangle(&p, cw, ch, (unsigned char*)gCursorBackup);

		gTag = FALSE;
	}
	else {
		p.x = *gCursorX;
		p.y = *gCursorY + GRAPHCHAR_HEIGHT;
		ret = __drawRectangle(&p, cw, ch, gCursorColor, (unsigned char*)gCursorBackup);
		gTag = TRUE;
	}

	gPrevX = *gCursorX;
	gPrevY = *gCursorY;

	return 0;
}