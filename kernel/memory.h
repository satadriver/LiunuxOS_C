#pragma once

#include "def.h"


#define PAGE_TABLE_INDEX_NOTEXIST	0
#define PAGE_TABLE_NOTEXIST			-1


#ifdef DLL_EXPORT

extern "C" __declspec(dllexport) int clearCR3(DWORD * cr3);

extern "C" __declspec(dllexport) DWORD copyKernelCR3(DWORD phyaddr, DWORD size, DWORD *cr3);

extern "C" __declspec(dllexport) DWORD mapPhyToLinear(DWORD linearaddr, DWORD phyaddr, DWORD size, DWORD * cr3,int attr);

extern "C" __declspec(dllexport) DWORD linear2phy(DWORD linear);

extern "C" __declspec(dllexport) DWORD linear2phyByPid(DWORD linearAddr, int pid);

extern "C" __declspec(dllexport) DWORD getTbPgOff(DWORD phyaddr, DWORD * tboff, DWORD *pgoff);
#else
extern "C" __declspec(dllimport) int clearCR3(DWORD * cr3);

extern "C" __declspec(dllimport) DWORD copyKernelCR3(DWORD phyaddr, DWORD size, DWORD * cr3);

extern "C" __declspec(dllimport) DWORD mapPhyToLinear(DWORD linearaddr, DWORD phyaddr, DWORD size, DWORD * cr3, int attr);

extern "C" __declspec(dllimport) DWORD linear2phy(DWORD linear);

extern "C" __declspec(dllimport) DWORD linear2phyByPid(DWORD linearAddr, int pid);

extern "C" __declspec(dllimport) DWORD getTbPgOff(DWORD phyaddr, DWORD * tboff, DWORD * pgoff);

#endif
