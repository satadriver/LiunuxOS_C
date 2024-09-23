#include "FAT32.h"
#include "../ata.h"
#include "../Utils.h"
#include "../video.h"
#include "FileUtils.h"
#include "Fat32File.h"
#include "../malloc.h"



FAT32_DBR			gFat32Dbr;
FAT32_FSINFO		gFsInfo;
unsigned int *		glpFAT = 0;
unsigned int *		glpFAT2 = 0;
LPFAT32DIRECTORY	glpRootDir = 0;

int		g_FirstClusterNO	= 2;
DWORD	g_SecsBeforeRootDir = 0;
int		gMaxDirsInPath		= 256;



int fat32Init() {
	int ret = 0;
	char szout[1024];
	readFile = readFat32File;
	writeFile = writeFat32File;

// #ifdef LIUNUX_DEBUG_FLAG
 	unsigned char szshow[4096];
// 	__printf((char*)szshow, "fat32 readFile:%x,writeFile:%x,MBR:\n", readFile, writeFile);
// 	__drawGraphChars((unsigned char*)szshow, 0);
// 
// 	__memset((char*)szshow, 0, 4096);
// 	__dump((char*)&gMBR, 512, 0, szshow);
// 	__drawGraphChars((unsigned char*)szshow, 0);
// #endif

	ret = getFat32DBR();
	if (ret <= 0)
	{
		
		__printf(szout, ( char*)"fat32 dbr format error\r\n");
		return FALSE;
	}

	g_bytesPerSec = gFat32Dbr.BPB_BytesPerSec;
	g_SecsPerCluster = gFat32Dbr.BPB_SecPerClus;
	g_FirstClusterNO = gFat32Dbr.BPB_RootClus;
	g_ClusterSize = g_bytesPerSec * gFat32Dbr.BPB_SecPerClus;
	g_SecsBeforeRootDir = getRootDirOffset();
	gMaxDirsInPath = gFat32Dbr.BPB_SecPerClus * g_bytesPerSec / sizeof(FAT32DIRECTORY);


#ifdef LIUNUX_DEBUG_FLAG
	__printf((char*)szshow, "fat32 rootdir:%x,sector per cluster:%x,bytes per sector:%u,root dir max:%d,cluster:%x,first cluster no:%x,fat32 DBR:\r\n",
		g_SecsBeforeRootDir, gFat32Dbr.BPB_SecPerClus,g_bytesPerSec, gMaxDirsInPath, g_ClusterSize, g_FirstClusterNO);
	__drawGraphChars((unsigned char*)szshow, 0);

// 	__memset((char*)szshow, 0, 4096);
// 	__dump((char*)&gFat32Dbr, 512, 0, szshow);
// 	__drawGraphChars((unsigned char*)szshow, 0);
#endif

	ret = getFsinfo();

// #ifdef LIUNUX_DEBUG_FLAG
// 	__drawGraphChars((unsigned char*)"fat32 FSINFO:\r\n",0);
// 	__memset((char*)szshow, 0, 4096);
// 	__dump((char*)&gFsInfo, 512, 0, szshow);
// 	__drawGraphChars((unsigned char*)szshow, 0);
// #endif
	
	ret = getFAT();
	//ret = getFAT2();

#ifdef LIUNUX_DEBUG_FLAG
	__drawGraphChars((unsigned char*)"fat32 FAT:\r\n",0);
	__memset((char*)szshow, 0, 4096);
	__dump((char*)glpFAT, 512, 0, szshow);
	__drawGraphChars((unsigned char*)szshow, 0);
#endif

	ret = getRootDir();

#ifdef LIUNUX_DEBUG_FLAG
	__drawGraphChars((unsigned char*)"fat32 ROOT:\r\n",0);
	__memset((char*)szshow, 0, 4096);
	__dump((char*)glpRootDir, 512, 0, szshow);
	__drawGraphChars((unsigned char*)szshow, 0);
#endif

	return ret;
}




int isFAT32(unsigned char * fat) {
	if (__memcmp((char*)fat, FAT_HEAD_FLAG, 8) == 0)
	{
		return TRUE;
	}

	return FALSE;
}

int checkFlag(unsigned char * sector) {
	unsigned short flag = *(unsigned short*)(sector + BYTES_PER_SECTOR - 2);
	if (flag != FAT32_FLAG)
	{
		return FALSE;
	}
	return TRUE;
}

