#include "process.h"
#include "def.h"
#include "video.h"
#include "Utils.h"
#include "task.h"
#include "Pe.h"
#include "file.h"
#include "processDOS.h"
#include "pevirtual.h"
#include "Kernel.h"
#include "memory.h"
#include "malloc.h"
#include "page.h"
#include "ListEntry.h"
#include "window.h"
#include "elf.h"
#include "apic.h"


void __kFreeProcess(int pid) {

	int cpu = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	freeProcessMemory(pid,cpu);

	freeProcessPages(pid,cpu);

	//DestroyProcessWindow(pid,cpu);
}


//1 先停止代码，然后释放内存，顺序不能反
//2 先停止其他线程，然后停止本线程，顺序不能反
//3 any thread of process can call this to terminate process resident in
// any process can call this to terminate self to other process with dwtid
//above so,the most import element is dwtid
#ifdef TASK_SWITCH_ARRAY
extern "C" __declspec(dllexport) void __terminateProcess(int dwtid, char* filename, char* funcname, DWORD lpparams) {

	int tid = dwtid & 0x7fffffff;

	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetTaskTssBase();

	LPPROCESS_INFO current = (LPPROCESS_INFO)GetCurrentTaskTssBase();

	int pid = tss[tid].pid;

	char szout[256];

	if (tid < 0 || tid >= TASK_LIMIT_TOTAL || tss[tid].tid != tid) {
		__printf(szout, "__terminateProcess tid:%x,pid:%x,current pid:%x,current tid:%x,filename:%s,funcname:%s\n",
			tid, pid, current->pid, current->tid, filename, funcname);
		return;
	}

	//__printf(szout, "__terminateProcess tid:%x,pid:%x,current pid:%x,current tid:%x,filename:%s,funcname:%s\n",tid, pid, current->pid, current->tid, filename, funcname);
	enter_task_array_lock();

	LPPROCESS_INFO process = 0;

	for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {

		if ((tss[i].status == TASK_RUN) && (tss[i].pid == pid))
		{
			if (tss[i].pid != tss[i].tid) {
				tss[i].status = TASK_OVER;
			}
			else {
				process = &tss[i];
			}
		}
	}
	
	tss[process->pid].status = TASK_OVER;

	if (current->tid == tid)
	{
		current->status = TASK_OVER;
	}

	if (current->pid == pid)
	{
		current->status = TASK_OVER;
	}

	int retvalue = 0;

	tss[process->pid].retValue = retvalue;

	__kFreeProcess(pid);

	leave_task_array_lock();

	__printf(szout, "pid:%d tid:%d file:%s function:%s terminate\r\n",pid,tid,filename,funcname);

	//tss[pid].window = 0;
	tss[pid].sleep = 0;

	if (dwtid & 0x80000000) {
		return;
	}
	else {
		__asm {
			//hlt
		}
		//__sleep(-1);

		__yield();
	}
}
#else
extern "C" __declspec(dllexport) void __terminateProcess(int dwtid, char* filename, char* funcname, DWORD lpparams) {
	
	int tid = dwtid & 0x7fffffff;

	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetTaskTssBase();

	int pid = tss[tid].pid;

	RemoveTaskListPid(pid);

	if (dwtid & 0x80000000) {
		return;
	}
	else {
		__sleep(-1);
	}
}
#endif



