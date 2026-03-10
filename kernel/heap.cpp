
#ifdef _DEBUG
#include "heap.h"
#include "malloc.h"
#include "process.h"
#include "memory.h"
#include "Utils.h"
#include "apic.h"
#include <stdlib.h>
#else
#include "heap.h"
#include "malloc.h"
#include "process.h"
#include "memory.h"
#include "Utils.h"
#include "apic.h"
#endif

#define HEAP_BUFFER_POSITION	0X80000000			//combind buffer 

#define HEAP_BUFFER_FREE		0X7FFFFFFF



int InitHeap(int pid) {

	return 0;
}





DWORD __heapFree(char * heapBase,int heapLimit,DWORD addr) {
	char szout[256];

	DWORD result = 0;

	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetCurrentTaskTssBase();

	__enterSpinlock(tss->lpheap_lock);

	if (addr >= (DWORD)heapBase && addr < (DWORD)heapBase + heapLimit) {

		MS_HEAP_STRUCT* heap = (MS_HEAP_STRUCT*)((UCHAR*)addr - sizeof(MS_HEAP_STRUCT));

		int heapSize = heap->size & HEAP_BUFFER_FREE;
		int heapHdrSize = sizeof(MS_HEAP_STRUCT) << 1;

		MS_HEAP_STRUCT* heapEnd = (MS_HEAP_STRUCT*)((UCHAR*)heap + sizeof(MS_HEAP_STRUCT) + heapSize);

		if ((heap->addr == addr && heapEnd->addr == addr) && (heap->size == heapEnd->size))
		{
			heap->size = heapSize;

			int nextSize = 0;
			int prevSize = 0;
			int prevHdrSize = 0;
			int nextHdrSize = 0;

			MS_HEAP_STRUCT* prevEnd = 0;
			MS_HEAP_STRUCT* prev = 0;
			if ((DWORD)heap <= (DWORD)heapBase) {
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
			if ((DWORD)next >= (DWORD)heapBase + heapLimit) {
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
				//__printf(szout, "%s %d heap address:%x format error!\r\n", __FUNCTION__, __LINE__, addr);
			}
			result = TRUE;
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

	int bs[8] =		{ 16,			32,				64,				128,			256,			512,			1024,			2048 };
	int cnt[8] =	{ 0x1000,		0x1000,			0x1000,			1024,			256,			256,			128,			64 };
	int size[8] =	{ 0x1000 * 0x10,0x1000 * 0x20,	0x1000 * 0x40,	0x400 * 0x80,	0x100 * 0x100,	0x100 * 0x200,	0x80 * 0x400,	0x40 * 0x800 };
	
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
				break;
			}
		}

		base = base + size[i];
	}
	
	__leaveSpinlock(tss->lpheap_lock);

	if (result == 0) {
		char szout[256];
		__printf(szout, "%s %d addr:%x error\r\n", __FUNCTION__, __LINE__, addr);
	}
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

int fast_heap_free_large(char* addr) {
	int result = 0;

	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetCurrentTaskTssBase();

	__enterSpinlock(tss->lpheap_lock);

	int bs[8] = { 16, 32, 64, 128, 256, 512, 1024, 2048 };
	int cnt[8] = { 0x1000,0x1000,0x1000,1024,256,256,128,64 };
	int size[8] = { 0x1000 * 0x10,0x1000 * 0x20,0x1000 * 0x40,0x400 * 0x80,0x100 * 0x100,0x100 * 0x200,0x80 * 0x400,0x40 * 0x800 };

	char* base = tss->fast_heap_large;

	for (int i = 0; i < 8; i++) {

		if (addr >= base && addr < base + size[i]) {
			int blockSize = bs[i];
			if (addr[blockSize - 1] & 0x80) {
				addr[blockSize - 1] = addr[blockSize - 1] & 0x7f;
				result = 1;
				break;
			}
			else {
				break;
			}
		}

		base = base + size[i];
	}

	__leaveSpinlock(tss->lpheap_lock);

	if (result == 0) {
		char szout[256];
		__printf(szout, "%s %d addr:%x error\r\n", __FUNCTION__, __LINE__, addr);
	}
	return result;
}

char* fast_heap_alloc_large(int allocSize) {

	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetCurrentTaskTssBase();

	__enterSpinlock(tss->lpheap_lock);

	char* ptr = 0;

	char* base = tss->fast_heap_large;

	int bs[8] = { 0x1000, 0x2000, 0x4000, 0x8000, 0x10000, 0x20000 ,0x40000,0x80000};
	int cnt[8] = { 0x100, 0x100,  0x80,   0x40,   0x20,    0x10 ,   0x10,   2};
	int size[8] = { 0x100000,0x200000,0x200000,0x200000,0x200000,0x200000,0x400000,0x100000 };

	int seq = -1;
	for (int i = 0; i < 8; i++) {

		if (bs[i] > allocSize) {
			seq = i;
			break;
		}

		base += size[i];
	}

	if (seq != -1) {

		int blockSize = bs[seq];

		for (int j = 0; j < cnt[seq]; j++) {
			if (base[blockSize - 1] & 0x80) {
				base += blockSize;
			}
			else {
				base[blockSize - 1] = base[blockSize - 1] | 0x80;
				ptr = base;
				break;
			}
		}
	}

	__leaveSpinlock(tss->lpheap_lock);
	return ptr;
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
		
		int blockSize = bs[seq];

		for (int j = 0; j < cnt[seq]; j++) {
			if (base[blockSize - 1] & 0x80) {
				base += blockSize;
			}
			else {
				base[blockSize - 1] = base[blockSize - 1] | 0x80;
				ptr = base;
				break;
			}
		}
	}

	__leaveSpinlock(tss->lpheap_lock);
	return ptr;
}


