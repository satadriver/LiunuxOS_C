#include "v86.h"
#include "def.h"

#include "Utils.h"
#include "video.h"
#include "ata.h"
#include "task.h"
#include "atapi.h"
#include "core.h"
#include "textMode.h"


//#define V86_INT13_MODE
//#define V86_INT255_MODE


int v86Int13Read(unsigned int secno, DWORD secnohigh, unsigned short seccnt, char* buf, int disk, int sectorsize) {

	unsigned int counter = 0;
	char szout[1024];
	LPV86VMIPARAMS params = (LPV86VMIPARAMS)V86VMIPARAMS_ADDRESS;
	while (params->bwork == 1)
	{
		__sleep(0);
		counter++;
		if (counter && (counter % VM_OUTPUT_BUSY_CONSTANT == 0))
		{
			__printf(szout, ( char*)"wait bwork to be free\n");
		}
	}

	params->intno = 0x13;
	params->reax = 0x4200;
	params->recx = 0;
	params->redx = disk;
	params->rebx = 0;
	params->resi = V86VMIDATA_OFFSET;
	params->redi = 0;
	params->res = 0;
	params->rds = V86VMIDATA_SEG;
	params->result = 0;

	LPINT13PAT pat = (LPINT13PAT)V86VMIDATA_ADDRESS;
	pat->len = 0x10;
	pat->reserved = 0;
	pat->seccnt = seccnt;
	pat->segoff = (INT13_RM_FILEBUF_SEG << 16) + INT13_RM_FILEBUF_OFFSET;
	pat->secnolow = secno;
	pat->secnohigh = secnohigh;

	params->bwork = 1;

	counter = 0;
	while (params->bwork == 1)
	{
		__sleep(0);
		counter++;
		if (counter && (counter % VM_OUTPUT_BUSY_CONSTANT == 0))
		{
			//__drawGraphChars((unsigned char*)"wait v86 code to clear bwork\n", 0);
		}
	}

	if (params->result > 0)
	{
		//__drawGraphChars((unsigned char*)"vm read sector ok\n", 0);

		__memcpy(buf, (char*)INT13_RM_FILEBUF_ADDR, seccnt * sectorsize);
		return seccnt * sectorsize;
	}
	else {
		__printf(szout, ( char*)"vm read sector error\n");
	}

	return 0;
}


int v86Int13Write(unsigned int secno, DWORD secnohigh, unsigned short seccnt, char* buf, int disk, int sectorsize) {

	LPV86VMIPARAMS params = (LPV86VMIPARAMS)V86VMIPARAMS_ADDRESS;
	while (params->bwork == 1)
	{
		__sleep(0);
	}

	params->intno = 0x13;
	params->reax = 0x4300;
	params->recx = 0;
	params->redx = disk;
	params->rebx = 0;
	params->resi = V86VMIDATA_OFFSET;
	params->redi = 0;
	params->res = 0;
	params->rds = V86VMIDATA_SEG;
	params->result = 0;

	__memcpy((char*)INT13_RM_FILEBUF_ADDR, buf, seccnt * sectorsize);

	LPINT13PAT pat = (LPINT13PAT)V86VMIDATA_ADDRESS;
	pat->len = 0x10;
	pat->reserved = 0;
	pat->seccnt = seccnt;
	pat->segoff = (INT13_RM_FILEBUF_SEG << 16) + INT13_RM_FILEBUF_OFFSET;
	pat->secnolow = secno;
	pat->secnohigh = secnohigh;

	params->bwork = 1;

	while (params->bwork == 1)
	{
		__sleep(0);
	}

	if (params->result)
	{
		return seccnt * sectorsize;
	}
	return 0;
}

