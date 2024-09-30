#include "keyboard.h"
#include "window.h"
#include "hardware.h"
#include "Utils.h"
#include "screenProtect.h"
#include "process.h"
#include "task.h"
#include "device.h"
#include "servicesProc.h"

DWORD gKbdTest = FALSE;

WORD gKeyboardID = 0;

//ctrl alt shift 等已经被过滤，肯定不会出现在当前的字母表中
unsigned char ScanCodesBuf[96] =
{ 0, VK_ESCAPE, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', VK_BACK, VK_TAB, \
'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0x0a, VK_CONTROL, 'a', 's',\
'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', VK_LSHIFT, '\\','z','x','c','v',
'b','n','m',',','.','/', VK_RSHIFT, VK_PRINT, VK_MENU, ' ', VK_CAPSLOCK, VK_F1, VK_F2, VK_F3, VK_F4, VK_F5,\
VK_F6, VK_F7, VK_F8, VK_F9, VK_F10, VK_NUMSLOCK, VK_SCROLLLOCK, VK_HOME, VK_UP, VK_PRIOR, '-', VK_LEFT, '5', VK_RIGHT, '+', VK_END,
VK_DOWN, VK_NEXT, VK_INSERT, VK_DELETE, 0, 0, 0, VK_F11, VK_F12, 0, 0, 0, 0, 0, 0, 0 };



unsigned char shiftScanCodesTransBuf[96]=
{0, VK_ESCAPE, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', VK_BACK, VK_TAB,
'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0x0a, VK_CONTROL, 'A', 'S',\
'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', VK_LSHIFT, '|', 'Z', 'X', 'C', 'V', 
'B', 'N', 'M', '<', '>', '?', VK_RSHIFT, '*', VK_MENU, ' ', VK_CAPSLOCK, VK_F1, VK_F2, VK_F3, VK_F4, VK_F5,\
VK_F6, VK_F7, VK_F8, VK_F9, VK_F10, VK_NUMSLOCK, VK_SCROLLLOCK, '7', '8', '9', '-', '4', '5', '6', '+', '1',
'2', '3', '0', '.', 0, 0, 0, VK_F11, VK_F12, 0, 0, 0, 0, 0, 0, 0 };


int isScancodeAsc(unsigned char c) {
	if ( (c >=  2 && c <= 0x0e) || (c >= 0x10 && c <= 0x1b) || (c >= 0x1e && c <=29) || (c >=0x2b && c <= 0x35) )
	{
		return TRUE;
	}
	return FALSE;
}

unsigned int __getchar(int wid) {

	if (isTopWindow(wid))
	{
		__asm {
			push wid
			mov edi,esp
			mov eax, 1
			int 80h
			add esp,4
		}
 	}
	else {
		return FALSE;
	}
}

int __putchar(char * s) {
	__asm {
		mov eax, 2
		mov edi,s
		int 80h
	}
}

unsigned int __kGetKbd(int wid) {

	if (isTopWindow(wid))
	{

	}
	else {
		return FALSE;
	}

	LPKBDBUFDATA data = (LPKBDBUFDATA)KEYBOARD_BUFFER;
	if (data->kbdBufHdr == data->kbdBufTail)
	{
		return 0;
	}

	unsigned int ch = data->kbdBuf[data->kbdBufTail] & 0xff;
	unsigned int status = data->kbdStatusBuf[data->kbdBufTail];

	data->kbdBufTail ++;
	if (data->kbdBufTail >= KEYBORAD_BUF_SIZE)
	{
		data->kbdBufTail = 0;
	}

	if (status & (SHIFTLEFT_SET_FLAG | SHIFTRIGHT_SET_FLAG))
	{
		ch= shiftScanCodesTransBuf[ch];
	}
	else {
		ch = ScanCodesBuf[ch];
	}

	if (status & CAPSLOCK_SET_FLAG)
	{
		if (ch >= 'a' && ch <= 'z')
		{
			ch -= 0x20;
		}
	}
	
	return ch;
}


