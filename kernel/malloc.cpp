
#include "malloc.h"
#include "def.h"
#include "Utils.h"
#include "video.h"
#include "descriptor.h"
#include "page.h"
#include "process.h"
#include "task.h"
#include "memory.h"
#include "heap.h"
#include "apic.h"

QWORD gAvailableSize = 0;

QWORD gAvailableBase = 0;

QWORD gAllocLimitSize = 0;

int gMemAllocLock = FALSE;

LPMEMALLOCINFO gMemAllocList = 0;


QWORD getBorderAddr() {
	return gAvailableBase + gAvailableSize;
}

int getAlignSize(int size, int allignsize) {
	int allocsize = size;
	int mod = size % allignsize;
	if (mod)
	{
		allocsize = allocsize + (allignsize - mod);
	}
	return allocsize;
}


//Bit Scan Forward
//格式: BSF dest, src
//影响标志位 : ZF
//功能：从源操作数的的最低位向高位搜索，将遇到的第一个“1”所在的位序号存入目标寄存器中，若所有位都是0，则ZF = 1，否则ZF = 0。

//Bit Scan Reverse
//BSR dest, src
//影响标志位 : ZF
//功能：从源操作数的的最高位向低位搜索，将遇到的第一个“1”所在的位序号存入目标寄存器中，若所有位都是0，则ZF = 1，否则ZF = 0。

//BTS指令
//格式: BTS OPD, OPS
//功能 :  源操作数OPS指定的位送CF标志, 目的操作数OPD中那一位置位.


//direction 1:upward 0:downward
DWORD pageAlignSize(DWORD blocksize,int direction)
{
	DWORD result = 0;

	__asm {
		xor eax, eax
		mov ecx,dword ptr blocksize
		bsr eax,ecx
		jz _Over
		
		xor edx, edx
		bts edx,eax

		cmp edx, blocksize
		jz _minimumSize
		cmp direction,0
		jz _minimumSize

		shl edx, 1
		_minimumSize:
		mov result, edx
		_Over :
	}

	return result;
}


int initMemory() {
	char szout[256];

	ClearMemAllocMap();
	
	int cnt = *(int*)MEMORYINFO_LOAD_ADDRESS;
	if (cnt <= 0)
	{
		return FALSE;
	}

	ADDRESS_RANGE_DESCRIPTOR_STRUCTURE * ards = (ADDRESS_RANGE_DESCRIPTOR_STRUCTURE*)(MEMORYINFO_LOAD_ADDRESS + sizeof(int));
	for ( int i = 0;i < cnt ;i ++)
	{
// 		int len = __printf(szout, "Memory address:%I64d,length:%I64d,type:%x\n",
// 			ards->BaseAddrLow, ards->BaseAddrHigh, ards->LengthLow, ards->LengthHigh, ards->Type);
		
		if (ards->Type == 1 )
		{
			__int64 low = ards->BaseAddrLow;
			__int64 high = ards->BaseAddrHigh;
			__int64 b = (high << 32) + low;
			if (b == 0x100000)	//0x100000 is the extended mem base
			{
				__int64 sl = ards->LengthLow;
				__int64 sh = ards->LengthHigh;
				__int64 s = (sh << 32) + sl;

				gAvailableBase = (QWORD)b;
				gAvailableSize = (QWORD)s;

				if (gAvailableBase > MEMMORY_ALLOC_BASE )
				{
					gAllocLimitSize = (gAvailableSize - gAvailableBase) / 1;
				}
				else {
					gAllocLimitSize = (gAvailableSize - MEMMORY_ALLOC_BASE) / 1;
				}
				
				//gAllocLimitSize = pageAlignmentSize(gAllocLimitSize, 0);

				int len = __printf(szout, "available memory address:%x,size:%x,alloc limit size:%x\n",
					gAvailableBase,gAvailableSize, gAllocLimitSize);
			}
		}

		ards++;
	}

	return 0;
}


