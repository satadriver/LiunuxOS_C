#pragma once
#include "def.h"
#include "task.h"

#define KBD_OUTPUT			1
#define KBD_INPUT			2
#define MOUSE_OUTPUT		3
#define GRAPH_CHAR_OUTPUT	4
#define RANDOM				5
#define SLEEP				6
#define TURNON_SCREEN		7
#define TURNOFF_SCREEN		8
#define CPU_MANUFACTORY		9
#define TIMESTAMP			10
#define SWITCH_SCREEN		11
#define CPUINFO				12
#define DRAW_MOUSE			13
#define RESTORE_MOUSE		14
#define SET_VIDEOMODE		15



DWORD servicesProc(LIGHT_ENVIRONMENT* stack);

void sleep(DWORD* params);

DWORD __random(DWORD init);

void __turnoffScreen();

void __turnonScreen();

void __switchScreen();

DWORD __cputype(unsigned long* params);

DWORD __cpuinfo(unsigned long* params);

DWORD __timestamp(unsigned long* params);

extern "C"  __declspec(dllexport) DWORD __kServicesProc(DWORD no,DWORD * params);

//https://www.felixcloutier.com/x86/cpuid
unsigned __int64 __cpuFreq();

unsigned int getcpuFreq();

unsigned __int64 getCpuFreq();



extern "C" __declspec(dllexport) int __readTemperature(DWORD * temp);

#ifdef DLL_EXPORT
extern "C"  __declspec(dllexport) unsigned __int64 __krdtsc();
#else
extern "C"  __declspec(dllimport) unsigned __int64 __krdtsc();
#endif