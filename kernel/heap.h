#pragma once


#include "def.h"

int CreateHeap();

int fast_heap_free(char* addr);

char* fast_heap_alloc(int allocSize);

DWORD __heapAlloc(int size);

DWORD __heapFree(DWORD addr);