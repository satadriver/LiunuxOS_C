#pragma once

#include "def.h"
#include "task.h"

#pragma pack(1)

typedef struct {

	DWORD ss0;
	DWORD gs;	//4
	DWORD fs;	//8
	DWORD es;	//12
	DWORD ds;	//16
	DWORD edi;	//20
	DWORD esi;	//24
	DWORD ebp;	//28
	DWORD esp0;	//32
	DWORD ebx;	//36
	DWORD edx;	//40
	DWORD ecx;	//44
	DWORD eax;	//48

	DWORD		errcode;	//52
	DWORD		eip;		//56
	DWORD		cs;			//60
	DWORD		eflags;		//64
	DWORD		esp3;		//68
	DWORD		ss3;		//72
	DWORD		es_v86;
	DWORD		ds_v86;
	DWORD		fs_v86;
	DWORD		gs_v86;
}EXCEPTIONCODESTACK, *LPEXCEPTIONCODESTACK;

typedef struct {

	DWORD ss0;
	DWORD gs;	//4
	DWORD fs;	//8
	DWORD es;	//12
	DWORD ds;	//16
	DWORD edi;	//20
	DWORD esi;	//24
	DWORD ebp;	//28
	DWORD esp0;	//32
	DWORD ebx;	//36
	DWORD edx;	//40
	DWORD ecx;	//44
	DWORD eax;	//48

	DWORD		eip;		//52
	DWORD		cs;			//56
	DWORD		eflags;		//60
	DWORD		esp3;		//64
	DWORD		ss3;		//68
	DWORD		es_v86;
	DWORD		ds_v86;
	DWORD		fs_v86;
	DWORD		gs_v86;
}EXCEPTIONSTACK, *LPEXCEPTIONSTACK;

#pragma pack()




#ifdef DLL_EXPORT

extern "C" __declspec(dllexport) void __kException(const char* descriptor, int num, LIGHT_ENVIRONMENT * tss);
#else

extern "C" __declspec(dllimport) void __kException(const char* descriptor, int num, LIGHT_ENVIRONMENT * tss);
#endif