#include "Thread.h"
#include "Utils.h"
#include "video.h"
#include "Pe.h"
#include "process.h"
#include "peVirtual.h"
#include "memory.h"
#include "apic.h"
#include "window.h"
#include "core.h"
#include "systemService.h"
#include "taskpriority.h"
#include "malloc.h"
#include "coprocessor.h"



//any thread can call this function to terminate self
//any thread can call this with tid to terminate other thread
//above so,the most import element is dwtid

#ifdef TASK_SWITCH_ARRAY
extern "C" __declspec(dllexport) DWORD __kTerminateThread(int tid, char* filename, char* funcname, DWORD lpparams) {

	char szout[256];

	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetTaskTssBase();
	LPPROCESS_INFO current = (LPPROCESS_INFO)GetCurrentTaskTssBase();

	int pid = tss[tid].pid;

	if (tid < 0 || tid >= TASK_LIMIT_TOTAL || tss[tid].tid != tid) {
		__printf(szout, "__kTerminateThread tid:%x,pid:%x,current pid:%x,current tid:%x,filename:%s,funcname:%s\n",
			tid, pid, current->pid, current->tid, filename, funcname);
		return 0;
	}

	enter_task_array_lock();
	if (current->tid == tid)
	{
		current->status = TASK_OVER;
	}
	else {
		//do nothing
	}

	tss[tid].status = TASK_OVER;
	
	if (tss[tid].level >= 3 && (tss[tid].tss.eflags & 0x20000) && tss[tid].stackbase) {
		__kFree(tss[tid].stackbase);
	}

	int cpu = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	//DestroyThreadWindow(tid, cpu);

	leave_task_array_lock();

	//__yield();

	__sleep(-1);

	return 0;
}
#else
extern "C" __declspec(dllexport) DWORD __kTerminateThread(int tid, char* filename, char* funcname, DWORD lpparams) {

	MarkTaskListTid(tid,1);

	while (1)
		Halt();
	//__sleep(-1);
	
	return 0;
}
#endif



DWORD __kCreateThread(DWORD addr, DWORD module, DWORD runparam, char* funcname) {
	int cpu = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	return __kCreateRemoteThread(cpu, addr, module, runparam, funcname);
}

