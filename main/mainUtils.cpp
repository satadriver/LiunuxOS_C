#include "mainUtils.h"
#include "def.h"
#include "video.h"
#include "Utils.h"
#include "Kernel.h"
#include "task.h"
#include "malloc.h"
#include "core.h"
#include "apic.h"
#include "systemService.h"
#include "libc.h"
#include "DeepLearning.h"
#include "cmosPeriodTimer.h"
#include "apicTimer.h"


int GetMaxProcessNum(int * cpuid) {

	int cpus[256];
	int cnt = GetCpu(cpus, sizeof(cpus) / sizeof(cpus[0]));

	int id = -1;
	int max = -1;
	for (int num = 0; num < cnt; num++) {
		int cpu = cpus[num];

		int counter = 0;
#ifdef TASK_SWITCH_ARRAY
		LPPROCESS_INFO tss = (LPPROCESS_INFO)GetTaskTssBaseId(cpu);
		for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
			if (tss[i].status == TASK_RUN)
			{
				counter++;
			}
		}
#else
		PROCESS_INFO_LIST* list = (PROCESS_INFO_LIST*)GetTaskListId(cpu);
		PROCESS_INFO_LIST* hdr = (PROCESS_INFO_LIST*)list->list.next;
		PROCESS_INFO_LIST* ptr = hdr;
		do {
			if (ptr == 0)
				break;
			LPPROCESS_INFO p = &ptr->proc;
			if (p->status == TASK_RUN) {
				counter++;
			}
			ptr = (PROCESS_INFO_LIST*)ptr->list.next;
		} while (ptr && ptr != hdr);
#endif
		if (counter > 1 && counter > max) {
			max = counter;
			id = cpu;
		}
	}
	*cpuid = id;
	return max;
}

int GetProcessNum() {
	int n = 0;
	int cpus[256];
	int cnt = GetCpu(cpus, sizeof(cpus) / sizeof(cpus[0]));
	for (int num = 0; num < cnt; num++) {
		int cpu = cpus[num];
		
		int counter = 0;
#ifdef TASK_SWITCH_ARRAY
		LPPROCESS_INFO tss = (LPPROCESS_INFO)GetTaskTssBaseId(cpu);
		for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
			if (tss[i].status == TASK_RUN)
			{		
				counter++;
			}
		}
#else
		PROCESS_INFO_LIST* list = (PROCESS_INFO_LIST*)GetTaskListId(cpu);
		PROCESS_INFO_LIST* hdr = (PROCESS_INFO_LIST*)list->list.next;
		PROCESS_INFO_LIST* ptr = hdr;
		do {
			if (ptr == 0)
				break;
			LPPROCESS_INFO p = &ptr->proc;
			if (p->status == TASK_RUN) {
				counter++;
			}
			ptr = (PROCESS_INFO_LIST*)ptr->list.next;
		} while (ptr && ptr != hdr);
#endif
		g_tasks_cpu_array[cpu] = counter;
		n += counter;
	}

	return n;
}


