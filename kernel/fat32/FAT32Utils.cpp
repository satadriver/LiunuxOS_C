
#include "FAT32.h"
#include "../ata.h"
#include "../Utils.h"
#include "../video.h"
#include "FAT32Utils.h"
#include "FileUtils.h"
#include "..\\malloc.h"


//取长文件名的前6个字符加上"~1"形成短文件名，扩展名不变
//如果已存在这个文件名，则符号"~"后的数字递增，直到5
int getFAT32FileName(char * fn,char * dst){

	char * dotstr = __strstr(fn, (char*)".");
	if (dotstr > 0)
	{
		int mainnamelen = dotstr - fn;

		char mainname[FAT_MAINNAME_LEN + 1];
		__memset(mainname, ' ', FAT_MAINNAME_LEN);
		*(mainname + FAT_MAINNAME_LEN) = 0;

		if (mainnamelen > FAT_MAINNAME_LEN)
		{
			__memcpy(mainname, fn, FAT_MAINNAME_LEN - 2);
			__memcpy(mainname + FAT_MAINNAME_LEN - 2, (char*)"~1", 2);
		}
		else {
			__memcpy(mainname, fn, mainnamelen);
		}

		char suffix[FAT_SUFFIXNAME_LEN + 1];
		__memset(suffix, ' ', FAT_SUFFIXNAME_LEN);
		*(suffix + FAT_SUFFIXNAME_LEN) = 0;

		int suffixlen = __strlen(dotstr + 1);
		if (suffixlen > FAT_SUFFIXNAME_LEN)
		{
			__memcpy(suffix, dotstr + 1, FAT_SUFFIXNAME_LEN);
		}
		else {
			__memcpy(suffix, dotstr + 1, suffixlen);
		}

		__strcpy(dst, mainname);
		__strcat(dst, suffix);

		return __strupr(dst);
	}
	else {
		char allname[FAT_NAMETOTAL_LEN + 1];
		__memset(allname, 0x20, FAT_NAMETOTAL_LEN);
		*(allname + FAT_NAMETOTAL_LEN) = 0;

		int len = __strlen(fn);

		if (len > FAT_MAINNAME_LEN)
		{
			__memcpy(allname, fn, FAT_MAINNAME_LEN - 2);
			__memcpy(allname + FAT_MAINNAME_LEN - 2, (char*)"~1", 2);
		}
		else {
			__memcpy(allname, fn, len);
		}

		__strcpy(dst, allname);
		return __strupr((char*)dst);
	}

	return 0;
}


int getUnicodeFN(LPFAT32DIRECTORY lpdir, char * filename) {
	int ret = 0;
	char szout[1024];

	LPFAT32_UNICODE_DIRECTORY unicodedir = (LPFAT32_UNICODE_DIRECTORY)lpdir;
	//bit6 代表是否是unicode，bit0-bit4代表序号
	int no = unicodedir->flag & 0x1f;
	int endflag = unicodedir->flag & 0x40;
	if (endflag == 0x40 && no > 0 && no <= 0x1f)
	{
		for (int j = 0; j < no - 1; j++)
		{
			if (unicodedir->attr != 0x0f)
			{
				return FALSE;
			}

			if ((unicodedir->flag & 0x40) == 0 && (unicodedir->flag & 0x1f) == 1)
			{
				break;
			}

			unicodedir++;
		}

		wchar_t orguni[MAX_PATH_SIZE];
		wchar_t *ptr = orguni;
		for (int j = no - 1; j >= 0; j--)
		{
			__memcpy((char*)ptr, (char*)unicodedir->firstUnicode, 10);
			ptr += 5;

			__memcpy((char*)ptr, (char*)unicodedir->midUnicode, 12);
			ptr += 6;

			__memcpy((char*)ptr, (char*)unicodedir->lastUnicode, 4);
			ptr += 2;

			unicodedir--;
		}
		*ptr = 0;

		char szfn[MAX_PATH_SIZE];
		ret = wideChar2MultiBytes(orguni, szfn);
		//string shortfilename = string((char*)lpdir[i].mainName,11);
		//unsigned int shortchecksum = getUnicodeDirectoryChecksum((unsigned char*)shortfilename.c_str());
		ret = __strcmp(szfn, filename);
		if (ret == 0)
		{
			return no;
		}
	}

	__printf(szout, "getUnicodeFN error\r\n");

	return FALSE;
}

