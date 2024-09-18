#include <windows.h>
#include "sectorRW.h"
#include <stddef.h>
#include <ntddscsi.h>		//SDK里面的头文件
#include <winioctl.h>      //DDK里面的头文件



HANDLE g_hDev = 0;

int unlockDevice(HANDLE hDevice) {
	DWORD Ret = 0;
	char out[1024];
	int ret = DeviceIoControl(hDevice, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &Ret, NULL);
	if (ret == FALSE) {
		wsprintfA(out, "DeviceIoControl error:%u", GetLastError());
		MessageBoxA(0, out, out, MB_OK);
		return FALSE;
	}

	DISK_GEOMETRY dg = { 0 };
	ret = DeviceIoControl(hDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &dg, sizeof(DISK_GEOMETRY), &Ret, NULL);
	if (ret == FALSE) {
		wsprintfA(out, "DeviceIoControl error:%u", GetLastError());
		MessageBoxA(0, out, out, MB_OK);
		return FALSE;
	}

	return TRUE;
}

int lockDevice(HANDLE hDevice) {
	DWORD iRet = 0;
	char out[1024];
	int ret = DeviceIoControl(hDevice, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &iRet, NULL);
	if (ret == FALSE)
	{
		wsprintfA(out, "lock device error:%u", GetLastError());
		MessageBoxA(0, out, out, MB_OK);
	}
	return ret;
}


void SectorReaderWriter::close() {
	int ret = 0;

	ret = lockDevice(g_hDev);
	
	CloseHandle(g_hDev);
	return;
}


int SectorReaderWriter::init() {
	int ret = 0;
	char out[1024];

	g_hDev = CreateFileA(DEVICENAME, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS |
		FILE_ATTRIBUTE_NORMAL |
		FILE_FLAG_WRITE_THROUGH |
		FILE_FLAG_NO_BUFFERING
		//FILE_FLAG_RANDOM_ACCESS | 
		//FILE_FLAG_SEQUENTIAL_SCAN
		, 0);

	if (g_hDev == INVALID_HANDLE_VALUE)
	{
		wsprintfA(out, "open harddisk device error:%u", GetLastError());
		MessageBoxA(0, out, out, MB_OK);
		return FALSE;
	}

	ret = unlockDevice(g_hDev);
	if (ret)
	{
		return TRUE;
	}
	else {
		wsprintfA(out, "unlock harddisk device error:%u", GetLastError());
		MessageBoxA(0, out, out, MB_OK);
		CloseHandle(g_hDev);
		return FALSE;
	}
}



int SectorReaderWriter::sectorWriter(const char *name, unsigned int sectoroffset, char * data) {
	int ret = 0;

	HANDLE hDev = CreateFileA(name, GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS | FILE_ATTRIBUTE_NORMAL, 0);
	if (hDev == INVALID_HANDLE_VALUE)
	{
		ret = GetLastError();
		return FALSE;
	}

	int status = unlockDevice(hDev);
	if (status == FALSE)
	{
		ret = GetLastError();
		CloseHandle(hDev);
		return FALSE;
	}

	LARGE_INTEGER li = { 0 };
	li.QuadPart = sectoroffset * SECTOR_SIZE;
	ret = SetFilePointer(hDev, li.LowPart, &li.HighPart, FILE_BEGIN);
	if (ret == INVALID_SET_FILE_POINTER)
	{
		ret = GetLastError();
		if (ret)
		{
			return FALSE;
		}
	}

	DWORD dwRet = 0;
	ret = WriteFile(hDev, data, SECTOR_SIZE, &dwRet, 0);
	CloseHandle(hDev);
	status = lockDevice(hDev);
	if (ret  && dwRet == SECTOR_SIZE)
	{
		return dwRet;
	}

	return FALSE;
}