LPMEMALLOCINFO isAddrExist(DWORD addr,int size) {

	LPMEMALLOCINFO info = (LPMEMALLOCINFO)gMemAllocList->list.next;

	LPMEMALLOCINFO base = info;

	do
	{
		if (info == 0)
		{
			return (LPMEMALLOCINFO)0;
		}	
		else if (info->addr == addr) 
		{
			return info;
		}
		else if ( (info->addr < addr) && ( info->addr + info->size > addr) )
		{
			return info;
		}
		else {
			info = (LPMEMALLOCINFO)info->list.next;
		}
	} while (info != base);

	return 0;
}

LPMEMALLOCINFO findAddr(DWORD addr) {

	LPMEMALLOCINFO info = (LPMEMALLOCINFO)gMemAllocList->list.next;

	LPMEMALLOCINFO base = info;

	do
	{
		if (info == 0)
		{
			return (LPMEMALLOCINFO)0;
		}
		else if (info->addr == addr)
		{
			return info;
		}
		else {
			info = (LPMEMALLOCINFO)info->list.next;
		}
	} while (info != base);

	return 0;
}



void ClearMemAllocMap() {
	
	LPMEMALLOCINFO item = (LPMEMALLOCINFO)MEMORY_ALLOC_BUFLIST;
	int cnt = MEMORY_ALLOC_BUFLIST_SIZE / sizeof(MEMALLOCINFO);
	for (int i = 0; i < cnt; i++)
	{
		__memset((char*)&item[i], 0, sizeof(MEMALLOCINFO));
	}

	gMemAllocList = (LPMEMALLOCINFO)MEMORY_ALLOC_BUFLIST;
	InitListEntry(&gMemAllocList->list);
}


int ClearMemAllocItem(LPMEMALLOCINFO item) {
	DWORD size = item->size;

	RemoveList(&gMemAllocList->list,(LPLIST_ENTRY)&item->list);
	item->addr = 0;
	item->size = 0;
	item->vaddr = 0;
	item->pid = 0;

	return size;
}

LPMEMALLOCINFO GetEmptyMemAllocItem() {
	LPMEMALLOCINFO item = (LPMEMALLOCINFO)(MEMORY_ALLOC_BUFLIST);

	int cnt = MEMORY_ALLOC_BUFLIST_SIZE / sizeof(MEMALLOCINFO) ;
	for ( int i = 1;i < cnt;i ++)
	{
		if ( item[i].size == 0 && item[i].addr == 0 && item[i].vaddr == 0 && item[i].pid == 0)
		{
			return &item[i];
		}
	}
	return 0;
}


int SetMemAllocItem(LPMEMALLOCINFO item,DWORD addr,DWORD vaddr,int size,int pid,int cpu) {
	if (vaddr)
	{
		item->vaddr = vaddr;
	}
	else {
		item->vaddr = addr;
	}
	item->pid = pid;
	item->size = size;
	item->addr = addr;
	item->cpu = cpu;

	InsertListTail(& (gMemAllocList->list), & item->list);
	return 0;
}


