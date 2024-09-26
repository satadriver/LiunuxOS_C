
#include "video.h"
#include "Utils.h"
#include "Kernel.h"
#include "window.h"
#include "mouse.h"
#include "malloc.h"
#include "task.h"
#include "math.h"
#include "gdi.h"
#include "PEresource.h"
#include "file.h"
#include "pci.h"


VESAINFORMATION* gVesaInfo;
unsigned char* gCCFontBase = 0;
DWORD gFontBase = 0;
unsigned int gGraphBase = 0;
int gVideoWidth = 0;
int gVideoHeight = 0;
int gBytesPerPixel = 0;
int gBytesPerLine = 0;
//int gBytesPerFont;
//int gFontLineSize = 0;
int gWindowHeight = 0;
unsigned int gWindowSize = 0;

int gBigFolderWidth = 0;
int gBigFolderHeight = 0;
int gSmallFolderWidth = 0;
int gSmallFolderHeight = 0;

int gShowX = 0;
int gShowY = 0;

int g_ScreenMode = 0;

//the RGB sequence of  the video screen pixel is BGR

int __initVideo(LPVESAINFORMATION vesaInfo, DWORD fontbase) {
	int result = 0;

	gVesaInfo = (VESAINFORMATION*)VESA_INFO_BASE;

	__memcpy((char*)gVesaInfo, (char*)vesaInfo, sizeof(VESAINFORMATION));

	__memcpy((char*)gVesaInfo + sizeof(VESAINFORMATION), (char*)vesaInfo + sizeof(VESAINFORMATION), sizeof(VESAINFOBLOCK));

	gFontBase = fontbase;
	gBytesPerPixel = vesaInfo->BitsPerPixel >> 3;
	gBytesPerLine = vesaInfo->BytesPerScanLine;
	gVideoHeight = vesaInfo->YRes;
	gVideoWidth = vesaInfo->XRes;
	gGraphBase = vesaInfo->PhyBasePtr + vesaInfo->OffScreenMemOffset;

	//gBytesPerFont = gBytesPerPixel * GRAPHCHAR_WIDTH;
	//gFontLineSize = gBytesPerLine * GRAPHCHAR_HEIGHT;

	gWindowHeight = gVideoHeight - TASKBAR_HEIGHT;
	gWindowSize = gWindowHeight * gBytesPerLine;

	gSmallFolderHeight = gVideoHeight / 20;
	gSmallFolderWidth = gVideoWidth / 20;

	gBigFolderHeight = gVideoHeight / 10;
	gBigFolderWidth = gVideoWidth / 10;

	gShowY = 0;
	gShowX = 0;

	POINT p;
	p.x = 0;
	p.y = 0;
	__drawRectWindow(&p, gVideoWidth, gVideoHeight, BACKGROUND_COLOR, 0);

	g_ScreenMode = TRUE;

	return 0;
}

int __getpos(int x, int y) {
	return y * gBytesPerLine + gBytesPerPixel * x;
}

int __drawShutdown(LPWINDOWCLASS window) {
	if (window->capHeight == 0)
	{
		return TRUE;
	}

	int singlefs = window->frameSize >> 1;

	int x = window->pos.x + singlefs + window->width - window->capHeight;

	int y = window->pos.y + singlefs;

	window->shutdownx = x;
	window->shutdowny = y;

	unsigned char* pos = (unsigned char*)__getpos(x, y) + gGraphBase;

	unsigned char* keepy = pos;

	for (int i = 0; i < window->capHeight; i++)
	{
		for (int j = 0; j < window->capHeight; j++)
		{
			if (i == j || i == 0 || j == 0 || (i + j == window->capHeight) )	////y坐标轴向下是正号，y=8 - x
			{
				int color = DEFAULT_FONT_COLOR;
				for (int k = 0; k < gBytesPerPixel; k++)
				{
					*pos = color;
					pos++;
					color = color >> 8;
				}
			}
			else {
				pos += gBytesPerPixel;
			}
		}
		keepy = keepy + gBytesPerLine;
		pos = keepy;
	}

	return (int)pos - gGraphBase;
}


