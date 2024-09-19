#pragma once

#include "../FileBrowser.h"


int getFat32RootDirs(LPFAT32DIRECTORY dir, LPFILEBROWSER files);

int getFat32NextDirs(DWORD secno, LPFILEBROWSER files);

int fat32FileReader(DWORD clusterno, int filesize, char * lpdata, int readsize);