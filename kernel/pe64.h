#pragma once
#include "def.h"

QWORD MemLoadDll64(char* filedata, char* addr);

QWORD getAddrFromName64(char* module, const char* funname);

QWORD getAddrFromOrd64(char* module, DWORD ord);

int relocTable64(char* pImageBase, ULONGLONG newBase);

