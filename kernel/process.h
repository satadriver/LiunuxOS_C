#pragma once
#include "def.h"
#include "descriptor.h"
#include "page.h"
#include "malloc.h"
#include "video.h"


#define DOS_COM_FILE		0
#define DOS_EXE_FILE		1
#define WINDOWS_EXE_FILE	2
#define WINDOWS_DLL_FILE	2
#define LINUX_ELF_FILE		3

#pragma pack(push,1)


typedef struct
{
	unsigned char	opcode;
	DWORD			offset;
	unsigned short	selector;
}JUMP32, *LPJUMP32;




#pragma pack(pop)

#pragma pack(1)
typedef struct  alignas(16)
{
	TSS tss;

	int cpuid;

	DWORD pid;

	DWORD tid;

	DWORD ppid;

	DWORD level;

	DWORD sleep;

	//物理地址而不是线性地址
	DWORD moduleaddr;				//可执行模块物理基地址，用于区别进程和线程
	DWORD moduleLinearAddr;			//可执行模块线性基地址，用于区别进程和线程

	DWORD espbase;

	DWORD heapbase;

	DWORD heapsize;

	DWORD vaddr;

	//内存分配的虚拟地址偏移
	DWORD vasize;

	DWORD errorno;

	DWORD counter;

	DWORD status;

	int retValue;

	int window;

	char* videoBase;
	int showX;
	int showY;

	DWORD dr0;
	DWORD dr1;
	DWORD dr2;
	DWORD dr3;
	DWORD dr6;
	DWORD dr7;

	char fpu;
	char copyMap;

	char filename[256];

	char funcname[64];

}PROCESS_INFO,*LPPROCESS_INFO;

#pragma pack()

#pragma pack(push,1)
typedef struct
{
	DWORD cmd;
	DWORD addr;
	DWORD filesize;
	char filename[256];
}TASKCMDPARAMS, *LPTASKCMDPARAMS;

typedef struct {
	DWORD eip;
	DWORD cs;
	DWORD eflags; 
}RETUTN_ADDRESS_0;

typedef struct {
	RETUTN_ADDRESS_0 ret0;
	DWORD esp3;
	DWORD ss3;
}RETUTN_ADDRESS_3;

typedef struct {
	RETUTN_ADDRESS_3 ret3;
	DWORD es;
	DWORD ds;
	DWORD fs;
	DWORD gs;

}RETUTN_ADDRESS_V86;

typedef struct
{
	DWORD terminate;		//ret address
	DWORD terminate2;		//param 1
	DWORD tid;
	char * filename;
	char * funcname;
	LPTASKCMDPARAMS lpcmdparams;
	char szFileName[256];
	char szFuncName[64];

	TASKCMDPARAMS cmdparams;
}TASKPARAMS, *LPTASKPARAMS;

typedef struct
{
	DWORD terminate;
	DWORD pid;
	char * filename;
	char * funcname;
	DWORD addr;
	DWORD param;
	char szFileName[256];
	char szFuncName[64];
}TASKDOSPARAMS, *LPTASKDOSPARAMS;

typedef struct {
	int number;
	LPPROCESS_INFO lptss;
}TASKRESULT,*LPTASKRESULT;

#pragma pack(pop)

int __initProcess(LPPROCESS_INFO tss, int num, DWORD filedata, char * filename, char * funcname, DWORD level, DWORD runparam);



void __kFreeProcess(int pid);

#ifdef DLL_EXPORT
extern "C" __declspec(dllexport) void __terminateProcess(int pid, char * filename, char * funcname, DWORD lpparams);
extern "C" __declspec(dllexport)int __kCreateProcessFromAddrFunc(DWORD filedata, int filesize, char * funcname,int syslevel, DWORD params);
extern "C" __declspec(dllexport)int __kCreateProcessFromName(char * filename, char * funcname, int syslevel, DWORD params);
extern "C" __declspec(dllexport)int __kCreateProcess(DWORD addr, int datasize, char * filename, char * funcname, int syslevel, DWORD param);
#else
extern "C" __declspec(dllimport) void __terminateProcess(int pid, char * filename, char * funcname, DWORD lpparams);
extern "C" __declspec(dllimport)int __kCreateProcessFromAddrFunc(DWORD filedata, int filesize, char * funcname, int syslevel, DWORD params);
extern "C" __declspec(dllimport)int __kCreateProcessFromName(char * filename, char * funcname, int syslevel, DWORD params);
extern "C" __declspec(dllimport)int __kCreateProcess(DWORD addr, int datasize, char * filename, char * funcname, int syslevel, DWORD param);
#endif



