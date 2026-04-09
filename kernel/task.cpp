#include "process.h"
#include "task.h"
#include "Utils.h"
#include "Kernel.h"
#include "video.h"
#include "Pe.h"
#include "processDOS.h"
#include "file.h"
#include "apictimer.h"
#include "page.h"
#include "def.h"
#include "malloc.h"
#include "core.h"
#include "isr.h"
#include "systemService.h"
#include "apic.h"
#include "apic.h"
#include "window.h"
#include "malloc.h"
#include "coprocessor.h"

int g_task_array_lock[256] ;
int g_task_list_lock [256];

int g_last_task_tid[TASK_LIMIT_TOTAL];

unsigned long long g_cpu_prev_tick[TASK_LIMIT_TOTAL];
unsigned long long g_cpu_tick[TASK_LIMIT_TOTAL];
unsigned long long g_cpu_start_tick[TASK_LIMIT_TOTAL];


int g_tagMsg = 0;



void enter_task_array_lock() {
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	__enterSpinlock(&g_task_array_lock[id]);
}

void leave_task_array_lock() {
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	__leaveSpinlock(&g_task_array_lock[id]);
}

void enter_task_list_lock() {
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	__enterSpinlock(&g_task_list_lock[id]);
}

void leave_task_list_lock() {
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	__leaveSpinlock(&g_task_list_lock[id]);
}

void enter_task_array_lock_id(int id) {
	__enterSpinlock(&g_task_array_lock[id]);
}

void leave_task_array_lock_id(int id) {
	__leaveSpinlock(&g_task_array_lock[id]);
}


TASK_LIST_ENTRY *gTasksListPtr[256];
TASK_LIST_ENTRY* gTasksListPos[256];


TASK_LIST_ENTRY* GetTaskListHeader() {
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	TASK_LIST_ENTRY *  list = (TASK_LIST_ENTRY*)(TASKS_LIST_BASE + sizeof(TASK_LIST_ENTRY) * TASK_LIMIT_TOTAL * id);
	return list;
}



void InitTaskArray() {
	LPPROCESS_INFO tssbase = GetTaskTssBase();
	for (int i = 0; i < TASK_LIMIT_TOTAL; i++)
	{
		tssbase[i].status = TASK_OVER;
	}

	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	g_task_list_lock[id] = 0;
	g_task_array_lock[id] = 0;
}


void InitTaskList() {

	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

	gTasksListPtr[id] = (TASK_LIST_ENTRY*)( TASKS_LIST_BASE + sizeof(TASK_LIST_ENTRY) * TASK_LIMIT_TOTAL * id);
	gTasksListPtr[id]->valid = 0;
	
	InitListEntry(&gTasksListPtr[id]->list);

	gTasksListPos[id] = gTasksListPtr[id];
}



TASK_LIST_ENTRY* SearchTaskListTid(int tid) {
	
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	TASK_LIST_ENTRY* result = 0;

	TASK_LIST_ENTRY *hdr = (TASK_LIST_ENTRY*)gTasksListPtr[id]->list.next;

	TASK_LIST_ENTRY *list = hdr;

	do
	{
		if (list->node->status == TASK_RUN && list->node->tid == tid) {

			break;
		}

		list = (TASK_LIST_ENTRY*)list->list.next;

	} while (list && list != hdr);

	return list;
}


TASK_LIST_ENTRY* SearchTaskListPid(int pid) {
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	TASK_LIST_ENTRY* result = 0;

	TASK_LIST_ENTRY* hdr = (TASK_LIST_ENTRY*)gTasksListPtr[id]->list.next;

	TASK_LIST_ENTRY* list = hdr;

	do
	{
		if (list->node->status == TASK_RUN && list->node->pid == pid) {

			break;
		}

		list = (TASK_LIST_ENTRY*)list->list.next;

	} while (list && list != hdr);

	return list;
}


TASK_LIST_ENTRY* GetFreeTaskList() {

	TASK_LIST_ENTRY* result = 0;

	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	TASK_LIST_ENTRY * list = (TASK_LIST_ENTRY*)(TASKS_LIST_BASE + sizeof(TASK_LIST_ENTRY) * TASK_LIMIT_TOTAL * id);

	int cnt = TASK_LIMIT_TOTAL;
	for (int i = 1; i < cnt; i++)
	{
		if (list[i].valid == 0  )
		{
			result = &list[i];
			break;
		}
	}

	return result;
}




TASK_LIST_ENTRY* InsertTaskList(int tid) {

	char szout[256];

	enter_task_list_lock();

	TASK_LIST_ENTRY* result = 0;

	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	TASK_LIST_ENTRY* list = (TASK_LIST_ENTRY*)(TASKS_LIST_BASE + sizeof(TASK_LIST_ENTRY) * TASK_LIMIT_TOTAL * id);

	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetTaskTssBase();

	for (int i = 1; i < TASK_LIMIT_TOTAL; i++)
	{
		if (list[i].valid == 0 ) {
			list[i].valid = TRUE;
			list[i].node = tss + tid;
			InsertListTail((LIST_ENTRY*)&(list->list), (LIST_ENTRY*)&(list[i].list) );

			result = &list[i];
			break;
		}
	}
	leave_task_list_lock();

	return result;
}