int __drawRectFrameWindow(LPPOINT p, int width, int height, int color, int framesize, int framecolor, char* backbuf) {
	int startpos = __getpos(p->x, p->y) + gGraphBase;
	unsigned char* ptr = (unsigned char*)startpos;
	unsigned char* keep = ptr;

	char* back = backbuf;

	int singlefs = framesize >> 1;

	for (int i = 0; i < height + framesize; i++)
	{
		for (int j = 0; j < width + framesize; j++)
		{
			int c = 0;

			if ((i <= singlefs) || (i >= height + singlefs))
			{
				c = framecolor;
			}
			else if ((j <= singlefs) || (j >= width + singlefs))
			{
				c = framecolor;
			}
			else {
				c = color;
			}

			for (int k = 0; k < gBytesPerPixel; k++)
			{
				if (backbuf && (*back != *ptr))
				{
					*back = *ptr;
				}
				back++;

				*ptr = (c & 0xff);
				ptr++;
				c = (c >> 8);
			}
		}

		keep += gBytesPerLine;
		ptr = (unsigned char*)keep;
	}

	return (int)ptr - gGraphBase;
}


int __drawRectangleFrameCaption(LPPOINT p, int width, int height, int color, int framesize, int framecolor, int capsize,
	int capcolor, char* capname, char* backdata) {
	int ret = 0;

	int startpos = __getpos(p->x, p->y) + gGraphBase;

	unsigned char* ptr = (unsigned char*)startpos;
	unsigned char* keep = ptr;

	int singlefs = framesize >> 1;

	for (int i = 0; i < height + framesize + capsize; i++)
	{
		for (int j = 0; j < width + framesize; j++)
		{
			int c = 0;

			if ((i < singlefs) || (i >= height + singlefs + capsize))
			{
				c = framecolor;
			}
			else if ((j < singlefs) || (j >= width + singlefs))
			{
				c = framecolor;
			}
			else if ((i >= singlefs) && (i < capsize + singlefs))
			{
				c = capcolor;
			}
			else {
				c = color;
			}

			for (int k = 0; k < gBytesPerPixel; k++)
			{
				*backdata = *ptr;
				backdata++;

				*ptr = (c & 0xff);
				ptr++;
				c = (c >> 8);
			}
		}

		keep += gBytesPerLine;
		ptr = (unsigned char*)keep;
	}

	int showend = (int)ptr;

	if (capname && *capname && capsize)
	{
		DWORD cappos = __getpos(p->x + singlefs, p->y + singlefs);

		showend = __drawGraphChar(( char*)capname, DEFAULT_FONT_COLOR, (unsigned int)cappos,0);
	}

	return (int)showend - gGraphBase;
}

int __restoreWindow(LPWINDOWCLASS window) {
	int startpos = window->pos.y * gBytesPerLine + window->pos.x * gBytesPerPixel + gGraphBase;
	unsigned char* ptr = (unsigned char*)startpos;
	unsigned char* keep = ptr;
	unsigned char* srcdata = (unsigned char*)window->backBuf;

	for (int i = 0; i < window->height + window->frameSize + window->capHeight; i++)
	{
		for (int j = 0; j < window->width + window->frameSize; j++)
		{
			for (int k = 0; k < gBytesPerPixel; k++)
			{
				if (*ptr != *srcdata) {

					*ptr = *srcdata;
				}
				ptr++;
				srcdata++;
			}
		}

		keep += gBytesPerLine;
		ptr = (unsigned char*)keep;
	}

	return (int)ptr - gGraphBase;
}

int __DestroyWindow(LPWINDOWCLASS window) {

	__kRestoreMouse();

	removeWindow(window->id);

	int size = __restoreWindow(window);

	__kRefreshMouseBackup();

	__kDrawMouse();

	if (window->backBuf)
	{
		__kFree(window->backBuf);
	}

	//__terminatePid(window->pid);

	return size;
}


