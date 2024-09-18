
#include "satadriver.h"
#include "Utils.h"
#include "video.h"
#include "atapi.h"

WORD gAtapiPort = 0;



unsigned char gAtapiCmdOpen[] =		{ 0x1b,0,0,0,2,0,0,0,0,0,0,0 };
unsigned char gAtapiCmdClose[] =	{ 0x1b,0,0,0,3,0,0,0,0,0,0,0 };

unsigned char gAtapiCmdRead[12] = { 0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
unsigned char gAtapiCmdWrite[12] = { 0xAA, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };



int checkAtapiPort(WORD port) {
	char gAtapiParams[512];
	waitFree(port);

	__asm {
		mov al, 0a1h
		mov dx,port
		out dx, al

		movzx edx,dx
		push edx
		call waitComplete
		add esp,4
		cmp eax,-1
		jnz _commandOK

		mov eax,-1
		jmp _checkAtapiEnd

		_commandOK:
		mov dx,port
		sub dx,7
		mov gAtapiPort,dx

		lea edi, gAtapiParams

		cld
		mov ecx, BYTES_PER_SECTOR / 4
		rep insd

		mov eax,1

		_checkAtapiEnd:
	}


	unsigned char szshow[0x1000];
	__dump((char*)gAtapiParams, BYTES_PER_SECTOR, 0, szshow);
	__drawGraphChars((unsigned char*)szshow, 0);
	return TRUE;
}

int atapiCmd(unsigned char *cmd) {

	waitFree(gAtapiPort + 7);

	__asm {
		mov dx, gAtapiPort + 1
		mov al, 0		//1 is dma,0 is pio
		out dx, al

		mov dx, gAtapiPort + 6
		mov al, 0xa0
		out dx, al

		mov dx, gAtapiPort + 4
		mov al, 12
		out dx, al

		mov dx, gAtapiPort + 5
		mov al, 0
		out dx, al

		mov dx, gAtapiPort+7
		mov al, 0a0h
		out dx, al

		movzx edx, gAtapiPort
		add edx, 7
		push edx
		call waitComplete
		add esp,4
		cmp eax,-1
		jz _atapi_a0_end

		mov dx, gAtapiPort + 4
		in al, dx
		mov cl, al

		mov dx, gAtapiPort + 5
		in al, dx
		mov ch, al

		mov cx, 6
		mov dx, gAtapiPort
		mov esi, cmd
		cld
		rep outsw
		_atapi_a0_end:
	}
}


int readAtapiSector(char * buf,unsigned int secno,unsigned char seccnt) {

	waitFree(gAtapiPort + 7);

	__asm {
		mov dx, gAtapiPort + 1
		mov al,0
		out dx,al

		mov dx, gAtapiPort + 6
		mov al, 0a0h
		out dx, al

		mov eax, ATAPI_SECTOR_SIZE
		movzx ecx, seccnt
		mul ecx
		mov dx, gAtapiPort + 4
		OUT dx,al
		mov dx, gAtapiPort + 5
		mov al,ah
		OUT dx,al

		mov dx, gAtapiPort + 7
		mov al, 0a0h
		out dx, al

		movzx edx, gAtapiPort
		add edx,7
		push edx
		call waitComplete
		add esp,4
		cmp eax, -1
		jz _atapi_a0_end

		mov dx, gAtapiPort + 4
		in al, dx
		mov cl, al

		mov dx, gAtapiPort + 5
		in al, dx
		mov ch, al

		lea esi, gAtapiCmdRead
		mov al,seccnt
		mov byte ptr [esi + 9], al

		mov eax, secno
		mov byte ptr[esi + 5], al
		mov byte ptr[esi + 4], ah
		shr eax,16
		mov byte ptr[esi + 3], al
		mov byte ptr[esi + 2], ah

		mov ecx, 6
		mov dx, gAtapiPort
		rep outsw

		movzx edx, gAtapiPort
		add edx, 7
		push edx
		call waitComplete
		add esp, 4
		cmp eax, -1
		jz _atapi_a0_end

		movzx edx, gAtapiPort
		mov edi, buf
		movzx ecx,seccnt
		shl ecx,9
		rep insd

		_atapi_a0_end:
	}
}


int getAtapiDev(int disk,int maxno) {

	LPV86VMIPARAMS params = (LPV86VMIPARAMS)V86VMIPARAMS_ADDRESS;

	for (int dev = disk; dev <= maxno; dev ++)
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

/*
AH = 46h
AL = 0 保留
DL = 驱动器号

返回:
CF = 0, AH = 0 成功
CF = 1, AH = 错误码
*/
int reject(int dev) {
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
	__asm {
		mov ah, 46h
		mov al, 0
		mov dl, byte ptr dev
		int 13h
		jc _rejectcdromErr
		mov eax, 1
		jmp _rejectcdromEnd
		_rejectcdromErr :
		mov eax, 0
		_rejectcdromEnd :
	}
}