int vm86ReadSector(unsigned int secno, DWORD secnohigh, unsigned int seccnt, char* buf) {

	int readcnt = seccnt / ONCE_READ_LIMIT;
	int readmod = seccnt % ONCE_READ_LIMIT;
	int ret = 0;
	CHAR* offset = buf;
	for (int i = 0; i < readcnt; i++)
	{
#ifdef V86_INT255_MODE
		ret = v86Int255Read(secno, secnohigh, ONCE_READ_LIMIT, offset, 0x80, BYTES_PER_SECTOR);
#else
		ret = v86Int13Read(secno, secnohigh, ONCE_READ_LIMIT, offset, 0x80, BYTES_PER_SECTOR);
#endif
		
		offset += (BYTES_PER_SECTOR * ONCE_READ_LIMIT);
		secno += ONCE_READ_LIMIT;
	}

	if (readmod)
	{
#ifdef V86_INT255_MODE
		ret = v86Int255Read(secno, secnohigh, readmod, offset, 0x80, BYTES_PER_SECTOR);
#else
		ret = v86Int13Read(secno, secnohigh, readmod, offset, 0x80, BYTES_PER_SECTOR);
#endif
	}
	return ret;
}


int vm86WriteSector(unsigned int secno, DWORD secnohigh, unsigned int seccnt, char* buf) {

	int readcnt = seccnt / ONCE_READ_LIMIT;
	int readmod = seccnt % ONCE_READ_LIMIT;
	int ret = 0;
	CHAR* offset = buf;
	for (int i = 0; i < readcnt; i++)
	{
#ifdef V86_INT255_MODE
		ret = v86Int255Write(secno, secnohigh, ONCE_READ_LIMIT, offset, 0x80, BYTES_PER_SECTOR);
#else
		ret = v86Int13Write(secno, secnohigh, ONCE_READ_LIMIT, offset, 0x80, BYTES_PER_SECTOR);
#endif

		offset += BYTES_PER_SECTOR * ONCE_READ_LIMIT;
		secno += ONCE_READ_LIMIT;
	}

	if (readmod)
	{
#ifdef V86_INT255_MODE
		ret = v86Int255Write(secno, secnohigh, readmod, offset, 0x80, BYTES_PER_SECTOR);
#else
		ret = v86Int13Write(secno, secnohigh, readmod, offset, 0x80, BYTES_PER_SECTOR);
#endif
	}
	return ret;
}





int v86Int255Read(unsigned int secnum, DWORD secnumHigh,unsigned int seccnt,char *buf,int disk,int secsize) {

	V86_INT_PARAMETER * params = (V86_INT_PARAMETER*)V86_INT_ADDRESS;
	params->intnum = 0x13;
	params->reax = 0x4200;
	params->recx = 0;
	params->redx = disk;
	params->rebx = 0;
	params->resi = V86VMIDATA_OFFSET;
	params->redi = 0;
	params->res = 0;
	params->rds = V86VMIDATA_SEG;
	params->result = 0;

	LPINT13PAT pat = (LPINT13PAT)V86VMIDATA_ADDRESS;
	pat->len = 0x10;
	pat->reserved = 0;
	pat->seccnt = seccnt;
	pat->segoff = (INT13_RM_FILEBUF_SEG << 16) + INT13_RM_FILEBUF_OFFSET;
	pat->secnolow = secnum;
	pat->secnohigh = secnumHigh;

	__asm {
		int 255
	}

	return params->result;
}



int v86Int255Write(unsigned int secnum, DWORD secnumhigh, unsigned short seccnt, char* buf, int disk, int sectorsize) {

	V86_INT_PARAMETER* params = (V86_INT_PARAMETER*)V86_INT_ADDRESS;
	params->intnum = 0x13;
	params->reax = 0x4300;
	params->recx = 0;
	params->redx = disk;
	params->rebx = 0;
	params->resi = V86VMIDATA_OFFSET;
	params->redi = 0;
	params->res = 0;
	params->rds = V86VMIDATA_SEG;
	params->result = 0;

	__memcpy((char*)INT13_RM_FILEBUF_ADDR, buf, seccnt * sectorsize);

	LPINT13PAT pat = (LPINT13PAT)V86VMIDATA_ADDRESS;
	pat->len = 0x10;
	pat->reserved = 0;
	pat->seccnt = seccnt;
	pat->segoff = (INT13_RM_FILEBUF_SEG << 16) + INT13_RM_FILEBUF_OFFSET;
	pat->secnolow = secnum;
	pat->secnohigh = secnumhigh;

	if (params->result)
	{
		return seccnt * sectorsize;
	}
	return 0;
}