int __drawWindow(LPWINDOWCLASS window) {

	int ret = 0;

	__kRestoreMouse();

	window->backsize = gBytesPerPixel * (window->height + window->frameSize + window->capHeight) * (window->width + window->frameSize);

	window->backBuf = __kMalloc(window->backsize);

	window->id = addWindow((DWORD)window, (DWORD*)&window->showX, (DWORD*)&window->showY, ~window->color, window->winname);

	ret = __drawRectangleFrameCaption(&window->pos, window->width, window->height, window->color, window->frameSize, window->frameColor,
		window->capHeight, window->capColor, window->caption, (char*)window->backBuf);

	ret = __drawShutdown(window);

	__kRefreshMouseBackup();

	__kDrawMouse();

	return 0;
}




int __drawRectWindow(LPPOINT p, int width, int height, int color, unsigned char* backbuf) {
	unsigned char* backup = backbuf;
	__kRestoreMouse();

	int startpos = p->y * gBytesPerLine + p->x * gBytesPerPixel + gGraphBase;
	unsigned char* ptr = (unsigned char*)startpos;
	unsigned char* keep = ptr;
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			int c = color;
			for (int k = 0; k < gBytesPerPixel; k++)
			{
				if (backbuf && (*backup != *ptr))
				{
					*backup = *ptr;
				}
				backup++;

				if (*ptr != (c & 0xff)) {
					*ptr = (c & 0xff);
				}

				ptr++;
				c = (c >> 8);
			}
		}

		keep += gBytesPerLine;
		ptr = (unsigned char*)keep;
	}
	__kRefreshMouseBackup();
	__kDrawMouse();
	return (int)ptr - gGraphBase;
}


int __restoreRectWindow(LPPOINT p, int width, int height, unsigned char* backup) {
	__kRestoreMouse();

	int startpos = p->y * gBytesPerLine + p->x * gBytesPerPixel + gGraphBase;
	unsigned char* ptr = (unsigned char*)startpos;
	unsigned char* keep = ptr;

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			for (int k = 0; k < gBytesPerPixel; k++)
			{
				if (*ptr != *backup) {
					*ptr = *backup;
				}
				ptr++;
				backup++;
			}
		}

		keep += gBytesPerLine;
		ptr = (unsigned char*)keep;
	}
	__kRefreshMouseBackup();
	__kDrawMouse();
	return (int)ptr - gGraphBase;
}



int __restoreRectFrameWindow(LPPOINT p, int width, int height, int framesize, unsigned char* backup) {
	int startpos = p->y * gBytesPerLine + p->x * gBytesPerPixel + gGraphBase;
	unsigned char* ptr = (unsigned char*)startpos;
	unsigned char* keep = ptr;

	for (int i = 0; i < height + framesize; i++)
	{
		for (int j = 0; j < width + framesize; j++)
		{
			for (int k = 0; k < gBytesPerPixel; k++)
			{
				if (*ptr != *backup) {
					*ptr = *backup;
				}
				ptr++;
				backup++;
			}
		}

		keep += gBytesPerLine;
		ptr = (unsigned char*)keep;
	}

	return (int)ptr - gGraphBase;
}




int __drawFileIconChars(FILEICON* filemap) {

	unsigned int pos = __getpos(filemap->showX, filemap->showY);

	char* str = filemap->name;

	int color = filemap->namecolor;

	int resultpos = __drawGraphChar(str,  color,  pos, 0);

	filemap->showY = resultpos / gBytesPerLine;
	filemap->showX = (resultpos % gBytesPerLine) / gBytesPerPixel;

	if (filemap->showY >= gWindowHeight)
	{
		filemap->showY = 0;
		filemap->showX = 0;
	}
	return 0;
}








int __backspaceChar() {

	gShowX -= GRAPHCHAR_WIDTH;
	if (gShowX < 0 && gShowY > 0)
	{
		gShowX = gVideoWidth - GRAPHCHAR_WIDTH;
		gShowY -= GRAPHCHAR_HEIGHT;
		if (gShowY <= 0)
		{
			gShowY = 0;
		}
	}
	else if (gShowX < 0 && gShowY <= 0)
	{
		gShowY = 0;
		gShowX = 0;
	}

	unsigned int pos = __getpos(gShowX, gShowY);
	int showpos = __drawGraphChar(( char*)" ", BACKGROUND_COLOR, pos, BACKGROUND_COLOR);

	return showpos;
}



