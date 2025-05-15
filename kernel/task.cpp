#include "process.h"
#include "task.h"
#include "Utils.h"
#include "Kernel.h"
#include "video.h"
#include "Pe.h"
#include "processDOS.h"
#include "file.h"
#include "timer8254.h"
#include "page.h"
#include "def.h"
#include "malloc.h"
#include "core.h"
#include "vectorRoutine.h"
#include "servicesProc.h"


/*
TASK_LIST_ENTRY *gTasksListPtr = 0;

void __terminateTask(int tid, char * filename, char * funcname, DWORD lpparams) {

	removeTaskList(tid);
	__sleep(-1);
}

TASK_LIST_ENTRY* searchTaskList(int tid) {
	TASK_LIST_ENTRY * list = (TASK_LIST_ENTRY*)TASKS_LIST_BASE;
	for (int i = 0; i < TASK_LIMIT_TOTAL; i++)
	{
		if (list[i].valid  && list[i].process->status == TASK_RUN && list[i].process->tid == tid) {
			return &list[i];
		}
	}
	return 0;
}

TASK_LIST_ENTRY* addTaskList(int tid) {
	LPPROCESS_INFO base = (LPPROCESS_INFO)TASKS_TSS_BASE;

	//TASK_LIST_ENTRY* tasklist = (TASK_LIST_ENTRY*)gTasksListPtr;

	TASK_LIST_ENTRY * list = (TASK_LIST_ENTRY*)TASKS_LIST_BASE;
	for (int i = 0; i < TASK_LIMIT_TOTAL; i++)
	{
		if (list[i].valid == 0 ) {
			list[i].valid = TRUE;

			list[i].process = base + tid;
			list[i].process->status = TASK_RUN;
			
			addlistTail((LIST_ENTRY*)&gTasksListPtr->list, (LIST_ENTRY*)&list[i].list);
			return &list[i];
		}
	}
	return 0;
}

TASK_LIST_ENTRY* removeTaskList(int tid) {

	TASK_LIST_ENTRY * list = (TASK_LIST_ENTRY*)gTasksListPtr;
	do 
	{
		if (list->valid && list->process && list->process->tid == tid)
		{
			if (gTasksListPtr == list) {
				//gTasksListPtr = (TASK_LIST_ENTRY*)list->list.next;
			}

			removelist((LIST_ENTRY*)&list->list);

			list->process->status = TASK_OVER;
			list->process = 0;

			list->valid = FALSE;

			return list;
		}
		list = (TASK_LIST_ENTRY *)list->list.next;

	} while (list && list != (TASK_LIST_ENTRY *)gTasksListPtr);

	return 0;
}
*/

//CF(bit 0) [Carry flag]   
//若算术操作产生的结果在最高有效位(most-significant bit)发生进位或借位则将其置1，反之清零。
//这个标志指示无符号整型运算的溢出状态，这个标志同样在多倍精度运算(multiple-precision arithmetic)中使用

//SF(bit 7) [Sign flag]   
//该标志被设置为有符号整型的最高有效位。(0指示结果为正，反之则为负) 

//OF(bit 11) [Overflow flag]   
//如果整型结果是较大的正数或较小的负数，并且无法匹配目的操作数时将该位置1，反之清零。这个标志为带符号整型运算指示溢出状态

//OF是有符号数运算结果的标志
//OF标志：这个标志有点复杂，其结果是CF标志和次最高位是否发生进位（如果进位是1，没进位是0）进行异或的结果
//OF只对有符号数运算有意义，CF对无符号数运算有意义
//MOV AX,858F
//SUB AX,7869

// IOPL是I/O保护机制中的关键之一，它位于EFLAGS寄存器的第12、13位。指令in、ins、out、outs、cli、sti只有在CPL<= IOPL时才能执行。
//这些指令被称为I/O敏感指令，如果特权级低的指令视图访问这些I/O敏感指令将会导致常规保护错误(#GP)
//可以改变IOPL的指令只有popfl和iret指令，但只有运行在特权级0的程序才能将其改变

int g_tagMsg = 0;

