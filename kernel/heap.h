#pragma once


#include "def.h"

int CreateHeap();

int fast_heap_free_large(char* addr);

char* fast_heap_alloc_large(int allocSize);

int fast_heap_free(char* addr);

char* fast_heap_alloc(int allocSize);

DWORD __heapAlloc(char* heapBase, int heapTop, int size);

DWORD __heapFree(char* heapBase, int heapLimit, DWORD addr);