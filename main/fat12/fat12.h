#pragma once
#include "../def.h"
#include "../FileManager.h"

#pragma pack(1)

typedef struct {
	BYTE    JmpCode[3];
	BYTE    BS_OEMName[8];		//3
	WORD    BPB_BytesPerSec;	//11
	BYTE    BPB_SecPerClus;		//13
	WORD    BPB_RsvdSecCnt;		//14
	BYTE    BPB_NumFATs;		//16
	WORD    BPB_RootEntCnt;		//17
	WORD    BPB_TotSec16;		//19
	BYTE    BPB_Media;			//21
	WORD    BPB_FATSz16;		//22
	WORD    BPB_SecPerTrk;		//24
	WORD    BPB_NumHeads;		//26
	DWORD   BPB_HiddSec;		//28
	DWORD   BPB_TotSec32;		//32
	BYTE    BS_DrvNum;			//36
	BYTE    BS_Reserved1;		//37
	BYTE    BS_BootSig;			//38
	DWORD   BS_VolID;			//39
	BYTE    BS_VolLab[11];		//43
	BYTE    BS_FileSysType[8];	//54
	char	code[450];			//62
	char	buffer[512];		//512
}FAT12DBR,*LPFAT12DBR;

#pragma pack()

#define SPT  18
#define HPC  2


int getCylinder(int lba);

int getHeader(int lba);

int getSector(int lba);

int vm86ReadFloppy(int c, int h, int s, short seccnt, char * buf, int disk);

int readFat12Dirs(DWORD secno, LPFILEBROWSER files);

int browseFat12File( LPFILEBROWSER files);

int fat12FileReader(DWORD clusterno, int filesize, char * lpdata, int readsize);

int getNextFAT12Cluster(int clusterno);

