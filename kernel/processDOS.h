#pragma once

#include "def.h"
#include "process.h"
#include "task.h"

#pragma pack(1)

typedef struct  
{
	DWORD address;
	int size;
	DWORD status;
	DWORD pid;
	char name[64];
}DOS_PE_CONTROL,*LPDOS_PE_CONTROL;

#pragma pack()


#ifdef DLL_EXPORT
extern "C"  __declspec(dllexport) int __createDosCodeProc(DWORD addr, int size, char* filename);
#else
extern "C"  __declspec(dllimport) int __createDosCodeProc(DWORD addr, int size, char* filename);
#endif

void V86ProcessCheck(LIGHT_ENVIRONMENT* env, LPPROCESS_INFO prev, LPPROCESS_INFO proc);

int __initDosTss(LPPROCESS_INFO tss, int num, DWORD addr, char * filename, char * funcname, DWORD syslevel, DWORD runparam);

int getVm86ProcAddr(char * fn,int type, DWORD filedata, int size, int pid);

int relocDos(DWORD loadseg);

DWORD __allocVm86Addr(char * fn,int type, DWORD filedata, int filesize, int pid);