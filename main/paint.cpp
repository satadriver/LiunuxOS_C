
#pragma once
#include "paint.h"
#include "video.h"
#include "Utils.h"
#include "guiHelper.h"
#include "mouse.h"
#include "keyboard.h"
#include "task.h"
#include "file.h"
#include "graph.h"
#include "Pe.h"
#include "gdi/jpeg.h"
#include "window.h"

#define PENCIL_WIDTH	5

#define PALETTE_FACTOR	3


int __kPaint(unsigned int retaddr, int tid, char * filename, char * funcname, DWORD runparam) {
	char szout[1024];

	int retvalue = 0;

	WINDOWCLASS window;
	initFullWindow(&window, filename, tid);

	unsigned int pencilColor = 0x00;

	int tabley = window.top + window.height - 256;
	int tablex = window.left;

	int fx = tablex;
	int fy = tabley;
	initTableNew(&window,fx,fy,0,2 );
	fx += 256;
	initTableNew(&window, fx, fy, 1, 2);
	fx += 256;
	initTableNew(&window, fx, fy, 0, 1);

	unsigned char* videoBase = (unsigned char*)GetVideoBase();

	while (1)
	{
		unsigned int ck = __kGetKbd(window.id);
		//unsigned int ck = __getchar(window.id);
		unsigned int asc = ck & 0xff;
		if (asc == 0x1b)
		{
			__DestroyWindow(&window);
			return 0;
		}


		MOUSEINFO mouseinfo;
		__memset((char*)&mouseinfo, 0, sizeof(MOUSEINFO));
		retvalue = __kGetMouse(&mouseinfo, window.id);
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


			if (mouseinfo.x >= tablex && mouseinfo.x <= tablex + 256*3)
			{
				if (mouseinfo.y >= tabley && mouseinfo.y <= window.bottom)
				{
					__kRestoreMouse();
					unsigned int* pos = (unsigned int*)(__getpos(mouseinfo.x, mouseinfo.y) + videoBase);
					pencilColor = *(unsigned int*)pos;

// 					__printf(szout, "set color:%x\r\n", pencilColor);

					__kRefreshMouseBackup();
 					__kDrawMouse();
					__sleep(0);
					continue;
				}
			}

			if (mouseinfo.x >= window.left && mouseinfo.x <= window.right)
			{
				if (mouseinfo.y >= window.top && mouseinfo.y <= window.bottom)
				{
					int miny = mouseinfo.y - PENCIL_WIDTH / 2;
					if (miny < window.top)
					{
						miny = window.top;
					}

					int minx = mouseinfo.x - PENCIL_WIDTH / 2;
					if (minx < window.left)
					{
						minx = window.left;
					}

					int maxy = mouseinfo.y + PENCIL_WIDTH / 2;
					if (maxy > window.bottom)
					{
						maxy = window.bottom;
					}

					int maxx = mouseinfo.x + PENCIL_WIDTH / 2;
					if (maxx > window.right)
					{
						maxx = window.right;
					}

					int x = maxx - minx;
					int y = maxy - miny;
					if (x <= 0 || y <= 0)
					{
						__sleep(0);
						continue;
					}

					//__kRestoreMouse();

					unsigned char * pos = (unsigned char *)__getpos(minx, miny) + (DWORD)videoBase;

					unsigned char * p = pos;

					for (int i = 0; i < y; i++)
					{
						for (int j = 0; j < x; j++)
						{
							unsigned char * c = (unsigned char*)&pencilColor;
							for (int k = 0; k < gBytesPerPixel; k++)
							{
								*p = *c;
								p++;
								c++;
							}
						}

						pos += gBytesPerLine;
						p = pos;
					}

					//__kRefreshMouseBackup();
					//__kDrawMouse();
				}
			}
		}
		else if (mouseinfo.status & 2)
		{

		}
		__sleep(0);
	}
	return 0;
}


void initTableNew(WINDOWCLASS * window,int tablex,int tabley,int c1,int c2) {

	COLOR32 colortable;
	colortable.blue = 0;
	colortable.red = 0;
	colortable.green = 0;
	colortable.t = 0;

	char * color = (char*)&colortable;
	
	unsigned char* videoBase = (unsigned char*)GetVideoBase();
	unsigned char * pos = (unsigned char*)__getpos(tablex, tabley) + (DWORD)videoBase;

	unsigned char * p = pos;

	for (int i = 0; i < 256; i++)
	{
		for (int j = 0; j < 256; j++)
		{
			unsigned char *c = (unsigned char*)color;
			for (int k = 0; k < gBytesPerPixel; k++)
			{
				*p = *c;
				p++;
				c++;
			}

			color[c1]++;
		}
		color[c2]++;

		pos += gBytesPerLine;
		p = pos;
	}
}


void initTable(WINDOWCLASS * window,int factor) {
	DWORD initc = 0x00ffffff;

	unsigned int inc = initc / (window->width * (window->height >> factor));
	if (inc == 0)
	{
		inc = 1;
	}

	int tabley = window->top + window->height - (window->height >> factor);
	int tablex = window->left;

	char* vb = GetVideoBase();
	unsigned char * pos = (unsigned char*)__getpos(tablex, tabley) + (unsigned long)vb;

	unsigned char * p = pos;
	
	for (int i = 0; i < (window->height >> factor); i++)
	{
		for (int j = 0; j < window->width; j++)
		{
			unsigned char *c = (unsigned char*)&initc;
			for (int k = 0; k < gBytesPerPixel; k++)
			{
				*p = *c;
				p++;
				c++;
			}

			initc -= inc;
		}
		pos += gBytesPerLine;
		p = pos;
	}
}


DWORD getColor(DWORD blue, DWORD green, DWORD red) {
	return (red << 16) + (green << 8) + blue;
}


void putColor(DWORD c,LPCOLOR32 color) {
	color->blue = c & 0xff;
	color->green = (c >> 8) & 0xff;
	color->red = (c >> 16) & 0xff;
	return;
}