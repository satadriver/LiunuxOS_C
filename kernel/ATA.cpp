#include "ata.h"
#include "def.h"
#include "Utils.h"
#include "pci.h"
#include "task.h"
#include "Utils.h"
#include "video.h"
#include "atapi.h"
#include "Kernel.h"
#include "v86.h"
#include "hardware.h"
#include "device.h"

DWORD gATADrv = 0;

DWORD gATAPIDrv = 0;

WORD gAtaBasePort = 0;

DWORD gATADev = 0;

DWORD gAtapiPackSize = 0;

WORD gAtapiBasePort = 0;

DWORD gATAPIDev = 0;

DWORD gMimo = 0;

DWORD gSecMax = ONCE_READ_LIMIT;

int(__cdecl* readSector)(unsigned int secnolow, DWORD secnohigh, unsigned int seccnt, char* buf) = readPortSector;

int(__cdecl* writeSector)(unsigned int secnolow, DWORD secnohigh, unsigned int seccnt, char* buf) = writePortSector;

//在编译器参数里（项目->属性->配置属性->C/C++->命令行->其他选项）
//说明如下:/Gs 设置堆栈检查字节数. 我们使用/Gs8192设置堆栈检查字节为8MB,经测试,对 __chkstk的调用没有了,成功达到我们的目的.

//1f7读出的是0x58，atapi 170读出的是0x41，含义不同
//1f7
//bit0 之前的命令发生错误 if is 1
//bit1 磁盘每转一周等于1
//bit2 ecc check correctly to read sector data if is 1
//bit3 work complete if is 1
//bit4 磁头停留在磁道上 if is 1
//bit5	write error if 1
//bit6 ready to work if is 1
//bit7 be busy if is 1

//1f6
//bit7:1
//bit6:1 is lba,0 is chs
//bit5:1
//bit4:0 is master,1 is slave
//bit0-bit3:if bit6 is 1,the sector no of 24-27,if bit6 is 0,header number

//柱面（cylinder），磁头（head）,sector
//LBA(逻辑扇区号)=磁头数 × 每磁道扇区数 × 当前所在柱面号 + 每磁道扇区数 × 当前所在磁头号 + 当前所在扇区号 – 1
//CHS=0/0/1，则根据公式LBA=255 × 63 × 0 + 63 × 0 + 1 – 1= 0
//CHS模式支持的硬盘 用8bit来存储磁头地址，用10bit来存储柱面地址，用6bit来存储扇区地址，
//而一个扇区共有512Byte，这样使用CHS寻址一块硬盘最大容量为256 * 1024 * 63 * 512B = 8064 MB
int checkIDEPort(unsigned short port) {

	int r = inportb(port + 7);
	if (r == 0x50)
	{	
		gAtaBasePort = port;

		gATADev = inportb(port + 6);

		if ( (gATADev & 0xa0) != 0xa0) {
			//gATADev = gATADev | 0xa0;
		}

		gATADrv = gATADev & 0x10;

		r = identifyDevice(port, 0xec,(char*) ATA_INFO_BASE);
		if (r) {
			unsigned char gc = *(unsigned char*)ATA_INFO_BASE;
			if (gc * 0x80) {
				return 1;
			}
			else {
				return FALSE;
			}
			return 1;
		}
		return 0;
	}
	else if( r == 0x41 || r == 0x40)
	{
		gAtapiBasePort = port;

		gATAPIDev = inportb(port + 6);
		if ( (gATAPIDev & 0xa0) != 0xa0) {
			//gATAPIDev = gATAPIDev | 0xa0;
		}

		gATAPIDrv = gATAPIDev & 0x10;

		r = identifyDevice(port , 0xa1 , (char*)ATAPI_INFO_BASE);
		if (r) {
			WORD gc = *(WORD*)ATAPI_INFO_BASE;
			if ( (gc & 3) == 1) {
				gAtapiPackSize = 16;
			}
			else if ((gc & 3) == 0) {
				gAtapiPackSize = 12;
			}

			return 2;
		}
		
		return FALSE;
	}
	else {
		return FALSE;
	}
}

