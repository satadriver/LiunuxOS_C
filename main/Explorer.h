#pragma once


#include "def.h"


#pragma pack(1)
typedef struct {

	int x;			
	int y;		
	int width;
	int height;
}WINDOW_PARAM;

#pragma pack()


extern "C" __declspec(dllexport) void __taskTest1(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);

extern "C" __declspec(dllexport) void __taskTest2(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);

extern "C" __declspec(dllexport) int __kExplorer(unsigned int retaddr, int tid, char * filename, char * funcname, DWORD param);