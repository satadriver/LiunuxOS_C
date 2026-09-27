#pragma once

#ifndef MALLOC_H_H_H
#define MALLOC_H_H_H

#include "def.h"
#include "ListEntry.h"
//#include "process.h"


//重复包含问题，该如何解决？


#pragma pack(1)

#ifdef LPMEMALLOCINFO
#error "LPMEMALLOCINFO already defined before malloc.h"
#endif

typedef struct
{
	LIST_ENTRY list;
	DWORD addr;
	DWORD size;
	DWORD vid;
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
	//WORD remainder;
}MS_HEAP_STRUCT;




#pragma pack()



QWORD getBorderAddr();

int SetMemAllocItem(LPMEMALLOCINFO item, DWORD addr, DWORD vaddr, int size,DWORD vid);

void ClearMemAllocMap();

int ClearMemAllocItem(LPMEMALLOCINFO item);

LPMEMALLOCINFO GetEmptyMemAllocItem();

int getAlignSize(int size, int allignsize);

LPMEMALLOCINFO isAddrExist(DWORD addr, int size);

LPMEMALLOCINFO findAddr(DWORD addr);

int initMemory();

DWORD pageAlignSize(DWORD size,int max);

DWORD __kProcessMalloc(DWORD s, DWORD *retsize, int pid,int cpuid, DWORD vaddr,int tag);

//void freeProcessMemory(LPPROCESS_INFO proc);

#ifdef DLL_EXPORT
extern "C"  __declspec(dllexport) LPMEMALLOCINFO gMemAllocList ;

extern "C"  __declspec(dllexport) QWORD gAvailableSize ;

extern "C"  __declspec(dllexport) QWORD gAvailableBase ;

extern "C"  __declspec(dllexport) QWORD gAllocLimitSize ;

extern "C"  __declspec(dllexport) int GetProcessMemory(int pid,int cpu, char * szout);
extern "C"  __declspec(dllexport) int __free(DWORD addr);
extern "C"  __declspec(dllexport) DWORD __malloc(DWORD s);

extern "C"  __declspec(dllexport) DWORD __kMalloc(DWORD size);

extern "C"  __declspec(dllexport) int __kFree(DWORD buf);
#else
extern "C"  __declspec(dllimport) LPMEMALLOCINFO gMemAllocList;
extern "C"  __declspec(dllimport) QWORD gAvailableSize;

extern "C"  __declspec(dllimport) QWORD gAvailableBase;

extern "C"  __declspec(dllimport) QWORD gAllocLimitSize;
extern "C"  __declspec(dllimport) int GetProcessMemory(int pid,int cpu, char * szout);
extern "C"  __declspec(dllimport) int __free(DWORD addr);
extern "C"  __declspec(dllimport) DWORD __malloc(DWORD s);
extern "C"  __declspec(dllimport) DWORD __kMalloc(DWORD size);

extern "C"  __declspec(dllimport) int __kFree(DWORD buf);
#endif

#endif