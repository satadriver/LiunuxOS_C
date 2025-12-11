#include "mouse.h"
#include "window.h"
#include "def.h"
#include "hardware.h"
#include "video.h"
#include "Utils.h"
#include "servicesProc.h"
#include "keyboard.h"
#include "hardware.h"
#include "servicesProc.h"
#include "screenProtect.h"
#include "device.h"


 DWORD gMouseTest = 1;

DWORD gMouseID = 0;

DWORD gMouseColor =	MOUSE_SHOW_COLOR;

#define MOUSE_FACTOR_SIZE	30

//1. many mouse command has no response 0xfa
//2. mouse packet process?


void mousetest() {

	char szout[1024];
	MOUSEINFO mouseinfo;
	__memset((char*)&mouseinfo, 0, sizeof(MOUSEINFO));
	unsigned int ret = GetMousePacket(&mouseinfo);
	if (ret)
	{
		DWORD pos = (gVideoHeight - GRAPHCHAR_HEIGHT*2) * gVideoWidth * gBytesPerPixel + (gVideoWidth/2)*gBytesPerPixel;

		int id = *(DWORD*)0xFEE00020 >> 24;

		__sprintf(szout, (char*)"cpu:%d,mouse X:%x,mouse Y:%x,status:%x        ", id,mouseinfo.x&0xff,mouseinfo.y&0xff,mouseinfo.status);
		__drawGraphChar(( char*)szout, 0, pos, TASKBARCOLOR);
	}
}

int getmouse(LPMOUSEINFO lpinfo,int wid) {

	__asm {
		push wid
		push lpinfo
		mov edi, esp
		mov eax, 3
		int 80h
		add esp,8
	}
}


void restoreMouse() {
	__asm {
		mov eax, RESTORE_MOUSE
		int 80h
	}
}


void drawMouse() {
	__asm {
		mov eax, DRAW_MOUSE
		int 80h
	}
}

void invalidMouse() {
	LPMOUSEDATA data = (LPMOUSEDATA)MOUSE_BUFFER;
	data->bInvalid = TRUE;
}



void __kMouseProc() {
	char szout[256];
	LPMOUSEDATA data = (LPMOUSEDATA)MOUSE_BUFFER;
	int * pos = (int*)&data->mintrData.status;
	int counter = 0;
	while (TRUE)
	{
		int status = inportb(0x64);
		if ((status & 1) == 0)
		{
			break;
		}

		if ((status & 0xe0) ) {
			//__printf(szout, (char*)"mouse data error\r\n");
			//return;
		}

		int md = inportb(0x60);
		*pos = md;
		pos++;

		counter++;
		if (gMouseID ==3 || gMouseID==4)
		{
			if (counter >= 4)
			{
				break;
			}
			else {
				continue;
			}
		}
		if (gMouseID ==0x81)
		{
			if (counter >= 5)
			{
				break;
			}
			else {
				continue;
			}
		}
		else {
			if (counter >= 3)
			{
				break;
			}
			else {
				continue;
			}
		}
	}

	if (gMouseID == 3 || gMouseID == 4)
	{
		if (counter != 4)
		{
			//return;
		}
	}
	else {
		if (counter != 3)
		{
			//return;
		}
	}
	if (counter < 3) {
		//return;
	}
	
	//https://wiki.osdev.org/PS/2_Mouse
	int state = data->mintrData.status;

	data->mintrData.x = data->mintrData.x - ((state << 4) & 0x100);

	data->mintrData.y = data->mintrData.y - ((state << 3) & 0x100);

	data->mintrData.y = -data->mintrData.y;

	if (data->mintrData.status & 0x20) {	//y = 1
		//data->mintrData.y = -data->mintrData.y;
	}
	else{
		
	}

	if (data->mintrData.status & 0x10) {	//x = 1
		//data->mintrData.x =  - data->mintrData.x;
	}
	else {

	}

	if ((data->mintrData.status & 0x80) == 0 && (data->mintrData.status & 0x40) == 0) {

		if (data->mintrData.x || data->mintrData.y)
		{

			__kRestoreMouse();
			
			data->mouseX += data->mintrData.x;
			if (data->mouseX >= gVideoWidth)
			{
				data->mouseX = gVideoWidth - 1;
			}
			else if (data->mouseX < 0)
			{
				data->mouseX = 0;
			}

			data->mouseY += data->mintrData.y;
			if (data->mouseY >= gVideoHeight)
			{
				data->mouseY = gVideoHeight - 1;
			}
			else if (data->mouseY < 0)
			{
				data->mouseY = 0;
			}
			__kDrawMouse();	
		}

		if (data->mintrData.status & 7)
		{
			data->mouseBuf[data->mouseBufHdr].status = data->mintrData.status;
			data->mouseBuf[data->mouseBufHdr].x = data->mouseX;
			data->mouseBuf[data->mouseBufHdr].y = data->mouseY;

			data->mouseBufHdr++;
			if (data->mouseBufHdr >= MOUSE_POS_LIMIT)
			{
				data->mouseBufHdr = 0;
			}
		}

	}
	if (gMouseTest)
	{
		mousetest();
	}
}




