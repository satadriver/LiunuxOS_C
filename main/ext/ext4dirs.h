#pragma once

#include "def.h"

#include "file.h"

int ReadExt4Dirs(DWORD nodenum, LPFILEBROWSER files);

int BrowseExt4File(LPFILEBROWSER files);

unsigned long Ext4FileReader(DWORD clusterno, int filesize, char* lpdata, int readsize);