int __drawGraphChars( char* str, int color) {
	if (g_ScreenMode == 0) {
		return FALSE;
	}
#ifdef LIUNUX_DEBUG_LOG_ON
	unsigned int pos = __getpos(gShowX, gShowY);
	int resultpos = __drawGraphChar(str, color, pos, 0);		//BACKGROUND_COLOR

	gShowY = resultpos / gBytesPerLine;

	gShowX = (resultpos % gBytesPerLine) / gBytesPerPixel;

	if (gShowY >= gWindowHeight)
	{
		gShowY = 0;
		gShowX = 0;
	}
#else
	logInMem((char*)font, __strlen((char*)font));
#endif
	return 0;
}



int __drawGraphChar( char* str, int color, unsigned int pos, int bgcolor) {
	if (g_ScreenMode == 0) {
		return FALSE;
	}
	int len = __strlen((char*)str);

	unsigned char* showpos = pos + (unsigned char*)gGraphBase;

	unsigned char* keepy = showpos;
	unsigned char* keepx = keepy;

	for (int i = 0; i < len; i++)
	{
		unsigned int ch = str[i];
		if (ch == '\n')
		{
			int posy = (unsigned int)(showpos - gGraphBase) / gBytesPerLine;
			unsigned int nowpos = __getpos(0, posy + GRAPHCHAR_HEIGHT);
			if (nowpos >= gWindowSize)
			{
				nowpos = 0;
			}
			showpos = (unsigned char*)nowpos + gGraphBase;

			keepx = showpos;
			keepy = showpos;
			continue;
		}
		else if (ch == '\r')
		{
			int posy = (unsigned int)(showpos - gGraphBase) / gBytesPerLine;
			showpos = (unsigned char*)__getpos(0, posy) + gGraphBase;
			keepx = showpos;
			keepy = showpos;
			continue;
		}
		else if (ch == 0)
		{
			break;
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
					for (int n = 0; n < gBytesPerPixel; n++)
					{
						*showpos = c;
						c = c >> 8;

						showpos++;
					}
				}
				else if (bgcolor) {
					c = bgcolor;
					for (int n = 0; n < gBytesPerPixel; n++)
					{
						*showpos = c;
						c = c >> 8;

						showpos++;
					}
				}
				else {
					showpos += gBytesPerPixel;
				}

				m = m >> 1;
			}

			keepx += gBytesPerLine;
			showpos = keepx;
		}

		keepy = keepy + GRAPHCHAR_WIDTH * gBytesPerPixel;

		int mod = (unsigned int)(keepy - gGraphBase) % gBytesPerLine;
		if (mod == 0)
		{
			keepy = keepy + (GRAPHCHAR_HEIGHT - 1) * gBytesPerLine;
			if ((unsigned int)(keepy + GRAPHCHAR_HEIGHT * gBytesPerLine - gGraphBase) >= gWindowSize)		//keep one line to show
			{
				keepy = (unsigned char*)gGraphBase;
			}
		}

		keepx = keepy;
		showpos = keepy;
	}
	return (int)(showpos - gGraphBase);
}


unsigned short* getGBKCCIdx(unsigned short gbk) {
	unsigned char low = (gbk & 0xff);
	unsigned char high = (gbk >> 8);

	unsigned int idx = ((low - 0xa1) * 94 + (high - 0xa1)) * 32;

	return (unsigned short*)(gCCFontBase + idx);
}


