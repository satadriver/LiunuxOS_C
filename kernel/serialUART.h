#pragma once

#include "task.h"

#define COM1PORT	0x3f8   
#define COM2PORT	0x2f8
#define COM3PORT	0x3e8
#define COM4PORT	0x2e8

#define COM5PORT	0x5F8
#define COM6PORT	0x4F8
#define COM7PORT	0x5E8
#define COM8PORT	0x4E8


#define COM_BUFFER_SIZE		1024



void initSerialUART(int port);

void initSerial();


#ifdef DLL_EXPORT
extern "C"  __declspec(dllexport) int sendUARTData(unsigned char* data, int size, int port);

extern "C"  __declspec(dllexport) void __kCom1Proc(LIGHT_ENVIRONMENT * stack);

extern "C"  __declspec(dllexport) void __kCom2Proc(LIGHT_ENVIRONMENT * stack);

extern "C"  __declspec(dllexport) int getCom1Data(unsigned char * data);

extern "C"  __declspec(dllexport) int getCom2Data(unsigned char * data);

#else
extern "C"  __declspec(dllimport) int sendUARTData(unsigned char* data, int size, int port);

extern "C"  __declspec(dllimport) void __kCom1Proc(LIGHT_ENVIRONMENT * stack);

extern "C"  __declspec(dllimport) void __kCom2Proc(LIGHT_ENVIRONMENT * stack);

extern "C"  __declspec(dllimport) int getCom1Data(unsigned char * data);

extern "C"  __declspec(dllimport) int getCom2Data(unsigned char * data);

#endif