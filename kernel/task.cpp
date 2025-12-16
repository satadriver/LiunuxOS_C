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
#include "apic.h"



unsigned long g_task_array_lock = 0;

unsigned long g_task_list_lock = 0;

void enter_task_array_lock() {
	__enterSpinlock(&g_task_array_lock);
}

void leave_task_array_lock() {
	__leaveSpinlock(&g_task_array_lock);
}

void enter_task_array_lock_cli() {
	__asm {cli}
	__enterSpinlock(&g_task_array_lock);
}

void leave_task_array_lock_sti() {
	__asm {sti}
	__leaveSpinlock(&g_task_array_lock);
}

void enter_task_list_lock() {
	__enterSpinlock(&g_task_list_lock);
}

void leave_task_list_lock() {
	__leaveSpinlock(&g_task_list_lock);
}

void enter_task_list_lock_cli() {
	__asm {cli}
	__enterSpinlock(&g_task_list_lock);
}

void leave_task_list_lock_sti() {
	__asm {sti}
	__leaveSpinlock(&g_task_list_lock);
}

TASK_LIST_ENTRY *gTasksListPtr = 0;
TASK_LIST_ENTRY* gTasksListPos = 0;


TASK_LIST_ENTRY* GetTaskListHeader() {
	return gTasksListPtr;
}


void InitTaskList() {
	__memset((char*)TASKS_LIST_BASE, 0, TASK_LIMIT_TOTAL * sizeof(TASK_LIST_ENTRY));

	gTasksListPtr = (TASK_LIST_ENTRY*)TASKS_LIST_BASE;
	//gTasksListPos = gTasksListPtr;
	//gTasksListPtr->process = (LPPROCESS_INFO)TASKS_TSS_BASE;
	//gTasksListPtr->valid = 1;
	InitListEntry(& gTasksListPtr->list);

	InsertTaskList_No_cli(0);

	gTasksListPos = (TASK_LIST_ENTRY*)gTasksListPtr->list.next;
}

void __terminateTask(int tid, char * filename, char * funcname, DWORD lpparams) {

	RemoveTaskListTid(tid);
	__sleep(-1);
}

TASK_LIST_ENTRY* SearchTaskListTid(int tid) {
	

	TASK_LIST_ENTRY* result = 0;
	TASK_LIST_ENTRY * list = (TASK_LIST_ENTRY*)TASKS_LIST_BASE;
	for (int i = 1; i < TASK_LIMIT_TOTAL; i++)
	{
		if (list[i].valid  && list[i].process->status == TASK_RUN && list[i].process->tid == tid) {
			result = &list[i];
			break;
		}
	}

	
	return result;
}


TASK_LIST_ENTRY* SearchTaskListPid(int pid) {


	TASK_LIST_ENTRY* result = 0;

	TASK_LIST_ENTRY* head = (TASK_LIST_ENTRY*)gTasksListPtr;
	if (head) {
		TASK_LIST_ENTRY* first = (TASK_LIST_ENTRY*)&(head->list.next);
		TASK_LIST_ENTRY* node = first;
		do
		{
			if (node && node->valid && node->process && node->process->pid == pid)
			{
				result = node;
				break;
			}
			node = (TASK_LIST_ENTRY*)node->list.next;

		} while (node && node->valid && node != (TASK_LIST_ENTRY*)first);
	}


	return result;
}

TASK_LIST_ENTRY* GetFreeTaskList() {

	TASK_LIST_ENTRY* result = 0;
	TASK_LIST_ENTRY *info = (TASK_LIST_ENTRY*)TASKS_LIST_BASE;

	int cnt = TASKS_LIST_BUF_SIZE / sizeof(TASK_LIST_ENTRY);
	for (int i = 1; i < cnt; i++)
	{
		if (info[i].valid == 0 && info[i].process == 0 )
		{
			result = &info[i];
			break;
		}
	}

	return result;
}



TASK_LIST_ENTRY* InsertTaskList_No_cli(int tid) {

	//__enterSpinlock(&g_task_list_lock);
	TASK_LIST_ENTRY* result = 0;
	LPPROCESS_INFO base = (LPPROCESS_INFO)TASKS_TSS_BASE;

	TASK_LIST_ENTRY* list = (TASK_LIST_ENTRY*)TASKS_LIST_BASE;
	for (int i = 1; i < TASK_LIMIT_TOTAL; i++)
	{
		if (list[i].valid == 0) {
			list[i].valid = TRUE;

			list[i].process = base + tid;
			list[i].process->status = TASK_RUN;

			InsertListTail((LIST_ENTRY*)&(gTasksListPtr->list), (LIST_ENTRY*)&(list[i].list));
			//gTasksListPos = &list[i];
			result = &list[i];
			break;
		}
	}
	//__leaveSpinlock(&g_task_list_lock);

	return result;
}


