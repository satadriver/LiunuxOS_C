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
	char szout[1024];
	int ret = readSector(0, 0, 1, (char*)&gMBR);

	if (*(WORD*)gMBR.systemFlag != 0xaa55)
	{
		__printf(szout,( char*)"MBR format error\r\n");
		return FALSE;
	}
	else {	
		__printf(szout, "getMBR ok\r\n");
	}

	for (int i = 0; i < 4; i++) {
		if (gMBR.dpt[i].flag & 0x80) {
			g_mpartOffset = gMBR.dpt[i].offset;
			g_epartOffset = gMBR.dpt[i+1].offset;

			if (gMBR.dpt[i].type == FAT32_PARTITION || gMBR.dpt[i].type == FAT32_PARTITION_2 ||
				gMBR.dpt[i].type == FAT32_LBA_PARTITION || gMBR.dpt[i].type == FAT32_HIDDEN)
			{
				return 1;
			}
			else if (gMBR.dpt[i].type == NTFS_PARTITION || gMBR.dpt[i].type == NTFS_HIDDEN)
			{
				return 2;
			}
			else if (gMBR.dpt[i].type == LINUX_SWAP_PARTITION || gMBR.dpt[i].type == LINUX_PARTITION || 
				gMBR.dpt[i].type == LINUX_EXTENDED_PARTITION)
			{
				return 3;
			}
			else if (gMBR.dpt[i].type == FAT16_OLD_PARTITION || gMBR.dpt[i].type == FAT16_OLD2_PARTITION ||
				gMBR.dpt[i].type == FAT16_PARTITION || gMBR.dpt[i].type == FAT16_HIDDEN)
			{
				return 4;
			}
			else if (gMBR.dpt[i].type == FAT12_PARTITION || gMBR.dpt[i].type == FAT12_HIDDEN)
			{
				return 5;
			}
			else {
				return 0;
			}
		}
	}

	return 0;
}


int logFile(char * log) {
	int len = __strlen(log);
	int ret = 0;

	ret = writeFile(LOG_FILENAME, log, len,  FILE_WRITE_APPEND);

	return ret;
}