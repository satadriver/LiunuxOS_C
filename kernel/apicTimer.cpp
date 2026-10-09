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

//TIMER_PROC_PARAM g8254Timer[REALTIMER_CALLBACK_MAX] = { 0 };
TIMER_PROC_PARAM * gApicTimer = 0;


int getTimer8254Delay(){
	int n = OSCILLATE_FREQUENCY / SYSTEM_TIMER0_FACTOR;
	return 1000 / n;
}

int getApicTimerDelay() {
	unsigned long long n = ApicTimerFreq();
	return (int) n/(1000/ TASK_TIME_SLICE);
}


void initApicTimer() {
	for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
		*(int*)(APICTIMER_TICK_COUNT + i * sizeof(int)) = 0;
	}
	
	//__memset((char*)g8254Timer, 0, REALTIMER_CALLBACK_MAX * sizeof(TIMER_PROC_PARAM));
	gApicTimer =(TIMER_PROC_PARAM*) __kMalloc(REALTIMER_CALLBACK_MAX * sizeof(TIMER_PROC_PARAM) * TASK_LIMIT_TOTAL);
}


int __kAddApicTimer(DWORD func, DWORD delay, DWORD param1, DWORD param2, DWORD param3, DWORD param4) {

	//unsigned long func = linear2phy((unsigned long)addr);
	//if (func == 0 || delay == 0)
	//{
	//	return -1;
	//}
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	DWORD* lptickcnt = (DWORD*)(APICTIMER_TICK_COUNT +id*sizeof(int));

	int dt = getTimer8254Delay();

	DWORD ticks = delay / dt;
	if (delay % dt) {
		ticks++;
	}

	for (int i = id* REALTIMER_CALLBACK_MAX; i < (id +1)* REALTIMER_CALLBACK_MAX; i++)
	{
		if (gApicTimer[i].func == 0 && gApicTimer[i].tickcnt == 0)
		{
			gApicTimer[i].func = func;
			gApicTimer[i].ticks = ticks;
			gApicTimer[i].tickcnt = *lptickcnt + ticks;
			gApicTimer[i].param1 = param1;
			gApicTimer[i].param2 = param2;
			gApicTimer[i].param3 = param3;
			gApicTimer[i].param4 = param4;
			LPPROCESS_INFO proc = (LPPROCESS_INFO)GetCurrentTaskTssBase();
			gApicTimer[i].pid = proc->pid;
			gApicTimer[i].tid = proc->tid;
			char szout[256];
			//__printf(szout, "%s addr:%x,num:%d,delay:%d,param1:%x,param2:%x,param3:%x,param4:%x\r\n", __FUNCTION__,func,i,delay,param1,param2,param3,param4);

			return i;
		}
	}

	return 0;
}



void __kRemoveApicTimer(int  n) {
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	if (n >= id* REALTIMER_CALLBACK_MAX && n < (id+1)* REALTIMER_CALLBACK_MAX)
	{
		gApicTimer[n].func = 0;
		gApicTimer[n].tickcnt = 0;
	}
}



void __kApicTimerProc() {

	int result = 0;
	//in both c and c++ language,the * priority is lower than ++
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	DWORD* lptickcnt = (DWORD*)(APICTIMER_TICK_COUNT + id*sizeof(int));

	(*lptickcnt)++;

	//DWORD* pdoscounter = (DWORD*)DOS_SYSTIMER_ADDR;
	//*pdoscounter = *lptickcnt;

	for (int i = id * REALTIMER_CALLBACK_MAX; i < (id + 1) * REALTIMER_CALLBACK_MAX; i++)
	{
		if (gApicTimer[i].func)
		{
			if (gApicTimer[i].tickcnt < *lptickcnt)
			{
				LPPROCESS_INFO proc = (LPPROCESS_INFO)GetCurrentTaskTssBase();
				if (gApicTimer[i].pid == proc->pid && gApicTimer[i].tid == proc->tid) {
					gApicTimer[i].tickcnt = *lptickcnt + gApicTimer[i].ticks;

					typedef int(*ptrfunction)(DWORD param1, DWORD param2, DWORD param3, DWORD param4);
					ptrfunction lpfunction = (ptrfunction)gApicTimer[i].func;
					result = lpfunction(gApicTimer[i].param1, gApicTimer[i].param2, gApicTimer[i].param3, gApicTimer[i].param4);
				}
			}
		}
	}
}





AlgorithmModel  * g_ratio_buf = 0;

#define INTER_CPU_RATE_MAX		0.1


extern "C" __declspec(dllexport)int __k8254TimerProc() {
	return 0;
}




