#pragma once

#include "process.h"
#include "task.h"
#include "def.h"

#ifdef DLL_EXPORT
extern "C" __declspec(dllexport) DWORD __kTerminateThread(int tid, char * filename, char * funcname, DWORD lpparams);
extern "C" __declspec(dllexport) DWORD __kCreateThread(DWORD addr,DWORD module, DWORD runparam, char * funcname);
#else
extern "C" __declspec(dllexport) DWORD __kTerminateThread(int tid, char * filename, char * funcname, DWORD lpparams);
extern "C" __declspec(dllimport) DWORD __kCreateThread(DWORD addr, DWORD module, DWORD runparam, char * funcname);
#endif