TASK_LIST_ENTRY* InsertTaskList(int tid) {

	enter_task_list_lock_cli();

	TASK_LIST_ENTRY* result = 0;
	LPPROCESS_INFO base = (LPPROCESS_INFO)TASKS_TSS_BASE;

	TASK_LIST_ENTRY * list = (TASK_LIST_ENTRY*)TASKS_LIST_BASE;
	for (int i = 1; i < TASK_LIMIT_TOTAL; i++)
	{
		if (list[i].valid == 0 ) {
			list[i].valid = TRUE;

			list[i].process = base + tid;
			list[i].process->status = TASK_RUN;
			
			InsertListTail((LIST_ENTRY*)&(gTasksListPtr->list), (LIST_ENTRY*)&(list[i].list) );
			//gTasksListPos = &list[i];
			result = &list[i];
			break;
		}
	}
	leave_task_list_lock_sti();

	return result;
}

TASK_LIST_ENTRY* RemoveTaskListTid(int tid) {

	enter_task_list_lock_cli();
	TASK_LIST_ENTRY* result = 0;
	int cnt = 0;
	TASK_LIST_ENTRY * head = (TASK_LIST_ENTRY*)gTasksListPtr;
	if (head) {
		TASK_LIST_ENTRY* first = (TASK_LIST_ENTRY*)&(head->list.next);
		TASK_LIST_ENTRY* node = first;
		do
		{
			if (node && node->valid && node->process && node->process->tid == tid)
			{
				RemoveList(&(gTasksListPtr->list), (LIST_ENTRY*)&(node->list) );

				node->process->status = TASK_OVER;
				node->process = 0;

				node->valid = FALSE;

				result = node;
	
				if (node == gTasksListPos) {
					gTasksListPos = (TASK_LIST_ENTRY*)node->list.next;
					if (gTasksListPos == 0) {
						//error
						char szout[256];
						__printf(szout, "%s error!\r\n", __FUNCTION__);
						
					}
				}

				//__printf(szout, "%s tid:%x,pid:%x,current pid:%x,current tid:%x,filename:%s,funcname:%s\n",__FUNCTION__,tid, pid, current->pid, current->tid, filename, funcname);
				
				break;
			}
			node = (TASK_LIST_ENTRY*)node->list.next;

			cnt++;
			if (cnt >= TASK_LIMIT_TOTAL) {
				char szout[256];
				__printf(szout, "%s max count:%d\r\n", __FUNCTION__, cnt);
				break;
			}

		} while (node && node->valid && node != (TASK_LIST_ENTRY*)first);
	}

	LPPROCESS_INFO tss = (LPPROCESS_INFO)TASKS_TSS_BASE;
	if (tss[tid].level >= 3 && (tss[tid].tss.eflags& 0x20000) && tss[tid].espbase) {
		__kFree(tss[tid].espbase);
	}

	LPPROCESS_INFO current = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	current->status = TASK_OVER;

	leave_task_list_lock_sti();

	return result;
}

TASK_LIST_ENTRY* RemoveTaskListPid(int pid) {

	enter_task_list_lock_cli();
	TASK_LIST_ENTRY* result = 0;

	LPPROCESS_INFO tss = (LPPROCESS_INFO)TASKS_TSS_BASE;

	LPPROCESS_INFO current = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	int cnt = 0;
	TASK_LIST_ENTRY* head = (TASK_LIST_ENTRY*)gTasksListPtr;
	if (head) {
		TASK_LIST_ENTRY* first = (TASK_LIST_ENTRY*)&(head->list.next);
		TASK_LIST_ENTRY* node = first;
		do
		{
			if (node && node->valid && node->process && node->process->pid == pid)
			{
				RemoveList(&(gTasksListPtr->list), (LIST_ENTRY*)&(node->list));

				node->process->status = TASK_OVER;
				node->process = 0;

				node->valid = FALSE;

				result = node;

				if (node == gTasksListPos) {
					gTasksListPos = (TASK_LIST_ENTRY*)node->list.next;
					if (gTasksListPos == 0) {
						//error
						char szout[256];
						__printf(szout, "%s error!\r\n", __FUNCTION__);

					}
				}

				if (first == node) {
					first = (TASK_LIST_ENTRY*)node->list.next;
					node = first;
				}
			}
			node = (TASK_LIST_ENTRY*)node->list.next;

			cnt++;
			if (cnt >= TASK_LIMIT_TOTAL) {
				char szout[256];
				__printf(szout, "%s max count:%d\r\n", __FUNCTION__, cnt);
				break;
			}

		} while (node && node->valid && node != (TASK_LIST_ENTRY*)first);
	}

	current->status = TASK_OVER;

	__kFreeProcess(pid);

	leave_task_list_lock_sti();

	return result;
}

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


