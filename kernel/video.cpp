
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
#include "apic.h"

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

//int gShowX = 0;
//int gShowY = 0;

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
	gGraphBase = (DWORD)vesaInfo->PhyBasePtr + vesaInfo->OffScreenMemOffset;

	//gBytesPerFont = gBytesPerPixel * GRAPHCHAR_WIDTH;
	//gFontLineSize = gBytesPerLine * GRAPHCHAR_HEIGHT;

	gWindowHeight = gVideoHeight - TASKBAR_HEIGHT;
	gWindowSize = gWindowHeight * gBytesPerLine;

	gSmallFolderHeight = gVideoHeight / 20;
	gSmallFolderWidth = gVideoWidth / 20;

	gBigFolderHeight = gVideoHeight / 10;
	gBigFolderWidth = gVideoWidth / 10;

	//gShowY = 0;
	//gShowX = 0;

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

int __drawMinimize(LPWINDOWCLASS window) {
	if (window->capHeight == 0)
	{
		return TRUE;
	}

	int singlefs = window->frameSize >> 1;

	int x = window->pos.x + singlefs + window->width - window->capHeight - window->capHeight;

	int y = window->pos.y + singlefs;

	window->minx = x;
	window->miny = y;

	char* videoBase = GetVideoBase();
	unsigned char* pos = (unsigned char*)__getpos(x, y) + (unsigned long)videoBase;

	unsigned char* keepy = pos;

	for (int i = 0; i < window->capHeight; i++)
	{
		for (int j = 0; j < window->capHeight; j++)
		{
			if ( i == 0 || j == 0 || i== (window->capHeight-1)||y== (window->capHeight-1) )	//y坐标轴向下是正号，y=8 - x
			{
				int color = DEFAULT_FONT_COLOR;
				for (int k = 0; k < gBytesPerPixel; k++)
				{
					*pos = color;
					pos++;
					color = color >> 8;
				}
			}
			else if (i== window->capHeight/2 ) {
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
	return (DWORD)pos - (DWORD)videoBase;
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

	char* videoBase = GetVideoBase();
	unsigned char* pos = (unsigned char*)__getpos(x, y) +(unsigned long) videoBase;

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
	return (DWORD)pos - (DWORD)videoBase;
}


int __drawRectFrame(LPPOINT p, int width, int height, int color, int framesize, int framecolor, char* backbuf) {
	char* videoBase = GetVideoBase();
	DWORD startpos = (DWORD)__getpos(p->x, p->y) + (DWORD)videoBase;
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

	return (DWORD)ptr - (DWORD)videoBase;
}

int __restoreRectFrame(LPPOINT p, int width, int height, int framesize, unsigned char* backup) {
	char* videoBase = GetVideoBase();
	DWORD startpos = p->y * gBytesPerLine + p->x * gBytesPerPixel + (DWORD)videoBase;
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

	return (DWORD)ptr - (DWORD)videoBase;
}

int __drawRectangleFrameCaption(LPPOINT p, int width, int height, int color, int framesize, int framecolor, int capsize,
	int capcolor, char* capname, char* backdata) {
	int ret = 0;
	char* videoBase = GetVideoBase();
	DWORD startpos = __getpos(p->x, p->y) + (DWORD)videoBase;

	unsigned char* ptr = (unsigned char*)startpos;
	unsigned char* keep = ptr;

	int singlefs = framesize >> 1;

	for (int y = 0; y < height + framesize + capsize; y++)
	{
		for (int x = 0; x < width + framesize; x++)
		{
			int c = 0;

			if ((y < singlefs) || (y >= height + singlefs + capsize))
			{
				c = framecolor;
			}
			else if ((y >= singlefs) && (y < capsize + singlefs))
			{
				c = capcolor;
			}
			else if ((x < singlefs) || (x >= width + singlefs))
			{
				c = framecolor;
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

	return (DWORD)showend - (DWORD)videoBase;
}

int __restoreWindow(LPWINDOWCLASS window) {
	__kRestoreMouse();

	char* videoBase = GetVideoBase();
	int startpos = window->pos.y * gBytesPerLine + window->pos.x * gBytesPerPixel + (DWORD)videoBase;
	unsigned char* ptr = (unsigned char*)startpos;
	unsigned char* keep = ptr;
	unsigned char* srcdata = (unsigned char*)window->backBuf;

	for (int i = 0; i < window->height + window->frameSize + window->capHeight; i++)
	{
		for (int j = 0; j < window->width + window->frameSize; j++)
		{
			for (int k = 0; k < gBytesPerPixel; k++)
			{
				//if (window->backBuf  && *ptr != *srcdata)
				{
					*ptr = *srcdata;
				}
				ptr++;
				srcdata++;
			}
		}

		keep += gBytesPerLine;
		ptr = (unsigned char*)keep;
	}

	__kRefreshMouseBackup();

	__kDrawMouse();

	return (DWORD)ptr - (DWORD)videoBase;
}

int __DestroyWindow(LPWINDOWCLASS window) {

	int size = __restoreWindow(window);

	removeWindow(window->id);

	if (window->backBuf)
	{
		__kFree(window->backBuf);
	}
	if (window->minBuf) {
		__kFree((DWORD)window->minBuf);
	}
	return size;
}


int __drawWindow(LPWINDOWCLASS window) {

	int ret = 0;

	__kRestoreMouse();

	window->backsize = gBytesPerPixel * (window->height + window->frameSize + window->capHeight) * (window->width + window->frameSize);

	window->backBuf = __kMalloc(window->backsize);

	ret = __drawRectangleFrameCaption(&window->pos, window->width, window->height, window->color, window->frameSize, window->frameColor,
		window->capHeight, window->capColor, window->caption, (char*)window->backBuf);

	window->id = addWindow((WINDOWCLASS*)window, window->winname);

	ret = __drawShutdown(window);
	__drawMinimize(window);

	__kRefreshMouseBackup();

	__kDrawMouse();

	return 0;
}




int __drawRectWindow(LPPOINT p, int width, int height, int color, unsigned char* backbuf) {
	unsigned char* backup = backbuf;
	__kRestoreMouse();
	char* videoBase = GetVideoBase();
	DWORD startpos = p->y * gBytesPerLine + p->x * gBytesPerPixel + (DWORD)videoBase;
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

				if (*ptr != (c & 0xff)) 
				{
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
	return (DWORD)ptr - (DWORD)videoBase;
}

int __DestroyRectWindow(LPPOINT p, int width, int height, unsigned char* backup) {
	__kRestoreMouse();
	unsigned char* back = backup;
	char* videoBase = GetVideoBase();
	int startpos = p->y * gBytesPerLine + p->x * gBytesPerPixel + (DWORD)videoBase;
	unsigned char* ptr = (unsigned char*)startpos;
	unsigned char* keep = ptr;

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			for (int k = 0; k < gBytesPerPixel; k++)
			{
				if (back && *ptr != *backup) 
				{
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
	return (DWORD)ptr - (DWORD)videoBase;
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


void clsClientRect(WINDOWCLASS * window) {
	char* videoBase = GetVideoBase();
	int startpos = window->showY * gBytesPerLine + window->showX * gBytesPerPixel + (DWORD)videoBase;
	unsigned char* ptr = (unsigned char*)startpos;
	unsigned char* keep = ptr;
	for (int i = 0; i < window->height; i++)
	{
		for (int j = 0; j < window->width; j++)
		{
			int c = window->color;
			for (int k = 0; k < gBytesPerPixel; k++)
			{
				*ptr = c & 0xff;
				ptr++;
				c = c >> 8;
			}
		}

		keep += gBytesPerLine;
		ptr = (unsigned char*)keep;
	}
}



int __backspaceChar() {
	int tid = __GetCurrentTid();
	WINDOWSINFO* winfo = __FindProcessWindow(tid);
	if (winfo == 0) {
		return 0;
	}
	WINDOWCLASS* window = winfo->window;
	window->showX -= GRAPHCHAR_WIDTH;
	if (window->showX < 0 && window->showY > 0)
	{
		window->showX = gVideoWidth - GRAPHCHAR_WIDTH;
		window->showY -= GRAPHCHAR_HEIGHT;
		if (window->showY <= 0)
		{
			window->showY = 0;
		}
	}
	else if (window->showX < 0 && window->showY <= 0)
	{
		window->showY = 0;
		window->showX = 0;
	}

	unsigned int pos = __getpos(window->showX, window->showY);
	int showpos = __drawGraphChar(( char*)" ", BACKGROUND_COLOR, pos, BACKGROUND_COLOR);

	return showpos;
}



int __drawGraphChars( char* str, int color) {
	if (g_ScreenMode == 0) {
		return FALSE;
	}
#ifdef LIUNUX_DEBUG_LOG_ON

	int* x = 0;
	int* y = 0;
	int ret = 0;

	int bkcolor = 0;
	LPWINDOWSINFO winfo = GetProcessTextPos(&x, &y);
	if (winfo ) {
		bkcolor = winfo->window->color;
	}
	else {
		bkcolor = BACKGROUND_COLOR;
	}

	unsigned int pos = __getpos(*x,*y);
	int resultpos = __drawGraphChar(str, color, pos, bkcolor);		//BACKGROUND_COLOR

	*y = resultpos / gBytesPerLine;

	*x = (resultpos % gBytesPerLine) / gBytesPerPixel;

	if (*y >= gWindowHeight)
	{
		*y = 0;
		*x = 0;
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
	unsigned char* videoBase = (unsigned char* )GetVideoBase();
	unsigned char* showpos = pos + (unsigned char*)videoBase;

	unsigned char* keepy = showpos;
	unsigned char* keepx = keepy;

	for (int i = 0; i < len; i++)
	{
		unsigned int ch = str[i];
		if (ch == '\n')
		{
			int posy = (unsigned int)(showpos - videoBase) / gBytesPerLine;
			unsigned int nowpos = __getpos(0, posy + GRAPHCHAR_HEIGHT);
			if (nowpos >= gWindowSize)
			{
				nowpos = 0;
			}
			showpos = (unsigned char*)nowpos + (DWORD)videoBase;

			keepx = showpos;
			keepy = showpos;
			continue;
		}
		else if (ch == '\r')
		{
			int posy = (unsigned int)(showpos - videoBase) / gBytesPerLine;
			showpos = (unsigned char*)__getpos(0, posy) + (DWORD)videoBase;
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

		int mod = (unsigned int)(keepy - videoBase) % gBytesPerLine;
		if (mod == 0)
		{
			keepy = keepy + (GRAPHCHAR_HEIGHT - 1) * gBytesPerLine;
			if ((unsigned int)(keepy + GRAPHCHAR_HEIGHT * gBytesPerLine - videoBase) >= gWindowSize)		//keep one line to show
			{
				keepy = (unsigned char*)videoBase;
			}
		}

		keepx = keepy;
		showpos = keepy;
	}
	return (int)(showpos - videoBase);
}


unsigned short* getGBKCCIdx(unsigned short gbk) {
	unsigned char low = (gbk & 0xff);
	unsigned char high = (gbk >> 8);

	unsigned int idx = ((low - 0xa1) * 94 + (high - 0xa1)) * 32;

	return (unsigned short*)(gCCFontBase + idx);
}


int __drawCC(unsigned char* str, int color, DWORD pos, DWORD bgcolor,WINDOWCLASS* w) {

	int len = __strlen((char*)str);

	unsigned char* videoBase = (unsigned char*)GetVideoBase();

	unsigned char* showpos = pos + (unsigned char*)videoBase;

	unsigned char* keepy = showpos;
	unsigned char* keepx = showpos;

	for (int i = 0; i < len; )
	{
		unsigned char chlow = str[i];
		if (chlow == '\n')
		{
			int posy = (unsigned int)(showpos - videoBase) / gBytesPerLine;
			int posx = (unsigned int)(( ((DWORD)showpos -(DWORD) videoBase) % gBytesPerLine)/gBytesPerPixel);
			if (w) {
				posx = w->showX;
			}
			unsigned int nowpos = __getpos(posx, posy + GRAPH_CHINESECHAR_HEIGHT);
			if (nowpos >= gWindowSize)
			{
				nowpos = 0;
			}
			showpos = (unsigned char*)nowpos +(DWORD) videoBase;

			keepx = showpos;
			keepy = showpos;
			i++;
			continue;
		}
		else if (chlow == '\r')
		{
			int posy = (unsigned int)(showpos - videoBase) / gBytesPerLine;
			int posx = 0;
			if (w) {
				posx = w->showX;
			}
			showpos = (unsigned char*)__getpos(posx, posy) + (DWORD)videoBase;
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
		int mod = (unsigned int)(keepy - videoBase) % gBytesPerLine;
		if (mod == 0)
		{
			keepy = keepy + (GRAPH_CHINESECHAR_HEIGHT - 1) * gBytesPerLine;
			if ((unsigned int)(keepy + GRAPH_CHINESECHAR_HEIGHT * gBytesPerLine - videoBase) >= gWindowSize)
			{
				keepy = (unsigned char*)videoBase;
			}
		}

		keepx = keepy;
		showpos = keepy;
	}
	return (int)(showpos - videoBase);
}


int __drawCCS(unsigned char* font, int color) {

	int* x = 0;
	int* y = 0;
	GetProcessTextPos(&x, &y);
	unsigned int pos = __getpos(*x, *y);
	int resultpos = __drawCC(font, color, pos, BACKGROUND_COLOR,0);

	*y = resultpos / gBytesPerLine;

	*x = (resultpos % gBytesPerLine) / gBytesPerPixel;

	if (*y >= gWindowHeight)
	{
		*y = 0;
		*x = 0;
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




int __restoreCircle(int x, int y, int radius,int radius2, unsigned char* backup) {

	//__kRestoreMouse();

	int squreRadius = radius * radius;
	int squreRadius2 = radius2 * radius2;

	int pixelcnt = 0;

	int startx = x - radius;
	if (startx < 0) {
		startx = 0;
	}
	int endx = x + radius;
	if (endx > gVideoWidth) {
		endx = gVideoWidth ;
	}
	int starty = y - radius;
	if (starty < 0) {
		starty = 0;
	}
	int endy = y + radius;
	if (endy > gVideoHeight ) {
		endy = gVideoHeight ;
	}

	unsigned int pos = __getpos(startx, starty);
	unsigned char* videoBase = (unsigned char*)GetVideoBase();
	unsigned char* showpos = pos + (unsigned char*)videoBase;

	unsigned char* keepy = showpos;
	for (int i = starty; i <= endy; i++)
	{
		for (int j = startx; j <= endx; j++)
		{
			int deltaX2 = (j - x) * (j - x);
			int deltaY2 = (i - y) * (i - y);
			if((deltaY2 + deltaX2 <= squreRadius) && (deltaX2 + deltaY2 >= squreRadius2))
			{
				for (int i = 0; i < gBytesPerPixel; i++)
				{
					//if (*showpos != *backup) 
					{
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
	//__kRefreshMouseBackup();
	//__kDrawMouse();

	return (unsigned int)showpos - (DWORD)videoBase;
}

extern "C"  __declspec(dllexport) int __drawCircle(int x, int y, int radius,int radius2, int color, unsigned char* back) {

	//__kRestoreMouse();
	int tag = 0;
	if ((radius & 0x80000000) && (radius2 & 0x80000000)) {
		tag = 1;
		radius -= 0x80000000;
		radius2 -= 0x80000000;
	}

	int squreRadius = radius * radius;

	int squreRadius2 = radius2 * radius2;

	int pixelcnt = 0;

	unsigned char* backup = back;

	int startx = x - radius;
	if (startx < 0){
		startx = 0;
	}
	int endx = x + radius;
	if (endx > gVideoWidth ) {
		endx = gVideoWidth ;
	}
	int starty = y - radius;
	if (starty < 0) {
		starty = 0;
	}
	int endy = y + radius;
	if (endy > gVideoHeight ) {
		endy = gVideoHeight;
	}

	unsigned int pos = __getpos(startx, starty);
	unsigned char* videoBase = (unsigned char*)GetVideoBase();
	unsigned char* showpos = pos + (unsigned char*)videoBase;
	//unsigned char * keepx = showpos;
	unsigned char* keepy = showpos;
	for (int i = starty; i <= endy; i++)
	{
		for (int j = startx; j <= endx; j++)
		{
			int deltaX2 = (j - x) * (j - x);
			int deltaY2 = (i - y) * (i - y);
			if ( (deltaY2 + deltaX2 <= squreRadius) && (deltaX2 + deltaY2 >= squreRadius2 ) )
			{
				unsigned int c = color;
				if (tag) {
					int cc = ((int)__sqrt(1.0*deltaX2 + 1.0*deltaY2)) & 0xff;
					c = cc | (cc << 8) | (cc << 16);
				}

				for (int i = 0; i < gBytesPerPixel; i++)
				{

					//if (back && (*backup != *showpos)) 
					{
						*backup = *showpos;
					}
					backup++;

					//if (*showpos != (c&0xff)) 
					{
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
	//__kRefreshMouseBackup();
	//__kDrawMouse();

	return (unsigned int)showpos - (DWORD)videoBase;
}









int removeFileManager(LPFMWINDOW w) {
	//__DestroyRectWindow(&w->window.pos, w->window.width, w->window.height, (unsigned char*)w->window.backBuf);
	__DestroyWindow(&w->window);

	//removeWindow(w->window.id);

	//__kFree(w->window.backBuf);

	//__terminatePid(w->pid);
	return 0;
}


int drawFileManager(LPFMWINDOW w) {
	w->window.capHeight = GRAPHCHAR_HEIGHT*2;
	w->window.frameSize = GRAPHCHAR_WIDTH;
	w->window.frameColor = FOLDERFONTBGCOLOR;
	w->window.capColor = 0x00ffff;

	w->window.next = 0;
	w->window.prev = 0;

	w->cpl = 3;
	w->window.color = 0xffffff;
	
	w->window.fontcolor = 0;
	w->window.height = gWindowHeight - w->window.capHeight - w->window.frameSize;
	w->window.width = gVideoWidth - w->window.frameSize;
	w->window.pos.x = 0;
	w->window.pos.y = 0;
	w->window.showX = w->window.pos.x + w->window.frameSize/2;
	w->window.showY = w->window.pos.y + w->window.frameSize/2 + w->window.capHeight;
	w->fsheight = GRAPHCHAR_HEIGHT * w->cpl;

	w->window.left = w->window.pos.x + w->window.frameSize/2;
	w->window.top = w->window.pos.y + (w->window.frameSize >> 1) + w->window.capHeight;

	w->window.right = w->window.pos.x + w->window.frameSize + w->window.width - 1;
	w->window.bottom = w->window.pos.y + (w->window.frameSize >> 1) + +w->window.capHeight + w->window.height - 1;

	w->window.zoomin = 1;

	w->window.backsize = gBytesPerPixel * (w->window.width) * (w->window.height);

	__strcpy(w->window.winname,(char*) "FileManagerWindow");

	//w->window.backBuf = (DWORD)__kMalloc(w->window.backsize);
	//if (w->window.backBuf) 
	{
		//__drawRectWindow(&w->window.pos, w->window.width, w->window.height, w->window.color, (unsigned char*)w->window.backBuf);
	}

	__drawWindow(&w->window);

	//w->window.id = addWindow((DWORD)&(w->window), (DWORD*)&w->window.pos.x, (DWORD*)&w->window.pos.y, 0, w->window.winname);

	return 0;
}



int __drawFileIcon(LPFILEICON computer) {

	__kRestoreMouse();

	computer->backsize = gBytesPerPixel * (computer->width + computer->frameSize) * (computer->height + computer->frameSize);

	unsigned int ptr = __drawRectFrame(&computer->pos, computer->width, computer->height,
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

	unsigned char* videoBase = (unsigned char*)GetVideoBase();

	unsigned int pos = __getpos(window->showX, window->showY) + (DWORD)videoBase;

	unsigned char* showpos = (unsigned char*)pos;
	unsigned char* keepy = showpos;
	unsigned char* keepx = keepy;

	for (int i = 0; i < len; i++)
	{
		unsigned int ch = str[i];
		if (ch == '\n')
		{
			int posy = (unsigned int)(showpos - videoBase) / gBytesPerLine;
			int posx = window->pos.x + (window->frameSize >> 1);

			posy += (GRAPHCHAR_HEIGHT * window->zoomin);
			if (posy >= window->pos.y + window->height + window->capHeight + (window->frameSize >> 1))
			{
				posy = window->pos.y + window->capHeight + (window->frameSize >> 1);
			}
			showpos = (unsigned char*)__getpos(posx, posy) + (DWORD)videoBase;

			keepx = showpos;
			keepy = showpos;
			continue;
		}
		else if (ch == '\r')
		{
			int posy = (unsigned int)(showpos - videoBase) / gBytesPerLine;
			int posx = window->pos.x + (window->frameSize >> 1);
			showpos = (unsigned char*)__getpos(posx, posy) + (DWORD)videoBase;
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

		int posx = (((unsigned int)keepy - (DWORD)videoBase) % gBytesPerLine) / gBytesPerPixel;
		if (posx >= window->pos.x + (window->frameSize >> 1) + window->width)
		{
			posx = (window->pos.x + (window->frameSize >> 1));
			int posy = (unsigned int)(keepy - videoBase) / gBytesPerLine;
			posy += (GRAPHCHAR_HEIGHT * window->zoomin);
			if (posy >= window->pos.y + window->capHeight + (window->frameSize >> 1) + window->height)
			{
				posy = window->pos.y + window->capHeight + (window->frameSize >> 1);
			}
			keepy = (unsigned char*)__getpos(posx, posy) + (DWORD)videoBase;
		}

		keepx = keepy;
		showpos = keepy;
	}

	int resultpos = (int)(showpos - videoBase);

	window->showX = (resultpos % gBytesPerLine) / gBytesPerPixel;
	window->showY = (resultpos / gBytesPerLine);

	return (int)(showpos - videoBase);
}










int __drawHorizon(int x, int y, int len, int colorBuf, int color, char* bak) {
	unsigned char* videoBase = (unsigned char*)GetVideoBase();
	unsigned char* pos = (unsigned char*)__getpos(x, y) + (DWORD)videoBase;
	unsigned char* back = (unsigned char*)bak;
	DWORD* lpc = (DWORD*)color;
	for (int i = 0; i < len; i++)
	{
		int c = color;
		if (colorBuf) {
			
			c = *lpc;
			lpc++;
		}
		for (int j = 0; j < gBytesPerPixel; j++)
		{
			if (bak) {
				*back = *pos;
				back++;
			}

			*pos = (c & 0xff);
			c = c >> 8;
			pos++;
			
		}
	}
	return len;
}


int __drawVertical(int x, int y, int len, int colorBuf, int color, char* bak) {
	unsigned char* videoBase = (unsigned char*)GetVideoBase();
	unsigned char* pos = (unsigned char*)__getpos(x, y)+(DWORD) videoBase;
	unsigned char* keep = pos;
	unsigned char* back =(unsigned char*) bak;
	DWORD* lpc = (DWORD*)color;
	for (int i = 0; i < len; i++)
	{
		int c = color;
		if (colorBuf) {
			
			c = *lpc;
			lpc++;
		}
		for (int j = 0; j < gBytesPerPixel; j++)
		{
			if (bak) {
				*back = *pos;
				back++;
			}
			*pos = (c & 0xff);
			c = c >> 8;
			pos++;
		}
		keep += gBytesPerLine;
		pos = keep;
	}
	return len;
}

int __drawDot(int x, int y, int colorBuf, DWORD color,char * bak) {
	unsigned char* videoBase = (unsigned char*)GetVideoBase();
	char* ptr = (char*)__getpos(x, y) + (DWORD)videoBase;
	char* c = (char*)&color;
	char* lpc = (char*)color;
	if (colorBuf) {

		c = lpc;
		lpc++;
	}
	unsigned char* back = (unsigned char*)bak;
	for (int k = 0; k < gBytesPerPixel; k++)
	{
		if (bak) {
			*back = *ptr;
			back++;
		}
		*ptr = *c;
		ptr++;
		c++;
	}

	return 1;
}

int __drawLine(int x1, int y1, int x2, int y2,int colorBuf, DWORD color, char* bak) {

	int ret = 0;
	if (x1 == x2) {
		if (y1 == y2) {
			
			return __drawDot(x1, y1, colorBuf, color,bak);
		}
		else {
			if (y1 >= y2) {
				return __drawVertical(x2, y2, y1-y2, colorBuf, color,bak);
			}
			else {
				return __drawVertical(x1, y1, y2-y1, colorBuf, color,bak);
			}	
		}
	}

	if (y1 == y2) {
		if (x1 == x2) {
			return __drawDot(x1, y1, colorBuf, color,bak);
		}
		else {
			if (x1 >= x2) {
				return __drawHorizon(x2, y2, x1 - x2, colorBuf, color,bak);
			}
			else {
				return __drawHorizon(x1, y1, x2 - x1, colorBuf, color,bak);
			}
		}
	}

	double k = 0.1;

	int reverse = 0;

	int lx = 0;
	int sx = 0;
	int ly = 0;
	int sy = 0;
	if ( __abs((double)x2 - (double)x1) >= __abs((double)y2 - (double)y1) ) {
		k = ( 1.0* ( (double)y2 - (double)y1) )/ ( 1.0*( (double)x2 - (double)x1) );
		if (x2 >= x1) {	
			sx = x1;
			lx = x2;
			sy = y1;
			ly = y2;			
		}
		else {
			sx = x2;
			lx = x1;
			sy = y2;
			ly = y1;
		}
	}
	else {
		k = ( 1.0* ((double)x2 - (double)x1) ) /( 1.0*( (double)y2 - (double)y1 ) );
		if (y2 >= y1) {
			sx = y1;
			lx = y2;
			sy = x1;
			ly = x2;		
		}
		else {
			sx = y2;
			lx = y1;
			sy = x2;
			ly = x1;	
		}

		reverse = 1;
	}
	
	int dx = lx - sx;
	for (int i = 0; i < dx; i++) {
		int px = sx + i;
		int py = sy + k*(1.0*(double)i) ;
		if (reverse) {
			ret = __drawDot(py, px, colorBuf, color,bak);
		}
		else {
			ret = __drawDot(px, py, colorBuf, color,bak);
		}	
		if (bak) {
			bak += gBytesPerPixel;
		}
		if (colorBuf) {
			color += gBytesPerPixel;
		}
	}
	return ret;
}


int __diamond(int startx, int starty, int raduis, int cnt, DWORD color) {

	int n = cnt, i, j;
	double t = PI * 2 / n, r = raduis;
	float x0 = startx, y0 = starty;
	double* x = (double*)__kMalloc(sizeof(double) * (cnt + 16));
	double* y = (double*)__kMalloc(sizeof(double) * (cnt + 16));
	for (i = 0; i < n; i++)
	{
		x[i] = r * __cos(i * t) + x0;
		y[i] = r * __sin(i * t) + y0;
	}

	for (i = 0; i <= n - 2; i++) {
		for (j = i + 1; j <= n - 1; j++) {
			__drawLine(x[i], y[i], x[j], y[j],0, color,0);
			color += 1;
		}
	}

	__kFree((unsigned long)x);
	__kFree((unsigned long)y);
	return 0;
}


int __diamond2(int startx, int starty, int raduis, int cnt, DWORD color) {
	//功能：一笔绘制金刚石图案（n>=5,n是奇数）

	int n = cnt, i, j;
	double t = PI * 2 / n, r = raduis;
	float x0 = startx, y0 = starty;
	double* x = (double*)__kMalloc(sizeof(double) * (cnt + 16));
	double* y = (double*)__kMalloc(sizeof(double) * (cnt + 16));
	for (i = 0; i < n; i++)
	{
		x[i] = r * __cos(i * t) + x0;
		y[i] = r * __sin(i * t) + y0;
	}

	for (i = 1; i <= n / 2; i++) {

		for (j = 0; j < n; j++)
		{
			if ((j + i) >= n)
			{
				int r = (j + i) % n;
				__drawLine(x[j], y[j], x[r], y[r], 0,color,0);
			}
			else {
				__drawLine(x[j], y[j], x[j + i], y[j + i],0, color,0);
			}
		}
	}
	__kFree((unsigned long)x);
	__kFree((unsigned long)y);
	return 0;
}



void initDesktopWindow(WINDOWCLASS* window, char* name, int tid,int show) {
	int ret = 0;
	__memset((char*)window, 0, sizeof(WINDOWCLASS));

	window->capColor = 0;
	window->capHeight = 0;
	window->color = BACKGROUND_COLOR;
	window->frameSize = 0;
	window->frameColor = 0;
	__strcpy(window->caption, name);
	__strcpy(window->winname, name);

	window->backBuf = 0;

	window->pos.x = FULLWINDOW_TOP;

	window->pos.y = FULLWINDOW_LEFT;

	window->width = gVideoWidth + window->frameSize;

	window->height = gWindowHeight + window->frameSize + window->capHeight;

	window->zoomin = 1;

	window->tid = tid;

	LPPROCESS_INFO tss = (LPPROCESS_INFO)TASKS_TSS_BASE;
	LPPROCESS_INFO proc = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	window->pid = proc->pid;

	window->left = (window->frameSize >> 1) + window->pos.x;
	window->top = (window->frameSize >> 1) + window->capHeight + window->pos.y;
	window->right = window->left + window->width - 1;
	window->bottom = window->top + window->height - 1;

	window->showX = window->left;
	window->showY = window->top;
	//window->cursorX = window->showX;
	//window->cursorY = window->showY;
	window->cursorColor = ~window->color;

	window->minBuf = 0;

	window->id = addWindow((WINDOWCLASS*)window, window->winname);

	if (show) {
		ret = __drawRectWindow(&window->pos, window->width, window->height, window->color, (unsigned char*)window->backBuf);
	}

	window->prev = 0;
	window->next = 0;

	proc->window = window->id;
	tss[tid].window = window->id;
}