void clearTssBuf(LPPROCESS_INFO tss) {
	__memset((CHAR*)tss, 0, sizeof(PROCESS_INFO));
	tss->status = TASK_SUSPEND;

	tss->tss.iomapEnd = 0xff;
	tss->tss.iomapOffset = 136;

	//tss->tss.trap = 1;
}


int __getFreeTask(LPTASKRESULT ret) {
	if (ret == 0)
	{
		return FALSE;
	}
	ret->lptss = 0;
	ret->number = 0;

	LPPROCESS_INFO tss = (LPPROCESS_INFO)TASKS_TSS_BASE;
	for (int i = 0;i < TASK_LIMIT_TOTAL; i++)
	{
		if (tss[i].status == TASK_OVER)
		{
			clearTssBuf(&tss[i]);

			ret->number = i;
			ret->lptss = &tss[i];
			return TRUE;
		}
	}

	return FALSE;
}



LPPROCESS_INFO __findProcessFuncName(char * funcname) {

	LPPROCESS_INFO p = (LPPROCESS_INFO)TASKS_TSS_BASE;
	for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
		if (p[i].status == TASK_RUN && __strcmp(p[i].funcname, funcname) == 0) {
			return & p[i];
		}
	}
	return FALSE;
}

LPPROCESS_INFO __findProcessFileName(char * filename) {
	LPPROCESS_INFO p = (LPPROCESS_INFO)TASKS_TSS_BASE;
	for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
		if (p[i].status == TASK_RUN && __strcmp(p[i].filename, filename) == 0) {
			return &p[i];
		}
	}
	return FALSE;
}



LPPROCESS_INFO __findProcessByPid(int pid) {
	LPPROCESS_INFO p = (LPPROCESS_INFO)TASKS_TSS_BASE;
	for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
		if (p[i].status == TASK_RUN && p[i].pid == pid ) {
			return &p[i];
		}
	}
	return FALSE;
}


LPPROCESS_INFO __findProcessByTid(int tid) {
	LPPROCESS_INFO p = (LPPROCESS_INFO)TASKS_TSS_BASE;
	for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
		if (p[i].status == TASK_RUN && p[i].tid == tid) {
			return &p[i];
		}
	}
	return FALSE;
}


int __terminateByFileName(char * filename) {

	LPPROCESS_INFO p = __findProcessFileName(filename);
	if (p)
	{
		p->status = TASK_OVER;
	}

	return FALSE;
}

int __terminateByFuncName(char * funcname) {

	PROCESS_INFO * p = __findProcessFuncName(funcname);
	if (p)
	{
		p->status = TASK_OVER;

	}

	return FALSE;
}

int __terminatePid(int pid) {

	PROCESS_INFO* p = __findProcessByPid(pid);
	if (p)
	{
		p->status = TASK_OVER;

	}
	return 0;
}


int __terminateTid(int tid) {

	PROCESS_INFO* p = __findProcessByTid(tid);
	if (p)
	{
		p->status = TASK_OVER;

	}

	return 0;
}



int __pauseTid(int tid) {

	PROCESS_INFO* p = __findProcessByTid(tid);
	if (p)
	{
		p->status = TASK_SUSPEND;
	}

	return 0;
}


int __resumeTid(int tid) {

	PROCESS_INFO* p = __findProcessByTid(tid);
	if (p)
	{
		p->status = TASK_RUN;
	}

	return 0;
}


int __pausePid(int pid) {

	PROCESS_INFO* p = __findProcessByPid(pid);
	if (p)
	{
		p->status = TASK_SUSPEND;
	}

	return 0;
}


int __resumePid(int pid) {

	PROCESS_INFO* p = __findProcessByPid(pid);
	if (p)
	{
		p->status = TASK_RUN;
	}

	return 0;
}


