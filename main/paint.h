#pragma once
#include "def.h"
#include "video.h"

#pragma pack(1)

typedef struct  
{
	unsigned char blue;
	unsigned char green;
	unsigned char red;
	unsigned char t;
}COLOR32,*LPCOLOR32;

#pragma pack()

void putColor(DWORD c, LPCOLOR32 color);
DWORD getColor(DWORD blue, DWORD green, DWORD red);

void initTableNew(WINDOWCLASS * window, int x,int y,int c1,int c2);

void initTable(WINDOWCLASS * window, int factor);
extern "C" __declspec(dllexport) int __kPaint(unsigned int retaddr, int pid, char * filename, char * funcname, DWORD runparam);