#pragma once

#include "def.h"
#include "ListEntry.h"

#pragma pack(1)

typedef struct 
{
	LIST_ENTRY list;
	DWORD addr;
	DWORD size;
	DWORD pid;
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
	USHORT size;
	UCHAR flag;
	UCHAR reserved;
}MS_HEAP_STRUCT;



#pragma pack()

int setMemAllocInfo(LPMEMALLOCINFO item, DWORD addr, DWORD vaddr, int size, int pid);

void resetAllMemAllocInfo();

int resetMemAllocInfo(LPMEMALLOCINFO item);

LPMEMALLOCINFO getExistAddr_size(DWORD addr,int size);

LPMEMALLOCINFO getExistAddr_none_size(DWORD addr);

LPMEMALLOCINFO getMemAllocInfo();

int getAlignedSize(int size, int allignsize);


LPMEMALLOCINFO getExistAddr(DWORD addr, int size);

int initMemory();

DWORD pageAlignmentSize(DWORD size,int max);

DWORD __kProcessMalloc(DWORD s, DWORD *retsize, int pid, DWORD vaddr);

void freeProcessMemory(int pid);

#ifdef DLL_EXPORT

extern "C"  __declspec(dllexport) int getProcMemory(int pid, char * szout);
extern "C"  __declspec(dllexport) int __free(DWORD addr);
extern "C"  __declspec(dllexport) DWORD __malloc(DWORD s);

extern "C"  __declspec(dllexport) DWORD __kMalloc(DWORD size);

extern "C"  __declspec(dllexport) int __kFree(DWORD buf);
#else
extern "C"  __declspec(dllimport) int getProcMemory(int pid, char * szout);
extern "C"  __declspec(dllimport) int __free(DWORD addr);
extern "C"  __declspec(dllimport) DWORD __malloc(DWORD s);
extern "C"  __declspec(dllimport) DWORD __kMalloc(DWORD size);

extern "C"  __declspec(dllimport) int __kFree(DWORD buf);
#endif