int __getFreeTask(LPTASKRESULT ret,int clear) {
	int result = 0;
	if (ret == 0)
	{
		return FALSE;
	}
	ret->lptss = 0;
	ret->number = 0;
	if (clear) {
		enter_task_array_lock_cli();
	}
	else {
		enter_task_array_lock();
	}

	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetTaskTssBase();
	for (int i = 0;i < TASK_LIMIT_TOTAL; i++)
	{
		if (tss[i].status == TASK_OVER)
		{
			clearTssBuf(&tss[i]);

			tss[i].status = TASK_SUSPEND;

			ret->number = i;
			ret->lptss = &tss[i];
			result = 1;
			break;
		}
	}
	if (clear) {

		leave_task_array_lock_sti();
	}
	else {
		leave_task_array_lock();
	}

	return result;
}



LPPROCESS_INFO __findProcessFuncName(const char * funcname) {

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
	enter_task_array_lock_cli();;
	LPPROCESS_INFO p = __findProcessFileName(filename);
	if (p)
	{
		p->status = TASK_OVER;
	}
	leave_task_array_lock_sti();
	return FALSE;
}

int __terminateByFuncName(char * funcname) {
	enter_task_array_lock_cli();
	PROCESS_INFO * p = __findProcessFuncName(funcname);
	if (p)
	{
		p->status = TASK_OVER;

	}
	leave_task_array_lock_sti();
	return FALSE;
}

int __terminatePid(int pid) {
	enter_task_array_lock_cli();
	PROCESS_INFO* p = __findProcessByPid(pid);
	if (p)
	{
		p->status = TASK_OVER;
	}
	leave_task_array_lock_sti();
	return 0;
}


int __terminateTid(int tid) {
	enter_task_array_lock_cli();
	PROCESS_INFO* p = __findProcessByTid(tid);
	if (p)
	{
		p->status = TASK_OVER;
	}
	leave_task_array_lock_sti();
	return 0;
}



int __pauseTid(int tid) {
	enter_task_array_lock_cli();
	PROCESS_INFO* p = __findProcessByTid(tid);
	if (p)
	{
		p->status = TASK_SUSPEND;
	}
	leave_task_array_lock_sti();
	return 0;
}


int __resumeTid(int tid) {
	enter_task_array_lock_cli();
	PROCESS_INFO* p = __findProcessByTid(tid);
	if (p)
	{
		p->status = TASK_RUN;
	}
	leave_task_array_lock_sti();
	return 0;
}


int __pausePid(int pid) {
	enter_task_array_lock_cli();
	PROCESS_INFO* p = __findProcessByPid(pid);
	if (p)
	{
		p->status = TASK_SUSPEND;
	}
	leave_task_array_lock_sti();
	return 0;
}


