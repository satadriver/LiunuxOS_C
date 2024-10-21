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
#include "coprocessor.h"
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

#define EXPLORER_TASKNAME			"__kExplorer"

#define ALARMER_SECOND_INTERVAL		60



int __kExplorer(unsigned int retaddr, int tid, char * filename, char * funcname, DWORD param) {
	int ret = 0;

	char szout[1024];

	//initWindowList();

	__printf(szout, "__kExplorer task retaddr:%x,pid:%x,name:%s,funcname:%s,param:%x\n", retaddr, tid, filename, funcname, param);

	//v86Process(0x4f02, 0, 0, 0x4112, 0, 0, 0, 0, 0x10);

	WINDOWCLASS window;
	initDesktopWindow(&window, EXPLORER_TASKNAME, tid);

	WINDOWCLASS taskbar;
	initTaskbarWindow(&taskbar, filename, tid);

	FILEICON computer;
	initIcon(&computer, "My Computer", tid,1, gVideoWidth - 2 * gBigFolderWidth, gBigFolderHeight);

	FILEICON atapi;
	initIcon(&atapi, "CD-ROM", tid,2, gVideoWidth - 2 * gBigFolderWidth, gBigFolderHeight + gBigFolderHeight + gBigFolderHeight);

	FILEICON floppy;
	initIcon(&floppy, "Floppy", tid,3, gVideoWidth - 2 * gBigFolderWidth,
		gBigFolderHeight + gBigFolderHeight + gBigFolderHeight + gBigFolderHeight + gBigFolderHeight);

	RIGHTMENU menu;
	initRightMenu(&menu, tid);

	__initMouse(gVideoWidth, gVideoHeight);

	char cputype[1024];
	getCpuType(cputype);
	char cpuinfo[1024];
	getCpuInfo(cpuinfo);
	__printf(szout, "CPU MODEL:%s,details:%s,SSE:%d,video height:%d,width:%d,pixel:%d\n", 
		cputype, cpuinfo,isSSE(), gVideoHeight, gVideoWidth, gBytesPerPixel);

	showPciDevs();

	__enableBreakPoint();

	enableSingleStep();

	disableSingleStep();

	enableOverflow();

	//__kAddAlarmTimer(ALARMER_SECOND_INTERVAL, (DWORD)__doAlarmTask, 0);

	sysEntryProc();

	callgateEntry(0, 0);

	displayCCPoem();

	int imageSize = 0;

	//runElfFunction("c:\\liunux\\test.so", "__testfunction");

	TASKCMDPARAMS taskcmd;
	__memset((char*)&taskcmd, 0, sizeof(TASKCMDPARAMS));

	while (1)
	{
		unsigned int ck = __kGetKbd(window.id) & 0xff;
		if (ck == VK_F1)
		{
			if (__findProcessFileName("__kConsole") == FALSE)
			{
				imageSize = getSizeOfImage((char*)MAIN_DLL_SOURCE_BASE);
				__kCreateProcess(MAIN_DLL_SOURCE_BASE, imageSize, "main.dll", "__kConsole", 3, 0);
			}
			continue;
		}
		else if (ck == VK_F2)
		{
			//__createDosCodeProc(gV86VMIEntry, gV86VMISize, "V86VMIEntry");
			continue;
		}
		else if (ck == VK_F3)
		{
			if (__findProcessFileName("__kClock") == FALSE)
			{
				TASKCMDPARAMS cmd;
				__memset((char*)&cmd, 0, sizeof(TASKCMDPARAMS));
				cmd.cmd = SHOW_SYSTEM_LOG;
				cmd.addr = LOG_BUFFER_BASE;
				cmd.filesize = (DWORD)gLogDataPtr - LOG_BUFFER_BASE;

				DWORD thread = getAddrFromName(MAIN_DLL_BASE, "__kClock");
				if (thread) {
					imageSize = getSizeOfImage((char*)MAIN_DLL_SOURCE_BASE);
					//__kCreateThread((DWORD)thread, MAIN_DLL_BASE, (DWORD)&cmd, "__kClock");
					__kCreateProcess(VSMAINDLL_LOAD_ADDRESS, imageSize, "main.dll", "__kClock", 3, 0);
				}
			}
			continue;
		}
		else if (ck == VK_F4) {
			if (__findProcessFileName("__kTestWindow") == FALSE)
			{
				TASKCMDPARAMS cmd;
				__memset((char*)&cmd, 0, sizeof(TASKCMDPARAMS));
				cmd.cmd = SHOW_SYSTEM_LOG;
				cmd.addr = LOG_BUFFER_BASE;
				cmd.filesize = (DWORD)gLogDataPtr - LOG_BUFFER_BASE;

				DWORD thread = getAddrFromName(MAIN_DLL_BASE, "__kTestWindow");
				if (thread) {
					imageSize = getSizeOfImage((char*)MAIN_DLL_SOURCE_BASE);
					//__kCreateThread((DWORD)thread, MAIN_DLL_BASE, (DWORD)&cmd, "__kClock");
					__kCreateProcess(VSMAINDLL_LOAD_ADDRESS, imageSize, "main.dll", "__kTestWindow", 3, 0);
				}
			}
			continue;
		}

		MOUSEINFO mouseinfo;
		__memset((char*)&mouseinfo, 0, sizeof(MOUSEINFO));
		ret = __kGetMouse(&mouseinfo, window.id);
		if (mouseinfo.status & 1)	//left click
		{
			if (menu.action)
			{
				menu.action = 0;

				__restoreRightMenu(&menu);

				if ((mouseinfo.x > menu.pos.x) && (mouseinfo.x < menu.pos.x + menu.width))
				{
					if (mouseinfo.y > menu.pos.y && mouseinfo.y < menu.pos.y + menu.height)
					{
						int funcno = (mouseinfo.y - menu.pos.y) / GRAPHCHAR_HEIGHT / 2;
						if (funcno > 0 && funcno < menu.validItem)
						{
							DWORD func = menu.funcaddr[funcno];

							int cnt = menu.paramcnt[funcno];

							int paramSize = cnt * sizeof(DWORD);

							DWORD * params = (DWORD*)&menu.funcparams[funcno][0];

							__asm {
								mov ecx, cnt
								cmp ecx, 0
								jz __callfunc
								mov esi, params
								add esi, paramSize
								__copyParams :
								sub esi, 4
								mov eax, [esi]
								push eax
								loop __copyParams
								__callfunc :
								mov eax, func
								call eax
								add esp, paramSize
							}
						}
					}
				}

				
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

			if (mouseinfo.x >= 0  && mouseinfo.x < gVideoWidth - TASKBAR_HEIGHT)
			{
				if ((mouseinfo.y >= gWindowHeight) && mouseinfo.y < gVideoHeight)
				{
					ret = TaskbarOnClick(&window);
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


int TaskbarOnClick(WINDOWCLASS *window) {
	return 0;
}


DWORD isDesktop(WINDOWCLASS * window) {
	int pid = window->pid;

	LPPROCESS_INFO tssbase = (LPPROCESS_INFO)TASKS_TSS_BASE;
	
	if (__strcmp(tssbase[pid].funcname, EXPLORER_TASKNAME) == 0)
	{
		return TRUE;
	}

	return FALSE;
}