int __drawCC(unsigned char* str, int color, DWORD pos, DWORD bgcolor) {

	int len = __strlen((char*)str);

	unsigned char* showpos = pos + (unsigned char*)gGraphBase;

	unsigned char* keepy = showpos;
	unsigned char* keepx = showpos;

	for (int i = 0; i < len; )
	{
		unsigned char chlow = str[i];
		if (chlow == '\n')
		{
			int posy = (unsigned int)(showpos - gGraphBase) / gBytesPerLine;
			unsigned int nowpos = __getpos(0, posy + GRAPH_CHINESECHAR_HEIGHT);
			if (nowpos >= gWindowSize)
			{
				nowpos = 0;
			}
			showpos = (unsigned char*)nowpos + gGraphBase;

			keepx = showpos;
			keepy = showpos;
			i++;
			continue;
		}
		else if (chlow == '\r')
		{
			int posy = (unsigned int)(showpos - gGraphBase) / gBytesPerLine;
			showpos = (unsigned char*)__getpos(0, posy) + gGraphBase;
			keepx = showpos;
			keepy = showpos;
			i++;
			continue;
		}
		else if (chlow == 0) {
			break;
		}

		unsigned char chhigh = str[i + 1];

		if (chlow >= 0xa0 && chhigh >= 0xa0)
		{
			//nothing to do
		}
		else if (chlow > 0 && chlow < 0x80)
		{
			unsigned char copychar[2];
			copychar[0] = chlow;
			copychar[1] = 0;
			i++;
			__drawGraphChars((char*)copychar, color);
			continue;
		}
		else {
			break;
		}

		unsigned short chw = *(WORD*)(str + i);
		i += 2;

		unsigned short* lpcc = getGBKCCIdx(chw);

		for (int j = 0; j < 16; j++)
		{
			unsigned short f = lpcc[j];
			f = __ntohs(f);
			unsigned int m = 0x8000;
			for (int k = 0; k < 16; k++)
			{
				unsigned int c = 0;
				if (f & m)
				{
					c = color;
					for (int n = 0; n < gBytesPerPixel; n++)
					{
						*showpos = (unsigned char)c;
						c = c >> 8;
						showpos++;
					}
				}
				else if (bgcolor) {
					c = bgcolor;
					for (int n = 0; n < gBytesPerPixel; n++)
					{
						*showpos = (unsigned char)c;
						c = c >> 8;
						showpos++;
					}
				}
				else {
					showpos += gBytesPerPixel;
				}

				m = m >> 1;
			}

			keepx += gBytesPerLine;
			showpos = keepx;
		}

		keepy = keepy + GRAPH_CHINESECHAR_WIDTH * gBytesPerPixel;

		//必须要能整除字体分辨率才行
		int mod = (unsigned int)(keepy - gGraphBase) % gBytesPerLine;
		if (mod == 0)
		{
			keepy = keepy + (GRAPH_CHINESECHAR_HEIGHT - 1) * gBytesPerLine;
			if ((unsigned int)(keepy + GRAPH_CHINESECHAR_HEIGHT * gBytesPerLine - gGraphBase) >= gWindowSize)
			{
				keepy = (unsigned char*)gGraphBase;
			}
		}

		keepx = keepy;
		showpos = keepy;
	}
	return (int)(showpos - gGraphBase);
}


int __drawCCS(unsigned char* font, int color) {
	unsigned int pos = __getpos(gShowX, gShowY);
	int resultpos = __drawCC(font, color, pos, BACKGROUND_COLOR);

	gShowY = resultpos / gBytesPerLine;

	gShowX = (resultpos % gBytesPerLine) / gBytesPerPixel;

	if (gShowY >= gWindowHeight)
	{
		gShowY = 0;
		gShowX = 0;
	}
	return 0;
}


int __drawGraphCharInt(char* font, int color, int pos, int bgcolor) {
	int params[4];
	__memset((char*)params, 0, 4 * sizeof(int));
	params[0] = (int)font;
	params[1] = color;
	params[2] = pos;
	params[3] = bgcolor;
	__asm {
		mov eax, 4
		lea edi, params
		int 80h
	}
}




int __restoreCircle(int x, int y, int radius, unsigned char* backup) {

	__kRestoreMouse();

	int squreRadius = radius * radius;

	int pixelcnt = 0;

	int startx = x - radius;
	int endx = x + radius;
	int starty = y - radius;
	int endy = y + radius;

	unsigned int pos = __getpos(startx, starty);
	unsigned char* showpos = pos + (unsigned char*)gGraphBase;

	unsigned char* keepy = showpos;
	for (int i = starty; i <= endy; i++)
	{
		for (int j = startx; j <= endx; j++)
		{
			int deltaX2 = (j - x) * (j - x);
			int deltaY2 = (i - y) * (i - y);
			if (deltaY2 + deltaX2 <= squreRadius)
			{
				for (int i = 0; i < gBytesPerPixel; i++)
				{
					if (*showpos != *backup) {
						*showpos = *backup;
					}
					showpos++;
					backup++;
				}

				pixelcnt++;
			}
			else {
				showpos += gBytesPerPixel;
			}
		}
		keepy += gBytesPerLine;
		showpos = keepy;
	}
	__kRefreshMouseBackup();
	__kDrawMouse();

	return (unsigned int)showpos - gGraphBase;
}

