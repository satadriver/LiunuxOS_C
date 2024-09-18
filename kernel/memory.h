#pragma once

#include "def.h"


#define PAGE_TABLE_INDEX_NOTEXIST	0
#define PAGE_TABLE_NOTEXIST			-1



int clearCr3(DWORD *cr3);

DWORD copyPdeTables(DWORD phyaddr, DWORD size, DWORD *tables);

DWORD mapPhyToLinear(DWORD linearaddr, DWORD phyaddr, DWORD size, DWORD * cr3);

DWORD linear2phy(DWORD linear);

DWORD linear2phy(DWORD linearAddr, int pid);

DWORD getTbPgOff(DWORD phyaddr, DWORD * tboff, DWORD *pgoff);
