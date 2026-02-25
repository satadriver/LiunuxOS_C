
#include "Utils.h"

#define memcpy __memcpy

#define memmove __memmove

#define memset __memset

#define memcmp __memcmp

#define strcmp __strcmp

#define strcpy __strcpy

#define strlen __strlen

#define strcat __strcat

#define strstr __strstr

#define wmemcpy __wmemcpy

#define wcsstr __wcsstr

#define wcslen __wcslen

#define wcscmp __wcscmp

#define wcscat __wcscat

int printf(const char* format, ...);

typedef char* va_list;

#define va_start(list,param1)   ( list = (va_list)&param1+ sizeof(param1) )

#define va_arg(list,mode)   ( (mode *) ( list += sizeof(mode) ) )[-1]

#define va_end(list) ( list = (va_list)0 )

