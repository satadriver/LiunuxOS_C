#pragma once
#include "def.h"

//#define LIUNUX_DEBUG_FLAG

#define HD_READ_COMMAND			0X20
#define HD_WRITE_COMMAND		0X30

#define HD_LBA48READ_COMMAND	0X24
#define HD_LBA48WRITE_COMMAND	0X34

#define HD_DMAREAD_COMMAND		0XC8
#define HD_DMAWRITE_COMMAND		0XCA

//multiple read command can read more than one sector with instruction rep insw/insb/insd
#define HD_MUTIPLEREAD_COMMAND		0X29
#define HD_MUTIPLEWRITE_COMMAND		0X39

#define DEVICE_MASTER			0XE0
#define DEVICE_SLAVE			0XF0

#define ONCE_READ_LIMIT			128


extern DWORD gATADrv ;

extern DWORD gATAPIDrv ;

extern DWORD gAtapiPackSize;

extern DWORD gATADev ;

extern DWORD gATAPIDev ;

extern WORD gAtaBasePort;

extern WORD gAtapiBasePort;




int writesector(int port, int size, char* buf);

int readsector(int port, int size, char* buf);

int identifyDevice(int port,int cmd,char * buffer);

int readSectorLBA48(unsigned int secnoLow, unsigned int secnoHigh, unsigned char seccnt, char* buf);

int writeSectorLBA48(unsigned int secnoLow, unsigned int secnoHigh, unsigned char seccnt, char* buf);

int readSectorLBA24(unsigned int secno, unsigned char seccnt, char* buf);

int writeSectorLBA24(unsigned int secno, unsigned char seccnt, char* buf);

int readSectorLBA24Mimo(unsigned int secno, unsigned char seccnt, char* buf);

int readSectorLBA48Mimo(unsigned int secnoLow, unsigned int secnoHigh, unsigned char seccnt, char* buf);

int readSectorLBA48Multiple(unsigned int secnoLow, unsigned int secnoHigh, unsigned char seccnt, char* buf);

void waitFree(WORD port);

void waitReady(WORD port);

int waitComplete(WORD port);

int checkIDEPort(unsigned short port);

int checkIDEMimo(unsigned int addr);

int __initIDE();

int getIDEPort();

int readPortSector(unsigned int secno, DWORD secnohigh, unsigned int seccnt, char* buf);
int writePortSector(unsigned int secno, DWORD secnohigh, unsigned int seccnt, char* buf);

void resetPort(int port);

#ifdef DLL_EXPORT

extern "C"  __declspec(dllexport)  int getIdeSeq(char* buf);

extern "C"  __declspec(dllexport)  int getIdeFirmVersion(char* buf);

extern "C"  __declspec(dllexport)  int getIdeType(char* buf);

extern "C"  __declspec(dllexport)  int getIdeMediumSeq(char* buf);

extern "C"  __declspec(dllexport)  int(__cdecl * readSector)(unsigned int secnolow, DWORD secnohigh, unsigned int seccnt, char* buf);
extern "C"  __declspec(dllexport)  int(__cdecl * writeSector)(unsigned int secnolow, DWORD secnohigh, unsigned int seccnt, char* buf);
#else
extern "C"  __declspec(dllimport)  int getIdeSeq(char* buf);

extern "C"  __declspec(dllimport)  int getIdeFirmVersion(char* buf);

extern "C"  __declspec(dllimport)  int getIdeType(char* buf);

extern "C"  __declspec(dllimport)  int getIdeMediumSeq(char* buf);


extern "C"  __declspec(dllimport)  int(__cdecl * readSector)(unsigned int secnolow, DWORD secnohigh, unsigned int seccnt, char* buf);
extern "C"  __declspec(dllimport)  int(__cdecl * writeSector)(unsigned int secnolow, DWORD secnohigh, unsigned int seccnt, char* buf);
#endif







