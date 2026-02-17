#include "mainUtils.h"
#include "def.h"
#include "video.h"
#include "Utils.h"
#include "Kernel.h"
#include "task.h"
#include "malloc.h"
#include "core.h"
#include "apic.h"

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

int getpids(char * szout) {
	int outlen = 0;
	int len = 0;

	int cpus[256];
	int cnt = GetCpu(cpus, sizeof(cpus) / sizeof(cpus[0]));
	for (int i = 0; i < cnt; i++) {
		LPPROCESS_INFO tss = (LPPROCESS_INFO)GetTaskTssBaseId(cpus[i]);
		for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
			if (tss[i].status == TASK_RUN)
			{
				len = __sprintf(szout + outlen, "filename:%s, funcname:%s, address:%x,cpu:%d, pid:%d,tid:%d,level:%d\r\n",
					tss[i].filename, tss[i].funcname, tss[i].moduleaddr, tss[i].cpuid, tss[i].pid, tss[i].tid, tss[i].level);
				outlen += len;
			}
		}
	}



	return outlen;
}



int CpuUsage(char * buf) {
	int* ids = (int*)CPU_ID_ADDRESS;
	int counter = *(int*)(CPU_TOTAL_ADDRESS);
	int len = 0;
	int offset = 0;
	
	for (int i = 0; i < counter; i++) {
		int id = ids[i];
		
		//unsigned long long t = g_cpu_active[id];

		len = __sprintf(buf + offset, "cpu:%d,active:%i64x,sleep:%i64x;", id, g_cpu_active[id], g_cpu_sleep[id]);
		offset += len;
	}
	return offset;
}


int getpid(int pid,char * szout) {
	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetTaskTssBase();
	for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
		if (tss[i].status == TASK_RUN && tss[i].pid == pid)
		{
			int len = __sprintf(szout, "filename:%s, funcname:%s, address:%x,cpu:%d, pid:%d,tid:%d,level:%d\r\n",
				tss[i].filename, tss[i].funcname, tss[i].moduleaddr,tss[i].cpuid,tss[i].pid, tss[i].tid, tss[i].level);
			return len;
		}
	}

	*szout = 0;
	return 0;
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