unsigned int __kGetKbdAndStatus(int wid,DWORD *status) {

	if (isTopWindow(wid))
	{

	}
	else {
		return FALSE;
	}

	LPKBDBUFDATA data = (LPKBDBUFDATA)KEYBOARD_BUFFER;
	if (data->kbdBufHdr == data->kbdBufTail)
	{
		return 0;
	}

	unsigned int ch = data->kbdBuf[data->kbdBufTail] & 0xff;
	*status = data->kbdStatusBuf[data->kbdBufTail];

	data->kbdBufTail++;
	if (data->kbdBufTail >= KEYBORAD_BUF_SIZE)
	{
		data->kbdBufTail = 0;
	}

	if (*status & (SHIFTLEFT_SET_FLAG | SHIFTRIGHT_SET_FLAG))
	{
		ch = shiftScanCodesTransBuf[ch];
	}
	else {
		ch = ScanCodesBuf[ch];
	}

	if (*status & CAPSLOCK_SET_FLAG)
	{
		if (ch >= 'a' && ch <= 'z')
		{
			ch -= 0x20;
		}
	}

	return ch;
}

void __kPutKbd(unsigned char c,int wid) {
	if (isTopWindow(wid))
	{

	}
	else {
		return;
	}

	LPKBDBUFDATA data = (LPKBDBUFDATA)KEYBOARD_BUFFER;
	data->kbdBuf[data->kbdBufHdr] = c;
	data->kbdStatusBuf[data->kbdBufHdr] = 0;

	data->kbdBufHdr += 1;
	if (data->kbdBufHdr >= KEYBORAD_BUF_SIZE )
	{
		data->kbdBufHdr = 0;
	}
}


unsigned int numslockOn[] = { '7',   '8',  '9',  '-',  '4',  '5', '6',  '+',  '1',  '2',  '3',  '0', '.' };
unsigned int numslockOff[] = { 0x47, 0x48, 0x49, '-',  0x4b, '5', 0x4d, '+',  0x4f, 0x50, 0x51, 0x52, 0x53};

int numsLockProc(unsigned int c) {
	if (c & 0x80)
	{
		return FALSE;
	}

	LPKBDBUFDATA data = (LPKBDBUFDATA)KEYBOARD_BUFFER;

	if (c == 0x45)
	{
		data->kbdStatus = data->kbdStatus ^NUMSLOCK_SET_FLAG;
		data->kbdLedStatus = data->kbdLedStatus ^ 2;
		__kKbdLed(data->kbdLedStatus);
		return c;
	}
	else if (c == 0x37 || c == 0xe01c || c == 0xe035)
	{
		c = c & 0xff;

		data->kbdBuf[data->kbdBufHdr] = c;
		data->kbdStatusBuf[data->kbdBufHdr] = data->kbdStatus;

		data->kbdBufHdr += 1;
		if (data->kbdBufHdr >= KEYBORAD_BUF_SIZE)
		{
			data->kbdBufHdr = 0;
		}
		return c;
	}
	else  if (c >= 0x47 && c <= 0x53)
	{
		if (c == 0x52)
		{
			data->kbdStatus = data->kbdStatus ^ INSERT_SET_FLAT;
			return c;
		}

		int no = c - 0x47;
		if (data->kbdStatus & NUMSLOCK_SET_FLAG)
		{
			c = numslockOn[no];
		}
		else {
			c = numslockOff[no];
		}

		data->kbdBuf[data->kbdBufHdr] = c;
		data->kbdStatusBuf[data->kbdBufHdr] = data->kbdStatus;

		data->kbdBufHdr ++;
		if (data->kbdBufHdr >= KEYBORAD_BUF_SIZE)
		{
			data->kbdBufHdr = 0;
		}
		return c;
	}

	return FALSE;
}

void setkbdStatus(DWORD status) {

}