DWORD __kCreateRemoteThread(int cpu,DWORD addr, DWORD module, DWORD runparam,char * funcname) {

	int ret = 0;

	char szout[256];

	TASKRESULT freetask;

#ifdef TASK_SWITCH_ARRAY
	enter_task_array_lock();
	ret = __GetFreeTask(&freetask, cpu);
	if (ret == FALSE)
	{
		leave_task_array_lock();
		return FALSE;
	}
#else
	PROCESS_INFO_LIST* list = (PROCESS_INFO_LIST*)__kMalloc(sizeof(PROCESS_INFO_LIST));
	PROCESS_INFO * lptss = &list->proc;
	lptss->cpuid = cpu;
	lptss->status = TASK_SUSPEND;
	freetask.lptss = lptss;
	freetask.number = GenPid();
#endif

	LPPROCESS_INFO process = (LPPROCESS_INFO)GetCurrentTaskTssBaseId(cpu);
	LPPROCESS_INFO tss = freetask.lptss;
	//__memset((char*)tss->tss.intMap, 0, sizeof(tss->tss.intMap));
	//__memset((char*)tss->tss.iomap, 0, sizeof(tss->tss.iomap));
	int tssSize = GetTssSize();
	__memcpy((char*)tss, (char*)process, tssSize);
	tss->copyMap = 0;
#ifdef SINGLE_TASK_TSS
	tss->tss.trap = 0;
#else
	tss->tss.trap = 0;
#endif

	tss->tss.cr3 = process->tss.cr3;

	tss->heapCnt = process->heapCnt;
	tss->lpHeapCnt = process->lpHeapCnt;
	tss->lpHeapBase = process->lpHeapBase;
	tss->heapBase[0] = process->heapBase[0];
	tss->fast_heap = process->fast_heap;
	tss->heapsize = process->heapsize;
	tss->lpheap_lock = process->lpheap_lock;
	tss->heap_lock = process->heap_lock;
#ifdef TASK_SWITCH_ARRAY
	tss->vid = freetask.number + (cpu << 16);
#else
	tss->vid = freetask.number;
#endif
	tss->pid = process->pid;
	tss->ppid = process->pid;
	tss->tid = freetask.number;
	tss->moduleBase = module;

	tss->cpuid = cpu;

	//tss->fpu = 0;
	tss->lock = 0;
	tss->lplock = &tss->lock;
	tss->level = process->level;
	tss->vaddr = process->vaddr;
	tss->lpvasize = process->lpvasize;
	tss->va_size = process->va_size;
	tss->tss.eflags = process->tss.eflags;
	int level = process->level;
	if (level)
	{
		tss->tss.eflags = tss->tss.eflags | ((level & 3) << 12);
	}
	tss->status = TASK_SUSPEND;
	tss->tss.link = 0;	
	tss->tss.iomapOffset = 136;
	tss->tss.iomapEnd = 0xff;

	tss->sleep = 0;

	tss->tss.eip = addr;
	
	tss->tss.eax = 0;
	tss->tss.ecx = 0;
	tss->tss.edx = 0;
	tss->tss.ebx = 0;
	tss->tss.esi = 0;
	tss->tss.edi = 0;
	tss->dr0 = 0;
	tss->dr1 = 0;
	tss->dr2 = 0;
	tss->dr3 = 0;
	tss->dr6 = 0xFFFF0FF0;
	tss->dr7 = 0x400;

	if (tss->level) {
		unsigned long esp0 = __kProcessMalloc(TASK_STACK0_SIZE, 0, process->pid, tss->cpuid, 0, PAGE_READWRITE | PAGE_USERPRIVILEGE | PAGE_PRESENT | 0x80000000);
		tss->tss.esp0 = esp0 + TASK_STACK0_SIZE - STACK_TOP_DUMMY;
	}
	tss->tss.ss0 = KERNEL_MODE_STACK;

	DWORD vaddr = tss->vaddr + *tss->lpvasize;
	LPTASKPARAMS params = 0;
	unsigned long espbase = 0;
	if (tss->level == 0)
	{
		tss->tss.ds = KERNEL_MODE_DATA;
		tss->tss.es = KERNEL_MODE_DATA;
		tss->tss.fs = KERNEL_MODE_DATA;
		tss->tss.gs = KERNEL_MODE_DATA;
		tss->tss.cs = KERNEL_MODE_CODE;
		tss->tss.ss = KERNEL_MODE_STACK;

		espbase = __kProcessMalloc(KTASK_STACK_SIZE, 0, process->pid,tss->cpuid,vaddr, 
			PAGE_READWRITE | PAGE_USERPRIVILEGE | PAGE_PRESENT | 0x80000000);
		if (espbase == FALSE)
		{
			tss->status = TASK_OVER;
			leave_task_array_lock();
			return FALSE;
		}
#ifndef DISABLE_PAGE_MAPPING
		tss->tss.esp = (DWORD)vaddr + KTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
		tss->tss.ebp = (DWORD)vaddr + KTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
#else
		tss->tss.esp = (DWORD)espbase + KTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
		tss->tss.ebp = (DWORD)espbase + KTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
#endif
		params = (LPTASKPARAMS)(espbase + KTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS));

#ifdef SINGLE_TASK_TSS
		RETUTN_ADDRESS_0* ret0 = (RETUTN_ADDRESS_0*)((char*)params - sizeof(RETUTN_ADDRESS_0));
		ret0->cs = tss->tss.cs;
		ret0->eip = tss->tss.eip;
		ret0->eflags = tss->tss.eflags;
		tss->tss.esp = (DWORD)ret0;
		tss->tss.ebp = (DWORD)ret0;
#else
		tss->tss.esp = (DWORD)tss->espbase + KTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
		tss->tss.ebp = (DWORD)tss->espbase + KTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
#endif
	}
	else {
		tss->tss.ds = USER_MODE_DATA | tss->level;
		tss->tss.es = USER_MODE_DATA | tss->level;
		tss->tss.fs = USER_MODE_DATA | tss->level;
		tss->tss.gs = USER_MODE_DATA | tss->level;
		tss->tss.cs = USER_MODE_CODE | tss->level;
		tss->tss.ss = USER_MODE_STACK | tss->level;

		espbase = __kProcessMalloc(UTASK_STACK_SIZE, 0,process->pid,tss->cpuid,vaddr, 
			PAGE_READWRITE | PAGE_USERPRIVILEGE | PAGE_PRESENT | 0x80000000);
		if (espbase == FALSE)
		{
			tss->status = TASK_OVER;
			leave_task_array_lock();
			return FALSE;
		}
#ifndef DISABLE_PAGE_MAPPING
		tss->tss.esp = (DWORD)vaddr + UTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
		tss->tss.ebp = (DWORD)vaddr + UTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
#else
		tss->tss.esp = (DWORD)espbase + UTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
		tss->tss.ebp = (DWORD)espbase + UTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
#endif
		params = (LPTASKPARAMS)(espbase + UTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS));

