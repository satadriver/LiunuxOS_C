

#include "heap.h"
#include "malloc.h"
#include "process.h"
#include "memory.h"
#include "Utils.h"
#include "apic.h"


#define HEAP_BUFFER_POSITION	0X80000000			//combind buffer 

#define HEAP_BUFFER_FREE		0X7FFFFFFF

int g_heap_alloc_lock = 0;



int InitHeap(int pid) {

	LPPROCESS_INFO procs = (LPPROCESS_INFO)GetTaskTssBase();
	if (procs[pid].heapbase && procs[pid].heapsize) {

		MS_HEAP_STRUCT* heap = (MS_HEAP_STRUCT*)procs[pid].heapbase;
		heap->addr = (DWORD)((char*)heap + sizeof(MS_HEAP_STRUCT));
		heap->size = procs[pid].heapsize - sizeof(MS_HEAP_STRUCT) * 2;
		MS_HEAP_STRUCT* heapEnd = (MS_HEAP_STRUCT*)((UCHAR*)heap + sizeof(MS_HEAP_STRUCT) + heap->size);
		heapEnd->addr = heap->addr;
		heapEnd->size = heap->size;	
	}
	return 0;
}

DWORD __heapFree(DWORD addr) {
	char szout[256];

	DWORD result = 0;

	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	if (addr >= tss->heapbase + tss->heapsize || addr <= tss->heapbase) {
		return result;
	}

	__enterSpinlock(&g_heap_alloc_lock);

	MS_HEAP_STRUCT* heap = (MS_HEAP_STRUCT*)((UCHAR*)addr - sizeof(MS_HEAP_STRUCT));

	int heapSize = heap->size & HEAP_BUFFER_FREE;
	int heapHdrSize = sizeof(MS_HEAP_STRUCT) << 1;
	
	MS_HEAP_STRUCT* heapEnd = (MS_HEAP_STRUCT*)((UCHAR*)heap + sizeof(MS_HEAP_STRUCT) + heapSize);

	if (heap->addr == addr && heapEnd->addr == addr)
	{
		heap->size = heapSize;

		int nextSize = 0;
		int prevSize = 0;
		int prevHdrSize = 0;
		int nextHdrSize = 0;

		MS_HEAP_STRUCT* prevEnd = 0;
		MS_HEAP_STRUCT* prev = 0;
		if ((DWORD)heap <= tss->heapbase) {
			prevEnd = (MS_HEAP_STRUCT*)heapEnd;
			prev = (MS_HEAP_STRUCT*)heap;
			prevSize = 0;
			prevHdrSize = 0;
		}
		else {
			prevEnd = (MS_HEAP_STRUCT*)((UCHAR*)heap - sizeof(MS_HEAP_STRUCT));
			prev = (MS_HEAP_STRUCT*)((UCHAR*)prevEnd - sizeof(MS_HEAP_STRUCT) - (prevEnd->size & HEAP_BUFFER_FREE) );
			prevSize = prev->size & HEAP_BUFFER_FREE;
			prevHdrSize = (sizeof(MS_HEAP_STRUCT) << 1);
		}

		MS_HEAP_STRUCT* next = (MS_HEAP_STRUCT*)((UCHAR*)heap + (heapSize) + (sizeof(MS_HEAP_STRUCT) << 1));
		MS_HEAP_STRUCT* nextEnd = 0;
		if ((DWORD)next  >= tss->heapbase + tss->heapsize ) {
			next = (MS_HEAP_STRUCT*)heap;
			nextEnd = (MS_HEAP_STRUCT*)heapEnd;
			nextSize = 0;
			nextHdrSize = 0;
		}
		else if (next->addr == 0 && next->size == 0 ) {
			next = (MS_HEAP_STRUCT*)heap;
			nextEnd = (MS_HEAP_STRUCT*)heapEnd;
			nextSize = 0;
			nextHdrSize = 0;
		}
		else {
			nextEnd = (MS_HEAP_STRUCT*)((UCHAR*)next + sizeof(MS_HEAP_STRUCT) + (next->size & HEAP_BUFFER_FREE));
			nextSize = next->size & HEAP_BUFFER_FREE;
			nextHdrSize = (sizeof(MS_HEAP_STRUCT) << 1);
		}

		if ((prev->size & 0x80000000) && (next->size & 0x80000000) )
		{
			heap->size = heap->size & HEAP_BUFFER_FREE;
			heapEnd->size = heap->size;
		}
		else if ((prev->size & 0x80000000) == 0 && (next->size & 0x80000000) == 0)
		{
			prev->addr = (DWORD)prev + sizeof(MS_HEAP_STRUCT);
			prev->size = (prevSize + heapSize + nextSize + prevHdrSize + nextHdrSize ) & HEAP_BUFFER_FREE;
			nextEnd->size = prev->size;
			nextEnd->addr = prev->addr;
		}
		else if ((prev->size & 0x80000000) && (next->size & 0x80000000) == 0)
		{
			heap->addr = (DWORD)heap + sizeof(MS_HEAP_STRUCT);
			heap->size = (heapSize + nextSize + nextHdrSize) & HEAP_BUFFER_FREE;

			nextEnd->addr = heap->addr;
			nextEnd->size = heap->size;
		}
		else if ((prev->size & 0x80000000) == 0 && (next->size & 0x80000000))
		{
			prev->addr = (DWORD)prev + sizeof(MS_HEAP_STRUCT);
			prev->size = (prevSize + heapSize + prevHdrSize) & HEAP_BUFFER_FREE;

			heapEnd->addr = prev->addr;
			heapEnd->size = prev->size;
		}
		
		result = TRUE;
	}
	else {
		__printf(szout, "%s %d heap address:%x not found\r\n", __FUNCTION__, __LINE__, addr);
	}

	__leaveSpinlock(&g_heap_alloc_lock);
	return result;
}

