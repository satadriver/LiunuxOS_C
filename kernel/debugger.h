#pragma once
#include "def.h"
#include "task.h"

#define DATA_BREAKPOINT 		 1
#define PORT_BREAKPOINT			 2
#define INSTRUCTION_BREAKPOINT 	 3
#define BD_BREAKPOINT 			 4
#define BS_BREAKPOINT 			 5
#define BT_BREAKPOINT 			 6
#define INT3_BREAKPOINT 		 7

void initDebugger();

void  BreakPointTrap(LIGHT_ENVIRONMENT* stack);

void  DebugTrap(LIGHT_ENVIRONMENT* stack);

#ifdef DLL_EXPORT

extern "C" __declspec(dllexport) int codeBreakPoint(unsigned int * addr, int len);
extern "C" __declspec(dllexport) int ioBreakPoint(unsigned int * addr, int len);
extern "C" __declspec(dllexport) int dataWBreakPoint(unsigned int * addr, int len);
extern "C" __declspec(dllexport) int dataRWBreakPoint(unsigned int * addr, int len);

extern "C" __declspec(dllexport) int enableSingleStep();
extern "C" __declspec(dllexport) int disableSingleStep();
extern "C" __declspec(dllexport) int enableGdDebugger();
extern "C" __declspec(dllexport) int disableGdDebugger();

extern "C" __declspec(dllexport) void __enableBreakPoint();

extern "C" __declspec(dllexport) void __kBreakPoint(LIGHT_ENVIRONMENT * stack);

extern "C" __declspec(dllexport) void __kDebugger(LIGHT_ENVIRONMENT * stack);
#else
extern "C" __declspec(dllimport) int codeBreakPoint(unsigned int * addr, int len);
extern "C" __declspec(dllimport) int ioBreakPoint(unsigned int * addr, int len);
extern "C" __declspec(dllimport) int dataWBreakPoint(unsigned int * addr, int len);
extern "C" __declspec(dllimport) int dataRWBreakPoint(unsigned int * addr, int len);

extern "C" __declspec(dllimport) int enableSingleStep();
extern "C" __declspec(dllimport) int disableSingleStep();
extern "C" __declspec(dllimport) int enableGdDebugger();
extern "C" __declspec(dllimport) int disableGdDebugger();

extern "C" __declspec(dllimport) void __enableBreakPoint();

extern "C" __declspec(dllimport) void __kBreakPoint(LIGHT_ENVIRONMENT * stack);

extern "C" __declspec(dllimport) void __kDebugger(LIGHT_ENVIRONMENT * stack);
#endif