#include "timer8254.h"

#include "cmosExactTimer.h"
#include "utils.h"


TIMER_PROC_PARAM g8254Timer[REALTIMER_CALLBACK_MAX] = { 0 };


int getTimer8254Delay(){

	int n = OSCILLATE_FREQUENCY / SYSTEM_TIMER0_FACTOR;
	return 1000 / n;

}


void init8254Timer() {
	__memset((char*)g8254Timer, 0, REALTIMER_CALLBACK_MAX * sizeof(TIMER_PROC_PARAM));
}


int __kAdd8254Timer(DWORD addr, DWORD delay, DWORD param1, DWORD param2, DWORD param3, DWORD param4) {
	if (addr == 0 || delay == 0)
	{
		return -1;
	}

	//char szout[1024];
	//__printf(szout, "__kAddCmosTimer addr:%x,delay:%d,param1:%x,param2:%x,param3:%x,param4:%x\r\n", 
	// addr,delay,param1,param2,param3,param4);

	DWORD* lptickcnt = (DWORD*)TIMER0_TICK_COUNT;

	int dt = getTimer8254Delay();

	DWORD ticks = delay / dt;

	int i = 0;
	for (i = 0; i < REALTIMER_CALLBACK_MAX; i++)
	{
		if (g8254Timer[i].func == 0 && g8254Timer[i].tickcnt == 0)
		{
			g8254Timer[i].func = addr;
			g8254Timer[i].ticks = ticks;
			g8254Timer[i].tickcnt = *lptickcnt + ticks;
			g8254Timer[i].param1 = param1;
			g8254Timer[i].param2 = param2;
			g8254Timer[i].param3 = param3;
			g8254Timer[i].param4 = param4;
			break;
		}
	}

	return i;
}



void __kRemove8254Timer(int no) {
	if (no >= 0 && no < REALTIMER_CALLBACK_MAX)
	{
		g8254Timer[no].func = 0;
		g8254Timer[no].tickcnt = 0;
	}
}



void __k8254TimerProc() {

	int result = 0;
	DWORD* lptickcnt = (DWORD*)TIMER0_TICK_COUNT;

	//in both c and c++ language,the * priority is lower than ++
	(*lptickcnt)++;

	DWORD* pdoscounter = (DWORD*)DOS_SYSTIMER_ADDR;
	*pdoscounter = *lptickcnt;

	//heptEOI();

	for (int i = 0; i < REALTIMER_CALLBACK_MAX; i++)
	{
		if (g8254Timer[i].func)
		{
			if (g8254Timer[i].tickcnt < *lptickcnt)
			{

				g8254Timer[i].tickcnt = *lptickcnt + g8254Timer[i].ticks;

				typedef int(*ptrfunction)(DWORD param1, DWORD param2, DWORD param3, DWORD param4);
				ptrfunction lpfunction = (ptrfunction)g8254Timer[i].func;
				result = lpfunction(g8254Timer[i].param1, g8254Timer[i].param2, g8254Timer[i].param3, g8254Timer[i].param4);
			}
		}
	}
}