void __kKeyboardProc() {

	unsigned int c = inportb(0x60);

	char szout[1024];

	unsigned int result = 0;

	LPKBDBUFDATA data = (LPKBDBUFDATA)KEYBOARD_BUFFER;
	if (c == 1 && (data->kbdStatus & CTRLLEFT_SET_FLAG))
	{
		LPPROCESS_INFO tss = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
		//__terminateProcess(tss->pid | 0x80000000,tss->funcname,tss->filename,0);
		return;
	}

	if (c == 0x1d || c == 0x9d)
	{
		data->kbdStatus = data->kbdStatus ^ CTRLLEFT_SET_FLAG;
		return;
	}else if (c == 0x2a || c == 0xaa)
	{
		data->kbdStatus = data->kbdStatus ^ SHIFTLEFT_SET_FLAG;
		return;
	}else if (c == 0x36 || c == 0xb6)
	{
		data->kbdStatus = data->kbdStatus ^ SHIFTRIGHT_SET_FLAG;
		return;
	}
	else if (c == 0x38 || c == 0xb8)
	{
		data->kbdStatus = data->kbdStatus ^ ALTLEFT_SET_FLAG;
		return;
	}

	else if (c == 0x3a )
	{
		data->kbdStatus = data->kbdStatus ^ CAPSLOCK_SET_FLAG;
		data->kbdLedStatus = data->kbdLedStatus ^4;
		__kKbdLed(data->kbdLedStatus);
		return;
	}
	else if (c == 0xba)
	{
		return;
	}

	else if (c == 0x45)
	{
		data->kbdStatus = data->kbdStatus ^ NUMSLOCK_SET_FLAG;
		data->kbdLedStatus = data->kbdLedStatus ^ 2;
		__kKbdLed(data->kbdLedStatus);
		return;
	}
	else if (c == 0xc5)
	{
		return;
	}

	else if (c == 0x46 )
	{
		data->kbdStatus = data->kbdStatus ^ SCROLLLOCK_SET_FLAG;
		data->kbdLedStatus = data->kbdLedStatus ^1;
		__kKbdLed(data->kbdLedStatus);
		return;
	}
	else if (c == 0xc6)
	{
		return;
	}

	else if (c == 0x53 )	//delete
	{
		if ( (data->kbdStatus & (CTRLLEFT_SET_FLAG | ALTLEFT_SET_FLAG)) || 
			(data->kbdStatus & (CTRLRIGHT_SET_FLAG | ALTRIGHT_SET_FLAG))  )
		{
			__reset();
		}
		else {
			//keep it
		}
	}
	else if (c == 0xd3)
	{
		return;
	}

	else if (c == 0x37 || (c >= 0x47 && c <= 0x53))
	{
		result = numsLockProc(c);
		return;
	}

	else if (c == 0xe0)
	{
		c = c << 8;
		c += inportb(0x60);
		if (c == 0xe053)
		{
			if ((data->kbdStatus & (CTRLLEFT_SET_FLAG | ALTLEFT_SET_FLAG)) || (data->kbdStatus & (CTRLRIGHT_SET_FLAG | ALTRIGHT_SET_FLAG)))
			{
				__reset();
			}
			else {
				//keep delete
			}
		}else if (c == 0xe0d3)
		{
			return;
		}
		
		else if (c == 0x0e01d || c == 0x0e09d)
		{
			data->kbdStatus = data->kbdStatus ^ CTRLRIGHT_SET_FLAG;
			return;
		}

		else if (c == 0x0e038 || c == 0x0e0b8)
		{
			data->kbdStatus = data->kbdStatus ^ ALTRIGHT_SET_FLAG;
			return;
		}
		
		else if (c == 0xe052)
		{
			data->kbdStatus = data->kbdStatus ^ INSERT_SET_FLAT;
			return;
		}else if (c == 0xe0d2)
		{
			return;
		}
		
		else if (c == 0xe02a)
		{
			c = c << 8;
			c+= inportb(0x60);
			c = c << 8;
			c += inportb(0x60);

			if (c == 0xe02ae037)
			{
				__kPrintScreen();
				return;
				//printfscreen make code,to keep it
			}
		}
		else if (c == 0xe0b7)
		{
			c = c << 8;
			c += inportb(0x60);
			c = c << 8;
			c += inportb(0x60);

			if (c == 0xe0b7e0aa)
			{
				//printfscreen break code
				return;
			}
		}
		else if (c == 0xe01c || c == 0xe035)
		{
			result = numsLockProc(c);
			return;
		}
		else {
			
		}
	}
	else if (c == 0xe1)
	{
		c = c << 8;
		c += inportb(0x60);
		c = c << 8;
		c+= inportb(0x60);

		if (c == 0xe11d45)
		{
			c = c << 8;
			c += inportb(0x60);

			DWORD hc = 0;
			hc += inportb(0x60);
			hc = hc << 8;
			hc += inportb(0x60);

			char pb[6];
			__asm {
				mov eax,c
				mov dword ptr [pb],eax

				mov eax,hc
				mov word ptr [pb + 4],ax
			}

			if (__memcmp(pb,"\xe1\x1d\x45\xe1\x9d\xc5",6) == 0 )
			{
				//make code is "\xe1\x1d\x45\xe1\x9d\xc5",but no break code!按下后6个字节扫描码，松开后不产生扫描码
				__printf(szout, ( char*)"get Pause/Break key!\r\n");
				pauseBreak();
			}

			return;
		}
	}
	else if (c == 0xe2)
	{
		c = c << 8;
		c += inportb(0x60);
		c = c << 8;
		c += inportb(0x60);
		c = c << 8;
		c += inportb(0x60);
	}

	else if ((c >= 0x3b && c <= 0x44) || c == 0x57 || c == 0x58)
	{
		//f1-f10,f11,f12
	}

	if (c & 0x80)
	{
		return;
	}


// 	char szout[1024];
// 	__printf(szout, "input key:%x,status:%x\n", c, data->kbdStatus);

	data->kbdBuf[data->kbdBufHdr] = c;
	data->kbdStatusBuf[data->kbdBufHdr] = data->kbdStatus;

	data->kbdBufHdr ++;
	if (data->kbdBufHdr >= KEYBORAD_BUF_SIZE)
	{
		data->kbdBufHdr = 0;
	}

	if (gKbdTest)
	{
		kbdtest();
	}
}