int CreateHeap() {
	int seq = 0;
	char szout[256];
	char* buf = 0;
	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	__enterSpinlock(tss->lpheap_lock);
	
	int tss_limit = (sizeof(PROCESS_INFO) + 0xfff) & 0xfffff000;
	int limit = (tss_limit - sizeof(PROCESS_INFO) - (*tss->lpHeapCnt - 1) * sizeof(DWORD) ) / sizeof(DWORD);
	if (limit > 0) {
		seq = *tss->lpHeapCnt;
		int allocSize = tss->heapsize << seq;
#ifdef _DEBUG
		buf = (char*)malloc(allocSize);

#else		
		buf = (char*)__kMalloc(allocSize);
#endif
		if (buf) {
			tss->lpHeapBase[seq] = buf;
			__memset((char*)buf, 0, allocSize);

			*tss->lpHeapCnt = seq + 1;
		}
		else {
			__printf(szout, "%s %d pid:%d malloc buffer error\r\n",__FUNCTION__,__LINE__, tss->pid);
		}
	}
	else {
		__printf(szout, "pid:%d sizeof(PROCESS_INFO):%x is too small to alloc new heap!\r\n",tss->pid, sizeof(PROCESS_INFO));
		seq = 0;
	}

	__leaveSpinlock(tss->lpheap_lock);
	return seq;
}

