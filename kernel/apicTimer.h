#pragma once

#include "def.h"

#define DOS_SYSTIMER_ADDR		0X46C




#ifdef DLL_EXPORT

extern "C" __declspec(dllexport)int __k8254TimerProc();
extern "C" __declspec(dllexport) void __kApicTimerProc();

extern "C" __declspec(dllexport) int __kAddApicTimer(DWORD addr, DWORD delay, DWORD param1, DWORD param2, DWORD param3, DWORD param4);

extern "C" __declspec(dllexport) void __kRemoveApicTimer(int n);

extern "C" __declspec(dllexport) void initApicTimer();
#else
extern "C" __declspec(dllimport)int __k8254TimerProc();
extern "C" __declspec(dllimport) void __kApicTimerProc();

extern "C" __declspec(dllimport) int __kAddApicTimer(DWORD addr, DWORD delay, DWORD param1, DWORD param2, DWORD param3, DWORD param4);

extern "C" __declspec(dllimport) void __kRemoveApicTimer(int n);

extern "C" __declspec(dllimport) void initApicTimer();
#endif


