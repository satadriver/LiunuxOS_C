#pragma once

#include "def.h"
#include "ListEntry.h"
#include "malloc.h"


#define PAGE_PRESENT		1
#define PAGE_READWRITE		2
#define PAGE_USERPRIVILEGE	4
#define PAGE_ACCESSED		0X20
#define PAGE_DIRT			0X40

#define PAGE64_MASK1 0XFF8000000000
#define PAGE64_MASK2   0X7FC0000000
#define PAGE64_MASK3     0X3FE00000
#define PAGE64_MASK4       0X1FF000


void initPaging();

LPMEMALLOCINFO getFreePageIdx();

int resetPageIdx(LPMEMALLOCINFO pde);

int insertPageIdx(LPMEMALLOCINFO info, DWORD addr, int size, int pid, DWORD vaddr);

LPMEMALLOCINFO isPageIdxExist(DWORD addr, int size);

LPMEMALLOCINFO findPageIdx(DWORD addr);

void freeProcessPages(int pid);

void linearMapping();

void InitPage64();

#ifdef DLL_EXPORT

extern "C"  __declspec(dllexport) DWORD __kPageAlloc(int size);

extern "C"  __declspec(dllexport) int __kFreePage(DWORD buf);
#else
extern "C"  __declspec(dllimport) DWORD __kPageAlloc(int size);

extern "C"  __declspec(dllimport) int __kFreePage(DWORD buf);
#endif

