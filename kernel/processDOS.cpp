#include "processDOS.h"
#include "Pe.h"
#include "Utils.h"
#include "Kernel.h"
#include "task.h"
#include "process.h"

//配置SCSI控制卡支持INT31H
//要想从扩充内存中获得16K页，可以使用Int 67h来调用EMM，并告诉它将该页放进页框架之中
//INT 18H - ROM BASIC 启动rom basic解释器
//执行磁带上的 BASIC 程序：“真正的”IBM 兼容机在 ROM 里内置 BASIC 程序，当引导失败时由 BIOS 调用此例程解释执行。
//（例：打印“Boot disk error. Replace disk and press any key to continue...”这类提示信息）
//INT 1Fh不可调用：指向视频图形字符表（包含从 80h 到 FFh 的 ASCII 字符的数据）的信息。
//INT 1Dh不可调用：指向视频参数表（包含视频模式的数据）的指针。
//INT 1Eh不可调用：指向软盘模式表（包含关于软驱的大量信息）的指针。
//INT 41h地址指针：硬盘参数表（第一硬盘）。
//INT 46h地址指针：硬盘参数表（第二硬盘）。
//INT 4Ah实时钟在闹铃时调用。
//int 19h 会将MBR的512字节装载到内存0x7c00中，然后JUMP到0x7c00处，开始执行MBR的可执行程序（master booter）


DOS_PE_CONTROL g_v86ControlBloack[LIMIT_V86_PROC_COUNT] = { 0 };


void V86ProcessCheck(LIGHT_ENVIRONMENT* env, LPPROCESS_INFO prev, LPPROCESS_INFO proc) {
	char szout[1024];
	if ((env->eflags & 0x20000) && prev->level == 3 && proc->level == 3) {
		DWORD reip = (WORD)prev->tss.eip;
		DWORD rcs = (WORD)prev->tss.cs;
		WORD code = *(WORD*)((rcs << 4) + reip + 2);
		WORD code2 = *(WORD*)((rcs << 4) + reip );
		if (code == 0xfeeb && code2 == 0xfeeb) {

			LPDOS_PE_CONTROL info = (LPDOS_PE_CONTROL)g_v86ControlBloack;
			for (int i = 0; i < LIMIT_V86_PROC_COUNT; i++)
			{
				if (info[i].pid == prev->pid)
				{
					proc->status = TASK_OVER;
					prev->status = TASK_OVER;
					info[i].status = TASK_OVER;
					__printf(szout,"kill dos program:%s,pid:%d\r\n",info[i].name, info[i].pid);
					break;
				}
			}
		}
	}
}


int getVm86ProcAddr(char * fn,int type, DWORD filedata, int size, int pid) {

	LPDOS_PE_CONTROL info = (LPDOS_PE_CONTROL)g_v86ControlBloack;
	for (int i = 0; i < LIMIT_V86_PROC_COUNT; i++)
	{
		if (info[i].status == TASK_OVER)
		{
			info[i].status = TASK_RUN;

			info[i].pid = pid;

			info[i].size = size;

			__strcpy(info[i].name, fn);

			if (type == DOS_EXE_FILE)
			{
				info[i].address = i * 0x1000 + DOS_LOAD_FIRST_SEG + 0x10;
			}
			else
			{
				info[i].address = i * 0x1000 + DOS_LOAD_FIRST_SEG + 0x10;
			}

			return info[i].address;
		}
	}


	return 0;
}

//dosfile must be align with 16b
int relocDos(DWORD loadseg) {
	IMAGE_DOS_HEADER* hdr = (IMAGE_DOS_HEADER*)(loadseg << 4);
	int dosseg = hdr->e_cparhdr + loadseg;
	hdr->e_cs = hdr->e_cs + dosseg;
	hdr->e_ss = hdr->e_ss + dosseg;

	int relocoff = hdr->e_lfarlc;
	unsigned short* relocs = (unsigned short*)((loadseg << 4) + relocoff);
	for (int i = 0; i < hdr->e_crlc; i++)
	{
		unsigned short relocseg = *(relocs + 1);
		unsigned short relocoff = *relocs;

		unsigned short* relocaddr = (unsigned short*)(((relocseg + dosseg) << 4) + relocoff);

		*relocaddr = (*relocaddr + dosseg);

		relocs += 2;
	}

	return hdr->e_crlc;
}


DWORD __allocVm86Addr(char * fn,int type, DWORD filedata, int filesize, int pid) {
	int ret = 0;

	DWORD seg = getVm86ProcAddr(fn,type, filedata, filesize, pid);
	if (seg >= INT13_RM_FILEBUF_SEG || seg <= 0)
	{
		return FALSE;
	}

	WORD offset = 0;

	DWORD dosaddr = (seg << 4) + offset;

	__memcpy((char*)dosaddr, (char*)filedata, filesize);

	if (type == DOS_EXE_FILE)
	{
		ret = relocDos(seg);
	}
	else {

	}

	return dosaddr;
}



int __createDosCodeProc(DWORD addr, int size, char* filename) {
	if (__findProcessFileName(filename))
	{
		return 0;
	}
	return __kCreateProcess(addr, size, filename, filename, DOS_PROCESS_RUNCODE | 3, 0);
}