int GetSimpleProcesses(char* szout) {
	int outlen = 0;
	int len = 0;

	unsigned long long tick = __krdtsc();
	int n = 0;
	int cpus[256];
	int cnt = GetCpu(cpus, sizeof(cpus) / sizeof(cpus[0]));
	for (int num = 0; num < cnt; num++) {
		int cpu = cpus[num];
		double cpu_diff = tick - g_cpu_start_tick[cpu];
		double cpu_usage = g_cpu_tick[cpu] / cpu_diff;
#ifdef TASK_SWITCH_ARRAY
		LPPROCESS_INFO tss = (LPPROCESS_INFO)GetTaskTssBaseId(cpu);
		for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
			if (tss[i].status == TASK_RUN)
			{
				double proc_diff = tss[i].tick_total;
				double proc_ratio = (double)tss[i].tick_run / proc_diff;
				double proc_usage = (double)tss[i].tick_run / (double)g_cpu_tick[cpu];
				double cost = tss[i].tick_switch_cost;
				double switch_cost = cost / g_timer_tick[cpu];
				len = __sprintf(szout + outlen,
					"[%d].file:%s, function:%s, cpu:%d, pid:%d, ppid:%d,tid:%d,level:%d,cpu usage:%lf£¬task rate:%lf,task usage:%lf,cost rate:%lf,counter:%x,priority:%d,delta:%d,lpvasize:%x\r\n",
					n++, tss[i].filename, tss[i].funcname,  tss[i].cpuid,
					tss[i].pid, tss[i].ppid, tss[i].tid, tss[i].level,
					cpu_usage, proc_ratio, proc_usage, switch_cost,  tss[i].counter, tss[i].priority, tss[i].delta, *tss[i].lpvasize);
				outlen += len;
			}
		}
#else
		PROCESS_INFO_LIST* list = (PROCESS_INFO_LIST*)GetTaskListId(cpu);
		PROCESS_INFO_LIST* hdr = (PROCESS_INFO_LIST*)list->list.next;
		PROCESS_INFO_LIST* ptr = hdr;
		do {
			if (ptr == 0)
				break;
			LPPROCESS_INFO p = &ptr->proc;
			if (p->status == TASK_RUN) {
				double proc_diff =  p->tick_total;
				double proc_ratio = (double)p->tick_run / proc_diff;
				double proc_usage = (double)p->tick_run / (double)g_cpu_tick[cpu];
				double cost = p->tick_switch_cost;
				double switch_cost = cost / g_timer_tick[cpu];
				len = __sprintf(szout + outlen,
					"[%d].file:%s, function:%s, cpu:%d, pid:%d, ppid:%d,tid:%d,level:%d,cpu usage:%lf£¬task rate:%lf,task usage:%lf,cost rate:%lf,counter:%x,priority:%d,delta:%d,lpvasize:%x\r\n",
					n++, p->filename, p->funcname, p->cpuid,
					p->pid, p->ppid, p->tid, p->level,
					cpu_usage, proc_ratio, proc_usage, switch_cost, p->counter, p->priority, p->delta, *p->lpvasize);
				outlen += len;
			}
			ptr = (PROCESS_INFO_LIST*)ptr->list.next;
		} while (ptr && ptr != hdr);
#endif
	}

	return outlen;
}

