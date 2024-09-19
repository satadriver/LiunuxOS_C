#pragma once
#include "console.h"

#pragma pack(1)

typedef struct
{
	WINDOWCLASS window;

	int cpl;
	int fsheight;

}FMWINDOW, * LPFMWINDOW;




#pragma pack()

int restoreFileManager(LPFMWINDOW w);

int drawFileManager(LPFMWINDOW w);

int __restoreRectangleFrame(LPPOINT p, int width, int height, int framesize, unsigned char* backup);

extern "C" __declspec(dllexport) int __kShowWindow(unsigned int retaddr, int tid, char * filename,char *funcname, DWORD param);



