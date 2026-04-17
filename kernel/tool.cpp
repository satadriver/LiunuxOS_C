
#include "tool.h"
#include "def.h"
#include "keyboard.h"
#include "systemService.h"
#include "Utils.h"
#include "apic.h"





int IpiSwitchTask(int src_tid)
{
	int ret = 0;
	int dst_id = GetIdleProcessor();

	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	if (dst_id == id) {
		
	}

	enter_task_array_lock();

	enter_task_array_lock_id(dst_id);

	LPPROCESS_INFO src_tss = (LPPROCESS_INFO)GetTaskTssBase();

	LPPROCESS_INFO dst_tss = (LPPROCESS_INFO)GetTaskTssBaseId(dst_id);

	int tssSize = (sizeof(PROCESS_INFO) + 0xfff) & 0xfffff000;

	for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
		if (dst_tss[i].status == TASK_OVER) {
			__memcpy((char*)&dst_tss[i], (char*)&src_tss[src_tid], tssSize);
			break;
		}
	}

	leave_task_array_lock_id(dst_id);

	leave_task_array_lock();

	return 0;
}



int WaitOrKey(int s,int wid,int key) {
	DWORD tick = *(DWORD*)CMOS_PERIOD_TICK_COUNT;
	DWORD dest = tick + s;

	while (tick < dest) {
		tick = *(DWORD*)CMOS_PERIOD_TICK_COUNT;

		int ck = __kGetKbd(wid) & 0xff;
		if (key) {
			if (ck == key)
				break;
		}
		else {
			if (ck) {
				break;
			}
		}

		__sleep(0);
	}

	return 0;
}