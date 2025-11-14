#pragma once


#include "def.h"
#include "task.h"
#include "process.h"

#define APIC_HPET_BASE			0XFED00000

#define IO_APIC_BASE			0xFEC00000

#define LOCAL_APIC_BASE			0xfee00000

#define APIC_CORE_MAX_COUNT		256


#pragma pack(1)


typedef struct {
	unsigned char version;
	unsigned char count : 5;
	unsigned char width : 1;		//1:64bit,0:32bit
	unsigned char reserved : 1;
	unsigned char compatable : 1;
	unsigned short venderid;
	DWORD tick;			//0x0429b17f
}HPET_GCAP_ID_REG;

#pragma pack()


//1. lfence sfence mfence
// 在读写指令前插入指令后，可以让高速缓存中的数据失效，重新从主内存加载数据、能让写入缓存的最新数据写回到主内存



unsigned long ReadIoApicReg(int reg);

unsigned long WriteIoApicReg(int reg, unsigned long value);

int getLocalApicID();

int enableLocalApic();

void enableIoApic();

DWORD * getOicBase();

int enableHpet();

int IsApicSupported();

char* getRcbaBase();

int initHpet();

void enableRcba();

void EnableInt13();

void enableIMCR();

void iomfence();

void setIoRedirect(int id, int idx, int vector, int mode);

void setIoApicID(int id);



void BPCodeStart();

extern "C" void __declspec(dllexport) __kApInitProc();

int AllocateApTask(int vn);


extern "C" void __declspec(dllexport) HpetTimer0Handler(LIGHT_ENVIRONMENT* stack);

extern "C" void __declspec(dllexport) IPIIntHandler(LIGHT_ENVIRONMENT* stack);
extern "C" void __declspec(dllexport) LVTTimerIntHandler(LIGHT_ENVIRONMENT* stack);
extern "C" void __declspec(dllexport) LVTTemperatureIntHandler(LIGHT_ENVIRONMENT* stack);
extern "C" void __declspec(dllexport) LVTErrorIntHandler(LIGHT_ENVIRONMENT* stack);
extern "C" void __declspec(dllexport) LVTPerformanceIntHandler(LIGHT_ENVIRONMENT* stack);
extern "C" void __declspec(dllexport) LVTLint1Handler(LIGHT_ENVIRONMENT* stack);

extern "C" void __declspec(dllexport) LVTLint0Handler(LIGHT_ENVIRONMENT* stack);
extern "C" void __declspec(dllexport) LVTCMCIHandler(LIGHT_ENVIRONMENT* stack);

#ifdef DLL_EXPORT

extern "C" __declspec(dllexport)  LPPROCESS_INFO GetCurrentTaskTssBase();
#else
extern "C"  __declspec(dllimport)  LPPROCESS_INFO GetCurrentTaskTssBase();
#endif