//allocate size is ( 2*sizeof(MS_HEAP_STRUCT) + MS_HEAP_STRUCT.size)
DWORD __heapAlloc(char * heapBase,int heapLimit, int size) {

	DWORD addr = 0;
	char szout[256];

	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetCurrentTaskTssBase();

	__enterSpinlock(tss->lpheap_lock);

	int allocSize = getAlignSize(size, sizeof(MS_HEAP_STRUCT)*2);

	MS_HEAP_STRUCT* lpheap = (MS_HEAP_STRUCT*)heapBase;

	while ((DWORD)lpheap + allocSize + (sizeof(MS_HEAP_STRUCT) << 1) <= (DWORD) heapBase + heapLimit)
	{
		int heapSize = (lpheap->size) & HEAP_BUFFER_FREE;
		if ((lpheap->size & HEAP_BUFFER_POSITION) && lpheap->size && lpheap->addr)
		{
			lpheap = (MS_HEAP_STRUCT*)( (UCHAR*)lpheap + heapSize + (sizeof(MS_HEAP_STRUCT) << 1) );
			continue;
		}
		else if ( ((lpheap->size & HEAP_BUFFER_POSITION) == 0) && lpheap->size && lpheap->addr)
		{
			if (heapSize > allocSize + (int)(sizeof(MS_HEAP_STRUCT) * 2))
			{
				lpheap->addr = (DWORD)((DWORD)lpheap + sizeof(MS_HEAP_STRUCT));
				lpheap->size = allocSize | HEAP_BUFFER_POSITION;

				MS_HEAP_STRUCT* heapend = (MS_HEAP_STRUCT*)((DWORD)lpheap + sizeof(MS_HEAP_STRUCT) + (allocSize));
				heapend->addr = lpheap->addr;
				heapend->size = lpheap->size;

				MS_HEAP_STRUCT* next = (MS_HEAP_STRUCT*)((DWORD)lpheap + (allocSize)+(sizeof(MS_HEAP_STRUCT) * 2));
				next->size = (heapSize - allocSize - (sizeof(MS_HEAP_STRUCT) * 2)) & HEAP_BUFFER_FREE;
				next->addr = (DWORD)((DWORD)next + sizeof(MS_HEAP_STRUCT));

				MS_HEAP_STRUCT* nextEnd = (MS_HEAP_STRUCT*)((DWORD)next + sizeof(MS_HEAP_STRUCT) + (next->size & HEAP_BUFFER_FREE));
				nextEnd->size = next->size;
				nextEnd->addr = next->addr;

				addr = lpheap->addr;

				break;
			}
			else if (heapSize == allocSize) {
				lpheap->addr = (DWORD)((DWORD)lpheap + sizeof(MS_HEAP_STRUCT));
				lpheap->size = allocSize | HEAP_BUFFER_POSITION;

				MS_HEAP_STRUCT* heapend = (MS_HEAP_STRUCT*)((DWORD)lpheap + sizeof(MS_HEAP_STRUCT) + (allocSize));
				heapend->addr = lpheap->addr;
				heapend->size = lpheap->size;
				addr = lpheap->addr;

				break;
			}
			else {
				lpheap = (MS_HEAP_STRUCT*)((UCHAR*)lpheap + heapSize + (sizeof(MS_HEAP_STRUCT) << 1));
				continue;
			}
		}
		else if(lpheap->size == 0 && lpheap->addr == 0){
			lpheap->addr = (DWORD)((DWORD)lpheap + sizeof(MS_HEAP_STRUCT));
			lpheap->size = (allocSize) | HEAP_BUFFER_POSITION;

			MS_HEAP_STRUCT* heapend = (MS_HEAP_STRUCT*)((UCHAR*)lpheap + sizeof(MS_HEAP_STRUCT) + allocSize);
			heapend->addr = lpheap->addr;
			heapend->size = lpheap->size;

			addr = lpheap->addr;
			break;
		}
		else {
			//lpheap = (MS_HEAP_STRUCT*)((UCHAR*)lpheap + heapSize + (sizeof(MS_HEAP_STRUCT) << 1));
			//continue;
			//__printf(szout, "%s %d heap alloc size:%x heap base:%x heap limit:%x heap:%x heap size:%x error\r\n", 
				//__FUNCTION__, __LINE__, allocSize, heapBase, heapLimit, lpheap->addr,lpheap->size);
			break;
		}
	}

	__leaveSpinlock(tss->lpheap_lock);

	if (addr == 0) {
		//__printf(szout, "%s %d heap alloc size:%x heap base:%x heap limit:%x heap:%x heap size:%x error\r\n",
			//__FUNCTION__, __LINE__, allocSize, heapBase, heapLimit, lpheap->addr, lpheap->size);
		//addr = 0;
	}
	
	return addr;
}






