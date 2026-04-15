#pragma once
#include "def.h"


// MSR寄存器地址定义
#define MSR_IA32_MPERF 0xE7   // 最大性能时钟计数
#define MSR_IA32_APERF 0xE8   // 实际性能时钟计数

// MSR 0x3F9 (MSR_PKG_C6_RESIDENCY)
#define MSR_PKG_C6_RESIDENCY 0x3F9  // 包级别C6状态驻留时间

// 其他相关MSR
#define MSR_PKG_C2_RESIDENCY 0x60D  // 包级别C2状态驻留时间
#define MSR_PKG_C3_RESIDENCY 0x3F8  // 包级别C3状态驻留时间
#define MSR_PKG_C7_RESIDENCY 0x3FA  // 包级别C7状态驻留时间
#define MSR_CORE_C3_RESIDENCY 0x3FC // 核心级别C3状态驻留时间
#define MSR_CORE_C6_RESIDENCY 0x3FD // 核心级别C6状态驻留时间


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


#define g_pm_enable_INVALID		1


int GetPmVersion();

int InitPm();

int GetCpuRate();

int IncreaseDelta(int v);

int AdjustApicTimer();

void EnableNXE();

#ifdef DLL_EXPORT
extern "C" __declspec(dllexport) int SysenterProc(char* params, int cnt);

extern "C" __declspec(dllexport) int SysenterEntry(char* params, int cnt);
extern "C" __declspec(dllexport) void __kCallGateProc(DWORD  params, DWORD size);
extern "C" __declspec(dllexport) void callgateEntry(char* params, DWORD size);

extern "C" __declspec(dllexport) void EnableSyscall();

extern "C" __declspec(dllexport)int SysenterInit(DWORD entryAddr);

extern "C" __declspec(dllexport) int g_pm_enable;

extern "C" __declspec(dllexport)void readmsr(DWORD no, DWORD* lowpart, DWORD* highpart);

extern "C" __declspec(dllexport)void writemsr(DWORD no, DWORD lowpart, DWORD highpart);

#else
extern "C" __declspec(dllimport) int SysenterProc(char* params, int cnt);

extern "C" __declspec(dllimport) int SysenterEntry(char* params, int cnt);
extern "C" __declspec(dllimport) void __kCallGateProc(DWORD  params, DWORD size);
extern "C" __declspec(dllimport) void callgateEntry(char* params, DWORD size);

extern "C" __declspec(dllimport) void EnableSyscall();

extern "C" __declspec(dllimport) void EnableSyscall();

extern "C" __declspec(dllimport) int g_pm_enable;

extern "C" __declspec(dllimport)void readmsr(DWORD no, DWORD* lowpart, DWORD* highpart);

extern "C" __declspec(dllimport)void writemsr(DWORD no, DWORD lowpart, DWORD highpart);

#endif