TASK_LIST_ENTRY* RemoveTaskListTid(int tid) {

	char szout[256];

	enter_task_list_lock();
	TASK_LIST_ENTRY* result = 0;
	int cnt = 0;

	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	TASK_LIST_ENTRY * head = gTasksListPtr[id];
	if (head) {
		TASK_LIST_ENTRY* first = (TASK_LIST_ENTRY*)&(head->list.next);
		TASK_LIST_ENTRY* list = first;
		do
		{
			if (list && list->valid && list->node && list->node->tid == tid)
			{
				RemoveList(&(gTasksListPtr[id]->list), (LIST_ENTRY*)&(list->list) );

				list->node->status = TASK_OVER;
				list->node = 0;

				list->valid = FALSE;

				result = list;
				if (gTasksListPos[id] == (TASK_LIST_ENTRY*)list) {
					gTasksListPos[id] = (TASK_LIST_ENTRY*)list->list.next;
				}

				//__printf(szout, "%s tid:%x,pid:%x,current pid:%x,current tid:%x,filename:%s,funcname:%s\n",__FUNCTION__,tid, pid, current->pid, current->tid, filename, funcname);
				
				break;
			}
			list = (TASK_LIST_ENTRY*)list->list.next;

			if (cnt++ >= TASK_LIMIT_TOTAL) {
				__printf(szout, "%s tid:%d max count:%d\r\n", __FUNCTION__,tid, cnt);
				break;
			}

		} while (list && list->valid && list != (TASK_LIST_ENTRY*)first);
	}

	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetTaskTssBase();

	if (tss[tid].level >= 3 && (tss[tid].tss.eflags& 0x20000) && tss[tid].espbase) {
		//__kFree(tss[tid].espbase);
	}

	LPPROCESS_INFO current = (LPPROCESS_INFO)GetCurrentTaskTssBase();

	LPPROCESS_INFO process = (LPPROCESS_INFO)tss + current->tid;
	
	current->status = TASK_OVER;
	process->status = TASK_OVER;

	DestroyThreadWindow(tid, id);

	leave_task_list_lock();

	return result;
}

TASK_LIST_ENTRY* RemoveTaskListPid(int pid) {
	char szout[256];
	enter_task_list_lock();
	TASK_LIST_ENTRY* result = 0;

	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

	int cnt = 0;
	TASK_LIST_ENTRY* head = gTasksListPtr[id];
	if (head) {
		TASK_LIST_ENTRY* first = (TASK_LIST_ENTRY*)&(head->list.next);
		TASK_LIST_ENTRY* list = first;
	
		do
		{
			if (list && list->valid && list->node && list->node->pid == pid)
			{
				RemoveList(&(gTasksListPtr[id]->list), (LIST_ENTRY*)&(list->list));

				list->node->status = TASK_OVER;
				list->node = 0;

				list->valid = FALSE;

				result = list;
				if (gTasksListPos[id] == (TASK_LIST_ENTRY*)list) {
					gTasksListPos[id] = (TASK_LIST_ENTRY*)list->list.next;
				}
				break;
			}
			list = (TASK_LIST_ENTRY*)list->list.next;

			if (cnt++ >= TASK_LIMIT_TOTAL) {
				
				__printf(szout, "%s max count:%d\r\n", __FUNCTION__, cnt);
				break;
			}

		} while (list && list->valid && list != (TASK_LIST_ENTRY*)first);
	}

	LPPROCESS_INFO current = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetTaskTssBase();
	LPPROCESS_INFO process = (LPPROCESS_INFO)tss + current->tid;

	current->status = TASK_OVER;
	process->status = TASK_OVER;

	for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
		if (tss[i].pid == pid) {
			tss[i].status = TASK_OVER;
		}
	}

	__kFreeProcess(pid);

	leave_task_list_lock();

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

void __terminateTask(int tid, char* filename, char* funcname, DWORD lpparams) {

	RemoveTaskListTid(tid);
	__sleep(-1);
}



void clearTssBuf(LPPROCESS_INFO tss) {
	__memset((CHAR*)tss, 0, sizeof(PROCESS_INFO));
	tss->status = TASK_SUSPEND;

	tss->tss.iomapEnd = 0xff;
	tss->tss.iomapOffset = 136;

	//tss->tss.trap = 1;
}


int __getFreeTask(LPTASKRESULT ret) {
	int result = 0;
	if (ret == 0)
	{
		return FALSE;
	}
	ret->lptss = 0;
	ret->number = 0;

	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	enter_task_array_lock();

	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetTaskTssBaseId(id);
	for (int i = 0;i < TASK_LIMIT_TOTAL; i++)
	{
		if (tss[i].status == TASK_OVER)
		{
			clearTssBuf(&tss[i]);

			tss[i].status = TASK_SUSPEND;

			tss[i].cpuid = id;

			ret->number = i;
			ret->lptss = &tss[i];
			result = 1;
			break;
		}
	}

	leave_task_array_lock();

	return result;
}



LPPROCESS_INFO __findProcessFuncName(const char * funcname) {

	LPPROCESS_INFO p = (LPPROCESS_INFO)GetTaskTssBase();
	for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
		if (p[i].status == TASK_RUN && __strcmp(p[i].funcname, funcname) == 0) {
			return & p[i];
		}
	}
	return FALSE;
}

LPPROCESS_INFO __findProcessFileName(char * filename) {
	LPPROCESS_INFO p = (LPPROCESS_INFO)GetTaskTssBase();
	for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
		if (p[i].status == TASK_RUN && __strcmp(p[i].filename, filename) == 0) {
			return &p[i];
		}
	}
	return FALSE;
}



