#include "cmosAlarm.h"
#include "hardware.h"
#include "def.h"
#include "Utils.h"
#include "video.h"
#include "window.h"
#include "keyboard.h"
#include "mouse.h"
#include "task.h"
#include "Pe.h"
#include "malloc.h"
#include "process.h"
#include "Thread.h"
#include "memory.h"
#include "cmosPeriodTimer.h"

CMOSALARM_PROCESS_LIST gCmosAlarmProc;


int isLeapYear(int year) {
	int ret = year % 100;
	if (ret == 0)
	{
		return FALSE;
	}

	ret = year % 4;
	if (ret)
	{
		return FALSE;
	}
	return TRUE;
}

int getDayOfMonth(int year, int month) {
	if (month == 2)
	{
		if (isLeapYear(year)) {
			return 29;
		}
		else {
			return 28;
		}
	}

	if (month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12)
	{
		return 31;
	}
	else {
		return 30;
	}
}



unsigned char bcd2b(unsigned char bcd) {
	unsigned char low = bcd & 0xf;
	unsigned char h = (bcd & 0xf0) >> 4;
	return h * 10 + low;
}

unsigned char b2bcd(unsigned char b) {
	int h = b / 10;
	int l = b % 10;
	return (h << 4) + l;
}

unsigned short makehalf(unsigned char low, unsigned char high) {
	return (high << 8) + low;
}




void addAlarmTimer(DWORD interval) {
	
	int ret = 0;

	unsigned char bcentury = readCmosPort(0x32);
	unsigned char byear = readCmosPort(9);
	unsigned int cy = ((unsigned int)bcd2b(bcentury) * 100) + (unsigned int)bcd2b(byear);

	unsigned char bmonth = readCmosPort(8);
	unsigned char month = bcd2b(bmonth);

	unsigned char bday = readCmosPort(7);
	unsigned char day = bcd2b(bday);

	unsigned char bhour = readCmosPort(4);
	unsigned char hour = bcd2b(bhour);

	unsigned char bminute = readCmosPort(2);
	unsigned char minute = bcd2b(bminute);

	unsigned char bsecond = readCmosPort(0);
	unsigned char second = bcd2b(bsecond);

	unsigned int dstyear = cy;
	unsigned int dstmonth = month;
	unsigned int dstday = day;
	unsigned int dsthour = hour;
	unsigned int dstmin = minute;
	unsigned int dstsecond = second;

	int carray = 0;

	dstsecond = second + interval;
	if (dstsecond >= 60)
	{
		dstsecond = dstsecond % 60;

		carray = dstsecond / 60 + 1;

		dstmin = minute + carray;

		if (dstmin >= 60)
		{
			carray = dstmin / 60+1;

			dstmin = dstmin % 60;

			dsthour = dsthour + carray;
			if (dsthour >= 24)
			{
				carray = dsthour / 24 + 1;

				dsthour = dsthour % 24;

				dstday = dstday + carray;

				int daysinmonth = getDayOfMonth(cy, month);
				if (dstday >= daysinmonth)
				{
					carray = dstday / daysinmonth + 1;
					dstmonth = dstmonth + carray;
					dstday = dstday % daysinmonth;

					if (dstmonth >= 12)
					{
						carray = dstmonth / 12 + 1;
						dstyear = dstyear + carray;
						dstmonth = dstmonth % 12;
					}
				}
			}
		}
	}

	writeCmosPort(0x0d, b2bcd(dstday));		//check if invalid?
	writeCmosPort(0x05, b2bcd(dsthour));
	writeCmosPort(0x03, b2bcd(dstmin));
	writeCmosPort(0x01, b2bcd(dstsecond));

	char szout[1024];
	__printf(szout, "set alarm at:%d/%d/%d %d:%d:%d\n", dstyear, dstmonth, dstday, dsthour, dstmin, dstsecond);

}





//from assembly code
void __kAlarmTimerProc() {

	char szout[1024];
	//__printf(szout, "__kCmosAlarmProc entry from assemble code\n");

	//can only exist one alarm
	DWORD addr = gCmosAlarmProc.addr;
	DWORD interval = gCmosAlarmProc.interval;
	DWORD param = gCmosAlarmProc.param;
	if (addr && interval)
	{
		__asm {
			mov eax, param
			push eax
			mov eax, addr
			call eax
			add esp, 4
		}
		gCmosAlarmProc.addr = 0;
		gCmosAlarmProc.interval = 0;

	}
}

int __kAddAlarmTimer( DWORD interval, DWORD linearaddr, DWORD param) {

	if (gCmosAlarmProc.addr == 0 && gCmosAlarmProc.interval == 0)
	{
		gCmosAlarmProc.addr = linearaddr;
		gCmosAlarmProc.interval = interval;
		gCmosAlarmProc.param = param;

		addAlarmTimer(interval);
		return TRUE;
	}

	return 0;
}

void __kRemoveAlarmTimer() {

	gCmosAlarmProc.addr = 0;
	gCmosAlarmProc.interval = 0;
	gCmosAlarmProc.param = 0;

}



//void __doAlarmTask(DWORD retaddr,DWORD pid,char * filename,char * funcname,DWORD param) {
void __doAlarmTask(DWORD  param) {

	char szout[1024];
	__printf(szout, "__doAlarmTask running\n");


}


