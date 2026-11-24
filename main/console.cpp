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
#include "cursor.h"
#include "paint.h"
#include "malloc.h"
#include "Thread.h"
#include "systemService.h"
#include "pci.h"
#include "cmd.h"
#include "apictimer.h"

#define MAX_HISTORY_CMD 32

extern "C" __declspec(dllexport) int __kConsole(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	int ret = 0;

	char szout[1024];
	//__printf(szout, "__kConsole task retaddr:%x,pid:%x,name:%s,funcname:%s,param:%x\n",retaddr, tid, filename,funcname,param);

	unsigned char szcmd[MAX_PATH_SIZE];
	__memset((char*)szcmd, 0, MAX_PATH_SIZE);
	int cmdptr = 0;

	char history[MAX_HISTORY_CMD][256];
	int historynum = 0;

	WINDOWCLASS window;
	initConsoleWindow(&window, __FUNCTION__, tid);

	TASKCMDPARAMS taskcmd;
	__memset((char*)&taskcmd, 0, sizeof(TASKCMDPARAMS));

	//setCursor( &window.showX, &window.showY, ~window.color);
	window.cursorColor = ~window.color;
	//window.showBakX = window.showX;
	//window.showBakY = window.showY;
	window.tag = 0;
	window.cursorID = __kAddApicTimer((DWORD)windowCursor, CURSOR_REFRESH_MILLISECONDS,(unsigned long) &window, 0, 0, 0);

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
		else if (asc == 0x0d || asc == 0x0a)
		{
			window.showX = (window.pos.x + (window.frameSize >> 1));

			window.showY = (window.showY + GRAPHCHAR_HEIGHT * window.zoomin);
			//above bottom - FrameSize>>1
			if (window.showY >= window.pos.y + window.height + window.capHeight + (window.frameSize >> 1))
			{
				window.showY = window.pos.y + window.capHeight + (window.frameSize >> 1);
			}

			__strlwr(( char*)szcmd);
			__cmd((char*)szcmd, &window, filename, tid);

			if(historynum >= MAX_HISTORY_CMD)
			{
				historynum = 0;
			}
			__strcpy(history[historynum++], (char*)szcmd);

			cmdptr = 0;
			szcmd[cmdptr] = 0;
		}
		else if (asc == VK_UP) {
			historynum--;
			if(historynum < 0)
			{
				historynum = 0;
			}
			__strcpy((char*)szcmd, history[historynum]);
			ret = __drawWindowChars((char*)szcmd, CONSOLE_FONT_COLOR, &window);
		}
		else if (asc == VK_DOWN ) {
			historynum++;
			if(historynum >= MAX_HISTORY_CMD)
			{
				historynum = MAX_HISTORY_CMD - 1;
			}
			__strcpy((char*)szcmd, history[historynum]);
			ret = __drawWindowChars((char*)szcmd, CONSOLE_FONT_COLOR, &window);		
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

		__sleep_delay(0);
	}
	return 0;
}

