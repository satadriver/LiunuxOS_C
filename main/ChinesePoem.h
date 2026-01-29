#pragma once

#include "console.h"

#pragma pack(1)


#pragma pack()

int displayCCPoem();

extern "C" __declspec(dllexport) int __kChinesePoem(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);



