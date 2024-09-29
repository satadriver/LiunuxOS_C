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


 DWORD gMouseTest = 0;

DWORD gMouseID = 0;

DWORD gMouseColor =	MOUSE_SHOW_COLOR;

#define MOUSE_FACTOR_SIZE	30


void mousetest() {

	char szout[1024];
	MOUSEINFO mouseinfo;
	__memset((char*)&mouseinfo, 0, sizeof(MOUSEINFO));
	unsigned int ret = __kGetMouse(&mouseinfo,gMouseTest);
	if (ret)
	{
		DWORD pos = (gVideoHeight - GRAPHCHAR_HEIGHT*2) * gVideoWidth * gBytesPerPixel + (gVideoWidth/2)*gBytesPerPixel;

		__sprintf(szout, "mouse x:%x,mouse y:%x,status:%x\n", mouseinfo.x,mouseinfo.y,mouseinfo.status);
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

		int md = inportbs(0x60);
		*pos = md;
		pos++;

		counter++;
		if (gMouseID)
		{
			if (counter >= 4)
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
			return;
		}
	}
	else {
		if (counter != 3)
		{
			return;
		}
	}
	
	if (data->mintrData.status & 0x20) {	//y = 1
		
	}
	else{
		//data->mintrData.y = -data->mintrData.y;
	}

	if (data->mintrData.status & 0x10) {	//x = 1
		//data->mintrData.x = -data->mintrData.x;
	}
	else {

	}

	data->mintrData.y = -data->mintrData.y;

	if (data->mintrData.x || data->mintrData.y)
	{
		//if (gScreenProtectWindowID == 0)
		{
			__kRestoreMouse();
		}

		data->mouseX += data->mintrData.x;
		if (data->mouseX > gVideoWidth)
		{
			data->mouseX = gVideoWidth;
		}
		else if (data->mouseX < 0)
		{
			data->mouseX = 0;
		}

		data->mouseY += data->mintrData.y;
		if (data->mouseY > gVideoHeight)
		{
			data->mouseY = gVideoHeight;
		}
		else if (data->mouseY < 0)
		{
			data->mouseY = 0;
		}

		//if (gScreenProtectWindowID == 0) 
		{
			__kDrawMouse();
		}
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

	if (gMouseTest)
	{
		mousetest();
	}
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

	unsigned char * storeptr = (unsigned char*)data->mouseCoverData;

	for ( int y = 0; y < data->mouseHeight; y++)
	{
		for ( int x = 0; x < data->mouseWidth; x++)
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

	unsigned char * storeptr = (unsigned char*)data->mouseCoverData;

	for ( int y = 0; y < data->mouseHeight; y++)
	{
		for ( int x = 0; x < data->mouseWidth; x++)
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

int getMouseID() {
	int mid = 0;
	__asm {
		call __wait8042Empty
		mov al, 0xd4
		out 64h, al

		call __wait8042Empty
		mov al, 0xf2
		out 60h, al

		//call waitPs2In
		//mov al, 0x20
		//out 64h, al

		//call waitPs2Out

		in al, 60h
		movzx eax, al
		mov mid, eax
	}
	return mid;
}


void __initMouse(int x,int y) {

	enableMouse();

	gMouseID = getMouseID();

	setMouseRate(200);

	char szout[1024];
	__printf(szout, "keyboard id:%x, mouse id:%d\n", gKeyboardID,gMouseID);

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
	LPMOUSEDATA data = (LPMOUSEDATA)MOUSE_BUFFER;
	data->mouseBuf[data->mouseBufHdr].status = info->status;
	data->mouseBuf[data->mouseBufHdr].x = info->x;
	data->mouseBuf[data->mouseBufHdr].y = info->y;

	data->mouseBufHdr++;
	if (data->mouseBufHdr >= MOUSE_POS_LIMIT)
	{
		data->mouseBufHdr = 0;
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
		outportb(0x20, 0x20);
		outportb(0xa0, 0xa0);
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