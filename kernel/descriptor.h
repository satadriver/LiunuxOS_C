#pragma once
#include "def.h"





#pragma pack(1)

typedef struct {
	WORD size;
	DWORD addr;
}DESCRIPTOR_REG;



typedef struct  
{
	unsigned char jmpcode;		//0xea
	unsigned short seg;
	unsigned long offset;
}JUMP_LONG;

typedef struct
{
	unsigned char callcode;		//0x9a
	unsigned short seg;
	unsigned long offset;
}CALL_LONG;

typedef struct
{
	unsigned char jmpcode;
	unsigned short seg;
	unsigned short offset;
}JUMP_SHORT;



#pragma pack()

extern "C" __declspec(dllexport) int sysEntryProc();

extern "C" __declspec(dllexport) int sysEntry();

int sysEntryInit(DWORD entryAddr);

void readmsr(DWORD no, DWORD* lowpart, DWORD* highpart);

void writemsr(DWORD no, DWORD lowpart, DWORD highpart);


extern "C" void __kCallGateProc(DWORD  params, DWORD size);

extern "C" __declspec(dllexport) void callgateEntry(DWORD  params, DWORD size);


#ifdef DLL_EXPORT
extern "C" __declspec(dllexport) void initEfer();

#else
extern "C" __declspec(dllimport) void initEfer();

#endif