void resetPort(int port) {
	outportb(0x3f6,4);

	outportb(0x376,4);

	__sleep(0);

	outportb(0x3f6, 0); //IRQ15

	outportb(0x376, 0);	//IRQ14
}

int checkIDEMimo(unsigned int addr) {

	char* p = (char*)addr;
	if (p[0] == 0x50) {
		return TRUE;
	}
	return FALSE;
}

int getIDEPort() {

	unsigned char szshow[1024];

	int ret = 0;

	gSecMax = ONCE_READ_LIMIT;
	readSector = readPortSector;
	writeSector = writePortSector;

	ret = checkIDEPort(0x3f0);

	__printf((char*)szshow, "getIDEPort 3f0 over\n");

	ret = checkIDEPort(0x370);

	__printf((char*)szshow, "getIDEPort 370 over\n");

	//1f7 = 3f6 = 3f7,376=377=177
	ret = checkIDEPort(0x1f0);

	__printf((char*)szshow, "getIDEPort 1f0 over\n");

	ret = checkIDEPort(0x170);
	__printf((char*)szshow, "getIDEPort 170 over\n");

	ret = checkIDEPort(0x168);

	ret = checkIDEPort(0x1e8);

	DWORD hdport[1024];
	DWORD dev = 0;
	DWORD irq = 0;
	int cnt = getPciDevBasePort(hdport, 0x0101, &dev, &irq);
	for (int i = 0; i < cnt; i++)
	{
		if (hdport[i])
		{
			__printf((char*)szshow, "sata port:%x\n", hdport[i]);

			if ((hdport[i] & 1) == 0)
			{
				gMimo = 1;

				ret = checkIDEMimo((hdport[i] & 0xffffFFF0));
				if (ret == 1)
				{
					gAtaBasePort = hdport[i] & 0xFFFffff0;
				}
				else if (ret == 2) {
					gAtapiBasePort = hdport[i] & 0xFFFffff0;
				}
			}
			else {
				ret = checkIDEPort((hdport[i] & 0xFFF0));
				if (ret == 1)
				{
					gAtaBasePort = hdport[i] & 0xFFF0;
					gATADev = inportb(gAtaBasePort + 6);
				}
				else if (ret == 2) {
					gAtapiBasePort = hdport[i] & 0xFFF0;
					gATAPIDev = inportb(gAtapiBasePort + 6);
				}
			}
		}
	}

	if (gAtaBasePort ) {
		__printf((char*)szshow, "ide master port:%x,device:%x,slave port:%x,device:%x\n", gAtaBasePort, gATADev, gAtapiBasePort, gATAPIDev);
	}
	else {
		gATAPIDev = 0xa0;
		gATADev = 0xe0;
		readSector = vm86ReadSector;
		writeSector = vm86WriteSector;
		__printf((char*)szshow, "int13 emulate ide read write sector\n");
	}

	return TRUE;
}

//376端口控制说明:
//读端口时跟1f6一致，一般为50h
//写入时得低4位:
//bit0: always be 0
//bit1:1关中断，0开中断
//bit2:1 reset复位磁盘,0不复位磁盘
//bit3:always be 0
//bit4-bit7:读取端口得值跟1f7读取得高4位一致

//bit1	nIEN	Set this to stop the current device from sending interrupts.
//bit2	SRST	Set, then clear(after 5us), this to do a "Software Reset" on all ATA drives on a bus, if one is misbehaving.
int __initIDE() {

	resetPort(0x3f6);	//IRQ15

	resetPort(0x376);	//IRQ14

	//resetPort(0x3e6);

	//resetPort(0x366);

	int r = getIDEPort();

	return r;
}