void debugReg(PROCESS_INFO *next, PROCESS_INFO * prev) {
	
	__asm {
		mov ebx, prev

		mov eax,dr0
		mov ds : [ebx + PROCESS_INFO.dr0],eax
		mov eax, dr1
		mov ds : [ebx + PROCESS_INFO.dr1], eax
		mov eax, dr2
		mov ds : [ebx + PROCESS_INFO.dr2], eax
		mov eax, dr3
		mov ds : [ebx + PROCESS_INFO.dr3], eax
		mov eax, dr6
		mov ds : [ebx + PROCESS_INFO.dr6], eax
		mov eax, dr7
		mov ds : [ebx + PROCESS_INFO.dr7], eax

		mov ebx, next

		mov eax, ds:[ebx + PROCESS_INFO.dr0]
		mov dr0,eax

		mov eax, ds : [ebx + PROCESS_INFO.dr1]
		mov dr1, eax

		mov eax, ds:[ebx + PROCESS_INFO.dr2]
		mov dr2, eax

		mov eax, ds:[ebx + PROCESS_INFO.dr3]
		mov dr3, eax

		mov eax, ds:[ebx + PROCESS_INFO.dr6]
		mov dr6, eax

		mov eax, ds:[ebx + PROCESS_INFO.dr7]
		mov dr7, eax
	}
}