LPPROCESS_INFO __findProcessByPid(int pid) {
	LPPROCESS_INFO p = (LPPROCESS_INFO)GetTaskTssBase();
	for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
		if (p[i].status == TASK_RUN && p[i].pid == pid ) {
			return &p[i];
		}
	}
	return FALSE;
}


LPPROCESS_INFO __findProcessByTid(int tid) {
	LPPROCESS_INFO p = (LPPROCESS_INFO)GetTaskTssBase();
	for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
		if (p[i].status == TASK_RUN && p[i].tid == tid) {
			return &p[i];
		}
	}
	return FALSE;
}


int __terminateByFileName(char * filename) {
	enter_task_array_lock();;
	LPPROCESS_INFO p = __findProcessFileName(filename);
	if (p)
	{
		p->status = TASK_OVER;
	}
	leave_task_array_lock();
	return FALSE;
}

int __terminateByFuncName(char * funcname) {
	enter_task_array_lock();
	PROCESS_INFO * p = __findProcessFuncName(funcname);
	if (p)
	{
		p->status = TASK_OVER;

	}
	leave_task_array_lock();
	return FALSE;
}

int __terminatePid(int pid) {
	enter_task_array_lock();
	PROCESS_INFO* p = __findProcessByPid(pid);
	if (p)
	{
		p->status = TASK_OVER;
	}
	leave_task_array_lock();
	return 0;
}


int __terminateTid(int tid) {
	enter_task_array_lock();
	PROCESS_INFO* p = __findProcessByTid(tid);
	if (p)
	{
		p->status = TASK_OVER;
	}
	leave_task_array_lock();
	return 0;
}



int __pauseTid(int tid) {
	enter_task_array_lock();
	PROCESS_INFO* p = __findProcessByTid(tid);
	if (p)
	{
		p->status = TASK_SUSPEND;
	}
	leave_task_array_lock();
	return 0;
}


int __resumeTid(int tid) {
	enter_task_array_lock();
	PROCESS_INFO* p = __findProcessByTid(tid);
	if (p)
	{
		p->status = TASK_RUN;
	}
	leave_task_array_lock();
	return 0;
}


int __pausePid(int pid) {
	enter_task_array_lock();
	PROCESS_INFO* p = __findProcessByPid(pid);
	if (p)
	{
		p->status = TASK_SUSPEND;
	}
	leave_task_array_lock();
	return 0;
}


