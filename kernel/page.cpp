#include "page.h"
#include "Utils.h"
#include "video.h"
#include "malloc.h"
#include "process.h"





DWORD gPageAllocLock = FALSE;

LPMEMALLOCINFO gPageAllocList = 0;



LPMEMALLOCINFO isPageIdxExist(DWORD addr,int size) {

	LPMEMALLOCINFO base = (LPMEMALLOCINFO)PAGE_ALLOC_LIST;
	LPMEMALLOCINFO info = (LPMEMALLOCINFO)(base->list.next);
	LPMEMALLOCINFO tmp = info;

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
	} while (info && (info != tmp));

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
	removelist(&gPageAllocList->list,(LPLIST_ENTRY)&pde->list);
	pde->addr = 0;
	pde->size = 0;
	pde->vaddr = 0;
	pde->pid = 0;
	return size;
}



int insertPageIdx(LPMEMALLOCINFO info, DWORD addr, int size,int pid,DWORD vaddr ) {
	info->size = size;
	info->addr = addr;
	LPPROCESS_INFO tss = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
	info->pid = tss->pid;

	addlistTail( &((LPMEMALLOCINFO)PAGE_ALLOC_LIST)->list, (LPLIST_ENTRY)&info->list);
	return TRUE;
}



extern "C"  __declspec(dllexport) DWORD __kPageAlloc(int size) {
	DWORD res = 0;

	if (size % PAGE_SIZE)
	{
		return FALSE;
	}

	int factor = 1;
	DWORD addr = PAGE_TABLE_BASE + size*factor;
	if (addr + size > PAGE_TABLE_BASE + PAGE_TABLE_SIZE)
	{
		return FALSE;
	}

	LPPROCESS_INFO tss = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;

	__enterSpinlock(&gPageAllocLock);

	LPMEMALLOCINFO info = isPageIdxExist(addr,size);
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
	}
	else {
		
		while (1)
		{
			if (factor > 1 && info->size <= size)
			{
				for (int i = 0; i < factor - 1; i++)
				{
					addr += size;
					if (addr + size > PAGE_TABLE_BASE + PAGE_TABLE_SIZE)
					{
						res = -1;
						break;
					}

					info = isPageIdxExist(addr,size);
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
						break;
					}
				}
			}
			else {
				//noting to do
			}

			if (res == 0) {
				factor = (factor << 1);
				addr = PAGE_TABLE_BASE + size * factor;
			}
			else {
				break;
			}
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

	LPMEMALLOCINFO info = isPageIdxExist(addr,0);
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
	
	LPPROCESS_INFO tss = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;

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


//R/W--位1是读/写（Read/Write）标志。如果等于1，表示页面可以被读、写或执行。如果为0，表示页面只读或可执行。
//当处理器运行在超级用户特权级（级别0、1或2）时，则R/W位不起作用。页目录项中的R/W位对其所映射的所有页面起作用。
//U/S--位2是用户/超级用户（User / Supervisor）标志。如果为1，那么运行在任何特权级上的程序都可以访问该页面。
//如果为0，那么页面只能被运行在超级用户特权级（0、1或2）上的程序访问。页目录项中的U / S位对其所映射的所有页面起作用。

void linearMapping() {

	DWORD* idx = (DWORD*)PTE_ENTRY_VALUE;
	DWORD* entry = (DWORD*)PDE_ENTRY_VALUE;

	DWORD buf = PAGE_PRESENT | PAGE_READWRITE| PAGE_USERPRIVILEGE;
	for (int i = 0; i < ITEM_IN_PAGE; i++) {
		entry[i] = (DWORD)idx | (PAGE_PRESENT | PAGE_READWRITE | PAGE_USERPRIVILEGE);

		for (int j = 0; j < ITEM_IN_PAGE; j++) {
			idx[j] = buf;
			buf += PAGE_SIZE;
		}
		idx += ITEM_IN_PAGE;
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

	linearMapping();

	__asm {
		mov eax, PDE_ENTRY_VALUE
		mov cr3,eax

		mov eax,cr0
		or eax,0x80000000
		mov cr0,eax
	}

	gPageAllocList = (LPMEMALLOCINFO)PAGE_ALLOC_LIST;
	LPMEMALLOCINFO pageList = (LPMEMALLOCINFO)PAGE_ALLOC_LIST;
	initListEntry(&pageList->list);
	pageList->addr = 0;
	pageList->size = 0;
	pageList->vaddr = 0;
	pageList->pid = 0;
}