extern "C"  __declspec(dllexport) int __drawColorCircle(int x, int y, int radius, int color, unsigned char* back) {

	__kRestoreMouse();

	int squreRadius = radius * radius;

	int pixelcnt = 0;

	unsigned char* backup = back;

	int startx = x - radius;
	int endx = x + radius;
	int starty = y - radius;
	int endy = y + radius;

	unsigned int pos = __getpos(startx, starty);
	unsigned char* showpos = pos + (unsigned char*)gGraphBase;
	//unsigned char * keepx = showpos;
	unsigned char* keepy = showpos;
	for (int i = starty; i <= endy; i++)
	{
		for (int j = startx; j <= endx; j++)
		{
			int deltaX2 = (j - x) * (j - x);
			int deltaY2 = (i - y) * (i - y);
			if (deltaY2 + deltaX2 <= squreRadius)
			{

				unsigned int c = color;
				//(color)++;

				for (int i = 0; i < gBytesPerPixel; i++)
				{

					if (back && (*backup != *showpos)) {
						*backup = *showpos;
					}
					backup++;

					if (*showpos != (c&0xff)) {
						*showpos = c;
					}
					showpos++;
					c = (c >> 8);
				}

				pixelcnt++;
			}
			else {
				showpos += gBytesPerPixel;
			}
		}
		keepy += gBytesPerLine;
		showpos = keepy;
	}
	__kRefreshMouseBackup();
	__kDrawMouse();

	return (unsigned int)showpos - gGraphBase;
}



int __drawCircle(int x, int y, int radius, int color, unsigned char* back) {
	__kRestoreMouse();

	int squreRadius = radius * radius;

	int pixelcnt = 0;

	unsigned char* backup = back;

	int startx = x - radius;
	int endx = x + radius;
	int starty = y - radius;
	int endy = y + radius;

	unsigned int pos = __getpos(startx, starty);
	unsigned char* showpos = pos + (unsigned char*)gGraphBase;
	//unsigned char * keepx = showpos;
	unsigned char* keepy = showpos;
	for (int i = starty; i <= endy; i++)
	{
		for (int j = startx; j <= endx; j++)
		{
			int deltaX2 = (j - x) * (j - x);
			int deltaY2 = (i - y) * (i - y);
			if (deltaY2 + deltaX2 <= squreRadius)
			{
				unsigned int c = color;
				for (int i = 0; i < gBytesPerPixel; i++)
				{
					if (back && (*backup != *showpos)) {
						*backup = *showpos;
					}
					backup++;

					if (*showpos != (c & 0xff)) {
						*showpos = c;
					}
					showpos++;
					c = (c >> 8);
				}

				pixelcnt++;
			}
			else {
				showpos += gBytesPerPixel;
			}
		}
		keepy += gBytesPerLine;
		showpos = keepy;
	}
	__kRefreshMouseBackup();
	__kDrawMouse();

	return (unsigned int)showpos - gGraphBase;
}





int removeFileManager(LPFMWINDOW w) {
	__restoreRectWindow(&w->window.pos, w->window.width, w->window.height, (unsigned char*)w->window.backBuf);

	removeWindow(w->window.id);

	__kFree(w->window.backBuf);

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

	__strcpy(w->window.winname, "FileManagerWindow");

	w->window.backBuf = (DWORD)__kMalloc(w->window.backsize);
	if (w->window.backBuf) {
		__drawRectWindow(&w->window.pos, w->window.width, w->window.height, w->window.color, (unsigned char*)w->window.backBuf);
	}

	w->window.id = addWindow((DWORD)&(w->window), (DWORD*)&w->window.pos.x, (DWORD*)&w->window.pos.y, 0, w->window.winname);

	return 0;
}



