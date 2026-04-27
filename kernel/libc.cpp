
#include "libc.h"
#include "video.h"
#include "process.h"
#include "def.h"
#include "window.h"
#include "apic.h"
#include "malloc.h"
#include "math.h"



#ifndef _DEBUG



#endif

FILE _iob[3];



extern "C" __declspec(dllexport)void* my_memcpy(void* dest, const void* src, size_t n) {
	int len = __memcpy((char*)dest, (char*)src, n);
	return dest;
}

extern "C" __declspec(dllexport)void* my_memmove(void* dest, const void* src, size_t n) {
	int len = __memmove((char*)dest, (char*)src, n);
	return dest;
}

extern "C" __declspec(dllexport)void* my_memset(void* ptr, int value, size_t num) {
	int len = __memset((char*)ptr, value, num);
	return ptr;
}

extern "C" __declspec(dllexport)int my_memcmp(const void* ptr1, const void* ptr2, size_t num) {
	return __memcmp((const char*)ptr1, (const char*)ptr2, num);
}

extern "C" __declspec(dllexport)char* my_strcpy(char* destination, const char* source) {
	int len = __strcpy(destination, (char* )source);
	return destination;
}
extern "C" __declspec(dllexport)size_t my_strlen(const char* str) {
	return __strlen(str);
}

extern "C" __declspec(dllexport)int my_strcmp(const char* str1, const char* str2) {
	return __strcmp(str1, str2);
}
extern "C" __declspec(dllexport)int my_strncmp(const char* str1, const char* str2, size_t n) {
	return __strncmp(str1, str2, n);
}
extern "C" __declspec(dllexport)char* my_strcat(char* destination, const char* source) {
	int len = __strcat(destination, (char* )source);
	return destination;
}
extern "C" __declspec(dllexport)char* my_strstr(const char* haystack, const char* needle) {
	return __strstr((char* )haystack, (char*)needle);
}
extern "C" __declspec(dllexport)wchar_t* my_wmemcpy(wchar_t* dest, const wchar_t* src, size_t n) {
	int len = __wmemcpy(dest, (wchar_t*)src, n);
	return dest;
}
extern "C" __declspec(dllexport)wchar_t* my_wcsstr(const wchar_t* haystack, const wchar_t* needle) {
	return __wcsstr((wchar_t*)haystack, (wchar_t*)needle);
}
extern "C" __declspec(dllexport)size_t my_wcslen(const wchar_t* str) {
	return __wcslen((wchar_t*)str);
}
extern "C" __declspec(dllexport)int my_wcscmp(const wchar_t* str1, const wchar_t* str2) {
	return __wcscmp((wchar_t*)str1, (wchar_t*)str2);
}
extern "C" __declspec(dllexport)wchar_t* my_wcscat(wchar_t* dest, const wchar_t* src) {
	int len = __wcscat(dest,(wchar_t* )src);
	return dest;
}

extern "C" __declspec(dllexport)wchar_t* my_wcscpy(wchar_t* dest, const wchar_t* src) {
	int len = my_wcslen(src);
	my_wmemcpy(dest, src,len);
	return dest;
}


extern "C" __declspec(dllexport)FILE* my_fopen(const char* filename, const char* mode) {
	my_printf("%s %d\r\n", __FUNCTION__, __LINE__);
	return 0;
}

extern "C" __declspec(dllexport)int my_fclose(FILE* fp) {
	my_printf("%s %d\r\n", __FUNCTION__, __LINE__);
	return 0;
}

extern "C" __declspec(dllexport)int my_fwrite(const void* buf, int cnt, int size,  FILE* fp) {
	my_printf("%s %d\r\n", __FUNCTION__, __LINE__);
	return 0;
}

extern "C" __declspec(dllexport)int my_fread( void* buf, int cnt, int size, FILE* fp) {
	my_printf("%s %d\r\n", __FUNCTION__, __LINE__);
	return 0;
}

extern "C" __declspec(dllexport)int my_fprintf(FILE* stream, const char* format, ...) {
	//printf("%s %d\r\n", __FUNCTION__, __LINE__);
	return 0;
}

extern "C" __declspec(dllexport)int my_fputc(int character, FILE* stream) {
	//printf("%s %d\r\n", __FUNCTION__, __LINE__);
	return 0;
}

extern "C" __declspec(dllexport)int my_fputs(const char* str, FILE* stream) {
	//printf("%s %d\r\n", __FUNCTION__, __LINE__);
	return 0;
}
extern "C" __declspec(dllexport)char* my_fgetc(char* str, int n, FILE* stream) {
	my_printf("%s %d\r\n", __FUNCTION__, __LINE__);
	return str;
}

extern "C" __declspec(dllexport)char* my_fgets(char* str, int n, FILE* stream) {
	my_printf("%s %d\r\n", __FUNCTION__, __LINE__);
	return str;
}



extern "C" __declspec(dllexport)void my_abort(void) {
	char szout[256];
	__printf(szout,"%s %d\r\n", __FUNCTION__, __LINE__);
}

extern "C" __declspec(dllexport)void* my_malloc(int size) {
	char * buf = (char*)__malloc(size);
	//__memset(buf, 0, size );
	if (buf == 0) {

		return 0;
	}
	return buf;
}

extern "C" __declspec(dllexport)void my_free(void* buf) {
	__free((unsigned long)buf);
	return;

}

extern "C" __declspec(dllexport)void* my_calloc(int cnt, int size) {
	char* buf = (char*)__malloc(size * cnt);
	if (buf) {
		__memset(buf, 0, size * cnt);
		return buf;
	}

	return 0;
}

extern "C" __declspec(dllexport)void* my_realloc(void* buf, int size) {
	if ( size == 0) {
		if (buf)
			__free((unsigned long)buf);
		return 0;
	}
	char* buffer = (char*)__malloc(size);
	if (buffer) {
		if (buf) {
			__memcpy(buffer, (char*)buf, size);
			__free((DWORD)buf);
		}
		else {
			__memset(buffer, 0, size);
		}
		return buffer;
	}

	return 0;
}







extern "C" __declspec(dllexport)int my_printf(const char* format, ...) {
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
			__drawWindowChars(buf, ~window->color, window);
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


int average(int count, ...) {
	va_list args;
	int sum = 0;

	va_start(args, count);

	for (int i = 0; i < count; i++) {
		int num = va_arg(args, int);  
		sum += num;
		my_printf("param %d: %d\n", i + 1, num);
	}

	va_end(args);

	return sum / count;
}

int va_test_fun() {
	int value = average(5, 1, 2, 3, 4, 5);
	my_printf("%s %d: %d\n\n", __FUNCTION__,__LINE__, value);

	return 0;
}