int __initProcess(LPPROCESS_INFO tss, int tid, DWORD filedata, char * filename, char * funcname,DWORD level, DWORD param) 
{
	int result = 0;

	char szout[256];

	tss->pid = tid;
	tss->tid = tid;
	tss->tss.cr3 = __kPageAlloc(PAGE_SIZE);
	__memset((char*)tss->tss.cr3, 0, PAGE_SIZE);
	copyKernelCR3(0, 0, (DWORD*)tss->tss.cr3);
	//tss->tss.cr3 = PDE_ENTRY_VALUE;

#ifdef SINGLE_TASK_TSS
	tss->tss.trap = 0;
#else
	tss->tss.trap = 0;
#endif
	tss->tss.trap = 0;
	
	tss->tss.ldt = 0;
	tss->fpu = TRUE;

	DWORD syslevel = level & 3;
	tss->level = syslevel;
	tss->copyMap = 0;

	DWORD eflags = 0x200202;	//if = 1,et = 1
	if (syslevel)
	{
		eflags |= (syslevel << 12);	//iopl = 3
	}
	//eflags |= 0x4000;		//nt == 1
	tss->tss.eflags = eflags;

	tss->tss.iomapOffset = 136;
	tss->tss.iomapEnd = 0xff;
	__memset((char*)tss->tss.intMap, 0, sizeof(tss->tss.intMap));
	__memset((char*)tss->tss.iomap, 0, sizeof(tss->tss.iomap));

	tss->vaddr = USER_SPACE_START;
	tss->vasize = 0;
	DWORD vaddr = tss->vaddr + tss->vasize;
	DWORD imagesize = getSizeOfImage((char*)filedata);
	DWORD alignsize = 0;
	DWORD pemap = (DWORD)__kProcessMalloc(imagesize,&alignsize, tss->pid,tss->cpuid, vaddr, PAGE_READWRITE | PAGE_USERPRIVILEGE | PAGE_PRESENT);
	if (pemap <= 0) {
		tss->status = TASK_OVER;
		__printf(szout, "%s %d ERROR\r\n",__FUNCTION__, __LINE__);
		return FALSE;
	}

	tss->moduleaddr = tss->vaddr + tss->vasize;
	tss->moduleLinearAddr = USER_SPACE_START;

	//__printf(szout, "membase:%x,va size:%x,va:%x\n",pemap,tss->vasize,tss->vaddr);

	mapFile((char*)filedata, (char*)pemap);

	DWORD entry = 0;
	DWORD type = getType((DWORD)pemap);
	if (type & 0x2000)	//dll
	{
		entry = getAddrFromName((DWORD)pemap, funcname);
		if (entry == FALSE) {
			__printf(szout, "%s %d ERROR\n", __FUNCTION__, __LINE__);
			__kFree(pemap);
			tss->status = TASK_OVER;
			return FALSE;
		}
		else {
			//getAddrFromName 已经加上了pemap，所以必须减去它
			entry = entry - pemap + USER_SPACE_START;
		}
	}
	else {
		entry = getEntry((char*)pemap) + USER_SPACE_START;
	}

#ifdef DISABLE_PAGE_MAPPING
	tss->tss.eip = entry - USER_SPACE_START + pemap;
	relocTableV((char*)pemap, pemap);
	importTable((DWORD)pemap);
	setImageBaseV((char*)pemap, pemap);
#else
	tss->tss.eip = entry;
	result = relocTableV((char*)pemap, USER_SPACE_START);
	result = importTable((DWORD)pemap);
	if (result == 0) {
		__printf(szout, "%s %s %s %d ERROR\n", __FUNCTION__, funcname, filename, __LINE__);
		__kFree(pemap);
		tss->status = TASK_OVER;
		return FALSE;
	}
	setImageBaseV((char*)pemap, USER_SPACE_START);
#endif

	tss->tss.eax = 0;
	tss->tss.ecx = 0;
	tss->tss.edx = 0;
	tss->tss.ebx = 0;
	tss->tss.esi = 0;
	tss->tss.edi = 0;

	tss->tss.esp0 = TASKS_STACK0_BASE + (TASK_LIMIT_TOTAL*tss->cpuid + tid + 1) * TASK_STACK0_SIZE - STACK_TOP_DUMMY;
	tss->tss.ss0 = KERNEL_MODE_STACK;

	vaddr = tss->vaddr + tss->vasize;
	DWORD espsize = 0;
	LPTASKPARAMS params = 0;
	DWORD heapsize = 0;
	if (syslevel == 0)
	{
		tss->tss.ds = KERNEL_MODE_DATA;
		tss->tss.es = KERNEL_MODE_DATA;
		tss->tss.fs = KERNEL_MODE_DATA;
		tss->tss.gs = KERNEL_MODE_DATA;
		tss->tss.cs = KERNEL_MODE_CODE;
		tss->tss.ss = KERNEL_MODE_STACK;

		tss->espbase = __kProcessMalloc(KTASK_STACK_SIZE, &espsize, tss->pid,tss->cpuid, vaddr, PAGE_READWRITE | PAGE_USERPRIVILEGE | PAGE_PRESENT);
		if (tss->espbase == FALSE)
		{
			__printf(szout, "%s %d ERROR\n", __FUNCTION__, __LINE__);
			__kFreeProcess(tss->pid);
			tss->status = TASK_OVER;
			return FALSE;
		}
#ifndef DISABLE_PAGE_MAPPING
		tss->tss.esp = (DWORD)vaddr + KTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
		tss->tss.ebp = (DWORD)vaddr + KTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
#else
		tss->tss.esp = (DWORD)tss->espbase + KTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
		tss->tss.ebp = (DWORD)tss->espbase + KTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
#endif
		params = (LPTASKPARAMS)(tss->espbase + KTASK_STACK_SIZE  - STACK_TOP_DUMMY - sizeof(TASKPARAMS));

#ifdef SINGLE_TASK_TSS
		RETUTN_ADDRESS_0* ret0 = (RETUTN_ADDRESS_0*)((char*)params - sizeof(RETUTN_ADDRESS_0));
		ret0->cs = tss->tss.cs;
		ret0->eip = tss->tss.eip;
		ret0->eflags = tss->tss.eflags;
		tss->tss.esp = (DWORD)ret0;
		tss->tss.ebp = (DWORD)ret0;
#else
		tss->tss.esp = (DWORD)tss->espbase + KTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
		tss->tss.ebp = (DWORD)tss->espbase + KTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
#endif
		heapsize = KTASK_STACK_SIZE;
	}
	else {
		tss->tss.ds = USER_MODE_DATA | syslevel ;
		tss->tss.es = USER_MODE_DATA | syslevel;
		tss->tss.fs = USER_MODE_DATA | syslevel ;
		tss->tss.gs = USER_MODE_DATA | syslevel ;
		tss->tss.cs = USER_MODE_CODE | syslevel ;
		tss->tss.ss = USER_MODE_STACK | syslevel ;

		tss->espbase = __kProcessMalloc(UTASK_STACK_SIZE,&espsize, tss->pid,tss->cpuid, vaddr, PAGE_READWRITE | PAGE_USERPRIVILEGE | PAGE_PRESENT);
		if (tss->espbase == FALSE)
		{
			__printf(szout, "%s %d ERROR\n", __FUNCTION__, __LINE__);
			__kFreeProcess(tss->pid);
			tss->status = TASK_OVER;
			return FALSE;
		}
#ifndef DISABLE_PAGE_MAPPING
		tss->tss.esp = (DWORD)vaddr + UTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
		tss->tss.ebp = (DWORD)vaddr + UTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
#else
		tss->tss.esp = (DWORD)tss->espbase + UTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
		tss->tss.ebp = (DWORD)tss->espbase + UTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
#endif
		params = (LPTASKPARAMS)(tss->espbase + UTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS));

#ifdef SINGLE_TASK_TSS
		tss->tss.esp = (DWORD)tss->espbase + UTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
		tss->tss.ebp = (DWORD)tss->espbase + UTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);

		RETUTN_ADDRESS_3* ret3 = (RETUTN_ADDRESS_3*)((char*)tss->tss.esp0 - sizeof(RETUTN_ADDRESS_3));
		ret3->ret0.cs = tss->tss.cs;
		ret3->ret0.eip = tss->tss.eip;
		ret3->ret0.eflags = tss->tss.eflags;
		ret3->esp3 = (DWORD)tss->tss.esp;
		ret3->ss3 = tss->tss.ss;

		tss->tss.esp = (DWORD)ret3;
		tss->tss.ebp = (DWORD)ret3;
		tss->tss.ss = KERNEL_MODE_STACK;
