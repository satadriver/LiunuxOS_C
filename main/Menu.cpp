#include "menu.h"
#include "window.h"
#include "mouse.h"
#include "video.h"
#include "Utils.h"
#include "screenProtect.h"
#include "console.h"
#include "paint.h"
#include "malloc.h"
#include "cmosAlarm.h"
#include "process.h"
#include "atapi.h"
#include "v86.h"

int gMenuID = 0;

int __restoreRightMenu(RIGHTMENU* menu) {

	__kRestoreMouse();

	int startpos = menu->pos.y * gBytesPerLine + menu->pos.x * gBytesPerPixel + gGraphBase;
	unsigned char * ptr = (unsigned char*)startpos;
	unsigned char * keep = ptr;
	unsigned char * srcdata = (unsigned char *)menu->backGround;

	for (int i = 0; i <= menu->height; i++)
	{
		for (int j = 0; j <= menu->width; j++)
		{
			for (int k = 0; k < gBytesPerPixel; k++)
			{
				*ptr = *srcdata;
				ptr++;
				srcdata++;
			}
		}

		keep += gBytesPerLine;
		ptr = (unsigned char*)keep;
	}

	menu->id = 0;

	__kRefreshMouseBackup();

	__kDrawMouse();

	__kFree(menu->backGround);

	return (int)ptr - gGraphBase;
}



int __drawRightMenu(RIGHTMENU *menu) {
	__kRestoreMouse();

	if (menu->pos.x + menu->width + TASKBAR_HEIGHT >= gVideoWidth)
	{
		menu->pos.x = gVideoWidth - menu->width - TASKBAR_HEIGHT;
	}

	if (menu->pos.y + menu->height + TASKBAR_HEIGHT >= gVideoHeight)
	{
		menu->pos.y = gVideoHeight - menu->height - TASKBAR_HEIGHT;
	}

	menu->backsize = gBytesPerPixel*(menu->width + 1)*(menu->height + 1);
	menu->backGround = __kMalloc(menu->backsize);

	menu->id = gMenuID;
	gMenuID++;

	int startpos = __getpos(menu->pos.x, menu->pos.y) + gGraphBase;
	unsigned char * ptr = (unsigned char*)startpos;
	unsigned char * keep = ptr;
	unsigned char * save = (unsigned char*)menu->backGround;

	for (int i = 0; i <= menu->height; i++)	//height
	{
		for (int j = 0; j <= menu->width; j++)		//width
		{
			unsigned int c = menu->color;

			if (i % (GRAPHCHAR_WIDTH * 2) == 0 || j % menu->width == 0)
			{
				c = 0;
			}
			else {
				c = menu->color;
			}

			for (int k = 0; k < gBytesPerPixel; k++)
			{
				*save = *ptr;
				save++;

				*ptr = (c & 0xff);
				ptr++;
				c = (c >> 8);
			}
		}

		keep += gBytesPerLine;
		ptr = (unsigned char*)keep;
	}

	for (int i = 0; i < menu->height / GRAPHCHAR_WIDTH / 2; i++)
	{
		if (menu->menuname[i][0] == 0)
		{
			break;
		}

		__drawGraphChar(( char*)menu->menuname[i], 0, startpos - gGraphBase, 0);
		startpos += GRAPHCHAR_HEIGHT*gBytesPerLine * 2;
	}

	__kRefreshMouseBackup();
	__kDrawMouse();

	return 0;
}

