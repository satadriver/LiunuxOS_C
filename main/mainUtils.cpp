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

int getcrs(char * szout) {

	if (szout)
	{
		*szout = 0;
	}
	else {
		return 0;
	}

	LPPROCESS_INFO process = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	int dsreg = process->tss.cs;
	if (dsreg & 3)
	{
		__printf(szout,"you have no privilege to get crs\r\n");
		return 0;
	}

	DWORD rcr0 = 0;
	DWORD rcr2 = 0;
	DWORD rcr3 = 0;
	DWORD rcr4 = 0;

	__asm {
		mov eax, cr0
		mov rcr0, eax

		mov eax, cr2
		mov rcr2, eax

		mov eax, cr3
		mov rcr3, eax

		//mov eax, cr4	//db 0fh, 20h, 0e0h
		__emit 0xf
		__emit 0x20
		__emit 0xe0
		mov rcr4, eax
	}

	int len = __printf(szout, "cr0:%x,cr2:%x,cr3:%x,:%x\n", rcr0, rcr2, rcr3, rcr4);
	return len;
}

int getmemmap(int pid, char* szout) {
	int cpu = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	return getProcMemory(pid,cpu, szout);
}

int GetAllProcesses(char* szout) {
	int outlen = 0;
	int len = 0;

	unsigned long long tick = __krdtsc();

	int cpus[256];
	int cnt = GetCpu(cpus, sizeof(cpus) / sizeof(cpus[0]));
	for (int i = 0; i < cnt; i++) {
		int cpu = cpus[i];
		double cpu_diff = tick - g_cpu_start_tick[cpu];
		double cpu_ratio = g_cpu_tick[cpu] / cpu_diff;
		LPPROCESS_INFO tss = (LPPROCESS_INFO)GetTaskTssBaseId(cpu);
		for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
			if (tss[i].status == TASK_RUN)
			{
				double proc_diff = tss[i].tick_total;
				double proc_ratio = (double)tss[i].tick / proc_diff;
				double cost = tss[i].tick_cost;
				double switch_cost = cost / g_unit_cost[cpu];
				len = __sprintf(szout + outlen, 
					"filename:%s, funcname:%s, base:%x,cpu:%d, pid:%d, ppid:%d,tid:%d,level:%d,tick:%i64x,cost:%i64x,start:%i64x,cpu usage:%lf£¬task usage:%lf,switch_cost:%lf,sleep:%x,counter:%x,slice:%d,priority:%d,delta:%d,lpvasize:%x,HeapCnt:%d\r\n\r\n",
					tss[i].filename, tss[i].funcname, tss[i].moduleBase, tss[i].cpuid,
					tss[i].pid, tss[i].ppid, tss[i].tid, tss[i].level, tss[i].tick, tss[i].tick_cost, tss[i].tick_start,
					cpu_ratio,proc_ratio, switch_cost,tss[i].sleep_total, tss[i].counter, tss[i].slice, tss[i].priority, tss[i].delta, *tss[i].lpvasize, *tss[i].lpHeapCnt);
				outlen += len;
			}
		}
	}

	return outlen;
}

int GetProcess(int cpuid,int pid, char* szout) {
	int cpus[256];
	int cnt = GetCpu(cpus, sizeof(cpus) / sizeof(cpus[0]));

	unsigned long long tick = __krdtsc();

	for (int i = 0; i < cnt; i++) {
		
		int cpu = cpus[i];
		if (cpu == cpuid) {
			double cpu_diff = tick - g_cpu_start_tick[cpu];
			double cpu_ratio = g_cpu_tick[cpu] / cpu_diff;

			LPPROCESS_INFO tss = (LPPROCESS_INFO)GetTaskTssBase();
			for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
				if (tss[i].status == TASK_RUN && tss[i].pid == pid)
				{
					double proc_diff = tss[i].tick_total;
					double proc_ratio = (double)tss[i].tick / proc_diff;
					double cost = tss[i].tick_cost;
					double switch_cost = cost / g_unit_cost[cpu];	
					int len = __sprintf(szout, 
						"filename:%s, funcname:%s, base:%x,cpu:%d, pid:%d, ppid:%d,tid:%d,level:%d,tick:%i64x,cost:%i64x,start:%i64x,cpu usage:%lf,task usage:%lf,switch_cost:%lf,sleep:%x,counter:%x,slice:%d,priority:%d,delta:%d,lpvasize:%x,HeapCnt:%d\r\n\r\n",
						tss[i].filename, tss[i].funcname, tss[i].moduleBase, tss[i].cpuid,
						tss[i].pid, tss[i].ppid, tss[i].tid, tss[i].level, tss[i].tick, tss[i].tick_cost,tss[i].tick_start,
						cpu_ratio, proc_ratio, switch_cost, tss[i].sleep_total, tss[i].counter, tss[i].slice, tss[i].priority, tss[i].delta, *tss[i].lpvasize, *tss[i].lpHeapCnt);
					return len;
				}
			}
		}
	}
	*szout = 0;
	return 0;
}

int CpuUsage(char* buf) {
	int* ids = (int*)CPU_ID_ADDRESS;
	int counter = *(int*)(CPU_TOTAL_ADDRESS);
	int len = 0;
	int offset = 0;

	unsigned long long tick = __krdtsc();

	for (int i = 0; i < counter; i++) {
		int id = ids[i];

		unsigned long long alive = tick - g_cpu_start_tick[id];

		double diff = alive;

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
			"cpu:%d,active:%i64x,alive:%i64x,rate:%lf,g_unit_cost:%i64x,g_apic_freq:%i64x,aperf:%i64x,mperf:%i64x,load:%lf,g_pm_enable:%d\r\n\r\n",
			id, g_cpu_tick[id], alive, usage, g_unit_cost[id], g_apic_freq[id], aperf, mperf,load, g_pm_enable);
		offset += len;
	}
	return offset;
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

	TssDescriptor * ldtbase = (TssDescriptor*)(GDT_BASE + ldtSelector);

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
		sgdt fword ptr[strgdt];
	}

	int l = 0;
	int outlen = 0;

	int gdtlen = *(WORD*)strgdt + 1;

	DWORD gdtbase = *(DWORD*)(strgdt + 2);

	__int64 * pgdts = (__int64*)gdtbase;

	int cnt = gdtlen >> 3;

	for (int i = 0; i < cnt; i++)
	{
		l = __sprintf(szout + outlen, "gdt %d:%I64x\n", i, pgdts[i]);
		outlen += l;
	}

	return outlen;
}


int getidt(char * szout) {
	char stridt[8] = { 0 };

	__asm {
		sidt fword ptr[stridt];
	}

	int l = 0;

	int outlen = 0;

	int idtlen = *(WORD*)stridt + 1;

	DWORD idtbase = *(DWORD*)(stridt + 2);

	__int64 * pidts = (__int64*)idtbase; 

	int cnt = idtlen >> 3;

	for (int i = 0; i < cnt; i++)
	{
		l =  __sprintf(szout + outlen, "idt %d:%I64x\n", i, pidts[i]);
		outlen += l;

	}

	return outlen;
}

void __kSysRegs() {

}