

#include "heap.h"
#include "malloc.h"
#include "process.h"
#include "memory.h"
#include "Utils.h"
#include "apic.h"


#define HEAP_BUFFER_POSITION	0X80000000			//combind buffer 

#define HEAP_BUFFER_FREE		0X7FFFFFFF

//int g_heap_alloc_lock = 0;

//int g_heap_fastalloc_lock = 0;

int InitHeap(int pid) {

	return 0;
}

DWORD __heapFree(DWORD addr) {
	char szout[256];

	DWORD result = 0;

	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetCurrentTaskTssBase();

	__enterSpinlock(tss->lpheap_lock);

	MS_HEAP_STRUCT* heap = (MS_HEAP_STRUCT*)((UCHAR*)addr - sizeof(MS_HEAP_STRUCT));

	int heapSize = heap->size & HEAP_BUFFER_FREE;
	int heapHdrSize = sizeof(MS_HEAP_STRUCT) << 1;
	
	MS_HEAP_STRUCT* heapEnd = (MS_HEAP_STRUCT*)((UCHAR*)heap + sizeof(MS_HEAP_STRUCT) + heapSize);

	int cnt = *tss->lpHeapCnt;
	for (int i = 0; i < cnt; i++) {
		if ( addr >= tss->lpHeapBase[i] && addr < tss->lpHeapBase[i] + tss->heapsize ) {

			if ( (heap->addr == addr && heapEnd->addr == addr) && (heap->size == heapEnd->size))
			{
				heap->size = heapSize;

				int nextSize = 0;
				int prevSize = 0;
				int prevHdrSize = 0;
				int nextHdrSize = 0;

				MS_HEAP_STRUCT* prevEnd = 0;
				MS_HEAP_STRUCT* prev = 0;
				if ((DWORD)heap <= tss->lpHeapBase[i]) {
					prevEnd = (MS_HEAP_STRUCT*)heapEnd;
					prev = (MS_HEAP_STRUCT*)heap;
					prevSize = 0;
					prevHdrSize = 0;
				}
				else {
					prevEnd = (MS_HEAP_STRUCT*)((UCHAR*)heap - sizeof(MS_HEAP_STRUCT));
					prev = (MS_HEAP_STRUCT*)((UCHAR*)prevEnd - sizeof(MS_HEAP_STRUCT) - (prevEnd->size & HEAP_BUFFER_FREE));
					prevSize = prev->size & HEAP_BUFFER_FREE;
					prevHdrSize = (sizeof(MS_HEAP_STRUCT) << 1);
				}

				MS_HEAP_STRUCT* next = (MS_HEAP_STRUCT*)((UCHAR*)heap + (heapSize)+(sizeof(MS_HEAP_STRUCT) << 1));
				MS_HEAP_STRUCT* nextEnd = 0;
				if ((DWORD)next >= tss->lpHeapBase[i] + tss->heapsize) {
					next = (MS_HEAP_STRUCT*)heap;
					nextEnd = (MS_HEAP_STRUCT*)heapEnd;
					nextSize = 0;
					nextHdrSize = 0;
				}
				else if (next->addr == 0 && next->size == 0) {
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

				if ((prev->size & HEAP_BUFFER_POSITION) && (next->size & HEAP_BUFFER_POSITION))
				{
					heap->size = heap->size & HEAP_BUFFER_FREE;
					heapEnd->size = heap->size;
				}
				else if ((prev->size & HEAP_BUFFER_POSITION) == 0 && (next->size & HEAP_BUFFER_POSITION) == 0)
				{
					prev->addr = (DWORD)prev + sizeof(MS_HEAP_STRUCT);
					prev->size = (prevSize + heapSize + nextSize + prevHdrSize + nextHdrSize) & HEAP_BUFFER_FREE;
					nextEnd->size = prev->size;
					nextEnd->addr = prev->addr;
				}
				else if ((prev->size & HEAP_BUFFER_POSITION) && (next->size & HEAP_BUFFER_POSITION) == 0)
				{
					heap->addr = (DWORD)heap + sizeof(MS_HEAP_STRUCT);
					heap->size = (heapSize + nextSize + nextHdrSize) & HEAP_BUFFER_FREE;

					nextEnd->addr = heap->addr;
					nextEnd->size = heap->size;
				}
				else if ((prev->size & HEAP_BUFFER_POSITION) == 0 && (next->size & HEAP_BUFFER_POSITION))
				{
					prev->addr = (DWORD)prev + sizeof(MS_HEAP_STRUCT);
					prev->size = (prevSize + heapSize + prevHdrSize) & HEAP_BUFFER_FREE;

					heapEnd->addr = prev->addr;
					heapEnd->size = prev->size;
				}
				else {
					__printf(szout, "%s %d heap address:%x format error!\r\n", __FUNCTION__, __LINE__, addr);
				}
				result = TRUE;
				break;
			}
		}
	}

	__leaveSpinlock(tss->lpheap_lock);

	if (result == 0) {
		__printf(szout, "%s %d heap address:%x format error!\r\n", __FUNCTION__, __LINE__, addr);
	}
	return result;
}

int fast_heap_free(char* addr) {
	int result = 0;

	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetCurrentTaskTssBase();

	__enterSpinlock(tss->lpheap_lock);

	int bs[8] = { 16, 32, 64, 128, 256, 512, 1024, 2048 };
	int cnt[8] = { 0x1000,0x1000,0x1000,1024,256,256,128,64 };
	int size[8] = { 0x1000 * 0x10,0x1000 * 0x20,0x1000 * 0x40,0x400 * 0x80,0x100 * 0x100,0x100 * 0x200,0x80 * 0x400,0x40 * 0x800 };
	
	char* base = tss->fast_heap;

	for (int i = 0; i < 8; i++) {

		if (addr >= base && addr < base + size[i]) {
			int blockSize = bs[i];
			if (addr[blockSize - 1] & 0x80) {
				addr[blockSize - 1] = addr[blockSize - 1] & 0x7f;
				result = 1;
				break;
			}
			else {
				char szout[256];
				__printf(szout, "%s %d addr:%x size:%x error\r\n", __FUNCTION__, __LINE__, addr, blockSize);
				break;
			}
		}

		base = base + size[i];
	}
	
	__leaveSpinlock(tss->lpheap_lock);

	return result;
}

int init_fastheap() {
	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	__enterSpinlock(tss->lpheap_lock);

	char* ptr = 0;

	int bs[8] = { 16, 32, 64, 128, 256, 512, 1024, 2048 };
	int cnt[8] = { 0x1000,0x1000,0x1000,1024,256,256,128,64 };
	int size[8] = { 0x1000 * 0x10,0x1000 * 0x20,0x1000 * 0x40,0x400 * 0x80,0x100 * 0x100,0x100 * 0x200,0x80 * 0x400,0x40 * 0x800 };

	ptr = tss->fast_heap;
			
	for (int i = 0; i < 8; i++) {
		int num = cnt[i];
		int blockSize = bs[i];
		for (int j = 0; j < num; j++) {
			ptr[blockSize - 1] = ptr[blockSize - 1] & 0x7f;
			ptr += blockSize;
		}
	}

	__leaveSpinlock(tss->lpheap_lock);
	return 0;
}

char* fast_heap_alloc(int allocSize) {
	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	__enterSpinlock(tss->lpheap_lock);

	char* ptr = 0;

	char* base = tss->fast_heap;
	
	int bs[8] = { 16, 32, 64, 128, 256, 512, 1024, 2048 };
	int cnt[8] = { 0x1000,0x1000,0x1000,1024,256,256,128,64 };
	int size[8] = { 0x1000*0x10,0x1000*0x20,0x1000*0x40,0x400*0x80,0x100*0x100,0x100*0x200,0x80*0x400,0x40*0x800 };

	int seq = -1;
	for (int i = 0; i < 8; i++) {

		if (bs[i] > allocSize) {
			seq = i;
			break;
		}

		base += size[i];
	}

	if (seq != -1) {
		
		ptr = base;
		int blockSize = bs[seq];

		for (int j = 0; j < cnt[seq]; j++) {
			if (ptr[blockSize - 1] & 0x80) {
				ptr += blockSize;
			}
			else {
				ptr[blockSize - 1] = ptr[blockSize - 1] | 0x80;
				break;
			}
		}
	}

	__leaveSpinlock(tss->lpheap_lock);
	return ptr;
}


int CreateHeap() {
	int result = 0;
	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	__enterSpinlock(tss->lpheap_lock);
	
	int limit = (0x1000 - sizeof(PROCESS_INFO) - (*tss->lpHeapCnt - 1) * sizeof(DWORD) ) / sizeof(DWORD);
	if (limit > 0) {
		int seq = *tss->lpHeapCnt;

		tss->lpHeapBase[seq] = __kMalloc(tss->heapsize);
		__memset((char*)tss->lpHeapBase[seq], 0, tss->heapsize);

		seq++;

		*tss->lpHeapCnt = seq;
	}
	else {
		char szout[256];
		__printf(szout, "pid:%d sizeof(PROCESS_INFO):%x is too small to alloc new heap!\r\n",tss->pid, sizeof(PROCESS_INFO));
	}

	__leaveSpinlock(tss->lpheap_lock);
	return result;
}

//allocate size is ( 2*sizeof(MS_HEAP_STRUCT) + MS_HEAP_STRUCT.size)
DWORD __heapAlloc(int size) {

	DWORD addr = 0;
	char szout[256];

	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	__enterSpinlock(tss->lpheap_lock);

	int allocsize = getAlignSize(size, sizeof(MS_HEAP_STRUCT)*2);

	int cnt = *tss->lpHeapCnt;

	for (int i = 0; i < cnt; i++) {

		MS_HEAP_STRUCT* lpheap = (MS_HEAP_STRUCT*)tss->lpHeapBase[i];

		while ( allocsize + (sizeof(MS_HEAP_STRUCT) << 1) <= tss->heapsize)
		{
			int heapSize = (lpheap->size) & HEAP_BUFFER_FREE;
			if ((lpheap->size & HEAP_BUFFER_POSITION) && lpheap->size && lpheap->addr)
			{
				lpheap = (MS_HEAP_STRUCT*)( (UCHAR*)lpheap + heapSize + (sizeof(MS_HEAP_STRUCT) << 1) );
				continue;
			}
			else if ( ((lpheap->size & HEAP_BUFFER_POSITION) == 0) && lpheap->size && lpheap->addr)
			{
				if (heapSize > allocsize + (sizeof(MS_HEAP_STRUCT) * 2))
				{
					lpheap->addr = (DWORD)((DWORD)lpheap + sizeof(MS_HEAP_STRUCT));
					lpheap->size = allocsize | HEAP_BUFFER_POSITION;

					MS_HEAP_STRUCT* heapend = (MS_HEAP_STRUCT*)((DWORD)lpheap + sizeof(MS_HEAP_STRUCT) + (allocsize));
					heapend->addr = lpheap->addr;
					heapend->size = lpheap->size;

					MS_HEAP_STRUCT* next = (MS_HEAP_STRUCT*)((DWORD)lpheap + (allocsize)+(sizeof(MS_HEAP_STRUCT) * 2));
					next->size = (heapSize - allocsize - (sizeof(MS_HEAP_STRUCT) * 2)) & HEAP_BUFFER_FREE;
					next->addr = (DWORD)((DWORD)next + sizeof(MS_HEAP_STRUCT));

					MS_HEAP_STRUCT* nextEnd = (MS_HEAP_STRUCT*)((DWORD)next + sizeof(MS_HEAP_STRUCT) + (next->size & HEAP_BUFFER_FREE));
					nextEnd->size = next->size;
					nextEnd->addr = next->addr;

					addr = lpheap->addr;
					//__memset((char*)addr, 0, size);
					break;
				}
				else if (heapSize == allocsize) {
					lpheap->addr = (DWORD)((DWORD)lpheap + sizeof(MS_HEAP_STRUCT));
					lpheap->size = allocsize | HEAP_BUFFER_POSITION;

					MS_HEAP_STRUCT* heapend = (MS_HEAP_STRUCT*)((DWORD)lpheap + sizeof(MS_HEAP_STRUCT) + (allocsize));
					heapend->addr = lpheap->addr;
					heapend->size = lpheap->size;
					addr = lpheap->addr;
					//__memset((char*)addr, 0, size);
					break;
				}
				else {
					lpheap = (MS_HEAP_STRUCT*)((UCHAR*)lpheap + (lpheap->size) + (sizeof(MS_HEAP_STRUCT) << 1));
					continue;
				}
			}
			else if(lpheap->size == 0 && lpheap->addr == 0){
				lpheap->addr = (DWORD)((DWORD)lpheap + sizeof(MS_HEAP_STRUCT));
				lpheap->size = (allocsize) | HEAP_BUFFER_POSITION;

				MS_HEAP_STRUCT* heapend = (MS_HEAP_STRUCT*)((UCHAR*)lpheap + sizeof(MS_HEAP_STRUCT) + allocsize);
				heapend->addr = lpheap->addr;
				heapend->size = lpheap->size;

				addr = lpheap->addr;
				//__memset((char*)addr, 0, size);
				break;
			}
			else {
				lpheap = (MS_HEAP_STRUCT*)((UCHAR*)lpheap + (lpheap->size) + (sizeof(MS_HEAP_STRUCT) << 1));
				continue;
			}
		}
		if (addr) {
			break;
		}
	}

	__leaveSpinlock(tss->lpheap_lock);

	if (addr == 0) {
		__printf(szout, "%s %d addr:%x size:%x error\r\n", __FUNCTION__, __LINE__, addr, allocsize);
	}
	
	return addr;
}






