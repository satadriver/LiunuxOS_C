#pragma once

#include "def.h"
#include "video.h"
#include "file.h"

#pragma pack(1)



#pragma pack()

#define FILE_DIR_FONT_COLOR			0X7F3F00
#define FILE_FILE_FONT_COLOR		0X9F9F
#define FILE_UNKNOWN_FONT_COLOR		0

int doFileAction(int gPartitionType,LPFILEBROWSER files);

int readFileData(int gPartitionType,DWORD secno, int filesize, char * databuf, int readsize);

int getPartitionInfo();

int readFileDirs(int gPartitionType,DWORD secno, LPFILEBROWSER files,DWORD ntfsseq);


extern "C" __declspec(dllexport) int __kFileManager(unsigned int retaddr, int tid,char * filename, char * funcname, DWORD param);

