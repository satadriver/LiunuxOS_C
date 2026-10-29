#include "apictimer.h"
#include "process.h"
#include "cmosExactTimer.h"
#include "utils.h"
#include "apic.h"
#include "memory.h"
#include "coprocessor.h"
#include "task.h"
#include "algorithm.h"
#include "systemService.h"


int g_apictimer_lock[256];

TIMER_PROC_PARAM * gApicTimer[256] ;

unsigned long g_timer_tick_cnt;

unsigned long g_apictimer_tick_cnt[256] ;



int getTimer8254Delay(){
	int n = TASK_TIME_SLICE;
	return 1000 / n;
}

int getApicTimerDelay() {
	unsigned long long n = ApicTimerFreq();
	return (int) n/(1000/ TASK_TIME_SLICE);
}


void initApicTimer() {
	for (int i = 0; i < 256; i++) {
		//g_apictimer_tick_cnt [i ] = 0;
	}
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

	gApicTimer[id] = (TIMER_PROC_PARAM*)__kMalloc(REALTIMER_CALLBACK_MAX * sizeof(TIMER_PROC_PARAM) );
}


int __kAddApicTimer(DWORD func, DWORD delay, DWORD param1, DWORD param2, DWORD param3, DWORD param4) {

	//unsigned long func = linear2phy((unsigned long)addr);
	//if (func == 0 || delay == 0)
	//{
	//	return -1;
	//}
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	DWORD* lptickcnt = (DWORD*)&(g_apictimer_tick_cnt [id]);

	int dt = getTimer8254Delay();

	DWORD ticks = delay / dt;
	if (delay % dt) {
		ticks++;
	}

	int seq = -1;

	__enterSpinlock(&g_apictimer_lock[id]);

	for (int i = 0; i <  REALTIMER_CALLBACK_MAX; i++)
	{
		if (gApicTimer[id][i].func == 0 && gApicTimer[id][i].tickcnt == 0)
		{
			gApicTimer[id][i].func = func;
			gApicTimer[id][i].ticks = ticks;
			gApicTimer[id][i].tickcnt = *lptickcnt + ticks;
			gApicTimer[id][i].param1 = param1;
			gApicTimer[id][i].param2 = param2;
			gApicTimer[id][i].param3 = param3;
			gApicTimer[id][i].param4 = param4;
			LPPROCESS_INFO proc = (LPPROCESS_INFO)GetCurrentTaskTssBase();
			gApicTimer[id][i].pid = proc->pid;
			gApicTimer[id][i].tid = proc->tid;
			char szout[256];
			//__printf(szout, "%s addr:%x,num:%d,delay:%d,param1:%x,param2:%x,param3:%x,param4:%x\r\n", __FUNCTION__,func,i,delay,param1,param2,param3,param4);

			seq = i;
			break;
		}
	}
	__leaveSpinlock(&g_apictimer_lock[id]);

	return seq;
}



void __kRemoveApicTimer(int  n) {

	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

	__enterSpinlock(&g_apictimer_lock[id]);
	
	if (n >= 0 && n < REALTIMER_CALLBACK_MAX)
	{
		gApicTimer[id][n].func = 0;
		gApicTimer[id][n].tickcnt = 0;
	}
	__leaveSpinlock(&g_apictimer_lock[id]);
}



void __kApicTimerProc() {

	int result = 0;
	//in both c and c++ language,the * priority is lower than ++
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	DWORD* lptickcnt = (DWORD*)&(g_apictimer_tick_cnt[ id]);

	(*lptickcnt)++;

	//DWORD* pdoscounter = (DWORD*)DOS_SYSTIMER_ADDR;
	//*pdoscounter = *lptickcnt;

	for (int i = 0; i < REALTIMER_CALLBACK_MAX; i++)
	{
		if (gApicTimer[id][i].func)
		{
			if (gApicTimer[id][i].tickcnt < *lptickcnt)
			{
				LPPROCESS_INFO proc = (LPPROCESS_INFO)GetCurrentTaskTssBase();
				if (gApicTimer[i][id].pid == proc->pid && gApicTimer[i][id].tid == proc->tid) {
					gApicTimer[i][id].tickcnt = *lptickcnt + gApicTimer[id][i].ticks;

					typedef int(*ptrfunction)(DWORD param1, DWORD param2, DWORD param3, DWORD param4);
					ptrfunction lpfunction = (ptrfunction)gApicTimer[id][i].func;
					result = lpfunction(gApicTimer[id][i].param1, gApicTimer[id][i].param2,
						gApicTimer[id][i].param3, gApicTimer[id][i].param4);
				}
			}
		}
	}
}







#define INTER_CPU_RATE_MAX		0.1


extern "C" __declspec(dllexport)int __k8254TimerProc() {
	return 0;
}




