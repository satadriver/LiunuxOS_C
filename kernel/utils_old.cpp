#include "Utils.h"
#include "video.h"
#include "cmosAlarm.h"
#include "cmosPeriodTimer.h"
#include "acpi.h"
#include "hardware.h"


int __memset_old(char* dst, int value, int len) {
#ifdef _DEBUG
	return 0;
#else
	__asm {
		mov ecx, len

		mov edi, dst

		mov al, byte ptr value

		cld

		rep stosb
		mov eax, len
	}
#endif
}


int __memset4(char* dst, int value, int len) {
	__asm {
		push ecx
		push edx
		push ebx
		push esi
		push edi

		mov ecx, len
		shr ecx, 2

		mov edi, dst

		mov eax, value

		cld

		rep stosd

		lea esi, value
		mov ecx, len
		and ecx, 3
		rep movsb

		pop edi
		pop esi
		pop ebx
		pop edx
		pop ecx
		mov eax, len
	}
}


int __strlen_old(char* s) {
	__asm {
		push esi
		mov esi, s
		cld
		_checkZero :
		lodsb
			cmp al, 0
			jnz _checkZero
			dec esi
			sub esi, s
			mov eax, esi
			pop esi
	}
}



int __memcpy_old(char* dst, char* src, int len) {
	__asm {
		mov esi, src

		mov edi, dst

		mov ecx, len

		cld

		rep movsb
		mov eax, len
	}
}


int __strcpy_old(char* dst, char* src) {
	__asm {
		mov edi, dst
		mov esi, src
		cld
		_copyBytes :
		lodsb
			stosb
			cmp al, 0
			jnz _copyBytes

			dec edi
			sub edi, dst
			mov eax, edi
	}
}


int __memcmp_old(char* src, char* dst, int len) {

	__asm {
		mov esi, src
		mov edi, dst
		mov ecx, len
		cld
		repz cmpsb
		jz _strEqual
		mov eax, -1
		jmp _memcmpEnd
		_strEqual :
		mov eax, 0
			_memcmpEnd :
			//不要在嵌入汇编中调用ret，会打乱c编译器的编译
			//ret
	}
}


int __strncpy_old(char* dst, char* src, int limit) {
	if (limit <= 0)
	{
		return FALSE;
	}

	int l = __strlen(src);
	if (l > limit)
	{
		l = limit;
		__memcpy(dst, src, l);
		*(dst + l) = 0;
		return l;
	}
	else {
		__asm {
			mov edi, dst
			mov esi, src
			cld
			_copyBytes :
			lodsb
				stosb
				cmp al, 0
				jnz _copyBytes

				dec edi
				sub edi, dst
				mov eax, edi
		}
	}
}



char* __strstr_old(char* src, char* dst) {

	int dstlen = __strlen(dst);
	int srclen = __strlen(src);
	if (dstlen > srclen)
	{
		return 0;
	}

	__asm {
		mov esi, src

		mov ecx, srclen
		sub ecx, dstlen
		inc ecx
		cld

		_checkStr :
		mov edx, ecx

			push esi

			mov edi, dst

			mov ecx, dstlen
			repz cmpsb

			pop esi

			jz _strstrFound

			mov ecx, edx

			inc esi
			loop _checkStr

			mov eax, 0
			jmp __strstrEnd

			_strstrFound :
		mov eax, esi

			__strstrEnd :
	}
}


int __strcmp_old(char* src, char* dst) {
	int srclen = __strlen(src);
	int dstlen = __strlen(dst);
	if (srclen != dstlen)
	{
		return -1;
	}

	__asm {
		mov esi, src
		mov edi, dst
		mov ecx, srclen
		repz cmpsb
		jz _strEqual	//do not use jecxz,if length equal and first n-1 is equal but last char is not equal,then ecx is 0,error
		mov eax, -1
		jmp _strcmpEnd
		_strEqual :
		mov eax, 0
			_strcmpEnd :
			//不要在嵌入汇编中调用ret，会打乱c编译器的编译
			//ret
	}
}