int GetAllProcesses(char* szout) {
	int outlen = 0;
	int len = 0;

	unsigned long long tick = __krdtsc();
	int n = 0;
	int cpus[256];
	int cnt = GetCpu(cpus, sizeof(cpus) / sizeof(cpus[0]));
	for (int num = 0; num < cnt; num++) {
		int cpu = cpus[num];
		double cpu_diff = tick - g_cpu_start_tick[cpu];
		double cpu_usage = g_cpu_tick[cpu] / cpu_diff;

#ifdef TASK_SWITCH_ARRAY
		LPPROCESS_INFO tss = (LPPROCESS_INFO)GetTaskTssBaseId(cpu);
		for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
			if (tss[i].status == TASK_RUN)
			{
				double proc_diff = tss[i].tick_total;
				double proc_ratio = (double)tss[i].tick_run / proc_diff;
				double proc_usage = (double)tss[i].tick_run / (double)g_cpu_tick[cpu];
				double cost = tss[i].tick_switch_cost;
				double switch_cost = cost / g_timer_tick[cpu];
				len = __sprintf(szout + outlen, 
					"[%d]. filename:%s, funcname:%s, base:%x,cpu:%d, pid:%d, ppid:%d,tid:%d,level:%d,tick:%i64x,cost:%i64x,start:%i64x,cpu usage:%lf£¬task rate:%lf,task usage:%lf,switch_cost:%lf,sleep:%x,counter:%x,slice:%d,priority:%d,delta:%d,lpvasize:%x,HeapCnt:%d\r\n\r\n",
					n++,tss[i].filename, tss[i].funcname, tss[i].moduleBase, tss[i].cpuid,
					tss[i].pid, tss[i].ppid, tss[i].tid, tss[i].level, tss[i].tick_run, tss[i].tick_switch_cost, tss[i].tick_start,
					cpu_usage,proc_ratio, proc_usage, switch_cost,tss[i].sleep_total, tss[i].counter, tss[i].slice, tss[i].priority, tss[i].delta, *tss[i].lpvasize, *tss[i].lpHeapCnt);
				outlen += len;
			}
		}
#else
		PROCESS_INFO_LIST* list = (PROCESS_INFO_LIST*)GetTaskListId(cpu);
		PROCESS_INFO_LIST* hdr = (PROCESS_INFO_LIST*)list->list.next;
		PROCESS_INFO_LIST* ptr = hdr;
		do {
			if (ptr == 0)
				break;
			LPPROCESS_INFO p = &ptr->proc;
			if (p->status == TASK_RUN) {
				double proc_diff = p->tick_total;
				double proc_ratio = (double)p->tick_run / proc_diff;
				double proc_usage = (double)p->tick_run / (double)g_cpu_tick[cpu];
				double cost = p->tick_switch_cost;
				double switch_cost = cost / g_timer_tick[cpu];
				len = __sprintf(szout + outlen,
					"[%d]. filename:%s, funcname:%s, base:%x,cpu:%d, pid:%d, ppid:%d,tid:%d,level:%d,tick:%i64x,cost:%i64x,start:%i64x,cpu usage:%lf£¬task rate:%lf,task usage:%lf,switch_cost:%lf,sleep:%x,counter:%x,slice:%d,priority:%d,delta:%d,lpvasize:%x,HeapCnt:%d\r\n\r\n",
					n++, p->filename, p->funcname, p->moduleBase, p->cpuid,
					p->pid, p->ppid, p->tid, p->level, p->tick_run, p->tick_switch_cost, p->tick_start,
					cpu_usage, proc_ratio, proc_usage, switch_cost, p->sleep_total, p->counter, p->slice, p->priority, 
					p->delta, *p->lpvasize, *p->lpHeapCnt);
				outlen += len;
			}
			ptr = (PROCESS_INFO_LIST*)ptr->list.next;
		} while (ptr && ptr != hdr);
#endif
	}

	return outlen;
}

