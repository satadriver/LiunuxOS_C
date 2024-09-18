#pragma once

#include "def.h"
#include "video.h"
#include "Utils.h"

#define RT_CURSOR           MAKEINTRESOURCE(1)
#define RT_BITMAP           MAKEINTRESOURCE(2)
#define RT_ICON             MAKEINTRESOURCE(3)
#define RT_MENU             MAKEINTRESOURCE(4)
#define RT_DIALOG           MAKEINTRESOURCE(5)
#define RT_STRING           MAKEINTRESOURCE(6)
#define RT_FONTDIR          MAKEINTRESOURCE(7)
#define RT_FONT             MAKEINTRESOURCE(8)
#define RT_ACCELERATOR      MAKEINTRESOURCE(9)
#define RT_RCDATA           MAKEINTRESOURCE(10)
#define RT_MESSAGETABLE     MAKEINTRESOURCE(11)

#pragma pack(1)
typedef struct tagBITMAPFILEHEADER
{
	UINT16 bfType;
	DWORD bfSize;			//整个BMP文件的大小
	UINT16 bfReserved1;
	UINT16 bfReserved2;
	DWORD bfOffBits;		//偏移数，即 位图文件头+位图信息头+调色板 的大小
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
	DWORD biSize;			//指定此结构体的长度，为40
	LONG biWidth;			//位图宽
	LONG biHeight;			//位图高
	WORD biPlanes;			//平面数，为1
	WORD biBitCount;		//采用颜色位数，可以是1，2，4，8，16，24，新的可以是32
	DWORD biCompression;	//压缩方式，可以是0，1，2，其中0表示不压缩
	DWORD biSizeImage;		//实际位图数据占用的字节数
	LONG biXPelsPerMeter;	//X方向分辨率
	LONG biYPelsPerMeter;	//Y方向分辨率
	DWORD biClrUsed;		//使用的颜色数，如果为0，则表示默认值(2^颜色位数)
	DWORD biClrImportant;	//重要颜色数，如果为0，则表示所有颜色都是重要的
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;

typedef struct tagRGBQUAD {
	BYTE    rgbBlue;
	BYTE    rgbGreen;
	BYTE    rgbRed;
	BYTE    rgbReserved;
} RGBQUAD;


typedef struct {
	short fix;
	short type;
	short cnt;
}ICON_HDR;

typedef struct {
	unsigned char	bwidth;
	unsigned char	bheight;
	unsigned char	bcolorcnt;
	unsigned char	breserved;
	WORD			wplanes;
	WORD			wbitcnt;
	DWORD			dwbytesinres;
	DWORD			dwimgoffset;
}ICON_DIR_ENTRY;

typedef struct {
	BITMAPINFOHEADER	bmphdr;
	RGBQUAD				quad;
	unsigned char		icxor;
	unsigned char		icand;
}ICONIMAGE;

#pragma pack()

extern "C" __declspec(dllexport) DWORD showIcon(int x, int y, LPBITMAPINFOHEADER lpbmpinfo);

int showTxtFile(char * filename);

int setCursor(unsigned char high8bits, unsigned char low8bits);

void sphere(int x, int y, int raduis, DWORD color, unsigned char* backup);

void sphere7(int x, int y, int raduis, DWORD color, unsigned char* backup);

extern "C" __declspec(dllexport) int displayCCPoem();

int initCCFont();


#ifdef DLL_EXPORT



extern "C" __declspec(dllexport) int showBmpBits(int x, int y, BITMAPINFOHEADER* info, unsigned char * data);

extern "C" int showBmpFile(char * filename);

extern "C" int showBmp(char * filename, unsigned char * data, int filesize,int x,int y);

extern "C" int playWavFile(char *filename);

#else


extern "C" __declspec(dllimport) int showBmpBits(int x, int y, BITMAPINFOHEADER* info, unsigned char * data);

extern "C" int showBmpFile(char * filename);

extern "C" int showBmp(char * filename, unsigned char * data, int filesize, int x, int y);


extern "C" int playWavFile(char *filename);

#endif



