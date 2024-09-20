#include "fat32/Fat32File.h"
#include "file.h"
#include "Utils.h"
#include "ata.h"
#include "NTFS/ntfs.h"
#include "video.h"
#include "NTFS/ntfsFile.h"



extern "C" __declspec(dllexport)  int (__cdecl * readFile)(char * filename, char ** buf) = readNtfsFile;

extern "C" __declspec(dllexport)  int (__cdecl * writeFile)(char * filename, char * buf, int size,  int writemode) = writeNtfsFile;

MBR		gMBR;
int		g_mpartOffset = 0;
int		g_epartOffset = 0;
int		g_bytesPerSec = BYTES_PER_SECTOR;
int		g_SecsPerCluster = 16;
int		g_ClusterSize = g_SecsPerCluster * g_bytesPerSec;


char * gLogDataPtr = (char*) LOG_BUFFER_BASE;

void logInMem(char * data,int len) {
	if (len >= 1024)
	{
		return;
	}

	if ((DWORD)gLogDataPtr + len >= LOG_BUFFER_BASE + 0x10000)
	{
		gLogDataPtr = (char*)LOG_BUFFER_BASE;
	}
	__memcpy(gLogDataPtr, data, len);
	gLogDataPtr += len;
}


int readFileTo(char * filename) {
	char * buf = (char*)FILE_BUFFER_ADDRESS;
	return readFile(filename, &buf);
}


int initFileSystem() {
	int ret = 0;
	ret = __initIDE();
	if (ret == FALSE)
	{
		return FALSE;
	}

	ret = getMBR();
	if (ret == 1)
	{
		ret = fat32Init();
	}else if (ret == 2)
	{
		ret = initNTFS();
	}
	else {
		return 0;
	}

	char szout[1024];
	__printf(szout, "initFileSystem ok\r\n");
	return TRUE;
}

int getMBR() {
	int ret = readSector(0, 0, 1, (char*)&gMBR);

	if (*(WORD*)gMBR.systemFlag != 0xaa55)
	{
		__drawGraphChars(( char*)"MBR format error\r\n", 0);
		return FALSE;
	}
	else {
		char szout[1024];
		__printf(szout, "getMBR ok\r\n");
	}

	g_mpartOffset = gMBR.dpt[0].offset;
	g_epartOffset = gMBR.dpt[1].offset;

	if (gMBR.dpt[0].type == FAT32_PARTITION || gMBR.dpt[0].type == FAT32_PARTITION_2 ||
		gMBR.dpt[0].type == FAT32_LBA_PARTITION || gMBR.dpt[0].type == FAT32_HIDDEN)
	{
		return 1;
	}
	else if (gMBR.dpt[0].type == NTFS_PARTITION || gMBR.dpt[0].type == NTFS_HIDDEN)
	{
		return 2;
	}
	else if (gMBR.dpt[0].type == LINUX_SWAP_PARTITION || gMBR.dpt[0].type == LINUX_PARTITION || gMBR.dpt[0].type == LINUX_EXTENDED_PARTITION)
	{
		return 3;
	}
	else if (gMBR.dpt[0].type == FAT16_OLD_PARTITION || gMBR.dpt[0].type == FAT16_OLD2_PARTITION ||
		gMBR.dpt[0].type == FAT16_PARTITION || gMBR.dpt[0].type == FAT16_HIDDEN)
	{
		return 4;
	}
	else if (gMBR.dpt[0].type == FAT12_PARTITION || gMBR.dpt[0].type == FAT12_HIDDEN)
	{
		return 5;
	}
	else {
		return 0;
	}
	return 0;
}


int logFile(char * log) {
	int len = __strlen(log);
	int ret = 0;

	ret = writeFile(LOG_FILENAME, log, len,  FILE_WRITE_APPEND);

	return ret;
}