extern "C" __declspec(dllexport)int SwitchTaskCPU(int lock) {

	char szout[256];

	int res = 0;

	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

	
	if (g_ratio_buf == 0) {
		int allocSize = sizeof(AlgorithmModel) * 256;
		unsigned long alignSize = 0;
		g_ratio_buf = (AlgorithmModel*)__kProcessMalloc(allocSize, &alignSize, 0, id, 0,
			PAGE_READWRITE | PAGE_USERPRIVILEGE | PAGE_PRESENT | 0x80000000);
	}

	//AlgorithmModel   g_ratio_buf[256];

	int* ids = (int*)CPU_ID_ADDRESS;
	int counter = *(int*)(CPU_TOTAL_ADDRESS);
	
	unsigned long long tick = __krdtsc();
	for (int i = 0; i < counter; i++) {
		int cpuid = ids[i];
		if (g_cpu_start_tick[cpuid] == 0 || g_cpu_tick[cpuid] == 0) {
			//__printf(szout, "%s %d cpu:%d g_cpu_start_tick or g_cpu_tick null\r\n", __FUNCTION__, __LINE__, cpuid);
		}
		else {
			double cpu_diff = tick - g_cpu_start_tick[cpuid];
			double cpu_ratio = (double)g_cpu_tick[cpuid] / cpu_diff;
			g_ratio_buf[i].fv = cpu_ratio;
			g_ratio_buf[i].id = cpuid;
		}
	}

	if (counter <= 1 || counter > TASK_LIMIT_TOTAL) {
		return 0;
	}

	BubbleSortd(g_ratio_buf, counter);

	int src_id = (int)g_ratio_buf[counter - 1].id;
	int dst_id = (int)g_ratio_buf[0].id;
	double src_fv = g_ratio_buf[counter - 1].fv;
	double dst_fv = g_ratio_buf[0].fv;
	if (src_fv - dst_fv >= INTER_CPU_RATE_MAX) {

	}
	else {
		return 0;
	}

	LPPROCESS_INFO src_tss = GetTaskTssBaseId(src_id);
	LPPROCESS_INFO src_current = GetCurrentTaskTssBaseId(src_id);
	extern int g_task_array_lock[256];
	if (id == src_id && lock)
	{
		res = 1;
	}
	else {
		res = __GetSpinlock(&g_task_array_lock[src_id]);
	}
	
	if (res == 0) {
		return 0;
	}
	int is_src_cur = 0;
	int is_src_proc = 0;

	double max = 0.0;
	int cnt = 0;
	int src_tid = -1;
	for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
		if (src_tss[i].status == TASK_RUN) {	
			cnt++;
			double cpu_diff = tick - g_cpu_start_tick[src_id];
			double proc_ratio = (double)src_tss[i].tick_run / cpu_diff;
			if (proc_ratio > max) {
				if ( (src_current->tid == src_tss[i].tid) || (src_tss[i].tid == src_tss[i].pid)){

				}
				else {
					max = proc_ratio;
					src_tid = src_tss[i].tid;
				}
			}
		}
	}
	
	if (src_tss[src_tid].pid == src_tss[src_tid].tid) {
		is_src_proc = 1;
	}
	
	if (src_current->tid == src_tid) {
		is_src_cur = 1;
	}

	LPPROCESS_INFO dst_tss = (LPPROCESS_INFO)GetTaskTssBaseId(dst_id);
	if (cnt > 1 && src_tid != -1 ) {
		if (dst_id == id && lock) {
			res = 1;
		}
		else {
			res = __GetSpinlock(&g_task_array_lock[dst_id]);
		}
		
		if (res) {
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

					if (is_src_proc) {
						dst_tss[i].pid = dst_tid;
					}

					dst_tss[i].lpHeapCnt = &dst_tss[i].heapCnt;
					dst_tss[i].lpheap_lock = &dst_tss[i].heap_lock;
					dst_tss[i].lpHeapBase =(char***) &dst_tss[i].heapBase;
					dst_tss[i].lpvasize = &dst_tss[i].va_size;

					if (is_src_cur)
					{
						src_current->status = TASK_OVER;
					}
					src_tss[src_tid].status = TASK_OVER;

					__printf(szout, "%s copy cpu:%d tid:%d to cpu:%d tid:%d,is_src_proc:%d,is_src_cur:%d\r\n",
						__FUNCTION__, src_id, src_tid, dst_id, dst_tid, is_src_proc, is_src_cur);
					break;
				}
			}
			leave_task_array_lock_id(dst_id);
		}	
	}

	leave_task_array_lock_id(src_id);

	return 0;
}