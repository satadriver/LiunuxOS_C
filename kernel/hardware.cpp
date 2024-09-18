#include "hardware.h"

unsigned int inportbs(unsigned short port) {
	__asm {
		mov dx, port
		in al, dx
		movsx eax, al
	}
}

unsigned int inportb(unsigned short port) {
	__asm {
		mov dx,port
		in al,dx
		movzx eax,al
	}
}


unsigned int inportw(unsigned short port) {
	__asm {
		mov dx, port
		in ax, dx
		movzx eax, ax
	}
}

unsigned int inportd(unsigned int port) {
	__asm {
		mov edx, port
		in eax, dx
	}
}


void outportb(unsigned short port,unsigned char value) {
	__asm {
		mov dx, port
		mov al, value
		out dx,al
	}
}


void outportw(unsigned short port, unsigned short value) {
	__asm {
		mov dx, port
		mov ax, value
		out dx,ax
	}
}

void outportd(unsigned short port, unsigned int value) {
	__asm {
		mov dx, port
		mov eax,value
		out dx,eax
	}
}