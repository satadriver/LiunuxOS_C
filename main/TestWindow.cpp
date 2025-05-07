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
#include "TestWindow.h"
#include "def.h"
#include "atapi.h"
#include "Explorer.h"
#include "console.h"
#include "video.h"
#include "mouse.h"
#include "keyboard.h"
#include "task.h"
#include "graph.h"
#include "soundBlaster/sbPlay.h"
#include "floppy.h"
#include "Utils.h"
#include "menu.h"

#include "Pe.h"
#include "window.h"
#include "ProcessDos.h"
#include "ata.h"

#include "Kernel.h"
#include "mainUtils.h"

#include "Utils.h"
#include "paint.h"
#include "malloc.h"
#include "Thread.h"
#include "mouse.h"
#include "pci.h"
#include "window.h"
#include "keyboard.h"
#include "FileBrowser.h"
#include "descriptor.h"
#include "debugger.h"
#include "gdi.h"
#include "pci.h"
#include "hept.h"
#include "cmosAlarm.h"
#include "elf.h"
#include "guihelper.h"
#include "coprocessor.h"

#define ALARMER_SECOND_INTERVAL		60




extern "C" __declspec(dllexport) void __MyTestTask(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char szout[1024];

	char cputype[1024];
	getCpuType(cputype);
	char cpuinfo[1024];
	getCpuInfo(cpuinfo);
	__printf(szout, "CPU MODEL:%s,details:%s,SSE:%d,video height:%d,width:%d,pixel:%d\n",
		cputype, cpuinfo, isSSE(), gVideoHeight, gVideoWidth, gBytesPerPixel);

	showPciDevs();

	__enableBreakPoint();

	enableSingleStep();

	disableSingleStep();

	enableOverflow();

	__kAddAlarmTimer(ALARMER_SECOND_INTERVAL, (DWORD)__doAlarmTask, 0);

	sysEntryProc();

	callgateEntry(0, 0);

	//runElfFunction("c:\\liunux\\test.so", "__testfunction");

	//return ;

	//while (1) 
	{
		__sleep(0);
	}

	return;
}

int g_test1Cnt = 0;

int g_test2Cnt = 0;

int g_test3Cnt = 0;

extern "C" __declspec(dllexport) void __taskTest1(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char szout[1024];
	while (g_test1Cnt++ < 10) {
		__printf(szout, "__taskTest1 tid:%d running %d!\r\n", tid, g_test1Cnt);
		__sleep(100);
	}
}

extern "C" __declspec(dllexport) void __taskTest2(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char szout[1024];
	while (g_test2Cnt++ < 10) {
		__printf(szout, "__taskTest2 tid:%d running %d!\r\n", tid, g_test2Cnt);
		__sleep(100);
	}
}

extern "C" __declspec(dllexport) void __taskTest3(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char szout[1024];
	while (g_test3Cnt++ < 10) {
		__printf(szout, "__taskTest3 tid:%d running %d!\r\n", tid, g_test3Cnt);
		__sleep(100);
	}
}


extern "C" __declspec(dllexport)int __kTestWindow(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD runparam) {

	char szout[1024];
	int ret = 0;
	__printf(szout, "%s task tid:%x,filename:%s,funcname:%s,param:%x\n", __FUNCTION__, tid, filename, funcname, runparam);

	LPTASKCMDPARAMS taskcmd = (LPTASKCMDPARAMS)runparam;

	WINDOWCLASS window;
	__memset((char*)&window, 0, sizeof(WINDOWCLASS));
	__strcpy(window.caption, filename);
	initFullWindow(&window, funcname, tid,0);

	DWORD address = getAddrFromName(MAIN_DLL_BASE, "__taskTest1");
	//__kCreateThread((DWORD)address, MAIN_DLL_BASE, (DWORD)0, "__taskTest1");
	//__kCreateProcessFromAddrFunc(MAIN_DLL_SOURCE_BASE, imagesize, "__taskTest1", 3, 0);
	DWORD address2 = getAddrFromName(MAIN_DLL_BASE, "__taskTest2");
	//__kCreateThread((DWORD)address2, MAIN_DLL_BASE, (DWORD)0, "__taskTest2");
	//__kCreateProcessFromAddrFunc(MAIN_DLL_SOURCE_BASE, imagesize, "__taskTest2", 3, 0);
	DWORD address3 = getAddrFromName(MAIN_DLL_BASE, "__taskTest3");
	//__kCreateThread((DWORD)address3, MAIN_DLL_BASE, (DWORD)0, "__taskTest3");
	//__kCreateProcessFromAddrFunc(MAIN_DLL_SOURCE_BASE, imagesize, "__taskTest3", 3, 0);

	readAtapiSector((char*)FLOPPY_DMA_BUFFER, 16, 1);
	__dump((char*)FLOPPY_DMA_BUFFER, 512, 1, (unsigned char*)FLOPPY_DMA_BUFFER + 0x1000);
	__printf(szout,(char*)FLOPPY_DMA_BUFFER + 0x1000);



	for (int i = 0; i < 0x20; i++) {
		int size = 0x10 * i + 8;
		char* buf =(char*) __malloc(size);
		__printf(szout,"buf :%x,size:%x\r\n", buf, size);
	}

	for (int i = 0; i < 0x20; i++) {
		int size = 0x10000 * i + 8;
		char* buf = (char*)__malloc(size);
		__printf(szout,"buf :%x,size:%x\r\n", buf, size);
	}

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
		ret = __kGetMouse(&mouseinfo, window.id);
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

