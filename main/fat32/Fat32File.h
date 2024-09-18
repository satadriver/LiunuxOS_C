#pragma once

#include "../def.h"

#include "FAT32Utils.h"
#include "FAT32.h"
#include "fileutils.h"


#ifdef DLL_EXPORT
//低2为标识打开文件的模式,00代表读文件，01写文件，10写目录
DWORD openFile(const char* fn, int mode, LPFAT32DIRECTORY dir,int *dirinsec);

DWORD openFileWrite(char * curpath, char * leastpath, int mode, int clusternum,LPFAT32DIRECTORY lpdirectory,LPFAT32DIRECTORY outdir,int *dirinsec);

int readFat32File(char * filename,char ** buf);

int writeFat32File(char * filename, char * buf,int size,int writemode);

int writeFat32Dir(char * filename);
#else
extern "C" __declspec(dllimport) DWORD openFile(const char* fn, int mode, LPFAT32DIRECTORY dir, int *dirinsec);

extern "C" __declspec(dllimport) DWORD openFileWrite(char * curpath, char * leastpath, int mode, int clusternumber, LPFAT32DIRECTORY lpdirectory,
	LPFAT32DIRECTORY outdir, int *dirinsec);

extern "C" __declspec(dllimport) int readFat32File(char * filename, char ** buf);

extern "C" __declspec(dllimport) int writeFat32File(char * filename, char * buf, int size, int writemode);

extern "C" __declspec(dllimport) int writeFat32Dir(char * filename);
#endif