int GetProcess(int cpuid,int pid, char* szout) {
	int cpus[256];
	int cnt = GetCpu(cpus, sizeof(cpus) / sizeof(cpus[0]));

	unsigned long long tick = __krdtsc();

	for (int num = 0; num < cnt; num++) {
		
		int cpu = cpus[num];
		if (cpu == cpuid) {
			double cpu_diff = tick - g_cpu_start_tick[cpu];
			double cpu_usage = g_cpu_tick[cpu] / cpu_diff;		
#ifdef TASK_SWITCH_ARRAY
			LPPROCESS_INFO tss = (LPPROCESS_INFO)GetTaskTssBase();
			for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
				if (tss[i].status == TASK_RUN && tss[i].pid == pid)
				{
					double proc_diff = tss[i].tick_total;
					double proc_ratio = (double)tss[i].tick_run / proc_diff;
					double cost = tss[i].tick_switch_cost;
					double switch_cost = cost / g_timer_tick[cpu];
					double proc_usage = (double)tss[i].tick_run / (double)g_cpu_tick[cpu];
					int len = __sprintf(szout, 
						"filename:%s, funcname:%s, base:%x,cpu:%d, pid:%d, ppid:%d,tid:%d,level:%d,tick:%i64x,cost:%i64x,start:%i64x,cpu usage:%lf,task ratio:%lf,task usage:%lf,switch_cost:%lf,sleep:%x,counter:%x,slice:%d,priority:%d,delta:%d,lpvasize:%x,HeapCnt:%d\r\n\r\n",
						tss[i].filename, tss[i].funcname, tss[i].moduleBase, tss[i].cpuid,
						tss[i].pid, tss[i].ppid, tss[i].tid, tss[i].level, tss[i].tick_run, tss[i].tick_switch_cost,tss[i].tick_start,
						cpu_usage, proc_ratio, proc_usage, switch_cost, tss[i].sleep_total, tss[i].counter, tss[i].slice, tss[i].priority, tss[i].delta, *tss[i].lpvasize, *tss[i].lpHeapCnt);
					return len;
				}
			}
#else
			PROCESS_INFO_LIST* list = (PROCESS_INFO_LIST*)GetTaskListId(cpu);
			PROCESS_INFO_LIST* hdr = (PROCESS_INFO_LIST*)list->list.next;
			PROCESS_INFO_LIST* ptr = hdr;
			do {
				if (ptr == 0)
					break;
				LPPROCESS_INFO p = &ptr->proc;
				if (p->status == TASK_RUN && p->pid == pid) {
					double proc_diff = p->tick_total;
					double proc_ratio = (double)p->tick_run / proc_diff;
					double cost = p->tick_switch_cost;
					double switch_cost = cost / g_timer_tick[cpu];
					double proc_usage = (double)p->tick_run / (double)g_cpu_tick[cpu];
					int len = __sprintf(szout,
						"filename:%s, funcname:%s, base:%x,cpu:%d, pid:%d, ppid:%d,tid:%d,level:%d,tick:%i64x,cost:%i64x,start:%i64x,cpu usage:%lf,task ratio:%lf,task usage:%lf,switch_cost:%lf,sleep:%x,counter:%x,slice:%d,priority:%d,delta:%d,lpvasize:%x,HeapCnt:%d\r\n\r\n",
						p->filename, p->funcname, p->moduleBase, p->cpuid,
						p->pid, p->ppid, p->tid, p->level, p->tick_run, p->tick_switch_cost, p->tick_start,
						cpu_usage, proc_ratio, proc_usage, switch_cost, p->sleep_total, p->counter, p->slice, p->priority, p->delta, *p->lpvasize, *p->lpHeapCnt);
					return len;
				}
				ptr = (PROCESS_INFO_LIST*)ptr->list.next;
			} while (ptr && ptr != hdr);
#endif
		}
	}
	*szout = 0;
	return 0;
}



int GetMemory(char * szout,int pid) {

	unsigned long long allocated = 0;

	unsigned long long cpuMem[MAX_CPU_COUNT];

	__memset((char*)cpuMem, 0, sizeof(cpuMem));

	LPMEMALLOCINFO info = (LPMEMALLOCINFO)gMemAllocList->list.next;

	LPMEMALLOCINFO base = info;

	int len = 0;

	unsigned long long procMem = 0;

	do
	{	
		allocated += info->size;

		info = (LPMEMALLOCINFO)info->list.next;

		int id = info->vid >> 16;
		cpuMem[id] += info->size;

		if (pid != -1 && (info->vid & 0xffff) == pid) {
			procMem += info->size;
		}
			
	} while (info != base);
	
	len += __sprintf(szout+len, "total size:%i64x,allocated size:%i64x, available size:%i64x\r\n",
		gAvailableSize, allocated, gAvailableSize - allocated);

	int* ids = (int*)g_cpu_id;
	int cpuCnt = g_cpu_total;

	for (int i = 0; i < cpuCnt; i++) {
		int id = ids[i];
		len += __sprintf(szout + len, "cpu:%d used size:%i64x\r\n",id, cpuMem[id]);
	}

	if (pid != -1) {
		len += __sprintf(szout + len, "pid:%d used size:%i64x\r\n", pid, procMem);
	}

	return len;
}



