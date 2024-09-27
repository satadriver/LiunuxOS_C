

#include "heap.h"
#include "malloc.h"
#include "process.h"
#include "memory.h"






DWORD __heapFree(DWORD addr) {

	LPPROCESS_INFO tss = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
	if (addr >= tss->heapbase + tss->heapsize || addr <= tss->heapbase) {
		return 0;
	}

	MS_HEAP_STRUCT test;
	test.size = 0x8000;

	MS_HEAP_STRUCT* heap = (MS_HEAP_STRUCT*)((UCHAR*)addr - sizeof(MS_HEAP_STRUCT));
	MS_HEAP_STRUCT* heapEnd = (MS_HEAP_STRUCT*)((UCHAR*)heap + sizeof(MS_HEAP_STRUCT) + heap->size );

	if (heap->addr == addr)
	{
		MS_HEAP_STRUCT* prevEnd = 0;
		MS_HEAP_STRUCT* prev = 0;
		if ((DWORD)heap <= tss->heapbase) {
			prevEnd = (MS_HEAP_STRUCT*)&test;
			prev = (MS_HEAP_STRUCT*)&test;
		}
		else {
			prevEnd = (MS_HEAP_STRUCT*)((UCHAR*)heap - sizeof(MS_HEAP_STRUCT));
			prev = (MS_HEAP_STRUCT*)((UCHAR*)prevEnd - prevEnd->size - sizeof(MS_HEAP_STRUCT));
		}

		MS_HEAP_STRUCT* next = (MS_HEAP_STRUCT*)((UCHAR*)heap + (heap->size) + (sizeof(MS_HEAP_STRUCT) << 1));
		MS_HEAP_STRUCT* nextEnd = 0;
		if ((DWORD)next  >= tss->heapbase + tss->heapsize ) {
			next = (MS_HEAP_STRUCT*)&test;
			nextEnd = (MS_HEAP_STRUCT*)&test;
		}
		else if (next->addr == 0 && next->size == 0 ) {
			next = (MS_HEAP_STRUCT*)&test;
			nextEnd = (MS_HEAP_STRUCT*)&test;
		}
		else {
			nextEnd = (MS_HEAP_STRUCT*)((UCHAR*)next + sizeof(MS_HEAP_STRUCT) + (next->size));
		}

		if ((prev->size & 0x8000) && (next->size & 0x8000) )
		{
			heap->size = heap->size & 0x7fff;
			heapEnd->size = heapEnd->size & 0x7fff;
		}
		else if ((prev->size & 0x8000) == 0 && (next->size & 0x8000) == 0)
		{
			prev->addr = (DWORD)prev + sizeof(MS_HEAP_STRUCT);
			prev->size = (prev->size + heap->size + next->size + (sizeof(MS_HEAP_STRUCT) << 1) *2 )| 0x8000;
			nextEnd->size = prev->size;
			nextEnd->addr = prev->addr;
		}
		else if ((prev->size & 0x8000) && (next->size & 0x8000) == 0)
		{
			heap->addr = (DWORD)heap + sizeof(MS_HEAP_STRUCT);
			heap->size = heap->size + next->size + (sizeof(MS_HEAP_STRUCT) << 1) | 0x8000;

			nextEnd->addr = heap->addr;
			nextEnd->size = heap->size;
		}
		else if ((prev->size & 0x8000) == 0 && (next->size & 0x8000))
		{
			prev->addr = (DWORD)prev + sizeof(MS_HEAP_STRUCT);
			prev->size = prev->size + heap->size + (sizeof(MS_HEAP_STRUCT) << 1) | 0x8000;

			heapEnd->addr = prev->addr;
			heapEnd->size = prev->size;
		}

		return TRUE;

	}
	else {

	}

	return FALSE;
}

//allocate size is ( 2*sizeof(MS_HEAP_STRUCT) + MS_HEAP_STRUCT.size)
DWORD __heapAlloc(int size) {

	int allocsize = getAlignSize(size, sizeof(MS_HEAP_STRUCT)*2);

	LPPROCESS_INFO tss = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;

	MS_HEAP_STRUCT* lpheap = (MS_HEAP_STRUCT*)tss->heapbase;

	while ((DWORD)lpheap + allocsize + (sizeof(MS_HEAP_STRUCT) << 1) <= tss->heapbase + tss->heapsize)
	{
		if ((lpheap->size & 0x8000) && lpheap->size && lpheap->addr)
		{
			lpheap = (MS_HEAP_STRUCT*)((UCHAR*)lpheap + (lpheap->size<<4) + (sizeof(MS_HEAP_STRUCT) << 1));
			continue;
		}
		else if (lpheap->size && lpheap->addr)
		{
			if ( (lpheap->size<<4) > allocsize + (sizeof(MS_HEAP_STRUCT)*2))
			{
				int oldsize = (lpheap->size)<<4;
				
				lpheap->addr = (WORD)(((DWORD)lpheap + sizeof(MS_HEAP_STRUCT))>>4);
				lpheap->size = (allocsize>>4) | 0x8000;

				MS_HEAP_STRUCT* heapend = (MS_HEAP_STRUCT*)((DWORD)lpheap + (lpheap->size) + sizeof(MS_HEAP_STRUCT));
				heapend->addr = lpheap->addr;
				heapend->size = lpheap->size;

				MS_HEAP_STRUCT* next = (MS_HEAP_STRUCT*)((DWORD)lpheap + (lpheap->size) + (sizeof(MS_HEAP_STRUCT) << 1));
				next->size = (oldsize - allocsize - (sizeof(MS_HEAP_STRUCT)*2))>>4;
				next->addr = (WORD)(((DWORD)next + sizeof(MS_HEAP_STRUCT))>>4);

				MS_HEAP_STRUCT* nextEnd = (MS_HEAP_STRUCT*)((DWORD)next + (next->size<<4) + sizeof(MS_HEAP_STRUCT) );
				nextEnd->size = next->size;
				nextEnd->addr = next->addr;

				return lpheap->addr;
			}
			else {
				lpheap = (MS_HEAP_STRUCT*)((UCHAR*)lpheap + (lpheap->size) + (sizeof(MS_HEAP_STRUCT) << 1));
				continue;
			}
		}
		else {
			lpheap->addr = (DWORD)(((DWORD)lpheap + sizeof(MS_HEAP_STRUCT))>>4);
			lpheap->size = (allocsize>>4) | 0x8000;

			MS_HEAP_STRUCT* heapend = (MS_HEAP_STRUCT*)((UCHAR*)lpheap + (lpheap->size) + sizeof(MS_HEAP_STRUCT));
			heapend->addr = lpheap->addr;
			heapend->size = lpheap->size;

			return lpheap->addr;
		}
	}
	return 0;
}






