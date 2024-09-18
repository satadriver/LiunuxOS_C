#pragma once
#include "console.h"

void initConsoleWindow(WINDOWCLASS * window, char * filename, int tid);

void initTaskbarWindow(WINDOWCLASS * window, char * filename, int tid);

void initDesktopWindow(WINDOWCLASS * window, char * filename, int tid);

void initBigClickItem(FILEMAP *clickitem, char * name, int tid,int id, int x, int y);
void initFullWindow(WINDOWCLASS * window, char * filename, int tid);
extern "C" __declspec(dllexport) int __kShowWindow(unsigned int retaddr, int tid, char * filename,char *funcname, DWORD param);



