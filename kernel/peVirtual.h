#pragma once

#include "def.h"
#include "Pe.h"

DWORD memLoadDllV(char* filedata, int size, char* addr, DWORD vaddr, DWORD * cr3);

int setImageBaseV(char* chBaseAddress,DWORD v);

bool relocTableV(char* chBaseAddress,DWORD v);

DWORD getAddrFromNameV(DWORD module, const char * funname,DWORD v);

DWORD getAddrFromOrdV(DWORD module, DWORD ord,DWORD v);





