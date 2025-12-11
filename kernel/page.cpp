#include "page.h"
#include "Utils.h"
#include "video.h"
#include "malloc.h"
#include "process.h"
#include "memory.h"
#include "apic.h"

//BIOS:
//https://blog.csdn.net/x86ipc/article/details/5303760

DWORD gPageAllocLock = FALSE;

LPMEMALLOCINFO gPageAllocList = 0;

LPMEMALLOCINFO findPageIdx(DWORD addr) {

	LPMEMALLOCINFO base = (LPMEMALLOCINFO)PAGE_ALLOC_LIST;
	LPMEMALLOCINFO info = (LPMEMALLOCINFO)(base->list.next);
	LPMEMALLOCINFO hdr = info;

	do
	{
		if (info == 0) {
			break;
		}

		if (info->addr == addr)
		{
			return info;
		}
		else {
			info = (LPMEMALLOCINFO)info->list.next;
		}
	} while (info && (info != hdr));

	return 0;
}

LPMEMALLOCINFO isPageIdxExist(DWORD addr,int size) {

	LPMEMALLOCINFO base = (LPMEMALLOCINFO)PAGE_ALLOC_LIST;
	LPMEMALLOCINFO info = (LPMEMALLOCINFO)(base->list.next);
	LPMEMALLOCINFO hdr = info;

	do
	{
		if (info == 0) {
			break;
		}

		if (info->addr == addr)
		{
			return info;
		}
		else if ( (info->addr < addr) && (info->addr + info->size > addr) ) 
		{
			return info;
		}
		else {
			info = (LPMEMALLOCINFO)info->list.next;
		}
	} while (info && (info != hdr));

	return 0;
}

LPMEMALLOCINFO getFreePageIdx() {

	LPMEMALLOCINFO info = (LPMEMALLOCINFO)PAGE_ALLOC_LIST;

	int cnt = PAGE_ALLOC_LIST_SIZE / sizeof(MEMALLOCINFO);
	for (int i = 1; i < cnt; i++)
	{
		if (info[i].size == 0 && info[i].addr == 0 && info[i].pid == 0 && info[i].vaddr == 0)
		{
			return &info[i];
		}
	}
	return 0;
}


int resetPageIdx(LPMEMALLOCINFO pde) {
	DWORD size = pde->size;
	RemoveList(&gPageAllocList->list,(LPLIST_ENTRY)&pde->list);
	pde->addr = 0;
	pde->size = 0;
	pde->vaddr = 0;
	pde->pid = 0;
	return size;
}



int insertPageIdx(LPMEMALLOCINFO info, DWORD addr, int size,int pid,DWORD vaddr ) {
	info->size = size;
	info->addr = addr;
	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	info->pid = tss->pid;

	InsertListTail( &((LPMEMALLOCINFO)PAGE_ALLOC_LIST)->list, (LPLIST_ENTRY)&info->list);
	return TRUE;
}



extern "C"  __declspec(dllexport) DWORD __kPageAlloc(int size) {
	DWORD res = 0;

	if (size % PAGE_SIZE)
	{
		return FALSE;
	}

	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetCurrentTaskTssBase();

	__enterSpinlock(&gPageAllocLock);

	int factor = 1;
	while (1)
	{
		for (int n = factor / 2; n && n < factor; )
		{
			DWORD addr = PAGE_TABLE_BASE + size * n;
			if (addr + size > PAGE_TABLE_BASE + PAGE_TABLE_SIZE)
			{
				res = -1;
				break;
			}

			LPMEMALLOCINFO info = isPageIdxExist(addr, size);
			if (info == 0)
			{
				info = getFreePageIdx();
				if (info)
				{
					insertPageIdx(info, addr, size, tss->pid, addr);
					res = addr;
				}
				else {
					res = -1;
				}
				break;
			}
			else {
				if (info->size > (DWORD)size) {
					int t = info->size / size;
					n += t;
					if (n >= factor) {
						while (n >= factor) {
							factor = factor << 1;
						}
					}

					continue;
				}
			}

			n++;
		}

		if (res == 0) {
			factor = (factor << 1);
		}
		else {
			break;
		}
	}
	
	__leaveSpinlock(&gPageAllocLock);

	if (res == -1) {
		res = 0;
	}

	return res;
}






