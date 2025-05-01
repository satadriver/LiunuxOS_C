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
#include	"clock.h"
#include "ScreenVector.h"
#include "DiamondVector.h"
#include "EllipseVector.h"
#include "SpiralBall.h"
#include "CubeVector.h"
#include "SquareVector.h"
#include "TrajectoryBall.h"
#include "CubeVector.h"

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

	//POPUPMENU popup;
	initPopupMenu(&gPopupMenu);

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

	__kAddAlarmTimer(ALARMER_SECOND_INTERVAL, (DWORD)__doAlarmTask, 0);

	sysEntryProc();

	callgateEntry(0, 0);

	displayCCPoem();

	int imageSize = getSizeOfImage((char*)MAIN_DLL_SOURCE_BASE);

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
				__kCreateProcess(MAIN_DLL_SOURCE_BASE, imageSize, "main.dll", "__kConsole", 3, 0);
			}
			continue;
		}
		else if (ck == VK_F2)
		{
			//__createDosCodeProc(gV86VMIEntry, gV86VMISize, "V86VMIEntry");
			if (__findProcessFileName("ScreenVideo") == FALSE)
			{
				__kCreateProcess(MAIN_DLL_SOURCE_BASE, imageSize, "main.dll", "ScreenVideo", 3, 0);
			}
			continue;
		}
		else if (ck == VK_F3)
		{
			if (__findProcessFileName("__kClock") == FALSE)
			{

				DWORD thread = getAddrFromName(MAIN_DLL_BASE, "__kClock");
				if (thread) {
					__kCreateProcess(VSMAINDLL_LOAD_ADDRESS, imageSize, "main.dll", "__kClock", 3, 0);
				}
			}
			continue;
		}
		else if (ck == VK_F4)
		{

			if (__findProcessFileName("TrajectoryBall") == FALSE)
			{
				__kCreateProcess(MAIN_DLL_SOURCE_BASE, imageSize, "main.dll", "TrajectoryBall", 3, 0);
			}
			continue;
		}
		else if (ck == VK_F5)
		{
			if (__findProcessFileName("SpiralBall") == FALSE)
			{
				__kCreateProcess(MAIN_DLL_SOURCE_BASE, imageSize, "main.dll", "SpiralBall", 3, 0);
			}
			continue;
		}
		else if (ck == VK_F6)
		{
			if (__findProcessFileName("__kPaint") == FALSE)
			{
				__kCreateProcess(MAIN_DLL_SOURCE_BASE, imageSize, "main.dll", "__kPaint", 3, 0);
			}
			continue;
		}
		else if (ck == VK_F7)
		{
			if (__findProcessFileName("CubeVector") == FALSE)
			{
				__kCreateProcess(MAIN_DLL_SOURCE_BASE, imageSize, "main.dll", "CubeVector", 3, 0);
			}
			continue;
		}
		else if (ck == VK_F8)
		{
			if (__findProcessFileName("__kChinesePoem") == FALSE)
			{
				__kCreateProcess(MAIN_DLL_SOURCE_BASE, imageSize, "main.dll", "__kChinesePoem", 3, 0);
			}
			continue;
		}
		else if (ck == VK_F9)
		{
			if (__findProcessFileName("DiamondVector") == FALSE)
			{
				__kCreateProcess(MAIN_DLL_SOURCE_BASE, imageSize, "main.dll", "DiamondVector", 3, 0);
			}
			continue;
		}
		else if (ck == VK_F10)
		{
			if (__findProcessFileName("EllipseVector") == FALSE)
			{
				__kCreateProcess(MAIN_DLL_SOURCE_BASE, imageSize, "main.dll", "EllipseVector", 3, 0);
			}
			continue;
		}
		else if (ck == VK_F11)
		{

			continue;
		}
		else if (ck == VK_F12) {
			if (__findProcessFileName("SquareVector") == FALSE)
			{
				__kCreateProcess(MAIN_DLL_SOURCE_BASE, imageSize, "main.dll", "SquareVector", 3, 0);
			}
			continue;
		}

		MOUSEINFO mouseinfo;
		__memset((char*)&mouseinfo, 0, sizeof(MOUSEINFO));
		ret = __kGetMouse(&mouseinfo, window.id);
		if (mouseinfo.status & 1)	//left click
		{
			if (menu.status)
			{
				menu.status = 0;

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
			else if (gPopupMenu.status) {
				gPopupMenu.status = 0;
				__restoreLeftMenu(&gPopupMenu);
				if ((mouseinfo.x > gPopupMenu.pos.x) && (mouseinfo.x < gPopupMenu.pos.x + gPopupMenu.width))
				{
					if (mouseinfo.y > gPopupMenu.pos.y && mouseinfo.y < gPopupMenu.pos.y + gPopupMenu.height)
					{
						int funcno = (mouseinfo.y - gPopupMenu.pos.y) / GRAPHCHAR_HEIGHT / 2;
						if (funcno > 0 && funcno < POPUPMENU_LIMIT)
						{
							int wid = gPopupMenu.item[funcno].windowid;
							char *winname = gPopupMenu.item[funcno].winname;
							MaximizeWindow(wid);
						}
					}
				}
			}
			else if (mouseinfo.x > gVideoWidth - TASKBAR_HEIGHT && mouseinfo.x < gVideoWidth)
			{
				if ((mouseinfo.y > gVideoHeight - TASKBAR_HEIGHT) && mouseinfo.y < gVideoHeight)
				{
					gPopupMenu.pos.x = mouseinfo.x;
					gPopupMenu.pos.y = mouseinfo.y;
					gPopupMenu.status = mouseinfo.status;
					__drawLeftMenu(&gPopupMenu);
				}
			}

			else if (mouseinfo.x >= 0 && mouseinfo.x < gVideoWidth - TASKBAR_HEIGHT)
			{
				if ((mouseinfo.y >= gWindowHeight) && mouseinfo.y < gVideoHeight)
				{
					ret = TaskbarOnClick(&window);
				}
			}
			else if (mouseinfo.x >= computer.pos.x && mouseinfo.x < computer.pos.x + computer.frameSize + computer.width)
			{
				if (mouseinfo.y >= computer.pos.y && mouseinfo.y <= computer.pos.y + computer.height + computer.frameSize)
				{
					taskcmd.cmd = UNKNOWN_FILE_SYSTEM;
					__strcpy(taskcmd.filename, "FileMgrHD");

					imageSize = getSizeOfImage((char*)MAIN_DLL_SOURCE_BASE);

					__kCreateProcess(MAIN_DLL_SOURCE_BASE, imageSize, "main.dll", "__kFileManager", 3, (DWORD)&taskcmd);
				}
			}

			else if (mouseinfo.x >= atapi.pos.x && mouseinfo.x < (atapi.pos.x + atapi.frameSize + atapi.width))
			{
				if (mouseinfo.y >= atapi.pos.y && mouseinfo.y <= (atapi.pos.y + atapi.height + atapi.frameSize))
				{
					taskcmd.cmd = CDROM_FILE_SYSTEM;
					__strcpy(taskcmd.filename, "FileMgrISO");
					imageSize = getSizeOfImage((char*)MAIN_DLL_SOURCE_BASE);
					__kCreateProcess(MAIN_DLL_SOURCE_BASE, imageSize, "main.dll", "__kFileManager", 3, (DWORD)&taskcmd);
					//__kCreateThread((DWORD)thread, MAIN_DLL_BASE, (DWORD)&cmd, "__kClock");
				}
			}

			else if (mouseinfo.x >= floppy.pos.x && mouseinfo.x < (floppy.pos.x + floppy.frameSize + floppy.width))
			{
				if (mouseinfo.y >= floppy.pos.y && mouseinfo.y <= (floppy.pos.y + floppy.height + floppy.frameSize))
				{
					taskcmd.cmd = FLOPPY_FILE_SYSTEM;
					__strcpy(taskcmd.filename, "FileMgrFllopy");
					imageSize = getSizeOfImage((char*)MAIN_DLL_SOURCE_BASE);
					__kCreateProcess(MAIN_DLL_SOURCE_BASE, imageSize, "main.dll", "__kFileManager", 3, (DWORD)&taskcmd);
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
					menu.status = mouseinfo.status;
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

