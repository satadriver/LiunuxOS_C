



#ifdef _DEBUG
#include "apic.h"
#include "task.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LPPROCESS_INFO g_proc_info = 0;

LPPROCESS_INFO DebugCreateProcess() {
	if (g_proc_info == 0) {
		g_proc_info = (LPPROCESS_INFO)malloc(0x10000);
		//SIZE_T size = 0x10000;
		//g_proc_info = (LPPROCESS_INFO)VirtualAlloc(0, 0x10000, MEM_COMMIT, PAGE_READWRITE);
		memset((char*)g_proc_info, 0, sizeof(PROCESS_INFO));
		int debug_heap_size = 0x100000;
		g_proc_info->fast_heap = (char*)malloc(debug_heap_size);
		g_proc_info->fast_heap_large = 0;
		//g_proc_info->fast_heap_large = (char*)malloc(debug_heap_size);

		int total = 6;
		g_proc_info->lpHeapBase = (char**)&g_proc_info->heapBase;
		for (int num = 0; num < total; num++) {
			char* buf = (char*)malloc(debug_heap_size << num);
			if (buf) {
				g_proc_info->lpHeapBase[num] = buf;
			}
			else {
				break;
			}
		}
		g_proc_info->heapCnt = total;

		g_proc_info->lpHeapCnt = &g_proc_info->heapCnt;
		g_proc_info->heapsize = debug_heap_size;
		g_proc_info->heap_lock = 0;
		g_proc_info->lpheap_lock = &g_proc_info->heap_lock;
		g_proc_info->va_size = 0;
		g_proc_info->lpvasize = &g_proc_info->va_size;
	}
	return 0;
}





#endif