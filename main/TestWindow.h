#pragma once


#include "def.h"

extern "C" __declspec(dllexport) int TestThread3_main(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);
extern "C" __declspec(dllexport) int TestThread2_main(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);

extern "C" __declspec(dllexport) int TestThread1_main(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);

extern "C" __declspec(dllexport) void __MyTestTask(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);

extern "C" __declspec(dllexport)int __kTestWindow(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD runparam);