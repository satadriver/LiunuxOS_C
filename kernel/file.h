#pragma once
#include "def.h"

#pragma pack(1)
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
	char hdr[20];
	char seq[20];
	char unknown1[6];
	char firmware_version[8];
	char type[40];
}HARDDISKINFORMATION;


typedef struct
{
	char pathname[256];
	DWORD secno;
	DWORD filesize;
	DWORD attrib;
}FILEBROWSER, *LPFILEBROWSER;

#pragma pack()

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






#define FILE_READ_OPEN			0
#define FILE_WRITE_OPEN			1
#define FOLDER_WRITE_OPEN		2

#define FILE_WRITE_APPEND		0X100
#define FILE_WRITE_COVER		0X200
#define FILE_WRITE_RESIZE		0X400





#ifdef DLL_EXPORT
extern "C" __declspec(dllexport) MBR gMBR;

extern "C" __declspec(dllexport) int g_mpartOffset;
extern "C" __declspec(dllexport) int g_epartOffset;
extern "C" __declspec(dllexport) int g_bytesPerSec;
extern "C" __declspec(dllexport) int g_SecsPerCluster;
extern "C" __declspec(dllexport) int g_ClusterSize;

extern "C" __declspec(dllexport) int getMBR();

extern "C" int logFile(char * log);

extern "C" __declspec(dllexport) char * gLogDataPtr;

void logInMem(char * data, int len);

extern "C" __declspec(dllexport) int readFileTo(char * filename);

extern "C" int initFileSystem();

extern "C" __declspec(dllexport)  int ( __cdecl * readFile)(char * filename,char ** buf);

extern "C" __declspec(dllexport)  int (__cdecl * writeFile)(char * filename, char * buf, int size, int writemode);
#else
extern "C" __declspec(dllimport) char * gLogDataPtr;
extern "C" __declspec(dllimport) MBR gMBR;

extern "C" __declspec(dllimport) int	g_mpartOffset;
extern "C" __declspec(dllimport) int	g_epartOffset;
extern "C" __declspec(dllimport) int g_bytesPerSec;
extern "C" __declspec(dllimport) int g_SecsPerCluster;
extern "C" __declspec(dllimport) int g_ClusterSize;

extern "C" __declspec(dllimport) int getMBR();

extern "C" int logFile(char * log);

extern "C" __declspec(dllimport)int readFileTo(char * filename);

extern "C" int initFileSystem();


extern "C" __declspec(dllimport)  int(__cdecl * readFile)(char * filename, char ** buf);

extern "C" __declspec(dllimport)  int(__cdecl * writeFile)(char * filename, char * buf, int size, int writemode);

#endif

//extern "C" __declspec(dllexport)  LPREADFILE readFile;
//extern "C" __declspec(dllexport)  LPWRITEFILE writeFile;




#define LOG_FILENAME		"liunux.log"