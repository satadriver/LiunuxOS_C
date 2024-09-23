
#include "FAT32.h"
#include "fat32file.h"
#include "fileutils.h"
#include "../Utils.h"
#include "../ata.h"
#include "../video.h"
#include "FAT32Utils.h"
#include "../FileBrowser.h"
#include "../malloc.h"


int getFat32RootDirs(LPFAT32DIRECTORY dir,LPFILEBROWSER files) {
	int cnt = 0;
	
	for (int i = 0;i < gMaxDirsInPath; i ++)
	{
		//ignore unicode filenames
		if ( *dir->mainName && dir->attr != 0x0f )
		{

			if (*dir->suffixName != 0x20)
			{
				__memcpy(files->pathname, (char*)dir->mainName, 8);
				*(files->pathname + 8) = 0;
				__strcat(files->pathname, ".");

				__memcpy(files->pathname + 9, (char*)dir->suffixName, 3);
				*(files->pathname + 12) = 0;
			}
			else {
				__memcpy(files->pathname, (char*)dir->mainName, 11);
				*(files->pathname + 11) = 0x20;	//make name length all 12 bytes
				*(files->pathname + 12) = 0;
			}

			files->secno = dir->clusterLow + ((DWORD)dir->clusterHigh << 16);
			files->attrib = dir->attr;
			if (dir->mainName[0] == 0xe5)
			{
				files->attrib |= 0x80;
			}
			files->filesize = dir->size;

			files++;

			cnt++;
		}

		dir++;
	}

	return cnt;
}



int getFat32NextDirs(DWORD clusterno, LPFILEBROWSER files) {
	int ret = 0;
	char *tmpbuf = (char*)__kMalloc(g_ClusterSize);

	DWORD secoff = (clusterno - g_FirstClusterNO)*g_SecsPerCluster + g_SecsBeforeRootDir;
	ret = readSector(secoff, 0, g_SecsPerCluster, (char*)tmpbuf);
	if (ret > 0)
	{
		ret = getFat32RootDirs((LPFAT32DIRECTORY)tmpbuf, files);	
	}
	__kFree((DWORD)tmpbuf);
	return ret;
}


int fat32FileReader(DWORD clusterno, int filesize,char * lpdata, int readsize) {

	char szout[1024];
	int ret = 0;
	if (readsize > filesize)
	{
		readsize = filesize;
	}
	else if (readsize <= 0)
	{
		return FALSE;
	}

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
		ret = readSector(sectorno, 0, g_SecsPerCluster, (char*)lpdata);
		if (ret == g_ClusterSize)
		{
			lpdata += g_ClusterSize;
		}
		else {
			__printf(szout,( char*)"fat32 read cluster error\n");
			break;
		}

		clusterno = getNextFAT32Cluster(clusterno);
		if (clusterno == FAT_END_FLAG || clusterno == FAT_EMPTY_CLUSTER)
		{
			break;
		}
	}

	return readsize;
}