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
#include "taskPriority.h"
#include "window.h"
#include "coprocessor.h"
#include "tool.h"


int g_task_array_lock[256] ;

int g_last_task_tid[MAX_CPU_COUNT];

unsigned long long g_cpu_prev_tick[MAX_CPU_COUNT];
unsigned long long g_cpu_tick[MAX_CPU_COUNT];
unsigned long long g_cpu_start_tick[MAX_CPU_COUNT];

int g_tagMsg = 0;



void enter_task_array_lock() {
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	__enterSpinlock(&g_task_array_lock[id]);
}

void leave_task_array_lock() {
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	__leaveSpinlock(&g_task_array_lock[id]);
}

void enter_task_array_lock_id(int id) {
	__enterSpinlock(&g_task_array_lock[id]);
}

void leave_task_array_lock_id(int id) {
	__leaveSpinlock(&g_task_array_lock[id]);
}

PROCESS_INFO_LIST *gTasksListPtr[256];

//PROCESS_INFO_LIST *g_error_proc_list = 0;

PROCESS_INFO_LIST * GetTaskListId(int id) {
	return gTasksListPtr[id];
}

#ifdef TASK_SWITCH_ARRAY
void InitTaskArray() {
	LPPROCESS_INFO tssbase = GetTaskTssBase();
	for (int i = 0; i < MAX_CPU_COUNT; i++)
	{
		tssbase[i].status = TASK_OVER;
	}

	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

	g_task_array_lock[id] = 0;
}

int __GetFreeTask(LPTASKRESULT ret, int id) {
	int result = 0;
	if (ret == 0)
	{
		return FALSE;
	}
	ret->lptss = 0;
	ret->number = 0;

	//int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	//enter_task_array_lock();

	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetTaskTssBaseId(id);
	for (int i = 0; i < TASK_LIMIT_TOTAL; i++)
	{
		if (tss[i].status == TASK_OVER)
		{
			clearTssBuf(&tss[i]);

			tss[i].status = TASK_SUSPEND;

			tss[i].cpuid = id;

			ret->number = i;
			ret->lptss = &tss[i];
			result = 1;
		
			char* fenv = GetFpuBuf(id,ret->number);				// (char*)g_fpu_status[id] + (ret->number << 9);
			InitTaskCoprocessor(ret->lptss, fenv);

			break;
		}
	}
	//leave_task_array_lock();
	return result;
}

#else

void InitTaskList() {
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	int size = sizeof(PROCESS_INFO_LIST);
	char* gbuf = (char*)__kmalloc(0x1000);
	gTasksListPtr[id] = (PROCESS_INFO_LIST*)(gbuf +id* OFFSETOF(PROCESS_INFO_LIST,proc));
	InitListEntry((LIST_ENTRY*)&(gTasksListPtr[id]->list));

	//g_error_proc_list = (PROCESS_INFO_LIST*)__kmalloc(sizeof(PROCESS_INFO_LIST));
	//g_error_proc_list->proc.status = TASK_OVER;
	//g_error_proc_list->list.next = (LIST_ENTRY*)  &g_error_proc_list->list;
	//g_error_proc_list->list.prev = (LIST_ENTRY*)  &g_error_proc_list->list;
}

PROCESS_INFO_LIST* GetRemoteTaskListTid(int id,int tid) {

	PROCESS_INFO_LIST* result = 0;

	PROCESS_INFO_LIST *hdr = (PROCESS_INFO_LIST*)gTasksListPtr[id]->list.next;

	PROCESS_INFO_LIST *list = hdr;

	while (list)
	{
		if ( list->proc.tid == tid) {
			result = list;
			break;
		}

		list = (PROCESS_INFO_LIST*)list->list.next;
		if (list == hdr) {
			break;
		}
	}

	return result;
}


PROCESS_INFO_LIST* GetRemoteTaskListPid(int id, int pid) {

	PROCESS_INFO_LIST* result = 0;

	PROCESS_INFO_LIST *hdr = (PROCESS_INFO_LIST*)gTasksListPtr[id]->list.next;

	PROCESS_INFO_LIST *list = hdr;

	while (list)
	{
		if ( list->proc.pid == pid) {
			result = list;
			break;
		}

		list = (PROCESS_INFO_LIST*)list->list.next;
		if (list == hdr) {
			break;
		}
	}

	return result;
}

PROCESS_INFO_LIST* GetTaskListTid(int tid) {
	
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

	PROCESS_INFO_LIST* result = 0;

	PROCESS_INFO_LIST *hdr = (PROCESS_INFO_LIST*)gTasksListPtr[id]->list.next;

	PROCESS_INFO_LIST *list = hdr;

	while (list )
	{
		if ( list->proc.tid == tid) {
			result = list;
			break;
		}

		list = (PROCESS_INFO_LIST*)list->list.next;
		if (list == hdr) {
			break;
		}
	} 

	return result;
}


