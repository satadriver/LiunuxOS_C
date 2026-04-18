#include "apictimer.h"
#include "process.h"
#include "cmosExactTimer.h"
#include "utils.h"
#include "apic.h"
#include "memory.h"
#include "apic.h"
#include "coprocessor.h"


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


#include "algorithm.h"
#include "systemService.h"


int g_task_switch_toggle = 0;


extern "C" __declspec(dllexport)int __k8254TimerProc() {
	return 0;

	if (g_task_switch_toggle == 0) {
		return 0;
	}
	char szout[256];

	int* ids = (int*)CPU_ID_ADDRESS;
	int counter = *(int*)(CPU_TOTAL_ADDRESS);
	AlgorithmModel times[TASK_LIMIT_TOTAL];
	unsigned long long tick = __krdtsc();
	for (int i = 0; i < counter; i++) {
		int id = ids[i];
		if (g_cpu_start_tick[id] == 0 || g_cpu_tick[id] == 0) {
			return 0;
		}
		double cpu_diff = tick - g_cpu_start_tick[id];
		double cpu_ratio = (double)g_cpu_tick[id] / cpu_diff;

		times[i].fv = cpu_ratio;
		times[i].id = id;
	}

	if (counter <= 1 || counter > TASK_LIMIT_TOTAL) {
		return 0;
	}

	BubbleSort_ull(times, counter);

	int src_id = (int)times[counter - 1].id;

	int dst_id = (int)times[0].id;
	double src_fv = times[counter - 1].fv;
	double dst_fv = times[0].fv;
	if (src_fv - dst_fv > 0.1) {

	}
	else {
		return 0;
	}

	LPPROCESS_INFO src_tss = GetTaskTssBaseId(src_id);

	enter_task_array_lock_id(src_id);

	double max = 0.0;
	int cnt = 0;
	int src_tid = -1;
	for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
		if (src_tss[i].status == TASK_RUN) {
			cnt++;
			double proc_diff = src_tss[i].tick_total;
			double proc_ratio = (double)src_tss[i].tick / proc_diff;
			if (proc_ratio > max) {
				max = proc_ratio;
				src_tid = src_tss[i].tid;
			}
		}
	}

	LPPROCESS_INFO src_current = GetCurrentTaskTssBaseId(src_id);

	int is_src_proc = 0;
	if (src_tss[src_tid].pid == src_tss[src_tid].tid) {
		is_src_proc = 1;
	}

	int is_src_cur = 0;
	if (src_current->tid == src_tid) {
		is_src_cur = 1;
	}

	if (cnt > 1 && src_tid != 0 && is_src_cur == 0) {
		
		LPPROCESS_INFO dst_tss = (LPPROCESS_INFO)GetTaskTssBaseId(dst_id);

		int tssSize = (sizeof(PROCESS_INFO) + 0xfff) & 0xfffff000;

		enter_task_array_lock_id(dst_id);

		for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
			if (dst_tss[i].status == TASK_OVER) {
				int dst_tid = i;

				__memcpy((char*)&dst_tss[i], (char*)&src_tss[src_tid], tssSize);
				dst_tss[i].cpuid = dst_id;

				char* src_fenv = (char*)g_fpu_status[src_id] + (src_tid << 9);

				char* dst_fenv = (char*)g_fpu_status[dst_id] + (dst_tid << 9);

				__memcpy(dst_fenv, src_fenv, 512);

				dst_tss[i].tid = dst_tid;

				if (is_src_proc) {
					dst_tss[i].pid = dst_tid;
					dst_tss[i].lpvasize = &dst_tss[i].va_size;

					dst_tss[i].lpHeapBase = &dst_tss[i].heapBase;

					dst_tss[i].lpHeapCnt = &dst_tss[i].heapCnt;

					dst_tss[i].lpheap_lock = &dst_tss[i].heap_lock;
				}

				src_tss[src_tid].status = TASK_OVER;

				__printf(szout, "%s copy cpu:%d tid:%d to cpu:%d tid:%d,is_src_proc:%d,is_src_cur:%d\r\n",
					__FUNCTION__, src_id, src_tid, dst_id, dst_tid, is_src_proc, is_src_cur);
				break;
			}
		}

		leave_task_array_lock_id(dst_id);
	}

	leave_task_array_lock_id(src_id);

	return 0;
}