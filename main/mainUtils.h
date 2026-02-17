#pragma once
#include "def.h"


extern "C"  __declspec(dllexport) void __kSysRegs();

int getldt(char * szout);

int getidt(char * szout);

int getgdt(char * szout);

int getcrs(char * szout);

int getGeneralRegs(char * szout);

int getpid(int pid, char * szout);

int CpuUsage(char* buf);

int getpids(char * szout);

int getmemmap(int pid, char * szout);