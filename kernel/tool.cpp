
#include "tool.h"
#include "def.h"
#include "keyboard.h"
#include "systemService.h"
#include "Utils.h"
#include "apic.h"
#include "cmosPeriodTimer.h"
#include "task.h"








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



LPPROCESS_INFO __findProcessFuncName(const char* funcname) {
#ifdef TASK_SWITCH_ARRAY
	LPPROCESS_INFO p = (LPPROCESS_INFO)GetTaskTssBase();
	for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
		if (p[i].status == TASK_RUN && __strcmp(p[i].funcname, funcname) == 0) {
			return &p[i];
		}
	}
#else
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	PROCESS_INFO_LIST* list = (PROCESS_INFO_LIST*)GetTaskListId(id);
	PROCESS_INFO_LIST* hdr = (PROCESS_INFO_LIST*)list->list.next;
	PROCESS_INFO_LIST* ptr = hdr;
	do {
		if (ptr == 0)
			break;
		LPPROCESS_INFO p = &ptr->proc;
		if (p->status == TASK_RUN && __strcmp(p->funcname, funcname) == 0) {
			return p;
		}
		ptr = (PROCESS_INFO_LIST*)ptr->list.next;
	} while (ptr && ptr != hdr);
#endif
	return FALSE;
}

LPPROCESS_INFO __findProcessFileName(char* filename) {
#ifdef TASK_SWITCH_ARRAY
	LPPROCESS_INFO p = (LPPROCESS_INFO)GetTaskTssBase();
	for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
		if (p[i].status == TASK_RUN && __strcmp(p[i].filename, filename) == 0) {
			return &p[i];
		}
	}
#else
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	PROCESS_INFO_LIST* list = (PROCESS_INFO_LIST*)GetTaskListId(id);
	PROCESS_INFO_LIST* hdr = (PROCESS_INFO_LIST*)list->list.next;
	PROCESS_INFO_LIST* ptr = hdr;
	do {
		if (ptr == 0)
			break;
		LPPROCESS_INFO p = &ptr->proc;
		if (p->status == TASK_RUN && __strcmp(p->filename, filename) == 0) {
			return p;
		}
		ptr = (PROCESS_INFO_LIST*)ptr->list.next;
	} while (ptr && ptr != hdr);
#endif
	return FALSE;
}



LPPROCESS_INFO __findProcessByPid(int pid) {
#ifdef TASK_SWITCH_ARRAY
	LPPROCESS_INFO p = (LPPROCESS_INFO)GetTaskTssBase();
	for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
		if (p[i].status == TASK_RUN && p[i].pid == pid) {
			return &p[i];
		}
	}
#else
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	PROCESS_INFO_LIST* list = (PROCESS_INFO_LIST*)GetTaskListId(id);
	PROCESS_INFO_LIST* hdr = (PROCESS_INFO_LIST*)list->list.next;
	PROCESS_INFO_LIST* ptr = hdr;
	do {
		if (ptr == 0)
			break;
		LPPROCESS_INFO p = &ptr->proc;
		if (p->status == TASK_RUN && p->pid == pid) {
			return p;
		}
		ptr = (PROCESS_INFO_LIST*)ptr->list.next;
	} while (ptr && ptr != hdr);
#endif
	return FALSE;
}


LPPROCESS_INFO __findProcessByTid(int tid) {
#ifdef TASK_SWITCH_ARRAY
	LPPROCESS_INFO p = (LPPROCESS_INFO)GetTaskTssBase();
	for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
		if (p[i].status == TASK_RUN && p[i].tid == tid) {
			return &p[i];
		}
	}
#else
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	PROCESS_INFO_LIST* list = (PROCESS_INFO_LIST*)GetTaskListId(id);
	PROCESS_INFO_LIST* hdr = (PROCESS_INFO_LIST*)list->list.next;
	PROCESS_INFO_LIST* ptr = hdr;
	do {
		if (ptr == 0)
			break;
		LPPROCESS_INFO p = &ptr->proc;
		if (p->status == TASK_RUN && p->tid == tid) {
			return p;
		}
		ptr = (PROCESS_INFO_LIST*)ptr->list.next;
	} while (ptr && ptr != hdr);
#endif
	return FALSE;
}


int __terminateByFileName(char* filename) {
	enter_task_array_lock();;
	LPPROCESS_INFO p = __findProcessFileName(filename);
	if (p)
	{
		p->status = TASK_OVER;
	}
	leave_task_array_lock();
	return FALSE;
}

int __terminateByFuncName(char* funcname) {
	enter_task_array_lock();
	PROCESS_INFO* p = __findProcessFuncName(funcname);
	if (p)
	{
		p->status = TASK_OVER;

	}
	leave_task_array_lock();
	return FALSE;
}

int __terminatePid(int pid) {
	enter_task_array_lock();
	PROCESS_INFO* p = __findProcessByPid(pid);
	if (p)
	{
		p->status = TASK_OVER;
	}
	leave_task_array_lock();
	return 0;
}


int __terminateTid(int tid) {
	enter_task_array_lock();
	PROCESS_INFO* p = __findProcessByTid(tid);
	if (p)
	{
		p->status = TASK_OVER;
	}
	leave_task_array_lock();
	return 0;
}



int __pauseTid(int tid) {
	enter_task_array_lock();
	PROCESS_INFO* p = __findProcessByTid(tid);
	if (p)
	{
		p->status = TASK_SUSPEND;
	}
	leave_task_array_lock();
	return 0;
}


int __resumeTid(int tid) {
	enter_task_array_lock();
	PROCESS_INFO* p = __findProcessByTid(tid);
	if (p)
	{
		p->status = TASK_RUN;
	}
	leave_task_array_lock();
	return 0;
}


int __pausePid(int pid) {
	enter_task_array_lock();
	PROCESS_INFO* p = __findProcessByPid(pid);
	if (p)
	{
		p->status = TASK_SUSPEND;
	}
	leave_task_array_lock();
	return 0;
}


int __resumePid(int pid) {
	enter_task_array_lock();
	PROCESS_INFO* p = __findProcessByPid(pid);
	if (p)
	{
		p->status = TASK_RUN;
	}
	leave_task_array_lock();
	return 0;
}