DWORD __kProcessMalloc(DWORD s,DWORD *outSize, int pid,int cpu,DWORD vaddr,int tag) {

	DWORD res = 0;

	char szout[256];

	DWORD size = pageAlignSize(s, 1);
	if ( size > gAllocLimitSize)
	{
		__printf(szout, "__kProcessMalloc pageAlignmentSize:%x gAllocLimitSize:%x\r\n", size, gAllocLimitSize);
		return FALSE;
	}
	else if (size < PAGE_SIZE)
	{
		size = PAGE_SIZE;
	}

	*outSize = size;

	__enterSpinlock(&gMemAllocLock);

	int factor = 1;

	do
	{
		for (int n = factor/2 ; n && n < factor; )
		{
			DWORD addr = MEMMORY_ALLOC_BASE + size * n;
			if ( addr + size > gAvailableBase + gAvailableSize)
			{
				res = -1;
				__printf(szout, "__kProcessMalloc addr:%x, size:%x exceed available addr:%x,size:%x\r\n", 
					addr, size, gAvailableBase, gAvailableSize);
				break;
			}

			LPMEMALLOCINFO info = isAddrExist(addr, size);
			if (info == 0)
			{
				info = GetEmptyMemAllocItem();
				if (info)
				{
					SetMemAllocItem(info, addr, vaddr, size, pid,cpu);

					res = addr;
					break;
				}
				else {
					__printf(szout, "getMemAllocInfo failed\r\n");
					res = -1;
					break;
				}
			}
			else {
				if (info->size > size) {
					int t = info->size / size;

					if (info->size % size) {
						__printf(szout, "isAddrExist size:%x size:%x error\r\n",info->size,size);
						t++;
					}

					n += t;

					while (n >= factor) {
						factor = factor << 1;
					}

					continue;
				}
				else {
					//
				}
			}

			n++;
		}

		if (res) {
			break;
		}

		factor = (factor << 1);
		
	} while (res == 0);

	if (res == -1) {
		res = 0;
	}

#ifndef DISABLE_PAGE_MAPPING
	if (res ) {		
		int vmtag = TRUE;
		if (vaddr == 0) {
			vaddr = res;
			vmtag = 0;
		}
		
		LPPROCESS_INFO lptss = GetTaskTssBaseId(cpu);
		LPPROCESS_INFO tss = (LPPROCESS_INFO)lptss + pid;
		LPPROCESS_INFO process = (LPPROCESS_INFO)GetCurrentTaskTssBase();

		enter_task_array_lock_id(cpu);

		if (process->pid == pid && process->cpuid == cpu)
		{
			if (vmtag) {
				vaddr = process->vaddr + process->vasize;
				process->vasize += size;
			}

			DWORD* cr3 = (DWORD*)process->tss.cr3;
			DWORD pagecnt = mapPhyToLinear(vaddr, res, size, cr3,tag);
		}
		else {
			if (vmtag) {
				vaddr = tss->vaddr + tss->vasize;
				tss->vasize += size;
			}

			DWORD* cr3 = (DWORD*)tss->tss.cr3;
			DWORD pagecnt = mapPhyToLinear(vaddr, res, size, cr3, tag);
		}
		
		leave_task_array_lock_id(cpu);
	}
#endif

	__leaveSpinlock(&gMemAllocLock);

	return res;
}

//R/W--位1是读/写（Read/Write）标志。如果等于1，表示页面可以被读、写或执行。如果为0，表示页面只读或可执行。
//当处理器运行在超级用户特权级（级别0、1或2）时，则R/W位不起作用。页目录项中的R/W位对其所映射的所有页面起作用。
//U/S--位2是用户/超级用户（User / Supervisor）标志。如果为1，那么运行在任何特权级上的程序都可以访问该页面。
//如果为0，那么页面只能被运行在超级用户特权级（0、1或2）上的程序访问。页目录项中的U / S位对其所映射的所有页面起作用。


DWORD __kMalloc(DWORD s) {

	char szout[256];
	DWORD size = 0;
	int len = 0;
	LPPROCESS_INFO process = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	DWORD ret = __kProcessMalloc(s, &size, process->pid, process->cpuid, 0,PAGE_READWRITE | PAGE_USERPRIVILEGE | PAGE_PRESENT);
	if (ret == 0) {	
		len = __printf(szout, "__kMalloc size:%x realSize:%x pid:%d error\n",s,size,process->pid);
	}
	else {
		//len = __printf(szout, "__kMalloc size:%x realSize:%x pid:%d addr:%x\n", s,size, process->pid,ret);
	}
	return ret;
}



