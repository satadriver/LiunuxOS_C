#pragma once

#include "../def.h"
#include "FAT32.h"


LPFAT32DIRECTORY getDirFromFileName(char * name, LPFAT32DIRECTORY dir);

int getFAT32FileName(char * filename, char * dst);

int clusterReader(LPFAT32DIRECTORY dir, char * buf, int readsize);

int clusterWriter(LPFAT32DIRECTORY dir, char * buf, int writesize, int dirsecoff, int mode, int dirinsec);

int clusterCoverWriter(LPFAT32DIRECTORY dir, char * buf, int writesize, int dirsecoff, int mode, int dirinsec);

int clusterAppendWriter(LPFAT32DIRECTORY dir, char * buf, int writesize, int dirsecoff, int mode, int dirinsec);

int getUnicodeFN(LPFAT32DIRECTORY dir, char * filename);

int setUnicodeFN(LPFAT32DIRECTORY lpdir, char * filename);