#ifndef SINGLE_TASK_TSS
extern "C"  __declspec(dllexport) DWORD __kTaskSchedule(LIGHT_ENVIRONMENT* env) {

	char szout[1024];

	__int64 timeh1 = __krdtsc();

	__k8254TimerProc();

	__asm {
		//clts			//before all fpu instructions
	}

	//__printf(szout, "__kTaskSchedule entry\r\n");

	LPPROCESS_INFO tss = (LPPROCESS_INFO)TASKS_TSS_BASE;
	LPPROCESS_INFO process = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
	LPPROCESS_INFO prev = (LPPROCESS_INFO)(tss + process->tid);

	if (process->tid != prev->tid) {
		__printf(szout, "__kTaskSchedule process tid:%d, prev tid:%d not same\r\n", process->tid, prev->tid);
		return 0;
	}

	V86ProcessCheck(env, prev, process);

	if (prev->status == TASK_TERMINATE || process->status == TASK_TERMINATE) {
		prev->status = TASK_OVER;
		process->status = TASK_OVER;
		if (prev->tid == prev->pid) {
			//__kFreeProcess(prev->pid);
		}
		else {
			//__kFree(prev->espbase);
		}
	}
	else if (prev->status == TASK_OVER || process->status == TASK_OVER) {
		process->status = TASK_OVER;
		prev->status = TASK_OVER;
		__printf(szout, "__kTaskSchedule prev status TASK_OVER!\r\n");
	}
	else if (process->status == TASK_RUN || prev->status == TASK_RUN)
	{
		if (process->sleep) {
			process->sleep--;
			prev->sleep = process->sleep;
		}
		else if (prev->sleep) {
			prev->sleep--;
			process->sleep = prev->sleep;
		}
		else {
			process->counter++;
		}
	}
	else if (process->status == TASK_SUSPEND || prev->status == TASK_SUSPEND) {
		process->status = TASK_SUSPEND;
		prev->status = TASK_SUSPEND;
	}
	else {
		__printf(szout, "__kTaskSchedule process status:%d, prev status:%d error\r\n", process->status, prev->status);
		return 0;
	}
	

	LPPROCESS_INFO next = prev;
	do {
		next++;
		if (next - tss >= TASK_LIMIT_TOTAL) {
			next = tss;
		}

		if (next == prev) {
			return FALSE;
		}

		if (next->status == TASK_TERMINATE) {
			next->status = TASK_OVER;
			if (next->tid == next->pid) {
				//__kFreeProcess(next->pid);
			}
			else {
				//__kFree(next->espbase);
			}
			continue;
		}
		else if (next->status == TASK_RUN) {
			if (next->sleep) {
				next->sleep--;
			}
			else {
				break;
			}
			continue;
		}
		else if (next->status == TASK_OVER) {
			continue;
		}
		else if (next->status == TASK_SUSPEND) {
			continue;
		}
	} while (TRUE);
	
	//切换到新任务的cr3和ldt会被自动加载，但是iret也会加载cr3和ldt，因此不需要手动加载
	//DescriptTableReg ldtreg;
	// 	__asm {
	//		sldt ldtreg;
	// 	}
	//process->tss.ldt = ldtreg.addr;

	debugReg(next, prev);

	if (prev->copyMap == 0) {
		int off = OFFSETOF(TSS, intMap);
		__memcpy((char*)prev, (char*)process, off);
		off = OFFSETOF(TSS, iomapEnd);
		int lsize = sizeof(PROCESS_INFO) - off;
		__memcpy((char*)prev + off, (char*)process + off, lsize);
	}
	else {
		__memcpy((char*)prev, (char*)process, sizeof(PROCESS_INFO));
	}
	if (process->copyMap == 0) {
		int off = OFFSETOF(TSS, intMap);
		__memcpy((char*)process, (char*)next, off);
		off = OFFSETOF(TSS, iomapEnd);
		int lsize = sizeof(PROCESS_INFO) - off;
		__memcpy((char*)process + off, (char*)next + off, lsize);
	}
	else {
		__memcpy((char*)process, (char*)next, sizeof(PROCESS_INFO));
	}
	
	//tasktest();

 	char * fenvprev = (char*)FPU_STATUS_BUFFER + (prev->tid << 9);
	//If a memory operand is not aligned on a 16-byte boundary, regardless of segment
	//The assembler issues two instructions for the FSAVE instruction (an FWAIT instruction followed by an FNSAVE instruction), 
	//and the processor executes each of these instructions separately.
	//If an exception is generated for either of these instructions, the save EIP points to the instruction that caused the exception.
	__asm {	
		FNCLEX
		//fwait
		fninit
		mov eax, fenvprev
		FxSAVE[eax]
		//fsave [fenv]
	}
	
	{
		char * fenvnext = (char*)FPU_STATUS_BUFFER + (next->tid << 9);
		__asm {
			mov eax, fenvnext
			//frstor [fenv]
			fxrstor[eax]
			FNCLEX
			fninit
		}
	}
	if ((g_tagMsg++) % 0x100 == 0 && g_tagMsg <= 0x200) {
		__int64 timeh2 = __krdtsc() - timeh1;

		DWORD cpureq;
		DWORD maxreq;
		DWORD busreq;
		__cpuFreq(&cpureq, &maxreq, &busreq);
		__int64 cpurate = cpureq;

		__printf(szout,
			"current link:%x,prev link:%x,next link:%x,stack eflags:%x,current eflags:%x,prev eflags:%x,next eflags:%x,new task pid:%d, tid:%d, old task pid:%d, tid:%d, timestamp:%i64x, cpurate:%i64x\r\n",
			process->tss.link, prev->tss.link, next->tss.link, env->eflags, process->tss.eflags, prev->tss.eflags, next->tss.eflags,
			prev->pid, prev->tid, next->pid, next->tid, timeh2, cpurate);
	}
	return TRUE;
}
#else
extern "C"  __declspec(dllexport) DWORD __kTaskSchedule(LIGHT_ENVIRONMENT * env) {

	char szout[1024];
	__int64 timeh1 = __krdtsc();
	__asm {
		clts			//before all fpu instructions
	}

	__k8254TimerProc();

	LPPROCESS_INFO tss = (LPPROCESS_INFO)TASKS_TSS_BASE;
	LPPROCESS_INFO process = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
	LPPROCESS_INFO prev = (LPPROCESS_INFO)(tss + process->tid);

	if (process->tid != prev->tid) {
		__printf(szout, "__kTaskSchedule process tid:%d, prev tid:%d not same\r\n", process->tid, prev->tid);
		return 0;
	}

	V86ProcessCheck(env, prev, process);

	if (prev->status == TASK_TERMINATE || process->status == TASK_TERMINATE) {
		prev->status = TASK_OVER;
		process->status = TASK_OVER;
		if (prev->tid == prev->pid) {
			//__kFreeProcess(prev->pid);
		}
		else {
			//__kFree(prev->espbase);
		}
	}
	else if (prev->status == TASK_OVER || process->status == TASK_OVER) {
		process->status = TASK_OVER;
		prev->status = TASK_OVER;
		__printf(szout, "__kTaskSchedule prev status TASK_OVER!\r\n");
	}
	else if (process->status == TASK_RUN || prev->status == TASK_RUN)
	{
		if (process->sleep) {
			process->sleep--;
			prev->sleep = process->sleep;
		}
		else if (prev->sleep) {
			prev->sleep--;
			process->sleep = prev->sleep;
		}
		else {
			process->counter++;
		}
	}
	else if (process->status == TASK_SUSPEND || prev->status == TASK_SUSPEND) {
		process->status = TASK_SUSPEND;
		prev->status = TASK_SUSPEND;
	}
	else {
		__printf(szout, "__kTaskSchedule process status:%d, prev status:%d error\r\n", process->status, prev->status);
		return 0;
	}


	LPPROCESS_INFO next = prev;
	do {
		next++;
		if (next - tss >= TASK_LIMIT_TOTAL) {
			next = tss;
		}

		if (next == prev) {
			if (prev->status == TASK_OVER) {
				__printf(szout,"task switch error!\r\n");
			}
			return FALSE;
		}

		if (next->status == TASK_TERMINATE) {
			next->status = TASK_OVER;
			if (next->tid == next->pid) {
				//__kFreeProcess(next->pid);
			}
			else {
				//__kFree(next->espbase);
			}
			continue;
		}
		else if (next->status == TASK_RUN) {
			if (next->sleep) {
				next->sleep--;
			}
			else {
				break;
			}
			continue;
		}
		else if (next->status == TASK_OVER) {
			continue;
		}
		else if (next->status == TASK_SUSPEND) {
			continue;
		}
	} while (TRUE);

	process->tss.eax = env->eax;
	process->tss.ecx = env->ecx;
	process->tss.edx = env->edx;
	process->tss.ebx = env->ebx;
	process->tss.esp = env->esp;
	process->tss.ebp = env->ebp;
	process->tss.esi = env->esi;
	process->tss.edi = env->edi;
	process->tss.ss = env->ss;
	process->tss.gs = env->gs;
	process->tss.fs = env->fs;
	process->tss.ds = env->ds;
	process->tss.es = env->es;

	process->tss.eip = env->eip;
	process->tss.cs = env->cs;
	process->tss.eflags = env->eflags;

	DWORD dwcr3 = 0;
	__asm {
		mov eax,cr3
		mov dwcr3,eax
	}
	process->tss.cr3 = dwcr3;

	if (env->eflags & 0x20000) 
	{
		//process->tss.gs = KERNEL_MODE_DATA;
		//process->tss.fs = KERNEL_MODE_DATA;
		//process->tss.ds = KERNEL_MODE_DATA;
		//process->tss.es = KERNEL_MODE_DATA;
		//process->tss.ss = KERNEL_MODE_DATA;
	}

	//切换到新任务的cr3和ldt会被自动加载，但是iret也会加载cr3和ldt，因此不需要手动加载
	//DescriptTableReg ldtreg;
	// 	__asm {
	//		sldt ldtreg;
	// 	}
	//process->tss.ldt = ldtreg.addr;
	debugReg(next, prev);

	if (prev->copyMap == 0) {
		int off = OFFSETOF(TSS, intMap);
		__memcpy((char*)prev, (char*)process, off);
		off = OFFSETOF(TSS, iomapEnd);
		int lsize = sizeof(PROCESS_INFO) - off;
		__memcpy((char*)prev+off, (char*)process+ off, lsize);
	}
	else {
		__memcpy((char*)prev, (char*)process, sizeof(PROCESS_INFO));
	}
	if (process->copyMap == 0) {
		int off = OFFSETOF(TSS, intMap);
		__memcpy((char*)process, (char*)next, off);
		off = OFFSETOF(TSS, iomapEnd);
		int lsize = sizeof(PROCESS_INFO)  - off;
		__memcpy((char*)process + off, (char*)next + off, lsize);
	}
	else {
		__memcpy((char*)process, (char*)next, sizeof(PROCESS_INFO));
	}

	if (process->tss.eflags & 0x20000) {
	}
	else if (process->tss.cs & 3) {
	}
	else {
	}

	//tasktest();

	char* fenvprev = (char*)FPU_STATUS_BUFFER + (prev->tid << 9);
	//If a memory operand is not aligned on a 16-byte boundary, regardless of segment
	//The assembler issues two instructions for the FSAVE instruction (an FWAIT instruction followed by an FNSAVE instruction), 
	//and the processor executes each of these instructions separately.
	//If an exception is generated for either of these instructions, the save EIP points to the instruction that caused the exception.
	__asm {
		FNCLEX
		fninit
		//fwait
		mov eax, fenvprev
		FxSAVE[eax]
		//fsave [fenv]
	}

	{
		char* fenvnext = (char*)FPU_STATUS_BUFFER + (next->tid << 9);
		__asm {
			mov eax, fenvnext
			//frstor [fenv]
			fxrstor[eax]
			FNCLEX
			fninit
		}
	}

	env->eax = process->tss.eax;
	env->ecx = process->tss.ecx;
	env->edx = process->tss.edx;
	env->ebx = process->tss.ebx;
	env->esp = process->tss.esp;
	env->ebp = process->tss.ebp;
	env->esi = process->tss.esi;
	env->edi = process->tss.edi;
	env->gs = process->tss.gs;
	env->fs = process->tss.fs;
	env->ds = process->tss.ds;
	env->es = process->tss.es;
	env->ss = process->tss.ss;

	if ((g_tagMsg++) % 0x100 == 0 && g_tagMsg <= 0x200) {
		__int64 timeh2 = __krdtsc() - timeh1;

		DWORD cpureq;
		DWORD maxreq;
		DWORD busreq;
		__cpuFreq(&cpureq, &maxreq, &busreq);
		__int64 cpurate = cpureq;

		__printf(szout,
			"current link:%x,prev link:%x,next link:%x,stack eflags:%x,current eflags:%x,prev eflags:%x,next eflags:%x,new task pid:%d, tid:%d, old task pid:%d, tid:%d, timestamp:%i64x, cpurate:%i64x\r\n",
			process->tss.link, prev->tss.link, next->tss.link, env->eflags, process->tss.eflags, prev->tss.eflags, next->tss.eflags,
			prev->pid, prev->tid, next->pid, next->tid, timeh2, cpurate);
	}

	return TRUE;
}
#endif


