#pragma once


#include "def.h"
#include "video.h"

#include "cmd.h"






int isTxtFile(char * filename, int fnlen);

int getPrevPath(char * path);



int beEndWith(char * str, char * flag);


void initConsoleWindow(WINDOWCLASS* window, char* filename, int tid);

void initTaskbarWindow(WINDOWCLASS* window, char* filename, int tid);

void initDesktopWindow(WINDOWCLASS* window, char* filename, int tid);

void initBigClickItem(FILEMAP* clickitem, char* name, int tid, int id, int x, int y);

void initFullWindow(WINDOWCLASS* window, char* filename, int tid);

extern "C" __declspec(dllexport) int __kTaskBar(unsigned int retaddr, int pid, char* filename, char* funcname, DWORD param);