int isGeometryMouse(int x,int y) {
	LPMOUSEDATA data = (LPMOUSEDATA)MOUSE_BUFFER;
	if ( (6 * y >= 4 * x) && (6 * y <= 9 * x) ) {
		if ( (y > data->mouseWidth *2 / 3) && (x > data->mouseWidth * 2 / 3) ) {
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}

int isGeometryBorder(int x, int y) {
	LPMOUSEDATA data = (LPMOUSEDATA)MOUSE_BUFFER;
	if ( (y + MOUSE_BORDER_WIDTH >= data->mouseWidth * 2 / 3) && (x + MOUSE_BORDER_WIDTH >= data->mouseWidth * 2 / 3) ) {
		return TRUE;
	}
	if (6 * y <= 4 * (x + MOUSE_BORDER_WIDTH) || 6 * y >= 9 * (x - MOUSE_BORDER_WIDTH)) {

		return TRUE;
	}
	return FALSE;
}

void __kDrawMouse() {
	LPMOUSEDATA data = (LPMOUSEDATA)MOUSE_BUFFER;
	int pos = __getpos(data->mouseX, data->mouseY) + gGraphBase;

	int h = data->mouseHeight;

	int w = data-> mouseWidth;
	if (data->mouseX + data->mouseWidth >= gVideoWidth) {
		w = ( data->mouseX + data->mouseWidth % gVideoWidth);
	}

	if (data->mouseY + data->mouseHeight >= gVideoHeight) {
		h = (data->mouseY + data->mouseHeight % gVideoHeight);
	}

	unsigned char * storeptr = (unsigned char*)data->mouseCoverData;

	for ( int y = 0; y < h; y++)
	{
		for ( int x = 0; x < w; x++)
		{
			int color = 0;
			if (isGeometryMouse(x,y) ) {
				if (isGeometryBorder(x,y) )
				{
					color = MOUSE_BORDER_COLOR;

				}
				else {
					color = gMouseColor;
					
				}
				unsigned char * showpos = (unsigned char*)__getpos(x, y) + pos;

				for (int i = 0; i < gBytesPerPixel; i++)
				{
					if (*showpos != *storeptr) {
						*storeptr = *showpos;
					}
					if (*showpos != ( color & 0xff)) {
						*showpos = color;
					}
					
					color >>= 8;
					showpos++;
					storeptr++;
				}
			}
		}
		gMouseColor += 0x01010101;
	}
}

void __kRestoreMouse() {

	LPMOUSEDATA data = (LPMOUSEDATA)MOUSE_BUFFER;
	int pos = __getpos(data->mouseX, data->mouseY) + gGraphBase;

	int h = data->mouseHeight;

	int w = data->mouseWidth;
	if (data->mouseX + data->mouseWidth >= gVideoWidth) {
		w = data->mouseX + data->mouseWidth % gVideoWidth;
	}

	if (data->mouseY + data->mouseHeight >= gVideoHeight) {
		h = data->mouseY + data->mouseHeight % gVideoHeight;
	}

	unsigned char * storeptr = (unsigned char*)data->mouseCoverData;

	for ( int y = 0; y < h; y++)
	{
		for ( int x = 0; x < w; x++)
		{
			if (isGeometryMouse(x, y))
			{
				unsigned char * showpos = (unsigned char*)__getpos(x, y) + pos;

				for (int i = 0; i < gBytesPerPixel; i++)
				{
					if (*showpos != *storeptr) {
						*showpos = *storeptr;
					}
					showpos++;
					storeptr++;
				}
			}
		}
	}
}

//4x< 6y < 9x
void __kRefreshMouseBackup() {
	return;

	LPMOUSEDATA data = (LPMOUSEDATA)MOUSE_BUFFER;
	int pos = __getpos(data->mouseX, data->mouseY) + gGraphBase;

	unsigned char * storebuf = (unsigned char*)data->mouseCoverData;

	for ( int y = 0; y < data->mouseHeight; y ++)
	{
		for ( int x = 0; x < data->mouseWidth; x ++)
		{
			if (isGeometryMouse(x, y))
			{
				unsigned char * showpos = (unsigned char*)__getpos(x, y) + pos;

				for (int i = 0;i < gBytesPerPixel;i ++)
				{
					if (*showpos != *storebuf) {
						*storebuf = *showpos;
					}
					storebuf++;
					showpos++;
				}
			}
		}
	}
}




void __initMouse(int x,int y) {

	enableMouse();
	setMouseScale();
	setMouseSampleRate(200);
	setMouseResolution(3);

	//enableMouseZAxis();

	gMouseID = getMouseID();
	char szout[1024];
	__printf(szout, (char*)"keyboard id:%x,mouse id:%x\n", gKeyboardID, gMouseID);
	//gMouseID = 0;

	LPMOUSEDATA data = (LPMOUSEDATA)MOUSE_BUFFER;
	data->mouseX = x/2;
	data->mouseY = y/2;

	if (x > y)
	{
		data->mouseWidth = y / MOUSE_FACTOR_SIZE;
		data->mouseHeight = y / MOUSE_FACTOR_SIZE;
	}
	else {
		data->mouseWidth = x / MOUSE_FACTOR_SIZE;
		data->mouseHeight = x / MOUSE_FACTOR_SIZE;
	}

	__kRefreshMouseBackup();
	__kDrawMouse();
}


void insertMouse(MOUSEINFO * info) {
	__kRestoreMouse();
	LPMOUSEDATA data = (LPMOUSEDATA)MOUSE_BUFFER;
	data->mouseX += info->x;
	data->mouseY += info->y;
	data->mintrData.status = info->status;

	//return;
	
	data->mouseBuf[data->mouseBufHdr].status = info->status;
	data->mouseBuf[data->mouseBufHdr].x = data->mouseX;
	data->mouseBuf[data->mouseBufHdr].y = data->mouseY;

	data->mouseBufHdr++;
	if (data->mouseBufHdr >= MOUSE_POS_LIMIT)
	{
		data->mouseBufHdr = 0;
	}
	
	__kRefreshMouseBackup();
	__kDrawMouse();
}

int GetMousePacket(LPMOUSEINFO lpmouse) {
	LPMOUSEDATA data = (LPMOUSEDATA)MOUSE_BUFFER;
	lpmouse->status = data->mintrData.status;
	lpmouse->x = data->mintrData.x;
	lpmouse->y = data->mintrData.y;
	lpmouse->z = data->mintrData.z;
	return 1;
}

int __kGetMouse(LPMOUSEINFO lpmouse, int wid) {

	if (isTopWindow(wid))
	{
		LPMOUSEDATA data = (LPMOUSEDATA)MOUSE_BUFFER;
		if (data->mouseBufHdr == data->mouseBufTail)
		{
			return FALSE;
		}

		lpmouse->status = data->mouseBuf[data->mouseBufTail].status;
		lpmouse->x = data->mouseBuf[data->mouseBufTail].x;
		lpmouse->y = data->mouseBuf[data->mouseBufTail].y;
		data->mouseBufTail++;
		if (data->mouseBufTail >= MOUSE_POS_LIMIT)
		{
			data->mouseBufTail = 0;
		}
		return TRUE;
	}
	else {
		return FALSE;
	}
}



__declspec(naked) void MouseIntProc() {

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
		mov ss,ax
	}
	{
		__kMouseProc();

#ifdef IO_APIC_ENABLE
		* (DWORD*)(LOCAL_APIC_BASE + 0xB0) = 0;
		*(DWORD*)(IO_APIC_BASE + 0x40) = 0;
#else
		outportb(0x20, 0x20);
		outportb(0xa0, 0x20);
#endif

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