PROCESS_INFO_LIST* GetTaskListPid(int pid) {
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

	PROCESS_INFO_LIST* result = 0;

	PROCESS_INFO_LIST* hdr = (PROCESS_INFO_LIST*)gTasksListPtr[id]->list.next;

	PROCESS_INFO_LIST* list = hdr;

	while (list )
	{
		if (list->proc.pid == pid) {
			result = list;
			break;
		}

		list = (PROCESS_INFO_LIST*)list->list.next;
		if (list == hdr) {
			break;
		}
	}

	return result;
}


int InsertTaskList(PROCESS_INFO_LIST* p) {

	char szout[256];
	int ret = 0;

	int id = p->proc.cpuid;

	enter_task_array_lock_id(id);	
	PROCESS_INFO_LIST* list = (PROCESS_INFO_LIST*)gTasksListPtr[id];
	if (list) {
		ret = InsertListTail((LIST_ENTRY*)&(list->list), (LIST_ENTRY*)&(p->list));
	}
	leave_task_array_lock_id(id);

	return ret;
}


PROCESS_INFO_LIST* RemoveTaskList(PROCESS_INFO_LIST * p) {

	char szout[256];

	PROCESS_INFO_LIST* result = 0;

	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

	enter_task_array_lock();

	PROCESS_INFO_LIST * head = gTasksListPtr[id];
	if (head) {
		PROCESS_INFO_LIST* hdr = (PROCESS_INFO_LIST*)&(head->list.next);
		PROCESS_INFO_LIST* list = hdr;
		while (list)
		{
			if (list &&  list->proc.tid == p->proc.tid && list == p)
			{
				LPPROCESS_INFO current = (LPPROCESS_INFO)GetCurrentTaskTssBase();
				PROCESS_INFO_LIST* node = (PROCESS_INFO_LIST*)GetTaskListTid(current->tid);
				LPPROCESS_INFO proc = &node->proc;
				if (proc != &list->proc)
					__printf(szout, "%s %d proc:%x,list proc:%x\r\n", __FUNCTION__, __LINE__, proc, &list->proc);

				current->status = TASK_OVER;
				proc->status = TASK_OVER;

				PROCESS_INFO_LIST* ptr = list;
				do {
					if (ptr->proc.pid == p->proc.pid) {
						ptr->proc.status = TASK_OVER;
					}
					ptr = (PROCESS_INFO_LIST*)ptr->list.next;
				} while (ptr && ptr != list);

				//__kFreeProcess(proc);

				//DestroyThreadWindow(p->proc.tid, id);

				//RemoveNode(&(gTasksListPtr[id]->list), &(list->list));

				result = list;
				break;
			}
			list = (PROCESS_INFO_LIST*)list->list.next;
			if (list == hdr) {
				break;
			}
		}
	}

	leave_task_array_lock();

	return result;
}

PROCESS_INFO_LIST* RemoveTaskListTid(int tid) {

	char szout[256];
	
	PROCESS_INFO_LIST* result = 0;

	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

	enter_task_array_lock();

	PROCESS_INFO_LIST * head = gTasksListPtr[id];
	if (head) {
		PROCESS_INFO_LIST* hdr = (PROCESS_INFO_LIST*)&(head->list.next);
		PROCESS_INFO_LIST* list = hdr;
		while (list )
		{
			if (list &&  list->proc.tid == tid)
			{
				LPPROCESS_INFO current = (LPPROCESS_INFO)GetCurrentTaskTssBase();
				PROCESS_INFO_LIST* node = (PROCESS_INFO_LIST*)GetTaskListTid(current->tid);
				LPPROCESS_INFO proc = &node->proc;
				if (proc != &list->proc)
					__printf(szout, "%s %d proc:%x,list proc:%x\r\n", __FUNCTION__, __LINE__, proc, &list->proc);

				current->status = TASK_OVER;
				proc->status = TASK_OVER;
				list->proc.status = TASK_OVER;

				//DestroyThreadWindow(tid, id);

				//RemoveNode((LIST_ENTRY*)&(gTasksListPtr[id]->list), (LIST_ENTRY*)&(list->list));

				result = list;
				break;
			}
			list = (PROCESS_INFO_LIST*)list->list.next;
			if (list == hdr) {
				break;
			}
		} 
	}

	leave_task_array_lock();

	return result;
}