//allocate size is ( 2*sizeof(MS_HEAP_STRUCT) + MS_HEAP_STRUCT.size)
DWORD __heapAlloc(int size) {

	DWORD addr = 0;

	__enterSpinlock(&g_heap_alloc_lock);

	int allocsize = getAlignSize(size, sizeof(MS_HEAP_STRUCT)*2);

	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetCurrentTaskTssBase();

	MS_HEAP_STRUCT* lpheap = (MS_HEAP_STRUCT*)tss->heapbase;

	while ((DWORD)lpheap + allocsize + (sizeof(MS_HEAP_STRUCT) << 1) <= tss->heapbase + tss->heapsize)
	{
		int heapSize = (lpheap->size)& HEAP_BUFFER_FREE;
		if ( (lpheap->size & HEAP_BUFFER_POSITION) && lpheap->size && lpheap->addr)
		{
			lpheap = (MS_HEAP_STRUCT*)((UCHAR*)lpheap + (lpheap->size & HEAP_BUFFER_FREE) + (sizeof(MS_HEAP_STRUCT) << 1) );
			continue;
		}
		else if (lpheap->size && lpheap->addr)
		{
			if (heapSize >= allocsize + (sizeof(MS_HEAP_STRUCT)*2) )
			{			
				lpheap->addr = (DWORD)((DWORD)lpheap + sizeof(MS_HEAP_STRUCT));
				lpheap->size = allocsize | HEAP_BUFFER_POSITION;

				MS_HEAP_STRUCT* heapend = (MS_HEAP_STRUCT*)((DWORD)lpheap + sizeof(MS_HEAP_STRUCT) + (allocsize));
				heapend->addr = lpheap->addr;
				heapend->size = lpheap->size;

				MS_HEAP_STRUCT* next = (MS_HEAP_STRUCT*)((DWORD)lpheap + (allocsize)+(sizeof(MS_HEAP_STRUCT) *2));
				next->size = (heapSize - allocsize - (sizeof(MS_HEAP_STRUCT) * 2)) & HEAP_BUFFER_FREE;
				next->addr = (DWORD)((DWORD)next + sizeof(MS_HEAP_STRUCT));

				MS_HEAP_STRUCT* nextEnd = (MS_HEAP_STRUCT*)((DWORD)next + sizeof(MS_HEAP_STRUCT) + (next->size & HEAP_BUFFER_FREE) );
				nextEnd->size = next->size;
				nextEnd->addr = next->addr;
				
				addr = lpheap->addr;
				break;
			}
			else if (heapSize == allocsize ) {
				lpheap->addr = (DWORD)((DWORD)lpheap + sizeof(MS_HEAP_STRUCT));
				lpheap->size = allocsize | HEAP_BUFFER_POSITION;

				MS_HEAP_STRUCT* heapend = (MS_HEAP_STRUCT*)((DWORD)lpheap + sizeof(MS_HEAP_STRUCT) + (allocsize));
				heapend->addr = lpheap->addr;
				heapend->size = lpheap->size;
				addr = lpheap->addr;
				break;
			}
			else {
				lpheap = (MS_HEAP_STRUCT*)((UCHAR*)lpheap + (lpheap->size) + (sizeof(MS_HEAP_STRUCT) << 1));
				continue;
			}
		}
		else {
			lpheap->addr = (DWORD)((DWORD)lpheap + sizeof(MS_HEAP_STRUCT));
			lpheap->size = (allocsize)| HEAP_BUFFER_POSITION;

			MS_HEAP_STRUCT* heapend = (MS_HEAP_STRUCT*)((UCHAR*)lpheap + sizeof(MS_HEAP_STRUCT) + allocsize);
			heapend->addr = lpheap->addr;
			heapend->size = lpheap->size;

			addr = lpheap->addr;
			break;
		}
	}

	__leaveSpinlock(&g_heap_alloc_lock);

	return addr;
}






