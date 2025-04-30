#pragma once

#include "def.h"

#define DOS_SYSTIMER_ADDR		0X46C




#ifdef DLL_EXPORT
extern "C" __declspec(dllexport) void __k8254TimerProc();

extern "C" __declspec(dllexport) int __kAdd8254Timer(DWORD addr, DWORD delay, DWORD param1, DWORD param2, DWORD param3, DWORD param4);

extern "C" __declspec(dllexport) void __kRemove8254Timer(int n);

extern "C" __declspec(dllexport) void init8254Timer();
#else
extern "C" __declspec(dllimport) void __k8254TimerProc();

extern "C" __declspec(dllimport) int __kAdd8254Timer(DWORD addr, DWORD delay, DWORD param1, DWORD param2, DWORD param3, DWORD param4);

extern "C" __declspec(dllimport) void __kRemove8254Timer(int n);

extern "C" __declspec(dllimport) void init8254Timer();
#endif


