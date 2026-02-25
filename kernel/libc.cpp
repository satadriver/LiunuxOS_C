
#include "libc.h"
#include "video.h"
#include "process.h"
#include "def.h"
#include "window.h"
#include "apic.h"


int printf(const char* format, ...) {
	char buf[1024];
	if (g_ScreenMode == 0) {
		return 0;
	}

	if (format == 0 ) {
		return FALSE;
	}

	int formatLen = __strlen((char*)format);
	if (formatLen == 0) {
		return FALSE;
	}

	void* params = 0;
	int param_cnt = 0;
	__asm {
		lea eax, format
		add eax, 4
		mov params, eax
	}

	int len = __kFormat(buf, (char*)format, (DWORD*)params);
	LPPROCESS_INFO proc = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	int cpu = __GetCurrentCpu();
	LPWINDOWSINFO winfo = __FindProcessWindow(proc->tid, cpu);
	if (winfo) {
		LPWINDOWCLASS window = winfo->window;
		if (window) {
			__drawWindowChars(buf, 0, window);
		}
		else {
			int endpos = __drawGraphChars((char*)buf, 0);
		}
	}
	else {
		int endpos = __drawGraphChars((char*)buf, 0);
	}

	return len;
}