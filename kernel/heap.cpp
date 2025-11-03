

#include "heap.h"
#include "malloc.h"
#include "process.h"
#include "memory.h"

#include "apic.h"




DWORD __heapFree(DWORD addr) {

	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	if (addr >= tss->heapbase + tss->heapsize || addr <= tss->heapbase) {
		return 0;
	}

	MS_HEAP_STRUCT test;
	test.size = 0x80000000;

	MS_HEAP_STRUCT* heap = (MS_HEAP_STRUCT*)((UCHAR*)addr - sizeof(MS_HEAP_STRUCT));

	int heapsize = heap->size & 0x7fffffff;
	MS_HEAP_STRUCT* heapEnd = (MS_HEAP_STRUCT*)((UCHAR*)heap + sizeof(MS_HEAP_STRUCT) + heapsize);

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
			prev = (MS_HEAP_STRUCT*)((UCHAR*)prevEnd - sizeof(MS_HEAP_STRUCT) - (prevEnd->size &0x7fffffff) );
		}

		MS_HEAP_STRUCT* next = (MS_HEAP_STRUCT*)((UCHAR*)heap + (heapsize) + (sizeof(MS_HEAP_STRUCT) << 1));
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
			nextEnd = (MS_HEAP_STRUCT*)((UCHAR*)next + sizeof(MS_HEAP_STRUCT) + (next->size & 0x7fffffff));
		}

		if ((prev->size & 0x80000000) && (next->size & 0x80000000) )
		{
			heap->size = heap->size & 0x7fffffff;
			heapEnd->size = heap->size;
		}
		else if ((prev->size & 0x80000000) == 0 && (next->size & 0x80000000) == 0)
		{
			prev->addr = (DWORD)prev + sizeof(MS_HEAP_STRUCT);
			prev->size = (prev->size + heap->size + next->size + (sizeof(MS_HEAP_STRUCT) << 1) *2 )| 0x80000000;
			nextEnd->size = prev->size;
			nextEnd->addr = prev->addr;
		}
		else if ((prev->size & 0x80000000) && (next->size & 0x80000000) == 0)
		{
			heap->addr = (DWORD)heap + sizeof(MS_HEAP_STRUCT);
			heap->size = heap->size + next->size + (sizeof(MS_HEAP_STRUCT) << 1) | 0x80000000;

			nextEnd->addr = heap->addr;
			nextEnd->size = heap->size;
		}
		else if ((prev->size & 0x80000000) == 0 && (next->size & 0x80000000))
		{
			prev->addr = (DWORD)prev + sizeof(MS_HEAP_STRUCT);
			prev->size = prev->size + heap->size + (sizeof(MS_HEAP_STRUCT) << 1) | 0x80000000;

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

	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetCurrentTaskTssBase();

	MS_HEAP_STRUCT* lpheap = (MS_HEAP_STRUCT*)tss->heapbase;

	while ((DWORD)lpheap + allocsize + (sizeof(MS_HEAP_STRUCT) << 1) <= tss->heapbase + tss->heapsize)
	{
		if ((lpheap->size & 0x80000000) && lpheap->size && lpheap->addr)
		{
			lpheap = (MS_HEAP_STRUCT*)((UCHAR*)lpheap + (lpheap->size) + (sizeof(MS_HEAP_STRUCT) << 1));
			continue;
		}
		else if (lpheap->size && lpheap->addr)
		{
			int oldsize = (lpheap->size );
			if (oldsize >= allocsize + (int)(sizeof(MS_HEAP_STRUCT)*2))
			{
				
				lpheap->addr = (DWORD)(((DWORD)lpheap + sizeof(MS_HEAP_STRUCT)));
				lpheap->size = allocsize | 0x80000000;

				MS_HEAP_STRUCT* heapend = (MS_HEAP_STRUCT*)((DWORD)lpheap + sizeof(MS_HEAP_STRUCT) + (allocsize));
				heapend->addr = lpheap->addr;
				heapend->size = lpheap->size;

				MS_HEAP_STRUCT* next = (MS_HEAP_STRUCT*)((DWORD)lpheap + (allocsize) + (sizeof(MS_HEAP_STRUCT) << 1));
				next->size = (oldsize - allocsize - (sizeof(MS_HEAP_STRUCT)*2));
				next->addr = (DWORD)(((DWORD)next + sizeof(MS_HEAP_STRUCT)));

				MS_HEAP_STRUCT* nextEnd = (MS_HEAP_STRUCT*)((DWORD)next + sizeof(MS_HEAP_STRUCT) + (next->size) );
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
			lpheap->addr = (DWORD)(((DWORD)lpheap + sizeof(MS_HEAP_STRUCT)));
			lpheap->size = (allocsize)|0x80000000 ;

			MS_HEAP_STRUCT* heapend = (MS_HEAP_STRUCT*)((UCHAR*)lpheap + sizeof(MS_HEAP_STRUCT) + (lpheap->size) );
			heapend->addr = lpheap->addr;
			heapend->size = lpheap->size;

			return lpheap->addr;
		}
	}
	return 0;
}