int setUnicodeFN(LPFAT32DIRECTORY lpdir, char * filename) {
	int ret = 0;
	int fnlen = __strlen(filename);
	int cnt = fnlen / 13;
	int mod = fnlen % 13;
	int totol = cnt;
	if (mod)
	{
		totol++;
	}
	if (totol > 0x1f || totol <= 0)
	{
		return FALSE;
	}

	char shortname[MAX_PATH_SIZE];
	ret = getFAT32FileName(filename, shortname);
	unsigned char checksum = getUnicodeDirectoryChecksum((unsigned char*)shortname);

	//wchar_t wszfn[1024] = { 0 };
	wchar_t wszfn[MAX_PATH_SIZE];
	__memset((char*)wszfn, 0, MAX_PATH_SIZE * sizeof(wchar_t));
	ret = multiBytes2WideChar(shortname, wszfn);

	LPFAT32_UNICODE_DIRECTORY tmpdir = (LPFAT32_UNICODE_DIRECTORY)(lpdir + totol - 1);
	wchar_t * tmpstr = wszfn;

	for (int i = 0; i < totol; i++)
	{
		__memset((char*)tmpdir, 0, sizeof(FAT32_UNICODE_DIRECTORY));

		tmpdir->attr = 0x0f;

		tmpdir->checksum = checksum;

		if (i == totol - 1)
		{
			tmpdir->flag = (0x40 + totol);

			__memmove((char*)tmpdir->firstUnicode, (char*)tmpstr, 10);
			tmpstr += 5;

			__memmove((char*)tmpdir->midUnicode, (char*)tmpstr, 12);
			tmpstr += 6;

			__memmove((char*)tmpdir->lastUnicode, (char*)tmpstr, 4);
			tmpstr += 2;
		}
		else {
			tmpdir->flag = i + 1;

			__memmove((char*)tmpdir->firstUnicode, (char*)tmpstr, 10);
			tmpstr += 5;

			__memmove((char*)tmpdir->midUnicode, (char*)tmpstr, 12);
			tmpstr += 6;

			__memmove((char*)tmpdir->lastUnicode, (char*)tmpstr, 4);
			tmpstr += 2;
		}

		tmpdir--;
	}

	return totol;
}


LPFAT32DIRECTORY getDirFromFileName(char * filename, LPFAT32DIRECTORY  lpdir) {
	int ret = 0;
	
	char fatname[FAT_NAMETOTAL_LEN + 1];
	__memset(fatname, 0, FAT_NAMETOTAL_LEN + 1);
	ret = getFAT32FileName(filename,fatname);

	for (int i = 0; i < gMaxDirsInPath; i++)
	{
// 		unsigned char szout[1024];
// 		__printf((char*)szout, "search file:%s in dir:%s", fatname, lpdir[i].mainName);
// 		__drawGraphChars(szout, 0);

		if ((lpdir[i].attr & FILE_ATTRIBUTE_DIRECTORY) || (lpdir[i].attr & FILE_ATTRIBUTE_ARCHIVE)) {
			if (__memcmp((char*)lpdir[i].mainName, fatname, FAT_MAINNAME_LEN ) == 0 &&
				__memcmp((char*)lpdir[i].suffixName, fatname + FAT_MAINNAME_LEN, FAT_SUFFIXNAME_LEN) == 0)
			{
				return &lpdir[i];
			}
		}
		else if (lpdir[i].attr == 0x0f)
		{
			int no = getUnicodeFN(&lpdir[i], filename);
			if (no > 0)
			{
				i += no;
				return &lpdir[i];
			}
			else {
				//i--;
			}
		}
	}

	return NULL;
}




int clusterReader( LPFAT32DIRECTORY dir, char * buf, int readsize) {
	char szout[1024];
	int ret = 0;
	if ((unsigned int)readsize > dir->size)
	{
		readsize = dir->size;
	}
	else if (readsize <= 0)
	{
		return FALSE;
	}

	char * lpdata = buf;
	int clusterno = getFirstClusterNo(dir);

	int readtimes = readsize / g_ClusterSize;
	int readmod = readsize % g_ClusterSize;
	if (readmod)
	{
		readtimes++;
	}

	int sectorno = 0;
	for (int i = 0; i < readtimes; i++)
	{
		sectorno = g_SecsBeforeRootDir + (clusterno - g_FirstClusterNO) * g_SecsPerCluster;
		ret = readSector(sectorno,0, g_SecsPerCluster, (char*)lpdata);
		if (ret == g_ClusterSize)
		{
			lpdata += g_ClusterSize;
		}
		else {
			__printf(szout, ( char*)"fat32 read cluster error\n");
			break;
		}

		clusterno = getNextFAT32Cluster( clusterno);
		if (clusterno == FAT_END_FLAG || clusterno == FAT_EMPTY_CLUSTER)
		{
			break;
		}
	}

	return readsize;
}