int GetHeap(int cpu, int tid,char * szout) {
	*szout = 0;
	int pos = 0;

	int cpus[256];
	int cnt = GetCpu(cpus, sizeof(cpus) / sizeof(cpus[0]));

	for (int num = 0; num < cnt; num++) {
		int id = cpus[num];
		
		if (id == cpu) {
#ifdef TASK_SWITCH_ARRAY
			LPPROCESS_INFO procs = GetTaskTssBaseId(id);
			for (int j = 0; j < TASK_LIMIT_TOTAL; j++) {
				if (procs[j].tid == tid && procs[j].status == TASK_RUN) {
					int heapcnt = *procs[j].lpHeapCnt;
					for (int k = 0; k < heapcnt; k++) {
						int len = __sprintf(szout + pos, "[%d] heap base:%x,heap size:%x\r\n",
							k,(*procs[j].lpHeapBase)[k], procs[j].heapsize << k);
						pos += len;
					}
				}
			}
#else
			PROCESS_INFO_LIST* list = (PROCESS_INFO_LIST*)GetTaskListId(cpu);
			PROCESS_INFO_LIST* hdr = (PROCESS_INFO_LIST*)list->list.next;
			PROCESS_INFO_LIST* ptr = hdr;
			do {
				if (ptr == 0)
					break;
				LPPROCESS_INFO p = &ptr->proc;
				if (p->status == TASK_RUN && p->tid == tid) {
					int heapcnt = *p->lpHeapCnt;
					for (int k = 0; k < heapcnt; k++) {
						int len = __sprintf(szout + pos, "[%d] heap base:%x,heap size:%x\r\n",
							k, (*p->lpHeapBase)[k], p->heapsize << k);
						pos += len;
					}
				}
				ptr = (PROCESS_INFO_LIST*)ptr->list.next;
			} while (ptr && ptr != hdr);
#endif
		}
	}
	return pos;
}


int CpuUsage(char* buf) {
	int* ids = (int*)g_cpu_id;
	int counter = g_cpu_total;
	int len = 0;
	int offset = 0;

	unsigned long long tick = __krdtsc();

	for (int i = 0; i < counter; i++) {
		int id = ids[i];

		unsigned long long total = tick - g_cpu_start_tick[id];

		double diff = total;

		double usage = g_cpu_tick[id];

		usage = usage / diff;

		double load = 1.0;
		unsigned long long aperf = 0;
		unsigned long long mperf = 0;

		if (g_pm_enable) {
			unsigned long long e7low = 0;
			unsigned long long e7high = 0;
			RdMsr(MSR_IA32_MPERF, (unsigned long*)&e7low, (unsigned long*)&e7high);

			mperf = (e7high << 32) + e7low;

			unsigned long long e8low = 0;
			unsigned long long e8high = 0;
			RdMsr(MSR_IA32_APERF, (unsigned long*)&e8low, (unsigned long*)&e8high);
			aperf = (e8high << 32) + e8low;

			load = (e8high << 32) + e8low;
			load = load / ((e7high << 32) + e7low);
		}
		
		len = __sprintf(buf + offset, 
			"[%d]. cpu:%d,active:%i64x,total:%i64x,rate:%lf,g_timer_tick:%i64x,g_apic_freq:%i64x,aperf:%i64x,mperf:%i64x,load:%lf,g_pm_enable:%d\r\n\r\n",
			i,id, g_cpu_tick[id], total, usage, g_timer_tick[id], g_apic_freq[id], aperf, mperf,load, g_pm_enable);
		offset += len;
	}
	return offset;
}