int readPortSector(unsigned int secno, DWORD secnohigh, unsigned int seccnt, char* buf) {
	int readcnt = seccnt / gSecMax;
	int readmod = seccnt % gSecMax;
	int ret = 0;
	CHAR* offset = buf;
	for (int i = 0; i < readcnt; i++)
	{
		//ret = readSectorLBA48Multiple(secno, secnohigh, gSecMax, offset);
		ret = readSectorLBA48(secno, secnohigh, gSecMax, offset);
		offset += (BYTES_PER_SECTOR * gSecMax);
		secno += gSecMax;
	}

	if (readmod)
	{
		//ret = readSectorLBA48Multiple(secno, secnohigh, gSecMax, offset);
		ret = readSectorLBA48(secno, secnohigh, readmod, offset);
	}
	return ret;
}


int writePortSector(unsigned int secno, DWORD secnohigh, unsigned int seccnt, char* buf) {
	int readcnt = seccnt / gSecMax;
	int readmod = seccnt % gSecMax;
	int ret = 0;
	CHAR* offset = buf;
	for (int i = 0; i < readcnt; i++)
	{
		ret = writeSectorLBA48(secno, secnohigh, gSecMax, offset);
		offset += BYTES_PER_SECTOR * gSecMax;
		secno += gSecMax;
	}

	if (readmod)
	{
		ret = writeSectorLBA48(secno, secnohigh, readmod, offset);
	}
	return ret;
}



int waitComplete(WORD port) {

	//waitInterval(0);
	//delay();

	int r = 0;
	//r = inportb(port - 6);
	//if (r == 0) 
	{
		int cnt = 64;
		while (cnt--) {
			r = inportb(port);
			//if (r & 1) {
			//	return FALSE;
			//}
			//else 
			if ((r & 0x88) == 8) {		//0xe9
				return TRUE;
			}
			else {
				char szout[1024];
				if ( (r & 0x80) == 0) 
				{
					__printf(szout, "waitComplete:%x,port:%x\r\n",r,port);
				}
				//delay();
				__sleep(0);
				//waitInterval0(1);
				continue;
			}
		}
	}
	return FALSE;
}

void waitFree(WORD port) {
	int cnt = 16;
	while (cnt--)
	{
		int r = inportb(port);
		if (r & 0x80) {
			char szout[1024];
			//__printf(szout, "waitFree:%x\r\n",r);
			__sleep(0);
			//waitInterval0(1);
			//delay();
			continue;
		}
		else {
			break;
		}
	}
}

void waitReady(WORD port) {
	int cnt = 16;
	while (cnt --)
	{
		int r = inportb(port);
		if (r & 0x40) {
			break;
		}
		else {
			char szout[1024];
			//__printf(szout, "waitReady:%x\r\n", r);
			__sleep(0);
			//waitInterval0(1);
			//delay();
			continue;
		}
	}
}



int writesector(int port,int len,char* buf) {
	__asm {
		cld
		mov esi, buf
		mov ecx, len
		mov edx, port
		rep outsd
	}
}

int readsector(int port,int len, char * buf) {
	__asm {
		cli

		cld
		mov edi,buf
		mov ecx, len
		mov edx, port
		rep insd

		sti
	}
}

int readSectorLBA24(unsigned int secno, unsigned char seccnt, char* buf) {

	waitFree(gAtaBasePort + 7);

	outportb(gAtaBasePort + 1, 0);	//dma = 1,pio = 0

	outportb(gAtaBasePort + 2, seccnt & 0xff);

	outportb(gAtaBasePort + 3, secno & 0xff);
	outportb(gAtaBasePort + 4, (secno>>8) & 0xff);
	outportb(gAtaBasePort + 5,( secno>>16) & 0xff);
	outportb(gAtaBasePort + 6, ((secno >> 24) & 0x0f) + gATADev);

	waitReady(gAtaBasePort + 7);
	outportb(gAtaBasePort + 7, HD_READ_COMMAND);

	char* lpbuf = buf;
	for (int i = 0; i < seccnt; i++) {
		int res = waitComplete(gAtaBasePort + 7);
		readsector(gAtaBasePort, BYTES_PER_SECTOR / 4, lpbuf);
		lpbuf += BYTES_PER_SECTOR;
	}

	return seccnt * BYTES_PER_SECTOR;
}



