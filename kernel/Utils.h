#pragma once

#include "def.h"

#ifndef _UTILS_H_H_H
#define _UTILS_H_H_H

#pragma pack(1)
typedef struct  
{
	DWORD low;
	DWORD high;
}MYINT64,*LPMYINT64;




#pragma pack()

DWORD CheckSpinlock(DWORD* lpv);

int __GetCurrentTid();

int __GetCurrentPid();

int __GetCurrentCpu();

char* getMainboardDate();

char* getComports();

char* getKeyboardBuf();

extern "C" __declspec(dllexport) char* getVGAInfo();

void logInMem(char* data, int len);

#define  __kPrintf(buf,format, ...) __printf(buf,format,__VA_ARGS__)

int __kFormat(char* buf, const char* format, DWORD* params);

extern "C" __declspec(dllexport) int strlf2lf(double f, char* buf);
extern "C" __declspec(dllexport) int strf2f(float f, char* buf);

extern "C" __declspec(dllexport) int __i64ToStrd64( __int64 v, char* strd);

#ifdef DLL_EXPORT
extern "C" __declspec(dllexport) char* gLogDataPtr;

extern "C"  __declspec(dllexport) int __strlen(const char * str);

extern "C"  __declspec(dllexport) int __memcpy(char * dst, char * src, int len);

extern "C"  __declspec(dllexport) int __wmemcpy(wchar_t* dst, wchar_t* src, int len);

extern "C"  __declspec(dllexport) int __strcpy(char * dst, char * src);

extern "C"  __declspec(dllexport) int __strncpy(char * dst, char * src, int limit);

extern "C"  __declspec(dllexport) int __memset(char * dst, int value, int len);
extern "C"  __declspec(dllexport) int __memset4(char * dst, int value, int len);

extern "C"  __declspec(dllexport) int __strcmp(const char * src, const char * dst);

extern "C"  __declspec(dllexport) int __memcmp(const char * src,const char * dst, int len);

extern "C"  __declspec(dllexport) char* __strstr(char * src, char * dst);

extern "C"  __declspec(dllexport) int __strcat(char * src, char * dst);

extern "C"  __declspec(dllexport) int __wcscat(wchar_t* src, wchar_t* dst);
extern "C"  __declspec(dllexport) int __wcscpy(wchar_t* dst, wchar_t* src);
extern "C"  __declspec(dllexport) int __wcscat(wchar_t* src, wchar_t* dst);
extern "C"  __declspec(dllexport) int __wcslen(wchar_t* s);
extern "C"  __declspec(dllexport) int __wcscmp(wchar_t* src, wchar_t* dst);
extern "C"  __declspec(dllexport) wchar_t* __wcsstr(wchar_t* src, wchar_t* dst);

extern "C"  __declspec(dllexport) int __i2strh(unsigned int n, int lowercase, unsigned char * dst);

extern "C"  __declspec(dllexport) int __strh2i(unsigned char * str);

extern "C"  __declspec(dllexport) int __i2strd(int h, char* strd);

extern "C"  __declspec(dllexport) int __i2stru(unsigned int h, char * strd);

extern "C"  __declspec(dllexport) int __strd2i(char * istr);

extern "C"  __declspec(dllexport) int __sprintf(char * buf,const char * format, ...);

extern "C"  __declspec(dllexport) int __printf(char * buf,const char * format, ...); 

extern "C"  __declspec(dllexport) int __dump(char * src, int len, int lowercase, unsigned char * dstbuf);

extern "C" __declspec(dllexport) int __substr(char * src, int pos, char * dst);
extern "C" __declspec(dllexport) int __substrLen(char * src, int pos, int len, char * dst);

extern "C" __declspec(dllexport) int __strupr(char * str);

extern "C" __declspec(dllexport) int __strlwr(char * str);

extern "C" __declspec(dllexport) int __shutdownSystem();
extern "C" __declspec(dllexport) int __reset();

extern "C" __declspec(dllexport) int __sleep(int millisecs);

extern "C" __declspec(dllexport) int __giveup();

extern "C" __declspec(dllexport) int getCpuType(char * name);

extern "C" __declspec(dllexport) int getCpuInfo(char * name);

extern "C" __declspec(dllexport) int lower2upper(char *data, int len);

extern "C" __declspec(dllexport) int upper2lower(char *data, int len);

extern "C" __declspec(dllexport) WORD __ntohs(WORD v);

extern "C" __declspec(dllexport)DWORD __ntohl(DWORD v);

extern "C" __declspec(dllexport)DWORD __lockInc(DWORD *v);