int SectorReaderWriter::sectorReader(const char *name, unsigned int sectoroffset, int cnt, char * data) {
	int ret = 0;

	HANDLE hDev = CreateFileA(name, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS | FILE_ATTRIBUTE_NORMAL, 0);
	if (hDev == INVALID_HANDLE_VALUE)
	{
		ret = GetLastError();
		return FALSE;
	}

	LARGE_INTEGER li = { 0 };
	li.QuadPart = sectoroffset * SECTOR_SIZE;
	ret = SetFilePointer(hDev, li.LowPart, &li.HighPart, FILE_BEGIN);
	if (ret == INVALID_SET_FILE_POINTER)
	{
		ret = GetLastError();
		if (ret)
		{
			return FALSE;
		}
	}

	DWORD dwRet = 0;
	ret = ReadFile(hDev, data, SECTOR_SIZE*cnt, &dwRet, 0);
	CloseHandle(hDev);
	if (ret  && dwRet == SECTOR_SIZE)
	{
		return dwRet;
	}

	return FALSE;
}

int SectorReaderWriter::dataReader(unsigned int sectoroffset, int readsize, char * data) {
	int ret = 0;

	LARGE_INTEGER offset = { 0 };
	offset.QuadPart = sectoroffset*SECTOR_SIZE;
	ret = SetFilePointer(g_hDev, offset.LowPart, &offset.HighPart, FILE_BEGIN);
	if (ret == INVALID_SET_FILE_POINTER)
	{
		ret = GetLastError();
		if (ret)
		{
			return FALSE;
		}
	}

	DWORD dwRet = 0;
	int readblocksize = (readsize / SECTOR_SIZE) * SECTOR_SIZE;
	int readmod = readsize % SECTOR_SIZE;
	if (readblocksize) {
		ret = ReadFile(g_hDev, data, readblocksize, &dwRet, 0);
		if (ret <= 0)
		{
			ret = GetLastError();
			return ret;
		}
	}


	if (readmod)
	{
		char *tmpbuf = new char[SECTOR_SIZE];
		memset(tmpbuf, 0, SECTOR_SIZE);
		ret = ReadFile(g_hDev, tmpbuf, SECTOR_SIZE, &dwRet, 0);
		
		if (ret <= 0)
		{
			delete[] tmpbuf;
			return FALSE;
		}
		memcpy(data + readblocksize, tmpbuf, readmod);
		delete[] tmpbuf;
		tmpbuf = 0;
	}

	return readsize;
}

//不能一次性写入超过64kb的数据？
int SectorReaderWriter::dataWriter(unsigned int sectoroffset, int datasize, char * data) {
#ifdef _DEBUG
	return TRUE;
#endif

	int ret = 0;
	int status = 0;

	LARGE_INTEGER offset = { 0 };
	offset.QuadPart = sectoroffset*SECTOR_SIZE;
	ret = SetFilePointer(g_hDev, offset.LowPart, &offset.HighPart, FILE_BEGIN);
	if (ret == INVALID_SET_FILE_POINTER)
	{
		ret = GetLastError();
		if (ret)
		{
			return FALSE;
		}
	}

	DWORD dwRet = 0;
	int writemod = datasize % SECTOR_SIZE;
	int writeblocksize = (datasize / SECTOR_SIZE) * SECTOR_SIZE;
	if (writeblocksize)
	{

		ret = WriteFile(g_hDev, data, writeblocksize, &dwRet, 0);
		if (ret <= 0)
		{
			ret = GetLastError();
			return FALSE;
		}
	}

	if (writemod)
	{
		char * lastsec = new char[SECTOR_SIZE];
		memset(lastsec, 0, SECTOR_SIZE);
		ret = ReadFile(g_hDev, lastsec, SECTOR_SIZE, &dwRet, 0);
		if (ret <= 0)
		{
			delete[] lastsec;
			return FALSE;
		}

		long dwMoveHigh = -1;
		ret = SetFilePointer(g_hDev, -SECTOR_SIZE, &dwMoveHigh, FILE_CURRENT);

		memcpy(lastsec, data + writeblocksize, writemod);
		ret = WriteFile(g_hDev, lastsec, SECTOR_SIZE, &dwRet, 0);
		delete[] lastsec;
		if (ret <= 0)
		{
			return FALSE;
		}
		lastsec = 0;
	}

	return datasize;
}