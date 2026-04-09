#pragma once
#include "descriptor.h"
#include "process.h"

#pragma pack(1)

typedef struct {
	unsigned int addr;
	char name[256];
}DLLMODULEINFO,*LPDLLMODULEINFO;


typedef struct {
	LIST_ENTRY list;
	LPPROCESS_INFO node;
	DWORD valid;
}TASK_LIST_ENTRY;


typedef struct {
	DWORD ss;	//0
	DWORD gs;	//4
	DWORD fs;	//8
	DWORD es;	//12
	DWORD ds;	//16
	DWORD edi;
	DWORD esi;
	DWORD ebp;
	DWORD esp;	//32
	DWORD ebx;
	DWORD edx;
	DWORD ecx;
	DWORD eax;	//48
	DWORD eip;	
	DWORD cs;	
	DWORD eflags;	
	DWORD esp3;	//64
	DWORD ss3;
	DWORD es_v86;
	DWORD ds_v86;
	DWORD fs_v86;
	DWORD gs_v86;
}LIGHT_ENVIRONMENT;

#pragma pack()


#define DOS_PROCESS_RUNCODE		0X80000000

#define TASK_OVER				0
#define TASK_RUN				1
#define TASK_SUSPEND			2
#define TASK_TERMINATE			4


void clearTssBuf(LPPROCESS_INFO tss);



extern "C" int g_last_task_tid[TASK_LIMIT_TOTAL];

void tasktest();

LPPROCESS_INFO MultipleTssSchedule(LIGHT_ENVIRONMENT* env);
LPPROCESS_INFO SingleTssSchedule(LIGHT_ENVIRONMENT* env);

extern "C" void __declspec(dllexport) yield(LIGHT_ENVIRONMENT * stack);

int __pausePid(int pid);

int __resumePid(int pid);

int __resumeTid(int tid);

int __pauseTid(int tid);

void debugReg(PROCESS_INFO* next, PROCESS_INFO* prev);

TASK_LIST_ENTRY* GetTaskListHeader();
void InitTaskList();
void InitTaskArray();
TASK_LIST_ENTRY* InsertTaskList(int tid);

TASK_LIST_ENTRY* RemoveTaskListTid(int tid);

TASK_LIST_ENTRY* RemoveTaskListPid(int pid);

//void __terminateTask(int pid, char * pname, char * funcname, DWORD lpparams);

void SetIVTVector();

extern "C" void ApTaskSchedule(LIGHT_ENVIRONMENT* stack);

int __initTask0(char* filename, char* funcname,int showx,int showy);

int __getFreeTask(LPTASKRESULT ret);


PROCESS_INFO* GetNextProcess();


#ifdef DLL_EXPORT

extern "C" __declspec(dllexport) unsigned long long g_cpu_prev_tick[TASK_LIMIT_TOTAL];
extern "C" __declspec(dllexport) unsigned long long g_cpu_tick[TASK_LIMIT_TOTAL];
extern "C" __declspec(dllexport) unsigned long long g_cpu_start_tick[TASK_LIMIT_TOTAL];

extern "C"  __declspec(dllexport) DWORD __kTaskSchedule(LIGHT_ENVIRONMENT*);

extern "C"  __declspec(dllexport) int __terminateTid(int tid);

extern "C"  __declspec(dllexport) int __terminatePid(int pid);

extern "C"  __declspec(dllexport) int __terminateByFileName(char * filename);

extern "C"  __declspec(dllexport) int __terminateByFuncName(char * funcname);

extern "C"  __declspec(dllexport) PROCESS_INFO *  __findProcessByTid(int tid);

extern "C"  __declspec(dllexport) PROCESS_INFO *  __findProcessByPid(int pid);

extern "C"  __declspec(dllexport) PROCESS_INFO *  __findProcessFileName(char * filename);

extern "C"  __declspec(dllexport) PROCESS_INFO *  __findProcessFuncName(const char * funcname);

extern "C"  __declspec(dllexport) void enter_task_array_lock();
extern "C"  __declspec(dllexport) void leave_task_array_lock();
extern "C"  __declspec(dllexport) void enter_task_list_lock();
extern "C"  __declspec(dllexport) void leave_task_list_lock();