int v86Process(int reax, int recx, int redx, int rebx, int resi, int redi, int rds, int res, int cmd) {

	do {
		TssDescriptor* lptssd = (TssDescriptor*)(GDT_BASE + kTssV86Selector);
		TSS* tss = (TSS*)V86_TSS_BASE;
		if ((lptssd->type & 2) || (tss->link)) {
			__sleep(0);
			break;
		}
		else {
			break;
		}
	} while (TRUE);

	V86_INT_PARAMETER* param = (V86_INT_PARAMETER*)V86_INT_ADDRESS;
	param->reax = reax;
	param->recx = recx;
	param->redx = redx;
	param->rebx = rebx;
	param->resi = resi;
	param->redi = redi;
	param->rds = rds;
	param->res = res;
	param->intnum = cmd;

	__asm {
		int 255
	}

	return param->result;
}



int getVideoMode(VesaSimpleInfo vsi[64] ) {

	int res = 0;
	int idx = 0;

	char szout[1024];

	res = v86Process(0x4f00, 0, 0, 0, 0, VESA_STATE_OFFSET, 0, VESA_STATE_SEG ,0x10 );
	if ((res & 0xffff) == 0x4f) {
		VESAINFOBLOCK* vib = (VESAINFOBLOCK*)VESA_STATE_ADDRESS;

		WORD * addr = (WORD*)(vib->mode_dos_offset + (vib->mode_dos_seg << 4));

		__sprintf(szout, "VESAINFOBLOCK address:%x\r\n", addr);
		outputStr(szout, OUTPUT_TEXTMODE_COLOR);

		WORD mode = *addr;
		while (mode != 0 && mode != 0xffff) {

			res = v86Process(0x4f01, mode, 0, 0, 0, VESA_STATE_OFFSET + 0x100, 0, VESA_STATE_SEG, 0x10);
			if ((res & 0xffff) == 0x4f)
			{
				VESAINFORMATION * vi = (VESAINFORMATION*)(VESA_STATE_OFFSET + 0x100 + (VESA_STATE_SEG << 4));
				if (vi->ModeAttr & 0x80) {
					if (vi->BitsPerPixel >= 24) {
						if (vi->XRes >= 800 && vi->YRes >= 600) {

							vsi[idx].mode = mode;
							vsi[idx].x = vi->XRes;
							vsi[idx].y = vi->YRes;
							vsi[idx].bpp = vi->BitsPerPixel / 8;

							vsi[idx].base = vi->PhyBasePtr;
							vsi[idx].offset = vi->OffScreenMemOffset;
							vsi[idx].size = vi->OffScreenMemSize;

							idx++;
						}
					}
				}
			}
			addr++;
			mode = *addr;
		} 
	}
	
	return idx;
}


/*
-----------------------------------------------------------
				功能0x00：返回VBE信息
------------------------------------------------------
入口：
	AX			0x4F00
	ES：DI		指向VBE信息块的指针
出口：
	AX			VBE返回值
------------------------------------------------------------

-----------------------------------------------------------
			功能0x01：返回VBE特定模式信息
------------------------------------------------------
入口：
	AX			0x4F01
	CX			模式号
	ES：DI		指向VBE特定模式信息块的指针
出口：
	AX			VBE返回值
------------------------------------------------------------

-----------------------------------------------------------
			功能0x02：设置VESA VBE 模式
------------------------------------------------------
入口：
	AX			0x4F02
	BX			模式号
出口：
	AX			VBE返回值
------------------------------------------------------------
当设置模式失败时，返回错误代码，一般返回AH=01H

VESA 2.0以上增加了BX中D14，D15的位定义，完整定义如下：
BX = 模式号
	D0～D8：9位模式号
	D9～D13：保留，必须为0
	D14 = 0：使用普通的窗口页面缓存模式，用VBE功能05H切换显示页面
		= 1：使用大的线性缓存区，其地址可从VBE功能01H的返回信息ModeInfo获得
	D15 = 0：清除显示内存
		= 1：不清除显示内存
------------------------------------------------------------
*/




