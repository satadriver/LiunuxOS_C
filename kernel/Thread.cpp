#include "Thread.h"
#include "Utils.h"
#include "video.h"
#include "Pe.h"
#include "process.h"
#include "peVirtual.h"
#include "memory.h"
#include "apic.h"
#include "window.h"

//any thread can call this function to terminate self
//any thread can call this with tid to terminate other thread
//above so,the most import element is dwtid

#ifdef TASK_SWITCH_ARRAY
extern "C" __declspec(dllexport) DWORD __kTerminateThread(int dwtid, char* filename, char* funcname, DWORD lpparams) {

	int tid = dwtid & 0x7fffffff;

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
	
	if (tss[tid].level >= 3 && (tss[tid].tss.eflags & 0x20000) && tss[tid].espbase) {
		__kFree(tss[tid].espbase);
	}

	int retvalue = 0;

	tss[tid].retValue = retvalue;

	int cpu = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	//DestroyThreadWindow(tid, cpu);

	leave_task_array_lock();

	if (dwtid & 0x80000000) {
		return 0;
	}
	else {
		//__sleep(-1);
		__asm {
			//hlt
		}
		__yield();
	}

	return 0;
}
#else
extern "C" __declspec(dllexport) DWORD __kTerminateThread(int dwtid, char* filename, char* funcname, DWORD lpparams) {

	int tid = dwtid & 0x7fffffff;

	RemoveTaskListTid(tid);

	if (dwtid & 0x80000000) {
		return 0;
	}
	else {
		__sleep(-1);
	}
	return 0;
}
#endif

DWORD __kCreateThread(DWORD addr, DWORD module, DWORD runparam,char * funcname) {

	int ret = 0;

	char szout[256];

	int cpu = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

	TASKRESULT freetask;
	ret = __getFreeTask(&freetask);
	if (ret == FALSE)
	{
		return FALSE;
	}

	DWORD imagesize = getSizeOfImage((char*)module);
	DWORD alignsize = getAlignSize(imagesize, PAGE_SIZE);
	
	//如果想要修改父进程的信息，必须在CURRENT_TASK_TSS_BASE中修改，否则线程切换时信息还是会被替换
	LPPROCESS_INFO process = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	LPPROCESS_INFO tss = freetask.lptss;
	__memset((char*)tss->tss.intMap, 0, sizeof(tss->tss.intMap));
	__memset((char*)tss->tss.iomap, 0, sizeof(tss->tss.iomap));
	tss->copyMap = 0;
#ifdef SINGLE_TASK_TSS
	tss->tss.trap = 1;
#else
	tss->tss.trap = 0;
#endif
	tss->tss.trap = 0;

	tss->tss.cr3 = process->tss.cr3;
	tss->heapbase = process->heapbase;
	tss->heapsize = process->heapsize;
	tss->pid = process->pid;
	tss->ppid = process->pid;
	tss->tid = freetask.number;

	tss->cpuid = cpu;

	tss->fpu = 0;
	tss->level = process->level;
	tss->vaddr = process->vaddr;
	tss->vasize = process->vasize;
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
	tss->moduleaddr = linear2phy(module);
	tss->moduleLinearAddr = module;
	
	tss->tss.eax = 0;
	tss->tss.ecx = 0;
	tss->tss.edx = 0;
	tss->tss.ebx = 0;
	tss->tss.esi = 0;
	tss->tss.edi = 0;

	tss->tss.esp0 = TASKS_STACK0_BASE + (TASK_LIMIT_TOTAL * tss->cpuid + tss->tid + 1) * TASK_STACK0_SIZE - STACK_TOP_DUMMY;
	tss->tss.ss0 = KERNEL_MODE_STACK;

	DWORD espsize = 0;
	DWORD vaddr = tss->vaddr + tss->vasize;
	LPTASKPARAMS params = 0;
	if (tss->level == 0)
	{
		tss->tss.ds = KERNEL_MODE_DATA;
		tss->tss.es = KERNEL_MODE_DATA;
		tss->tss.fs = KERNEL_MODE_DATA;
		tss->tss.gs = KERNEL_MODE_DATA;
		tss->tss.cs = KERNEL_MODE_CODE;
		tss->tss.ss = KERNEL_MODE_STACK;

		tss->espbase = __kProcessMalloc(KTASK_STACK_SIZE, &espsize, process->pid,tss->cpuid,vaddr, PAGE_READWRITE | PAGE_USERPRIVILEGE | PAGE_PRESENT);
		if (tss->espbase == FALSE)
		{
			tss->status = TASK_OVER;
			return FALSE;
		}
#ifndef DISABLE_PAGE_MAPPING
		tss->tss.esp = (DWORD)vaddr + KTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
		tss->tss.ebp = (DWORD)vaddr + KTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
#else
		tss->tss.esp = (DWORD)tss->espbase + KTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
		tss->tss.ebp = (DWORD)tss->espbase + KTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
#endif
		params = (LPTASKPARAMS)(tss->espbase + KTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS));

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

		tss->espbase = __kProcessMalloc(UTASK_STACK_SIZE, &espsize,process->pid,tss->cpuid,vaddr, PAGE_READWRITE | PAGE_USERPRIVILEGE | PAGE_PRESENT);
		if (tss->espbase == FALSE)
		{
			tss->status = TASK_OVER;
			return FALSE;
		}
#ifndef DISABLE_PAGE_MAPPING
		tss->tss.esp = (DWORD)vaddr + UTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
		tss->tss.ebp = (DWORD)vaddr + UTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
#else
		tss->tss.esp = (DWORD)tss->espbase + UTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
		tss->tss.ebp = (DWORD)tss->espbase + UTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
#endif
		params = (LPTASKPARAMS)(tss->espbase + UTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS));

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

	__strcpy(tss->filename, process->filename);
	__strcpy(tss->funcname, funcname);

	tss->showX = 0;
	tss->showY = 0;
	tss->window = 0;
	tss->videoBase = (char*)gGraphBase;

	//enter_task_array_lock();
	tss->status = TASK_RUN;
	//leave_task_array_lock();

#ifdef TASK_SWITCH_ARRAY

#else
	InsertTaskList(tss->tid);
#endif
	return TRUE;
}