extern "C" __declspec(dllexport)void __initSpinlock(DWORD * v);
extern "C" __declspec(dllexport)DWORD __enterSpinlock(DWORD * v);
extern "C" __declspec(dllexport)DWORD __leaveSpinlock(DWORD * v);
extern "C" __declspec(dllexport)DWORD __enterLock(DWORD* lockvalue);
extern "C" __declspec(dllexport)DWORD __leaveLock(DWORD* lockvalue);

extern "C"  __declspec(dllexport) int __spinlockEntry(DWORD* lockv);

extern "C"  __declspec(dllexport) int __spinlockLeave(DWORD * lockv);

extern "C" __declspec(dllexport) int asc2unicode(char * asc, int asclen, short * unicode);

extern "C" __declspec(dllexport) int unicode2asc(short * unicode, int unicodelen, char * asc);
#else

extern "C"  __declspec(dllimport) int __strlen(const char * str);

extern "C"  __declspec(dllimport) int __memcpy(char * dst, char * src, int len);

extern "C"  __declspec(dllimport) int __strcpy(char * dst, char * src);

extern "C"  __declspec(dllimport) int __strncpy(char * dst, char * src, int limit);

extern "C"  __declspec(dllimport) int __memset(char * dst, int value, int len);
extern "C"  __declspec(dllimport) int __memset4(char * dst, int value, int len);

extern "C"  __declspec(dllimport) int __strcmp(const char * src, const char * dst);

extern "C"  __declspec(dllimport) int __memcmp(const char * src,const char * dst, int len);

extern "C"  __declspec(dllimport) char* __strstr(char * src, char * dst);

extern "C"  __declspec(dllimport) int __strcat(char * src, char * dst);

extern "C"  __declspec(dllimport) int __wmemcpy(wchar_t* dst, wchar_t* src, int len);

extern "C"  __declspec(dllimport) int __wcscat(wchar_t* src, wchar_t* dst);
extern "C"  __declspec(dllimport) int __wcscpy(wchar_t* dst, wchar_t* src);
extern "C"  __declspec(dllimport) int __wcscat(wchar_t* src, wchar_t* dst);
extern "C"  __declspec(dllimport) int __wcslen(wchar_t* s);
extern "C"  __declspec(dllimport) int __wcscmp(wchar_t* src, wchar_t* dst);
extern "C"  __declspec(dllimport) wchar_t* __wcsstr(wchar_t* src, wchar_t* dst);

extern "C"  __declspec(dllimport) int __i2strh(unsigned int n, int lowercase, unsigned char * dst);

extern "C"  __declspec(dllimport) int __strh2i(unsigned char * str);

extern "C"  __declspec(dllimport) int __i2strd( int h, char * strd);

extern "C"  __declspec(dllimport) int __strd2i(char * istr);

extern "C"  __declspec(dllexport) int __i2stru(unsigned int h, char* strd);

extern "C"  __declspec(dllimport) int __sprintf(char * buf,const char * format, ...);

extern "C"  __declspec(dllimport) int __printf(char * buf, const char * format, ...);

extern "C"  __declspec(dllimport) int __dump(char * src, int len, int lowercase, unsigned char * dstbuf);

extern "C" __declspec(dllimport) int __substr(char * src, int pos, char * dst);
extern "C" __declspec(dllimport) int __substrLen(char * src, int pos, int len, char * dst);

extern "C" __declspec(dllimport) int __strupr(char * str);

extern "C" __declspec(dllimport) int __strlwr(char * str);

extern "C" __declspec(dllimport) int __shutdownSystem();
extern "C" __declspec(dllimport) int __reset();

extern "C" __declspec(dllimport) int __sleep(int millisecs);

extern "C" __declspec(dllimport) int __giveup();


extern "C" __declspec(dllimport) int getCpuType(char * name);

extern "C" __declspec(dllimport) int getCpuInfo(char * name);

extern "C" __declspec(dllimport) int lower2upper(char *data, int len);

extern "C" __declspec(dllimport) int upper2lower(char *data, int len);

extern "C" __declspec(dllimport)WORD __ntohs(WORD v);
extern "C" __declspec(dllimport)DWORD __ntohl(DWORD v);
extern "C" __declspec(dllimport)DWORD __lockInc(DWORD *v);
extern "C" __declspec(dllimport)void __initSpinlock(DWORD * v);
extern "C" __declspec(dllimport)DWORD __enterSpinlock(DWORD * v);
extern "C" __declspec(dllimport)DWORD __leaveSpinlock(DWORD * v);
extern "C"  __declspec(dllimport) int __spinlockEntry(DWORD* lockv);

extern "C"  __declspec(dllimport) int __spinlockLeave(DWORD* lockv);

extern "C" __declspec(dllimport) int asc2unicode(char * asc, int asclen, short * unicode);

extern "C" __declspec(dllimport) int unicode2asc(short * unicode, int unicodelen, char * asc);
#endif


#define __memmove __memcpy


#endif