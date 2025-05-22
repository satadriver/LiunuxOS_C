
#include "ata.h"
#include "Utils.h"
#include "video.h"
#include "atapi.h"
#include "hardware.h"
#include "ata.h"



unsigned char gAtapiCmdRead10[10] = { 0x28,0,0,0,0,0,0,0,0,0 };

unsigned char gAtapiCmdWrite10[10] = { 0x2a,0,0,0,0,0,0,0,0,0 };

unsigned char gAtapiCmdSeek10[10] = { 0x2b,0,0,0,0,0,0,0,0,0 };

unsigned char gAtapiCmdOpen[16] =	{ 0x1b,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0};

unsigned char gAtapiCmdClose[16] =	{ 0x1b,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0 };

unsigned char gAtapiCmdRead[16] = { 0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,0,0,0,0 };

unsigned char gAtapiCmdWrite[16] = {0xAA, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,0,0,0,0 };

//2,3,4,5 is bit31-bit24,bit23-bit16,bit15-bit8,bit7-bit0,6,7,8,9 is sector number, bit31-bit24,bit23-bit16,bit15-bit8,bit7-bit0



int writeAtapiCMD(unsigned short* cmd) {
	//gAtapiPackSize = 12;
	__asm {
		cli
		mov dx, gAtapiBasePort
		movzx ecx, gAtapiPackSize
		shr ecx, 1
		mov esi, cmd
		cld
		//rep outsw
		_sendAtapiCmd:
		lodsw
		out dx,ax
		loop _sendAtapiCmd
		sti
	}
	return 0;
}

//atapi free value is 0x41,not 0x50


int atapiCMD(unsigned short *cmd) {

	waitFree(gAtapiBasePort + 7);

	outportb(gAtapiBasePort + 1, 0);	//dma = 1,pio = 0
	outportb(gAtapiBasePort + 4, gAtapiPackSize );
	outportb(gAtapiBasePort + 5, 0);
	outportb(gAtapiBasePort + 6, gATAPIDev);
	outportb(gAtapiBasePort + 7, 0xa0);

	int res = waitComplete(gAtapiBasePort + 7);

	int low = inportb(gAtapiBasePort + 4);
	int high = inportb(gAtapiBasePort + 5);
	writeAtapiCMD(cmd);

	return 0;
}


int readAtapiSector(char * buf,unsigned int secnum,unsigned int seccnt) {

	waitFree(gAtapiBasePort + 7);

	int readsize = ATAPI_SECTOR_SIZE * seccnt;

	outportb(gAtapiBasePort + 1, 0);	//dma = 1,pio = 0
	outportb(gAtapiBasePort + 4, readsize& 0xff);
	outportb(gAtapiBasePort + 5, (readsize >> 8)&0xff);
	//outportb(gAtapiBasePort + 4, gAtapiPackSize);
	//outportb(gAtapiBasePort + 5, 0);
	
	outportb(gAtapiBasePort + 6, gATAPIDev);
	outportb(gAtapiBasePort + 7, 0xa0);

	int res = waitComplete(gAtapiBasePort + 7);

	int low = inportb(gAtapiBasePort + 4);
	int high = inportb(gAtapiBasePort + 5);
	gAtapiCmdRead[0] = 0xa8;
	gAtapiCmdRead[1] = 0;

	gAtapiCmdRead[6] = (seccnt>>24)&0xff;
	gAtapiCmdRead[7] = (seccnt >> 16) & 0xff;
	gAtapiCmdRead[8] = (seccnt >> 8) & 0xff;
	gAtapiCmdRead[9] = seccnt&0xff;

	gAtapiCmdRead[5] = secnum&0xff;
	gAtapiCmdRead[4] = (secnum>>8) & 0xff;
	gAtapiCmdRead[3] = (secnum >> 16) & 0xff;
	gAtapiCmdRead[2] = (secnum >> 24) & 0xff;
	writeAtapiCMD((unsigned short*)gAtapiCmdRead);

	char* lpbuf = buf;
	for (DWORD i = 0; i < seccnt; i++) {
		int res = waitComplete(gAtapiBasePort + 7);
		res = readsector(gAtapiBasePort, ATAPI_SECTOR_SIZE / 4,lpbuf);
		lpbuf += ATAPI_SECTOR_SIZE;
	}

	return readsize;
}


int writeAtapiSector(char* buf, unsigned int secnum, unsigned int seccnt) {

	waitFree(gAtapiBasePort + 7);

	int readsize = ATAPI_SECTOR_SIZE * seccnt;

	outportb(gAtapiBasePort + 1, 0);	//dma = 1,pio = 0
	outportb(gAtapiBasePort + 4, readsize & 0xff);
	outportb(gAtapiBasePort + 5, (readsize >> 8) & 0xff);
	outportb(gAtapiBasePort + 6, gATAPIDev);
	outportb(gAtapiBasePort + 7, 0xa0);

	int res = waitComplete(gAtapiBasePort + 7);

	int low = inportb(gAtapiBasePort + 4);
	int high = inportb(gAtapiBasePort + 5);

	gAtapiCmdWrite[0] = 0xaa;
	gAtapiCmdWrite[1] = 0;

	gAtapiCmdWrite[6] = (seccnt >> 24) & 0xff;
	gAtapiCmdWrite[7] = (seccnt >> 16) & 0xff;
	gAtapiCmdWrite[8] = (seccnt >> 8) & 0xff;
	gAtapiCmdWrite[9] = seccnt & 0xff;

	gAtapiCmdWrite[5] = secnum & 0xff;
	gAtapiCmdWrite[4] = (secnum >> 8) & 0xff;
	gAtapiCmdWrite[3] = (secnum >> 16) & 0xff;
	gAtapiCmdWrite[2] = (secnum >> 24) & 0xff;
	writeAtapiCMD((unsigned short*)gAtapiCmdWrite);

	char* lpbuf = buf;
	for (DWORD i = 0; i < seccnt; i++) {
		int res = waitComplete(gAtapiBasePort + 7);
		res = writesector(gAtapiBasePort, ATAPI_SECTOR_SIZE / 4, lpbuf);
		lpbuf += ATAPI_SECTOR_SIZE;
	}

	return readsize;
}




