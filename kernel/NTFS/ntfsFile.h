#pragma once

#include "ntfs.h"

#ifdef DLL_EXPORT
// extern "C" __declspec(dllexport) DWORD getIdxNextDir(char * filename, char * buf);
// extern "C" __declspec(dllexport) DWORD getRootNextDir(LPCommonAttributeHeader hdr, char * filename);
// extern "C" __declspec(dllexport) DWORD getNtfsDir(DWORD secoff, char * filename);
extern "C" __declspec(dllexport) unsigned long long getNtfsFileData(unsigned long long secoff, char** buf);

int readNtfsFile(const char* filename, char** buf);

int writeNtfsFile(const char* filename, char* buf, int size, int writemode);


#else
int readNtfsFile(const char* filename, char** buf);

int writeNtfsFile(const char* filename, char* buf, int size, int writemode);

// extern "C" __declspec(dllimport) DWORD getIdxNextDir(char * filename, char * buf);
// extern "C" __declspec(dllimport) DWORD getRootNextDir(LPCommonAttributeHeader hdr, char * filename);
// extern "C" __declspec(dllimport) DWORD getNtfsDir(DWORD secoff, char * filename);
extern "C" __declspec(dllimport) unsigned long long getNtfsFileData(unsigned long long secoff, char** buf);
#endif