int __printf_old(char* buf, char* format, ...) {
	int seq = 0;
	int len = 0;

	__asm {
		lea eax, format
		add eax, 4
		mov seq, eax

		mov esi, format
		mov edi, buf
		cld
		_printfGetBytes :
		lodsb
			cmp al, 0
			jz _printfEnd
			cmp al, '%'
			jz _loadNextByte
			stosb
			jmp _printfGetBytes

			_loadNextByte :
		cmp byte ptr[esi], 'd'
			jz _printfInt
			cmp byte ptr[esi], 'x'
			jz _printfHex
			cmp byte ptr[esi], 'p'
			jz _printfHex
			cmp byte ptr[esi], 'u'
			jz _printfInt
			cmp byte ptr[esi], 's'
			jz _printfstr
			cmp dword ptr[esi], 0x64343649		//I64d
			jz _printfInt64
			cmp dword ptr[esi], 0x75343649		//I64u
			jz _printfInt64
			stosb
			jmp _printfGetBytes

			_printfInt :
		inc esi
			push edi
			mov eax, seq
			push[eax]
			add eax, 4
			mov seq, eax
			call __i2strd
			add esp, 8
			add edi, eax
			jmp _printfGetBytes

			_printfHex :
		mov ax, 7830h
			stosw
			inc esi
			push edi
			push 0
			mov eax, seq
			push[eax]
			add eax, 4
			mov seq, eax
			call __i2strh
			add esp, 12
			add edi, 8
			jmp _printfGetBytes

			_printfstr :
		inc esi
			mov eax, seq
			push[eax]
			add eax, 4
			mov seq, eax
			push edi
			call __strcpy
			add esp, 8
			add edi, eax
			jmp _printfGetBytes

			_printfInt64 :
		add esi, 4
			mov eax, edi
			add eax, 8
			push eax
			//push edi+8
			push 0
			mov eax, seq
			push[eax]
			add eax, 4
			mov seq, eax
			call __i2strh
			add esp, 12
			//add edi, 8

			push edi
			push 0
			mov eax, seq
			push[eax]
			add eax, 4
			mov seq, eax
			call __i2strh
			add esp, 12

			add edi, 16
			jmp _printfGetBytes

			_printfEnd :
		stosb
			mov eax, edi
			sub eax, buf
			dec eax
			mov len, eax
	}
	if (g_ScreenMode) {
		int showlen = __drawGraphChars((unsigned char*)buf, 0);
	}
	else {

	}
	return len;
}


int __sprintf_old(char* buf, char* format, ...) {
	int seq = 0;

	int len = 0;
	__asm {
		lea eax, format
		add eax, 4	//size of pointer is 4
		mov seq, eax	//seq is address of params after format in esp

		mov esi, format
		mov edi, buf
		cld

		_printfGetBytes :
		lodsb
			cmp al, 0
			jz _printfEnd
			cmp al, '%'
			jz _loadNextByte
			stosb
			jmp _printfGetBytes

			_loadNextByte :
		cmp byte ptr[esi], 'd'
			jz _printfInt
			cmp byte ptr[esi], 'x'
			jz _printfHex
			cmp byte ptr[esi], 'p'
			jz _printfHex
			cmp byte ptr[esi], 'u'
			jz _printfInt
			cmp byte ptr[esi], 's'
			jz _printfstr
			cmp dword ptr[esi], 0x64343649		//I64d
			jz _printfInt64
			cmp dword ptr[esi], 0x75343649		//I64u
			jz _printfInt64
			stosb
			jmp _printfGetBytes


			_printfHex :
		mov ax, 7830h
			stosw
			inc esi
			push edi
			push 0	//lowercase is 0,else uppercase
			mov eax, seq
			push[eax]
			add eax, 4
			mov seq, eax
			call __i2strh
			add esp, 12
			add edi, 8
			jmp _printfGetBytes

			_printfInt :
		inc esi
			push edi
			mov eax, seq
			push[eax]
			add eax, 4
			mov seq, eax
			call __i2strd
			add esp, 8
			add edi, eax
			jmp _printfGetBytes

			_printfstr :
		inc esi
			mov eax, seq
			push[eax]
			add eax, 4
			mov seq, eax
			push edi
			call __strcpy
			add esp, 8
			add edi, eax
			jmp _printfGetBytes

			//__int64高位4字节在前，低位4字节在后一个参数中
			_printfInt64 :
		add esi, 4
			mov eax, edi
			add eax, 8
			push eax
			//push edi
			push 0
			mov eax, seq
			push[eax]
			add eax, 4
			mov seq, eax
			call __i2strh
			add esp, 12
			//add edi, 8

			push edi
			push 0
			mov eax, seq
			push[eax]
			add eax, 4
			mov seq, eax
			call __i2strh
			add esp, 12

			//add edi, 8
			add edi, 16
			jmp _printfGetBytes

			_printfEnd :
		stosb
			mov eax, edi
			sub eax, buf
			dec eax
			mov len, eax
	}

	return len;
}