extern "C" __declspec(dllexport) int __kDrawWindowsMenu() {

	__kRestoreMouse();

	int startpos = __getpos(gVideoWidth - TASKBAR_HEIGHT, gVideoHeight - TASKBAR_HEIGHT) + gGraphBase;
	unsigned char * ptr = (unsigned char*)startpos;
	unsigned char * keep = ptr;

	int firstcolor = 0xff0000;
	int secondcolor = 0x00ff00;
	int thirdcolor = 0x0000ff;
	int fourthcolor = 0xffff00;

	for (int i = 0; i < TASKBAR_HEIGHT; i++)	//height
	{
		for (int j = 0; j < TASKBAR_HEIGHT; j++)		//width
		{
			int c = 0;

			if ((i < TASKBAR_HEIGHT / 2) && (j < TASKBAR_HEIGHT / 2))
			{
				c = firstcolor;
			}
			else if ((i < TASKBAR_HEIGHT / 2) && (j > TASKBAR_HEIGHT / 2))
			{
				c = secondcolor;
			}
			else if ((i > TASKBAR_HEIGHT / 2) && (j < TASKBAR_HEIGHT / 2))
			{
				c = thirdcolor;
			}
			else if ((i > TASKBAR_HEIGHT / 2) && (j > TASKBAR_HEIGHT / 2))
			{
				c = fourthcolor;
			}
			else {
				c = 0xffffff;
			}

			for (int k = 0; k < gBytesPerPixel; k++)
			{
				*ptr = (c & 0xff);
				ptr++;
				c = (c >> 8);
			}
		}

		keep += gBytesPerLine;
		ptr = (unsigned char*)keep;
	}
	__kRefreshMouseBackup();
	__kDrawMouse();

	return (int)ptr - gGraphBase;
}


void initRightMenu(RIGHTMENU * menu,int tid) {
	__memset((char*)menu, 0, sizeof(RIGHTMENU));

	__strcpy(menu->name, "WindowsRightClickMenu");
	menu->color = FOLDERFONTBGCOLOR;
	menu->height = RIGHTCLICK_MENU_HEIGHT;
	menu->width = RIGHTCLICK_MENU_WIDTH;
	__strcpy(menu->menuname[0], "Shutdown System");
	__strcpy(menu->menuname[1], "Reset System");
	__strcpy(menu->menuname[2], "Screen Protect");
	__strcpy(menu->menuname[3], "cmd");
	__strcpy(menu->menuname[4], "Paint");
	__strcpy(menu->menuname[5], "Screen Color");
	__strcpy(menu->menuname[6], "Reject CDROM");
	__strcpy(menu->menuname[7], "Vector Graph");
	__strcpy(menu->menuname[8], "Trajectory Ball");
	menu->menuname[9][0] = 0;

	menu->funcaddr[0] = (DWORD)__shutdownSystem;
	menu->funcaddr[1] = (DWORD)__reset;
	menu->funcaddr[2] = (DWORD)initScreenProtect;
	menu->funcaddr[3] = (DWORD)__kConsole;
	menu->funcaddr[4] = (DWORD)__kPaint;
	menu->funcaddr[5] = (DWORD)refreshScreenColor;
	menu->funcaddr[6] = (DWORD)rejectCDROM;
	menu->funcaddr[7] = (DWORD)initVectorGraph;
	menu->funcaddr[8] = (DWORD)initTrajectory;
	menu->validItem = 9;

	menu->paramcnt[0] = 0;
	menu->paramcnt[1] = 0;
	menu->paramcnt[2] = 0;
	menu->paramcnt[3] = 5;
	menu->paramcnt[4] = 5;
	menu->paramcnt[5] = 0;
	menu->paramcnt[6] = 1;
	menu->paramcnt[7] = 0;
	menu->paramcnt[8] = 0;

	menu->funcparams[3][4] = 0;
	menu->funcparams[3][3] = (DWORD)"__kConsole";
	menu->funcparams[3][2] = (DWORD)"main.dll";
	menu->funcparams[3][1] = tid;
	menu->funcparams[3][0] = (DWORD)__terminateProcess;

	menu->funcparams[4][4] = 0;
	menu->funcparams[4][3] = (DWORD)"__kPaint";
	menu->funcparams[4][2] = (DWORD)"main.dll";
	menu->funcparams[4][1] = tid;
	menu->funcparams[4][0] = (DWORD)__terminateProcess;

	menu->funcparams[5][0] = 0;

	menu->funcparams[6][0] = 0x81;

	menu->funcparams[7][0] = 0;
	menu->funcparams[8][0] = 0;

	menu->id = 0;

	menu->tid = tid;

	__kDrawWindowsMenu();
}
