
#include "cmosPeriodTimer.h"
#include "cmosAlarm.h"
#include "Utils.h"
#include "video.h"
#include "ListEntry.h"
#include "hardware.h"
#include "systemService.h"
#include "cmosExactTimer.h"
#include "apic.h"

TIMER_PROC_PARAM gExactTimer[REALTIMER_CALLBACK_MAX] = { 0 };


void initExactTimer() {
	*((DWORD*)CMOS_EXACT_TICK_COUNT) = 0;
	__memset((char*)gExactTimer, 0, REALTIMER_CALLBACK_MAX * sizeof(TIMER_PROC_PARAM));
}


int __kAddExactTimer(DWORD addr, DWORD delay, DWORD param1, DWORD param2, DWORD param3, DWORD param4) {
	if (addr == 0 || delay == 0)
	{
		return -1;
	}

	DWORD* lptickcnt = (DWORD*)CMOS_EXACT_TICK_COUNT;

	DWORD ticks = delay / CMOS_EXACT_INTERVAL;		
	if (delay % CMOS_EXACT_INTERVAL) {
		ticks++;
	}

	for (int i = 0; i < REALTIMER_CALLBACK_MAX; i++)
	{
		if (gExactTimer[i].func == 0 && gExactTimer[i].tickcnt == 0)
		{
			gExactTimer[i].func = addr;
			gExactTimer[i].ticks = ticks;
			gExactTimer[i].tickcnt = *lptickcnt + ticks;
			gExactTimer[i].param1 = param1;
			gExactTimer[i].param2 = param2;
			gExactTimer[i].param3 = param3;
			gExactTimer[i].param4 = param4;

			LPPROCESS_INFO proc = (LPPROCESS_INFO)GetCurrentTaskTssBase();
			gExactTimer[i].pid = proc->pid;
			gExactTimer[i].tid = proc->tid;

			/*
			char* pde = 0;
			__asm {
				mov eax,cr3
				mov [pde],eax
			}
			gExactTimer[i].cr3 = pde;
			*/

			char szout[256];
			__printf(szout, "%s addr:%x,num:%d,delay:%d,param1:%x,param2:%x,param3:%x,param4:%x\r\n",
				__FUNCTION__,
				addr,i, delay, param1, param2, param3, param4);
			return i;
		}
	}

	return -1;
}



void __kRemoveExactTimer(int n) {
	if (n >= 0 && n < REALTIMER_CALLBACK_MAX)
	{
		gExactTimer[n].func = 0;
		gExactTimer[n].tickcnt = 0;
	}
}



void __kExactTimerProc() {

	int result = 0;
	DWORD* lptickcnt = (DWORD*)CMOS_EXACT_TICK_COUNT;

	//in both c and c++ language,the * priority is lower than ++
	(*lptickcnt)++;

	for (int i = 0; i < REALTIMER_CALLBACK_MAX; i++)
	{
		if (gExactTimer[i].func)
		{
			if (gExactTimer[i].tickcnt <= *lptickcnt)
			{
				LPPROCESS_INFO proc = (LPPROCESS_INFO)GetCurrentTaskTssBase();
				if (gExactTimer[i].pid == proc->pid && gExactTimer[i].tid == proc->tid) {

					gExactTimer[i].tickcnt = *lptickcnt + gExactTimer[i].ticks;

					typedef int(*ptrfunction)(DWORD param1, DWORD param2, DWORD param3, DWORD param4);
					ptrfunction lpfunction = (ptrfunction)gExactTimer[i].func;
					result = lpfunction(gExactTimer[i].param1, gExactTimer[i].param2, gExactTimer[i].param3, gExactTimer[i].param4);
				}
			}
		}
	}
}