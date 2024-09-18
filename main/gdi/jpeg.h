#pragma once

#include "../def.h"

int testjpeg();

extern "C" __declspec(dllexport) BOOL LoadJpegFile(char * hJpegBuf, int JpegBufSize, char * bmpfiledata, int * bmpdatasize);