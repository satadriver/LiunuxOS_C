
#include "cmosPeriodTimer.h"
#include "cmosAlarm.h"
#include "Utils.h"
#include "video.h"
#include "ListEntry.h"
#include "hardware.h"
#include "servicesProc.h"
#include "cmosExactTimer.h"

TIMER_PROC_PARAM gExactTimer[REALTIMER_CALLBACK_MAX] = { 0 };


void initExactTimer() {
	__memset((char*)gExactTimer, 0, REALTIMER_CALLBACK_MAX * sizeof(TIMER_PROC_PARAM));
}


int __kAddExactTimer(DWORD addr, DWORD delay, DWORD param1, DWORD param2, DWORD param3, DWORD param4) {
	if (addr == 0 || delay == 0)
	{
		return -1;
	}

	//char szout[1024];
	//__printf(szout, "__kAddCmosTimer addr:%x,delay:%d,param1:%x,param2:%x,param3:%x,param4:%x\r\n", 
	// addr,delay,param1,param2,param3,param4);

	DWORD* lptickcnt = (DWORD*)CMOS_EXACT_TICK_COUNT;

	DWORD ticks = delay / CMOS_EXACT_INTERVAL;		

	int i = 0;
	for (i = 0; i < REALTIMER_CALLBACK_MAX; i++)
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
			break;
		}
	}

	return i;
}



void __kRemoveExactTimer(int no) {
	if (no >= 0 && no < REALTIMER_CALLBACK_MAX)
	{
		gExactTimer[no].func = 0;
		gExactTimer[no].tickcnt = 0;
	}
}



void __kExactTimerProc() {

	int result = 0;
	DWORD* lptickcnt = (DWORD*)CMOS_EXACT_TICK_COUNT;
	DWORD tickcnt = *lptickcnt;
	//in both c and c++ language,the * priority is lower than ++
	(*lptickcnt)++;

	for (int i = 0; i < REALTIMER_CALLBACK_MAX; i++)
	{
		if (gExactTimer[i].func)
		{
			if (gExactTimer[i].tickcnt <= *lptickcnt)
			{

				gExactTimer[i].tickcnt = *lptickcnt + gExactTimer[i].ticks;

				typedef int(*ptrfunction)(DWORD param1, DWORD param2, DWORD param3, DWORD param4);
				ptrfunction lpfunction = (ptrfunction)gExactTimer[i].func;
				result = lpfunction(gExactTimer[i].param1, gExactTimer[i].param2, gExactTimer[i].param3, gExactTimer[i].param4);
			}
		}
	}
}