#else
		tss->tss.esp = (DWORD)tss->espbase + UTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
		tss->tss.ebp = (DWORD)tss->espbase + UTASK_STACK_SIZE - STACK_TOP_DUMMY - sizeof(TASKPARAMS);
#endif

		heapsize = UTASK_STACK_SIZE;
	}
	
	vaddr = tss->vaddr + tss->vasize;
	DWORD heapbase = __kProcessMalloc(heapsize, &heapsize, tss->pid,tss->cpuid, vaddr, PAGE_READWRITE | PAGE_USERPRIVILEGE | PAGE_PRESENT);
#ifndef DISABLE_PAGE_MAPPING
	tss->heapbase = vaddr;
#else
	tss->heapbase = heapbase;
#endif
	tss->heapsize = heapsize;
	
	DWORD funTerminate = (DWORD)getAddrFromName(KERNEL_DLL_BASE, "__terminateProcess");
	params->terminate = (DWORD)funTerminate;
	params->terminate2 = (DWORD)funTerminate;
	params->tid = tid;
	__strcpy(params->szFileName, filename);
	params->filename = params->szFileName;
	__strcpy(params->szFuncName, funcname);
	params->funcname = params->szFuncName;
	params->lpcmdparams = &params->cmdparams;
	if (param)
	{
		__memcpy((char*)params->lpcmdparams, (char*)param, sizeof(TASKCMDPARAMS));
	}
	else {
		params->lpcmdparams = 0;
	}

	tss->counter = 0;
	tss->errorno = 0;

	tss->pid = tid;
	tss->tid = tid;
	__strcpy(tss->filename, filename);
	__strcpy(tss->funcname, funcname);

	tss->showX = 0;
	tss->showY = 0;
	tss->window = 0;
	tss->videoBase = (char*)gGraphBase;

	LPPROCESS_INFO thistss = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	tss->ppid = thistss->pid;
	tss->sleep = 0;

	//__printf(szout, "imagebase:%x,imagesize:%x,map base:%x,entry:%x,cr3:%x,esp:%x,cpu:%d,pid:%d,tid:%d\n",getImageBase((char*)pemap), imagesize, pemap, entry, tss->tss.cr3,tss->espbase,tss->cpuid,tss->pid,tss->tid);

	//enter_task_array_lock();

	tss->status = TASK_RUN;

	//leave_task_array_lock();

