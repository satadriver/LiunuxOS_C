#include "ntfs.h"
#include "../file.h"
#include "../ata.h"
#include "ntfsFile.h"
#include "../Utils.h"
#include "../video.h"

NTFSDBR				gNtfsDbr;

unsigned __int64	gMsfOffset;

char		gMsfRoot[MFTEntrySize];


int initNTFS() {
	int ret = 0;

	readFile = readNtfsFile;
	writeFile = writeNtfsFile;

	ret = getNtfsDBR();

	g_bytesPerSec = gNtfsDbr.bytePerSector;
	g_SecsPerCluster = gNtfsDbr.secPerCluster;
	g_ClusterSize = g_SecsPerCluster * g_bytesPerSec;
	gMsfOffset = gNtfsDbr.hideSectors + gNtfsDbr.MFT * g_SecsPerCluster;

	ret = readMSFRoot();

#ifdef LIUNUX_DEBUG_FLAG
	unsigned char szout[256];
	__printf((char*)szout, "ntfs bytes per sector:%x,cluster size:%x,sector per cluster:%x\n",
		g_bytesPerSec, g_ClusterSize, g_SecsPerCluster);
	__drawGraphChars((unsigned char*)szout, 0);
	__memset((char*)szout, 0, 4096);
	__dump((char*)&gNtfsDbr, BYTES_PER_SECTOR, 0, szout);
	__drawGraphChars(szout, 0);
#endif

	return 0;
}



int getNtfsDBR() {
	int ret = 0;

	for (int i = 0; i < 4; i++)
	{
		if (gMBR.dpt[i].flag == 0x80) {
			unsigned int secno = gMBR.dpt[i].offset;
			ret = readSector(secno, 0, 1, (char*)&gNtfsDbr);
			if (__memcmp((char*)gNtfsDbr.FsID, "NTFS    ", 8) == 0)
			{
				break;
			}
		}
	}

	return ret;
}



int readMSFRoot() {

	DWORD low = (gMsfOffset + MSF_ROOTDIR_OFFSET) & 0xffffffff;
	DWORD high = ( (gMsfOffset + MSF_ROOTDIR_OFFSET) >> 32 ) & 0xffff;
	int ret = readSector(low, high, 2, gMsfRoot);
	return ret;
}


int getNtfsClusterStream(unsigned char* ptr, unsigned __int64* clsno, DWORD* clscnt) {
	unsigned int i = 0;

	while (*(ptr + i))
	{
		int clscntbytes = (*(ptr + i) & 0xf);
		int clsnobytes = ((*(ptr + i) & 0xf0) >> 4);
		if (clscntbytes == 0 || clscntbytes > 4 || clsnobytes == 0 || clsnobytes > 4)
		{
			break;
		}

		i++;

		char* lpclscnt = (char*)clscnt;
		for (int j = 0; j < clscntbytes; j++)
		{
			lpclscnt[j] = ptr[i + j];
		}
		i += clscntbytes;


		unsigned __int64 nextclsno = 0;
		char* lpclsno = (char*)&nextclsno;
		for (int k = 0; k < clsnobytes; k++)
		{
			lpclsno[k] = ptr[i + k];
		}
		i += clsnobytes;

		if (*clscnt == 2 && (nextclsno & 0x8000))
		{
			nextclsno = nextclsno | 0xffffffffffff0000;
		}
		else if (*clscnt == 3 && (nextclsno & 0x800000))
		{
			nextclsno = nextclsno | 0xffffffffff000000;
		}
		else if (*clscnt == 4 && (nextclsno & 0x80000000))
		{
			nextclsno = nextclsno | 0xffffffff00000000;
		}

		*clsno = *clsno + nextclsno;
	}
	return i;
}



unsigned long long make_uint64(unsigned char* buf, int lenth)
{
	unsigned long long ui = 0;
	if (lenth > 8)
	{
		return (unsigned long long)0;
	}

	for (int i = 0; i < lenth; i++)
	{
		ui = (buf[i] << 8 * i) | ui;
	}
	return ui;
}


long long make_int64(char* buf, int lenth)
{
	long long ui = 0;
	if (lenth > 8)
	{
		return ( long long)0;
	}

	for (int i = 0; i < lenth; i++)
	{
		ui = (buf[i] << 8 * i) | ui;
	}

	// 判断符号位,为负则需减取反  
	if (buf[lenth - 1] >= 0x80)
	{
		 long long xorval = 0;
		for (int i = 0; i < lenth; i++)
		{
			xorval = xorval | (0xFF << 8 * i);
		}
		ui = -((ui - 1) ^ xorval);
	}
	return ui;
}