int writeSectorLBA24(unsigned int secno, unsigned char seccnt, char* buf) {

	waitFree(gAtaBasePort + 7);

	outportb(gAtaBasePort + 1, 0);	//dma = 1,pio = 0

	outportb(gAtaBasePort + 2, seccnt & 0xff);

	outportb(gAtaBasePort + 3, secno & 0xff);
	outportb(gAtaBasePort + 4, (secno >> 8) & 0xff);
	outportb(gAtaBasePort + 5, (secno >> 16) & 0xff);
	outportb(gAtaBasePort + 6, ((secno >> 24) & 0x0f) + gATADev);

	waitReady(gAtaBasePort + 7);
	outportb(gAtaBasePort + 7, HD_WRITE_COMMAND);

	char* lpbuf = buf;
	for (int i = 0; i < seccnt; i++) {
		int res = waitComplete(gAtaBasePort + 7);
		writesector(gAtaBasePort, BYTES_PER_SECTOR / 4, lpbuf);
		lpbuf += BYTES_PER_SECTOR;
	}

	return seccnt * BYTES_PER_SECTOR;
}


//most 6 bytes sector no
int readSectorLBA48(unsigned int secnoLow, unsigned int secnoHigh, unsigned char seccnt, char* buf) {

	waitFree(gAtaBasePort + 7);

	outportb(gAtaBasePort + 1, 0);	//dma = 1,pio = 0
	outportb(gAtaBasePort + 1, 0);	//dma = 1,pio = 0

	outportb(gAtaBasePort + 2, 0);	//172,first high byte of sector counter,then low byte of counter
	
	outportb(gAtaBasePort + 5, (secnoHigh >> 8) & 0xff);
	outportb(gAtaBasePort + 4, secnoHigh & 0xff);
	outportb(gAtaBasePort + 3, (secnoLow>>24) & 0xff);

	outportb(gAtaBasePort + 2, seccnt & 0xff);

	outportb(gAtaBasePort + 5, (secnoLow >> 16) & 0xff);
	outportb(gAtaBasePort + 4, (secnoLow >> 8) & 0xff);
	outportb(gAtaBasePort + 3, secnoLow & 0xff);
	
	outportb(gAtaBasePort + 6, 0x40 | gATADrv);

	waitReady(gAtaBasePort + 7);

	outportb(gAtaBasePort + 7, HD_LBA48READ_COMMAND);

	char* lpbuf = buf;
	for (int i = 0; i < seccnt; i++) {
		int res = waitComplete(gAtaBasePort + 7);
		readsector(gAtaBasePort, BYTES_PER_SECTOR / 4, lpbuf);
		lpbuf += BYTES_PER_SECTOR;
	}

	return seccnt * BYTES_PER_SECTOR;
}

int readSectorLBA48Multiple(unsigned int secnoLow, unsigned int secnoHigh, unsigned char seccnt, char* buf) {

	waitFree(gAtaBasePort + 7);

	outportb(gAtaBasePort + 1, 0);	//dma = 1,pio = 0
	outportb(gAtaBasePort + 1, 0);	//dma = 1,pio = 0

	outportb(gAtaBasePort + 2, 0);	//172,first high byte of sector counter,then low byte of counter


	outportb(gAtaBasePort + 5, (secnoHigh >> 8) & 0xff);
	outportb(gAtaBasePort + 4, secnoHigh & 0xff);
	outportb(gAtaBasePort + 3, (secnoLow >> 24) & 0xff);

	outportb(gAtaBasePort + 2, seccnt & 0xff);

	outportb(gAtaBasePort + 5, (secnoLow >> 16) & 0xff);
	outportb(gAtaBasePort + 4, (secnoLow >> 8) & 0xff);
	outportb(gAtaBasePort + 3, secnoLow & 0xff);

	outportb(gAtaBasePort + 6, gATADev);

	waitReady(gAtaBasePort + 7);

	outportb(gAtaBasePort + 7, 0xc4);

	char* lpbuf = buf;
	int res = waitComplete(gAtaBasePort + 7);
	for (int i = 0; i < seccnt; i++) {	
		readsector(gAtaBasePort, BYTES_PER_SECTOR / 4, lpbuf);
		lpbuf += BYTES_PER_SECTOR;
	}

	return seccnt * BYTES_PER_SECTOR;
}

