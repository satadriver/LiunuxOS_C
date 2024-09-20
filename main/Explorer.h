#pragma once


#include "def.h"
#include "video.h"


#pragma pack(1)
typedef struct {

	int x;			
	int y;		
	int width;
	int height;
}WINDOW_PARAM;

#pragma pack()

int TaskbarOnClick(WINDOWCLASS *window);

DWORD isDesktop(WINDOWCLASS *window);


extern "C" __declspec(dllexport) void __taskTest1(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);

extern "C" __declspec(dllexport) void __taskTest2(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);

extern "C" __declspec(dllexport) void __taskTest3(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);

extern "C" __declspec(dllexport) int __kExplorer(unsigned int retaddr, int tid, char * filename, char * funcname, DWORD param);