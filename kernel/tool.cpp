
#include "tool.h"
#include "def.h"
#include "keyboard.h"
#include "systemService.h"
#include "Utils.h"
#include "apic.h"









int WaitOrKey(int s,int wid,int key) {
	DWORD tick = *(DWORD*)CMOS_PERIOD_TICK_COUNT;
	DWORD dest = tick + s;

	while (tick < dest) {
		tick = *(DWORD*)CMOS_PERIOD_TICK_COUNT;

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