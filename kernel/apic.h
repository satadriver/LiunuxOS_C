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
	unsigned char width : 1;
	unsigned char reserved : 1;
	unsigned char compatable : 1;
	unsigned short venderid;
	DWORD tick;
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

void EnableFloatError();

void enableIMCR();

void iomfence();

void setIoRedirect(int id, int idx, int vector, int mode);

void setIoApicID(int id);

extern "C" void __declspec(dllexport) IPIIntHandler(LIGHT_ENVIRONMENT * stack);

void BPCodeStart();

extern "C" void __declspec(dllexport) __kApInitProc();

int AllocateApTask(int vn);

#ifdef DLL_EXPORT

extern "C" __declspec(dllexport)  LPPROCESS_INFO GetCurrentTaskTssBase();
#else
extern "C"  __declspec(dllimport)  LPPROCESS_INFO GetCurrentTaskTssBase();
#endif