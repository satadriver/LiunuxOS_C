
#pragma once

#ifndef LIBC_H_H_H
#define LIBC_H_H_H 

#include "Utils.h"





// assert.h 中的简化定义
#define assert(expression) ((void)0)  // 当 NDEBUG 定义时

typedef char* va_list;


#define va_start(list,param1)   ( list = (va_list)&param1+ sizeof(param1) )

#define va_arg(list,mode)   ( (mode *) ( list += sizeof(mode) ) )[-1]

#define va_end(list) ( list = (va_list)0 )

// Visual Studio 中的定义




#pragma pack(1)


struct _iobuf {
    char* _ptr;      // 缓冲区当前指针
    int _cnt;        // 缓冲区剩余字符数
    char* _base;     // 缓冲区基址
    int _flag;       // 文件标志
    int _file;       // 文件描述符
    int _charbuf;    // 字符缓冲区标志
    int _bufsiz;     // 缓冲区大小
    char* _tmpfname; // 临时文件名
};

#pragma pack()


typedef struct _iobuf FILE;






// Visual Studio 中的 <stdio.h> 核心定义


// 典型的 _CRTIMP 定义
#ifdef _CRTIMP
#undef _CRTIMP
#endif

#ifdef DLL_EXPORT
#define _CRTIMP __declspec(dllexport)  // 构建 CRT DLL 时
#else
#define _CRTIMP __declspec(dllimport)  // 使用 CRT DLL 时
#endif




// 判断是否为整数类型（包括字符、布尔）
#define IS_INTEGER_TYPE(type) \
    _Generic((type)0, \
        _Bool: 1, \
        char: 1, \
        signed char: 1, \
        unsigned char: 1, \
        short: 1, \
        unsigned short: 1, \
        int: 1, \
        unsigned int: 1, \
        long: 1, \
        unsigned long: 1, \
        long long: 1, \
        unsigned long long: 1, \
        default: 0 \
    )

// 判断是否为浮点类型
#define IS_FLOAT_TYPE(type) \
    _Generic((type)0, \
        float: 1, \
        double: 1, \
        long double: 1, \
        default: 0 \
    )

// 判断是否为指针类型
#define IS_POINTER_TYPE(type) \
    _Generic((type)0, \
        void*: 1, \
        int*: 1, \
        char*: 1, \
        default: \
            _Generic((type)0, \
                default: __builtin_types_compatible_p(type, __typeof__(*(void**)0)) \
            ) \
    )

// 判断是否为标量类型（整数、浮点、指针）
#define IS_SCALAR_TYPE(type) \
    (IS_INTEGER_TYPE(type) || IS_FLOAT_TYPE(type) || IS_POINTER_TYPE(type))

// 判断值是否为标量
#define IS_SCALAR(x) IS_SCALAR_TYPE(__typeof__(x))




int va_test_fun();

int average(int count, ...);



#ifdef DLL_EXPORT

extern "C" __declspec(dllexport) FILE _iob[3];

// 标准流宏定义
#define stdin   (&_iob[0])
#define stdout  (&_iob[1])
#define stderr  (&_iob[2])
extern "C" __declspec(dllexport) void* my_calloc(int cnt, int size);
extern "C" __declspec(dllexport) void* my_realloc(void* buf, int size);
extern "C" __declspec(dllexport) void* my_malloc(int size);
extern "C" __declspec(dllexport) void my_free(void* buf);
extern "C" __declspec(dllexport) void my_abort(void);

extern "C" __declspec(dllexport) void* my_memcpy(void* dest, const void* src, size_t n);
extern "C" __declspec(dllexport) void* my_memmove(void* dest, const void* src, size_t n);
extern "C" __declspec(dllexport) void* my_memset(void* ptr, int value, size_t num);
extern "C" __declspec(dllexport) int my_memcmp(const void* ptr1, const void* ptr2, size_t num);

extern "C" __declspec(dllexport) int my_strncmp(const char* str1, const char* str2, size_t n);
extern "C" __declspec(dllexport) char* my_strcpy(char* destination, const char* source);
extern "C" __declspec(dllexport) size_t my_strlen(const char* str);
extern "C" __declspec(dllexport) int my_strcmp(const char* str1, const char* str2);
extern "C" __declspec(dllexport) char* my_strcat(char* destination, const char* source);
extern "C" __declspec(dllexport) char* my_strstr(const char* haystack, const char* needle);

