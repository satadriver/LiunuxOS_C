#pragma once

#define u8 unsigned char
#define u16 unsigned short
#define u32 unsigned int
#define u64 unsigned __int64

#include "Utils.h"

#pragma pack(1)

typedef struct  {
	u32 signature;
	u32 length;
	u8 version;
	u8 checksum;
	u8 oem[6];
	u8 oemTableID[8];
	u32 oemVersion;
	u32 creatorID;
	u32 creatorVersion;
	//u32 data[0];
} ACPIHeader;

typedef struct  {
	ACPIHeader header;
	u32 localApicAddress;
	u32 flags;
	u8 data[0];
}  ACPIHeaderApic;

typedef struct  {
	u8 type;
	u8 length;
}  ApicHeader;

typedef struct  {
	ApicHeader header;
	u8 apicProcessorID;
	u8 apicID;
	u32 flags;
}  LocalApic;

typedef struct  {
	ApicHeader header;
	u8 ioApicID;
	u8 reserved;
	u32 ioApicAddress;
	u32 globalSystemInterruptBase;
}  IOApic;

#pragma pack()

#define APIC_TYPE_LOCAL_APIC         0x0
#define APIC_TYPE_IO_APIC            0x1
#define APIC_TYPE_INTERRUPT_OVERRIDE 0x2

void initHPET();

extern "C"  __declspec(dllexport) DWORD getRCBA();