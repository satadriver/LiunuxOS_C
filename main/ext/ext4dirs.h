#pragma once

#include "def.h"

#include "file.h"

int ReadExtentTree();

int ReadInodes(unsigned int* addr, int cnt, char* buf, DWORD total);

int ReadExt4Dirs(DWORD nodenum, LPFILEBROWSER files);

int BrowseExt4RootDir(LPFILEBROWSER files);

unsigned long Ext4FileReader(DWORD clusterno, int* filesize, char** lpdata);