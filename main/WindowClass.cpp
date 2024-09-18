#pragma once
#include "windowclass.h"
#include "video.h"
#include "Utils.h"

#include "mouse.h"
#include "keyboard.h"
#include "task.h"
#include "file.h"
#include "graph.h"
#include "Pe.h"
#include "gdi/jpeg.h"
#include "keyboard.h"
#include "mouse.h"
#include "window.h"



int __kShowWindow(unsigned int retaddr, int tid, char * filename, char * funcname,DWORD runparam) {
 	char szout[1024];
//  	__printf(szout, "__kShowWindow task tid:%x,filename:%s,funcname:%s,param:%x\n", tid, filename,funcname,runparam);
//  	__drawGraphChars((unsigned char*)szout, 0);

	int retvalue = 0;

	LPTASKCMDPARAMS taskcmd = (LPTASKCMDPARAMS)runparam;
	int cmd = taskcmd->cmd;

// 	__printf(szout, "cmd:%d,addr:%x,filesize:%d,filename:%s\n", taskcmd->cmd, taskcmd->addr, taskcmd->filesize, taskcmd->filename);
// 	__drawGraphChars((unsigned char*)szout, 0);

	WINDOWCLASS window;
	__memset((char*)&window, 0, sizeof(WINDOWCLASS));
	__strcpy(window.caption, filename);
	initFullWindow(&window, funcname, tid);

	if (cmd == SHOW_WINDOW_BMP || cmd == SHOW_WINDOW_TXT || cmd == SHOW_WINDOW_JPEG)
	{
		int filesize = 0;
		char * filebuf = 0;
		if (taskcmd->addr == 0 || taskcmd->filesize == 0)
		{
			filesize = readFile(taskcmd->filename,&filebuf);
			if (filesize <= 0)
			{
				__printf(szout, "__kFullWindowPic read file:%s error\n", filename);

				__restoreWindow(&window);
				return -1;
			}
		}
		else {
			filebuf = (char*)taskcmd->addr;
			filesize = taskcmd->filesize;
		}

		if (cmd == SHOW_WINDOW_BMP)
		{
			retvalue = showBmp(filename, (unsigned char *)filebuf, filesize, window.showX, window.showY);
			if (retvalue <= 0)
			{
				__printf(szout, "__kFullWindowPic showBmp:%s error\n", filename);

				//__restoreWindow(&window);
				//return -1;
			}
		}
		else if (cmd == SHOW_WINDOW_TXT)
		{
			unsigned char * data = (unsigned char*)filebuf;
			*(data + filesize) = 0;

			DWORD cappos = __getpos(window.showX, window.showY);
			
			int showend = __drawGraphChar((unsigned char*)data, DEFAULT_FONT_COLOR, (unsigned int)cappos,window.color);
			//int showend = __drawGraphCharPos((unsigned char*)data, DEFAULT_FONT_COLOR, (unsigned int)cappos);
		}
		else if (cmd == SHOW_WINDOW_JPEG) {
			int bmpsize = filesize * 16;
			char * bmpdata = (char*)__kMalloc(bmpsize);
			
			retvalue = LoadJpegFile(filebuf, filesize, bmpdata, &bmpsize);
			if (retvalue)
			{
				retvalue = showBmp(filename, (unsigned char *)bmpdata, bmpsize, window.showX, window.showY);
			}
			else {
				__drawGraphChars((unsigned char*)"decode jpeg error\r\n", 0);
			}
			__kFree((DWORD)bmpdata);
		}

		__kFree((DWORD)filebuf);

	}
	else if (cmd == SHOW_SYSTEM_LOG)
	{
		unsigned char * data = (unsigned char*)taskcmd->addr;
		*(data + taskcmd->filesize) = 0;

		DWORD cappos = __getpos(window.showX, window.showY);

		int showend = __drawGraphChar((unsigned char*)data, DEFAULT_FONT_COLOR, (unsigned int)cappos, window.color);
	}
	else if (cmd == SHOW_TEST_WINDOW)
	{
		gKbdTest = window.id;
		gMouseTest = window.id;
	}

	while (1)
	{
		//unsigned int ck = __getchar(window.id);
		unsigned int ck = __kGetKbd(window.id);
		unsigned int asc = ck & 0xff;
		if (asc == 0x1b)
		{
			if (cmd == SHOW_TEST_WINDOW)
			{
				gKbdTest = FALSE;
				gMouseTest = FALSE;
			}
			__restoreWindow(&window);
			return 0;

// 			__terminatePid(pid);
// 			__sleep(-1);
		}


		MOUSEINFO mouseinfo;
		__memset((char*)&mouseinfo, 0, sizeof(MOUSEINFO));
		retvalue = getmouse(&mouseinfo,window.id);
		if (mouseinfo.status & 1)	//left click
		{
			if (mouseinfo.x >= window.shutdownx && mouseinfo.x <= window.shutdownx + window.capHeight)
			{
				if (mouseinfo.y >= window.shutdowny && mouseinfo.y <= window.shutdowny + window.capHeight)
				{
					__restoreWindow(&window);
					return 0;

					//__terminatePid(pid);
					//__sleep(-1);
				}
			}
		}

		__sleep(0);
	}
	return 0;
}

