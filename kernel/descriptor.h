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

extern "C" __declspec(dllexport) int SysenterProc(char* params, int cnt);

extern "C" __declspec(dllexport) int SysenterEntry(char * params,int cnt);

int SysenterInit(DWORD entryAddr);

void readmsr(DWORD no, DWORD* lowpart, DWORD* highpart);

void writemsr(DWORD no, DWORD lowpart, DWORD highpart);


extern "C" void __kCallGateProc(DWORD  params, DWORD size);

extern "C" __declspec(dllexport) void callgateEntry(char*  params, DWORD size);

void EnableNXE();

#ifdef DLL_EXPORT
extern "C" __declspec(dllexport) void EnableSyscall();

#else
extern "C" __declspec(dllimport) void EnableSyscall();

#endif



