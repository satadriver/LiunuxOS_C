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
#include "memory.h"
#include "Pe.h"
#include "window.h"
#include "ProcessDos.h"
#include "ata.h"
#include "TestWindow.h"
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
#include"clock.h"
#include "ScreenVector.h"
#include "DiamondVector.h"
#include "EllipseVector.h"
#include "SpiralBall.h"
#include "CubeVector.h"
#include "SquareVector.h"
#include "TrajectoryBall.h"
#include "CubeVector.h"
#include "servicesproc.h"
#include "apic.h"


int __kExplorer(unsigned int retaddr, int tid, char * filename, char * funcname, DWORD param) {
	int ret = 0;

	char szout[1024];

	__printf(szout, "__kExplorer task retaddr:%x,pid:%x,name:%s,funcname:%s,param:%x\n", retaddr, tid, filename, funcname, param);
	
	WINDOWCLASS window;
	initDesktopWindow(&window, EXPLORER_TASKNAME, tid,1);

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

	initPopupMenu(&gPopupMenu);

	__initMouse(gVideoWidth, gVideoHeight);

	int imageSize = getSizeOfImage((char*)MAIN_DLL_SOURCE_BASE);

	TASKCMDPARAMS taskcmd;
	__memset((char*)&taskcmd, 0, sizeof(TASKCMDPARAMS));

	//__kCreateProcess(MAIN_DLL_SOURCE_BASE, imageSize, "main.dll", "__MyTestTask", 3, 0);
	//__MyTestTask(0, 0, 0, 0, 0);
	//displayCCPoem();

	while (1)
	{
		//__printf(szout, "test start\r\n");

		MOUSEINFO mouseinfo;
		__memset((char*)&mouseinfo, 0, sizeof(MOUSEINFO));

		unsigned int ck = __kGetKbd(window.id) & 0xff;
		if (ck) {
			__printf(szout, "click key:%x\r\n", ck);
		}

		if (ck == VK_RIGHT || ck == VK_LEFT || ck == VK_UP || ck == VK_DOWN || ck == 0x0d||ck == 0x0a) {
			if (ck == VK_RIGHT) {
				mouseinfo.status = 0;
				mouseinfo.x = MOUSE_GRANULARITY;
				mouseinfo.y = 0;
				insertMouse(&mouseinfo);
			}
			else if (ck == VK_LEFT) {
				mouseinfo.status = 0;
				mouseinfo.x = -MOUSE_GRANULARITY;
				mouseinfo.y = 0;
				insertMouse(&mouseinfo);
			}
			else if (ck == VK_UP) {
				mouseinfo.status = 0;
				mouseinfo.x = 0;
				mouseinfo.y =- MOUSE_GRANULARITY;
				insertMouse(&mouseinfo);
			}
			else if (ck == VK_DOWN) {
				mouseinfo.status = 0;
				mouseinfo.x = 0;
				mouseinfo.y = MOUSE_GRANULARITY;
				insertMouse(&mouseinfo);
			}
			else if (ck == 0x0d) {
				mouseinfo.status = 1;
				mouseinfo.x = 0;
				mouseinfo.y = 0;
				insertMouse(&mouseinfo);
			}
			else if (ck == 0x0a) {
				mouseinfo.status = 2;
				mouseinfo.x = 0;
				mouseinfo.y = 0;
				insertMouse(&mouseinfo);
			}
		}
		else if (ck == VK_F1)
		{
			if (__findProcessFileName("__kConsole") == FALSE)
			{
				unsigned long module = linear2phy((unsigned long)"main.dll");
				unsigned long func = linear2phy((unsigned long)"__kConsole");
				__kCreateProcess(MAIN_DLL_SOURCE_BASE, imageSize, (char*)"main.dll", (char*)"__kConsole", 3, 0);
				//__ipiCreateProcess(MAIN_DLL_SOURCE_BASE, imageSize, (char*)"main.dll", (char*)"__kConsole", 3, 0);
				//unsigned long addr = getAddrFromName(MAIN_DLL_SOURCE_BASE, (char*)"__kConsole");
				//if (addr) 
				{
					//__printf(szout, "%s:%d to call __ipiCreateThread \r\n", __FUNCTION__, __LINE__);
					//__ipiCreateThread(addr, (char*)MAIN_DLL_SOURCE_BASE, 0, (char*)"__kConsole");
					//__kCreateThread(addr, (unsigned long)MAIN_DLL_SOURCE_BASE, 0, (char*)"__kConsole");
					
				}
				//else {
				//	__printf(szout, "%s:%d error\r\n", __FUNCTION__,__LINE__);
				//}
			}
			continue;
		}
		else if (ck == VK_F2)
		{
			//__createDosCodeProc(gV86VMIEntry, gV86VMISize, "V86VMIEntry");
			if (__findProcessFileName("ScreenVector") == FALSE)
			{
				__kCreateProcess(MAIN_DLL_SOURCE_BASE, imageSize, "main.dll", "ScreenVector", 3, 0);
			}
			continue;
		}
		else if (ck == VK_F3)
		{
			if (__findProcessFileName("__kClock") == FALSE)
			{

				DWORD thread = getAddrFromName(MAIN_DLL_BASE, "__kClock");
				if (thread) {
					__kCreateProcess(MAIN_DLL_SOURCE_BASE, imageSize, "main.dll", "__kClock", 3, 0);
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

			//continue;
		}
		else if (ck == VK_F12) {
			if (__findProcessFileName("SquareVector") == FALSE)
			{
				__kCreateProcess(MAIN_DLL_SOURCE_BASE, imageSize, "main.dll", "SquareVector", 3, 0);
			}
			continue;
		}


		ret = __kGetMouse(&mouseinfo, window.id);
		if (mouseinfo.status & 1)	//left click
		{
			//__printf(szout, "mouse left click,x:%d,y:%d,computer left:%d,right:%d,top:%d,bottom:%d\r\n",mouseinfo.x,mouseinfo.y, computer.pos.x,
			//	computer.pos.x + computer.frameSize + computer.width, computer.pos.y, computer.pos.y + computer.height + computer.frameSize);

			if (menu.status)
			{
				//__printf(szout, "menu  on\r\n");

				menu.status = 0;

				__restoreRightMenu(&menu);

				if ((mouseinfo.x > menu.pos.x) && (mouseinfo.x < menu.pos.x + menu.width)&&
					mouseinfo.y > menu.pos.y && mouseinfo.y < menu.pos.y + menu.height)
				{
					int funcno = (mouseinfo.y - menu.pos.y) / GRAPHCHAR_HEIGHT / 2;
					if (funcno >= 0 && funcno < RIGHTCLICK_MENU_HEIGHT/2/ GRAPHCHAR_HEIGHT)
					{
						DWORD func = menu.funcaddr[funcno];
						if (func) {
							int cnt = menu.paramcnt[funcno];

							int paramSize = cnt * sizeof(DWORD);

							DWORD* params = (DWORD*)&menu.funcparams[funcno][0];

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
				//__printf(szout, "popup  on\r\n");
				gPopupMenu.status = 0;
				__restoreLeftMenu(&gPopupMenu);
				if ( (mouseinfo.x >= gPopupMenu.pos.x) && (mouseinfo.x <= gPopupMenu.pos.x + gPopupMenu.width)&&
					mouseinfo.y >= gPopupMenu.pos.y && mouseinfo.y <= gPopupMenu.pos.y + gPopupMenu.height)	{

					int seq = (mouseinfo.y - gPopupMenu.pos.y) / GRAPHCHAR_HEIGHT / 2;
					int cnt = LEFTCLICK_MENU_HEIGHT / 2 / GRAPHCHAR_HEIGHT;
					if (seq >= 0 && seq < cnt)
					{
						LPWINDOWCLASS window = gPopupMenu.item[seq].window;
						int valid = gPopupMenu.item[seq].valid;
						if (valid &&  window) {
							MaximizeWindow(window);
						}
					}
				}
			}
			else if (mouseinfo.x > gVideoWidth - TASKBAR_HEIGHT && mouseinfo.x < gVideoWidth && 
				mouseinfo.y > gVideoHeight - TASKBAR_HEIGHT && mouseinfo.y < gVideoHeight)
			{
				//__printf(szout, "popup click\r\n");

				gPopupMenu.pos.x = mouseinfo.x;
				gPopupMenu.pos.y = mouseinfo.y;
				gPopupMenu.status = mouseinfo.status;
				__drawLeftMenu(&gPopupMenu);
			}
			else if (mouseinfo.x >= 0 && mouseinfo.x < gVideoWidth - TASKBAR_HEIGHT && mouseinfo.y > gWindowHeight && mouseinfo.y < gVideoHeight)
			{
				//__printf(szout, "taskbar click\r\n");
				ret = TaskbarOnClick(&window);	
			}
			else if (mouseinfo.x >= computer.pos.x && mouseinfo.x <= (computer.pos.x + computer.frameSize + computer.width) &&
				mouseinfo.y >= computer.pos.y && mouseinfo.y <= (computer.pos.y + computer.height + computer.frameSize)){
					//__printf(szout, "open file manager\r\n");
					taskcmd.cmd = UNKNOWN_FILE_SYSTEM;
					__strcpy(taskcmd.filename, "FileMgrHD");

					imageSize = getSizeOfImage((char*)MAIN_DLL_SOURCE_BASE);

					__kCreateProcess(MAIN_DLL_SOURCE_BASE, imageSize, "main.dll", "__kFileManager", 3, (DWORD)&taskcmd);
				
			}
			else if (mouseinfo.x >= atapi.pos.x && mouseinfo.x <= (atapi.pos.x + atapi.frameSize + atapi.width) && 
				mouseinfo.y >= atapi.pos.y && mouseinfo.y <= (atapi.pos.y + atapi.height + atapi.frameSize)){
					taskcmd.cmd = CDROM_FILE_SYSTEM;
					__strcpy(taskcmd.filename, "FileMgrISO");
					imageSize = getSizeOfImage((char*)MAIN_DLL_SOURCE_BASE);
					__kCreateProcess(MAIN_DLL_SOURCE_BASE, imageSize, "main.dll", "__kFileManager", 3, (DWORD)&taskcmd);
					//__kCreateThread((DWORD)thread, MAIN_DLL_BASE, (DWORD)&cmd, "__kClock");
				
			}
			else if (mouseinfo.x >= floppy.pos.x && mouseinfo.x < (floppy.pos.x + floppy.frameSize + floppy.width) &&
				mouseinfo.y >= floppy.pos.y && mouseinfo.y <= (floppy.pos.y + floppy.height + floppy.frameSize) ){
					taskcmd.cmd = FLOPPY_FILE_SYSTEM;
					__strcpy(taskcmd.filename, "FileMgrFllopy");
					imageSize = getSizeOfImage((char*)MAIN_DLL_SOURCE_BASE);
					__kCreateProcess(MAIN_DLL_SOURCE_BASE, imageSize, "main.dll", "__kFileManager", 3, (DWORD)&taskcmd);
				
			}	
		}
		else if (mouseinfo.status & 2)	//right click
		{
			//__printf(szout, "mouse right click\r\n");

			if (mouseinfo.x > gVideoWidth - TASKBAR_HEIGHT && mouseinfo.x < gVideoWidth && 
				mouseinfo.y > gVideoHeight - TASKBAR_HEIGHT && mouseinfo.y < gVideoHeight){
					menu.pos.x = mouseinfo.x;
					menu.pos.y = mouseinfo.y;
					menu.status = mouseinfo.status;
					__drawRightMenu(&menu);
				
			}
		}
		else if (mouseinfo.status & 4)	//middle click
		{
// 			menu.pos.x = mouseinfo.x;
// 			menu.pos.y = mouseinfo.y;
// 			menu.action = mouseinfo.status;
		}
		//__giveup();
		__sleep(0);
	}
	return 0;
}


int TaskbarOnClick(WINDOWCLASS *window) {
	return 0;
}


DWORD isDesktop(WINDOWCLASS * window) {
	int pid = window->pid;

	LPPROCESS_INFO tssbase = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	
	if (__strcmp(tssbase[pid].funcname, EXPLORER_TASKNAME) == 0)
	{
		return TRUE;
	}

	return FALSE;
}