extern "C" __declspec(dllexport) wchar_t* my_wmemcpy(wchar_t* dest, const wchar_t* src, size_t n);
extern "C" __declspec(dllexport) wchar_t* my_wcsstr(const wchar_t* haystack, const wchar_t* needle);
extern "C" __declspec(dllexport) size_t my_wcslen(const wchar_t* str);
extern "C" __declspec(dllexport) int my_wcscmp(const wchar_t* str1, const wchar_t* str2);
extern "C" __declspec(dllexport) wchar_t* my_wcscat(wchar_t* dest, const wchar_t* src);
extern "C" __declspec(dllexport) wchar_t* my_wcscpy(wchar_t* dest, const wchar_t* src);

extern "C" __declspec(dllexport) FILE* my_fopen(const char* filename, const char* mode);
extern "C" __declspec(dllexport) int my_fclose(FILE * stream);
extern "C" __declspec(dllexport) int my_fwrite(const void* buf, int cnt, int size, FILE* fp);
extern "C" __declspec(dllexport) int my_fread(void* buf, int cnt, int size, FILE* fp);

extern "C" __declspec(dllexport) int my_printf(const char* format, ...);
extern "C" __declspec(dllexport) int my_fprintf(FILE* stream, const char* format, ...);

extern "C" __declspec(dllexport) int my_fputc(int character, FILE * stream);
extern "C" __declspec(dllexport) int my_fputs(const char* str, FILE * stream);
extern "C" __declspec(dllexport) char* my_fgets(char* str, int n, FILE * stream);
extern "C" __declspec(dllexport) char* my_fgetc(char* str, int n, FILE * stream);
#else
extern "C" __declspec(dllimport) FILE _iob[];

// 标准流宏定义
#define stdin   (&_iob[0])
#define stdout  (&_iob[1])
#define stderr  (&_iob[2])

extern "C" __declspec(dllimport) void* my_calloc(int cnt, int size);
extern "C" __declspec(dllimport) void* my_realloc(void* buf, int size);
extern "C" __declspec(dllimport) void* my_malloc(int size);

extern "C" __declspec(dllimport) void my_free(void* buf);

extern "C" __declspec(dllimport) void my_abort(void);

extern "C" __declspec(dllimport) void* my_memcpy(void* dest, const void* src, size_t n);
extern "C" __declspec(dllimport) void* my_memmove(void* dest, const void* src, size_t n);
extern "C" __declspec(dllimport) void* my_memset(void* ptr, int value, size_t num);
extern "C" __declspec(dllimport) int my_memcmp(const void* ptr1, const void* ptr2, size_t num);

extern "C" __declspec(dllimport) char* my_strcpy(char* destination, const char* source);
extern "C" __declspec(dllimport) size_t my_strlen(const char* str);
extern "C" __declspec(dllimport) int my_strcmp(const char* str1, const char* str2);
extern "C" __declspec(dllimport) int my_strncmp(const char* str1, const char* str2, size_t n);
extern "C" __declspec(dllimport) char* my_strcat(char* destination, const char* source);
extern "C" __declspec(dllimport) char* my_strstr(const char* haystack, const char* needle);

extern "C" __declspec(dllimport) wchar_t* my_wmemcpy(wchar_t* dest, const wchar_t* src, size_t n);
extern "C" __declspec(dllimport) wchar_t* my_wcsstr(const wchar_t* haystack, const wchar_t* needle);
extern "C" __declspec(dllimport) size_t my_wcslen(const wchar_t* str);
extern "C" __declspec(dllimport) int my_wcscmp(const wchar_t* str1, const wchar_t* str2);
extern "C" __declspec(dllimport) wchar_t* my_wcscat(wchar_t* dest, const wchar_t* src);
extern "C" __declspec(dllimport) wchar_t* my_wcscpy(wchar_t* dest, const wchar_t* src);

extern "C" __declspec(dllimport) FILE * my_fopen(const char* filename, const char* mode);
extern "C" __declspec(dllimport) int my_fclose(FILE * stream);
extern "C" __declspec(dllimport) int my_fwrite(const void* buf, int cnt, int size, FILE * fp);
extern "C" __declspec(dllimport) int my_fread(void* buf, int cnt, int size, FILE * fp);

extern "C" __declspec(dllimport) int my_printf(const char* format, ...);
extern "C" __declspec(dllimport) int my_fprintf(FILE * stream, const char* format, ...);

extern "C" __declspec(dllimport) int my_fputc(int character, FILE * stream);
extern "C" __declspec(dllimport) int my_fputs(const char* str, FILE * stream);
extern "C" __declspec(dllimport) char* my_fgets(char* str, int n, FILE * stream);
extern "C" __declspec(dllimport) char* my_fgetc(char* str, int n, FILE * stream);

#endif




#endif