extern "C"  __declspec(dllexport) int __kFreePage(DWORD addr) {

	__enterSpinlock(&gPageAllocLock);

	LPMEMALLOCINFO info = findPageIdx(addr);
	if (info)
	{
		DWORD size = resetPageIdx(info);
	}
	else {
		char szout[1024];
		int len = __printf(szout, "__kFreePage not found address:%x\n", addr);
	}
	__leaveSpinlock(&gPageAllocLock);

	return FALSE;
}



//make sure the first in the list is not to be deleted,or else will be locked
void freeProcessPages(int pid) {

	__enterSpinlock(&gPageAllocLock);
	
	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetCurrentTaskTssBase();

	LPMEMALLOCINFO base = (LPMEMALLOCINFO)(LPMEMALLOCINFO)gPageAllocList->list.next;
	LPMEMALLOCINFO info = base;
	do
	{
		if (info->pid == pid)
		{
			resetPageIdx(info);
		}

		info = (LPMEMALLOCINFO)info->list.next;

	} while (info != (LPMEMALLOCINFO)base);

	__leaveSpinlock(&gPageAllocLock);
}




void InitPage64(QWORD * base) {
#ifdef _DEBUG
	gAvailableSize = 0x44000000;
#endif
	QWORD bs5 = 0x1000;

	QWORD bs4 = 0x1000 / 8 * 0x1000;	//2M

	QWORD bs3 = 0x1000 / 8 * bs4;		//1G

	QWORD bs2 = 0x1000 / 8 * bs3;		//512G

	QWORD bs1 = 0x1000 / 8 * bs2;		//512*512G

	QWORD cnt1 = gAvailableSize / bs1;
	QWORD mod1 = gAvailableSize % bs1;
	if (mod1) {
		cnt1++;
	}

	QWORD cnt2 = gAvailableSize / bs2;
	QWORD mod2 = gAvailableSize % bs2;
	if (mod2) {
		cnt2++;
	}

	QWORD cnt3 = gAvailableSize / bs3;
	QWORD mod3 = gAvailableSize % bs3;
	if (mod3) {
		cnt3++;
	}

	QWORD cnt4 = gAvailableSize / bs4;
	QWORD mod4 = gAvailableSize % bs4;
	if (mod4) {
		cnt4++;
	}

	QWORD* table1 = (QWORD*)base;

	QWORD ps1 = (cnt1) * 0x1000;

	QWORD* table2 = (QWORD*)table1  + ps1 / sizeof(QWORD);

	QWORD ps2 = (cnt2) * 0x1000;

	QWORD* table3= (QWORD*)table2 + ps2 / sizeof(QWORD);

	QWORD ps3 = cnt3* 0x1000;

	QWORD* table4 = (QWORD*)table3 +  ps3 / sizeof(QWORD);

	QWORD ps4 = cnt4 * 0x1000;
	
	for (QWORD i = 0; i < cnt1; i++) {
		
		for (QWORD j = 0; j < cnt2; j++) {
			table1[j] = ((QWORD)table2 | 7) ;
			for (QWORD k = 0; k < cnt3; k++) {
				table2[k] = ((QWORD)table3 | 7) +(k / 512) * PAGE_SIZE;
				for (QWORD n = 0; n < cnt4; n++) {
					table3[n] = ((QWORD)table4 | 7) ;
					for (QWORD m = 0; m < 512; m++) 
					{
						QWORD v = ((m << 12) | 7)+n*bs4;
						table4[m] = v ;
					}
					table4 += PAGE_SIZE / 8;
				}
				//table3 += PAGE_SIZE / 8;
			}
			table2 += PAGE_SIZE / 8;
		}
	}
}




