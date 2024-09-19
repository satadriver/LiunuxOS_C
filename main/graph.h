#pragma once

#include "def.h"
#include "video.h"
#include "Utils.h"


int showTxtFile(char * filename);

int setCursor(unsigned char high8bits, unsigned char low8bits);

#ifdef DLL_EXPORT

extern "C" int showBmpFile(char * filename);

extern "C" int showBmp(char * filename, unsigned char * data, int filesize,int x,int y);

extern "C" int playWavFile(char *filename);

#else

extern "C" int showBmpFile(char * filename);

extern "C" int showBmp(char * filename, unsigned char * data, int filesize, int x, int y);

extern "C" int playWavFile(char *filename);

#endif