void tasktest(LPPROCESS_INFO gTasksListPtr, LPPROCESS_INFO gPrevTasksPtr) {
	static int gTestFlag = 0;
	if (gTestFlag >= 0 && gTestFlag <= -1)
	{
		char szout[1024];
		__printf(szout,
			"saved  cr3:%x,pid:%x,name:%s,level:%u,esp0:%x,ss0:%x,eip:%x,cs:%x,esp3:%x,ss3:%x,eflags:%x,link:%x,\r\n"
			"loaded cr3:%x,pid:%x,name:%s,level:%u,esp0:%x,ss0:%x,eip:%x,cs:%x,esp3:%x,ss3:%x,eflags:%x,link:%x.\r\n\r\n",
			gPrevTasksPtr->tss.cr3, gPrevTasksPtr->pid, gPrevTasksPtr->filename, gPrevTasksPtr->level,
			gPrevTasksPtr->tss.esp0, gPrevTasksPtr->tss.ss0, gPrevTasksPtr->tss.eip, gPrevTasksPtr->tss.cs,
			gPrevTasksPtr->tss.esp, gPrevTasksPtr->tss.ss, gPrevTasksPtr->tss.eflags, gPrevTasksPtr->tss.link,
			gTasksListPtr->tss.cr3, gTasksListPtr->pid, gTasksListPtr->filename, gTasksListPtr->level,
			gTasksListPtr->tss.esp0, gTasksListPtr->tss.ss0, gTasksListPtr->tss.eip, gTasksListPtr->tss.cs,
			gTasksListPtr->tss.esp, gTasksListPtr->tss.ss, gTasksListPtr->tss.eflags, gTasksListPtr->tss.link);
		gTestFlag++;
	}
}

