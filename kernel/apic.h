#pragma once


#include "def.h"
#include "task.h"

#define APIC_HPET_BASE  0XFED00000


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



extern "C" int g_ApNumber;

int getLocalApicID();

int enableLocalApic();

void enableIoApic();

DWORD * getOicBase();

int enableHpet();



DWORD* getRcbaBase();

int initHpet();

void enableRcba();

void enableFerr();

void enableIMCR();

extern "C" void __declspec(dllexport) __kApInitProc();

void iomfence();

void setIoRedirect(int id, int idx, int vector, int mode);

void setIoApicID(int id);

extern "C" void __declspec(dllexport) IPIIntHandler(LIGHT_ENVIRONMENT * stack);

void BPCodeStart();

int AllocateAP(int vn);