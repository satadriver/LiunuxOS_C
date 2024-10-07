#pragma once

#include "descriptor.h"
#include "def.h"
#include "video.h"





#ifdef DLL_EXPORT

extern "C" __declspec(dllexport)  DWORD gVideoMode;

extern "C" __declspec(dllexport)  DWORD gV86VMIEntry;
extern "C" __declspec(dllexport)  DWORD gV86VMISize;
extern "C" __declspec(dllexport)  DWORD gV86IntProc;
extern "C" __declspec(dllexport)  DWORD gKernel16;
extern "C" __declspec(dllexport)  DWORD gKernel32;
extern "C" __declspec(dllexport)  DWORD gKernelData;



extern "C" __declspec(dllexport) void __kKernelMain(DWORD retaddr, int pid, char * pname,char * funcname,DWORD param);


extern "C" __declspec(dllexport) int __kernelEntry(LPVESAINFORMATION vesa, DWORD fontbase, DWORD v86ProcessBase, int v86ProcessLen,
	DWORD v86IntBase, DWORD kerneldata, DWORD kernel16, DWORD kernel32);
#else
extern "C" __declspec(dllimport)  DWORD gVideoMode;
extern "C" __declspec(dllimport)  DWORD gV86VMIEntry;
extern "C" __declspec(dllimport)  DWORD gV86VMISize;
extern "C" __declspec(dllimport)  DWORD gV86IntProc;
extern "C" __declspec(dllimport)  DWORD gKernel16;
extern "C" __declspec(dllimport)  DWORD gKernel32;
extern "C" __declspec(dllimport)  DWORD gKernelData;

extern "C" __declspec(dllimport) void __kKernelMain(DWORD retaddr, int pid, char * pname, char * funcname, DWORD param);

extern "C" __declspec(dllimport) int __kernelEntry(LPVESAINFORMATION vesa, DWORD fontbase, DWORD v86ProcessBase, int v86ProcessLen,
	DWORD v86IntBase, DWORD kerneldata, DWORD kernel16, DWORD kernel32);

#endif






