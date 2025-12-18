#pragma once

#include "def.h"
#include "ListEntry.h"


extern QWORD gAvailableSize;

extern QWORD gAvailableBase;

#pragma pack(1)

typedef struct 
{
	LIST_ENTRY list;
	DWORD addr;
	DWORD size;
	DWORD pid;
	int cpu;
	DWORD vaddr;
}MEMALLOCINFO,*LPMEMALLOCINFO;

typedef struct
{
	unsigned int BaseAddrLow;
	unsigned int BaseAddrHigh;
	unsigned int LengthLow;
	unsigned int LengthHigh;
	unsigned int Type;
}ADDRESS_RANGE_DESCRIPTOR_STRUCTURE;


typedef struct  
{
	DWORD addr;
	DWORD size;

}MS_HEAP_STRUCT;



#pragma pack()

DWORD getBorderAddr();

int SetMemAllocItem(LPMEMALLOCINFO item, DWORD addr, DWORD vaddr, int size, int pid,int cpu);

void ClearMemAllocMap();

int ClearMemAllocItem(LPMEMALLOCINFO item);

LPMEMALLOCINFO GetEmptyMemAllocItem();

int getAlignSize(int size, int allignsize);

LPMEMALLOCINFO isAddrExist(DWORD addr, int size);

LPMEMALLOCINFO findAddr(DWORD addr);

int initMemory();

DWORD pageAlignSize(DWORD size,int max);

DWORD __kProcessMalloc(DWORD s, DWORD *retsize, int pid,int cpu, DWORD vaddr,int tag);

void freeProcessMemory(int pid,int cpu);

#ifdef DLL_EXPORT

extern "C"  __declspec(dllexport) int getProcMemory(int pid,int cpu, char * szout);
extern "C"  __declspec(dllexport) int __free(DWORD addr);
extern "C"  __declspec(dllexport) DWORD __malloc(DWORD s);

extern "C"  __declspec(dllexport) DWORD __kMalloc(DWORD size);

extern "C"  __declspec(dllexport) int __kFree(DWORD buf);
#else
extern "C"  __declspec(dllimport) int getProcMemory(int pid,int cpu, char * szout);
extern "C"  __declspec(dllimport) int __free(DWORD addr);
extern "C"  __declspec(dllimport) DWORD __malloc(DWORD s);
extern "C"  __declspec(dllimport) DWORD __kMalloc(DWORD size);

extern "C"  __declspec(dllimport) int __kFree(DWORD buf);
#endif