int __initDosTss(LPPROCESS_INFO tss, int pid, DWORD addr, char * filename, char * funcname, DWORD level, DWORD runparam) {

	int ret = 0;

	char szout[1024];

	if ((level & 3) == 0)
	{
		return FALSE;
	}
	
	tss->tss.iomapOffset = 136;
	tss->tss.iomapEnd = 0xff;
	__memset((char*)tss->tss.intMap, 0, sizeof(tss->tss.intMap));
	__memset((char*)tss->tss.iomap, 0, sizeof(tss->tss.iomap));

	//由于是单处理器，所以每个进程装入的时候必须打开中断位，否则一个进程一旦独占了唯一的一个cpu会导致无法中断
	DWORD eflags = 0x220202;
	eflags = eflags | ((level&3) << 12);
	//eflags |= 0x4000;		//nt == 1

	WORD seg = (unsigned short)(addr >> 4);

	WORD offset = (addr & 0x0f);

	tss->sleep = 0;
	tss->copyMap = 0;
	tss->fpu = TRUE;
#ifdef SINGLE_TASK_TSS
	tss->tss.trap = 1;
#else
	tss->tss.trap = 0;
#endif

	tss->tss.esp0 = TASKS_STACK0_BASE + (pid + 1) * TASK_STACK0_SIZE - STACK_TOP_DUMMY;
	tss->tss.ss0 = KERNEL_MODE_STACK;

	tss->tss.ldt = 0;

	tss->tss.eflags = eflags;

	tss->tss.eax = 0;
	tss->tss.ecx = 0;
	tss->tss.edx = 0;
	tss->tss.ebx = 0;
	tss->tss.ebp = 0;
	tss->tss.esi = 0;
	tss->tss.edi = 0;

	if (level & DOS_PROCESS_RUNCODE)
	{
		tss->tss.eip = offset;
		tss->tss.cs = seg;

		tss->tss.ss = KERNEL_BASE_SEGMENT;
		tss->tss.esp = V86_STACK_SIZE - STACK_TOP_DUMMY;

		tss->tss.ds = seg;
		tss->tss.es = seg;
		tss->tss.fs = seg;
		tss->tss.gs = seg;

		/*
		tss->tss.esp = tss->tss.esp - sizeof(TASKDOSPARAMS);
		LPTASKDOSPARAMS params = (LPTASKDOSPARAMS)(tss->tss.esp + (tss->tss.ss << 4));
		params->terminate = (DWORD)0;
		params->pid = pid;						//param1:pid
		__strcpy(params->szFileName, filename);
		params->filename = params->szFileName;		//param2:filename
		__strcpy(params->szFuncName, funcname);
		params->funcname = params->szFuncName;		//param2:filename
		*/

		//__printf(szout, "__kCreateTask in file dos file:%s\r\n", filename);
		//__drawGraphChars((unsigned char*)szout, 0);
	}
	else if (__memcmp((char*)addr, "MZ", 2) == 0)
	{
		IMAGE_DOS_HEADER * dos = (IMAGE_DOS_HEADER*)addr;

		tss->tss.eip = dos->e_ip;
		tss->tss.cs = dos->e_cs;

		tss->tss.esp = dos->e_sp;
		tss->tss.ss = dos->e_ss;

		//DWORD dospsp = seg + dos->e_cparhdr - 0x10;
		DWORD dospsp = seg + dos->e_cparhdr;

		tss->tss.ds = dospsp;
		tss->tss.es = dospsp;
		tss->tss.fs = dospsp;
		tss->tss.gs = dospsp;

		//__printf(szout, "__kCreateTask dos exe file:%s\r\n", filename);
		//__drawGraphChars((unsigned char*)szout, 0);
	}
	else {
		tss->tss.eip = 0x100;

		tss->tss.cs = seg - 0x10;

		tss->tss.esp = V86_STACK_SIZE - 2;
		tss->tss.ss = tss->tss.cs;

		tss->tss.ds = tss->tss.cs;
		tss->tss.es = tss->tss.cs;
		tss->tss.fs = tss->tss.cs;
		tss->tss.gs = tss->tss.cs;
		tss->tss.ecx = 0xfff;

		//*(WORD*)(tss->tss.esp + addr) = 0;

		__printf(szout, "__kCreateTask dos com file:%s\r\n\r\n", filename);

	}

#ifdef SINGLE_TASK_TSS
	RETUTN_ADDRESS_V86* retv86 = (RETUTN_ADDRESS_V86*)((char*)tss->tss.esp0 - sizeof(RETUTN_ADDRESS_V86));
	retv86->ret3.ret0.cs = tss->tss.cs;
	retv86->ret3.ret0.eip = tss->tss.eip;
	retv86->ret3.ret0.eflags = tss->tss.eflags;
	retv86->ret3.esp3 = (DWORD)tss->tss.esp;
	retv86->ret3.ss3 = tss->tss.ss;
	retv86->ds = tss->tss.ds;
	retv86->es = tss->tss.es;
	retv86->fs = tss->tss.fs;
	retv86->gs = tss->tss.gs;

	tss->tss.ds = KERNEL_MODE_STACK;
	tss->tss.es = KERNEL_MODE_STACK;
	tss->tss.fs = KERNEL_MODE_STACK;
	tss->tss.gs = KERNEL_MODE_STACK;

	tss->tss.esp = (DWORD)retv86;
	tss->tss.ss = KERNEL_MODE_STACK;
#else
#endif

	tss->tss.cr3 = PDE_ENTRY_VALUE;

	tss->pid = pid;
	tss->tid = pid;

	tss->vasize = 0;
	tss->vaddr = addr;

	tss->level = level & 3;

	tss->showX = 0;
	tss->showY = 32;
	tss->window = 0;
	tss->videoBase = (char*)gGraphBase;

	tss->counter = 0;
	
	tss->moduleaddr = addr;
	tss->moduleLinearAddr = addr;

	__strcpy(tss->filename, filename);

	__strcpy(tss->funcname, funcname);

	//addTaskList(tss->tid);
	tss->status = TASK_RUN;
	return TRUE;
}