void linearMapping() {

	DWORD* idx = (DWORD*)PTE_ENTRY_VALUE;
	DWORD* entry = (DWORD*)PDE_ENTRY_VALUE;

	DWORD buf = PAGE_PRESENT | PAGE_READWRITE| PAGE_USERPRIVILEGE;
	__memset((char*)PDE_ENTRY_VALUE, 0, PAGE_SIZE);

#if 0
//#ifndef DISABLE_PAGE_MAPPING
	mapPhyToLinear(0, 0, 0x80000000, (DWORD*)PDE_ENTRY_VALUE, PAGE_PRESENT | PAGE_READWRITE | PAGE_USERPRIVILEGE);
	mapPhyToLinear(0x80000000, 0x80000000, 0x80000000, (DWORD*)PDE_ENTRY_VALUE, PAGE_PRESENT | PAGE_READWRITE | PAGE_USERPRIVILEGE);
	return;

	int cnt = MEMMORY_ALLOC_BASE / (PAGE_SIZE * ITEM_IN_PAGE);
	for (int i = 0; i < cnt; i++)
	{
		//entry[i] = (DWORD)idx | (PAGE_PRESENT | PAGE_READWRITE | PAGE_USERPRIVILEGE);
		idx += ITEM_IN_PAGE;
	}
	mapPhyToLinear(0, 0, MEMMORY_ALLOC_BASE, (DWORD*)PDE_ENTRY_VALUE, PAGE_PRESENT | PAGE_READWRITE | PAGE_USERPRIVILEGE);

	int begin = MEMMORY_ALLOC_BASE / (PAGE_SIZE * ITEM_IN_PAGE);
	int end = (getBorderAddr() & (~(PAGE_SIZE * ITEM_IN_PAGE - 1))) / (PAGE_SIZE * ITEM_IN_PAGE);
	for (int i = begin; i < end; i++)
	{
		for (int k = 0; i < ITEM_IN_PAGE; k++) {

		}
		char * pageidx =(char*) (entry[i]&0xfffff000);
		__memset(pageidx, 0, PAGE_SIZE);
	}

	mapPhyToLinear(0xc0000000, 0xc0000000, 0x40000000, (DWORD*)PDE_ENTRY_VALUE, PAGE_PRESENT | PAGE_READWRITE | PAGE_USERPRIVILEGE);

	return;
#endif

	unsigned long start = MEMMORY_ALLOC_BASE / (PAGE_SIZE * ITEM_IN_PAGE);
	unsigned long highaddr = (getBorderAddr() ) & (~((PAGE_SIZE * ITEM_IN_PAGE) - 1));
	unsigned long stop = highaddr / (PAGE_SIZE * ITEM_IN_PAGE);

	for (unsigned long i = 0; i < ITEM_IN_PAGE; i++) {
		entry[i] = (DWORD)idx | (PAGE_PRESENT | PAGE_READWRITE | PAGE_USERPRIVILEGE);
		for (unsigned long j = 0; j < ITEM_IN_PAGE; j++) {
			if (i >= start && i < stop) {
				idx[j] = 0;
				idx[j] = buf;
			}
			else {
				idx[j] = buf;
			}
			
			buf += PAGE_SIZE;
		}

		idx += ITEM_IN_PAGE;
	}


	for (unsigned long i = start; i < stop; i++)
	{
		char* pageidx = (char*)(entry[i] & 0xfffff000);
		//__memset(pageidx, 0, PAGE_SIZE);
	}

}

/*
 处理器仅仅会缓存那些P位是1的那些页表项，而且，TLB的工作和CR3寄存器的PCD位和PWT是无关的。
 对于页表项的修改不会同时反映到TLB中，一定要刷新TLB，不然对页表的设置就是无效的。
 TLB是软件不可直接访问的，只能通过显式刷新CR3，或者任务切换隐式刷新TLB，
 但是要注意的是，这样的刷新方法对于那些标记为全局（G=1）的页表无效。

 TLB还可以单个刷新，利用invlpg命令（invalidate TLB Entry）。
 invlpg的格式为invlpg m32，当执行这条指令的时候，处理器会用给出的线性地址搜索TLB，找到那个条目，然后从内存中重新加载其内容到相应的TLB页表数据中
 invlpg是特权指令，必须要在CPL为特权0级执行，该指令不影响任何标志位。
*/

void initPaging() {

	gPageAllocList = (LPMEMALLOCINFO)PAGE_ALLOC_LIST;
	LPMEMALLOCINFO pageList = (LPMEMALLOCINFO)PAGE_ALLOC_LIST;
	InitListEntry(&pageList->list);
	pageList->addr = 0;
	pageList->size = 0;
	pageList->vaddr = 0;
	pageList->pid = 0;

	linearMapping();

	__asm {
		mov eax, PDE_ENTRY_VALUE
		mov cr3, eax

		mov eax, cr0
		or eax, 0x80000000
		mov cr0, eax
	}
}