int __kFree(DWORD physicalAddr) {

	char szout[256];
	__enterSpinlock(&gMemAllocLock);

	LPMEMALLOCINFO info = findAddr(physicalAddr);
	if (info)
	{
		//int len = __printf(szout, "__kFree address:%x size:%x pid:%d vaddr:%x\n", physicalAddr, info->size, info->pid, info->vaddr);
		DWORD size = ClearMemAllocItem(info);	
	}
	else {	
		int len = __printf(szout, "__kFree not found address:%x\n", physicalAddr);
	}

	__leaveSpinlock(&gMemAllocLock);

	return FALSE;
}

//return virtual address
DWORD __malloc(DWORD s) {
	DWORD res = 0;
	if (s <= HEAP_SIZE/2)
	{
		res = __heapAlloc(s);
		if (res) {
			return res;
		}
		else {

		}
	}

	char szout[256];

	DWORD size = 0;
	LPPROCESS_INFO process = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	DWORD vaddr = process->vaddr + process->vasize;
	res = __kProcessMalloc(s,&size, process->pid,process->cpuid,vaddr, PAGE_READWRITE | PAGE_USERPRIVILEGE | PAGE_PRESENT);
	if (res)
	{
		if (vaddr >= USER_SPACE_END)
		{
			__kFree(res);
			return FALSE;
		}

		DWORD * cr3 = (DWORD *)process->tss.cr3;
		DWORD pagecnt = mapPhyToLinear(vaddr, res, size, cr3, PAGE_READWRITE | PAGE_USERPRIVILEGE | PAGE_PRESENT);
		if (pagecnt)
		{
			return vaddr;
		}
	}

	int len = __printf(szout, "__malloc size:%x error\n", size);

	return FALSE;
}



int __free(DWORD linearAddr) {

	LPPROCESS_INFO process = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	if (linearAddr >= process->heapbase + process->heapsize)
	{
		return __heapFree(linearAddr);
	}

	__enterSpinlock(&gMemAllocLock);

	DWORD phyaddr = linear2phy(linearAddr);
	if (phyaddr)
	{
		LPMEMALLOCINFO info = findAddr(phyaddr);
		if (info)
		{
			DWORD size = ClearMemAllocItem(info);
		}
		else {
			char szout[256];
			int len = __printf(szout, "__free not found linear address:%x,physical address:%x\n", linearAddr, phyaddr);
		}
	}

	__leaveSpinlock(&gMemAllocLock);

	return FALSE;
}



//make sure the first in the list is not to be deleted,or else will be locked
void freeProcessMemory(int pid,int cpu) {

	__enterSpinlock(&gMemAllocLock);

	LPMEMALLOCINFO base = (LPMEMALLOCINFO)gMemAllocList->list.next;
	LPMEMALLOCINFO info =(LPMEMALLOCINFO) base;
	do
	{
		if (info == 0)
		{
			break;
		}
		else if (info->pid == pid && info->cpu == cpu)
		{
			ClearMemAllocItem(info);
		}

		info = (LPMEMALLOCINFO)info->list.next;

	} while (info != (LPMEMALLOCINFO)base);

	__leaveSpinlock(&gMemAllocLock);
}




unsigned char* __slab_malloc(int size) {
	return 0;
}


int getProcMemory(int pid,int cpu, char* szout) {
	int offset = 0;

	LPPROCESS_INFO processes = (LPPROCESS_INFO) GetTaskTssBase();
	LPPROCESS_INFO tss = processes + pid;
	if (tss->status != TASK_RUN)
	{
		return FALSE;
	}

	LPMEMALLOCINFO base = (LPMEMALLOCINFO)gMemAllocList->list.next;
	LPMEMALLOCINFO info = base;
	do
	{
		if (info == 0) {
			break;
		}
		if (info->pid == pid)
		{
			int len = __printf(szout + offset, 
				"memory:%x,virtual memory:%x,size:%x,pid:%x\n", info->addr, info->vaddr, info->size, info->pid);
			offset += len;
		}

		info = (LPMEMALLOCINFO)info->list.next;

	} while (info != (LPMEMALLOCINFO)base);

	return offset;
}