int __resumePid(int pid) {
	enter_task_array_lock_cli();
	PROCESS_INFO* p = __findProcessByPid(pid);
	if (p)
	{
		p->status = TASK_RUN;
	}
	leave_task_array_lock_sti();
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


#ifdef TASK_SWITCH_ARRAY
LPPROCESS_INFO SingleTssSchedule(LIGHT_ENVIRONMENT* env) {
	char szout[1024];
	int ret = 0;
	ret = __enterSpinlock(&g_task_array_lock);

	LPPROCESS_INFO tss = (LPPROCESS_INFO)TASKS_TSS_BASE;
	LPPROCESS_INFO process = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	LPPROCESS_INFO current = (LPPROCESS_INFO)(tss + process->tid);
	LPPROCESS_INFO next = current;

	int cpuid = *(DWORD*)(LOCAL_APIC_BASE + 0x20)>>24;

	if (current->status == TASK_TERMINATE || process->status == TASK_TERMINATE) {
		current->status = TASK_OVER;
		process->status = TASK_OVER;
		if (current->tid == current->pid) {
			//__kFreeProcess(prev->pid);
		}
		else {
			//__kFree(prev->espbase);
		}
	}
	else if (current->status == TASK_OVER || process->status == TASK_OVER) {
		process->status = TASK_OVER;
		current->status = TASK_OVER;
		//__printf(szout, "%s prev tss status TASK_OVER error!\r\n",__FUNCTION__);
	}
	else if (process->status == TASK_RUN && current->status == TASK_RUN)
	{
		if (process->sleep) {
			process->sleep--;
			current->sleep = process->sleep;
		}
		else if (current->sleep) {
			current->sleep--;
			process->sleep = current->sleep;
		}
		else {
			process->counter++;
		}
	}
	else if (process->status == TASK_SUSPEND || current->status == TASK_SUSPEND) {
		//process->status = TASK_SUSPEND;
		//prev->status = TASK_SUSPEND;
	}
	else if (current->status != process->status) {
		__printf(szout, "%s prev tss status %d/%d error!\r\n", current->status,process->status);
	}
	else {
		__printf(szout, "__kTaskSchedule process status:%d, prev status:%d error\r\n", process->status, current->status);
		goto __SingleTssSchedule_end;
	}
	
	do {
		next++;
		if (next - tss >= TASK_LIMIT_TOTAL) {
			next = tss;
		}

		if (next == current) {
			goto __SingleTssSchedule_end;
		}

		if (cpuid != next->cpuid) {
			continue;
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
		mov eax, cr3
		mov dwcr3, eax
	}
	process->tss.cr3 = dwcr3;

	//切换到新任务的cr3和ldt会被自动加载，但是iret也会加载cr3和ldt，因此不需要手动加载
	//DescriptTableReg ldtreg;
	// 	__asm {
	//		sldt ldtreg;
	// 	}
	//process->tss.ldt = ldtreg.addr;

	debugReg(next, current);

	if (current->copyMap == 0) {
		int off = OFFSETOF(TSS, intMap);
		__memcpy((char*)current, (char*)process, off);
		off = OFFSETOF(TSS, iomapEnd);
		int lsize = sizeof(PROCESS_INFO) - off;
		__memcpy((char*)current + off, (char*)process + off, lsize);
	}
	else {
		__memcpy((char*)current, (char*)process, sizeof(PROCESS_INFO));
	}
	if (next->copyMap == 0) {
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

	char* fenvprev = (char*)FPU_STATUS_BUFFER + (current->tid << 9);
	//If a memory operand is not aligned on a 16-byte boundary, regardless of segment
	//The assembler issues two instructions for the FSAVE instruction 
	// (an FWAIT instruction followed by an FNSAVE instruction), 
	//and the processor executes each of these instructions separately.
	//If an exception is generated for either of these instructions,
	// the save EIP points to the instruction that caused the exception.
	__asm {
		FNCLEX
		fninit
		//fwait
		mov eax, fenvprev
		FxSAVE[eax]
		//fsave [fenv]
	}

	char* fenvnext = (char*)FPU_STATUS_BUFFER + (next->tid << 9);
	__asm {
		mov eax, fenvnext
		//frstor [fenv]
		fxrstor[eax]
		FNCLEX
		fninit
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
__SingleTssSchedule_end:
	__leaveSpinlock(&g_task_array_lock);
	return next;
}
#else
LPPROCESS_INFO SingleTssSchedule(LIGHT_ENVIRONMENT* env) {
	char szout[1024];
	
	int ret = 0;
	ret = __enterSpinlock(&g_task_list_lock);
	ret = __enterSpinlock(&g_task_array_lock);
	LPPROCESS_INFO process = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	LPPROCESS_INFO tss = (LPPROCESS_INFO)TASKS_TSS_BASE;
	LPPROCESS_INFO proc = (LPPROCESS_INFO)(tss + process->tid);
	TASK_LIST_ENTRY* next = (TASK_LIST_ENTRY*)gTasksListPos->list.next;

	int cpuid = *(DWORD*)(LOCAL_APIC_BASE + 0x20)>>24;

	if (proc->status == TASK_TERMINATE  ) {
		proc->status = TASK_OVER;	
	}

	if (process->status == TASK_TERMINATE) {
		process->status = TASK_OVER;
	}

	if (proc->status == TASK_OVER || process->status == TASK_OVER) {
		//__printf(szout, "%s prev tss status TASK_OVER error!\r\n",__FUNCTION__);
	}

	if (process->status == TASK_RUN )
	{
		if (process->sleep) {
			process->sleep--;
		}
		else {
			process->counter++;
		}
	}

	if ( proc->status == TASK_RUN)
	{
		if (proc->sleep) {
			proc->sleep--;
		}
		else {
			proc->counter++;
		}
	}

	if (process->status == TASK_SUSPEND) {
	}

	if (proc->status == TASK_SUSPEND) {
	}

	do {
		if (next->process == proc) {
			goto  __SingleTssSchedule_end;
		}
		if (next->process->cpuid == cpuid) {
			if (next->process->status == TASK_TERMINATE) {
				next->process->status = TASK_OVER;
			}

			if (next->process->status == TASK_RUN) {
				if (next->process->sleep) {
					next->process->sleep--;
				}
				else {
					break;
				}
			}
		}

		next = (TASK_LIST_ENTRY * )(next->list.next);
	} while (next && (next->process != proc) );

	gTasksListPos = next;

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
		mov eax, cr3
		mov dwcr3, eax
	}
	process->tss.cr3 = dwcr3;

	//切换到新任务的cr3和ldt会被自动加载，但是iret也会加载cr3和ldt，因此不需要手动加载
	//DescriptTableReg ldtreg;
	// 	__asm {
	//		sldt ldtreg;
	// 	}
	//process->tss.ldt = ldtreg.addr;

	debugReg(next->process, proc);

	if (proc->copyMap == 0) {
		int off = OFFSETOF(TSS, intMap);
		__memcpy((char*)proc, (char*)process, off);
		off = OFFSETOF(TSS, iomapEnd);
		int lsize = sizeof(PROCESS_INFO) - off;
		__memcpy((char*)proc + off, (char*)process + off, lsize);
	}
	else {
		__memcpy((char*)proc, (char*)process, sizeof(PROCESS_INFO));
	}
	if (process->copyMap == 0) {
		int off = OFFSETOF(TSS, intMap);
		__memcpy((char*)process, (char*)next->process, off);
		off = OFFSETOF(TSS, iomapEnd);
		int lsize = sizeof(PROCESS_INFO) - off;
		__memcpy((char*)process + off, (char*)next->process + off, lsize);
	}
	else {
		__memcpy((char*)process, (char*)next->process, sizeof(PROCESS_INFO));
	}

	//tasktest();

	char* fenvprev = (char*)FPU_STATUS_BUFFER + (process->tid << 9);
	//If a memory operand is not aligned on a 16-byte boundary, regardless of segment
	//The assembler issues two instructions for the FSAVE instruction 
	// (an FWAIT instruction followed by an FNSAVE instruction), 
	//and the processor executes each of these instructions separately.
	//If an exception is generated for either of these instructions,
	// the save EIP points to the instruction that caused the exception.
	__asm {
		FNCLEX
		fninit
		//fwait
		mov eax, fenvprev
		FxSAVE[eax]
		//fsave [fenv]
	}

	char* fenvnext = (char*)FPU_STATUS_BUFFER + (next->process->tid << 9);
	__asm {
		mov eax, fenvnext
		//frstor [fenv]
		fxrstor[eax]
		FNCLEX
		fninit
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

__SingleTssSchedule_end:
	ret = __leaveSpinlock(&g_task_array_lock);
	__leaveSpinlock(&g_task_list_lock);

	return next->process;
}
#endif



#ifdef TASK_SWITCH_ARRAY
LPPROCESS_INFO MultipleTssSchedule(LIGHT_ENVIRONMENT* env) {
	char szout[1024];
	int ret = 0;
	ret = __enterSpinlock(&g_task_array_lock);
	LPPROCESS_INFO tss = (LPPROCESS_INFO)TASKS_TSS_BASE;
	LPPROCESS_INFO process = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	LPPROCESS_INFO current = (LPPROCESS_INFO)(tss + process->tid);

	LPPROCESS_INFO next = current;

	int cpuid = *(DWORD*)(LOCAL_APIC_BASE + 0x20);
	cpuid = cpuid >> 24;

	if (current->status == TASK_TERMINATE || process->status == TASK_TERMINATE) {
		current->status = TASK_OVER;
		process->status = TASK_OVER;
		if (current->tid == current->pid) {
			//__kFreeProcess(prev->pid);
		}
		else {
			//__kFree(prev->espbase);
		}
	}
	else if (current->status == TASK_OVER || process->status == TASK_OVER) {
		//process->status = TASK_OVER;
		//prev->status = TASK_OVER;
		//__printf(szout, "%s prev tss status TASK_OVER error!\r\n",__FUNCTION__);
	}
	else if (process->status == TASK_RUN || current->status == TASK_RUN)
	{
		if (process->sleep) {
			process->sleep--;
			current->sleep = process->sleep;
		}
		else if (current->sleep) {
			current->sleep--;
			process->sleep = current->sleep;
		}
		else {
			process->counter++;
		}
	}
	else if (process->status == TASK_SUSPEND || current->status == TASK_SUSPEND) {
		//process->status = TASK_SUSPEND;
		//prev->status = TASK_SUSPEND;
	}
	else {
		__printf(szout, "__kTaskSchedule process status:%d, prev status:%d error\r\n", process->status, current->status);
		goto __MultipleTssSchedule_end;
	}
	
	do {
		next++;
		if (next - tss >= TASK_LIMIT_TOTAL) {
			next = tss;
		}

		if (next == current) {
			goto __MultipleTssSchedule_end;
		}

		if (cpuid != next->cpuid) {
			continue;
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

	debugReg(next, current);

	if (current->copyMap == 0) {
		int off = OFFSETOF(TSS, intMap);
		__memcpy((char*)current, (char*)process, off);
		off = OFFSETOF(TSS, iomapEnd);
		int lsize = sizeof(PROCESS_INFO) - off;
		__memcpy((char*)current + off, (char*)process + off, lsize);
	}
	else {
		__memcpy((char*)current, (char*)process, sizeof(PROCESS_INFO));
	}
	if (next->copyMap == 0) {
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

	char* fenvprev = (char*)FPU_STATUS_BUFFER + (current->tid << 9);
	//If a memory operand is not aligned on a 16-byte boundary, regardless of segment
	//The assembler issues two instructions for the FSAVE instruction 
	// (an FWAIT instruction followed by an FNSAVE instruction), 
	//and the processor executes each of these instructions separately.
	//If an exception is generated for either of these instructions,
	// the save EIP points to the instruction that caused the exception.
	__asm {
		FNCLEX
		//fwait
		fninit
		mov eax, fenvprev
		FxSAVE[eax]
		//fsave [fenv]
	}

	char* fenvnext = (char*)FPU_STATUS_BUFFER + (next->tid << 9);
	__asm {
		mov eax, fenvnext
		//frstor [fenv]
		fxrstor[eax]
		FNCLEX
		fninit
	}

	__MultipleTssSchedule_end:
	__leaveSpinlock(&g_task_array_lock);
	return next;
}
#else
LPPROCESS_INFO MultipleTssSchedule(LIGHT_ENVIRONMENT* env) {

	char szout[1024];
	int ret = 0;
	ret = __enterSpinlock(&g_task_list_lock);

	ret = __enterSpinlock(&g_task_array_lock);

	LPPROCESS_INFO process = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	LPPROCESS_INFO tss = (LPPROCESS_INFO)TASKS_TSS_BASE;
	LPPROCESS_INFO proc = (LPPROCESS_INFO)(tss + process->tid);

	TASK_LIST_ENTRY* next = (TASK_LIST_ENTRY*)gTasksListPos->list.next;

	int cpuid = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

	if (proc->status == TASK_TERMINATE) {
		proc->status = TASK_OVER;
	}

	if (process->status == TASK_TERMINATE) {
		process->status = TASK_OVER;
	}

	if (proc->status == TASK_OVER || process->status == TASK_OVER) {
		//__printf(szout, "%s prev tss status TASK_OVER error!\r\n",__FUNCTION__);
	}

	if (process->status == TASK_RUN)
	{
		if (process->sleep) {
			process->sleep--;
		}
		else {
			process->counter++;
		}
	}

	if (proc->status == TASK_RUN)
	{
		if (proc->sleep) {
			proc->sleep--;
		}
		else {
			proc->counter++;
		}
	}

	if (process->status == TASK_SUSPEND) {
	}

	if (proc->status == TASK_SUSPEND) {
	}

	do {
		if (next->process == proc) {
			goto  __MultipleTssSchedule_end;
		}

		if (next->process->cpuid == cpuid) {
			if (next->process->status == TASK_TERMINATE) {
				next->process->status = TASK_OVER;
			}

			if (next->process->status == TASK_RUN) {
				if (next->process->sleep) {
					next->process->sleep--;
				}
				else {
					break;
				}
			}
		}

		next = (TASK_LIST_ENTRY*)(next->list.next);
	} while (next && (next->process != proc));

	gTasksListPos = next;

	//切换到新任务的cr3和ldt会被自动加载，但是iret也会加载cr3和ldt，因此不需要手动加载
	//DescriptTableReg ldtreg;
	// 	__asm {
	//		sldt ldtreg;
	// 	}
	//process->tss.ldt = ldtreg.addr;

	debugReg(next->process, proc);

	if (proc->copyMap == 0) {
		int off = OFFSETOF(TSS, intMap);
		__memcpy((char*)proc, (char*)process, off);
		off = OFFSETOF(TSS, iomapEnd);
		int lsize = sizeof(PROCESS_INFO) - off;
		__memcpy((char*)proc + off, (char*)process + off, lsize);
	}
	else {
		__memcpy((char*)proc, (char*)process, sizeof(PROCESS_INFO));
	}
	if (process->copyMap == 0) {
		int off = OFFSETOF(TSS, intMap);
		__memcpy((char*)process, (char*)next->process, off);
		off = OFFSETOF(TSS, iomapEnd);
		int lsize = sizeof(PROCESS_INFO) - off;
		__memcpy((char*)process + off, (char*)next->process + off, lsize);
	}
	else {
		__memcpy((char*)process, (char*)next->process, sizeof(PROCESS_INFO));
	}

	//tasktest();

	char* fenvprev = (char*)FPU_STATUS_BUFFER + (proc->tid << 9);
	//If a memory operand is not aligned on a 16-byte boundary, regardless of segment
	//The assembler issues two instructions for the FSAVE instruction 
	// (an FWAIT instruction followed by an FNSAVE instruction), 
	//and the processor executes each of these instructions separately.
	//If an exception is generated for either of these instructions,
	// the save EIP points to the instruction that caused the exception.
	__asm {
		FNCLEX
		//fwait
		fninit
		mov eax, fenvprev
		FxSAVE[eax]
		//fsave [fenv]
	}

	char* fenvnext = (char*)FPU_STATUS_BUFFER + (next->process->tid << 9);
	__asm {
		mov eax, fenvnext
		//frstor [fenv]
		fxrstor[eax]
		FNCLEX
		fninit
	}
__MultipleTssSchedule_end:
	ret = __leaveSpinlock(&g_task_array_lock);
	__leaveSpinlock(&g_task_list_lock);
	return next->process;
}

#endif




extern "C"  __declspec(dllexport) DWORD __kTaskSchedule(LIGHT_ENVIRONMENT* env) {

	char szout[1024];

	__int64 timeh1 = __krdtsc();

	__k8254TimerProc();

	ActiveApTask(TASK_SWITCH_VECTOR);
	
	__asm {
		//clts			//before all fpu instructions
	}

	//__printf(szout, "__kTaskSchedule entry\r\n");

	LPPROCESS_INFO tss = (LPPROCESS_INFO)TASKS_TSS_BASE;
	LPPROCESS_INFO process = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	LPPROCESS_INFO current = (LPPROCESS_INFO)(tss + process->tid);

	if (process->tid != current->tid) {
		__printf(szout, "__kTaskSchedule process tid:%d, prev tid:%d not same\r\n", process->tid, current->tid);
		return 0;
	}

	V86ProcessCheck(env, current, process);

#ifdef SINGLE_TASK_TSS
	LPPROCESS_INFO next = SingleTssSchedule(env);
#else
	LPPROCESS_INFO next = MultipleTssSchedule(env);
#endif
	
	if (next && (g_tagMsg++) % 0x100 == 0 && g_tagMsg == 0x100) {
		__int64 timeh2 = __krdtsc() - timeh1;

		DWORD cpureq;
		DWORD maxreq;
		DWORD busreq;
		__cpuFreq(&cpureq, &maxreq, &busreq);
		__int64 cpurate = cpureq;

		__printf(szout,
			"current link:%x,prev link:%x,next link:%x,stack eflags:%x,current eflags:%x,prev eflags:%x,next eflags:%x,new task pid:%d, tid:%d, old task pid:%d, tid:%d, timestamp:%i64x, cpurate:%i64x\r\n",
			process->tss.link, current->tss.link, next->tss.link, env->eflags, process->tss.eflags, current->tss.eflags, next->tss.eflags,
			current->pid, current->tid, next->pid, next->tid, timeh2, cpurate);
	}
	return TRUE;
}








void SetIVTVector() {
	DWORD addr = IVT_PROCESS_ADDRESS;				//from 0x500 to 0x7c00 is available memory address
	
	*(DWORD*)addr = 0xcf;			//iret opcode
	DWORD vector = (IVT_PROCESS_SEG << 16) + IVT_PROCESS_OFFSET;
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







int __initTask0(char * videobase) {

	LPPROCESS_INFO tssbase = (LPPROCESS_INFO)TASKS_TSS_BASE;
	for (int i = 0; i < TASK_LIMIT_TOTAL; i++)
	{
		//__memset((char*)&tssbase[i], 0, sizeof(PROCESS_INFO));
		tssbase[i].status = TASK_OVER;
	}

	//initTaskSwitchTss();
	LPPROCESS_INFO process0 = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	__strcpy(process0->filename, (char*)LIUNUX_KERNEL32_DLL);
	__strcpy(process0->funcname, (char*)"__kKernel");
	process0->status = TASK_RUN;
	process0->tid = KERNEL_PROCESS_ID;
	process0->pid = KERNEL_PROCESS_ID;
	process0->cpuid = 0;
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
	__memcpy((char*)TASKS_TSS_BASE, (char*)GetCurrentTaskTssBase(), sizeof(PROCESS_INFO));

#ifdef TASK_SWITCH_ARRAY

#else
	InitTaskList();

	//InsertTaskList(KERNEL_PROCESS_ID);
#endif

	//__memset((char*)V86_TASKCONTROL_ADDRESS, 0, LIMIT_V86_PROC_COUNT*12);

	return 0;
}


//在V86模式下，CPL=3，执行特权指令时，或者要引起出错码为0的通用保护故障，或者要引起 非法操作码故障。
//由于CPL = 3， 所以如果IOPL < 3，那么执行CLI或STI指令将引起通用保护故障。
//输入 / 输出指令IN、INS、OUT或OUTS的 敏感条件仅仅是当前V86任务TSS内的I / O许可位图，而忽略EFLAGS中的IOPL。
//在V86模式下， 当IOPL < 3时，执行指令PUSHF、POPF、INT n及IRET会引起出错码为0的通用保护故障。
//采取上述措施的目的是使操作系统软件可以支持一个“虚拟EFLAGS”寄存器。









extern "C" void __declspec(naked)  BspTaskSchedule(LIGHT_ENVIRONMENT* stack) {
	__asm {
		pushad
		push ds
		push es
		push fs
		push gs
		push ss

		push esp
		sub esp, 4
		push ebp
		mov ebp, esp

		mov eax, KERNEL_MODE_DATA
		mov ds, ax
		mov es, ax
		MOV FS, ax
		MOV GS, AX
		mov ss, ax

		//clts
		cli
	}

	{
		//LPPROCESS_INFO process = (LPPROCESS_INFO)GetCurrentTaskTssBase();
		char szout[1024];
		//__printf(szout,"TimerInterrupt\r\n");

#ifdef SINGLE_TASK_TSS
		LPPROCESS_INFO next = SingleTssSchedule(stack);
#else
		LPPROCESS_INFO next = MultipleTssSchedule(stack);
#endif

		* (DWORD*)(LOCAL_APIC_BASE + 0xb0) = 0;

	}

	__asm {
#ifdef SINGLE_TASK_TSS
		call GetCurrentTaskTssBase
		mov edx, eax
		mov eax, dword ptr ds : [edx + PROCESS_INFO.tss.cr3]
		mov cr3, eax
#endif

		mov esp, ebp
		pop ebp
		add esp, 4
		pop esp
		pop ss
		pop gs
		pop fs
		pop es
		pop ds
		popad

#ifdef SINGLE_TASK_TSS
		mov esp, ss: [esp - 20]
#endif	
		//clts
		//sti

		iretd

		jmp BspTaskSchedule
	}
}


extern "C" void __declspec(naked) ApTaskSchedule(LIGHT_ENVIRONMENT* stack) {

	__asm {
		pushad
		push ds
		push es
		push fs
		push gs
		push ss

		push esp
		sub esp, 4
		push ebp
		mov ebp, esp

		mov eax, KERNEL_MODE_DATA
		mov ds, ax
		mov es, ax
		MOV FS, ax
		MOV GS, AX
		mov ss, ax

		//clts
		//cli
	}

	{
		//LPPROCESS_INFO process = (LPPROCESS_INFO)GetCurrentTaskTssBase();
		char szout[1024];
		//__printf(szout,"TimerInterrupt\r\n");

		LPPROCESS_INFO next = SingleTssSchedule(stack);

		*(DWORD*)(LOCAL_APIC_BASE + 0xb0) = 0;
	}

	__asm {
		call GetCurrentTaskTssBase
		mov edx, eax
		mov eax, dword ptr ds : [edx + PROCESS_INFO.tss.cr3]
		mov cr3, eax

		mov esp, ebp
		pop ebp
		add esp, 4
		pop esp
		pop ss
		pop gs
		pop fs
		pop es
		pop ds
		popad

		mov esp, ss: [esp - 20]

		clts
		//sti

		iretd

		jmp ApTaskSchedule
	}
}

extern "C" void __declspec(dllexport) GiveupLive( LIGHT_ENVIRONMENT * stack) {
	
	__asm{cli}
	int ret = IsBspProcessor();
	if (ret) {
#ifdef SINGLE_TASK_TSS
		LPPROCESS_INFO next = SingleTssSchedule(stack);
#else
		LPPROCESS_INFO next = MultipleTssSchedule(stack);

		__asm {
			mov eax, stack
			mov esp, eax
			mov ebp, eax

			//mov esp, ebp
			//pop ebp
			//add esp, 4
			//pop esp

			pop ss
			pop gs
			pop fs
			pop es
			pop ds
			popad

			//clts
			//sti

			iretd
		}
#endif
	}
	else {
		LPPROCESS_INFO next = SingleTssSchedule(stack);
	}
	
	__asm {
		mov eax, stack
		mov esp,eax
		mov ebp,eax

		call GetCurrentTaskTssBase
		mov edx, eax
		mov eax, dword ptr ds : [edx + PROCESS_INFO.tss.cr3]
		mov cr3, eax

		//mov esp, ebp
		//pop ebp
		//add esp, 4
		//pop esp

		pop ss
		pop gs
		pop fs
		pop es
		pop ds
		popad

		mov esp, ss: [esp - 20]

		//clts
		//sti

		iretd
	}

}

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



void initTaskSwitchTss() {

	DescriptTableReg idtbase;
	__asm {
		sidt idtbase
	}

	IntTrapGateDescriptor* descriptor = (IntTrapGateDescriptor*)idtbase.addr;

	initKernelTss((TSS*)GetCurrentTaskTssBase(), TASKS_STACK0_BASE + TASK_STACK0_SIZE - STACK_TOP_DUMMY,
		KERNEL_TASK_STACK_TOP, 0, PDE_ENTRY_VALUE, 0);
	makeTssDescriptor((DWORD)GetCurrentTaskTssBase(), 3, sizeof(TSS) - 1, (TssDescriptor*)(GDT_BASE + kTssTaskSelector));
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