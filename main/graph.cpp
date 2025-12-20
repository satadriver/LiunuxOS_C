
#include "graph.h"
#include "fat32/Fat32File.h"
#include "video.h"
#include "gdi/jpeg.h"
#include "soundBlaster/sbPlay.h"
#include "gdi.h"
#include "malloc.h"

int showTxtFile(char * filename) {

	int ret = 0;
	char * data = 0;
	int filesize = readFile(filename,(char**)&data);

	*(data + filesize) = 0;

	__drawGraphChars( data, 0);

	__kFree((DWORD)data);

	return TRUE;
}


int showBmpFile(char * filename) {
	int ret = 0;
	unsigned char * data = 0;
	int filesize = readFile(filename,(char**)&data);

	ret = showBmp(filename, (unsigned char *)data, filesize,0,0);
	__kFree((DWORD)data);
	return ret;
}

int playWavFile(char *filename) {
	int ret = 0;
	unsigned char * data = 0;
	int filesize = readFile(filename, (char**)&data);

	ret = sbplay(( char *)data, filesize);
	__kFree((DWORD)data);
	return ret;
}



int showBmp(char * filename, unsigned char * data, int filesize,int x,int y) {
	char szout[256];

	//__printf(szout, "show bmp file:%s\n", filename);

	int ret = 0;
	BITMAPFILEHEADER * bmphdr = (BITMAPFILEHEADER*)data;
	if (bmphdr->bfType != 0x4d42)
	{
		__printf(szout, ( char*)"bmp file format error\n");
		return FALSE;
	}

	BITMAPINFOHEADER * info = (BITMAPINFOHEADER*)(data + sizeof(BITMAPFILEHEADER));

	ret = showBmpBits(x, y, info, data + bmphdr->bfOffBits);

	__printf(szout, "bmp file:%s,size:%d,bit:%d,width:%d,height:%d\n", filename, filesize, info->biBitCount, info->biWidth, info->biHeight);

	return ret;
}


void setPallette(unsigned char * data) {
	for (int i = 0; i < 256; i++ )
	{
		__asm {
			mov dx,3c8h
			mov al,byte ptr i
			out dx,al

			movzx eax,byte ptr data[i*4+2]
			mov ecx,63
			mul ecx
			mov ecx,255
			div ecx
			mov dx, 3c9h
			out dx,al


			movzx eax, byte ptr data[i*4 + 1]
			mov ecx, 63
			mul ecx
			mov ecx, 255
			div ecx
			mov dx, 3c9h
			out dx, al


			movzx eax, byte ptr data[i*4]
			mov ecx, 63
			mul ecx
			mov ecx, 255
			div ecx
			mov dx, 3c9h
			out dx, al
		}
	}
}


int setCursor(unsigned char high8bits, unsigned char low8bits) {
	__asm {
		mov al,0eh
		mov dx,3d4h
		out dx,al

		mov al,byte ptr high8bits
		mov dx,3d5h
		out dx,al

		mov al,0fh
		mov dx,3d4h
		out dx,al

		mov al, byte ptr low8bits
		mov dx,3d5h
		out dx,al
	}

	return TRUE;
}