int isFAT32DBR(LPFAT32_DBR lpfat32dbr) {

	char* oem = (char*)lpfat32dbr->BS_OEMName;
	if (__memcmp(oem,"MSDOS5.0",8) )
	{
		return FALSE;
	}

	char * type = (char*)lpfat32dbr->BS_FilSysType1;
	if (__memcmp(type,"FAT32",5) )
	{
		return FALSE;
	}

	if (checkFlag((unsigned char*)lpfat32dbr) == FALSE)
	{
		return FALSE;
	}

	return TRUE;
}






int getFat32DBR() {
	int ret = 0;
	
	for (int i = 0; i < 4; i++) {
		if (gMBR.dpt[i].flag & 0x80) {
			int secno = gMBR.dpt[i].offset;
			ret = readSector(secno, 0, 1, (char*)&gFat32Dbr);
			ret = isFAT32DBR(&gFat32Dbr);
			if (ret) {
				break;
			}
		}
	}

	return ret;
}

int getFSINFOOffset() {
	int offset = gFat32Dbr.FSInfo + g_mpartOffset;
	//int offset = gFat32Dbr.FSInfo + gFat32Dbr.BPB_HiddSec;
	return offset;
}

int getFsinfo() {
	int fsinfosector = getFSINFOOffset();
	int ret = 0;

	ret = readSector(fsinfosector,0, 1, (char*)&gFsInfo);

	return TRUE;
}

DWORD getFATOffset() {
	//int reserveDbrSec = gFat32Dbr.BPB_RsvdSecCnt + g_mpartOffset;
	DWORD reserveDbrSec = gFat32Dbr.BPB_RsvdSecCnt + gFat32Dbr.BPB_HiddSec;
	return reserveDbrSec;
}

int getFAT() {
	if (glpFAT == 0)
	{
		glpFAT = (unsigned int*)__kMalloc(gFat32Dbr.BPB_FATSz32*g_bytesPerSec);
	}
	

	DWORD offset = getFATOffset();

	int ret = readSector(offset, 0, gFat32Dbr.BPB_FATSz32, (char*)glpFAT);

	return ret;
}

DWORD getFAT2Offset() {
	//int reserveDbrSec = gFat32Dbr.BPB_RsvdSecCnt + g_mpartOffset;
	DWORD reserveDbrSec = gFat32Dbr.BPB_RsvdSecCnt + gFat32Dbr.BPB_HiddSec;
	
	DWORD fatSize = gFat32Dbr.BPB_FATSz32;

	//int fatCnt = gFat32Dbr.BPB_NumFATs;

	DWORD secOffset = fatSize + reserveDbrSec;
	return secOffset;
}

int getFAT2() {
	DWORD offset = getFAT2Offset();
	if (offset == 0)
	{
		return 0;
	}

	if (glpFAT2 == 0)
	{
		glpFAT2 = (unsigned int*)__kMalloc(gFat32Dbr.BPB_FATSz32*g_bytesPerSec);
	}

	int ret = readSector(offset, 0, gFat32Dbr.BPB_FATSz32, (char*)glpFAT2);
	return ret;
}

//保留扇区数+FAT 表的个数*FAT 表的大小扇区数+（（该簇簇号-2）*每簇扇区数） = 某簇起始扇区号
DWORD getRootDirOffset() {
	DWORD reserveDbrSec = gFat32Dbr.BPB_RsvdSecCnt + gFat32Dbr.BPB_HiddSec;
	//int reserveDbrSec = gFat32Dbr.BPB_RsvdSecCnt + g_mpartOffset;

	DWORD fatSize = gFat32Dbr.BPB_FATSz32;

	DWORD fatCnt = gFat32Dbr.BPB_NumFATs;

	DWORD secOffset = (fatCnt * fatSize) + reserveDbrSec + (g_FirstClusterNO - 2)*g_SecsPerCluster;
	return secOffset;
}

int getRootDir() {
	if (glpRootDir == 0)
	{
		glpRootDir = (LPFAT32DIRECTORY)__kMalloc(g_ClusterSize * 2);
	}
	
	DWORD offset = getRootDirOffset();
	//int rootdirsize = gFat32Dbr.BPB_SecPerClus * g_bytesPerSec;

	int ret = readSector(offset, 0, gFat32Dbr.BPB_SecPerClus, (char*)glpRootDir);
	
	return ret;
}


