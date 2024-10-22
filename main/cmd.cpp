
#include "cmd.h"
#include "console.h"
#include "video.h"
#include "mouse.h"
#include "keyboard.h"
#include "task.h"
#include "graph.h"
#include "soundBlaster/sbPlay.h"
#include "core.h"
#include "Utils.h"
#include "menu.h"
#include "guihelper.h"
#include "Pe.h"
#include "window.h"
#include "cmosExactTimer.h"
#include "ata.h"

#include "Kernel.h"
#include "mainUtils.h"

#include "Utils.h"
#include "paint.h"
#include "malloc.h"
#include "Thread.h"
#include "servicesProc.h"
#include "pci.h"


#pragma comment(linker, "/STACK:0x100000")


int __cmd(char* cmd, WINDOWCLASS* window, char* pidname, int pid) {

	//cmd size is always less than 256 bytes
	char szout[0x1000];
	int ret = 0;

	TASKCMDPARAMS taskcmd;
	__memset((char*)&taskcmd, 0, sizeof(TASKCMDPARAMS));

	int cmdlen = __strlen(cmd);
	//upper2lower(cmd, cmdlen);

	char params[COMMAND_LINE_STRING_LIMIT][COMMAND_LINE_STRING_LIMIT];
	__memset((char*)params, 0, COMMAND_LINE_STRING_LIMIT * COMMAND_LINE_STRING_LIMIT);

	int paramcnt = parseCmdParams(cmd, params);

	if (__strcmp(params[0], "open") == 0 && paramcnt >= 2)
	{
		char* filename = params[1];
		int fnlen = __strlen(filename);
		if (__memcmp(filename + fnlen - 4, ".bmp", 4) == 0)
		{
			__strcpy(taskcmd.filename, filename);
			taskcmd.cmd = SHOW_WINDOW_BMP;
			//DWORD addr = getAddrFromName(MAIN_DLL_BASE, "__kShowWindow");
			//return __kCreateThread(addr, (DWORD)&taskcmd, "__kShowWindow");

			int imagesize = getSizeOfImage((char*)MAIN_DLL_SOURCE_BASE);
			return __kCreateProcess(MAIN_DLL_SOURCE_BASE, imagesize, "main.dll", "__kShowWindow", 3, (DWORD)&taskcmd);
		}
		else if (__memcmp(filename + fnlen - 4, ".jpg", 4) == 0 || __memcmp(filename + fnlen - 5, ".jpeg", 5) == 0)
		{
			__strcpy(taskcmd.filename, filename);
			taskcmd.cmd = SHOW_WINDOW_JPEG;
			//DWORD addr = getAddrFromName(MAIN_DLL_BASE, "__kShowWindow");
			//return __kCreateThread(addr, (DWORD)&taskcmd, "__kShowWindow");
			int imagesize = getSizeOfImage((char*)MAIN_DLL_SOURCE_BASE);
			return __kCreateProcess(MAIN_DLL_SOURCE_BASE, imagesize, "main.dll", "__kShowWindow", 3, (DWORD)&taskcmd);
		}
		else if (isTxtFile(filename, fnlen))
		{
			__strcpy(taskcmd.filename, filename);
			taskcmd.cmd = SHOW_WINDOW_TXT;
			//DWORD addr = getAddrFromName(MAIN_DLL_BASE, "__kShowWindow");
			//return __kCreateThread(addr, (DWORD)&taskcmd, "__kShowWindow");
			int imagesize = getSizeOfImage((char*)MAIN_DLL_SOURCE_BASE);
			return __kCreateProcess(MAIN_DLL_SOURCE_BASE, imagesize, "main.dll", "__kShowWindow", 3, (DWORD)&taskcmd);
		}
		else if (__memcmp(filename + fnlen - 4, ".zip", 4) == 0 || __memcmp(filename + fnlen - 4, ".apk", 4) == 0)
		{

		}
		else if (__memcmp(filename + __strlen(filename) - 4, ".exe", 4) == 0 ||
			__memcmp(filename + __strlen(filename) - 4, ".com", 4) == 0 ||
			__memcmp(filename + __strlen(filename) - 4, ".dll", 4) == 0 ||
			__memcmp(filename + __strlen(filename) - 4, ".sys", 4) == 0 ||
			__memcmp(filename + __strlen(filename) - 3, ".so", 3) == 0)
		{
			//dump filename function_name param
			if (paramcnt >= 3)
			{
				if (paramcnt >= 4)
				{
					return __kCreateProcessFromName(filename, params[2], 3, (DWORD)params[3]);
				}
				else {
					return __kCreateProcessFromName(filename, params[2], 3, 0);
				}
			}
			else {
				return __kCreateProcessFromName(filename, filename, 3, 0);
			}
		}
		else if (__memcmp(filename + fnlen - 4, ".wav", 4) == 0)
		{
			return playWavFile(filename);
		}
	}
	else if (__strcmp(params[0], "ls") == 0) {

	}
	else if (__strcmp(params[0], "cd") == 0) {

	}
	else if (__strcmp(params[0], "keyboardID") == 0)
	{
		*szout = 0;
		__sprintf(szout, "keyboard id:%x", gKeyboardID);
		ret = __drawWindowChars(( char*)&szout, CONSOLE_FONT_COLOR, window);
	}
	else if (__strcmp(params[0], "mouseID") == 0) {
		*szout = 0;
		__sprintf(szout, "mouse id:%x", gMouseID);
		ret = __drawWindowChars(( char*)&szout, CONSOLE_FONT_COLOR, window);
	}
	else if (__strcmp(params[0], "reg") == 0 && paramcnt >= 2)
	{
		if (__strcmp(params[1], "idt") == 0)
		{
			*szout = 0;
			getidt(szout);
			ret = __drawWindowChars(( char*)&szout, CONSOLE_FONT_COLOR, window);
		}
		else if (__strcmp(params[1], "gdt") == 0)
		{
			*szout = 0;
			getgdt(szout);
			ret = __drawWindowChars(( char*)&szout, CONSOLE_FONT_COLOR, window);
		}
		else if (__strcmp(params[1], "ldt") == 0)
		{
			*szout = 0;
			getldt(szout);
			ret = __drawWindowChars(( char*)&szout, CONSOLE_FONT_COLOR, window);
		}
		else if (__strcmp(params[1], "crs") == 0)
		{
			*szout = 0;
			getcrs(szout);
			ret = __drawWindowChars(( char*)&szout, CONSOLE_FONT_COLOR, window);
		}
		else if (__strcmp(params[1], "general") == 0)
		{
			*szout = 0;
			getGeneralRegs(szout);
			ret = __drawWindowChars(( char*)&szout, CONSOLE_FONT_COLOR, window);
		}
	}
	else if (__strcmp(params[0], "loadfiles") == 0)
	{
		DATALOADERINFO* info = (DATALOADERINFO*)(gKernelData << 4);
		__sprintf(szout,
			"flag:%s,mbr:%d,mbrbak:%d,loaderSec:%d,loaderSecCnt:%d,kSec:%d,kSecCnt:%d,kdllSec:%d,kdllSecCnt:%d,mdllSec:%d,mdllSecCnt:%d,fontSec:%d,fontSecCnt:%d\r\n",
			&info->_flags, info->_bakMbrSecOff, info->_bakMbr2SecOff, info->_loaderSecOff, info->_loaderSecCnt,
			info->_kernelSecOff, info->_kernelSecCnt, info->_kdllSecOff, info->_kdllSecCnt,
			info->_maindllSecOff, info->_maindllSecCnt, info->_fontSecOff, info->_fontSecCnt);
		ret = __drawWindowChars(( char*)&szout, CONSOLE_FONT_COLOR, window);
	}
	else if (__strcmp(params[0], "segments") == 0)
	{
		__sprintf(szout, "Kernel:%x,Kernel16:%x,KernelData:%x\r\n", gKernel32, gKernel16, gKernelData);
		ret = __drawWindowChars(( char*)&szout, CONSOLE_FONT_COLOR, window);
		return 0;
	}
	else if (__strcmp(params[0], "threads") == 0)
	{
		*szout = 0;
		getpids(szout);
		ret = __drawWindowChars(( char*)&szout, CONSOLE_FONT_COLOR, window);
	}
	else if (__strcmp(params[0], "thread") == 0)
	{
		if (paramcnt >= 2)
		{
			int i = __strd2i(params[1]);
			*szout = 0;
			getpid(i, szout);
			ret = __drawWindowChars(( char*)&szout, CONSOLE_FONT_COLOR, window);
		}
	}
	else if (__strcmp(params[0], "run") == 0)
	{
		if (paramcnt >= 2 && __strcmp(params[1], "paint") == 0)
		{
			__memset((char*)&taskcmd, 0, sizeof(TASKCMDPARAMS));
			int imagesize = getSizeOfImage((char*)MAIN_DLL_SOURCE_BASE);

			return __kCreateProcess(MAIN_DLL_SOURCE_BASE, imagesize, "main.dll", "__kPaint", 3, (DWORD)&taskcmd);
		}
	}
	else if (__strcmp(params[0], "tss") == 0) {
		LPPROCESS_INFO process = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
		DWORD eflags = 0;
		__asm {
			pushfd
			pop eflags
		}
		TssDescriptor* descrptor = (TssDescriptor*)(GDT_BASE + kTssTaskSelector);
		__sprintf(szout, "tid:%d, pid:%d ,link:%d,NT:%x,busy:%x\r\n",
			process->tid, process->pid, process->tss.link, process->tss.trap, eflags & 0x4000, descrptor->type & 2);
		ret = __drawWindowChars(( char*)&szout, CONSOLE_FONT_COLOR, window);
		process->tss.link = 0;
	}
	else if (__strcmp(params[0], "alloc") == 0)
	{
		if (paramcnt >= 3)
		{
			int size = __strh2i((unsigned char*)params[1]);
			int cnt = __strh2i((unsigned char*)params[2]);
			for (int i = 0; i < cnt; i++)
			{
				DWORD addr = __malloc(size);
				__sprintf(szout, "malloc size:%x,address:%x\r\n", size, addr);
				ret = __drawWindowChars(( char*)&szout, CONSOLE_FONT_COLOR, window);
			}
		}
		else if (paramcnt >= 2)
		{
			int size = __strh2i((unsigned char*)params[1]);
			DWORD addr = __malloc(size);
			__sprintf(szout, "malloc size:%x,address:%x\r\n", size, addr);
			ret = __drawWindowChars(( char*)&szout, CONSOLE_FONT_COLOR, window);
		}
	}
	else if (__strcmp(params[0], "free") == 0)
	{
		if (paramcnt >= 2)
		{
			DWORD addr = __strh2i((unsigned char*)params[1]);
			int size = __free(addr);
			__sprintf(szout, "free size:%x,address:%x\r\n", size, addr);
			ret = __drawWindowChars(( char*)&szout, CONSOLE_FONT_COLOR, window);
		}
	}
	else if (__strcmp(params[0], "dumpm") == 0)
	{
		if (paramcnt >= 2)
		{
			DWORD addr = __strh2i((unsigned char*)params[1]);

			int len = 0x40;

			if (paramcnt >= 3)
			{
				len = __strh2i((unsigned char*)params[2]);
			}
			__dump((char*)addr, len, TRUE, (unsigned char*)szout);

			ret = __drawWindowChars(( char*)&szout, CONSOLE_FONT_COLOR, window);
		}
	}
	else if (__strcmp(params[0], "editm") == 0)
	{
		if (paramcnt >= 3)
		{
			DWORD addr = __strh2i((unsigned char*)params[1]);
			__strcpy((char*)addr, params[2]);
		}
	}

	else if (__strcmp(params[0], "memlist") == 0 && paramcnt >= 2)
	{
		int pid = __strh2i((unsigned char*)params[1]);
		*szout = 0;
		int len = getmemmap(pid, szout);
		ret = __drawWindowChars(( char*)&szout, CONSOLE_FONT_COLOR, window);
	}
	else if (__strcmp(params[0], "testme") == 0)
	{
		__strcpy(taskcmd.filename, params[0]);
		taskcmd.cmd = SHOW_TEST_WINDOW;
		int imagesize = getSizeOfImage((char*)MAIN_DLL_SOURCE_BASE);
		return __kCreateProcess(MAIN_DLL_SOURCE_BASE, imagesize, "main.dll", "__kShowWindow", 3, (DWORD)&taskcmd);
	}
	else if (__strcmp(params[0], "timer0Tick") == 0)
	{
		DWORD cnt = *((DWORD*)TIMER0_TICK_COUNT);
		__sprintf(szout, "%x\r\n", cnt);
		ret = __drawWindowChars(( char*)&szout, CONSOLE_FONT_COLOR, window);
	}
	else if (__strcmp(params[0], "cmosPeriod") == 0)
	{
		DWORD cnt = *((DWORD*)CMOS_PERIOD_TICK_COUNT);
		__sprintf(szout, "%x\r\n", cnt);
		ret = __drawWindowChars((char*)&szout, CONSOLE_FONT_COLOR, window);
	}
	else if (__strcmp(params[0], "cmosExact") == 0)
	{
		DWORD cnt = *((DWORD*)CMOS_EXACT_TICK_COUNT);
		__sprintf(szout, "%x\r\n", cnt);
		ret = __drawWindowChars((char*)&szout, CONSOLE_FONT_COLOR, window);
	}
	else if (__strcmp(params[0], "time") == 0)
	{
		__sprintf(szout, "%s\n", (char*)CMOS_DATETIME_STRING);
		ret = __drawWindowChars(( char*)&szout, CONSOLE_FONT_COLOR, window);
	}
	else if (__strcmp(params[0], "rdtsc") == 0)
	{	
		__sprintf(szout, "rdtsc:%I64x\n", __krdtsc());
		ret = __drawWindowChars(( char*)&szout, CONSOLE_FONT_COLOR, window);
	}
	else if (__strcmp(params[0], "rdpmc") == 0 && paramcnt>=2)
	{
		DWORD num = __strh2i((unsigned char*)params[1]);

		unsigned __int64 res = 0;
		__asm {
			mov eax, num
			mov ecx,num
			rdpmc
			mov dword ptr [res], eax
			mov dword ptr [res+4], edx
		}
		__sprintf(szout, "rdpmc:%i64x\n", res);
		ret = __drawWindowChars(( char*)&szout, CONSOLE_FONT_COLOR, window);
	}
	else if (__strcmp(params[0], "temperature") == 0)
	{
		DWORD tj = 0;
		DWORD temp = __readTemperature(&tj);
		__sprintf(szout, "tj:%x,temperature:%d\n", temp);
		ret = __drawWindowChars(( char*)&szout, CONSOLE_FONT_COLOR, window);
	}
	else if (__strcmp(params[0], "exit") == 0)
	{
		__DestroyWindow(window);
		return 0;
	}
	else if (__strcmp(params[0], "cls") == 0)
	{
		__DestroyWindow(window);
		//__drawWindow(window, FALSE);
		initConsoleWindow(window, pidname, pid);
	}
	else if (__strcmp(params[0], "reset") == 0)
	{
		__reset();
	}
	else if (__strcmp(params[0], "inport") == 0 || __strcmp(params[0], "outpport") == 0)
	{
		DWORD port = __strh2i((unsigned char*)params[1]);
			
		if (__strcmp(params[0], "inport") == 0)
		{
			__asm {
				mov edx, port
				in eax, dx
			}
		}
		else if (__strcmp(params[0], "outport") == 0)
		{
			DWORD num = __strh2i((unsigned char*)params[2]);
			__asm {
				mov edx, port
				mov eax, num
				out dx, eax
			}
		}
	}
	else if (__strcmp(params[0], "cpu") == 0)
	{
		char cpuinfo[256];
		char cputype[256];
		getCpuInfo(cpuinfo);
		getCpuType(cputype);

		__sprintf(szout, "cpu brand:%s,type:%s\n", cpuinfo, cputype);
		ret = __drawWindowChars(( char*)&szout, CONSOLE_FONT_COLOR, window);
	}
	else if (__strcmp(params[0], "pcidev") == 0)
	{
		unsigned long devbuf[1024];
		int cnt = listpci(devbuf);
		if (cnt > 0)
		{
			for (int i = 0; i < cnt; )
			{
				char szout[1024];
				__sprintf(szout, "\npci type:%x,device:%x\n", devbuf[i], devbuf[i + 1]);

				i += 2;

				ret = __drawWindowChars((char*)szout, CONSOLE_FONT_COLOR, window);
			}
		}
	}
	else if (__strcmp(params[0], "hdseq") == 0) {
		char seq[1024];
		getIdeSeq(seq);
		ret = __drawWindowChars((char*)&seq, CONSOLE_FONT_COLOR, window);
	}
	else if (__strcmp(params[0], "hdversion") == 0) {
		char seq[1024];
		getIdeFirmVersion(seq);
		ret = __drawWindowChars((char*)&seq, CONSOLE_FONT_COLOR, window);
	}
	else if (__strcmp(params[0], "hdtype") == 0) {
		char seq[1024];
		getIdeType(seq);
		ret = __drawWindowChars((char*)&seq, CONSOLE_FONT_COLOR, window);
	}
	else if (__strcmp(params[0], "hdmedium") == 0) {
		char seq[1024];
		getIdeMediumSeq(seq);
		ret = __drawWindowChars((char*)&seq, CONSOLE_FONT_COLOR, window);
	}
	else if (__strcmp(params[0], "vga") == 0) {
		ret = __drawWindowChars((char*)getVGAInfo(), CONSOLE_FONT_COLOR, window);
	}
	else if (__strcmp(params[0], "vesaMode") == 0) {
		char mode[1024];
		__sprintf(mode,"%d\r\n", gVideoMode);
		ret = __drawWindowChars((char*)mode, CONSOLE_FONT_COLOR, window);
	}
	else {
		ret = __drawWindowChars((char*)"Unrecognized command!\r\n", CONSOLE_FONT_COLOR, window);
	}
	return 0;
}



int parseCmdParams(char* cmd, char params[COMMAND_LINE_STRING_LIMIT][COMMAND_LINE_STRING_LIMIT]) {
	char* str = cmd;
	int counter = 0;

	int tag = FALSE;

	for (int i = 0; i <= __strlen(cmd); i++)
	{
		if (cmd[i] == ' ' || cmd[i] == 0)
		{
			if (tag)
			{
				int len = &cmd[i] - str;
				if (len > 0 && len < COMMAND_LINE_STRING_LIMIT)
				{
					__memcpy(params[counter], str, len);
					*(params[counter] + len) = 0;
					counter++;
				}
				else {
					//show error
				}

				//str = cmd + i + 1;

				tag = FALSE;
			}
		}
		else {
			if (tag == FALSE)
			{
				tag = TRUE;
				str = &cmd[i];
			}
		}
	}

	return counter;
}