int GetTaskRatio(char* szout) {

	int outlen = 0;
	int len = 0;
	int n = 0;
	unsigned long long tick = __krdtsc();

	int cpus[256];
	int cnt = GetCpu(cpus, sizeof(cpus) / sizeof(cpus[0]));
	for (int num = 0; num < cnt; num++) {
		int cpu = cpus[num];
		double cpu_diff = tick - g_cpu_start_tick[cpu];
		double cpu_usage = g_cpu_tick[cpu] / cpu_diff;
		
#ifdef TASK_SWITCH_ARRAY
		LPPROCESS_INFO tss = (LPPROCESS_INFO)GetTaskTssBaseId(cpu);
		for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
			if (tss[i].status == TASK_RUN)
			{
				double proc_diff = tss[i].tick_total;
				double proc_alive = (double)tss[i].tick_run / proc_diff;
				double cost = tss[i].tick_switch_cost;
				double switch_cost = cost / g_timer_tick[cpu];
				double proc_cpu = (double)tss[i].tick_run / (double)g_cpu_tick[cpu];
				len = __sprintf(szout + outlen,
					"[%d]. funcname:%s, cpu:%d,pid:%d,tid:%d,g_cpu_tick:%i64x,cpu usage:%lf,tick_run:%i64x,task alive:%lf,task cpu:%lf,switch_cost:%lf\r\n",
					n++,tss[i].funcname,  tss[i].cpuid,tss[i].pid,tss[i].tid, g_cpu_tick[cpu], cpu_usage, tss[i].tick_run, proc_alive, proc_cpu,switch_cost);
				outlen += len;
			}
		}
#else
		PROCESS_INFO_LIST* list = (PROCESS_INFO_LIST*)GetTaskListId(cpu);
		PROCESS_INFO_LIST* hdr = (PROCESS_INFO_LIST*)list->list.next;
		PROCESS_INFO_LIST* ptr = hdr;
		do {
			if (ptr == 0)
				break;
			LPPROCESS_INFO p = &ptr->proc;
			if (p->status == TASK_RUN) {
				double proc_diff = p->tick_total;
				double proc_alive = (double)p->tick_run / proc_diff;
				double cost = p->tick_switch_cost;
				double switch_cost = cost / g_timer_tick[cpu];
				double proc_cpu = (double)p->tick_run / (double)g_cpu_tick[cpu];
				len = __sprintf(szout + outlen,
					"[%d]. funcname:%s, cpu:%d,pid:%d,tid:%d,g_cpu_tick:%i64x,cpu usage:%lf,tick_run:%i64x,task alive:%lf,task cpu:%lf,switch_cost:%lf\r\n",
					n++, p->funcname, p->cpuid, p->pid, p->tid, g_cpu_tick[cpu], cpu_usage, p->tick_run, proc_alive, proc_cpu, switch_cost);
				outlen += len;
			}
			ptr = (PROCESS_INFO_LIST*)ptr->list.next;
		} while (ptr && ptr != hdr);
#endif
	}

	return outlen;
}



DWORD InterruptPerSec() {
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20)>>24;
	DWORD tick = g_cmos_period_tick_cnt;
	DWORD tick2 = tick;
	while (tick2 == tick) {
		tick = g_cmos_period_tick_cnt;
		__sleep(0);
	}

	tick2 = tick + 1;

	DWORD ints = g_apictimer_tick_cnt[id];

	while (tick2 != tick) {
		tick = g_cmos_period_tick_cnt;
		__sleep(0);
	}

	DWORD ints2 = g_apictimer_tick_cnt [ id ];
	return ints2 - ints;
}



int getGeneralRegs(char * szout) {
	DWORD reax;
	DWORD recx;
	DWORD redx;
	DWORD rebx;
	DWORD rebp, resp, resi, redi;
	DWORD reip;

	DWORD eflags;

	DWORD res, rds, rfs, rgs, rss, rcs;
	__asm {
		mov reax,eax
		mov recx,ecx
		mov redx,edx
		mov rebx,ebx

		mov rebp, ebp
		mov resp, esp
		mov resi, esi
		mov redi, edi
		call _next

		_next:
		mov eax, dword ptr ss : [esp]
		mov reip,eax
		add esp,4

		xor eax,eax
		mov ax,cs
		mov rcs,eax

		mov ax,ds
		mov rds,eax

		mov ax,es
		mov res,eax

		mov ax,ss
		mov rss,eax

		mov ax,fs
		mov rfs,eax

		mov ax,gs
		mov rgs,eax

		pushfd
		pop eax
		mov eflags,eax
	}

	int len = __sprintf(szout, "eax:%x,ecx:%x,edx:%x,ebx:%x,ebp:%x,esp:%x,esi:%x,edi:%x,eflags:%x,ss:%x,es:%x,ds:%x,fs:%x,gs:%x,cs:%x,eip:%x\r\n",
		reax, recx, redx, rebx, rebp, resp, resi, redi, eflags,rss, res, rds, rfs, rgs, rcs, reip);
	return len;
}