#define IVT_PROCESS_SEGMENT		0X500

void SetIVTVector() {
	DWORD addr = IVT_PROCESS_SEGMENT;				//from 0x500 to 0x7c00 is available memory address
	DWORD vector = (addr << 4) + 0;
	*(DWORD*)addr = 0xcf;			//iret opcode
	DWORD* ivt = (DWORD*)0;		//dos int call
	for (int seq = 0; seq < 0x100; seq++) {
		if (seq >= 0x10 && seq < 0x20) {
			continue;
		}
		else if (seq == 0x20 || seq == 0x21) {
			continue;
		}
		else {
			ivt[seq] = vector;
		}
	}
}

void initTaskSwitchTss() {

	DescriptTableReg idtbase;
	__asm {
		sidt idtbase
	}

	IntTrapGateDescriptor* descriptor = (IntTrapGateDescriptor*)idtbase.addr;

	initKernelTss((TSS*)CURRENT_TASK_TSS_BASE, TASKS_STACK0_BASE + TASK_STACK0_SIZE - STACK_TOP_DUMMY,
		KERNEL_TASK_STACK_TOP, 0, PDE_ENTRY_VALUE, 0);
	makeTssDescriptor(CURRENT_TASK_TSS_BASE, 3, sizeof(TSS) - 1, (TssDescriptor*)(GDT_BASE + kTssTaskSelector));
#ifdef SINGLE_TASK_TSS
	makeIntGateDescriptor((DWORD)TimerInterrupt, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_MASTER + 0);
#else
	initKernelTss((TSS*)TIMER_TSS_BASE, TSSTIMER_STACK0_TOP, TSSTIMER_STACK_TOP, (DWORD)TimerInterrupt, PDE_ENTRY_VALUE, 0);
	makeTssDescriptor((DWORD)TIMER_TSS_BASE, 3, sizeof(TSS) - 1, (TssDescriptor*)(GDT_BASE + kTssTimerSelector));
	makeTaskGateDescriptor((DWORD)kTssTimerSelector, 3, (TaskGateDescriptor*)(descriptor + INTR_8259_MASTER + 0));
#endif

	__asm
	{
		mov eax, kTssTaskSelector
		ltr ax
		mov ax, ldtSelector
		lldt ax
	}
}