extern "C"  __declspec(dllexport) void enter_task_array_lock();
extern "C"  __declspec(dllexport) void leave_task_array_lock();
extern "C"  __declspec(dllexport) void enter_task_list_lock();
extern "C"  __declspec(dllexport) void leave_task_list_lock();

extern "C"  __declspec(dllexport) void enter_task_array_lock_id(int id);

extern "C"  __declspec(dllexport) void leave_task_array_lock_id(int id);

extern "C" __declspec(dllexport) int __kKernelProcess(LIGHT_ENVIRONMENT * stack);
#else
extern "C" __declspec(dllimport) unsigned long long g_cpu_prev_tick[TASK_LIMIT_TOTAL];
extern "C" __declspec(dllimport) unsigned long long g_cpu_tick[TASK_LIMIT_TOTAL];
extern "C" __declspec(dllimport) unsigned long long g_cpu_start_tick[TASK_LIMIT_TOTAL];

extern "C"  __declspec(dllimport) DWORD __kTaskSchedule(LIGHT_ENVIRONMENT*);

extern "C"  __declspec(dllimport) int __terminateTid(int tid);

extern "C"  __declspec(dllimport) int __terminatePid(int pid);

extern "C"  __declspec(dllimport) int __terminateByFileName(char * filename);

extern "C"  __declspec(dllimport) int __terminateByFuncName(char * funcname);

extern "C"  __declspec(dllimport) PROCESS_INFO * __findProcessByTid(int tid);

extern "C"  __declspec(dllimport) PROCESS_INFO *  __findProcessByPid(int pid);

extern "C"  __declspec(dllimport) PROCESS_INFO *  __findProcessFileName(char * filename);

extern "C"  __declspec(dllimport) PROCESS_INFO *  __findProcessFuncName(const char * funcname);

extern "C"  __declspec(dllimport) void enter_task_array_lock();
extern "C"  __declspec(dllimport) void leave_task_array_lock();
extern "C"  __declspec(dllimport) void enter_task_list_lock();
extern "C"  __declspec(dllimport) void leave_task_list_lock();

extern "C"  __declspec(dllimport) void enter_task_array_lock();
extern "C"  __declspec(dllimport) void leave_task_array_lock();
extern "C"  __declspec(dllimport) void enter_task_list_lock();
extern "C"  __declspec(dllimport) void leave_task_list_lock();
extern "C"  __declspec(dllimport) void enter_task_array_lock_id(int id);

extern "C"  __declspec(dllimport) void leave_task_array_lock_id(int id);

extern "C" __declspec(dllimport) int __kKernelProcess(LIGHT_ENVIRONMENT * stack);
#endif



// data
//cpl <= dpl,rpl >= cpl
//stack
//cpl == dpl == rpl
//code 
//cpl <= dpl,rpl >= cpl
//int gate and trap gate
//cpl <= dpl, rpl in intgate >=  dpl that in code descriptor
//call gate
//cpl <= dpl, rpl in callgate >=  dpl that in code descriptor
//task gate
//cpl <= dpl, rpl in taskgate >=  dpl that in code descriptor

//CPL是当前进程的权限级别(Current Privilege Level)，是当前正在执行的代码所在的段的特权级，存在于cs寄存器的低两位
//RPL说明的是进程对段访问的请求权限(Request Privilege Level)，是对于段选择子而言的，每个段选择子有自己的RPL，
//它说明的是进程对段访问的请求权限，有点像函数参数
//DPL存储在段描述符中，规定访问该段的权限级别(Descriptor Privilege Level)，每个段的DPL固定

//CPL <= DPL (门): 当前运行级不能低于门，如果是外部中断或CPU异常会免去这一判断
//DPL0陷阱门: 用于CPU异常，DPL为0，不允许用户态直接使用int指令访问，硬件中断免去这一判断，因此可以在用户产生CPU异常
//如果是由INT n指令或INTO指令引起转移，还要检查中断门、陷阱门或任务门描述符中的DPL是否满足CPL<=DPL(对于其它的异常或中断，门中的DPL被 忽略)