PROCESS_INFO_LIST* RemoveTaskListPid(int pid) {
	char szout[256];
	
	PROCESS_INFO_LIST* result = 0;

	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

	//__printf(szout, "%s %d\r\n", __FUNCTION__, __LINE__);

	enter_task_array_lock();

	PROCESS_INFO_LIST* head = gTasksListPtr[id];
	if (head) {
		PROCESS_INFO_LIST* hdr = (PROCESS_INFO_LIST*)&(head->list.next);
		PROCESS_INFO_LIST* list = hdr;
	
		while (list )
		{
			if (list && list->proc.pid == pid)
			{		

				LPPROCESS_INFO current = (LPPROCESS_INFO)GetCurrentTaskTssBase();

				PROCESS_INFO_LIST* node = GetTaskListTid(current->tid);
				LPPROCESS_INFO proc = &node->proc;
				if (proc != &list->proc)
					__printf(szout, "%s %d proc:%x,list proc:%x\r\n", __FUNCTION__, __LINE__, proc, &list->proc);

				current->status = TASK_OVER;
				proc->status = TASK_OVER;

				PROCESS_INFO_LIST* ptr = list;
				do {
					if (ptr->proc.pid == pid) {
						ptr->proc.status = TASK_OVER;
					}
					ptr = (PROCESS_INFO_LIST*) ptr->list.next;

				} while (ptr && ptr != list);

				//__kFreeProcess(proc);

				//DestroyThreadWindow(pid, id);

				//RemoveNode((LIST_ENTRY*)&(gTasksListPtr[id]->list), (LIST_ENTRY*)&(list->list));
				result = list;
				break;
			}
			list =(PROCESS_INFO_LIST*) list->list.next;
			if (list == hdr) {
				break;
			}
		} 
	}

	leave_task_array_lock();

	return result;
}


void __kExitProcess(PROCESS_INFO_LIST* list) {

	//int id = list->proc.cpuid;
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	RemoveNode((LIST_ENTRY*)&(gTasksListPtr[id]->list), (LIST_ENTRY*)&(list->list));

	__kFreeProcess(&list->proc);
	return;
}

#endif





