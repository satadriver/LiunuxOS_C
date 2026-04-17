#pragma once
#include "def.h"


int HeapAllocTest(int cnt,int size, unsigned long long* total);

int getldt(char * szout);

int getidt(char * szout);

int getgdt(char * szout);

int getcrs(char * szout);

int getGeneralRegs(char * szout);

DWORD InterruptPerSec();

int GetCpuRatio(char* szout);

int CpuUsage(char* buf);

int GetAllProcesses(char* szout);

int GetProcess(int cpuid,int pid, char* szout);

int getmemmap(int pid, char * szout);