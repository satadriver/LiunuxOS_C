#include "apictimer.h"
#include "process.h"
#include "cmosExactTimer.h"
#include "utils.h"
#include "apic.h"
#include "memory.h"
#include "apic.h"

//TIMER_PROC_PARAM g8254Timer[REALTIMER_CALLBACK_MAX] = { 0 };
TIMER_PROC_PARAM * gApicTimer = 0;


int getTimer8254Delay(){
	int n = OSCILLATE_FREQUENCY / SYSTEM_TIMER0_FACTOR;
	return 1000 / n;
}

int getApicTimerDelay() {
	unsigned long long n = ApicTimerFreq();
	return n/(1000/ TASK_TIME_SLICE);
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
			__printf(szout, "%s addr:%x,num:%d,delay:%d,param1:%x,param2:%x,param3:%x,param4:%x\r\n", __FUNCTION__,
				func,i,delay,param1,param2,param3,param4);

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



extern "C" __declspec(dllexport)int __k8254TimerProc() {
	return 0;
}