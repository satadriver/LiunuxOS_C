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
#include "windowclass.h"
#include "Pe.h"
#include "window.h"
#include "ProcessDos.h"
#include "ata.h"
#include "UserUtils.h"
#include "Kernel.h"
#include "sysregs.h"
#include "WindowClass.h"
#include "Utils.h"
#include "paint.h"
#include "malloc.h"
#include "Thread.h"
#include "mouse.h"
#include "pci.h"
#include "window.h"
#include "keyboard.h"
#include "FileManager.h"
#include "descriptor.h"
#include "debugger.h"
#include "gdi.h"
#include "pci.h"
#include "hept.h"
#include "cmosAlarm.h"
#include "elf.h"
#include "v86.h"

#define EXPLORER_TASKNAME	"__kExplorer"


int g_test1Cnt = 0;

int g_test2Cnt = 0;

int g_test3Cnt = 0;

extern "C" __declspec(dllexport) void __taskTest1(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char szout[1024];
	while (g_test1Cnt++ < 10) {
		__printf(szout, "__taskTest1 tid:%d running %d!\r\n",tid, g_test1Cnt);
		__sleep(100);
	}
}

extern "C" __declspec(dllexport) void __taskTest2(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char szout[1024];
	while (g_test2Cnt++ < 10) {
		__printf(szout, "__taskTest2 tid:%d running %d!\r\n",tid, g_test2Cnt);
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

int __kExplorer(unsigned int retaddr, int tid, char * filename, char * funcname, DWORD param) {
	int ret = 0;

	char szout[1024];
	initWindowList();

	__printf(szout, "__kExplorer task retaddr:%x,pid:%x,name:%s,funcname:%s,param:%x\n", retaddr, tid, filename, funcname, param);

	//v86Process(0x4f02, 0, 0, 0x4112, 0, 0, 0, 0, 0x10);

	WINDOWCLASS window;
	initDesktopWindow(&window, EXPLORER_TASKNAME, tid);

	WINDOWCLASS taskbar;
	initTaskbarWindow(&taskbar, filename, tid);

	FILEMAP computer;
	initBigClickItem(&computer, "My Computer", tid,1, gVideoWidth - 2 * gBigFolderWidth, gBigFolderHeight);

	FILEMAP atapi;
	initBigClickItem(&atapi, "CD-ROM", tid,2, gVideoWidth - 2 * gBigFolderWidth, gBigFolderHeight + gBigFolderHeight + gBigFolderHeight);

	FILEMAP floppy;
	initBigClickItem(&floppy, "Floppy", tid,3, gVideoWidth - 2 * gBigFolderWidth,
		gBigFolderHeight + gBigFolderHeight + gBigFolderHeight + gBigFolderHeight + gBigFolderHeight);

	__kDrawWindowsMenu();

	RIGHTMENU menu;
	initWindowsRightMenu(&menu, tid);

	char cputype[1024];
	getCpuType(cputype);
	char cpuinfo[1024];
	getCpuInfo(cpuinfo);
	__printf(szout, "CPU MODEL:%s,details:%s,video height:%d,width:%d,pixel:%d\n", cputype, cpuinfo, gVideoHeight, gVideoWidth, gBytesPerPixel);

	showPciDevs();

	__enableBreakPoint();

	enableOverflow();

	__kAddAlarmTimer(60, (DWORD)__doAlarmTask, 0);

	sysEntryProc();

	callgateEntry(0, 0);

	displayCCPoem();

	int imagesize = getSizeOfImage((char*)MAIN_DLL_BASE);
	DWORD address = getAddrFromName(MAIN_DLL_BASE, "__taskTest1");
	//__kCreateThread((DWORD)address, MAIN_DLL_BASE, (DWORD)0, "__taskTest1");
	//__kCreateProcessFromAddrFunc(MAIN_DLL_SOURCE_BASE, imagesize, "__taskTest1", 3, 0);
	DWORD address2 = getAddrFromName(MAIN_DLL_BASE, "__taskTest2");
	//__kCreateThread((DWORD)address2, MAIN_DLL_BASE, (DWORD)0, "__taskTest2");
	//__kCreateProcessFromAddrFunc(MAIN_DLL_SOURCE_BASE, imagesize, "__taskTest2", 3, 0);
	DWORD address3 = getAddrFromName(MAIN_DLL_BASE, "__taskTest3");
	//__kCreateThread((DWORD)address3, MAIN_DLL_BASE, (DWORD)0, "__taskTest3");
	//__kCreateProcessFromAddrFunc(MAIN_DLL_SOURCE_BASE, imagesize, "__taskTest3", 3, 0);

	//readAtapiSector((char*)FLOPPY_DMA_BUFFER, 16, 1);
	//readFloppySector(0, FLOPPY_DMA_BUFFER, 0, 4);
	//__dump((char*)FLOPPY_DMA_BUFFER, 512, 1, (unsigned char*)FLOPPY_DMA_BUFFER + 0x1000);
	//__drawGraphChars((unsigned char*)FLOPPY_DMA_BUFFER + 0x1000, 0);

	//runElfFunction("c:\\liunux\\test.so", "__testfunction");

	//__drawLine(100, 100, 200, 200, 0);

	//__diamond(400, 400, 200, 7, 0);

	TASKCMDPARAMS taskcmd;
	__memset((char*)&taskcmd, 0, sizeof(TASKCMDPARAMS));

	while (1)
	{
		unsigned int ck = __kGetKbd(window.id);
		//unsigned int ck = __getchar(window.id);
		unsigned int asc = ck & 0xff;
		if (asc == VK_F1)
		{
			if (__findProcessFileName("__kConsole") == FALSE)
			{
				int imagesize = getSizeOfImage((char*)MAIN_DLL_SOURCE_BASE);
				__kCreateProcess(MAIN_DLL_SOURCE_BASE, imagesize, "main.dll", "__kConsole", 3, 0);
			}
			continue;
		}
		else if (asc == VK_F2)
		{
			if (__findProcessFileName("__kLogWatch") == FALSE)
			{
				TASKCMDPARAMS cmd;
				__memset((char*)&cmd, 0, sizeof(TASKCMDPARAMS));
				cmd.cmd = SHOW_SYSTEM_LOG;
				cmd.addr = LOG_BUFFER_BASE;
				cmd.filesize = (DWORD)gLogDataPtr - LOG_BUFFER_BASE;

				DWORD thread = getAddrFromName(MAIN_DLL_BASE, "__kShowWindow");
				return __kCreateThread((DWORD)thread, MAIN_DLL_BASE, (DWORD)&cmd, "__kLogWatch");
				//__kCreateProcess(VSMAINDLL_LOAD_ADDRESS, 0x100000, "main.dll", "__kLogWatch", 3, 0);
			}
			continue;
		}
		else if (asc == VK_F3)
		{
			__createDosCodeProc(gV86VMIEntry, gV86VMISize, "V86VMIEntry");
			continue;
		}

		MOUSEINFO mouseinfo;
		__memset((char*)&mouseinfo, 0, sizeof(MOUSEINFO));
		ret = __kGetMouse(&mouseinfo, window.id);
		if (mouseinfo.status & 1)	//left click
		{
			if (menu.action)
			{
				__restoreMenu(&menu);

				if ((mouseinfo.x > menu.pos.x) && (mouseinfo.x < menu.pos.x + menu.width))
				{
					if (mouseinfo.y > menu.pos.y && mouseinfo.y < menu.pos.y + menu.height)
					{
						int funcno = (mouseinfo.y - menu.pos.y) / GRAPHCHAR_HEIGHT / 2;
						if (funcno > 0 && funcno < menu.validItem)
						{
							DWORD func = menu.funcaddr[funcno];

							int cnt = menu.paramcnt[funcno];

							int stacksize = cnt * 4;

							DWORD * thisparams = (DWORD*)&menu.funcparams[funcno][0];

							__asm {
								mov ecx, cnt
								cmp ecx, 0
								jz _callfunc
								mov esi, thisparams
								add esi, stacksize
								sub esi,4
								_params :
								mov eax, [esi]
								push eax
								sub esi, 4
								loop _params
								_callfunc :
								mov eax, func
								call eax
								add esp, stacksize
							}
						}
					}
				}

				menu.action = 0;
			}

			if (mouseinfo.x >= computer.pos.x && mouseinfo.x < computer.pos.x + computer.frameSize + computer.width)
			{
				if (mouseinfo.y >= computer.pos.y && mouseinfo.y <= computer.pos.y + computer.height + computer.frameSize)
				{
					taskcmd.cmd = UNKNOWN_FILE_SYSTEM;
					__strcpy(taskcmd.filename, "FileMgrHD");

					int imagesize = getSizeOfImage((char*)MAIN_DLL_SOURCE_BASE);

					__kCreateProcess(MAIN_DLL_SOURCE_BASE, imagesize, "main.dll", "__kFileManager", 3, (DWORD)&taskcmd);
				}
			}

			if (mouseinfo.x >= atapi.pos.x && mouseinfo.x < (atapi.pos.x + atapi.frameSize + atapi.width))
			{
				if (mouseinfo.y >= atapi.pos.y && mouseinfo.y <= (atapi.pos.y + atapi.height + atapi.frameSize))
				{
					taskcmd.cmd = CDROM_FILE_SYSTEM;
					__strcpy(taskcmd.filename, "FileMgrISO");
					int imagesize = getSizeOfImage((char*)MAIN_DLL_SOURCE_BASE);
					__kCreateProcess(MAIN_DLL_SOURCE_BASE, imagesize, "main.dll", "__kFileManager", 3, (DWORD)&taskcmd);
				}
			}

			if (mouseinfo.x >= floppy.pos.x && mouseinfo.x < (floppy.pos.x + floppy.frameSize + floppy.width))
			{
				if (mouseinfo.y >= floppy.pos.y && mouseinfo.y <= (floppy.pos.y + floppy.height + floppy.frameSize))
				{
					taskcmd.cmd = FLOPPY_FILE_SYSTEM;
					__strcpy(taskcmd.filename, "FileMgrFllopy");
					int imagesize = getSizeOfImage((char*)MAIN_DLL_SOURCE_BASE);
					__kCreateProcess(MAIN_DLL_SOURCE_BASE, imagesize, "main.dll", "__kFileManager", 3, (DWORD)&taskcmd);
				}
			}
		}
		else if (mouseinfo.status & 2)	//right click
		{
			if (mouseinfo.x > gVideoWidth - TASKBAR_HEIGHT && mouseinfo.x < gVideoWidth)
			{
				if ((mouseinfo.y > gVideoHeight - TASKBAR_HEIGHT) && mouseinfo.y < gVideoHeight)
				{
					menu.pos.x = mouseinfo.x;
					menu.pos.y = mouseinfo.y;
					menu.action = mouseinfo.status;
					__drawRightMenu(&menu);
				}
			}
		}
		else if (mouseinfo.status & 4)	//middle click
		{
// 			menu.pos.x = mouseinfo.x;
// 			menu.pos.y = mouseinfo.y;
// 			menu.action = mouseinfo.status;
		}

		__sleep(0);
	}
	return 0;
}


DWORD isDesktop() {
	LPPROCESS_INFO tss = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
	if (__strcmp(tss->funcname, EXPLORER_TASKNAME) == 0)
	{
		return TRUE;
	}

	return FALSE;
}