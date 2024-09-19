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
		//retvalue = getmouse(&mouseinfo,window.id);
		retvalue = __kGetMouse(&mouseinfo, window.id);
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


int restoreFileManager(LPFMWINDOW w) {
	__restoreRectangle(&w->window.pos, w->window.width, w->window.height, (unsigned char*)w->window.backGround);
	removeWindow(w->window.id);

	__kFree(w->window.backGround);

	//__terminatePid(w->pid);
	return 0;
}


int drawFileManager(LPFMWINDOW w) {
	w->window.capHeight = 0;
	w->window.frameSize = 0;

	w->window.next = 0;
	w->window.prev = 0;

	w->cpl = 3;
	w->window.color = 0xffffff;
	w->window.capColor = 0;
	w->window.fontcolor = 0;
	w->window.height = gVideoHeight;
	w->window.width = gVideoWidth;
	w->window.pos.x = 0;
	w->window.pos.y = 0;
	w->fsheight = GRAPHCHAR_HEIGHT * w->cpl;

	w->window.backsize = gBytesPerPixel * (w->window.width) * (w->window.height);

	w->window.backGround = (DWORD)__kMalloc(w->window.backsize);

	w->window.id = addWindow(TRUE, (DWORD*)&w->window.pos.x, (DWORD*)&w->window.pos.y, 0, w->window.winname);

	__drawRectangle(&w->window.pos, w->window.width, w->window.height, w->window.color, (unsigned char*)w->window.backGround);

	return 0;

}

int __restoreRectangleFrame(LPPOINT p, int width, int height, int framesize, unsigned char* backup) {
	int startpos = p->y * gBytesPerLine + p->x * gBytesPerPixel + gGraphBase;
	unsigned char* ptr = (unsigned char*)startpos;
	unsigned char* keep = ptr;

	for (int i = 0; i < height + framesize; i++)
	{
		for (int j = 0; j < width + framesize; j++)
		{
			for (int k = 0; k < gBytesPerPixel; k++)
			{
				*ptr = *backup;
				ptr++;
				backup++;
			}
		}

		keep += gBytesPerLine;
		ptr = (unsigned char*)keep;
	}

	return (int)ptr - gGraphBase;
}