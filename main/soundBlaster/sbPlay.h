#pragma once

#include "../def.h"



#define SOUNDBLASTER_BASE_PORT	0X220

#define FORMAT_MONO      00
#define FORMAT_STEREO    0X20
#define FORMAT_SIGNED    0X10
#define FORMAT_UNSIGNED  00


#pragma pack(1)

typedef struct {
	char id[4];
	DWORD size;
	char type[4];
}WAVFILEHEADER;

typedef struct
{
	char id[4];
	DWORD size;					//10h

	WORD wFormatag;				//14h 编码格式，包括WAVE_FORMAT_PCM，WAVEFORMAT_ADPCM等
	WORD nChannls;				//16h 声道数，单声道为1，双声道为2
	DWORD nSamplesPerSec;		//18h 采样频率
	DWORD nAvgBytesperSec;		//1ch 每秒的数据量
	WORD nBlockAlign;			//20h 块对齐
	WORD wBitsPerSample;		//22h WAVE文件的采样大小
} WAVEFORMAT;

#pragma pack()

extern int gWavDataSize;
extern char * gWavDataPtr;
extern WAVEFORMAT * gWavFormat;

#ifdef DLL_EXPORT
extern "C" __declspec(dllexport) int sbplay(char * data, int size);

extern "C" __declspec(dllexport) void __kSoundInterruptionProc();
#else
extern "C" __declspec(dllimport) int sbplay(char * data, int size);

extern "C" __declspec(dllimport) void __kSoundInterruptionProc();
#endif