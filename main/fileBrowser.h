#pragma once

#include "def.h"
#include "video.h"
#include "file.h"

#pragma pack(1)



#pragma pack()

#define FILE_DIR_FONT_COLOR			0X7F3F00
#define FILE_FILE_FONT_COLOR		0
#define FILE_UNKNOWN_FONT_COLOR		0xff0000

int doOpenFile(int partitionType,LPFILEBROWSER files);

int readFileData(int partitionType, unsigned __int64 secno, unsigned __int64 filesize, char * databuf, unsigned __int64 readsize);

int getPartitionInfo();

int readFileDirs(int partitionType, unsigned __int64 secno, LPFILEBROWSER files, unsigned __int64 ntfsseq);


extern "C" __declspec(dllexport) int __kFileManager(unsigned int retaddr, int tid,char * filename, char * funcname, DWORD param);

