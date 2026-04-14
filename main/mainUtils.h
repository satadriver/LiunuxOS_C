#pragma once
#include "def.h"


extern "C"  __declspec(dllexport) void __kSysRegs();

int getldt(char * szout);

int getidt(char * szout);

int getgdt(char * szout);

int getcrs(char * szout);

int getGeneralRegs(char * szout);

DWORD InterruptPerSec();

int CpuUsage(char* buf);

int GetAllProcesses(char* szout);

int GetProcess(int cpuid,int pid, char* szout);

int getmemmap(int pid, char * szout);