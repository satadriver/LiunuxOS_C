#pragma once
#include "def.h"

#define REALTIMER_CALLBACK_MAX	64

#define CMOS_EXACT_INTERVAL		15		//15.625 ms

#pragma pack(1)

typedef struct
{
	DWORD func;
	DWORD ticks;
	DWORD tickcnt;
	DWORD param1;
	DWORD param2;
	DWORD param3;
	DWORD param4;
	int pid;
	int tid;
}TIMER_PROC_PARAM;

#pragma pack()



#ifdef DLL_EXPORT
extern "C" __declspec(dllexport) void __kExactTimerProc();
extern "C" __declspec(dllexport) int __kAddExactTimer(DWORD addr, DWORD delay, DWORD param1, DWORD param2, DWORD param3, DWORD param4);
extern "C" __declspec(dllexport) void __kRemoveExactTimer(int no);
#else
extern "C" __declspec(dllimport) void __kExactTimerProc();
extern "C" __declspec(dllimport) int __kAddExactTimer(DWORD addr, DWORD delay, DWORD param1, DWORD param2, DWORD param3, DWORD param4);
extern "C" __declspec(dllimport) void __kRemoveExactTimer(int no);
#endif




void initExactTimer();