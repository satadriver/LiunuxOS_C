#pragma once

#include "def.h"
#include "video.h"
#include "file.h"

#pragma pack(1)



#pragma pack()

#define FILE_DIR_FONT_COLOR			0X7F3F00
#define FILE_FILE_FONT_COLOR		0X9F9F
#define FILE_UNKNOWN_FONT_COLOR		0

int doFileAction(int partitionType,LPFILEBROWSER files);

int readFileData(int partitionType, unsigned __int64 secno, unsigned __int64 filesize, char * databuf, unsigned __int64 readsize);

int getPartitionInfo();

int readFileDirs(int partitionType, unsigned __int64 secno, LPFILEBROWSER files,DWORD ntfsseq);


extern "C" __declspec(dllexport) int __kFileManager(unsigned int retaddr, int tid,char * filename, char * funcname, DWORD param);