void saveScreen() {

	LPV86VMIPARAMS params = (LPV86VMIPARAMS)V86VMIPARAMS_ADDRESS;

	while (params->bwork == 1)
	{
		__sleep(0);
	}

	params->intno = 0x10;
	params->reax = 0x4f04;
	//cx:
	//d0:hardware
	//d1:bios
	//d2:dac
	//d3:register
	params->recx = 1;
	params->redx = 1;
	params->rebx = VESA_STATE_OFFSET;
	params->resi = 0;
	params->redi = 0;
	params->res = VESA_STATE_SEG;
	params->rds = 0;
	params->result = 0;

	params->bwork = 1;

	while (params->bwork == 1)
	{
		__sleep(0);
	}
}


void restoreScreen() {
	LPV86VMIPARAMS params = (LPV86VMIPARAMS)V86VMIPARAMS_ADDRESS;
	while (params->bwork == 1)
	{
		__sleep(0);
	}

	params->intno = 0x10;
	params->reax = 0x4f04;
	params->recx = 1;
	params->redx = 2;
	params->rebx = VESA_STATE_OFFSET;
	params->resi = 0;
	params->redi = 0;
	params->res = VESA_STATE_SEG;
	params->rds = 0;
	params->result = 0;

	params->bwork = 1;

	while (params->bwork == 1)
	{
		__sleep(0);
	}
}

int setVideoMode(int mode) {

	LPV86VMIPARAMS params = (LPV86VMIPARAMS)V86VMIPARAMS_ADDRESS;
	while (params->bwork == 1)
	{
		__sleep(0);
	}

	params->intno = 0x10;
	params->reax = 0x4f02;
	params->recx = 0;
	params->redx = 0;
	params->rebx = mode|0x4000;
	params->resi = 0;
	params->redi = 0;
	params->res = 0;
	params->rds = 0;
	params->result = 0;

	params->bwork = 1;

	while (params->bwork == 1)
	{
		__sleep(0);
	}
	if (params->result)
	{
		return FALSE;
	}

	return TRUE;
}





int getAtapiDev(int disk, int maxno) {

	LPV86VMIPARAMS params = (LPV86VMIPARAMS)V86VMIPARAMS_ADDRESS;

	for (int dev = disk; dev <= maxno; dev++)
	{
		while (params->bwork == 1)
		{
			__sleep(0);
		}

		params->intno = 0x13;
		params->reax = 0x4100;
		params->recx = 0;
		params->redx = dev;
		params->rebx = 0x55aa;
		params->resi = 0;
		params->redi = 0;
		params->res = 0;
		params->rds = 0;
		params->result = 0;

		params->bwork = 1;

		while (params->bwork == 1)
		{
			__sleep(0);
		}

		if (params->result)
		{
			return dev;
		}
	}

	return -1;
}





int rejectAtapi(int dev) {
	int res = v86Process(0x4600, 0, dev, 0, 0, 0, 0, 0, 0x13); //jc error
	return res;
}


//弹出可移动驱动器中的介质
//入口 :
//AH = 46h
//AL = 0 保留
//DL = 驱动器号
//int 13h
//返回 :
//CF = 0，AH = 0 成功
//CF = 1，AH = 错误码
int rejectCDROM(int dev) {
	return 0;

	if (dev <= 0)
	{
		dev = getAtapiDev(0x81, 0xff);
		if (dev <= 0)
		{
			return FALSE;
		}
		rejectAtapi(dev);
	}

	LPV86VMIPARAMS params = (LPV86VMIPARAMS)V86VMIPARAMS_ADDRESS;
	while (params->bwork == 1)
	{
		__sleep(0);
	}

	params->intno = 0x13;
	params->reax = 0x4600;
	params->recx = 0;
	params->redx = dev;
	params->rebx = 0;
	params->resi = 0;
	params->redi = 0;
	params->res = 0;
	params->rds = 0;
	params->result = 0;

	params->bwork = 1;

	while (params->bwork == 1)
	{
		__sleep(0);
	}
	if (params->result)
	{
		return dev;
	}

	return FALSE;
}