int writeSectorLBA48(unsigned int secnoLow, unsigned int secnoHigh, unsigned char seccnt, char* buf) {

	waitFree(gAtaBasePort + 7);

	outportb(gAtaBasePort + 1, 0);	//dma = 1,pio = 0
	outportb(gAtaBasePort + 1, 0);	//dma = 1,pio = 0

	outportb(gAtaBasePort + 2, 0);	//172,first high byte of sector counter,then low byte of counter
	
	outportb(gAtaBasePort + 5, (secnoHigh >> 8) & 0xff);
	outportb(gAtaBasePort + 4, secnoHigh & 0xff);
	outportb(gAtaBasePort + 3, (secnoLow >> 24) & 0xff);

	outportb(gAtaBasePort + 2, seccnt & 0xff);

	outportb(gAtaBasePort + 5, (secnoLow >> 16) & 0xff);
	outportb(gAtaBasePort + 4, (secnoLow >> 8) & 0xff);
	outportb(gAtaBasePort + 3, secnoLow & 0xff);

	outportb(gAtaBasePort + 6, 0x40 | gATADrv);

	waitReady(gAtaBasePort + 7);
	outportb(gAtaBasePort + 7, HD_LBA48WRITE_COMMAND);

	char* lpbuf = buf;
	for (int i = 0; i < seccnt; i++) {
		int res = waitComplete(gAtaBasePort + 7);
		writesector(gAtaBasePort, BYTES_PER_SECTOR / 4, lpbuf);
		lpbuf += BYTES_PER_SECTOR;
	}

	return seccnt * BYTES_PER_SECTOR;
}


//bit0:0==memmory address,1== io address
//bit1:size larger than 1MB
//bit2:0 == 32bits address,1 == 64 bits address
//bit4:1== prefetch,0==false
int readSectorLBA24Mimo(unsigned int secno, unsigned char seccnt, char* buf) {
	return 0;
}

int readSectorLBA48Mimo(unsigned int secnoLow, unsigned int secnoHigh, unsigned char seccnt, char* buf) {
	return 0;
}

int identifyDevice(int port,int cmd,char * buffer) {	// IDENTIFY PACKET DEVICE – A1h and  IDENTIFY  DEVICE – ECh

	char szout[0x1000];

	waitFree(port + 7);

	outportb(port + 1, 0);	//dma = 1,pio = 0
	outportb(port + 2, 0);
	outportb(port + 3, 0);
	outportb(port + 4, 0);
	outportb(port + 5, 0);
	outportb(port + 6, 0);

	waitReady(port + 7);

	outportb(port + 7, cmd);

	int res = waitComplete(port + 7);
	if (res) {
		readsector(port, BYTES_PER_SECTOR / 4, buffer);

		unsigned char szshow[0x1000];
		__dump((char*)buffer, BYTES_PER_SECTOR, 0, szshow);
		__printf(szout,( char*)szshow);
	}
	else {
		__printf(szout, "%s waitComplete result:%d, cmd:%x,port:%x ERROR\r\n", __FUNCTION__,res,cmd, port);
	}
	
	//__printf(szout, "harddisk sequence:%s,firmware version:%s,type:%s,type sequence:%s\r\n",
	//	(char*)HARDDISK_INFO_BASE + 20, (char*)HARDDISK_INFO_BASE + 46, (char*)HARDDISK_INFO_BASE + 54, (char*)HARDDISK_INFO_BASE + 176 * 2);

	return res;
}


int readDmaSectorLBA48(unsigned int secnoLow, unsigned int secnoHigh, unsigned char seccnt, char* buf, int device) {
	return 0;
}
