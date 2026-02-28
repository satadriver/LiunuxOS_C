

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

#ifdef _iobuf
#undef _iobuf
#endif



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

// _iob 数组声明




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


#ifdef DLL_EXPORT

extern "C" __declspec(dllexport) FILE _iob[3];

// 标准流宏定义
#define stdin   (&_iob[0])
#define stdout  (&_iob[1])
#define stderr  (&_iob[2])

extern "C" __declspec(dllexport) void* memcpy(void* dest, const void* src, size_t n);

extern "C" __declspec(dllexport) void* memmove(void* dest, const void* src, size_t n);

extern "C" __declspec(dllexport) void* memset(void* ptr, int value, size_t num);

extern "C" __declspec(dllexport) int memcmp(const void* ptr1, const void* ptr2, size_t num);
extern "C" __declspec(dllexport) int strncmp(const char* str1, const char* str2, size_t n);
extern "C" __declspec(dllexport) char* strcpy(char* destination, const char* source);
extern "C" __declspec(dllexport) size_t strlen(const char* str);
extern "C" __declspec(dllexport) int strcmp(const char* str1, const char* str2);
extern "C" __declspec(dllexport) char* strcat(char* destination, const char* source);
extern "C" __declspec(dllexport) char* strstr(const char* haystack, const char* needle);
extern "C" __declspec(dllexport) wchar_t* wmemcpy(wchar_t* dest, const wchar_t* src, size_t n);
extern "C" __declspec(dllexport) wchar_t* wcsstr(const wchar_t* haystack, const wchar_t* needle);
extern "C" __declspec(dllexport) size_t wcslen(const wchar_t* str);
extern "C" __declspec(dllexport) int wcscmp(const wchar_t* str1, const wchar_t* str2);
extern "C" __declspec(dllexport) wchar_t* wcscat(wchar_t* dest, const wchar_t* src);

extern "C" __declspec(dllexport) int printf(const char* format, ...);

extern "C" __declspec(dllexport) void* calloc(int cnt,int size);

extern "C" __declspec(dllexport) void* malloc(int size);

extern "C" __declspec(dllexport) void* realloc(void*buf,int size);

extern "C" __declspec(dllexport) void free(void* buf);

extern "C" __declspec(dllexport) void abort(void);

extern "C" __declspec(dllexport) FILE* fopen(const char* filename, const char* mode);
extern "C" __declspec(dllexport) int fclose(FILE * stream);
extern "C" __declspec(dllexport) int fwrite(const void* buf, int cnt, int size, FILE* fp);
extern "C" __declspec(dllexport) int fread(void* buf, int cnt, int size, FILE* fp);
extern "C" __declspec(dllexport) int fprintf(FILE* stream, const char* format, ...);

extern "C" __declspec(dllexport) int fputc(int character, FILE * stream);
extern "C" __declspec(dllexport) int fputs(const char* str, FILE * stream);

extern "C" __declspec(dllexport) char* fgets(char* str, int n, FILE * stream);

extern "C" __declspec(dllexport) int abs(int x);
extern "C" __declspec(dllexport) double pown(double x, int n);
extern "C" __declspec(dllexport) double cos(double x);
extern "C" __declspec(dllexport) double sin(double x);
extern "C" __declspec(dllexport) double pow(double a, int b);
extern "C" __declspec(dllexport) double sqrt(double x);

extern "C" __declspec(dllexport) float sqrtf(float x);
extern "C" __declspec(dllexport) float cosf(float x);
extern "C" __declspec(dllexport) float sinf(float x);

extern "C" __declspec(dllexport) float fabsf(float x);
extern "C" __declspec(dllexport) float fabs(float x);

extern "C" __declspec(dllexport) double log(double x);

extern "C" __declspec(dllexport) double exp(double x);

extern "C" __declspec(dllexport) float logf(float x);

extern "C" __declspec(dllexport) float expf(float x);

#else
extern "C" __declspec(dllimport) FILE _iob[];

// 标准流宏定义
#define stdin   (&_iob[0])
#define stdout  (&_iob[1])
#define stderr  (&_iob[2])

extern "C" __declspec(dllimport) void* memcpy(void* dest, const void* src, size_t n);

extern "C" __declspec(dllimport) void* memmove(void* dest, const void* src, size_t n);

extern "C" __declspec(dllimport) void* memset(void* ptr, int value, size_t num);

extern "C" __declspec(dllimport) int memcmp(const void* ptr1, const void* ptr2, size_t num);
extern "C" __declspec(dllimport) int strncmp(const char* str1, const char* str2, size_t n);
extern "C" __declspec(dllimport) char* strcpy(char* destination, const char* source);
extern "C" __declspec(dllimport) size_t strlen(const char* str);
extern "C" __declspec(dllimport) int strcmp(const char* str1, const char* str2);
extern "C" __declspec(dllimport) char* strcat(char* destination, const char* source);
extern "C" __declspec(dllimport) char* strstr(const char* haystack, const char* needle);
extern "C" __declspec(dllimport) wchar_t* wmemcpy(wchar_t* dest, const wchar_t* src, size_t n);
extern "C" __declspec(dllimport) wchar_t* wcsstr(const wchar_t* haystack, const wchar_t* needle);
extern "C" __declspec(dllimport) size_t wcslen(const wchar_t* str);
extern "C" __declspec(dllimport) int wcscmp(const wchar_t* str1, const wchar_t* str2);
extern "C" __declspec(dllimport) wchar_t* wcscat(wchar_t* dest, const wchar_t* src);

extern "C" __declspec(dllimport) int printf(const char* format, ...);

extern "C" __declspec(dllimport) void* calloc(int cnt, int size);

extern "C" __declspec(dllimport) void* malloc(int size);

extern "C" __declspec(dllimport) void* realloc(void* buf, int size);

extern "C" __declspec(dllimport) void free(void* buf);

extern "C" __declspec(dllimport) void abort(void);

extern "C" __declspec(dllimport) FILE * fopen(const char* filename, const char* mode);
extern "C" __declspec(dllimport) int fclose(FILE * stream);
extern "C" __declspec(dllimport) int fwrite(const void* buf, int cnt, int size, FILE * fp);
extern "C" __declspec(dllimport) int fread(void* buf, int cnt, int size, FILE * fp);
extern "C" __declspec(dllimport) int fprintf(FILE * stream, const char* format, ...);

extern "C" __declspec(dllimport) int fputc(int character, FILE * stream);
extern "C" __declspec(dllimport) int fputs(const char* str, FILE * stream);

extern "C" __declspec(dllimport) char* fgets(char* str, int n, FILE * stream);

extern "C" __declspec(dllimport) double abs(double x);
extern "C" __declspec(dllimport) double pown(double x, int n);
extern "C" __declspec(dllimport) double cos(double x);
extern "C" __declspec(dllimport) double sin(double x);
extern "C" __declspec(dllimport) double pow(double a, int b);
extern "C" __declspec(dllimport) double sqrt(double x);

extern "C" __declspec(dllimport) float sqrtf(float x);
extern "C" __declspec(dllimport) float cosf(float x);
extern "C" __declspec(dllimport) float sinf(float x);

extern "C" __declspec(dllimport) float fabsf(float x);
extern "C" __declspec(dllimport) float fabs(float x);

extern "C" __declspec(dllimport) double log(double x);

extern "C" __declspec(dllimport) double exp(double x);

extern "C" __declspec(dllimport) float logf(float x);

extern "C" __declspec(dllimport) float expf(float x);
#endif

#endif