int __drawFileIcon(LPFILEICON computer) {

	__kRestoreMouse();

	computer->backsize = gBytesPerPixel * (computer->width + computer->frameSize) * (computer->height + computer->frameSize);

	unsigned int ptr = __drawRectFrameWindow(&computer->pos, computer->width, computer->height,
		computer->color, computer->frameSize, computer->frameColor, (char*)computer->backGround);

	DWORD offset = 0;
	DWORD size = 0;
	int ret = getResFromID(MAIN_DLL_BASE, computer->id, 3, &offset, &size);
	if (offset && size)
	{
		LPBITMAPINFOHEADER bmpinfo = (LPBITMAPINFOHEADER)offset;
		bmpinfo->biHeight = bmpinfo->biHeight / 2;
		unsigned char* data = (unsigned char*)offset + sizeof(BITMAPINFOHEADER);
		showBmpBits(computer->pos.x, computer->pos.y, (LPBITMAPINFOHEADER)offset, data);
	}

	ptr = ptr + GRAPHCHAR_HEIGHT * gBytesPerLine * computer->zoomin + GRAPHCHAR_HEIGHT * 4 * gBytesPerPixel * computer->zoomin;

	computer->showX = (ptr % gBytesPerLine) / gBytesPerPixel;
	computer->showY = ptr / gBytesPerLine;

	int showend = __drawFileIconChars(computer);

	__kRefreshMouseBackup();
	__kDrawMouse();

	return showend;
}




int __clearWindowChar(WINDOWCLASS* window) {

	window->showX -= GRAPHCHAR_WIDTH * window->zoomin;
	if ((window->showX < window->pos.x + (window->frameSize >> 1)) && (window->showY > window->pos.y + window->capHeight + (window->frameSize >> 1)))
	{
		window->showX = window->pos.x + (window->frameSize >> 1) + window->width - GRAPHCHAR_WIDTH * window->zoomin;

		window->showY -= (GRAPHCHAR_HEIGHT * window->zoomin);
		if (window->showY < window->pos.y + window->capHeight + (window->frameSize >> 1))
		{
			window->showY = window->pos.y + (window->frameSize >> 1) + window->capHeight;
		}
	}
	else if ((window->showX < window->pos.x + (window->frameSize >> 1)) && (window->showY <= window->pos.y + window->capHeight + (window->frameSize >> 1)))
	{
		window->showX = window->pos.x + (window->frameSize >> 1);
		window->showY = window->pos.y + (window->frameSize >> 1) + window->capHeight;
	}

	int x = window->showX;
	int y = window->showY;
	int showpos = __drawWindowChars(( char*)" ", DEFAULT_FONT_COLOR, window);
	window->showX = x;
	window->showY = y;
	return showpos;
}


int __drawWindowChars( char* str, int color, WINDOWCLASS* window) {

	int len = __strlen((char*)str);

	unsigned int pos = __getpos(window->showX, window->showY) + gGraphBase;

	unsigned char* showpos = (unsigned char*)pos;
	unsigned char* keepy = showpos;
	unsigned char* keepx = keepy;

	for (int i = 0; i < len; i++)
	{
		unsigned int ch = str[i];
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
					c = window->color;
					for (int n = 0; n < gBytesPerPixel * window->zoomin; n++)
					{
						*showpos = c;
						c = c >> 8;
						showpos++;
					}
				}
				m = m >> 1;
			}

			keepx += gBytesPerLine * window->zoomin;
			showpos = keepx;
		}

		keepy = keepy + GRAPHCHAR_WIDTH * gBytesPerPixel * window->zoomin;

		int posx = (((unsigned int)keepy - gGraphBase) % gBytesPerLine) / gBytesPerPixel;
		if (posx >= window->pos.x + (window->frameSize >> 1) + window->width)
		{
			posx = (window->pos.x + (window->frameSize >> 1));
			int posy = (unsigned int)(keepy - gGraphBase) / gBytesPerLine;
			posy += (GRAPHCHAR_HEIGHT * window->zoomin);
			if (posy >= window->pos.y + window->capHeight + (window->frameSize >> 1) + window->height)
			{
				posy = window->pos.y + window->capHeight + (window->frameSize >> 1);
			}
			keepy = (unsigned char*)__getpos(posx, posy) + gGraphBase;
		}

		keepx = keepy;
		showpos = keepy;
	}

	int resultpos = (int)(showpos - gGraphBase);

	window->showX = (resultpos % gBytesPerLine) / gBytesPerPixel;
	window->showY = (resultpos / gBytesPerLine);

	return (int)(showpos - gGraphBase);
}










