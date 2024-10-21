#include "ios9660.h"
#include "ata.h"
#include "Utils.h"
#include "atapi.h"
#include "../FileBrowser.h"
#include "file.h"
#include "../guiHelper.h"
#include "def.h"
#include "malloc.h"
#include "VM86.h"
#include "atapi.h"

#ifndef SINGLE_TASK_TSS
#define APAPI_INT13_READWRITE
#endif


#ifdef APAPI_INT13_READWRITE
int gAtapiDev = -1;
#endif


int readIso9660Dirs(DWORD secno, LPFILEBROWSER files) {
	int cnt = 0;
	int iret = 0;
	char buf[ATAPI_SECTOR_SIZE * 2];
	char szout[1024];

#ifndef APAPI_INT13_READWRITE
	iret = readAtapiSector(buf, secno, 1);
#else
	iret = v86Int13Read(secno, 0, 1, buf, gAtapiDev, ATAPI_SECTOR_SIZE);
#endif
	if (iret <= 0)
	{
		__printf(szout,( char*)"readIso9660Dirs iso9660 file system read sector error\n");
		return FALSE;
	}

	int dirno = 0;
	LPISO9660FSDIR dir = (LPISO9660FSDIR)buf;
	while (1)
	{
		if (dir->len < ISO9660FS_LEAST_DIR_SIZE)
		{
			break;
		}

		if (dir->flag == 0)
		{
			dir->flag |= FILE_ATTRIBUTE_ARCHIVE;
		}
		else if (dir->flag == 2)
		{
			dir->flag |= FILE_ATTRIBUTE_DIRECTORY;
		}

		if (dirno == 0) {
			__strcpy(files->pathname, ".");
		}
		else if (dirno == 1)
		{
			__strcpy(files->pathname, "..");
		}
		else if (dir->filenamelen > 1)
		{
			__memcpy(files->pathname, &dir->filename, dir->filenamelen);
			*(files->pathname + dir->filenamelen) = 0;
			if (beEndWith(files->pathname, ";1"))
			{
				*(files->pathname + __strlen(files->pathname) - 2) = 0;
			}
		}

		files->attrib = dir->flag;
		files->filesize = dir->datasize;
		files->secno = dir->lba;

		files++;

		cnt++;

		dirno++;

		dir = (LPISO9660FSDIR)((char*)dir + dir->len);
	}

	return cnt;
}

int browseISO9660File(LPFILEBROWSER files) {
	int iret = 0;
	char szout[1024];
#ifdef APAPI_INT13_READWRITE
	if (gAtapiDev == -1)
	{
		gAtapiDev = getAtapiDev(0x81,0xff);
		if (gAtapiDev == -1)
		{
			__printf(szout,( char*)"not found atapi device\n");
			return FALSE;
		}
		else {
			__printf(szout, "find atapi device:%x\n", gAtapiDev);
		}
	}
#endif

	char buf[ATAPI_SECTOR_SIZE * 2];
#ifndef APAPI_INT13_READWRITE
	iret = readAtapiSector(buf, ISO9660FS_VOLUME_DESCRIPTOR_NO, 1);
#else
	iret = v86Int13Read(ISO9660FS_VOLUME_DESCRIPTOR_NO, 0, 1, buf, gAtapiDev, ATAPI_SECTOR_SIZE);
#endif
	if (iret <= 0)
	{
		__printf(szout,( char*)"browseISO9660File read cdrom ISO9660FS_VOLUME_DESCRIPTOR_NO error\n");
		return FALSE;
	}

	if (*buf != 1 || __memcmp(buf + 1, "CD001", 5) /*|| *(buf+6) != 1*/)
	{
		__printf(szout,( char*)"iso 9660 file system format error\n");
		return FALSE;
	}

	ISO9660FSDIR vterminate;
	__memcpy((char*)&vterminate, buf + 0x9c, *(buf + 0x9c));
#ifndef APAPI_INT13_READWRITE
	iret = readAtapiSector(buf, vterminate.lba, 1);
#else
	iret = v86Int13Read(vterminate.lba, 0, 1, buf, gAtapiDev, ATAPI_SECTOR_SIZE);
#endif
	if (iret <= 0)
	{
		__printf(szout,( char*)"iso 9660 file system read sector error\n");
		return FALSE;
	}

	int cnt = 0;

	int dirno = 0;
	LPISO9660FSDIR dir = (LPISO9660FSDIR)buf;
	while (1)
	{
		if ((DWORD)dir - (DWORD)buf >= vterminate.datasize || dir->len < ISO9660FS_LEAST_DIR_SIZE)
		{
			break;
		}

		if (dir->flag == 0)
		{
			dir->flag |= FILE_ATTRIBUTE_ARCHIVE;
		}
		else if (dir->flag == 2)
		{
			dir->flag |= FILE_ATTRIBUTE_DIRECTORY;
		}

		if (dirno == 0) {
			__strcpy(files->pathname, ".");
		}else if (dirno == 1)
		{
			__strcpy(files->pathname, "..");
		}
		else if (dir->filenamelen > 1)
		{
			__memcpy(files->pathname, &dir->filename, dir->filenamelen);
			*(files->pathname + dir->filenamelen) = 0;
			if (beEndWith(files->pathname, ";1"))
			{
				*(files->pathname + __strlen(files->pathname) - 2) = 0;
			}
		}

		files->attrib = dir->flag;
		files->filesize = dir->datasize;
		files->secno = dir->lba;

		files++;

		cnt++;

		dirno++;

		dir = (LPISO9660FSDIR)((char*)dir + dir->len);
	}

	return cnt;
}

int readIso9660File(DWORD secno,DWORD seccnt, char ** buf) {

	int iret = 0;

	if (buf == 0)
	{
		return FALSE;
	}

	if (*buf == 0)
	{
		*buf = (char*)__kMalloc(0x4000);
	}
	int times = seccnt / 32;
	int mod = seccnt % 32;
	for (int i = 0; i < times; i ++)
	{
#ifndef APAPI_INT13_READWRITE
		iret = readAtapiSector(*buf, secno, 32);
#else
		iret = v86Int13Read(secno, 0, 32, *buf, gAtapiDev, ATAPI_SECTOR_SIZE);
#endif
		if (iret <= 0)
		{
			return FALSE;
		}

		*buf += 0x10000;
		secno += 32;
	}

	if (mod)
	{
#ifndef APAPI_INT13_READWRITE
		iret = readAtapiSector(*buf, secno, mod);
#else
		iret = v86Int13Read(secno, 0, mod, *buf, gAtapiDev, ATAPI_SECTOR_SIZE);
#endif
		if (iret <= 0)
		{
			return FALSE;
		}
	}

	return seccnt*ATAPI_SECTOR_SIZE;
}