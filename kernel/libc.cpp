
#include "libc.h"
#include "video.h"
#include "process.h"
#include "def.h"
#include "window.h"
#include "apic.h"
#include "malloc.h"
#include "math.h"


#pragma intrinsic(wcslen)  // 启用内部函数
#pragma function(wcslen)   // 强制使用函数调用而不是内部函数


#pragma intrinsic(memcpy)  // 启用内部函数
#pragma function(memcpy)   // 强制使用函数调用而不是内部函数


#pragma intrinsic(memmove)  // 启用内部函数
#pragma function(memmove)   // 强制使用函数调用而不是内部函数


#pragma intrinsic(wcscmp)  // 启用内部函数
#pragma function(wcscmp)   // 强制使用函数调用而不是内部函数

#pragma intrinsic(wcscat)  // 启用内部函数
#pragma function(wcscat)   // 强制使用函数调用而不是内部函数


#pragma intrinsic(strlen)  // 启用内部函数
#pragma function(strlen)   // 强制使用函数调用而不是内部函数


#pragma intrinsic(strcpy)  // 启用内部函数
#pragma function(strcpy)   // 强制使用函数调用而不是内部函数


#pragma intrinsic(strcmp)  // 启用内部函数
#pragma function(strcmp)   // 强制使用函数调用而不是内部函数

#pragma intrinsic(strcat)  // 启用内部函数
#pragma function(strcat)   // 强制使用函数调用而不是内部函数



#pragma intrinsic(memset)  // 启用内部函数
#pragma function(memset)   // 强制使用函数调用而不是内部函数

#pragma intrinsic(memcmp)  // 启用内部函数
#pragma function(memcmp)   // 强制使用函数调用而不是内部函数

#pragma intrinsic(abs)  // 启用内部函数
#pragma function(abs)   // 强制使用函数调用而不是内部函数



FILE _iob[3];


void* memcpy(void* dest, const void* src, size_t n) {
	int len = __memcpy((char*)dest, (char*)src, n);
	return dest;
}

void* memmove(void* dest, const void* src, size_t n) {
	int len = __memmove((char*)dest, (char*)src, n);
	return dest;
}

void* memset(void* ptr, int value, size_t num) {
	int len = __memset((char*)ptr, value, num);
	return ptr;
}

int memcmp(const void* ptr1, const void* ptr2, size_t num) {
	return __memcmp((const char*)ptr1, (const char*)ptr2, num);
}

char* strcpy(char* destination, const char* source) {
	int len = __strcpy(destination, (char* )source);
	return destination;
}
size_t strlen(const char* str) {
	return __strlen(str);
}

int strcmp(const char* str1, const char* str2) {
	return __strcmp(str1, str2);
}
int strncmp(const char* str1, const char* str2, size_t n) {

	return __strncmp(str1, str2, n);
}
char* strcat(char* destination, const char* source) {
	int len = __strcat(destination, (char* )source);
	return destination;
}
char* strstr(const char* haystack, const char* needle) {
	return __strstr((char* )haystack, (char*)needle);
}
wchar_t* wmemcpy(wchar_t* dest, const wchar_t* src, size_t n) {
	int len = __wmemcpy(dest, (wchar_t*)src, n);
	return dest;
}
wchar_t* wcsstr(const wchar_t* haystack, const wchar_t* needle) {
	return __wcsstr((wchar_t*)haystack, (wchar_t*)needle);
}
size_t wcslen(const wchar_t* str) {
	return __wcslen((wchar_t*)str);
}
int wcscmp(const wchar_t* str1, const wchar_t* str2) {
	return __wcscmp((wchar_t*)str1, (wchar_t*)str2);
}
wchar_t* wcscat(wchar_t* dest, const wchar_t* src) {
	int len = __wcscat(dest,(wchar_t* )src);
	return dest;
}


void abort(void) {

}


FILE* fopen(const char* filename, const char* mode) {
	return 0;
}


int fclose(FILE* fp) {
	return 0;
}

int fwrite(const void* buf, int cnt, int size,  FILE* fp) {
	return 0;
}

int fread( void* buf, int cnt, int size, FILE* fp) {
	return 0;
}

int fprintf(FILE* stream, const char* format, ...) {
	return 0;
}

int fputc(int character, FILE* stream) {
	return 0;
}

int fputs(const char* str, FILE* stream) {
	return 0;
}
char* fgets(char* str, int n, FILE* stream) {
	return str;
}

void* calloc(int cnt,int size) {
	char * buf = (char*)__malloc(size*cnt);
	__memset(buf, 0, size);
	return buf;
}

void* malloc(int size) {
	return (char*)__malloc(size);
}

void free(void* buf) {
	__free((unsigned long)buf);
	return;
}

void* realloc(void* buf, int size) {
	char* buffer = (char*)__malloc(size);
	memcpy(buffer, (char*)buf, size);
	free(buf);
	return buffer;
}

int printf(const char* format, ...) {
	char buf[1024];
	if (g_ScreenMode == 0) {
		return 0;
	}

	if (format == 0 ) {
		return FALSE;
	}

	int formatLen = __strlen((char*)format);
	if (formatLen == 0) {
		return FALSE;
	}

	void* params = 0;
	int param_cnt = 0;
	__asm {
		lea eax, format
		add eax, 4
		mov params, eax
	}

	int len = __kFormat(buf, (char*)format, (DWORD*)params);
	LPPROCESS_INFO proc = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	int cpu = __GetCurrentCpu();
	LPWINDOWSINFO winfo = __FindProcessWindow(proc->tid, cpu);
	if (winfo) {
		LPWINDOWCLASS window = winfo->window;
		if (window) {
			__drawWindowChars(buf, 0, window);
		}
		else {
			int endpos = __drawGraphChars((char*)buf, 0);
		}
	}
	else {
		int endpos = __drawGraphChars((char*)buf, 0);
	}

	return len;
}

float fabsf(float x) {
	return __abs(x);
}

float fabs(float x) {
	return __abs(x);
}

int abs(int x) {
	return __abs(x);
}
 double pown(double x, int n) {
	return __pown(x, n);
}
 double cos(double x) {
	return __cos(x);
}
double sin(double x) {
	return __sin(x);
}
 double pow(double a, int b) {
	return __pow(a, b);
}
double sqrt(double x) {
	return __sqrt(x);
}



float sqrtf(float x) {
	return __sqrt(x);
}
float cosf(float x) {
	return __cos(x);
}
float sinf(float x) {
	return __sin(x);
}





float logf(float x) {
	return __log(x);
}

float expf(float x) {
	return __exp(x);
}


double log(double x) {
	return __log(x);
}

double exp(double x) {
	return __exp(x);
}
