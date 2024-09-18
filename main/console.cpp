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
#include "windowclass.h"
#include "Pe.h"
#include "window.h"
#include "cmosExactTimer.h"
#include "ata.h"
#include "UserUtils.h"
#include "Kernel.h"
#include "sysregs.h"
#include "WindowClass.h"
#include "Utils.h"
#include "paint.h"
#include "malloc.h"
#include "Thread.h"
#include "servicesProc.h"
#include "pci.h"







int __cmd(char* cmd, WINDOWCLASS* window, char* pidname, int pid) {

	//cmd size is always less than 256 bytes
	char szout[0x1000];
	int ret = 0;

	TASKCMDPARAMS taskcmd;
	__memset((char*)&taskcmd, 0, sizeof(TASKCMDPARAMS));

	int cmdlen = __strlen(cmd);
	upper2lower(cmd, cmdlen);

	char params[COMMAND_LINE_STRING_LIMIT][COMMAND_LINE_STRING_LIMIT];
	__memset((char*)params, 0, COMMAND_LINE_STRING_LIMIT * COMMAND_LINE_STRING_LIMIT);

	int paramcnt = parseCmdParams(cmd, params);

	if (__strcmp(params[0], "dump") == 0 && paramcnt >= 2)
	{
		char* filename = params[1];
		int fnlen = __strlen(filename);
		if (__memcmp(filename + fnlen - 4, ".bmp", 4) == 0)
		{
			__strcpy(taskcmd.filename, filename);
			taskcmd.cmd = SHOW_WINDOW_BMP;
			//DWORD addr = getAddrFromName(MAIN_DLL_BASE, "__kShowWindow");
			//return __kCreateThread(addr, (DWORD)&taskcmd, "__kShowWindow");

			int imagesize = getSizeOfImage((char*)MAIN_DLL_SOURCE_BASE);
			return __kCreateProcess(MAIN_DLL_SOURCE_BASE, imagesize, "main.dll", "__kShowWindow", 3, (DWORD)&taskcmd);
		}
		else if (__memcmp(filename + fnlen - 4, ".jpg", 4) == 0 || __memcmp(filename + fnlen - 5, ".jpeg", 5) == 0)
		{
			__strcpy(taskcmd.filename, filename);
			taskcmd.cmd = SHOW_WINDOW_JPEG;
			//DWORD addr = getAddrFromName(MAIN_DLL_BASE, "__kShowWindow");
			//return __kCreateThread(addr, (DWORD)&taskcmd, "__kShowWindow");
			int imagesize = getSizeOfImage((char*)MAIN_DLL_SOURCE_BASE);
			return __kCreateProcess(MAIN_DLL_SOURCE_BASE, imagesize, "main.dll", "__kShowWindow", 3, (DWORD)&taskcmd);
		}
		else if (isTxtFile(filename, fnlen))
		{
			__strcpy(taskcmd.filename, filename);
			taskcmd.cmd = SHOW_WINDOW_TXT;
			//DWORD addr = getAddrFromName(MAIN_DLL_BASE, "__kShowWindow");
			//return __kCreateThread(addr, (DWORD)&taskcmd, "__kShowWindow");
			int imagesize = getSizeOfImage((char*)MAIN_DLL_SOURCE_BASE);
			return __kCreateProcess(MAIN_DLL_SOURCE_BASE, imagesize, "main.dll", "__kShowWindow", 3, (DWORD)&taskcmd);
		}
		else if (__memcmp(filename + fnlen - 4, ".zip", 4) == 0 || __memcmp(filename + fnlen - 4, ".apk", 4) == 0)
		{

		}
		else if (__memcmp(filename + __strlen(filename) - 4, ".exe", 4) == 0 ||
			__memcmp(filename + __strlen(filename) - 4, ".com", 4) == 0 ||
			__memcmp(filename + __strlen(filename) - 4, ".dll", 4) == 0 ||
			__memcmp(filename + __strlen(filename) - 4, ".sys", 4) == 0 ||
			__memcmp(filename + __strlen(filename) - 3, ".so", 3) == 0)
		{
			//dump filename function_name param
			if (paramcnt >= 3)
			{
				if (paramcnt >= 4)
				{
					return __kCreateProcessFromName(filename, params[2], 3, (DWORD)params[3]);
				}
				else {
					return __kCreateProcessFromName(filename, params[2], 3, 0);
				}
			}
			else {
				return __kCreateProcessFromName(filename, filename, 3, 0);
			}
		}
		else if (__memcmp(filename + fnlen - 4, ".wav", 4) == 0)
		{
			return playWavFile(filename);
		}
	}
	else if(__strcmp(params[0], "keyboardID") == 0)
	{
		*szout = 0;
		__sprintf(szout, "keyboard id:%x", gKeyboardID);
		ret = __outputConsole((unsigned char*)&szout, CONSOLE_FONT_COLOR, window);
	}
	else if (__strcmp(params[0], "mouseID") == 0) {
		*szout = 0;
		__sprintf(szout, "mouse id:%x", gMouseID);
		ret = __outputConsole((unsigned char*)&szout, CONSOLE_FONT_COLOR, window);
	}
	else if (__strcmp(params[0], "reg") == 0 && paramcnt >= 2)
	{
		if (__strcmp(params[1], "idt") == 0)
		{
			*szout = 0;
			getidt(szout);
			ret = __outputConsole((unsigned char*)&szout, CONSOLE_FONT_COLOR, window);
		}
		else if (__strcmp(params[1], "gdt") == 0)
		{
			*szout = 0;
			getgdt(szout);
			ret = __outputConsole((unsigned char*)&szout, CONSOLE_FONT_COLOR, window);
		}
		else if (__strcmp(params[1], "ldt") == 0)
		{
			*szout = 0;
			getldt(szout);
			ret = __outputConsole((unsigned char*)&szout, CONSOLE_FONT_COLOR, window);
		}
		else if (__strcmp(params[1], "cr") == 0)
		{
			*szout = 0;
			getcrs(szout);
			ret = __outputConsole((unsigned char*)&szout, CONSOLE_FONT_COLOR, window);
		}
		else if (__strcmp(params[1], "general") == 0)
		{
			*szout = 0;
			getGeneralRegs(szout);
			ret = __outputConsole((unsigned char*)&szout, CONSOLE_FONT_COLOR, window);
		}
	}
	else if (__strcmp(params[0], "loadfiles") == 0)
	{
		DATALOADERINFO* info = (DATALOADERINFO*)(gKernelData<<4);
		__sprintf(szout, 
			"flag:%s,mbr:%d,mbrbak:%d,loaderSec:%d,loaderSecCnt:%d,kSec:%d,kSecCnt:%d,kdllSec:%d,kdllSecCnt:%d,mdllSec:%d,mdllSecCnt:%d,fontSec:%d,fontSecCnt:%d\r\n",
			&info->_flags, info->_bakMbrSecOff, info->_bakMbr2SecOff, info->_loaderSecOff, info->_loaderSecCnt,
			info->_kernelSecOff, info->_kernelSecCnt, info->_kdllSecOff, info->_kdllSecCnt,
			info->_maindllSecOff, info->_maindllSecCnt, info->_fontSecOff, info->_fontSecCnt);
		ret = __outputConsole((unsigned char*)&szout, CONSOLE_FONT_COLOR, window);
	}
	else if (__strcmp(params[0], "segments") == 0)
	{
		__sprintf(szout, "Kernel:%x,Kernel16:%x,KernelData:%x\r\n", gKernel32, gKernel16, gKernelData);
		ret = __outputConsole((unsigned char*)&szout, CONSOLE_FONT_COLOR, window);
		return 0;
	}
	else if (__strcmp(params[0], "threads") == 0)
	{
		*szout = 0;
		getpids(szout);
		ret = __outputConsole((unsigned char*)&szout, CONSOLE_FONT_COLOR, window);
	}
	else if (__strcmp(params[0], "thread") == 0)
	{
		if (paramcnt >= 2)
		{
			int i = __strd2i(params[1]);
			*szout = 0;
			getpid(i, szout);
			ret = __outputConsole((unsigned char*)&szout, CONSOLE_FONT_COLOR, window);
		}
	}
	else if (__strcmp(params[0], "run") == 0)
	{
		if (paramcnt >= 2 && __strcmp(params[1], "paint") == 0)
		{
			__memset((char*)&taskcmd, 0, sizeof(TASKCMDPARAMS));
			int imagesize = getSizeOfImage((char*)MAIN_DLL_SOURCE_BASE);

			return __kCreateProcess(MAIN_DLL_SOURCE_BASE, imagesize, "main.dll", "__kPaint", 3, (DWORD)&taskcmd);
		}
	}
	else if (__strcmp(params[0], "tssStatus") == 0) {
		LPPROCESS_INFO process = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
		DWORD eflags = 0;
		__asm {
			pushfd
			pop eflags
		}
		TssDescriptor* descrptor = (TssDescriptor*)(GDT_BASE + kTssTaskSelector);
	 	__sprintf(szout, "tid:%d, pid:%d ,link:%d,NT:%x,busy:%x\r\n",
			process->tid, process->pid, process->tss.link,process->tss.trap,eflags & 0x4000,descrptor->type & 2);
		ret = __outputConsole((unsigned char*)&szout, CONSOLE_FONT_COLOR, window);
	 	process->tss.link = 0; 	
	}
	else if (__strcmp(params[0], "alloc") == 0)
	{
		if (paramcnt >= 3)
		{
			int size = __strh2i((unsigned char*)params[1]);
			int cnt = __strh2i((unsigned char*)params[2]);
			for (int i = 0; i < cnt; i++)
			{
				DWORD addr = __malloc(size);
				__sprintf(szout, "malloc size:%x,address:%x\r\n", size, addr);
				ret = __outputConsole((unsigned char*)&szout, CONSOLE_FONT_COLOR, window);
			}
		}
		else if (paramcnt >= 2)
		{
			int size = __strh2i((unsigned char*)params[1]);
			DWORD addr = __malloc(size);
			__sprintf(szout, "malloc size:%x,address:%x\r\n", size, addr);
			ret = __outputConsole((unsigned char*)&szout, CONSOLE_FONT_COLOR, window);
		}
	}
	else if (__strcmp(params[0], "free") == 0)
	{
		if (paramcnt >= 2)
		{
			DWORD addr = __strh2i((unsigned char*)params[1]);
			int size = __free(addr);
			__sprintf(szout, "free size:%x,address:%x\r\n", size, addr);
			ret = __outputConsole((unsigned char*)&szout, CONSOLE_FONT_COLOR, window);
		}
	}
	else if (__strcmp(params[0], "dumpm") == 0)
	{
		if (paramcnt >= 2)
		{
			DWORD addr = __strh2i((unsigned char*)params[1]);

			int len = 0x40;

			if (paramcnt >= 3)
			{
				len = __strh2i((unsigned char*)params[2]);
			}
			__dump((char*)addr, len, TRUE, (unsigned char*)szout);

			ret = __outputConsole((unsigned char*)&szout, CONSOLE_FONT_COLOR, window);
		}
	}
	else if (__strcmp(params[0], "editmem") == 0)
	{
		if (paramcnt >= 3)
		{
			DWORD addr = __strh2i((unsigned char*)params[1]);
			__strcpy((char*)addr, params[2]);
		}
	}

	else if (__strcmp(params[0], "memlist") == 0 && paramcnt >= 2)
	{
		int pid = __strh2i((unsigned char*)params[1]);
		*szout = 0;
		int len = getmemmap(pid, szout);
		ret = __outputConsole((unsigned char*)&szout, CONSOLE_FONT_COLOR, window);
	}
	else if (__strcmp(params[0], "testme") == 0)
	{
		__strcpy(taskcmd.filename, params[0]);
		taskcmd.cmd = SHOW_TEST_WINDOW;
		int imagesize = getSizeOfImage((char*)MAIN_DLL_SOURCE_BASE);
		return __kCreateProcess(MAIN_DLL_SOURCE_BASE, imagesize, "main.dll", "__kShowWindow", 3, (DWORD)&taskcmd);
	}
	else if (__strcmp(params[0], "tickcnt") == 0)
	{
		DWORD cnt = *(DWORD*)TIMER0_TICK_COUNT;
		__sprintf(szout, "%x\r\n", cnt);
		ret = __outputConsole((unsigned char*)&szout, CONSOLE_FONT_COLOR, window);
	}
	else if (__strcmp(params[0], "time") == 0)
	{
		__sprintf(szout, "%s\n", (char*)CMOS_DATETIME_STRING);
		ret = __outputConsole((unsigned char*)&szout, CONSOLE_FONT_COLOR, window);
	}
	else if (__strcmp(params[0], "rdtsc") == 0)
	{
		DWORD l = 0;
		DWORD h = 0;
		__asm {
			rdtsc
			mov l, eax
			mov h, edx
		}
		__sprintf(szout, "rdtsc:%x%x\n", h, l);
		ret = __outputConsole((unsigned char*)&szout, CONSOLE_FONT_COLOR, window);
	}
	else if (__strcmp(params[0], "rdpmc") == 0)
	{
		DWORD l = 0;
		DWORD h = 0;
		__asm {
			mov eax, 0
			rdpmc
			mov l, eax
			mov h, edx
		}
		__sprintf(szout, "rdpmc:%x%x\n", h, l);
		ret = __outputConsole((unsigned char*)&szout, CONSOLE_FONT_COLOR, window);
	}
	else if (__strcmp(params[0], "cpu_temprature") == 0)
	{
		DWORD tj = 0;
		DWORD temp = __readTemperature(&tj);
		__sprintf(szout, "tj:%x,temprature:%d\n", temp);
		ret = __outputConsole((unsigned char*)&szout, CONSOLE_FONT_COLOR, window);
	}
	else if (__strcmp(params[0], "exit") == 0)
	{
		__restoreWindow(window);
		return 0;
	}
	else if (__strcmp(params[0], "cleans") == 0)
	{
		__restoreWindow(window);
		//__drawWindow(window, FALSE);
		initConsoleWindow(window, pidname, pid);
	}
	else if (__strcmp(params[0], "resetsystem") == 0)
	{
		__reset();
	}
	else if (__strcmp(params[0], "inp ") == 0 || __strcmp(params[0], "outp ") == 0)
	{
		DWORD n = __strh2i((unsigned char*)params[1]);
		DWORD port = __strh2i((unsigned char*)params[2]);
		if (__strcmp(params[0], "inp ") == 0)
		{
			__asm {
				mov edx, port
				mov eax, n
				out dx, eax
			}
		}
		else if (__strcmp(params[0], "outp ") == 0)
		{
			__asm {
				mov edx, port
				mov eax, n
				out dx, eax
			}
		}
	}
	else if (__strcmp(params[0], "cpuinfo") == 0)
	{
		char cpuinfo[256];
		char cputype[256];
		getCpuInfo(cpuinfo);
		getCpuType(cputype);

		__sprintf(szout, "cpuinfo:%s,cpu type:%s\n", cpuinfo, cputype);
		ret = __outputConsole((unsigned char*)&szout, CONSOLE_FONT_COLOR, window);
	}
	else if (__strcmp(params[0], "pcidev") == 0)
	{
		//showAllPciDevs();
	}
	return 0;
}


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