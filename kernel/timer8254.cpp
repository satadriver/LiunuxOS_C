#include "timer8254.h"
#include "process.h"
#include "cmosExactTimer.h"
#include "utils.h"
#include "apic.h"
#include "memory.h"

//TIMER_PROC_PARAM g8254Timer[REALTIMER_CALLBACK_MAX] = { 0 };
TIMER_PROC_PARAM * g8254Timer = 0;


int getTimer8254Delay(){

	int n = OSCILLATE_FREQUENCY / SYSTEM_TIMER0_FACTOR;
	return 1000 / n;

}


void init8254Timer() {
	for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
		*(int*)(TIMER0_TICK_COUNT + i * sizeof(int)) = 0;
	}
	
	//__memset((char*)g8254Timer, 0, REALTIMER_CALLBACK_MAX * sizeof(TIMER_PROC_PARAM));
	g8254Timer =(TIMER_PROC_PARAM*) __kMalloc(REALTIMER_CALLBACK_MAX * sizeof(TIMER_PROC_PARAM) * TASK_LIMIT_TOTAL);
}


int __kAdd8254Timer(DWORD func, DWORD delay, DWORD param1, DWORD param2, DWORD param3, DWORD param4) {

	//unsigned long func = linear2phy((unsigned long)addr);
	//if (func == 0 || delay == 0)
	//{
	//	return -1;
	//}
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	DWORD* lptickcnt = (DWORD*)(TIMER0_TICK_COUNT+id*sizeof(int));

	int dt = getTimer8254Delay();

	DWORD ticks = delay / dt;
	if (delay % dt) {
		ticks++;
	}

	for (int i = id* REALTIMER_CALLBACK_MAX; i < (id +1)* REALTIMER_CALLBACK_MAX; i++)
	{
		if (g8254Timer[i].func == 0 && g8254Timer[i].tickcnt == 0)
		{
			g8254Timer[i].func = func;
			g8254Timer[i].ticks = ticks;
			g8254Timer[i].tickcnt = *lptickcnt + ticks;
			g8254Timer[i].param1 = param1;
			g8254Timer[i].param2 = param2;
			g8254Timer[i].param3 = param3;
			g8254Timer[i].param4 = param4;
			LPPROCESS_INFO proc = (LPPROCESS_INFO)GetCurrentTaskTssBase();
			g8254Timer[i].pid = proc->pid;
			g8254Timer[i].tid = proc->tid;
			char szout[256];
			__printf(szout, "%s addr:%x,num:%d,delay:%d,param1:%x,param2:%x,param3:%x,param4:%x\r\n", __FUNCTION__,
				func,i,delay,param1,param2,param3,param4);

			return i;
		}
	}

	return 0;
}



void __kRemove8254Timer(int  n) {
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	if (n >= id* REALTIMER_CALLBACK_MAX && n < (id+1)* REALTIMER_CALLBACK_MAX)
	{
		g8254Timer[n].func = 0;
		g8254Timer[n].tickcnt = 0;
	}
}



void __k8254TimerProc() {

	int result = 0;
	//in both c and c++ language,the * priority is lower than ++
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	DWORD* lptickcnt = (DWORD*)(TIMER0_TICK_COUNT + id*sizeof(int));

	(*lptickcnt)++;

	//DWORD* pdoscounter = (DWORD*)DOS_SYSTIMER_ADDR;
	//*pdoscounter = *lptickcnt;

	for (int i = id * REALTIMER_CALLBACK_MAX; i < (id + 1) * REALTIMER_CALLBACK_MAX; i++)
	{
		if (g8254Timer[i].func)
		{
			if (g8254Timer[i].tickcnt < *lptickcnt)
			{
				LPPROCESS_INFO proc = (LPPROCESS_INFO)GetCurrentTaskTssBase();
				if (g8254Timer[i].pid == proc->pid && g8254Timer[i].tid == proc->tid) {
					g8254Timer[i].tickcnt = *lptickcnt + g8254Timer[i].ticks;

					typedef int(*ptrfunction)(DWORD param1, DWORD param2, DWORD param3, DWORD param4);
					ptrfunction lpfunction = (ptrfunction)g8254Timer[i].func;
					result = lpfunction(g8254Timer[i].param1, g8254Timer[i].param2, g8254Timer[i].param3, g8254Timer[i].param4);
				}
			}
		}
	}
}



