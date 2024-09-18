#pragma once

#include "Utils.h"

#define u8 unsigned char
#define u16 unsigned short
#define u32 unsigned int
#define u64 unsigned __int64



#pragma pack(1)


typedef struct {
	char signature[8];
	unsigned char checksum;
	char oemid[6];
	unsigned char revision;
	DWORD rsdt;
}RSDT_HEADER;


typedef struct {
	char Signature[8];
	uint8_t Checksum;
	char OEMID[6];
	uint8_t Revision;
	uint32_t RsdtAddress;      // deprecated since version 2.0

	uint32_t Length;
	uint64_t XsdtAddress;
	uint8_t ExtendedChecksum;
	uint8_t reserved[3];
} XSDP_HEADER;

typedef struct {
	char Signature[4];
	uint32_t Length;
	uint8_t Revision;
	uint8_t Checksum;
	char OEMID[6];
	char OEMTableID[8];
	uint32_t OEMRevision;
	uint32_t CreatorID;
	uint32_t CreatorRevision;
}ACPISDTHeader;

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
	//u8 data[0];
}  ACPIHeaderApic;

typedef struct  {
	u8 addressSpaceID;
	u8 registerBitWidth;
	u8 registerBitOffset;
	u8 reserved;
	u64 address;
}  ACPIAddressFormat;

typedef struct  {
	ACPIHeader header;
	u32 firmwareControl;
	u32 dsdt;
	u8 reserved;
	u8 preferredPMProfile;
	u16 sciInterrupt;
	u32 smiCommandPort;
	u8 acpiEnable;
	u8 acpiDisable;
	u8 unused1[56 - 54];
	u32 eventRegister1a;
	u32 eventRegister1b;
	u32 controlRegister1a; /*PM1a_CNT_BLK*/
						   /*System port address of the PM1a Control Register Block.*/
	u32 controlRegister1b; /*PM1b_CNT_BLK*/
						   /*System port address of the PM1b Control Register Block.*/
	u8 unused2[88 - 72];
	u8 eventRegister1Length;
	u8 unused3[116 - 89];
	ACPIAddressFormat resetRegister;
	u8 resetValue;   /*Indicates the value to write to */
					 /*the RESET_REG port to reset the system.*/
	u8 unused4[268 - 129];

} ACPIFadt;


typedef struct {
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



#define APIC_TYPE_LOCAL_APIC			0x0
#define APIC_TYPE_IO_APIC				0x1
#define APIC_TYPE_INTERRUPT_OVERRIDE	0x2
#define SCI_ENABLED						0x1

//https://blog.csdn.net/miss_lazygoat/article/details/48161645?spm=1001.2101.3001.6650.2&utm_medium=distribute.pc_relevant.none-task-blog-2%7Edefault%7EBlogCommendFromBaidu%7ERate-2-48161645-blog-18353301.235%5Ev43%5Epc_blog_bottom_relevance_base9&depth_1-utm_source=distribute.pc_relevant.none-task-blog-2%7Edefault%7EBlogCommendFromBaidu%7ERate-2-48161645-blog-18353301.235%5Ev43%5Epc_blog_bottom_relevance_base9&utm_relevant_index=5


void heptEOI();

void initHPET();

extern "C"  __declspec(dllexport) int doReboot(void);

extern "C"  __declspec(dllexport) int doPowerOff(void);

extern "C"  __declspec(dllexport) DWORD getRCBA();

extern "C"  __declspec(dllexport) DWORD enableFloatIRQ();

int initACPI(void);