void initTaskbarWindow(WINDOWCLASS * window, char * filename, int tid) {

	__memset((char*)window, 0, sizeof(WINDOWCLASS));

	window->capColor = 0;
	window->capHeight = 0;
	window->color = TASKBARCOLOR;
	window->frameSize = 0;
	window->frameColor = 0;
	__strcpy(window->caption, filename);

	window->pos.x = FULLWINDOW_TOP;

	window->pos.y = gWindowHeight;

	window->width = gVideoWidth + window->frameSize;

	window->height = gVideoHeight - gWindowHeight;

	window->zoomin = 1;

	window->tid = tid;

	LPPROCESS_INFO p = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
	window->pid = p->pid;

	window->left = window->pos.x + (window->frameSize >> 1);
	window->top = window->pos.y + (window->frameSize >> 1) + window->capHeight;
	window->right = gVideoWidth - (window->frameSize >> 1) - 1;
	window->bottom = gVideoHeight - (window->frameSize >> 1) - 1;

	window->showX = window->pos.x + (window->frameSize >> 1);
	window->showY = window->pos.y + (window->frameSize >> 1) + window->capHeight;

	__drawBackGroundWindow(window, FALSE);

	window->prev = 0;
	window->next = 0;
}


void initDesktopWindow(WINDOWCLASS * window, char * name, int tid) {
	__memset((char*)window, 0, sizeof(WINDOWCLASS));

	window->capColor = 0;
	window->capHeight = 0;
	window->color = BACKGROUND_COLOR;
	window->frameSize = 0;
	window->frameColor = 0;
	__strcpy(window->caption, name);
	__strcpy(window->winname, name);

	window->backGround = 0;

	window->pos.x = FULLWINDOW_TOP;

	window->pos.y = FULLWINDOW_LEFT;

	window->width = gVideoWidth + window->frameSize;

	window->height = gWindowHeight + window->frameSize + window->capHeight;

	window->zoomin = 1;

	window->tid = tid;

	LPPROCESS_INFO p = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
	window->pid = p->pid;

	window->left = (window->frameSize >> 1)+ window->pos.x;
	window->top = (window->frameSize >> 1) + window->capHeight + window->pos.y;
	window->right = window->left + window->width  - 1;
	window->bottom = window->top + window->height  - 1;

	window->showX = window->left;
	window->showY = window->top;

	window->id = addWindow(FALSE, (DWORD*)&window->showX, (DWORD*)&window->showY, ~window->color,window->winname);

	__drawBackGroundWindow(window, FALSE);

	__initMouse(gVideoWidth, gVideoHeight);

	window->prev = 0;
	window->next = 0;
}


void initFullWindow(WINDOWCLASS * window,char * functionname,int tid) {

	window->capColor = 0x00ffff;
	window->capHeight = GRAPHCHAR_HEIGHT * 2;
	window->color = CONSOLE_FONT_COLOR;
	window->frameSize = GRAPHCHAR_WIDTH;
	window->frameColor = FOLDERFONTBGCOLOR;
	__strcpy(window->caption, functionname);
	__strcpy(window->winname, functionname);

	window->pos.x = FULLWINDOW_TOP;

	window->pos.y = FULLWINDOW_LEFT;

	window->width = gVideoWidth - window->frameSize;

	window->height = gWindowHeight - window->frameSize - window->capHeight;

	window->zoomin = 1;

	window->tid = tid;

	LPPROCESS_INFO p = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
	window->pid = p->pid;

	window->left = window->frameSize >> 1;
	window->top = (window->frameSize >> 1) + window->capHeight;
	window->right = gVideoWidth - (window->frameSize>>1) - 1;
	window->bottom = gWindowHeight - (window->frameSize >> 1) - 1;

	window->showX = window->pos.x + (window->frameSize >> 1);
	window->showY = window->pos.y + (window->frameSize >> 1) + window->capHeight;

	window->prev = 0;
	window->next = 0;

	__drawWindow(window,TRUE);
}


void initConsoleWindow(WINDOWCLASS * window, char * filename, int tid) {
	window->capColor = 0x00ffff;
	window->capHeight = GRAPHCHAR_HEIGHT * 2;
	window->color = DEFAULT_FONT_COLOR;
	window->frameSize = GRAPHCHAR_WIDTH;
	window->frameColor = FOLDERFONTBGCOLOR;
	__strcpy(window->caption, filename);
	__strcpy(window->winname, "__Console");

	window->width = gVideoWidth / 2;
	window->height = gVideoHeight / 2;

	window->pos.x = gVideoWidth / 4;

	window->pos.y = gVideoHeight / 4;

	window->left = window->pos.x + window->frameSize;
	window->top = window->pos.y + (window->frameSize >> 1) + window->capHeight;

	window->right = window->pos.x + window->frameSize + window->width - 1;
	window->bottom = window->pos.y + (window->frameSize >> 1) + +window->capHeight + window->height - 1;

	window->zoomin = 1;
	window->tid = tid;
	LPPROCESS_INFO p = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
	window->pid = p->pid;

	window->showX = window->pos.x + (window->frameSize >> 1);
	window->showY = window->pos.y + (window->frameSize >> 1) + window->capHeight;

	window->prev = 0;
	window->next = 0;

	__drawWindow(window, TRUE);
}


void initBigClickItem(FILEMAP *clickitem, char * name, int tid,int id, int x, int y) {
	__memset((char*)clickitem, 0, sizeof(FILEMAP));

	clickitem->tid = tid;

	clickitem->id = id;

	LPPROCESS_INFO p = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
	clickitem->pid = p->pid;

	clickitem->color = FOLDERCOLOR;
	clickitem->width = gBigFolderWidth;
	clickitem->height = gBigFolderHeight;
	clickitem->pos.x = x;
	clickitem->pos.y = y;
	clickitem->namecolor = FOLDERFONTCOLOR;
	clickitem->zoomin = 1;
	__strcpy(clickitem->name, name);
	clickitem->frameColor = FOLDERFONTBGCOLOR;
	clickitem->frameSize = GRAPHCHAR_WIDTH;
	clickitem->namebgcolor = DEFAULT_FONT_COLOR;

	int ret = __drawFileMap(clickitem);
}