#ifdef TASK_SWITCH_ARRAY

#else
	InsertTaskList(tss->tid);
#endif

	return TRUE;
}


int __kCreateProcessFromName(char * filename, char * funcname, int syslevel, DWORD params) {
	int result = 0;
	char szout[256];
	if (filename == 0 || funcname == 0)
	{
		__printf(szout, "%s %d error\n", __FUNCTION__, __LINE__);
		return FALSE;
	}

	int filesize = 0;
	char * filedata = 0;
	filesize = readFile(filename, (char**)&filedata);
	if (filesize <= 0)
	{
		__printf(szout, "%s %d error\n", __FUNCTION__, __LINE__);
		return FALSE;
	}

	result = __kCreateProcess((DWORD)filedata, filesize,filename,funcname, syslevel, params);

	__kFree((DWORD)filedata);

	return TRUE;
}


int __kCreateProcessFromAddrFunc(DWORD filedata, int filesize,char * funcname,int syslevel, DWORD params) {
 	char filename[1024];
// 	__getDateTimeStr(filename);

	__sprintf(filename, "process_%x", *((unsigned int*)TIMER0_TICK_COUNT));

	return __kCreateProcess(filedata, filesize, filename, funcname, syslevel, params);
}


int __kCreateProcess(DWORD filedata, int filesize,char * filename,char * funcname, int syslevel, DWORD params) {

	int ret = 0;
	char szout[256];

 	//DWORD filedata = linear2phy(lpfiledata);
 	//char * filename = (char *)linear2phy((DWORD)fn);
 	//char * funcname = (char *)linear2phy((DWORD)funname);
 	//DWORD params = linear2phy(lpparams);

	//__printf(szout, "%s base:%x size:%x module:%s addr:%p function:%s addr:%p\r\n", __FUNCTION__,filedata,filesize, filename, filename, funcname, funcname);

	int mode = syslevel & 0xfffffffc;
	DWORD level = syslevel & 3;

	int cpu = *(int*)(LOCAL_APIC_BASE + 0x20) >> 24;

	TASKRESULT result;
	ret = __getFreeTask(&result);
	if (ret == FALSE)
	{
		__printf(szout, "%s %d error\n",__FUNCTION__, __LINE__);
		return FALSE;
	}

	if (mode & DOS_PROCESS_RUNCODE)
	{
		ret = __initDosTss(result.lptss, result.number, filedata, filename, funcname, mode + (level | 3), params);
		return ret;
	}

	int petype = getPeType(filedata);
	if (petype == DOS_EXE_FILE || petype == DOS_COM_FILE)
	{
		if (filesize == 0)
		{
			return FALSE;
		}
		else {
			DWORD dosaddr = __allocVm86Addr(filename,petype,filedata, filesize, result.number);
			if (dosaddr)
			{
				ret = __initDosTss(result.lptss, result.number, dosaddr, filename, funcname, 3, params);
				return ret;
			}
			else {
				__printf(szout, "%s %d error\n", __FUNCTION__, __LINE__);
				return FALSE;
			}
		}
	}
	else if (petype == 2)
	{
		DWORD type = getType((DWORD)filedata);
		if (type & 0x2000)
		{
			if (funcname == 0)
			{
				__printf(szout, "%s %d error\n", __FUNCTION__, __LINE__);
				return FALSE;
			}
		}
		ret = __initProcess(result.lptss, result.number, filedata, filename, funcname, level, params);
		return ret;
	}
	else if (petype == 3)
	{
		return runElfFunction(filename, funcname);
	}
	else {
		ret = FALSE;
	}

	return ret;
}


