
#include "video.h"
#include "Utils.h"
#include "exception.h"
#include "task.h"
#include "process.h"
#include "Pe.h"
#include "memory.h"
#include "Thread.h"
#include "apic.h"


#define EXCEPTION_TIPS_COLOR 0X9F3F00

int g_ExceptionCounter = 0;

void __kException(const char* descriptor, int num, LIGHT_ENVIRONMENT* param){

	char showinfo[0x1000];

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

		//mov eax, cr4
		__emit 0x0f
		__emit 0x20
		__emit 0xe0
		mov rcr4, eax
	}

	LPPROCESS_INFO process = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	int tid = process->tid;
	int pid = process->pid;
	int level = process->level;

	int len = 0;
	if (num == 8 || num == 10 || num == 11 || num == 12 || num == 13 || num == 14 || num == 17 || num == 21)
	{
		LPEXCEPTIONCODESTACK tss = (LPEXCEPTIONCODESTACK)param;
		if (tss->eflags & 0x20000)
		{
			DWORD rva = tss->eip;

			len = __sprintf(showinfo,
				"VM86 Exception:%d,tid:%d,pid:%d,CODE:%x,RVA:%x,esp0:%x,ss0:%x,eip:%x,cs:%x,eflags:%x,esp3:%x,ss3:%x,"
				"ds:%x,es:%x,fs:%x,gs:%x,eax:%x,ecx:%x,edx:%x,ebx:%x,ebp:%x,esi:%x,edi:%x,ds_v86:%x,es_v86:%x,fs_v86:%x,gs_v86:%x,\
				cr0:%x,cr2:%x,cr3:%x,cr4:%x\r\n",
				num, tid,pid, tss->errcode, rva, tss->esp0, tss->ss0, tss->eip, tss->cs, tss->eflags, tss->esp3, tss->ss3,
				tss->ds, tss->es, tss->fs, tss->gs, tss->eax, tss->ecx, tss->edx, tss->ebx, tss->ebp, tss->esi, tss->edi,
				tss->ds_v86, tss->es_v86, tss->fs_v86, tss->gs_v86, rcr0, rcr2, rcr3, rcr4);
		}
		else if ((level & 3) ||( tss->cs & 3))
		{
			DWORD rva = rvaInFile(process->moduleaddr, tss->eip );

			len = __sprintf(showinfo,
				"Exception:%d,tid:%d,pid:%d,CODE:%x,RVA:%x,esp0:%x,ss0:%x,eip:%x,cs:%x,eflags:%x,esp3:%x,ss3:%x,"
				"ds:%x,es:%x,fs:%x,gs:%x,eax:%x,ecx:%x,edx:%x,ebx:%x,ebp:%x,esi:%x,edi:%x,cr0:%x,cr2:%x,cr3:%x,cr4:%x\r\n",
				num,tid, pid, tss->errcode,rva, tss->esp0, tss->ss0, tss->eip, tss->cs, tss->eflags, tss->esp3, tss->ss3,
				tss->ds, tss->es, tss->fs, tss->gs, tss->eax, tss->ecx, tss->edx, tss->ebx, tss->ebp, tss->esi, tss->edi,
				rcr0,rcr2,rcr3, rcr4);
		}
		else {
			DWORD rva = rvaInFile(process->moduleaddr, tss->eip );

			len = __sprintf(showinfo,
				"Exception:%d,tid:%d,pid:%d,CODE:%x,RVA:%x,esp0:%x,ss0:%x,eip:%x,cs:%x,eflags:%x,"
				"ds:%x,es:%x,fs:%x,gs:%x,eax:%x,ecx:%x,edx:%x,ebx:%x,ebp:%x,esi:%x,edi:%x,cr0:%x,cr2:%x,cr3:%x,cr4:%x\r\n",
				num,tid, pid, tss->errcode,rva, tss->esp0, tss->ss0, tss->eip, tss->cs, tss->eflags,
				tss->ds, tss->es, tss->fs, tss->gs, tss->eax, tss->ecx, tss->edx, tss->ebx, tss->ebp, tss->esi, tss->edi,
				rcr0, rcr2, rcr3, rcr4);
		}
	}
	else {
		LPEXCEPTIONSTACK tss = (LPEXCEPTIONSTACK)param;

		if (tss->eflags & 0x20000)
		{
			DWORD rva = tss->eip;

			len = __sprintf(showinfo,
				"VM86 Exception:%d,tid:%d,pid:%d,RVA:%x,esp0:%x,ss0:%x,eip:%x,cs:%x,eflags:%x,esp3:%x,ss3:%x,"
				"ds:%x,es:%x,fs:%x,gs:%x,eax:%x,ecx:%x,edx:%x,ebx:%x,ebp:%x,esi:%x,edi:%x,ds_v86:%x,es_v86:%x,fs_v86:%x,gs_v86:%x,\
			cr0:%x,cr2:%x,cr3:%x,cr4:%x\r\n",
				num, tid, pid, rva, tss->esp0, tss->ss0, tss->eip, tss->cs, tss->eflags, tss->esp3, tss->ss3,
				tss->ds, tss->es, tss->fs, tss->gs, tss->eax, tss->ecx, tss->edx, tss->ebx, tss->ebp, tss->esi, tss->edi,
				tss->ds_v86, tss->es_v86, tss->fs_v86, tss->gs_v86, rcr0, rcr2, rcr3, rcr4);
		}
		else if ((level & 3) || (tss->cs & 3))
		{

			DWORD rva = rvaInFile(process->moduleaddr, tss->eip );

			len = __sprintf(showinfo,
				"Exception:%d,tid:%d,pid:%d,RVA:%x,esp0:%x,ss0:%x,eip:%x,cs:%x,eflags:%x,esp3:%x,ss3:%x,"
				"ds:%x,es:%x,fs:%x,gs:%x,eax:%x,ecx:%x,edx:%x,ebx:%x,ebp:%x,esi:%x,edi:%x,cr0:%x,cr2:%x,cr3:%x,cr4:%x\r\n",
				num,tid, pid, rva,tss->esp0, tss->ss0, tss->eip, tss->cs, tss->eflags, tss->esp3, tss->ss3,
				tss->ds, tss->es, tss->fs, tss->gs, tss->eax, tss->ecx, tss->edx, tss->ebx, tss->ebp, tss->esi, tss->edi,
				rcr0, rcr2, rcr3, rcr4);
		}
		else {
			DWORD rva = rvaInFile(process->moduleaddr, tss->eip );

			len = __sprintf(showinfo,
				"Exception:%d,tid:%d,pid:%d,RVA:%x,esp0:%x,ss0:%x,eip:%x,cs:%x,eflags:%x,"
				"ds:%x,es:%x,fs:%x,gs:%x,eax:%x,ecx:%x,edx:%x,ebx:%x,ebp:%x,esi:%x,edi:%x,cr0:%x,cr2:%x,cr3:%x,cr4:%x\r\n",
				num,tid, pid, rva, tss->esp0, tss->ss0, tss->eip, tss->cs, tss->eflags,
				tss->ds, tss->es, tss->fs, tss->gs, tss->eax, tss->ecx, tss->edx, tss->ebx, tss->ebp, tss->esi, tss->edi,
				rcr0, rcr2, rcr3, rcr4);
		}
	}

	__sprintf(showinfo + len, "Exception level:%d,status:%x,counter:%d,base:%x,sleep:%d,function:%s,file:%s\r\n",
		process->level, process->status, process->counter, process->moduleaddr, process->sleep, process->funcname, process->filename);

	if (g_ExceptionCounter++ < 4) {
		__drawGraphChars(( char*)showinfo, EXCEPTION_TIPS_COLOR);
	}
	
	if (pid == tid) {
		__terminateProcess(tid | 0x80000000, process->filename, process->funcname, 0);
	}
	else {
		__kTerminateThread(tid | 0x80000000, process->filename, process->funcname, 0);
	}

	__asm {
		hlt
	}
}