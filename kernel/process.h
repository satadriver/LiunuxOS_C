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


typedef struct {
	int heapSize;
	int heapCnt;
	int heapLock;
	char* fastHeap;
	char ** lpheapBase;
	char* HeapBase;
}ProcessHeap;


typedef struct 
{
	TSS tss;
	//char unused[3];

	char level;
	char fpu;
	char copyMap;

	int cpuid;

	int fcpu;

	DWORD pid;

	DWORD tid;

	DWORD ppid;

	DWORD status;

	DWORD espbase;	//esp base address,esp is esp top but not equal to base !

	DWORD moduleBase;

	DWORD vaddr;

	//ÄÚ´æ·ÖÅäµÄÐéÄâµØÖ·Æ«ÒÆ
	DWORD * lpvasize;

	DWORD errorno;

	int priority;
	int delta;
	int authority;

	unsigned long long tick;
	unsigned long long tick_total;
	unsigned long long prev_tick;
	unsigned long long tick_start;
	unsigned long long tick_cost;

	DWORD sleep;

	DWORD sleep_total;

	DWORD counter;

	DWORD slice;

	DWORD frac_slice;

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

	char* fast_heap_large;
	int large_heap_size;
	char* fast_heap;
	int* lpHeapCnt;
	int* lpheap_lock;
	DWORD heapsize;
	char*** lpHeapBase;

	char filename[256];
	char funcname[64];
	DWORD va_size;
	int heapCnt;
	int heap_lock;
	char* heapBase[12];

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
extern "C" __declspec(dllexport)int __kCreateProcessFromAddr(DWORD filedata, int filesize, char * funcname,int syslevel, DWORD params);
extern "C" __declspec(dllexport)int __kCreateProcessFromName(char * filename, char * funcname, int syslevel, DWORD params);
extern "C" __declspec(dllexport)int __kCreateProcess(DWORD addr, int datasize, char * filename, char * funcname, int syslevel, DWORD param);
#else
extern "C" __declspec(dllimport) void __terminateProcess(int pid, char * filename, char * funcname, DWORD lpparams);
extern "C" __declspec(dllimport)int __kCreateProcessFromAddr(DWORD filedata, int filesize, char * funcname, int syslevel, DWORD params);
extern "C" __declspec(dllimport)int __kCreateProcessFromName(char * filename, char * funcname, int syslevel, DWORD params);
extern "C" __declspec(dllimport)int __kCreateProcess(DWORD addr, int datasize, char * filename, char * funcname, int syslevel, DWORD param);
#endif