#ifdef SINGLE_TASK_TSS
		RETUTN_ADDRESS_3* ret3 = (RETUTN_ADDRESS_3*)((char*)tss->tss.esp0 - sizeof(RETUTN_ADDRESS_3));
		ret3->ret0.cs = tss->tss.cs;
		ret3->ret0.eip = tss->tss.eip;
		ret3->ret0.eflags = tss->tss.eflags;
		ret3->esp3 = (DWORD)tss->tss.esp;
		ret3->ss3 = tss->tss.ss;

		tss->tss.esp = (DWORD)ret3;
		tss->tss.ebp = (DWORD)ret3;
		tss->tss.ss = KERNEL_MODE_STACK;
#else
		tss->tss.esp = (DWORD)tss->espbase + UTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
		tss->tss.ebp = (DWORD)tss->espbase + UTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
#endif
	}
	tss->stackbase = espbase;

	params->terminate = (DWORD)__kTerminateThread;
	params->terminate2 = (DWORD)__kTerminateThread;
	params->tid = freetask.number;							//param1:pid
	__strcpy(params->szFileName, process->filename);
	params->filename = params->szFileName;		//param2:filename
	__strcpy(params->szFuncName, funcname);
	params->funcname = params->szFuncName;		//param2:filename
	params->lpcmdparams = &params->cmdparams;
	if (runparam)
	{
		__memcpy((char*)params->lpcmdparams, (char*)runparam, sizeof(TASKCMDPARAMS));
	}

	tss->counter = 0;
	tss->sleep = 0;
	tss->sleep_total = 0;

	tss->errorno = 0;

	tss->delta = DYNAMIC_PRIORITY;
	tss->authority = AUTHORITY_PRIORITY;
	tss->priority = process->priority;
	tss->delay = 0;

	tss->tick_run = 0;
	tss->prev_tick = __krdtsc();
	tss->tick_start = __krdtsc();
	tss->tick_total = 0;

	tss->slice = process->slice;
	tss->frac_slice = 0;

	__strcpy(tss->filename, process->filename);
	__strcpy(tss->funcname, funcname);

	tss->showX = 0;
	tss->showY = 0;
	tss->window = 0;
	tss->videoBase = (char*)gGraphBase;

	tss->status = TASK_RUN;
	
#ifdef TASK_SWITCH_ARRAY
	char* fenv = GetFpuBuf(cpu, tss->tid );
	InitTaskCoprocessor(tss, fenv);
	leave_task_array_lock();
#else
	InitTaskCoprocessor(tss, list->fpenv);
	PROCESS_INFO_LIST * father = GetRemoteTaskListPid(cpu,tss->pid,1);
	InsertListTail(&father->child,&list->child);
	InsertTaskList(list);
#endif

	return TRUE;
}


int TraversalThread(PROCESS_INFO_LIST *list) {
	int cnt = 0;
	LIST_ENTRY * hdr = list->child.next;
	LIST_ENTRY * entry = hdr;
	do {
		if (entry == 0)
			break;
		PROCESS_INFO_LIST *listnode = (PROCESS_INFO_LIST*)((char*)entry - OFFSETOF(PROCESS_INFO_LIST, child));

		entry = entry->next;
	} while (hdr != entry);
	return cnt;
}