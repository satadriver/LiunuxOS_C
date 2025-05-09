#pragma once

#include <windows.h>
#include <iostream>

using namespace std;



#define LIUNUX_FLAG					0x00474a4c

#define MBR_FILENAME				"mbr.com"
#define LOADER_FILENAME				"loader.com"
#define FONT_FILENAME				"font.db"
#define KERNEL_EXE_FILENAME			"kernel.exe"
#define KERNEL_DLL_FILENAME			"kernel.dll"
#define MAIN_DLL_FILENAME			"main.dll"


#pragma pack(1)
typedef struct  
{
	int flag;			//0
	short loaderSecCnt;	//4
	int loaderSecOff;	//6
	short kernelSecCnt;	//10
	int kernelSecOff;	//12
	int mbrSecOff;		//16
	int mbr2SecOff;		//20
	short fontSecCnt;	//24
	int fontSecOff;		//26
	short kerdllSecCnt;	//30
	int kerdllSecOff;	//32
	short maindllSecCnt;	//36
	int maindllSecOff;		//38
	char reserved[22];		//42
}LIUNUX_OS_DATA,*LPLIUNUX_OS_DATA;


typedef struct {
	unsigned char flag;		//80 = active,else = inactive
	unsigned char startchannel;
	unsigned char startvolume;
	unsigned char startdiskno;
	unsigned char type;		//4
	unsigned char endchannel;
	unsigned char endvolume;
	unsigned char enddiskno;
	unsigned int offset;			//8
	unsigned int sectortotal;		//12
}MBR_DPT, *LPMBR_DPT;

typedef struct {
	unsigned char code[446];
	MBR_DPT dpt[4];
	unsigned char systemFlag[2];
}MBR, *LPMBR;

typedef struct
{
	unsigned char BS_jmpBoot[3];		//跳转指令			offset: 0

	unsigned char BS_OEMName[8];		//原始制造商		offset: 3		MSDOS5.0

	unsigned short BPB_BytesPerSec;		//每扇区字节数		offset:11

	unsigned char BPB_SecPerClus; 		//每簇扇区数		offset:13

	unsigned short BPB_RsvdSecCnt; 		//保留扇区数目		offset:14

	unsigned char BPB_NumFATs; 			//此卷中FAT表数		offset:16

	unsigned short BPB_RootEntCnt;		//FAT32为0			offset:17

	unsigned short BPB_TotSec16; 		//FAT32为0			offset:19

	unsigned char BPB_Media;			//存储介质			offset:21

	unsigned short BPB_FATSz16;			//FAT32为 0			offset:22

	unsigned short BPB_SecPerTrk;		//磁道扇区数		offset:24

	unsigned short BPB_NumHeads;		//磁头数			offset:26

	unsigned int BPB_HiddSec; 			//FAT区前隐扇区数	offset:28

	unsigned int BPB_TotSec32;			//该卷总扇区数		offset:32

	unsigned int BPB_FATSz32; 			//一个FAT表扇区数	offset:36

	unsigned short BPB_ExtFlags;		//FAT32特有			offset:40

	unsigned short BPB_FSVer;			//FAT32特有			offset:42

	unsigned int BPB_RootClus;			//根目录簇号		offset:44

	unsigned short FSInfo;				//保留扇区FSINFO扇区数 offset:48

	unsigned short BPB_BkBootSec;		//备份通常为6		offset:50

	unsigned char BPB_Reserved[12];		//扩展用			offset:52

	unsigned char BS_DrvNum;			//offset:64

	unsigned char BS_Reserved1;			//offset:65

	unsigned char BS_BootSig;			//offset:66

	unsigned int BS_VolID;				//offset:67

	unsigned char BS_FilSysType[11];	//offset:71

	unsigned char BS_FilSysType1[8];	//"FAT32 " offset:82

	unsigned char runCode[420];
	unsigned short systemFlag;
}FAT32_DBR, *LPFAT32_DBR;


typedef struct _NTFSDBR {
	BYTE JMP[3];						//0 跳转指令
	BYTE FsID[8];						//3 文件系统ID
	unsigned short bytePerSector;		//11 每扇区字节数
	BYTE secPerCluster;					//13 每簇扇区数
	BYTE reservedBytes[2];				//14 2个保留字节
	BYTE zeroBytes[3];					//16 三个0字节
	BYTE unusedBytes1[2];				//19 2个未用字节
	BYTE mediaType;						//21 媒体类型
	BYTE unusedBytes2[2];				//22 2个未用字节
	unsigned short secPerTrack;			//24 每磁道扇区数
	unsigned short Heads;				//26 磁头数
	unsigned int hideSectors;			//28 隐藏扇区数
	BYTE unusedBytes3[4];				//32 4个未用字节
	BYTE usedBytes[4];					//36 4个固定字节0x800080
	unsigned __int64 totalSectors;		//40 总扇区数
	unsigned __int64 MFT;				//48 MFT起始簇号
	unsigned __int64 MFTMirror;			//56 MFTMirror文件起始簇号
	char fileRecord;					//64 文件记录
	BYTE unusedBytes4[3];				//65 3个未用字节
	char indexSize;						//68 索引缓冲区大小
	BYTE unusedBytes5[3];				//69 未用字节
	BYTE volumeSerialID64[8];			//72 卷序列号
	unsigned int checkSum;				//80 校验和
	BYTE bootCode[426];					//84 引导代码
	BYTE endSignature[2];				//510 结束标志
}NTFSDBR, *LPNTFSDBR;
#pragma pack()

//#define START_SEARCH_SECTOR_NO 256

#define SECTOR_SIZE					512
#define DEVICENAME					"\\\\.\\PHYSICALDRIVE0"

#define EMPTY_SECTOR_NEED			0x800

#define MBR_SIGNATURE               0xAA55


#define EXTENDED_DOS_PARTITION      0x05
#define EXTENDED_WINDOWS_PARTITION  0x0F
#define FAT12_PARTITION				1
#define LINUX_SWAP_PARTITION		0X82
#define LINUX_PARTITION				0X83
#define LINUX_EXTENDED_PARTITION	0X85
#define FAT16_OLD_PARTITION			4
#define FAT16_OLD2_PARTITION		6
#define EXTENDED_PARTITION			0X05
#define FAT32_PARTITION				0X0B
#define FAT32_PARTITION_2			0X0C
#define FAT16_PARTITION				0X0E
#define FAT32_LBA_PARTITION			0X0F
#define FAT12_HIDDEN				0X11
#define FAT16_HIDDEN				0X16
#define FAT32_HIDDEN				0X1B
#define NTFS_PARTITION				7
#define NTFS_HIDDEN					0X17

class SectorReaderWriter {
public:

	static int sectorWriter(const char *name, unsigned int sectoroff, char * data);
	static int dataWriter(unsigned int sectoroffset, int datasize, char * data);

	static int sectorReader(const char *name, unsigned int sectoroffset, int cnt, char * data);

	static int dataReader(unsigned int sectoroffset, int readsize, char * data);

	static int init();
	static void close();
};