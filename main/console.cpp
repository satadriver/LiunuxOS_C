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
#include "systemService.h"
#include "pci.h"
#include "cmd.h"
#include "timer8254.h"



extern "C" __declspec(dllexport) int __kConsole(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	int ret = 0;

	char szout[1024];
	//__printf(szout, "__kConsole task retaddr:%x,pid:%x,name:%s,funcname:%s,param:%x\n",retaddr, tid, filename,funcname,param);

	unsigned char szcmd[MAX_PATH_SIZE];
	__memset((char*)szcmd, 0, MAX_PATH_SIZE);
	int cmdptr = 0;

	WINDOWCLASS window;
	initConsoleWindow(&window, __FUNCTION__, tid);

	TASKCMDPARAMS taskcmd;
	__memset((char*)&taskcmd, 0, sizeof(TASKCMDPARAMS));

	//setCursor( &window.showX, &window.showY, ~window.color);
	window.cursorColor = ~window.color;
	//window.showBakX = window.showX;
	//window.showBakY = window.showY;
	window.tag = 0;
	window.cursorID = __kAdd8254Timer((DWORD)windowCursor, CURSOR_REFRESH_MILLISECONDS,(unsigned long) &window, 0, 0, 0);

	while (1)
	{
		unsigned int asc = __kGetKbd(window.id)&0xff;
		//unsigned int asc = __getchar(window.id);
		if (asc == 8)
		{
			ret = __clearWindowChar(&window);
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
			__drawWindowChars(( char*)sztab, DEFAULT_FONT_COLOR, &window);
		}
		else if (asc == 0x0d)
		{
			window.showX = (window.pos.x + (window.frameSize >> 1));

			window.showY = (window.showY + GRAPHCHAR_HEIGHT * window.zoomin);
			//above bottom - FrameSize>>1
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
			RemoveCursor(&window);
			__DestroyWindow(&window);
			//__terminateTid(tid);
			return 0;
		}
		else if (asc)
		{
			szcmd[cmdptr] = (unsigned char)asc;
			cmdptr++;
			if (cmdptr >= MAX_PATH_SIZE)
			{
				cmdptr = 0;
			}
			szcmd[cmdptr] = 0;

			//ret = putchar((char*)&asc);
			ret = __drawWindowChars(( char*)&asc, CONSOLE_FONT_COLOR, &window);
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
					RemoveCursor(&window);
					//removeCursor();
					__DestroyWindow(&window);
					
					//__terminateTid(tid);
					return 0;
				}
			}
			if (mouseinfo.x >= window.minx && mouseinfo.x <= window.minx + window.capHeight)
			{
				if (mouseinfo.y >= window.miny && mouseinfo.y <= window.miny + window.capHeight)
				{
					MinimizeWindow(&window);
				}
			}
		}
		else if (mouseinfo.status & 4)	//middle click
		{

		}

		__sleep(0);
	}
	return 0;
}













#if 0
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
		
		ret = __DestroyRectWindow(&p, cw, ch, (unsigned char*)gCursorBackup);

		gTag = FALSE;
	}
	else {
		p.x = *gCursorX;
		p.y = *gCursorY + GRAPHCHAR_HEIGHT;
		ret = __drawRectWindow(&p, cw, ch, gCursorColor, (unsigned char*)gCursorBackup);
		gTag = TRUE;
	}

	gPrevX = *gCursorX;
	gPrevY = *gCursorY;

	return 0;
}

#endif


int windowCursor(WINDOWCLASS * w, int p2, int p3, int p4) {

	int ret = 0;

	//WINDOWSINFO* wif = isWindowExist(w->id);

	int ch = GRAPHCHAR_HEIGHT / 2;
	int cw = GRAPHCHAR_WIDTH;

	POINT p;

	if (w->tag) {
		if (w->showBakX != w->showX || w->showBakY != w->showY) {
			p.x = w->showBakX;
			p.y = w->showBakY + GRAPHCHAR_HEIGHT;
		}
		else {
			p.x = w->showX;
			p.y = w->showY + GRAPHCHAR_HEIGHT;
		}

		ret = __DestroyRectWindow(&p, cw, ch, (unsigned char*)w->cursorBuf);

		w->tag = FALSE;
	}
	else {
		p.x = w->showX;
		p.y = w->showY + GRAPHCHAR_HEIGHT;
		ret = __drawRectWindow(&p, cw, ch, w->cursorColor, (unsigned char*)w->cursorBuf);
		w->tag = TRUE;
	}

	w->showBakX = w->showX;
	w->showBakY = w->showY;

	return 0;
}


int RemoveCursor(WINDOWCLASS* w) {

	__kRemoveExactTimer(w->cursorID);

	return 0;
}
