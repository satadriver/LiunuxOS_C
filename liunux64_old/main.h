
#pragma once

#include "def.h"


#ifdef DLL_EXPORT

extern "C" __declspec(dllexport)  DWORD gVideoMode;

extern "C" __declspec(dllexport)  DWORD gV86VMIEntry;
extern "C" __declspec(dllexport)  DWORD gV86VMISize;
extern "C" __declspec(dllexport)  DWORD gV86IntProc;
extern "C" __declspec(dllexport)  DWORD gKernel16;
extern "C" __declspec(dllexport)  DWORD gKernel32;
extern "C" __declspec(dllexport)  DWORD gKernelData;


#else
extern "C" __declspec(dllimport)  DWORD gVideoMode;
extern "C" __declspec(dllimport)  DWORD gV86VMIEntry;
extern "C" __declspec(dllimport)  DWORD gV86VMISize;
extern "C" __declspec(dllimport)  DWORD gV86IntProc;
extern "C" __declspec(dllimport)  DWORD gKernel16;
extern "C" __declspec(dllimport)  DWORD gKernel32;
extern "C" __declspec(dllimport)  DWORD gKernelData;



#endif








extern "C" __declspec(dllexport) int __kKernelEntry64();