int getNextFAT32Cluster(DWORD no) {
	int cluster = glpFAT[no];
	return cluster;
}

int getNextFAT32EmptyCluster(DWORD clusterno) {
	
	DWORD max = (gFat32Dbr.BPB_FATSz32*g_bytesPerSec) / sizeof(int);

	int nextfreeclusno = clusterno;
	for (; nextfreeclusno < max;  nextfreeclusno++)
	{
		int clustervalue = glpFAT[nextfreeclusno];
		if (clustervalue == FAT_EMPTY_CLUSTER)
		{
			return nextfreeclusno;
		}
		else
		{
			continue;
		}
	}
	return FALSE;
}

unsigned int getLastClusterNo(LPFAT32DIRECTORY lpdir) {
	unsigned int cno = getFirstClusterNo(lpdir);
	while (1)
	{
		unsigned int value = glpFAT[cno];
		if (value == FAT_END_FLAG)
		{
			return cno;
		}
		else if ((value >= g_FirstClusterNO) && (value < FAT_END_FLAG) )
		{
			cno = value;
		}
		else if (value == FAT_EMPTY_CLUSTER)	//create new file,but no data
		{
			return cno;
		}
		else
		{
			break;
		}
	}
	return FALSE;
}


unsigned int getFirstClusterNo(LPFAT32DIRECTORY lpdir) {
	unsigned short high = lpdir->clusterHigh;
	unsigned short low = lpdir->clusterLow;
	unsigned int no = (high << 16) + low;
	return no;
}

unsigned short getClusterNoLow(unsigned int clusterno) {
	return clusterno & 0xffff;
}
unsigned short getClusterNoHigh(unsigned int clusterno) {
	return (clusterno >> 16);
}

int isValidCluster(unsigned int no) {
	if (no >= g_FirstClusterNO && no <= FAT_END_FLAG)
	{
		return TRUE;
	}

	return FALSE;
}

int getFsinfoFreeCluster() {
	return gFsInfo.nextCluster;
}

int updateFSINFO(int leastclusters, int nextfreecluster) {
	gFsInfo.nextCluster = nextfreecluster;
	gFsInfo.freeClusterCnt = leastclusters;

	int ret = writeSector( gFat32Dbr.FSInfo, 0, 1,(char*)&gFsInfo);
	return ret;
}

int updateFAT(int clusterno) {
	//int sectoroffset = (clusterno - g_FirstClusterNO) * sizeof(int) / g_bytesPerSec;
	int sectoroffset = clusterno * sizeof(int) / g_bytesPerSec;

	int sectorno = sectoroffset + getFATOffset();

	char * data = (char*)(sectoroffset*g_bytesPerSec + (unsigned char*)glpFAT);

	int ret = writeSector(sectorno, 0, 1, data);

	if (gFat32Dbr.BPB_NumFATs >= 2)
	{
		sectorno = sectoroffset + getFAT2Offset();

		ret = writeSector(sectorno, 0, 1, data);
	}
	return ret;
}

int updateSecsTotal(unsigned int size) {
	int secs = size / g_bytesPerSec;
	gFat32Dbr.BPB_TotSec32 -= secs;
	int ret = writeSector(0, 0, 1, (char*)&gFat32Dbr);
	return ret;
}



unsigned int getUnicodeDirectoryChecksum(unsigned char * shortname) {
	unsigned int i, j = 0, chksum = 0;
	for (i = 11; i > 0; i--) {
		chksum = ((chksum & 1) ? 0x80 : 0) + (chksum >> 1) + shortname[j++];
	}
	return chksum;
}

int getKey(FAT32_DBR dbr, FAT32_FSINFO info,unsigned char * key) {

	int totalsecs = dbr.BPB_TotSec32;
	int fatsize = dbr.BPB_FATSz32;
	int beforefat = dbr.BPB_RsvdSecCnt;
	int secsperclu = dbr.BPB_SecPerClus;

	unsigned int * pkey = (unsigned int*)key;

	pkey[0] = totalsecs;
	pkey[1] = fatsize;
	pkey[2] = beforefat;
	pkey[3] = secsperclu;

	for (int i = 0 ;i < 16 ;i ++)
	{
		key[i] ^= dbr.runCode[i];
	}
 
	return 16;
}



