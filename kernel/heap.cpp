

#include "heap.h"
#include "malloc.h"
#include "process.h"
#include "memory.h"






DWORD __heapFree(DWORD addr) {
	LPPROCESS_INFO tss = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;

	MS_HEAP_STRUCT test;
	test.flag = 1;

	MS_HEAP_STRUCT* heap = (MS_HEAP_STRUCT*)((UCHAR*)addr - sizeof(MS_HEAP_STRUCT));
	MS_HEAP_STRUCT* heapEnd = (MS_HEAP_STRUCT*)((UCHAR*)heap + heap->size + sizeof(MS_HEAP_STRUCT));
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
		if (next->addr == 0 && next->size == 0 && next->flag == 0 && next->reserved == 0) {
			next = (MS_HEAP_STRUCT*)&test;
			nextEnd = (MS_HEAP_STRUCT*)&test;
		}
		else if (tss->heapbase + tss->heapsize - (DWORD)next < 3 * sizeof(MS_HEAP_STRUCT)) {
			next = (MS_HEAP_STRUCT*)&test;
			nextEnd = (MS_HEAP_STRUCT*)&test;
		}
		else {
			nextEnd = (MS_HEAP_STRUCT*)((UCHAR*)next + (next->size) + sizeof(MS_HEAP_STRUCT));
		}

		if ((prev->flag & 1) == 1 && (next->flag & 1) == 1)
		{
			heap->flag = 0;
			heapEnd->flag = 0;
		}
		else if ((prev->flag & 1) == 0 && (next->flag & 1) == 0)
		{
			prev->addr = (DWORD)prev + sizeof(MS_HEAP_STRUCT);
			prev->size = prev->size + heap->size + next->size + (sizeof(MS_HEAP_STRUCT) << 1) + (sizeof(MS_HEAP_STRUCT) << 1);
			prev->flag = 0;
			nextEnd->size = prev->size;
			nextEnd->addr = prev->addr;
			nextEnd->flag = prev->flag;
		}
		else if ((prev->flag & 1) == 1 && (next->flag & 1) == 0)
		{
			heap->addr = (DWORD)heap + sizeof(MS_HEAP_STRUCT);
			heap->size = heap->size + next->size + (sizeof(MS_HEAP_STRUCT) << 1);
			heap->flag = 0;
			nextEnd->addr = heap->addr;
			nextEnd->flag = heap->flag;
			nextEnd->size = heap->size;
		}
		else if ((prev->flag & 1) == 0 && (next->flag & 1) == 1)
		{
			prev->addr = (DWORD)prev + sizeof(MS_HEAP_STRUCT);
			prev->size = prev->size + heap->size + (sizeof(MS_HEAP_STRUCT) << 1);
			prev->flag = 0;
			heapEnd->addr = prev->addr;
			heapEnd->flag = prev->flag;
			heapEnd->size = prev->size;
		}

		return TRUE;
	}

	return FALSE;
}


DWORD __heapAlloc(int size) {

	int allocsize = getAlignedSize(size, sizeof(MS_HEAP_STRUCT));

	LPPROCESS_INFO tss = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;

	MS_HEAP_STRUCT* lpheap = (MS_HEAP_STRUCT*)tss->heapbase;

	while ((DWORD)lpheap + allocsize + (sizeof(MS_HEAP_STRUCT) << 1) <= tss->heapbase + tss->heapsize)
	{
		if ((lpheap->flag & 1) && lpheap->size && lpheap->addr)
		{
			lpheap = (MS_HEAP_STRUCT*)((UCHAR*)lpheap + (lpheap->size) + (sizeof(MS_HEAP_STRUCT) << 1));
			continue;
		}
		else if (lpheap->size && lpheap->addr)
		{
			if ((lpheap->size) >= allocsize)
			{
				int oldsize = (lpheap->size);

				lpheap->flag = 1;
				lpheap->addr = (DWORD)((UCHAR*)lpheap + sizeof(MS_HEAP_STRUCT));
				lpheap->size = allocsize;

				MS_HEAP_STRUCT* heapend = (MS_HEAP_STRUCT*)((UCHAR*)lpheap + (lpheap->size) + sizeof(MS_HEAP_STRUCT));
				heapend->addr = lpheap->addr;
				heapend->size = lpheap->size;
				heapend->flag = lpheap->flag;

				MS_HEAP_STRUCT* next = (MS_HEAP_STRUCT*)((UCHAR*)lpheap + (lpheap->size) + (sizeof(MS_HEAP_STRUCT) << 1));
				next->size = (oldsize - allocsize - (sizeof(MS_HEAP_STRUCT) << 1));
				next->addr = (DWORD)((UCHAR*)next + sizeof(MS_HEAP_STRUCT));
				next->flag = 0;

				return lpheap->addr;
			}
			else {
				lpheap = (MS_HEAP_STRUCT*)((UCHAR*)lpheap + (lpheap->size) + (sizeof(MS_HEAP_STRUCT) << 1));
				continue;
			}
		}
		else {
			lpheap->flag = 1;
			lpheap->addr = (DWORD)((UCHAR*)lpheap + sizeof(MS_HEAP_STRUCT));
			lpheap->size = allocsize;

			MS_HEAP_STRUCT* heapend = (MS_HEAP_STRUCT*)((UCHAR*)lpheap + (lpheap->size) + sizeof(MS_HEAP_STRUCT));
			heapend->addr = lpheap->addr;
			heapend->size = lpheap->size;
			heapend->flag = lpheap->flag;

			return lpheap->addr;
		}
	}
	return 0;
}