int __resumePid(int pid) {
	enter_task_array_lock();
	PROCESS_INFO* p = __findProcessByPid(pid);
	if (p)
	{
		p->status = TASK_RUN;
	}
	leave_task_array_lock();
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
	char szout[256];
	int ret = 0;

	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetTaskTssBase();
	LPPROCESS_INFO proc = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	LPPROCESS_INFO prev = (LPPROCESS_INFO)(tss + proc->tid);
	LPPROCESS_INFO next = prev;

	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	ret = __GetSpinlock(&g_task_array_lock[id]);
	if (ret == 0) {
		return prev;
	}

	if (prev->status == TASK_TERMINATE || proc->status == TASK_TERMINATE) {
		prev->status = TASK_OVER;
		proc->status = TASK_OVER;
		if (prev->tid == prev->pid) {
			//__kFreeProcess(prev->pid);
		}
		else {
			//__kFree(prev->espbase);
		}
	}
	else if (prev->status == TASK_OVER || proc->status == TASK_OVER) {
		proc->status = TASK_OVER;
		prev->status = TASK_OVER;
		//__printf(szout, "%s prev tss status TASK_OVER error!\r\n",__FUNCTION__);
	}
	else if (proc->status == TASK_RUN && prev->status == TASK_RUN)
	{
		if (proc->sleep) {
			proc->sleep--;
			prev->sleep = proc->sleep;
		}
		else if (prev->sleep) {
			prev->sleep--;
			proc->sleep = prev->sleep;
		}
		else {
			proc->counter++;
		}

		proc->frac_slice++;
		if (proc->frac_slice >= proc->slice) {
			proc->frac_slice = 0;
		}
		else {
			goto __SingleTssSchedule_end;
		}
	}
	else if (proc->status == TASK_SUSPEND || prev->status == TASK_SUSPEND) {
		//process->status = TASK_SUSPEND;
		//prev->status = TASK_SUSPEND;
	}
	else if (prev->status != proc->status) {
		__printf(szout, "%s prev tss status %d/%d error!\r\n", prev->status, proc->status);
	}
	else {
		__printf(szout, "__kTaskSchedule process status:%d, prev status:%d error\r\n", proc->status, prev->status);
		goto __SingleTssSchedule_end;
	}
#ifdef TASK_SWITCH_PRIORITY
	next = GetReadyProcess();
	if(next == prev) {
		goto __SingleTssSchedule_end;
	}
#else
	do {
		next++;
		if (next - tss >= TASK_LIMIT_TOTAL) {
			next = tss;
		}

		if (next == prev) {
			if (next->status == TASK_OVER) {
				__printf(szout, "%s %d status error!\r\n",__FUNCTION__,__LINE__);
			}
			goto __SingleTssSchedule_end;
		}

		if (id != next->cpuid) {
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
#endif

	g_last_task_tid[id] = proc->tid;

	proc->tss.eax = env->eax;
	proc->tss.ecx = env->ecx;
	proc->tss.edx = env->edx;
	proc->tss.ebx = env->ebx;
	proc->tss.esp = env->esp;
	proc->tss.ebp = env->ebp;
	proc->tss.esi = env->esi;
	proc->tss.edi = env->edi;
	proc->tss.ss = env->ss;
	proc->tss.gs = env->gs;
	proc->tss.fs = env->fs;
	proc->tss.ds = env->ds;
	proc->tss.es = env->es;

	proc->tss.eip = env->eip;
	proc->tss.cs = env->cs;
	proc->tss.eflags = env->eflags;

	DWORD old_cr3 = 0;
	__asm {
		mov eax, cr3
		mov [old_cr3], eax
	}
	proc->tss.cr3 = old_cr3;

	//切换到新任务的cr3和ldt会被自动加载，但是iret也会加载cr3和ldt，因此不需要手动加载
	//DescriptTableReg old_ldt;
	// 	__asm {
	//		sldt old_ldt;
	// 	}
	//process->tss.ldt = old_ldt.addr;

	debugReg(next, proc);

	char* fenvprev = (char*)g_fpu_status[id] + (prev->tid << 9);
	//If a memory operand is not aligned on a 16-byte boundary, regardless of segment
	//The assembler issues two instructions for the FSAVE instruction 
	// (an FWAIT instruction followed by an FNSAVE instruction), 
	//and the processor executes each of these instructions separately.
	//If an exception is generated for either of these instructions,
	// the save EIP points to the instruction that caused the exception.
	char* fenvnext = (char*)g_fpu_status[id] +  (next->tid << 9);
	if (proc->fpu == 0 || prev->fpu == 0) {
		__asm {
			fninit
			mov eax, fenvprev
			FxSAVE[eax]
		}
		proc->fpu++;
		prev->fpu++;
	}
	if (next->fpu == 0) {
		next->fpu++;
		__asm {
			fninit
			mov eax, fenvnext
			FxSAVE[eax]
		}
	}
	__asm {
		mov eax, fenvprev
		FxSAVE[eax]
		////fsave [fenv]

		mov eax, fenvnext
		////frstor [fenv]
		fxrstor[eax]
	}

	if (prev->copyMap == 0) {
		int off = OFFSETOF(TSS, intMap);
		__memcpy((char*)prev, (char*)proc, off);
		off = OFFSETOF(PROCESS_INFO, level);
		int lsize = sizeof(PROCESS_INFO) - off;
		__memcpy((char*)prev + off, (char*)proc + off, lsize);
	}
	else {
		__memcpy((char*)prev, (char*)proc, sizeof(PROCESS_INFO));
	}
	if (next->copyMap == 0) {
		int off = OFFSETOF(TSS, intMap);
		__memcpy((char*)proc, (char*)next, off);
		off = OFFSETOF(PROCESS_INFO, level);
		int lsize = sizeof(PROCESS_INFO) - off;
		__memcpy((char*)proc + off, (char*)next + off, lsize);
	}
	else {
		__memcpy((char*)proc, (char*)next, sizeof(PROCESS_INFO));
	}

	env->eax = proc->tss.eax;
	env->ecx = proc->tss.ecx;
	env->edx = proc->tss.edx;
	env->ebx = proc->tss.ebx;
	env->esp = proc->tss.esp;
	env->ebp = proc->tss.ebp;
	env->esi = proc->tss.esi;
	env->edi = proc->tss.edi;
	env->gs = proc->tss.gs;
	env->fs = proc->tss.fs;
	env->ds = proc->tss.ds;
	env->es = proc->tss.es;
	env->ss = proc->tss.ss;

__SingleTssSchedule_end:
	ret = __leaveSpinlock(&g_task_array_lock[id]);
	return next;
}
#else
LPPROCESS_INFO SingleTssSchedule(LIGHT_ENVIRONMENT* env) {
	char szout[256];
	
	int ret = 0;
	LPPROCESS_INFO process = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetTaskTssBase();
	LPPROCESS_INFO prev = (LPPROCESS_INFO)(tss + process->tid);
	TASK_LIST_ENTRY* next = (TASK_LIST_ENTRY*)gTasksListPos[id]->list.next;

	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	ret = __GetSpinlock(&g_task_list_lock[id]);
	if (ret == 0) {
		return prev;
	}
	ret = __GetSpinlock(&g_task_array_lock[id]);
	if (ret == 0) {
		__leaveSpinlock(&g_task_list_lock[id]);
		return prev;
	}

	if (prev->status == TASK_TERMINATE  ) {
		prev->status = TASK_OVER;
		process->status = TASK_OVER;
	}

	if (process->status == TASK_TERMINATE) {
		process->status = TASK_OVER;
		prev->status = TASK_OVER;
	}

	if (prev->status == TASK_OVER || process->status == TASK_OVER) {
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

	if (prev->status == TASK_RUN)
	{
		if (prev->sleep) {
			prev->sleep--;
		}
		else {
			prev->counter++;
		}
	}

	if (process->status == TASK_SUSPEND) {
	}

	if (prev->status == TASK_SUSPEND) {
	}

	process->frac_slice++;
	if (process->frac_slice >= process->slice) {
		process->frac_slice = 0;
	}
	else {
		goto __SingleTssSchedule_end;
	}

	TASK_LIST_ENTRY* ptr = next;
	do {

		if (next->node->cpuid == id) {
			if (next->node->status == TASK_TERMINATE) {
				next->node->status = TASK_OVER;
			}

			if (next->node->status == TASK_RUN) {
				if (next->node->sleep) {
					next->node->sleep--;
				}
				else {
					break;
				}
			}
		}

		next = (TASK_LIST_ENTRY * )(next->list.next);
		if (next == ptr) {
			if (process->status == TASK_SUSPEND || process->status == TASK_OVER || process->status == TASK_TERMINATE) {
				__printf(szout, "%s %d task status:%d error\r\n", __FUNCTION__,__LINE__,process->status);
			}
			goto  __SingleTssSchedule_end;
		}

	} while (next && (next != ptr) );

	gTasksListPos[id] = next;
	g_last_task_tid[id] = prev->tid;

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

	DWORD old_cr3 = 0;
	__asm {
		mov eax, cr3
		mov old_cr3, eax
	}
	process->tss.cr3 = old_cr3;

	//切换到新任务的cr3和ldt会被自动加载，但是iret也会加载cr3和ldt，因此不需要手动加载
	//DescriptTableReg ldtreg;
	// 	__asm {
	//		sldt ldtreg;
	// 	}
	//process->tss.ldt = ldtreg.addr;

	debugReg(next->node, process);

	char* fenvprev = (char*)g_fpu_status[id] + (process->tid * 512);
	char* fenvnext = (char*)g_fpu_status[id] + (next->node->tid * 512);
	if (process->fpu == 0) {
		__asm {
			fninit
			mov eax, fenvprev
			FxSAVE[eax]
		}
		process->fpu++;
	}
	if (next->node->fpu == 0) {
		next->node->fpu ++;
		__asm {
			fninit
			mov eax, fenvnext
			FxSAVE[eax]
		}
	}
	__asm {
		//fninit
		//FNCLEX
		////fwait

		mov eax, fenvprev
		FxSAVE ds:[eax]
		////fsave [fenv]

		mov eax, fenvnext
		////frstor [fenv]
		fxrstor ds:[eax]
	}

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
		__memcpy((char*)process, (char*)next->node, off);
		off = OFFSETOF(TSS, iomapEnd);
		int lsize = sizeof(PROCESS_INFO) - off;
		__memcpy((char*)process + off, (char*)next->node + off, lsize);
	}
	else {
		__memcpy((char*)process, (char*)next->node, sizeof(PROCESS_INFO));
	}

	//tasktest();

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
	ret = __leaveSpinlock(&g_task_array_lock[id]);
	ret = __leaveSpinlock(&g_task_list_lock[id]);
	return next->node;
}
#endif

#ifdef TASK_SWITCH_ARRAY
LPPROCESS_INFO MultipleTssSchedule(LIGHT_ENVIRONMENT* env) {
	char szout[256];
	int ret = 0;
	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetTaskTssBase();
	LPPROCESS_INFO process = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	LPPROCESS_INFO current = (LPPROCESS_INFO)(tss + process->tid);
	LPPROCESS_INFO next = current;
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	ret = __GetSpinlock(&g_task_array_lock[id]);
	if (ret == 0) {
		return current;
	}

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

		process->frac_slice++;
		if (process->frac_slice >= process->slice) {
			process->frac_slice = 0;
		}
		else {
			goto __MultipleTssSchedule_end;
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
#ifdef TASK_SWITCH_PRIORITY
	next = GetReadyProcess();
	if (next == current) {
		goto __MultipleTssSchedule_end;
	}
#else
	do {
		next++;
		if (next - tss >= TASK_LIMIT_TOTAL) {
			next = tss;
		}

		if (next == current) {
			goto __MultipleTssSchedule_end;
		}

		if (id != next->cpuid) {
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
#endif
	g_last_task_tid[id] = current->tid;

	//切换到新任务的cr3和ldt会被自动加载，但是iret也会加载cr3和ldt，因此不需要手动加载
	//DescriptTableReg ldtreg;
	// 	__asm {
	//		sldt ldtreg;
	// 	}
	//process->tss.ldt = ldtreg.addr;

	debugReg(next, current);

	char* fenvprev = (char*)g_fpu_status[id] +(current->tid << 9);
	//If a memory operand is not aligned on a 16-byte boundary, regardless of segment
	//The assembler issues two instructions for the FSAVE instruction 
	// (an FWAIT instruction followed by an FNSAVE instruction), 
	//and the processor executes each of these instructions separately.
	//If an exception is generated for either of these instructions,
	// the save EIP points to the instruction that caused the exception.
	char* fenvnext = (char*)g_fpu_status[id] + (next->tid << 9);
	if (current->fpu == 0) {
		__asm {
			fninit
			mov eax, fenvprev
			FxSAVE[eax]
		}
		current->fpu++;
	}
	if (next->fpu == 0) {
		next->fpu++;
		__asm {
			fninit
			mov eax, fenvnext
			FxSAVE[eax]
		}
	}
	__asm {
		//fninit
		//FNCLEX
		////fwait

		mov eax, fenvprev
		FxSAVE[eax]
		////fsave [fenv]

		mov eax, fenvnext
		////frstor [fenv]
		fxrstor[eax]
	}

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

	__MultipleTssSchedule_end:
	__leaveSpinlock(&g_task_array_lock[id]);
	return next;
}
#else
LPPROCESS_INFO MultipleTssSchedule(LIGHT_ENVIRONMENT* env) {

	char szout[256];
	int ret = 0;
	LPPROCESS_INFO proc = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetTaskTssBase();
	LPPROCESS_INFO prev = (LPPROCESS_INFO)(tss + proc->tid);
	TASK_LIST_ENTRY* next = (TASK_LIST_ENTRY*)gTasksListPos[id]->list.next;
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	ret = __GetSpinlock(&g_task_list_lock[id]);
	if (ret == 0) {
		return prev;
	}
	ret = __GetSpinlock(&g_task_array_lock[id]);
	if (ret == 0) {
		__leaveSpinlock(&g_task_list_lock[id]);
		return prev;
	}

	if (prev->status == TASK_TERMINATE) {
		prev->status = TASK_OVER;
		proc->status = TASK_OVER;
	}

	if (proc->status == TASK_TERMINATE) {
		proc->status = TASK_OVER;
		prev->status = TASK_OVER;
	}

	if (prev->status == TASK_OVER || proc->status == TASK_OVER) {
		//__printf(szout, "%s prev tss status TASK_OVER error!\r\n",__FUNCTION__);
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

	if (prev->status == TASK_RUN)
	{
		if (prev->sleep) {
			prev->sleep--;
		}
		else {
			prev->counter++;
		}
	}

	if (proc->status == TASK_SUSPEND) {
	}

	if (prev->status == TASK_SUSPEND) {
	}

	proc->frac_slice++;
	if (proc->frac_slice >= proc->slice) {
		proc->frac_slice = 0;
	}
	else {
		goto __MultipleTssSchedule_end;
	}

	TASK_LIST_ENTRY* ptr = next;
	do {
		if (next->node->cpuid == id) {
			if (next->node->status == TASK_TERMINATE) {
				next->node->status = TASK_OVER;
			}

			if (next->node->status == TASK_RUN) {
				if (next->node->sleep) {
					next->node->sleep--;
				}
				else {
					break;
				}
			}
		}

		next = (TASK_LIST_ENTRY*)(next->list.next);
		if (next->node == ptr->node || next == ptr) {

			if (proc->status == TASK_SUSPEND || proc->status == TASK_OVER || proc->status == TASK_TERMINATE) {
				__printf(szout, "current task status:%d error\r\n", proc->status);
			}
			goto  __MultipleTssSchedule_end;
		}
	} while (next && (next != ptr));

	g_last_task_tid[id] = proc->tid;

	gTasksListPos[id] = next;

	//切换到新任务的cr3和ldt会被自动加载，但是iret也会加载cr3和ldt，因此不需要手动加载
	//DescriptTableReg ldtreg;
	// 	__asm {
	//		sldt ldtreg;
	// 	}
	//process->tss.ldt = ldtreg.addr;

	debugReg(next->node, proc);

	char* fenvprev = (char*)g_fpu_status[id] + (proc->tid << 9);
	//If a memory operand is not aligned on a 16-byte boundary, regardless of segment
	//The assembler issues two instructions for the FSAVE instruction 
	// (an FWAIT instruction followed by an FNSAVE instruction), 
	//and the processor executes each of these instructions separately.
	//If an exception is generated for either of these instructions,
	// the save EIP points to the instruction that caused the exception.
	char* fenvnext = (char*)g_fpu_status[id] + (next->node->tid << 9);
	if (proc->fpu == 0) {
		__asm {
			fninit
			mov eax, fenvprev
			FxSAVE[eax]
		}
		proc->fpu++;
	}
	if (next->node->fpu == 0) {
		next->node->fpu++;
		__asm {
			fninit
			mov eax, fenvnext
			FxSAVE[eax]
		}
	}
	__asm {
		//fninit
		//FNCLEX
		////fwait

		mov eax, fenvprev
		FxSAVE[eax]
		////fsave [fenv]

		mov eax, fenvnext
		////frstor [fenv]
		fxrstor[eax]
	}

	if (prev->copyMap == 0) {
		int off = OFFSETOF(TSS, intMap);
		__memcpy((char*)prev, (char*)proc, off);
		off = OFFSETOF(TSS, iomapEnd);
		int lsize = sizeof(PROCESS_INFO) - off;
		__memcpy((char*)prev + off, (char*)proc + off, lsize);
	}
	else {
		__memcpy((char*)prev, (char*)proc, sizeof(PROCESS_INFO));
	}
	if (proc->copyMap == 0) {
		int off = OFFSETOF(TSS, intMap);
		__memcpy((char*)proc, (char*)next->node, off);
		off = OFFSETOF(TSS, iomapEnd);
		int lsize = sizeof(PROCESS_INFO) - off;
		__memcpy((char*)proc + off, (char*)next->node + off, lsize);
	}
	else {
		__memcpy((char*)proc, (char*)next->node, sizeof(PROCESS_INFO));
	}

__MultipleTssSchedule_end:
	ret = __leaveSpinlock(&g_task_array_lock[id]);
	ret = __leaveSpinlock(&g_task_list_lock[id]);
	return next->node;
}
#endif




extern "C"  __declspec(dllexport) DWORD __kTaskSchedule(LIGHT_ENVIRONMENT* env) {

	char szout[256];
	
	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetTaskTssBase();
	LPPROCESS_INFO current  = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	LPPROCESS_INFO prev = (LPPROCESS_INFO)(tss + current->tid);

	unsigned long long time1 = __krdtsc();

	if (current->prev_tick && prev->prev_tick) {
		current->tick += time1 - current->prev_tick;
		prev->tick += time1 - prev->prev_tick;

		prev->prev_tick = 0;
		current->prev_tick = 0;
		//__printf(szout, "%s %d current tick:%I64x prev tick:%I64x\r\n",__FUNCTION__, __LINE__, current->tick, prev->tick);
	}

	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	if (g_pm_enable == 0) {	
		if (g_cpu_prev_tick[id]) {
			g_cpu_tick[id] += time1 - g_cpu_prev_tick[id];
			g_cpu_prev_tick[id] = 0;
		}
	}
	else {
		DWORD high = 0;
		DWORD low = 0;
		readmsr(MSR_IA32_MPERF, &low, &high);
		unsigned long long aperf = ((unsigned long long)high << 32) + low;
		if(aperf > time1) {
			g_cpu_tick[id] = aperf - time1;
		}
		else {
			g_cpu_tick[id] = time1 - aperf;
		}
		
		g_cpu_prev_tick[id] = 0;
		if ((g_tagMsg++) % 0x100 == 0x100) {
			__printf(szout, "cpu:%d tick:%I64x,aperf:%i64x,timer:%i64x\r\n", id, g_cpu_tick[id], aperf,time1);
		}
	}

	__kApicTimerProc();

	ActiveApTask(TASK_SWITCH_VECTOR);
	
#ifndef SINGLE_TASK_TSS
	__asm {
		clts			//multiple tss for task switch,must to do this
	}
#endif

	if (prev->tid != current->tid) {
		__printf(szout, "__kTaskSchedule process tid:%d, prev tid:%d not same\r\n", prev->tid, current->tid);
		return 0;
	}

	V86ProcessCheck(env,current,prev);

#ifdef SINGLE_TASK_TSS
	LPPROCESS_INFO next = SingleTssSchedule(env);
#else
	LPPROCESS_INFO next = MultipleTssSchedule(env);
#endif
	
	__int64 time2 = __krdtsc();

	/*
	if (next && (g_tagMsg++) % 0x100 == 0 && g_tagMsg == 0x100) {
		__int64 deltaTime = time2 - time1;

		DWORD cpureq;
		DWORD maxreq;
		DWORD busreq;
		__cpuFreq(&cpureq, &maxreq, &busreq);
		__int64 cpurate = cpureq;

		__printf(szout,
			"current link:%x,prev link:%x,next link:%x,stack eflags:%x,current eflags:%x,prev eflags:%x,next eflags:%x,new task pid:%d, tid:%d, old task pid:%d, tid:%d, timestamp:%i64x, cpurate:%i64x\r\n",
			prev->tss.link, current->tss.link, next->tss.link, env->eflags, prev->tss.eflags, current->tss.eflags, next->tss.eflags,
			current->pid, current->tid, next->pid, next->tid, deltaTime, cpurate);
	}
	*/

	current->prev_tick = time2;
	if (next) {
		next->prev_tick = time2;
	}

	//__printf(szout, "%s %d current prev tick:%I64x next prev tick:%I64x\r\n", __FUNCTION__,__LINE__,current->prev_tick,next->prev_tick);

	if (g_pm_enable == 0) {
		g_cpu_prev_tick[id] = time2;
	}
	else {
		g_cpu_prev_tick[id] = 0;
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
		else if (seq >= 0x20 && seq < 0x30) {
			continue;
		}
		else {
			ivt[seq] = vector;
		}
	}
}








int __initTask0(char * filename,char *funcname,int showx,int showy) {
	char szout[256];
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	LPPROCESS_INFO tssbase = SetTaskTssBase();

	InitTaskArray();

	TASKRESULT freeTss;
	__getFreeTask(&freeTss);
	int tid = freeTss.number;

	unsigned long stacktop = (unsigned long)(KERNEL_STACK_BASE + KTASK_STACK_SIZE * (id + 1) - STACK_TOP_DUMMY);
	//unsigned long stack0top = (unsigned long)(g_stack0_base[id] + TASK_STACK0_SIZE * ( 0 + 1) - STACK_TOP_DUMMY);

	//why not use process0 but lpproc?
	LPPROCESS_INFO process0  = (LPPROCESS_INFO)GetCurrentTaskTssBase();

	LPPROCESS_INFO lpproc = (LPPROCESS_INFO)(freeTss.lptss);
	
	__strcpy(process0->filename, (char*)filename);
	__strcpy(process0->funcname, (char*)funcname);
	process0->status = TASK_RUN;
	process0->tid = tid;
	process0->pid = tid;
	process0->ppid = 0;
	process0->cpuid = id;
	process0->espbase = stacktop;
	process0->level = 0;
	process0->vaddr = 0;
	process0->lpvasize = &lpproc->va_size;
	process0->va_size = 0;

	process0->showX = showx;
	process0->showY = showy;
	process0->window = 0;

	process0->slice = 1;
	process0->frac_slice = 0;
	process0->counter = 0;
	process0->sleep = 0;
	process0->sleep_total = 0;
	process0->errorno = 0;

	process0->videoBase = (char*)gGraphBase;

	process0->moduleBase = KERNEL_DLL_BASE;

	process0->heapCnt = 1;
	process0->lpHeapCnt = &lpproc->heapCnt;
	process0->heap_lock = 0;
	process0->lpheap_lock = &lpproc->heap_lock;
	process0->lpHeapBase = &lpproc->heapBase;

	process0->large_heap_size = 0;
	process0->fast_heap_large = 0;

	process0->delta = 0;
	process0->priority = 0;
	process0->tick = 0;
	process0->prev_tick = 0;
	process0->tick_start = __krdtsc();

	int bsp = IsBspProcessor();
	if (bsp) {
		process0->heapBase = (char*)BSP_HEAP_BASE;
		//__printf(szout,"%s %d process0->lpHeapBase[0]:%x\r\n", __FUNCTION__, __LINE__, process0->lpHeapBase[0]);
		process0->heapsize = HEAP_SIZE; 
		process0->fast_heap = (char*)BSP_FAST_HEAP;
	}
	else {
		DWORD size = HEAP_SIZE;
		char* buf = (char*)__kProcessMalloc(HEAP_SIZE, &size, 0, id, 0, PAGE_READWRITE | PAGE_USERPRIVILEGE | PAGE_PRESENT);
		//__printf(szout,"%s %d __kProcessMalloc heap base:%x\r\n", __FUNCTION__, __LINE__, buf);
		process0->heapBase = (char*)buf;
		process0->heapsize = HEAP_SIZE;
		process0->fast_heap = (char*)__kProcessMalloc(HEAP_SIZE, &size, 0, id, 0, PAGE_READWRITE | PAGE_USERPRIVILEGE | PAGE_PRESENT);
		
	}
	__memset((char*) process0->heapBase, 0, HEAP_SIZE);
	__memset(process0->fast_heap, 0, HEAP_SIZE);

	__memcpy((char*)lpproc, (char*)process0, sizeof(PROCESS_INFO));
	
	__printf(szout, "%s %d cpu:%d *lpHeapCnt:%d,*lpheap_lock:%d,*lpHeapBase:%x\r\n",__FUNCTION__,__LINE__,id,
		*process0->lpHeapCnt, *process0->lpheap_lock, process0->lpHeapBase[0]);

#ifdef TASK_SWITCH_ARRAY

#else
	InitTaskList();
	InsertTaskList(tid);
#endif

	//__memset((char*)V86_TASKCONTROL_ADDRESS, 0, LIMIT_V86_PROC_COUNT*12);

	return tid;
}


//在V86模式下，CPL=3，执行特权指令时，或者要引起出错码为0的通用保护故障，或者要引起 非法操作码故障。
//由于CPL = 3， 所以如果IOPL < 3，那么执行CLI或STI指令将引起通用保护故障。
//输入 / 输出指令IN、INS、OUT或OUTS的 敏感条件仅仅是当前V86任务TSS内的I / O许可位图，而忽略EFLAGS中的IOPL。
//在V86模式下， 当IOPL < 3时，执行指令PUSHF、POPF、INT n及IRET会引起出错码为0的通用保护故障。
//采取上述措施的目的是使操作系统软件可以支持一个“虚拟EFLAGS”寄存器。



extern "C" void __declspec(naked) ApTaskSchedule(LIGHT_ENVIRONMENT* stack) {

	__asm {
		cli
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
		sub esp, NATIVE_STACK_LIMIT

		mov eax, KERNEL_MODE_DATA
		mov ds, ax
		mov es, ax
		MOV FS, ax
		MOV GS, AX
		mov ss, ax

	}

	{
		//LPPROCESS_INFO process = (LPPROCESS_INFO)GetCurrentTaskTssBase();
		char szout[256];
		//__printf(szout,"ApTaskSchedule\r\n");

		LPPROCESS_INFO next = SingleTssSchedule(stack);

		//ipi command need to send eoi to local apic
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

		iretd

		jmp ApTaskSchedule
	}
}

extern "C" void __declspec(dllexport) yield( LIGHT_ENVIRONMENT * stack) {
	
	//__asm{cli}
	int ret = IsBspProcessor();
	if (ret) {
#ifdef SINGLE_TASK_TSS
		LPPROCESS_INFO next = SingleTssSchedule(stack);
#else
		//need to run in another tss
		//LPPROCESS_INFO next = MultipleTssSchedule(stack);		//something error
		//__sleep(0);
		//LPPROCESS_INFO next = SingleTssSchedule(stack);

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

			clts

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

		clts

		iretd
	}

}

void tasktest(LPPROCESS_INFO gTasksListPtr, LPPROCESS_INFO gPrevTasksPtr) {
	static int gTestFlag = 0;
	if (gTestFlag >= 0 && gTestFlag <= -1)
	{
		char szout[256];
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


PROCESS_INFO* GetNextProcess() {
	char szout[256];
	int ret = 0;
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetTaskTssBase();
	LPPROCESS_INFO process = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	LPPROCESS_INFO current = (LPPROCESS_INFO)(tss + process->tid);
	LPPROCESS_INFO next = current;

	do {
		next++;
		if (next - tss >= TASK_LIMIT_TOTAL) {
			next = tss;
		}

		if (next == current) {
			return 0;
		}

		if (id != next->cpuid) {
			continue;
		}

		if (next->status == TASK_TERMINATE) {
			next->status = TASK_OVER;
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

	return next;
}



extern "C" __declspec(naked) int __kKernelProcess(LIGHT_ENVIRONMENT * stack) {
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
		MOV fs, ax
		MOV gs, ax
		mov ss, ax
	}

	while (1) {
		__asm {
			hlt
		}
	}

	__asm {
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
		ret
	}
}
