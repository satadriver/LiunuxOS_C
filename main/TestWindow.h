#pragma once


#include "def.h"

extern "C" __declspec(dllexport) void __MyTestTask(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);

extern "C" __declspec(dllexport) void __taskTest1(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);

extern "C" __declspec(dllexport) void __taskTest2(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);

extern "C" __declspec(dllexport) void __taskTest3(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);

extern "C" __declspec(dllexport)int __kTestWindow(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD runparam);