void clearTssBuf(LPPROCESS_INFO tss) {
	__memset((CHAR*)tss, 0, sizeof(PROCESS_INFO));
	tss->status = TASK_SUSPEND;

	tss->tss.iomapEnd = 0xff;
	tss->tss.iomapOffset = 136;

	//tss->tss.trap = 1;
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


LPPROCESS_INFO SingleTssSchedule(LIGHT_ENVIRONMENT* env) {
	char szout[256];
	int ret = 0;

	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
		
	LPPROCESS_INFO proc = (LPPROCESS_INFO)GetCurrentTaskTssBase();
#ifdef TASK_SWITCH_ARRAY
	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetTaskTssBase();
	LPPROCESS_INFO prev = (LPPROCESS_INFO)(tss + proc->tid);
	LPPROCESS_INFO next = prev;
#else
	PROCESS_INFO_LIST* prevlist = (PROCESS_INFO_LIST*)GetRemoteTaskListTid(id, proc->tid);
	if (prevlist == 0)
	{
		__printf(szout, "%s %d error!\r\n", __FUNCTION__,__LINE__);
		return 0;			//error
	}
	LPPROCESS_INFO prev = &prevlist->proc;
	PROCESS_INFO_LIST* nextlist = (PROCESS_INFO_LIST*)prevlist->list.next;
	LPPROCESS_INFO next = (LPPROCESS_INFO)& nextlist->proc;
#endif

	if (prev->status == TASK_TERMINATE || proc->status == TASK_TERMINATE || 
		prev->status == TASK_OVER || proc->status == TASK_OVER) {
		prev->status = TASK_OVER;
		proc->status = TASK_OVER;
#ifndef TASK_SWITCH_ARRAY
		if(prev->tid == prev->pid){
			//prevlist = (PROCESS_INFO_LIST*)prevlist->list.next;
			//prev = &prevlist->proc;
			//__kExitProcess(prevlist);
		}
#endif
	}
	else if (proc->status == TASK_RUN  && prev->status == TASK_RUN )
	{
		if (proc->sleep) {
			proc->sleep--;
			prev->sleep = proc->sleep;
		}
		else if (prev->sleep) {
			prev->sleep--;
			proc->sleep = prev->sleep;
		}
		prev->counter++;

		proc->counter++;

		proc->frac_slice++;
		if (proc->frac_slice >= proc->slice) {
			proc->frac_slice = 0;
		}
		else {
			return 0;		//stop  task switching,continue to run old task
		}
	}
	else if (proc->status == TASK_SUSPEND  || prev->status == TASK_SUSPEND ) {
		proc->status = TASK_SUSPEND;
		prev->status = TASK_SUSPEND;
	}
	else if (prev->status != proc->status) {
		__printf(szout, "%s prev tss status %d/%d error!\r\n", prev->status, proc->status);
	}
	else {
		__printf(szout, "%s process status:%d, prev status:%d error\r\n", __FUNCTION__, proc->status, prev->status);
		return 0;	//error
	}

#ifdef TASK_SWITCH_PRIORITY
	next = PrioritySchedule();
	if(next == prev || next == 0) {
		return 0;
	}
#else
	do {
#ifdef TASK_SWITCH_ARRAY
		next++;
		if (next - tss >= TASK_LIMIT_TOTAL) {
			next = tss;
		}
#endif
		if (next == prev) {
			if (next->status == TASK_OVER || next->status == TASK_TERMINATE) {
				__printf(szout, "%s %d task status:%d error\r\n", __FUNCTION__, __LINE__, next->status);
			}
			return 0;		//something error
		}

		if (next->cpuid == id) {
			if (next->status == TASK_RUN) {
				if (next->sleep) {
					next->sleep--;
				}
				else {
					break;
				}
			}
		}
#ifndef TASK_SWITCH_ARRAY	
		PROCESS_INFO_LIST* tmplist = (PROCESS_INFO_LIST*)nextlist->list.next;
		if (next->status == TASK_TERMINATE || next->status == TASK_OVER) {
			next->status = TASK_OVER;
			if (next->tid == next->pid) {
			//	__kExitProcess(nextlist);
			}
		}
		nextlist = tmplist;
		next = (LPPROCESS_INFO)& nextlist->proc;
#endif
	} while (next && (next != prev));
#endif

	g_last_task_tid[id] = prev->tid;

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

#ifdef TASK_SWITCH_ARRAY
	char* fenvprev = (char*)g_fpu_status[id] + (prev->tid << 9);
	//If a memory operand is not aligned on a 16-byte boundary, regardless of segment
	//The assembler issues two instructions for the FSAVE instruction 
	// (an FWAIT instruction followed by an FNSAVE instruction), 
	//and the processor executes each of these instructions separately.
	//If an exception is generated for either of these instructions,
	// the save EIP points to the instruction that caused the exception.
	char* fenvnext = (char*)g_fpu_status[id] +  (next->tid << 9);
#else
	//nextlist = (PROCESS_INFO_LIST*)((char*)next - OFFSETOF(PROCESS_INFO_LIST, proc));
	char* fenvprev = prevlist->fpenv;
	char* fenvnext =  nextlist->fpenv;
#endif
	if (proc->status == TASK_RUN && prev->status == TASK_RUN) {
		if (proc->fpu == 0 || prev->fpu == 0) {
			//__memcpy(fenvprev, g_fpu_init_buf, sizeof(g_fpu_init_buf));
			__asm {
				//fninit
				mov eax, fenvprev
				FxSAVE[eax]
			}
			proc->fpu++;
			prev->fpu++;
		}
	}
	if (next->fpu == 0) {
		next->fpu++;
		//__memcpy(fenvnext, g_fpu_init_buf, sizeof(g_fpu_init_buf));
		__asm {
			fninit
			mov eax, fenvnext
			FxSAVE[eax]
		}
	}

#ifdef TASK_SWITCH_COPROCESSOR_LAZY
	__asm {
		mov eax, cr0
		or eax, 8		;设置 bit 3 (TS)
		mov cr0, eax
	}
#else
	if (proc->status == TASK_RUN && prev->status == TASK_RUN) {
		__asm {
			mov eax, fenvprev
			FxSAVE[eax]
			//fsave [eax]
		}
	}
	__asm{
		mov eax, fenvnext
		//frstor [eax]
		fxrstor[eax]
	}
#endif
	if (proc->status == TASK_RUN && prev->status == TASK_RUN) {
		if (prev->copyMap == 0) {
			int off = OFFSETOF(TSS, intMap);
			__memcpy((char*)prev, (char*)proc, off);
			int off1 = OFFSETOF(PROCESS_INFO, level);
			int off2 = OFFSETOF(PROCESS_INFO, filename);
			int lsize = off2 - off1;
			__memcpy((char*)prev + off1, (char*)proc + off1, lsize);
		}
		else {
			__memcpy((char*)prev, (char*)proc, sizeof(PROCESS_INFO));
		}
	}
	if (next->copyMap == 0) {
		int off = OFFSETOF(TSS, intMap);
		__memcpy((char*)proc, (char*)next, off);
		int off1 = OFFSETOF(PROCESS_INFO, level);
		int off2 = OFFSETOF(PROCESS_INFO, filename);
		int lsize = off2 - off1;
		__memcpy((char*)proc + off1, (char*)next + off1, lsize);
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

	DWORD regcr3 = proc->tss.cr3;
	__asm {
		mov eax, regcr3
		mov cr3,eax
	}

	return next;
}



LPPROCESS_INFO MultipleTssSchedule(LIGHT_ENVIRONMENT* env) {
	char szout[256];
	int ret = 0;

	LPPROCESS_INFO proc = (LPPROCESS_INFO)GetCurrentTaskTssBase();
#ifdef TASK_SWITCH_ARRAY
	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetTaskTssBase();
	LPPROCESS_INFO prev = (LPPROCESS_INFO)(tss + proc->tid);
	LPPROCESS_INFO next = prev;
#else
	PROCESS_INFO_LIST* prevlist = (PROCESS_INFO_LIST*)GetTaskListTid(proc->tid);
	if (prevlist == 0) {
		__printf(szout, "%s %d error!\r\n", __FUNCTION__,__LINE__);
		return 0;
	}
	LPPROCESS_INFO prev = &prevlist->proc;
	PROCESS_INFO_LIST* nextlist = (PROCESS_INFO_LIST*)prevlist->list.next;
	LPPROCESS_INFO next = (LPPROCESS_INFO)& nextlist->proc;
#endif

	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

	if (prev->status == TASK_TERMINATE || proc->status == TASK_TERMINATE||
		prev->status == TASK_OVER || proc->status == TASK_OVER) {
		prev->status = TASK_OVER;
		proc->status = TASK_OVER;
#ifndef TASK_SWITCH_ARRAY
		//RemoveNode(&(gTasksListPtr[id]->list), &(prevlist->list));
#endif
	}
	else if (proc->status == TASK_RUN || prev->status == TASK_RUN)
	{
		if (proc->sleep) {
			proc->sleep--;
			prev->sleep = proc->sleep;
		}
		else if (prev->sleep) {
			prev->sleep--;
			proc->sleep = prev->sleep;
		}
		prev->counter++;
		proc->counter++;
		proc->frac_slice++;
		if (proc->frac_slice >= proc->slice) {
			proc->frac_slice = 0;
		}
		else {
			return 0;
		}
	}
	else if (proc->status == TASK_SUSPEND || prev->status == TASK_SUSPEND) {
		proc->status = TASK_SUSPEND;
		prev->status = TASK_SUSPEND;
	}
	else {
		__printf(szout, "%s process status:%d, prev status:%d error\r\n",__FUNCTION__, prev->status, prev->status);
		return 0;
	}

#ifdef TASK_SWITCH_PRIORITY
	next = PrioritySchedule();
	if (next == prev) {
		return 0;
	}
#else
	do {
#ifdef TASK_SWITCH_ARRAY
		next++;
		if (next - tss >= TASK_LIMIT_TOTAL) {
			next = tss;
		}
#endif
		if (next == prev) {
			if (next->status == TASK_SUSPEND || next->status == TASK_OVER || next->status == TASK_TERMINATE) {
				__printf(szout, "%s %d current task status:%d error\r\n", __FUNCTION__, __LINE__, next->status);
			}
			return 0;
		}

		if (id == next->cpuid) {
			if (next->status == TASK_TERMINATE || next->status == TASK_OVER) {
				next->status = TASK_OVER;
#ifndef TASK_SWITCH_ARRAY
				PROCESS_INFO_LIST* tmplist = (PROCESS_INFO_LIST*)nextlist->list.next;
				//RemoveNode(&(gTasksListPtr[id]->list), &(nextlist->list));
				nextlist = tmplist;
				next = (LPPROCESS_INFO)&nextlist->proc;
				continue;
#endif
			}
			else if (next->status == TASK_RUN) {
				if (next->sleep) {
					next->sleep--;
				}
				else {
					break;
				}
			}
		}
#ifndef TASK_SWITCH_ARRAY
		nextlist = (PROCESS_INFO_LIST*)nextlist->list.next;
		next = (LPPROCESS_INFO)&nextlist->proc;
#endif
	} while (next && (next != prev));

#endif

	g_last_task_tid[id] = prev->tid;

	//切换到新任务的cr3和ldt会被自动加载，但是iret也会加载cr3和ldt，因此不需要手动加载
	//DescriptTableReg ldtreg;
	// 	__asm {
	//		sldt ldtreg;
	// 	}
	//process->tss.ldt = ldtreg.addr;

	debugReg(next, prev);

#ifdef TASK_SWITCH_ARRAY
	char* fenvprev = (char*)g_fpu_status[id] +(prev->tid << 9);
	//If a memory operand is not aligned on a 16-byte boundary, regardless of segment
	//The assembler issues two instructions for the FSAVE instruction 
	// (an FWAIT instruction followed by an FNSAVE instruction), 
	//and the processor executes each of these instructions separately.
	//If an exception is generated for either of these instructions,
	// the save EIP points to the instruction that caused the exception.
	char* fenvnext = (char*)g_fpu_status[id] + (next->tid << 9);
#else
	//nextlist = (PROCESS_INFO_LIST*)((char*)next - OFFSETOF(PROCESS_INFO_LIST, proc));
	char* fenvprev = prevlist->fpenv;
	char* fenvnext = nextlist->fpenv;
#endif
	if (prev->fpu == 0) {
		__asm {
			//fninit
			mov eax, fenvprev
			FxSAVE[eax]
		}
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

#ifdef TASK_SWITCH_COPROCESSOR_LAZY
	__asm {
		mov eax, cr0
		or eax, 8; 设置 bit 3 (TS)
		mov cr0, eax
	}
#else
	__asm {
		mov eax, fenvprev
		FxSAVE[eax]
		//fsave [fenv]

		mov eax, fenvnext
		//frstor [fenv]
		fxrstor[eax]
	}
#endif
	if (prev->copyMap == 0) {
		int off = OFFSETOF(TSS, intMap);
		__memcpy((char*)prev, (char*)proc, off);
		int off1 = OFFSETOF(PROCESS_INFO, level);
		int off2 = OFFSETOF(PROCESS_INFO, filename);
		int lsize = off2 - off1;
		__memcpy((char*)prev + off1, (char*)proc + off1, lsize);
	}
	else {
		__memcpy((char*)prev, (char*)proc, sizeof(PROCESS_INFO));
	}
	if (next->copyMap == 0) {
		int off = OFFSETOF(TSS, intMap);
		__memcpy((char*)proc, (char*)next, off);

		int off1 = OFFSETOF(PROCESS_INFO, level);
		int off2 = OFFSETOF(PROCESS_INFO, filename);
		int lsize = off2 - off1;
		__memcpy((char*)proc + off1, (char*)next + off1, lsize);
	}
	else {
		__memcpy((char*)proc, (char*)next, sizeof(PROCESS_INFO));
	}

	DWORD regcr3 = proc->tss.cr3;
	__asm {
		mov eax, regcr3
		mov cr3, eax
	}

	return next;
}



extern "C"  __declspec(dllexport) DWORD __kTaskSchedule(LIGHT_ENVIRONMENT* env) {

	char szout[256];

	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

#ifndef SINGLE_TASK_TSS
	__asm {
		//clts			// for multiple tss task switch
	}
#endif
	 
	__kApicTimerProc();

	//SwitchTaskCPU();

	int ret = __GetSpinlock(&g_task_array_lock[id]);
	if (ret) {
		unsigned long long tick1 = __krdtsc();
	
		LPPROCESS_INFO current = (LPPROCESS_INFO)GetCurrentTaskTssBase();
#ifdef TASK_SWITCH_ARRAY
		LPPROCESS_INFO tss = (LPPROCESS_INFO)GetTaskTssBase();
		LPPROCESS_INFO prev = (LPPROCESS_INFO)(tss + current->tid);
#else
		PROCESS_INFO_LIST* prevlist = (PROCESS_INFO_LIST*)GetRemoteTaskListTid(id,current->tid);
		LPPROCESS_INFO prev = &prevlist->proc;
#endif
		if (env->eip == (DWORD)g_hlt_addr) {

		}
		else {
			if (current->prev_tick) {
				current->tick_run += (tick1 - current->prev_tick);
			}
			if (prev->prev_tick) {
				prev->tick_run += (tick1 - prev->prev_tick);
			}
			if (g_cpu_prev_tick[id]) {
				g_cpu_tick[id] += (tick1 - g_cpu_prev_tick[id]);
			}
		}
		if (current->prev_tick) {
			current->tick_total += tick1 - current->prev_tick;
		}
		if (prev->prev_tick) {
			prev->tick_total += tick1 - prev->prev_tick;
		}

		if (prev->tid != current->tid) {
			//__printf(szout, "%s %d process tid:%d, prev tid:%d not same\r\n",__FUNCTION__,__LINE__, prev->tid, current->tid);
			//return 0;
		}

		V86ProcessCheck(env, current, 0);

#ifdef SINGLE_TASK_TSS
		LPPROCESS_INFO next = SingleTssSchedule(env);
#else
		LPPROCESS_INFO next = MultipleTssSchedule(env);
#endif
		__int64 tick2 = __krdtsc();

		current->tick_switch_cost = (current->tick_switch_cost + (tick2 - tick1))/2 ;

		current->prev_tick = tick2;
		if (next) {
			next->prev_tick = tick2;
		}

		g_cpu_prev_tick[id] = tick2;

#ifndef TASK_SWITCH_ARRAY
		PROCESS_INFO_LIST* first = (PROCESS_INFO_LIST * )GetTaskListId(id)->list.next;	
		PROCESS_INFO_LIST* list = first;
		do {
			PROCESS_INFO_LIST* tmplist = list;
			list = (PROCESS_INFO_LIST*)list->list.next;
			LPPROCESS_INFO p = &tmplist->proc;
			if (p->status == TASK_OVER || p->status == TASK_TERMINATE) {
				if (p->pid == p->tid) {
					__kFreeProcess(p);
				}
				RemoveNode((LIST_ENTRY*)&(gTasksListPtr[id]->list), (LIST_ENTRY*)&(tmplist->list));
			}
		} while (list != first);
#endif
		ret = __leaveSpinlock(&g_task_array_lock[id]);
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
	int ret = 0;
	char szout[256];
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

	//why not use process0 but lpproc?
	LPPROCESS_INFO process0 = (LPPROCESS_INFO)GetCurrentTaskTssBase();

	TASKRESULT freeTss;

#ifdef TASK_SWITCH_ARRAY
	LPPROCESS_INFO tssbase = SetTaskTssBase();
	InitTaskArray();
	ret = __GetFreeTask(&freeTss, id);

	char* fenv = GetFpuBuf(id,freeTss.number);
	InitTaskCoprocessor(process0, fenv);
#else
	InitTaskList();
	PROCESS_INFO_LIST* list = (PROCESS_INFO_LIST*)__kmalloc(sizeof(PROCESS_INFO_LIST));
	PROCESS_INFO * lptss = &list->proc;
	lptss->cpuid = id;
	lptss->status = TASK_SUSPEND;
	freeTss.lptss = lptss;
	freeTss.number = GenPid();

	char* fenv = list->fpenv;
	InitTaskCoprocessor(process0, fenv);
#endif
	
	int tid = freeTss.number;

	unsigned long stacktop = (unsigned long)(KERNEL_STACK_BASE + KTASK_STACK_SIZE * (id + 1) - STACK_TOP_DUMMY);

	LPPROCESS_INFO lpproc = (LPPROCESS_INFO)(freeTss.lptss);
	
	__strcpy(process0->filename, (char*)filename);
	__strcpy(process0->funcname, (char*)funcname);
	process0->status = TASK_RUN;
	process0->tid = tid;
	process0->pid = tid;
	process0->ppid = 0;
	process0->cpuid = id;
#ifdef TASK_SWITCH_ARRAY
	process0->vid = (id << 16) + tid;
#else
	process0->vid = tid;
#endif
	
	process0->espbase = stacktop;
	process0->level = 0;
	process0->vaddr = 0;
	process0->lpvasize = &lpproc->va_size;
	process0->va_size = 0;

	process0->showX = showx;
	process0->showY = showy;
	process0->window = 0;

	process0->slice = TASK_SLICE_KERNEL;
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
	process0->lpHeapBase = (char***)&lpproc->heapBase;

	process0->large_heap_size = 0;
	process0->fast_heap_large = 0;

	process0->priority = 0;
	process0->delta = DYNAMIC_PRIORITY;
	process0->authority = AUTHORITY_PRIORITY;

	process0->tick_run = 0;
	process0->prev_tick = 0;
	process0->tick_start = __krdtsc();
	process0->tick_total = 0;
	process0->tick_switch_cost = 0;

	DWORD size = HEAP_SIZE;
	char* buf = (char*)__kProcessMalloc(HEAP_SIZE, &size, 0, id, 0, PAGE_READWRITE | PAGE_USERPRIVILEGE | PAGE_PRESENT);

	process0->heapBase[0] = (char*)buf;
	process0->heapsize = HEAP_SIZE;
	process0->fast_heap = (char*)__kProcessMalloc(HEAP_SIZE, &size, 0, id, 0, PAGE_READWRITE | PAGE_USERPRIVILEGE | PAGE_PRESENT);	
	
	__memset((char*) process0->heapBase[0], 0, HEAP_SIZE);
	__memset(process0->fast_heap, 0, HEAP_SIZE);

	__memcpy((char*)lpproc, (char*)process0, sizeof(PROCESS_INFO));
	
	__printf(szout, "%s %d cpu:%d *lpHeapCnt:%d,*lpheap_lock:%d,*lpHeapBase:%x\r\n",__FUNCTION__,__LINE__,id,
		*process0->lpHeapCnt, *process0->lpheap_lock, process0->lpHeapBase);

#ifdef TASK_SWITCH_ARRAY

#else
	InsertTaskList(list);
#endif

	//__memset((char*)V86_TASKCONTROL_ADDRESS, 0, LIMIT_V86_PROC_COUNT*12);

	return tid;
}


//在V86模式下，CPL=3，执行特权指令时，或者要引起出错码为0的通用保护故障，或者要引起 非法操作码故障。
//由于CPL = 3， 所以如果IOPL < 3，那么执行CLI或STI指令将引起通用保护故障。
//输入 / 输出指令IN、INS、OUT或OUTS的 敏感条件仅仅是当前V86任务TSS内的I / O许可位图，而忽略EFLAGS中的IOPL。
//在V86模式下， 当IOPL < 3时，执行指令PUSHF、POPF、INT n及IRET会引起出错码为0的通用保护故障。
//采取上述措施的目的是使操作系统软件可以支持一个“虚拟EFLAGS”寄存器。



extern "C" void __declspec(naked) __TaskSchedule(LIGHT_ENVIRONMENT* stack) {

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
		sub esp, NATIVE_STACK_LIMIT

		mov eax, KERNEL_MODE_DATA
		mov ds, ax
		mov es, ax
		MOV FS, ax
		MOV GS, AX
		mov ss, ax
	}

	{
		//char szout[256];
		//__printf(szout,"%s\r\n",__FUNCTION__);

		__kTaskSchedule(stack);

		*(DWORD*)(LOCAL_APIC_BASE + 0xb0) = 0;
	}

	__asm {
		//call GetCurrentTaskTssBase
		//mov edx, eax
		//mov eax, dword ptr ds : [edx + PROCESS_INFO.tss.cr3]
		//mov cr3, eax

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

		iretd

		jmp __TaskSchedule
	}
}

extern "C" void __declspec(dllexport) yield( LIGHT_ENVIRONMENT * stack) {

	__kTaskSchedule(stack);
	
	__asm {
		mov eax, stack
		mov esp,eax
		mov ebp,eax

		//call GetCurrentTaskTssBase
		//mov edx, eax
		//mov eax, dword ptr ds : [edx + PROCESS_INFO.tss.cr3]
		//mov cr3, eax

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
#ifdef SINGLE_TASK_TSS
		mov esp, ss: [esp - 20]
#endif
		//clts

		iretd
	}
}

void tasktest(LIGHT_ENVIRONMENT* env, LPPROCESS_INFO current, LPPROCESS_INFO next, LPPROCESS_INFO prev) {
	static int gTestFlag = 0;
	if (gTestFlag >= 0 && gTestFlag <= -1)
	{
		char szout[256];

		DWORD cpureq;
		DWORD maxreq;
		DWORD busreq;
		__cpuFreq(&cpureq, &maxreq, &busreq);
		__int64 cpurate = cpureq;

		__printf(szout,
			"current link:%x,prev link:%x,next link:%x,stack eflags:%x,current eflags:%x,prev eflags:%x,next eflags:%x,new task pid:%d, tid:%d, old task pid:%d, tid:%d,cpurate:%i64x\r\n",
			prev->tss.link, current->tss.link, next->tss.link, env->eflags, prev->tss.eflags, current->tss.eflags, next->tss.eflags,
			current->pid, current->tid, next->pid, next->tid, cpurate);

		__printf(szout,
			"saved  cr3:%x,pid:%x,name:%s,level:%u,esp0:%x,ss0:%x,eip:%x,cs:%x,esp3:%x,ss3:%x,eflags:%x,link:%x,\r\n"
			"loaded cr3:%x,pid:%x,name:%s,level:%u,esp0:%x,ss0:%x,eip:%x,cs:%x,esp3:%x,ss3:%x,eflags:%x,link:%x.\r\n\r\n",
			prev->tss.cr3, prev->pid, prev->filename, prev->level,
			prev->tss.esp0, prev->tss.ss0, prev->tss.eip, prev->tss.cs,
			prev->tss.esp, prev->tss.ss, prev->tss.eflags, prev->tss.link,
			next->tss.cr3, next->pid, next->filename, next->level,
			next->tss.esp0, next->tss.ss0, next->tss.eip, next->tss.cs,
			next->tss.esp, next->tss.ss, next->tss.eflags, next->tss.link);

		gTestFlag++;
	}
}


