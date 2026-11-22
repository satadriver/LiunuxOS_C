#pragma once

#include "Utils.h"

#define u8 unsigned char
#define u16 unsigned short
#define u32 unsigned int
#define u64 unsigned __int64

//http://rweverything.com/







#pragma pack(1)

/* ---------- ACPI 表头结构 ---------- */
typedef struct {
	char     signature[4];
	uint32_t length;
	uint8_t  revision;
	uint8_t  checksum;
	char     oem_id[6];
	char     oem_table_id[8];
	uint32_t oem_revision;
	uint32_t creator_id;
	uint32_t creator_revision;
} acpi_sdt_header_t;

typedef struct {
	u32 signature;
	u32 length;
	u8 version;		//8
	u8 checksum;
	u8 oem[6];
	u8 oemTableID[8];	//16
	u32 oemVersion;		//24
	u32 creatorID;		//28
	u32 creatorVersion;	//32
	//u32 data[0];
} ACPIHeader;

typedef struct {
	ACPIHeader header;
	u32 localApicAddress;
	u32 flags;
	//u8 data[0];
}  ACPIHeaderApic;

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

/* ---------- RSDP 结构（兼容 ACPI 1.0 与 2.0+） ---------- */
typedef struct {
	char     signature[8];      /* "RSD PTR " */
	uint8_t  checksum;
	char     oem_id[6];
	uint8_t  revision;
	uint32_t rsdt_address;      /* 32 位物理地址 */
								/* 以下字段仅在 revision >= 2 时有效 */
	uint32_t length;
	uint64_t xsdt_address;      /* 64 位物理地址 */
	uint8_t  extended_checksum;
	uint8_t  reserved[3];
} acpi_rsdp_t;

/* ---------- MADT 子表头 ---------- */
typedef struct {
	uint8_t type;
	uint8_t length;
} madt_subtable_header_t;

typedef struct {
	madt_subtable_header_t header;
	u8 apicProcessorID;
	u8 apicID;
	u32 flags;
}  LocalApic;


typedef struct {
	madt_subtable_header_t header;
	u8 ioApicID;
	u8 reserved;
	u32 ioApicAddress;
	u32 globalSystemInterruptBase;
	u32 apciid;
}  IOApic;


/* ---------- Processor Local APIC 条目（Type 0） ---------- */
typedef struct {
	madt_subtable_header_t header;  /* type = 0, length = 8 */
	uint8_t  acpi_processor_id;
	uint8_t  apic_id;
	uint32_t flags;                 /* bit 0: Enabled */
}madt_local_apic_t;


typedef struct {
	ACPIHeader Header;
	UINT32                      Entry;
} RSDT_TABLE;

typedef struct {
	ACPIHeader Header;
	UINT64                      Entry;
} XSDT_TABLE;



typedef struct  {
	u8 addressSpaceID;
	u8 registerBitWidth;
	u8 registerBitOffset;
	u8 reserved;
	u64 address;
}  ACPIAddressFormat;


typedef struct  {
	ACPIHeader header;
	u32 firmwareControl;		//36
	u32 dsdt;		//40
	u8 reserved;	//44
	u8 preferredPMProfile;	//45
	u16 sciInterrupt;		//46
	u32 smiCommandPort;	//48
	u8 acpiEnable;		//52
	u8 acpiDisable;		//53
	u8 unused1[56 - 54];	//54
	u32 eventRegister1a;	//56
	u32 eventRegister1b;	//60
	u32 controlRegister1a; /*PM1a_CNT_BLK*/
						   /*System port address of the PM1a Control Register Block.*/
	u32 controlRegister1b; /*PM1b_CNT_BLK*/
						   /*System port address of the PM1b Control Register Block.*/
	u8 unused2[88 - 72];	//72
	u8 eventRegister1Length;	//88
	u8 unused3[116 - 89];		//89
	ACPIAddressFormat resetRegister;	//116
	u8 resetValue;   /*Indicates the value to write to */		//128
					 /*the RESET_REG port to reset the system.*/
	u8 unused4[268 - 129];	//129
} ACPIFadt;








#pragma pack()



#define APIC_TYPE_LOCAL_APIC			0x0
#define APIC_TYPE_IO_APIC				0x1
#define APIC_TYPE_INTERRUPT_OVERRIDE	0x2
#define SCI_ENABLED						0x1

//https://blog.csdn.net/miss_lazygoat/article/details/48161645?spm=1001.2101.3001.6650.2&utm_medium=distribute.pc_relevant.none-task-blog-2%7Edefault%7EBlogCommendFromBaidu%7ERate-2-48161645-blog-18353301.235%5Ev43%5Epc_blog_bottom_relevance_base9&depth_1-utm_source=distribute.pc_relevant.none-task-blog-2%7Edefault%7EBlogCommendFromBaidu%7ERate-2-48161645-blog-18353301.235%5Ev43%5Epc_blog_bottom_relevance_base9&utm_relevant_index=5



extern int g_cpu_total_apci;

extern "C"  __declspec(dllexport) int doReboot(void);

extern "C"  __declspec(dllexport) int doPowerOff(void);

int parseDT(ACPIHeader* dt);
int parseRSDT(ACPIHeader* rsdt);
int parseXSDT(ACPIHeader* rsdt);
int parseRSDP(XSDP_HEADER* xsdp);
int initACPI(void);