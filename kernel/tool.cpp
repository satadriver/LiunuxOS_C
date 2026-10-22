
#include "tool.h"
#include "def.h"
#include "keyboard.h"
#include "systemService.h"
#include "Utils.h"
#include "apic.h"
#include "cmosPeriodTimer.h"









int WaitOrKey(int s,int wid,int key) {
	DWORD tick = g_cmos_period_tick_cnt;
	DWORD dest = tick + s;

	while (tick < dest) {
		tick = g_cmos_period_tick_cnt;

		if (key) {
			int ck = __kGetKbd(wid) & 0xff;
			if (ck == key)
				break;
		}
		else {

		}

		__sleep(0);
	}

	return 0;
}