int __drawHorizon(int x, int y, int len, int color) {
	unsigned char* pos = (unsigned char*)__getpos(x, y);
	for (int i = 0; i < len; i++)
	{
		int c = color;
		for (int j = 0; j < gBytesPerPixel; j++)
		{
			*pos = (c & 0xff);
			c = c >> 8;
			pos++;
		}
	}
	return len;
}


int __drawVertical(int x, int y, int len, int color) {
	unsigned char* pos = (unsigned char*)__getpos(x, y);
	unsigned char* keep = pos;
	for (int i = 0; i < len; i++)
	{
		int c = color;
		for (int j = 0; j < gBytesPerPixel; j++)
		{
			*pos = (c & 0xff);
			c = c >> 8;
			pos++;
		}
		keep += gBytesPerLine;
		pos = keep;
	}
	return len;
}

int __drawDot(int x, int y, DWORD color) {
	char* ptr = (char*)__getpos(x, y) + gGraphBase;
	char* c = (char*)&color;
	for (int k = 0; k < gBytesPerPixel; k++)
	{
		*ptr = *c;
		ptr++;
		c++;
	}

	return 1;
}

int __drawLine(int x1, int y1, int x2, int y2, DWORD color) {

	int ret = 0;
	if (x1 == x2) {
		if (y1 == y2) {
			return __drawDot(x1, y1, color);
		}
		else {
			if (y1 >= y2) {
				return __drawVertical(x2, y2, y1-y2, color);
			}
			else {
				return __drawVertical(x1, y1, y2-y1, color);
			}	
		}
	}

	double k = (y2 - y1) / (x2 - x1);
	if (k == 0) {
		//
	}

	int lx = x1;
	int sx = x2;
	int ly = y1;
	int sy = y2;
	if (lx < x2) {
		lx = x2;
		sx = x1;

		ly = y2;
		sy = y1;
	}

	int dx = lx - sx;
	for (int i = 0; i < dx; i++) {
		int px = sx + i;
		int py = k*i + sy;
		ret = __drawDot(px, py,color);
	}
	return ret;
}


int __diamond(int startx, int starty, int raduis, int cnt, DWORD color) {

	int n = cnt, i, j;
	double t = 3.14159 * 2 / n, r = raduis;
	double x0 = startx, y0 = starty, x[64], y[64];
	for (i = 0; i < n; i++)
	{
		x[i] = r * cos(i * t) + x0;
		y[i] = r * sin(i * t) + y0;
	}

	for (i = 0; i <= n - 2; i++) {
		for (j = i + 1; j <= n - 1; j++) {
			__drawLine(x[i], y[i], x[j], y[j], color);
		}
	}

	return 0;
}


int __diamond2(int startx, int starty, int raduis, int cnt, DWORD color) {
	//功能：一笔绘制金刚石图案（n>=5,n是奇数）

	int n = cnt, i, j;
	double t = 3.14159 * 2 / n, r = raduis;
	double x0 = startx, y0 = starty, x[64], y[64];
	for (i = 0; i < n; i++)
	{
		x[i] = r * cos(i * t) + x0;
		y[i] = r * sin(i * t) + y0;
	}

	for (i = 1; i <= n / 2; i++) {

		for (j = 0; j < n; j++)
		{
			if ((j + i) >= n)
			{
				int r = (j + i) % n;
				__drawLine(x[j], y[j], x[r], y[r], color);
			}
			else {
				__drawLine(x[j], y[j], x[j + i], y[j + i], color);
			}
		}
	}

	return 0;
}