
#include "FAT32.h"
#include "fat32file.h"
#include "fileutils.h"
#include "../Utils.h"
#include "../ata.h"
#include "../video.h"
#include "FAT32Utils.h"
#include "../malloc.h"

DWORD gFat32BusyFlag = 0;

DWORD openFile(const char* fn, int writemode, LPFAT32DIRECTORY outdir,int *dirinsec) {
	int ret = 0;
	unsigned char szout[1024];

	char subpath[MAX_PATH_SIZE];
	ret = removeDriver((char*)fn,subpath);

	char nextpath[MAX_PATH_SIZE];

	LPFAT32DIRECTORY lpdir = (LPFAT32DIRECTORY)__kMalloc(g_ClusterSize * 2);;
	__memcpy((char*)lpdir, (char*)glpRootDir, g_ClusterSize);

	int sectoroffset = g_SecsBeforeRootDir;

	int dirsecno = FALSE;

	unsigned int clusterno = g_FirstClusterNO;

	while (__strlen(subpath) > 0)
	{
		ret = getNextPath(nextpath, subpath);
		if (__strlen(nextpath) > 0)
		{
			LPFAT32DIRECTORY finddir = getDirFromFileName(nextpath, lpdir);
			if (finddir)
			{
				clusterno = getFirstClusterNo(finddir);
				if (*subpath == 0)
				{
					__memcpy((char*)outdir, (char*)finddir, sizeof(FAT32DIRECTORY));
					*dirinsec = (finddir - lpdir) * sizeof(FAT32DIRECTORY) % g_bytesPerSec;
					dirsecno = sectoroffset + (finddir - lpdir) * sizeof(FAT32DIRECTORY) / g_bytesPerSec;
					break;
				}
				else {
					sectoroffset = (clusterno - g_FirstClusterNO)*g_SecsPerCluster + g_SecsBeforeRootDir;
					ret = readSector(sectoroffset, 0, g_SecsPerCluster, (char*)lpdir);
					if (ret)
					{
						continue;
					}
					else {
						__printf((char*)szout, "open file:%s read dir in sector:%d error", fn, sectoroffset);

						break;
					}
				}
			}
			else if ( (writemode & FOLDER_WRITE_OPEN) || (writemode & FILE_WRITE_OPEN) )
			{
				dirsecno = openFileWrite(nextpath, subpath, writemode, clusterno, lpdir, outdir, dirinsec);
				break;
			}
			else {
				//__printf((char*)szout, "NOT FOUND FILE:%s,something error\r\n", fn);
				//__drawGraphChars(szout, 0);
				break;
			}
		}
		else {
			//__printf((char*)szout, "NOT FOUND FILE:%s\r\n", fn);
			//__drawGraphChars(szout, 0);
			break;
		}
	}

	__kFree((DWORD)lpdir);
	return dirsecno;
}