//sldt lldt, str ltr is all 16 bit instructions
int getldt(char * szout) {

	WORD ldt = 0;

	__asm {
		sldt ax
		mov ldt,ax
	}

	int len = 0;
	int outlen = 0;

	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

	char* lpgdt = g_gdt_base[id];

	TssDescriptor * ldtbase = (TssDescriptor*)(lpgdt + ldtSelector);

	int ldtlen = ldtbase->len + 1 + ((ldtbase->lenHigh) << 16) ;

	__printf(szout, "ldt selector:%d,base:%x,size:%d\r\n",ldt, ldtbase, ldtlen);

	__int64 * pldts = (__int64*)((ldtbase->baseLow) + (ldtbase->baseMid << 16) + (ldtbase->baseHigh << 24));

	int cnt = ldtlen >> 3;

	for (int i = 0; i < cnt; i++)
	{
		len = __sprintf(szout + outlen, "ldt %d:%I64x\n", i, pldts[i]);
		outlen += len;
	}

	return outlen;
}

int getgdt(char * szout) {
	char strgdt[8] = { 0 };
	
	__asm {
		sgdt fword ptr [strgdt];
	}

	int len = 0;
	int outlen = 0;

	int gdtlen = *(WORD*)strgdt + 1;

	DWORD gdtbase = *(DWORD*)(strgdt + 2);

	__int64 * pgdts = (__int64*)gdtbase;

	int cnt = gdtlen >> 3;

	for (int i = 0; i < 64; i++)
	{
		if ( pgdts[i] || i == 0) {
			len = __sprintf(szout + outlen, "gdt %d:%I64x\n", i, pgdts[i]);
			outlen += len;
		}
		else {
			if (i == 0)
				continue;
			//break;
		}
	}

	return outlen;
}


int getidt(char * szout) {
	char stridt[8] = { 0 };

	__asm {
		sidt fword ptr[stridt];
	}

	int len = 0;

	int outlen = 0;

	int idtlen = *(WORD*)stridt + 1;

	DWORD idtbase = *(DWORD*)(stridt + 2);

	__int64 * pidts = (__int64*)idtbase; 

	int cnt = idtlen >> 3;

	for (int i = 0; i < cnt; i++)
	{
		len =  __sprintf(szout + outlen, "idt %d:%I64x\n", i, pidts[i]);
		outlen += len;
	}

	return outlen;
}

#ifdef _DEBUG

#else

#endif

#define malloc my_malloc
#define free my_free
#define realloc my_realloc
#define calloc my_calloc
#define printf my_printf

int HeapAllocTest(int cnt,int max,unsigned long long * total) {

	int err = 0;
	*total = 0;

	char buf[64];
	char** p = (char**)malloc(cnt * sizeof(int));
	if (p == 0) {
		return err;
	}

	for (int i = 0; i < cnt; i++) {
		int size = __random(0) % max;
		p[i] = (char*)malloc(size);
		__sprintf(p[i], "%x", p[i]);
		if (p[i] == 0) {
			printf("[%d] %s %d malloc address:%x,size:%x\r\n", i, __FUNCTION__,__LINE__, p[i], size);
			err++;
			continue;
		}
		(*total) += size;
	}

	for (int i = 0; i < cnt; i++) {
		
		__sprintf(buf, "%x", p[i]);
		if (__strcmp(buf, p[i])) {
			printf("[%d]:free address:%x,value:%s not equal:%s\r\n", i, p[i],p[i], buf);
			err++;
		}
		free(p[i]);
	}

	free(p);
	return err;
}