extern "C" __declspec(dllexport)int SwitchTaskCPU() {

	char szout[256];

	int res = 0;

	AlgorithmModel ratio_buf[256];

	int* ids = (int*)g_cpu_id;
	int counter = g_cpu_total;
	unsigned long long tick = __krdtsc();
	for (int i = 0; i < counter; i++) {
		int cpuid = ids[i];
		if (g_cpu_start_tick[cpuid] == 0 || g_cpu_tick[cpuid] == 0) {
			//__printf(szout, "%s %d cpu:%d g_cpu_start_tick or g_cpu_tick null\r\n", __FUNCTION__, __LINE__, cpuid);
			return 0;
		}
		else {
			double cpu_diff = tick - g_cpu_start_tick[cpuid];
			double cpu_ratio = (double)g_cpu_tick[cpuid] / cpu_diff;
			ratio_buf[i].fv = cpu_ratio;
			ratio_buf[i].id = cpuid;
		}
	}

	if (counter <= 1 || counter > MAX_CPU_COUNT) {
		return 0;
	}

	BubbleSortd(ratio_buf, counter);

	int src_id = (int)ratio_buf[counter - 1].id;
	int dst_id = (int)ratio_buf[0].id;
	double src_fv = ratio_buf[counter - 1].fv;
	double dst_fv = ratio_buf[0].fv;
	if (src_fv - dst_fv < INTER_CPU_RATE_MAX) {
		return 0;
	}
	
	res = __GetSpinlock(&g_task_array_lock[src_id]);
	if (res == 0) {
		return 0;
	}
	LPPROCESS_INFO src_current = GetCurrentTaskTssBaseId(src_id);
	double max = 0.0;
	
	int pid = -1;
	int target_cnt = 0;
	int active_cnt = 0;
	double cpu_diff = tick - g_cpu_start_tick[src_id];
#ifdef TASK_SWITCH_ARRAY
	int src_tid = -1;
	LPPROCESS_INFO src_tss = GetTaskTssBaseId(src_id);	
	for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
		if (src_tss[i].status == TASK_RUN) {	
			active_cnt++;
			if (src_tss[i].pid == pid)
			{
				continue;
			}
				
			double proc_ratio = (double)src_tss[i].tick_run / cpu_diff;
			if ( proc_ratio > max && (src_tss[i].tid == src_tss[i].pid) ) {
				if ( (src_current->tid == src_tss[i].tid) ){

				}
				else {
					max = proc_ratio;
					src_tid = src_tss[i].tid;
					pid = src_tss[i].pid;
					target_cnt++;
				}
			}
		}
	}

	if (target_cnt > 1 && active_cnt > 1) {	
		res = __GetSpinlock(&g_task_array_lock[dst_id]);
		if (res) {
			LPPROCESS_INFO dst_tss = (LPPROCESS_INFO)GetTaskTssBaseId(dst_id);
			for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
				if (dst_tss[i].status == TASK_OVER) {
					int dst_tid = i;

					int tssSize = (sizeof(PROCESS_INFO) + 0xfff) & 0xfffff000;
					__memcpy((char*)&dst_tss[i], (char*)&src_tss[src_tid], tssSize);
					dst_tss[i].cpuid = dst_id;

					char* src_fenv = (char*)g_fpu_status[src_id] + (src_tid << 9);
					char* dst_fenv = (char*)g_fpu_status[dst_id] + (dst_tid << 9);
					__memcpy(dst_fenv, src_fenv, 512);

					dst_tss[i].tid = dst_tid;
					dst_tss[i].pid = dst_tid;

					dst_tss[i].lpHeapCnt = &dst_tss[i].heapCnt;
					dst_tss[i].lpheap_lock = &dst_tss[i].heap_lock;
					dst_tss[i].lpHeapBase = (char***)&dst_tss[i].heapBase;
					dst_tss[i].lpvasize = &dst_tss[i].va_size;

					src_tss[src_tid].status = TASK_OVER;

					__printf(szout, "%s copy cpu:%d tid:%d to cpu:%d tid:%d\r\n", __FUNCTION__, src_id, src_tid, dst_id, dst_tid);
					break;
				}
			}
			leave_task_array_lock_id(dst_id);
		}	
	}

#else
	PROCESS_INFO_LIST* targetlist = 0;
	PROCESS_INFO_LIST* list = (PROCESS_INFO_LIST*)GetTaskListId(src_id);
	PROCESS_INFO_LIST* src_list = (PROCESS_INFO_LIST*)list->list.next;
	PROCESS_INFO_LIST* ptr = src_list;
	do {
		if (ptr == 0)
			break;
		LPPROCESS_INFO p = &ptr->proc;
		if (p->status == TASK_RUN) {
			active_cnt++;
			if (p->pid == pid)
			{
				continue;
			}

			double proc_ratio = (double)p->tick_run / cpu_diff;
			if (proc_ratio > max && (p->tid == p->pid)) {
				if (src_current->tid == p->tid) {

				}
				else {
					max = proc_ratio;
					targetlist = ptr;
					pid = p->pid;
					target_cnt++;
				}
			}
		}
		ptr = (PROCESS_INFO_LIST*)ptr->list.next;
	} while (ptr && ptr != src_list);

	if (target_cnt > 1 && active_cnt > 1) {
		
		res = __GetSpinlock(&g_task_array_lock[dst_id]);
		if (res) {
			PROCESS_INFO_LIST* newlist = (PROCESS_INFO_LIST*)__kMalloc(sizeof(PROCESS_INFO_LIST));
			PROCESS_INFO* newproc = &newlist->proc;

			__memcpy((char*)newproc, (char*)&targetlist->proc, sizeof(PROCESS_INFO) );
			newproc->cpuid = dst_id;

			newproc->lpHeapCnt = &newproc->heapCnt;
			newproc->lpheap_lock = &newproc->heap_lock;
			newproc->lpHeapBase = (char***)&newproc->heapBase;
			newproc->lpvasize = &newproc->va_size;

			newproc->status = TASK_RUN;

			PROCESS_INFO_LIST* root = (PROCESS_INFO_LIST*)gTasksListPtr[dst_id];

			InsertListTail((LIST_ENTRY*)&(root->list), (LIST_ENTRY*)&(newlist->list));

			RemoveNode(&(list->list), &(targetlist->list));
			
			__printf(szout, "%s copy cpu:%d pid:%d to cpu:%d\r\n", __FUNCTION__, src_id, pid, dst_id);
		}
		leave_task_array_lock_id(dst_id);	
	}
#endif

	leave_task_array_lock_id(src_id);

	return 0;
}