//when over,the dir in fat is set end,but file in fat not set
DWORD openFileWrite(char* curpath, char* leastpath, int writemode, int clusternum,LPFAT32DIRECTORY lpdirectory, LPFAT32DIRECTORY outdir,int * dirinsec) 
{
	int ret = 0;
	char szout[1024];

	int writesize = outdir->size;

	LPFAT32DIRECTORY lpdir = lpdirectory;
	char lpnewdir[0x1000];

	char currentdir[MAX_PATH_SIZE];
	__strcpy(currentdir,curpath);

	char leastdir[MAX_PATH_SIZE];
	__strcpy(leastdir, leastpath);

	int nextfreeclusno = getFsinfoFreeCluster();

	int clusterno = clusternum;

	int secoff = 0;

	while(*currentdir )
	{
		for (int i = 0; i < gMaxDirsInPath; i++)
		{
			lpdir = &lpdirectory[i];
			if (__memcmp((char*)lpdir->mainName, EMPTY_MAINNAME, FAT_MAINNAME_LEN) == 0 &&
				__memcmp((char*)lpdir->suffixName, EMPTY_SUFFIXNAME, FAT_SUFFIXNAME_LEN) == 0)
			{
				if (__strlen(currentdir) > FAT_NAMETOTAL_LEN)
				{
					int unidirsize = setUnicodeFN(lpdir, currentdir);
					lpdir += unidirsize;
				}
				break;
			}
			else {
				lpdir = 0;
			}
		}
		
		if (lpdir == 0)
		{

			__printf(szout, ( char *)"not found empty dir\n");
			secoff = 0;
			break;
		}

		__memset((char*)lpdir, 0, sizeof(FAT32DIRECTORY));
		char fatname[260];
		ret = getFAT32FileName(currentdir,fatname);
		__memcpy((char*)lpdir->mainName, fatname, FAT_MAINNAME_LEN + FAT_SUFFIXNAME_LEN);

		//int iflastfile = 0;

		if (*leastdir == 0)	
		{
			if (writemode & FILE_WRITE_OPEN)
			{
				//iflastfile = TRUE;

				lpdir->attr = FILE_ATTRIBUTE_ARCHIVE;

				lpdir->size = writesize;

				lpdir->clusterHigh = getClusterNoHigh(nextfreeclusno);
				lpdir->clusterLow = getClusterNoLow(nextfreeclusno);

				//glpFAT[nextfreeclusno] = FAT_END_FLAG;
				//ret = updateFAT(nextfreeclusno);

				//gFsInfo.freeClusterCnt = gFsInfo.freeClusterCnt - 1;
				//int nnfcn = getNextFAT32EmptyCluster(nextfreeclusno);
				//ret = updateFSINFO(gFsInfo.freeClusterCnt, nnfcn);
			}
			else if (writemode & FOLDER_WRITE_OPEN) {
				lpdir->attr = FILE_ATTRIBUTE_DIRECTORY;

				lpdir->size = 0;

				lpdir->clusterHigh = getClusterNoHigh(nextfreeclusno);
				lpdir->clusterLow = getClusterNoLow(nextfreeclusno);

				glpFAT[nextfreeclusno] = FAT_END_FLAG;
				ret = updateFAT(nextfreeclusno);

				gFsInfo.freeClusterCnt = gFsInfo.freeClusterCnt - 1;
				int nnfcn = getNextFAT32EmptyCluster(nextfreeclusno);
				ret = updateFSINFO(gFsInfo.freeClusterCnt, nnfcn);
			}
			else {
				__printf(szout, ( char *)"create file mode error\n");
				secoff = 0;
				break;
			}
		}
		else {
			lpdir->attr = FILE_ATTRIBUTE_DIRECTORY;

			lpdir->size = 0;

			lpdir->clusterHigh = getClusterNoHigh(nextfreeclusno);
			lpdir->clusterLow = getClusterNoLow(nextfreeclusno);

			glpFAT[nextfreeclusno] = FAT_END_FLAG;
			ret = updateFAT(nextfreeclusno);

			gFsInfo.freeClusterCnt = gFsInfo.freeClusterCnt - 1;
			int nnfcn = getNextFAT32EmptyCluster(nextfreeclusno);
			ret = updateFSINFO(gFsInfo.freeClusterCnt, nnfcn);
		}

		*dirinsec = (lpdir - lpdirectory) * sizeof(FAT32DIRECTORY) % g_bytesPerSec;
		*dirinsec = *dirinsec | 0x80000000;
		int dirsecoffset = (lpdir - lpdirectory) * sizeof(FAT32DIRECTORY) / g_bytesPerSec;
		int sectoroffset = (clusterno - g_FirstClusterNO)*g_SecsPerCluster + g_SecsBeforeRootDir;
		secoff = sectoroffset + dirsecoffset;
		char * dirdataoff = (char*)lpdirectory + dirsecoffset * g_bytesPerSec;

		__memcpy((char*)outdir, (char*)lpdir, sizeof(FAT32DIRECTORY));

		//if (iflastfile == TRUE)
		{
			//return secoff;
		}

		ret = writeSector(secoff, 0, 1, (char*)dirdataoff);
		if (ret <= 0)
		{
			__printf(szout, ( char *)"create file write error\n");
			secoff = 0;
			break;
		}

		clusterno = nextfreeclusno;

		nextfreeclusno = getNextFAT32EmptyCluster(nextfreeclusno );

		ret = getNextPath(currentdir, leastdir);
			
		__memset(lpnewdir, 0,g_bytesPerSec);
		lpdirectory = (LPFAT32DIRECTORY)lpnewdir;
	} 

	return secoff;
}




int readFat32File(const char * filename, char ** buf) {
	if (buf == 0)
	{
		return 0;
	}

	int ret = 0;

	int dirinsec = 0;
	FAT32DIRECTORY dir;
	__memset((char*)&dir, 0, sizeof(FAT32DIRECTORY));

	int secno = openFile(filename, FILE_READ_OPEN, &dir, &dirinsec);
	if (secno)
	{
		if (*buf == 0)
		{
			*buf = (char*)__kMalloc(dir.size);
		}
		ret = clusterReader(&dir, *buf, dir.size);

// 		unsigned char szout[1024];
// 		__printf((char*)szout, "find file:%s in dir:%x,sector no:%x,dir in sector:%x", filename, dir, secno, dirinsec);
// 		__drawGraphChars(szout, 0);
		*(*buf + dir.size) = 0;

		ret = dir.size;
	}
	return ret;
}

int writeFat32Dir(char * filename) {

	int dirinsec = 0;

	FAT32DIRECTORY dir;
	__memset((char*)&dir, 0, sizeof(FAT32DIRECTORY));

	return openFile(filename, FOLDER_WRITE_OPEN,&dir,&dirinsec);
}


int writeFat32File(const char * filename, char * buf,int size, int writemode) {
	int ret = 0;

	int dirinsec = 0;

	FAT32DIRECTORY dir;
	__memset((char*)&dir, 0, sizeof(FAT32DIRECTORY));
	dir.size = size;

	int secno = openFile(filename, FILE_WRITE_OPEN, &dir, &dirinsec);
	if (secno)
	{
		ret = clusterWriter(&dir, buf, size, secno, writemode, dirinsec);

// 		unsigned char szout[1024];
// 		__printf((char*)szout, "find file:%s in dir:%x,sector no:%x,dir in sector:%x", filename, dir, secno, dirinsec);
// 		__drawGraphChars(szout, 0);
	}

	return ret;
}


