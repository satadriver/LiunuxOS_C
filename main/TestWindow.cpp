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
#include "utils.h"
#include "systemservice.h"
#include "math.h"

#define ALARMER_SECOND_INTERVAL		60




extern "C" __declspec(dllexport) void __MyTestTask(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char szout[1024];

	char cputype[1024];
	getCpuType(cputype);
	char cpuinfo[1024];
	getCpuInfo(cpuinfo);
	__printf(szout, "CPU MODEL:%s,details:%s,SSE:%d,video height:%d,width:%d,pixel:%d\n",
		cputype, cpuinfo, isSSE(), gVideoHeight, gVideoWidth, gBytesPerPixel);

	showAllPciDevs();

	__enableBreakPoint();

	enableSingleStep();

	disableSingleStep();

	enableOverflow();

	__kAddAlarmTimer(ALARMER_SECOND_INTERVAL*5, (DWORD)__doAlarmTask, 0);

	int params[16];
	
	callgateEntry((char*)params, 4);

	SysenterProc((char*)params,3);

	__yield();
	
	//runElfFunction("c:\\liunux\\test.so", "__testfunction");
	
	return;
}


extern "C" __declspec(dllexport) int TestThread0_main(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (1) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}


extern "C" __declspec(dllexport) int TestThread1_main(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];
	while (1) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
		//Halt();
		__asm {
			//hlt
		}
	}

	return 0;
}
extern "C" __declspec(dllexport) int TestThread2_main(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param)
{
	float f1 = PI;
	while (1) {
		f1 = __sinf(f1 / 3);
		if (f1 < 0.00001f && f1 > -0.00001f) {
			f1 = PI;
		}
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestThread3_main(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (1) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}





extern "C" __declspec(dllexport) int TestThread4_main(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (1) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}


extern "C" __declspec(dllexport) int TestThread5_main(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (1) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}


extern "C" __declspec(dllexport) int TestThread6_main(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (1) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}


extern "C" __declspec(dllexport) int TestThread7_main(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (1) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}


extern "C" __declspec(dllexport) int TestThread8_main(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (1) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}


extern "C" __declspec(dllexport) int TestThread9_main(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (1) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}



extern "C" __declspec(dllexport) int TestThread10_main(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (1) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}


extern "C" __declspec(dllexport) int TestThread11_main(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (1) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestThread12_main(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (1) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}


extern "C" __declspec(dllexport) int TestThread13_main(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (1) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestThread14_main(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (1) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}


extern "C" __declspec(dllexport) int TestThread15_main(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (1) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}



extern "C" __declspec(dllexport)int __kTestWindow(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD runparam) {

	char szout[1024];
	int ret = 0;
	__printf(szout, "%s task tid:%x,filename:%s,funcname:%s,param:%x\n", __FUNCTION__, tid, filename, funcname, runparam);

	LPTASKCMDPARAMS taskcmd = (LPTASKCMDPARAMS)runparam;

	WINDOWCLASS window;
	__memset((char*)&window, 0, sizeof(WINDOWCLASS));
	__strcpy(window.caption, funcname);
	initFullWindow(&window, funcname, tid,0);

	readAtapiSector((char*)FLOPPY_DMA_BUFFER, 16, 1);
	__dump((char*)FLOPPY_DMA_BUFFER, 512, 1, (unsigned char*)FLOPPY_DMA_BUFFER + 0x1000);
	__printf(szout,(char*)FLOPPY_DMA_BUFFER + 0x1000);

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




extern "C" __declspec(dllexport) int Process_Test_Main(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (1) {

		__sleep(0);

	}

	return 0;
}