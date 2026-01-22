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
#define GIVEUP_LIFE			16
#define SRV_IPI_CREATEPROC		17
#define SRV_IPI_CREATETHREAD	18


#ifdef DLL_EXPORT

#else

#endif








#ifdef DLL_EXPORT
extern "C" __declspec(dllexport) int __readTemperature(DWORD * temp);
extern "C" __declspec(dllexport) int __kVm86IntProc();
extern "C"  __declspec(dllexport) unsigned __int64 __krdtsc();
extern "C" __declspec(dllexport) DWORD vm86IntProc(LIGHT_ENVIRONMENT* stack);

extern "C" __declspec(dllexport) DWORD ServiceEntry(LIGHT_ENVIRONMENT* stack);

extern "C" __declspec(dllexport) void sleep(DWORD* params);

extern "C" __declspec(dllexport) DWORD __random(DWORD init);

extern "C" __declspec(dllexport) void __turnoffScreen();

extern "C" __declspec(dllexport) void __turnonScreen();

extern "C" __declspec(dllexport) void __switchScreen();

extern "C" __declspec(dllexport) DWORD __cputype(unsigned long* params);

extern "C" __declspec(dllexport) DWORD __cpuinfo(unsigned long* params);

extern "C" __declspec(dllexport) DWORD __timestamp(unsigned long* params);

extern "C"  __declspec(dllexport) DWORD __kServicesProc(DWORD no, DWORD * params, LIGHT_ENVIRONMENT * stack);



//https://www.felixcloutier.com/x86/cpuid
extern "C" __declspec(dllexport) unsigned __int64 __cpuFreq(DWORD* cpu, DWORD* max, DWORD* bus);

extern "C" __declspec(dllexport) unsigned int getcpuFreq();

extern "C" __declspec(dllexport) unsigned __int64 getCpuFreq();

extern "C"  __declspec(dllexport) void __ipiCreateProcess(DWORD base, int size, char* module, char* func, int level, unsigned long p);

extern "C"  __declspec(dllexport)void __ipiCreateThread(DWORD addr, char* module, unsigned long p, char* func);
#else
extern "C"  __declspec(dllimport) unsigned __int64 __krdtsc();
extern "C" __declspec(dllimport) int __readTemperature(DWORD * temp);
extern "C" __declspec(dllimport) int __kVm86IntProc();
extern "C"  __declspec(dllimport) unsigned __int64 __krdtsc();
extern "C" __declspec(dllimport) DWORD vm86IntProc(LIGHT_ENVIRONMENT * stack);

extern "C" __declspec(dllimport) DWORD ServiceEntry(LIGHT_ENVIRONMENT * stack);

extern "C" __declspec(dllimport) void sleep(DWORD * params);

extern "C" __declspec(dllimport) DWORD __random(DWORD init);

extern "C" __declspec(dllimport) void __turnoffScreen();

extern "C" __declspec(dllimport) void __turnonScreen();

extern "C" __declspec(dllimport) void __switchScreen();

extern "C" __declspec(dllimport) DWORD __cputype(unsigned long* params);

extern "C" __declspec(dllimport) DWORD __cpuinfo(unsigned long* params);

extern "C" __declspec(dllimport) DWORD __timestamp(unsigned long* params);

extern "C"  __declspec(dllimport) DWORD __kServicesProc(DWORD no, DWORD * params, LIGHT_ENVIRONMENT * stack);

//https://www.felixcloutier.com/x86/cpuid
extern "C" __declspec(dllimport) unsigned __int64 __cpuFreq(DWORD * cpu, DWORD * max, DWORD * bus);

extern "C" __declspec(dllimport) unsigned int getcpuFreq();

extern "C" __declspec(dllimport) unsigned __int64 getCpuFreq();

extern "C"  __declspec(dllimport) int __ipiCreateProcess(DWORD base, int size, char* module, char* func, int level, unsigned long p);

extern "C"  __declspec(dllimport)void __ipiCreateThread(DWORD addr, char* module, unsigned long p, char* func);
#endif