int __initTask0(char * videobase) {

	LPPROCESS_INFO tssbase = (LPPROCESS_INFO)TASKS_TSS_BASE;
	for (int i = 0; i < TASK_LIMIT_TOTAL; i++)
	{
		//__memset((char*)&tssbase[i], 0, sizeof(PROCESS_INFO));
		tssbase[i].status = TASK_OVER;
	}

	//initTaskSwitchTss();
	LPPROCESS_INFO process0 = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
	__strcpy(process0->filename, KERNEL_DLL_MODULE_NAME);
	__strcpy(process0->funcname, "__kKernel");
	process0->status = TASK_RUN;
	process0->tid = 0;
	process0->pid = 0;
	process0->espbase = KERNEL_TASK_STACK_TOP;
	process0->level = 0;
	process0->counter = 0;
	process0->vaddr = 0;
	process0->vasize = MEMMORY_ALLOC_BASE;
	process0->moduleaddr = (DWORD)KERNEL_DLL_BASE;
	process0->videoBase = (char*)videobase;
	process0->showX = 0;
	process0->showY = 0;
	process0->window = 0;
	__memcpy((char*)TASKS_TSS_BASE, (char*)CURRENT_TASK_TSS_BASE, sizeof(PROCESS_INFO));

	/*
	__memset((char*)TASKS_LIST_BASE, 0, TASK_LIMIT_TOTAL * sizeof(TASK_LIST_ENTRY));
	gTasksListPtr = (TASK_LIST_ENTRY*)TASKS_LIST_BASE;
	initListEntry(&gTasksListPtr->list);
	gTasksListPtr->process = (LPPROCESS_INFO)TASKS_TSS_BASE;
	gTasksListPtr->valid = TRUE;
	*/

	//__memset((char*)V86_TASKCONTROL_ADDRESS, 0, LIMIT_V86_PROC_COUNT*12);

	return 0;
}


//在V86模式下，CPL=3，执行特权指令时，或者要引起出错码为0的通用保护故障，或者要引起 非法操作码故障。
//由于CPL = 3， 所以如果IOPL < 3，那么执行CLI或STI指令将引起通用保护故障。
//输入 / 输出指令IN、INS、OUT或OUTS的 敏感条件仅仅是当前V86任务TSS内的I / O许可位图，而忽略EFLAGS中的IOPL。
//在V86模式下， 当IOPL < 3时，执行指令PUSHF、POPF、INT n及IRET会引起出错码为0的通用保护故障。
//采取上述措施的目的是使操作系统软件可以支持一个“虚拟EFLAGS”寄存器。