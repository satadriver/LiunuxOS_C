#pragma once

#include "def.h"
#include "process.h"

void enableSSE();

void enableAVX();

void InitTaskCoprocessor(LPPROCESS_INFO tss, char* fenv);

char* GetFpuBuf(int cpu, int tid);

void initCoprocessor();

#ifdef DLL_EXPORT
extern "C" __declspec(dllexport) char g_fpu_init_buf[512];

extern "C" __declspec(dllexport) char* g_fpu_status[MAX_CPU_COUNT];

extern "C" __declspec(dllexport) int isSSE();
extern "C" __declspec(dllexport) void __kCoprocessor();
#else
extern "C" __declspec(dllexport) char g_fpu_init_buf[512];
extern "C" __declspec(dllimport) char* g_fpu_status[MAX_CPU_COUNT];
extern "C" __declspec(dllimport) int isSSE();
extern "C" __declspec(dllimport) void __kCoprocessor();
#endif