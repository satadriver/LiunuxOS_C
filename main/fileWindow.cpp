#pragma once

#include "video.h"
#include "Utils.h"

#include "mouse.h"
#include "keyboard.h"
#include "task.h"
#include "file.h"
#include "graph.h"
#include "Pe.h"
#include "gdi/jpeg.h"

#include "window.h"
#include "guiHelper.h"
#include "fileWindow.h"



int __kShowWindow(unsigned int retaddr, int tid, char * filename, char * funcname,DWORD runparam) {
 	char szout[1024];
//  	__printf(szout, "__kShowWindow task tid:%x,filename:%s,funcname:%s,param:%x\n", tid, filename,funcname,runparam);

	int retvalue = 0;

	LPTASKCMDPARAMS taskcmd = (LPTASKCMDPARAMS)runparam;
	int cmd = taskcmd->cmd;

// 	__printf(szout, "cmd:%d,addr:%x,filesize:%d,filename:%s\n", taskcmd->cmd, taskcmd->addr, taskcmd->filesize, taskcmd->filename);

	WINDOWCLASS window;
	__memset((char*)&window, 0, sizeof(WINDOWCLASS));
	__strcpy(window.caption, filename);
	initFullWindow(&window, funcname, tid,0);

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

				__DestroyWindow(&window);
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
			}
		}
		else if (cmd == SHOW_WINDOW_TXT)
		{
			unsigned char * data = (unsigned char*)filebuf;
			*(data + filesize) = 0;

			DWORD cappos = __getpos(window.showX, window.showY);
			
			int showend = __drawGraphChar(( char*)data, DEFAULT_FONT_COLOR, (unsigned int)cappos,window.color);
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
				__printf(szout, ( char*)"decode jpeg error\r\n");
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

		int showend = __drawGraphChar(( char*)data, DEFAULT_FONT_COLOR, (unsigned int)cappos, window.color);
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
			__DestroyWindow(&window);
			return 0;
		}


		MOUSEINFO mouseinfo;
		__memset((char*)&mouseinfo, 0, sizeof(MOUSEINFO));
		//retvalue = getmouse(&mouseinfo,window.id);
		retvalue = __kGetMouse(&mouseinfo, window.id);
		if (mouseinfo.status & 1)	//left click
		{
			if (mouseinfo.x >= window.shutdownx && mouseinfo.x <= window.shutdownx + window.capHeight)
			{
				if (mouseinfo.y >= window.shutdowny && mouseinfo.y <= window.shutdowny + window.capHeight)
				{
					__DestroyWindow(&window);
					return 0;

					//__terminatePid(pid);
					//__sleep(-1);
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

		__sleep(0);
	}
	return 0;
}