int clusterWriter(LPFAT32DIRECTORY dir, char * buf, int writesize, int dirsecoff, int mode, int dirinsec) {
	int ret = 0;

	if (writesize == 0 || buf == 0 || dir == 0 || dirsecoff == 0 || mode == 0 )
	{
		return 0;
	}

	if (mode & FILE_WRITE_APPEND)
	{
		return clusterAppendWriter(dir, buf, writesize, dirsecoff, mode, dirinsec);
	}
	else if (mode & FILE_WRITE_COVER)
	{
		return clusterCoverWriter(dir, buf, writesize, dirsecoff, mode, dirinsec);
	}
	else {
		return 0;
	}
}

int clusterCoverWriter(LPFAT32DIRECTORY dir, char * buf, int writesize,int dirsecoff,int mode,int dirinsec) 
{
	int ret = 0;

	DWORD clusterno = getFirstClusterNo(dir);
	DWORD newclusterno = 0;
	if (dirinsec & 0x80000000)
	{
		newclusterno = getNextFAT32EmptyCluster(clusterno);
	}
	else {
		newclusterno = getNextFAT32Cluster(clusterno);
	}
	if (newclusterno == FAT_EMPTY_CLUSTER || newclusterno == FAT_END_FLAG)
	{
		return FALSE;
	}

	int prevclusterno = 0;
	int lastclusterno = 0;

	int sectorno = 0;

	char * lpdata = buf;

	int writetimes = writesize / g_ClusterSize;
	int writemod = writesize % g_ClusterSize;

	for (int i = 0; i < writetimes; i++)
	{
		sectorno = g_SecsBeforeRootDir + (clusterno - g_FirstClusterNO) * g_SecsPerCluster;
		ret = writeSector(sectorno, 0, g_SecsPerCluster, (char*)lpdata);
		if (ret)
		{
			lpdata += g_ClusterSize;
		}
		else {
			return FALSE;
		}

		glpFAT[clusterno] = newclusterno;
		ret = updateFAT(clusterno);

		gFsInfo.freeClusterCnt = gFsInfo.freeClusterCnt - 1;
		ret = updateFSINFO(gFsInfo.freeClusterCnt, newclusterno);

		prevclusterno = clusterno;

		clusterno = newclusterno;
		if (dirinsec & 0x80000000)
		{
			newclusterno = getNextFAT32EmptyCluster(clusterno);
		}
		else {
			newclusterno = getNextFAT32Cluster(clusterno);
		}
	}

	char *tmpbuf = (char *)__kMalloc(g_ClusterSize * 2);

	if (writemod)
	{
		__memset(tmpbuf, g_ClusterSize, 0);
		sectorno = g_SecsBeforeRootDir + (clusterno - g_FirstClusterNO) * g_SecsPerCluster;
		if (dirinsec & 0x80000000)
		{
			ret = readSector(sectorno, 0, g_SecsPerCluster, (char*)tmpbuf);
		}
		__memmove(tmpbuf, lpdata, writemod);
		ret = writeSector(sectorno, 0, g_SecsPerCluster, (char*)tmpbuf);

		lastclusterno = glpFAT[clusterno];

		glpFAT[clusterno] = FAT_END_FLAG;
		ret = updateFAT(clusterno);

		gFsInfo.freeClusterCnt = gFsInfo.freeClusterCnt - 1;
		ret = updateFSINFO(gFsInfo.freeClusterCnt, newclusterno);
	}
	else {
		lastclusterno = glpFAT[prevclusterno];

		glpFAT[prevclusterno] = FAT_END_FLAG;
		ret = updateFAT(prevclusterno);
	}

	if (mode & FILE_WRITE_RESIZE)
	{
		if (dirinsec & 0x80000000) {
			while (1)
			{
				if (glpFAT[lastclusterno] != FAT_END_FLAG &&glpFAT[lastclusterno] != FAT_EMPTY_CLUSTER)
				{
					DWORD tmpno = glpFAT[lastclusterno];
					glpFAT[lastclusterno] = FAT_EMPTY_CLUSTER;
					lastclusterno = tmpno;
				}
				else {
					break;
				}
			}
		}
	}

	__memset(tmpbuf, 0, g_bytesPerSec);
	ret = readSector(dirsecoff, 0, 1, tmpbuf);
	LPFAT32DIRECTORY lptmpdir = (LPFAT32DIRECTORY)(tmpbuf + (dirinsec & 0x7fffffff));
	__memcpy((char*)lptmpdir, (char*)dir, sizeof(FAT32DIRECTORY));
	ret = writeSector(dirsecoff, 0, 1, tmpbuf);

	__kFree((DWORD)tmpbuf);

	return writesize;
}



