#pragma once
#include "video.h"
#include "Utils.h"
#include "def.h"


#define CURSOR_REFRESH_MILLISECONDS		300


void setCursor( int*  x, int * y, unsigned int color);

int drawCursor(int p1, int p2, int p3, int p4);

int removeCursor();

int __clearChar(WINDOWCLASS * window);

extern "C" int __cmd(char * cmd, WINDOWCLASS*window,char * filename,int pid);

int __outputConsole(unsigned char * font, int color, WINDOWCLASS * window);
int __outputConsoleStr(unsigned char * font, int color, int bgcolor, WINDOWCLASS * window);


extern "C" __declspec(dllexport) int __kConsole(unsigned int retaddr, int tid, char * filename,char * funcname, DWORD param);
