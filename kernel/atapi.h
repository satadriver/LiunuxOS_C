#pragma once

#include "def.h"



int checkAtapiPort(WORD port);

int writeAtapiCMD(unsigned short* cmd);






#ifdef DLL_EXPORT
extern "C" __declspec(dllexport) int atapiCMD(unsigned short* cmd);
extern "C" __declspec(dllexport) int writeAtapiSector(char* buf, unsigned int secnum, unsigned char seccnt);

extern "C" __declspec(dllexport) int readAtapiSector(char* buf, unsigned int secno, unsigned char seccnt);
#else
extern "C" __declspec(dllimport) int atapiCMD(unsigned short* cmd);
extern "C" __declspec(dllimport) int writeAtapiSector(char* buf, unsigned int secnum, unsigned char seccnt);

extern "C" __declspec(dllimport) int readAtapiSector(char* buf, unsigned int secno, unsigned char seccnt);

#endif