int clusterAppendWriter(LPFAT32DIRECTORY dir, char * buf, int writesize,int dirsecoff,int writemode,int dirinsec)
{
	int ret = 0;

	int writecnt = writesize;

	char * lpdata = buf;

	DWORD sectorno = 0;

	char *tmpbuf = (char *)__kMalloc(g_ClusterSize*2);
	__memset(tmpbuf, 0, g_ClusterSize);

	DWORD clusterno = 0;
	if (dirinsec & 0x80000000)
	{
		clusterno = getFirstClusterNo(dir);
	}
	else {
		DWORD lastclusterno = getLastClusterNo(dir);
		if (lastclusterno == FALSE)
		{
			return FALSE;
		}

		int filemodsize = (dir->size) & (g_ClusterSize - 1);
		if (filemodsize)
		{
			sectorno = g_SecsBeforeRootDir + (lastclusterno - g_FirstClusterNO) * g_SecsPerCluster;
			ret = readSector(sectorno, 0, g_SecsPerCluster, tmpbuf);

			int firstwritelen = 0;
			int lastclusterfree = g_ClusterSize - filemodsize;
			if (lastclusterfree >= writesize) {
				firstwritelen = writesize;
			}
			else {
				firstwritelen = lastclusterfree;
			}
			__memcpy(tmpbuf + filemodsize, lpdata, firstwritelen);

			ret = writeSector(sectorno, 0, g_SecsPerCluster, tmpbuf);

			lpdata += firstwritelen;
			writesize -= firstwritelen;
			if (writesize <= 0)
			{
				ret = readSector(dirsecoff, 0, 1, tmpbuf);
				LPFAT32DIRECTORY lptmpdir = (LPFAT32DIRECTORY)(tmpbuf + (dirinsec & 0x7fffffff));
				lptmpdir->size = lptmpdir->size + writecnt;
				ret = writeSector(dirsecoff, 0, 1, tmpbuf);

				__kFree((DWORD)tmpbuf);

				glpFAT[lastclusterno] = FAT_END_FLAG;
				ret = updateFAT(lastclusterno);

				return writecnt;
			}
		}

		clusterno = getFsinfoFreeCluster();
		glpFAT[lastclusterno] = clusterno;
		ret = updateFAT(lastclusterno);
	}

	DWORD prevclusterno = 0;

	DWORD newclusterno = getNextFAT32EmptyCluster(clusterno);

	int writetimes = writesize / g_ClusterSize;
	int writemod = writesize % g_ClusterSize;
	for (int i = 0; i < writetimes; i++)
	{
		sectorno = g_SecsBeforeRootDir + (clusterno - g_FirstClusterNO) * g_SecsPerCluster;
		ret = writeSector(sectorno, 0, g_SecsPerCluster, (char*)lpdata);
		if (ret)
		{
			lpdata += g_ClusterSize;
		}
		else {
			__kFree((DWORD)tmpbuf);
			return FALSE;
		}


		gFsInfo.freeClusterCnt = gFsInfo.freeClusterCnt - 1;
		ret = updateFSINFO(gFsInfo.freeClusterCnt, newclusterno);

		glpFAT[clusterno] = newclusterno;
		ret = updateFAT(clusterno);

		prevclusterno = clusterno;

		clusterno = newclusterno;

		newclusterno = getNextFAT32EmptyCluster( clusterno);
	}


	if (writemod)
	{
		__memset(tmpbuf, g_ClusterSize, 0);
		sectorno = g_SecsBeforeRootDir + (clusterno - g_FirstClusterNO) * g_SecsPerCluster;
		ret = readSector(sectorno, 0, g_SecsPerCluster, tmpbuf);
		__memmove(tmpbuf, lpdata, writemod);
		ret = writeSector(sectorno, 0, g_SecsPerCluster,tmpbuf);

		glpFAT[clusterno] = FAT_END_FLAG;
		ret = updateFAT(clusterno);

		newclusterno = getNextFAT32EmptyCluster(clusterno);
		gFsInfo.freeClusterCnt = gFsInfo.freeClusterCnt - 1;
		ret = updateFSINFO(gFsInfo.freeClusterCnt, newclusterno);
	}
	else {
		glpFAT[prevclusterno] = FAT_END_FLAG;
		ret = updateFAT(prevclusterno);
	}

	if (dirinsec & 0x80000000) {

	}
	else {
		__memset(tmpbuf, 0, g_bytesPerSec);
		ret = readSector(dirsecoff, 0, 1, tmpbuf);
		LPFAT32DIRECTORY lptmpdir = (LPFAT32DIRECTORY)(tmpbuf + (dirinsec & 0x7fffffff));
		lptmpdir->size = lptmpdir->size + writecnt;
		ret = writeSector(dirsecoff, 0, 1, tmpbuf);
	}

	__kFree((DWORD)tmpbuf);
	return writesize;
}