void __kKbdLed(unsigned char cmd) {
	__waitPs2In();
	outportb(0x64, 0xad);	//; disable keyboard

	__waitPs2In();
	outportb(0x60, 0xed);		//; send ED command to 8048, not 8042 in cpu bridge

	__waitPs2Out();
	int ack = inportb(0x60);	

	__waitPs2In();
	outportb(0x60, cmd);		//; send command data to 8048

	__waitPs2Out();
	ack = inportb(0x60);

	__waitPs2In();
	outportb(0x64, 0xae);
}


void kbdtest() {
	
	char szout[1024];

	DWORD status = 0;
	unsigned int key = __kGetKbdAndStatus(gKbdTest,&status);
	if (key)
	{
		DWORD pos = ( GRAPHCHAR_HEIGHT * 2) * gVideoWidth * gBytesPerPixel + (gVideoWidth / 2)*gBytesPerPixel;

		__sprintf(szout, "input key:%x,%s,status:%x\n", key,&key,status);
		__drawGraphChar(( char*)szout, 0, pos, TASKBARCOLOR);

		if (key == 0x1b) {
			__kPutKbd(1,gKbdTest);
		}
	}
}




__declspec(naked) void KeyboardIntProc() {
	__asm {
		pushad
		push ds
		push es
		push fs
		push gs
		push ss

		push esp
		sub esp, 4
		push ebp
		mov ebp, esp

		mov ax, KERNEL_MODE_DATA
		mov ds, ax
		mov es, ax
		MOV FS, ax
		MOV GS, AX
	}

	{
		__kKeyboardProc();
		outportb(0x20, 0x20);
	}

	__asm {
		mov dword ptr ds : [SLEEP_TIMER_RECORD] , 0
		mov eax, TURNON_SCREEN
		int 80h

		mov esp, ebp
		pop ebp
		add esp, 4
		pop esp

		pop ss
		pop gs
		pop fs
		pop es
		pop ds
		popad

		iretd
	}
}