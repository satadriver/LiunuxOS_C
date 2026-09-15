#include "descriptor.h"
#include "def.h"
#include "process.h"
#include "Utils.h"
#include "Pe.h"
#include "malloc.h"
#include "page.h"
#include "video.h"
#include "Utils.h"
#include "Kernel.h"
#include "apic.h"
#include "systemService.h"
#include "device.h"

//reference:https://zhuanlan.zhihu.com/p/678582574

//Extended Feature Enable Register(EFER) is a model - specific register added in the AMD K6 processor, 
//to allow enabling the SYSCALL / SYSRET instruction, and later for entering and exiting long mode.
//This register becomes architectural in AMD64 and has been adopted by Intel.Its MSR number is 0xC0000080.
void EnableSyscall() {
	DWORD highpart, lowpart;
	readmsr(0xC0000080, &lowpart, &highpart);

	//readmsr(0x1f80, &highpart, &lowpart);

	__asm {
		mov eax, [lowpart]
		or eax, 0x001
		mov[lowpart], eax
	}

	writemsr(0xC0000080, lowpart, highpart);
}

void EnableNXE() {
	DWORD highpart, lowpart;
	readmsr(0xC0000080, &lowpart, &highpart);

	//readmsr(0x1f80, &highpart, &lowpart);

	__asm {
		mov eax, [lowpart]
		or eax, 0x800
		mov[lowpart], eax
	}

	writemsr(0xC0000080, lowpart, highpart);
}


//长调用最终调用在哪里.是由调用门(段描述符)来指定的.而不是EIP.EIP是废弃的
extern "C" __declspec(naked) void __kCallGateProc(DWORD  params, DWORD count) {

	__asm {
		mov ebp, esp
		sub esp, NATIVE_STACK_LIMIT
	}

	{
		char szout[256];
		__printf(szout, "%s %d param1:%x,param2:%x\r\n",__FUNCTION__,__LINE__, params, count);
	}

	__asm {
		mov esp, ebp

		//mov eax, ss: [esp + 4]
		//mov ss : [esp + 8],eax
		//mov eax,ss:[esp ]
		//mov ss:[esp + 4],eax
		//pushfd
		//pop eax
		//mov ss : [esp + 12] , eax
		//add esp,4
		//iretd

		retf 0x08		//ca 08 00		在长调用中使用retf，这点需要注意.
	}
	/*
	ret（近返回）：
	C3（无操作数）
	C2 imm16（带栈调整，如 ret 4）
	retf（远返回）：
	CB（无操作数）
	CA imm16（带栈调整，如 retf 8）
	*/

	//RET immed16:		C2 
	//RET :				C3 
	//RETF immed16:		CA 
	//RETF :			CB 
	//IRET :			CF 
	//IRET [bits 16]:	CF 
	//IRETD :			66 CF

	//机器码对应表：
	//https://defuse.ca/online-x86-assembler.htm#disassembly
}



extern "C" __declspec(dllexport) void callgateEntry(char*  params,DWORD count) {

	__asm {
		pushfd
		pushad
		push ds
		push es
		push fs
		push gs
		push ss

		//cli

		push dword ptr count
		push params

		_emit 0x9a

		_emit 0
		_emit 0
		_emit 0
		_emit 0

		_emit callGateSelector
		_emit 0

		//retf 0x08 will balance the stack
		//add esp,8

		//sti

		pop ss
		pop gs
		pop fs
		pop es
		pop ds
		popad
		popfd
	}

	char szout[256];
	__printf(szout, "callgateEntry leave\r\n");

#if 0
 	CALL_LONG calllong;
 	calllong.callcode = 0x9a;
 	calllong.seg = seg;
 	calllong.offset = (DWORD)__kCallGateProc;
 	__asm {
 		lea eax, calllong
 		jmp eax
 	}
#endif
}











void readmsr(DWORD num, DWORD *lowpart, DWORD * highpart) {
	__asm {
		xor eax, eax
		xor edx, edx

		mov ecx, num
		rdmsr

		mov ecx, lowpart
		mov[ecx], eax

		mov ecx, highpart
		mov[ecx], edx
	}

	//char szout[256];
	//__printf(szout, "read msr:%x,high:%x,low:%x\r\n", num, *highpart, *lowpart);

}

void writemsr(DWORD num, DWORD lowpart, DWORD highpart) {
	__asm {
		mov ecx, num

		mov eax, lowpart

		mov edx, highpart

		wrmsr
	}
}

void syscall() {

}

void sysleave() {

}



DWORD g_sysEntryInit = 0;

DWORD g_sysEntryStack3 = 0;

DWORD g_sysEntryEip3 = 0;







extern "C" __declspec(naked) int SysenterEntry(char * params,int cnt) {
	__asm {
		push 0
		push ebp
		mov ebp,esp
		sub esp, NATIVE_STACK_LIMIT
	}

	{
		WORD rcs = 0;
		DWORD resp = 0;
		WORD rss = 0;
		DWORD reip = 0;

		__asm {
			mov ax, cs
			mov rcs, ax

			call __eip_value
			__eip_value :
			pop eax
			mov reip,eax

			mov ax, ss
			mov rss, ax

			mov resp, esp
		}

		char szout[256];
		__printf(szout, "%s %d cs:%x,eip:%x,ss:%x,esp:%x,params:%x,cnt:%x\r\n",__FUNCTION__,__LINE__, rcs, reip, rss, resp+ NATIVE_STACK_LIMIT + 8,params,cnt);
		//__printf(szout, "%s %d cs:%x,eip:%x,ss:%x,esp:%x,params:%x,cnt:%x\r\n", __FUNCTION__, __LINE__, rcs, reip, rss, resp);
		
	}

	__asm {
		mov esp, ebp
		add esp, 8
		nop
		mov edx, ds: [g_sysEntryEip3]
		nop
		mov ecx, ds : [g_sysEntryStack3]
		nop

		_emit 0x0f
		_emit 0x35
	}
}



//only be invoked in ring3,in ring0 will cause exception 0dh
extern "C" __declspec(dllexport) int SysenterProc(char * params,int cnt) {

	if (g_sysEntryInit == 0) {
		int res = SysenterInit((DWORD)SysenterEntry);
		if (res) {
			g_sysEntryInit = TRUE;
		}
		else {
			return 0;
		}
	}
	
	__asm {
		mov ax, cs
		test ax, 3
		jz __sysEntryExit

		pushfd
		pushad
		push ds
		push es
		push fs
		push gs
		push ss

		mov edi, SYSENTER_STACK_TOP
		mov eax, params
		mov [edi],eax
		mov eax, cnt
		mov [edi+4],eax

		mov ds : [g_sysEntryStack3] , esp
		lea eax, __sysEntryExit
		mov ds:[g_sysEntryEip3],eax

		//cli

		_emit 0x0f
		_emit 0x34

		__sysEntryExit :

		//sti

		pop ss
		pop gs
		pop fs
		pop es
		pop ds
		popad
		popfd
	}	
}

int SysenterInit(DWORD entryaddr) {
	WORD regcs = 0;
	__asm {
		mov ax, cs
		mov regcs, ax
	}
	if (regcs & 3)
	{
		return FALSE;
	}

	DWORD csseg = KERNEL_MODE_CODE;

	DWORD high = 0;

	writemsr(0x174, csseg, high);

	DWORD esp0 = SYSENTER_STACK_TOP;

	writemsr(0x175, esp0, high);

	DWORD eip = (DWORD)entryaddr;

	writemsr(0x176, eip, high);

	g_sysEntryInit = TRUE;

	return TRUE;
}

int g_pm_enable = FALSE;


int IsPmEnable() {
	int ver = 0;
	__asm {
		mov eax, 0ah
		cpuid
		mov[ver], eax
	}

	return ver&0xff;
}

int GetApicTimerDivideCode(int divide) {
	if (divide == 16) {
		return 3;
	}
	else if (divide == 32) {
		return 8;
	}
	else if (divide == 64) {
		return 9;
	}
	else if (divide == 128) {
		return 0x0a;
	}
	else if(divide == 1) {
		return 0x0b;
	}
	else {
		return -1;
	}
}

int GetPmVersion() {
	int ver = IsPmEnable();
	if (ver == 0)
	{
		return 0;
	}

	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	unsigned long long tick = __krdtsc();

	DWORD low = 0;
	DWORD high = 0;
	readmsr(MSR_IA32_APERF, &low, &high);
	if(low || high ) {
		g_pm_enable = g_pm_enable| TRUE;
		unsigned long long aprf = ((unsigned long long)high << 32) + low;

		g_cpu_tick[id] = tick - aprf;
		g_cpu_prev_tick[id] = tick;
		g_cpu_start_tick[id] = tick;
	}
	else {
		g_cpu_tick[id] = 0;
		g_cpu_prev_tick[id] = tick;
		g_cpu_start_tick[id] = tick;
	}

	char szout[256];
	__printf(szout, "%s %d performance monitor version: %x\r\n",__FUNCTION__,__LINE__, ver);
	return ver;
}



int InitPm() {
	int ver = IsPmEnable();
	if (ver == 0)
	{
		return 0;
	}

	unsigned long low = 0;
	unsigned long high = 0;
	low = 0x5300c0;
	//low = 0x0043003c;
	writemsr(0x186, low, 0);		//IA32_PERFEVTSELx

	writemsr(0xc1, low, high);
	readmsr(0xc1, &low, &high);

	unsigned long long tick = __krdtsc();
	
	char szout[256];
	__printf(szout, "%s %d performance monitor counter low:%x,high:%x,rdtsc:%I64x\r\n", __FUNCTION__, __LINE__, low,high,tick);
	return 0;
}


int GetCpuRate() {
	int ver = GetPmVersion();
	if (ver == 0) {
		return 0;
	}

	unsigned long e7low = 0;
	unsigned long e7high = 0;

	readmsr(0xe7, &e7low,& e7high);

	unsigned long e8low = 0;
	unsigned long e8high = 0;

	readmsr(0xe8, &e8low, &e8high);

	unsigned long long tick = __krdtsc();

	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

	char szout[256];
	__printf(szout, "%s %d performance monitor counter e7low:%x,e7high:%x ,e8low:%x,e8high:%x,rdtsc:%I64x\r\n",
		__FUNCTION__, __LINE__, e7low, e7high, e8low, e8high, tick);
	return 0;

}

//IA32_FIXED_CTR_CTRL 0x38d
//IA32_FIXED_CTR0	//0x309

//IA32_APERF	//0xe8
//IA32_MPERF	//0xe7


IPI_MSG_PARAM* GetIpiMsg() {
	IPI_MSG_PARAM* msg = (IPI_MSG_PARAM*)IPI_MSG_BASE;

	int cnt = 0x10000 / sizeof(IPI_MSG_PARAM);
	for(int i = 0; i < cnt; i++) {
		if (msg[i].valid == 0) {
			return &msg[i];
		}
	}
	return 0;
}


IPI_MSG_PARAM* SetIpiMsg() {
	IPI_MSG_PARAM* msg = (IPI_MSG_PARAM*)IPI_MSG_BASE;

	int cnt = 0x10000 / sizeof(IPI_MSG_PARAM);
	for (int i = 0; i < cnt; i++) {
		if (msg[i].valid == 0) {
			msg[i].valid = 1;
			return &msg[i];
		}
	}
	return 0;
}


int IncreaseDelta(int v) {
	if (v == 0) {
		v = 1;
	}
	extern int g_task_array_lock[256];
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	__enterSpinlock(&g_task_array_lock[id]);
	PROCESS_INFO* tss = GetTaskTssBase();
	PROCESS_INFO* current = GetCurrentTaskTssBase();
	LPPROCESS_INFO proc = (LPPROCESS_INFO)(tss + current->tid);
	proc->priority = proc->priority | v;
	current->priority = proc->priority;
	proc->authority+=v;
	if (proc->authority > DYNAMIC_PRIORITY) {
		proc->authority = DYNAMIC_PRIORITY;
	}
	current->authority = proc->authority;
	__leaveSpinlock(&g_task_array_lock[id]);
	return 0;
}

int AdjustApicTimer_new() {
	char szout[256];
	char cpuinfo[256] = { 0 };

	getCpuInfo(cpuinfo);
	char* hdr = __strstr(cpuinfo, "CPU @ ");
	if (hdr) {
		hdr += 6;
		char* end = __strstr(hdr, "Hz");
		if (end) {
			end--;
			char buf[32] = { 0 };
			__memcpy(buf, hdr, end - hdr);
			double freq = strlf2lf(buf);
			int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
			double v = 0.0;
			if (*(end ) == 'G') {
				v = freq * 1000000000;
			}
			else if (*(end ) == 'M') {
				v = freq * 1000000;
			}
			else {
				return 0;
			}

			__printf(szout, "%s cpu:%d apic timer frequency:%x\r\n", __FUNCTION__, id, (DWORD)v);
			*(DWORD*)(LOCAL_APIC_BASE + 0x380) = (DWORD)v/(1000 / TASK_TIME_SLICE);
			return (DWORD)v;
		}
	}
	return 0;
}


int AdjustApicTimer() {
	return 1;

	char szout[256];

	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

	DWORD tick1 = *(DWORD*)CMOS_PERIOD_TICK_COUNT;
	DWORD tick2 = tick1;
	while (tick2 == tick1) {
		tick2 = *(DWORD*)CMOS_PERIOD_TICK_COUNT;
		__delay();
	}

	unsigned long long tc1 = __krdtsc();
	DWORD ts1 = *(DWORD*)(APICTIMER_TICK_COUNT + id * sizeof(int));

	tick1 = tick2 + 1;
	while (tick2 <  tick1) {
		tick2 = *(DWORD*)CMOS_PERIOD_TICK_COUNT;
		__delay();
	}

	DWORD ts2 = *(DWORD*)(APICTIMER_TICK_COUNT + id * sizeof(int));
	unsigned long long tc2 = __krdtsc();

	DWORD ts = ts2 - ts1;
	if (ts == 0)
	{
		//__printf(szout,"%s cpu:%d delta is null\r\n",__FUNCTION__, id);
		return 0;
	}

	DWORD tc = tc2 - tc1;

	//  x(should be 100) = [ts/g_apic_freq[id] = tc / g_timer_cost[id] ]

	unsigned long old_f = g_apic_freq[id];

	unsigned long old_c = g_timer_cost[id];

	double tps = tc / (1000 / TASK_TIME_SLICE); 
	
	double times = g_timer_cost[id]/tps;

	double newf = g_apic_freq[id] / times;

	newf = newf / LOCAL_APIC_DIVIDE;

	*(DWORD*)(LOCAL_APIC_BASE + 0x380) = (DWORD)newf;

	g_apic_freq[id] = (DWORD)newf;

	g_timer_cost[id] = (DWORD)tc/(1000/ TASK_TIME_SLICE);

	__printf(szout, "%s cpuid:%d intPerSec:%x, old g_apic_freq:%x,g_apic_freq:%x,old g_timer_cost:%x, g_timer_cost:%x\r\n", 
		__FUNCTION__,id, ts, old_f,(DWORD)g_apic_freq[id],old_c, (DWORD)g_timer_cost[id]);

	return newf;
}


unsigned long Get8254TickCount_error() {

	DWORD tick1 = *(DWORD*)CMOS_PERIOD_TICK_COUNT;
	DWORD tick2 = tick1;
	while (tick2 == tick1) {
		tick2 = *(DWORD*)CMOS_PERIOD_TICK_COUNT;
	}

	DWORD ts1 = *(DWORD*)(TIMER_TICK_COUNT);

	while (tick1 < tick2) {
		tick1 = *(DWORD*)CMOS_PERIOD_TICK_COUNT;
	}

	DWORD ts2 = *(DWORD*)(TIMER_TICK_COUNT);

	DWORD delta = ts2 - ts1;

	return delta;
}

unsigned long Get8254TickCount() {

	DWORD tick1 = *(DWORD*)CMOS_PERIOD_TICK_COUNT;
	DWORD tick2 = tick1;
	while (tick2 == tick1) {
		tick2 = *(DWORD*)CMOS_PERIOD_TICK_COUNT;
	}

	DWORD ts1 = *(DWORD*)(TIMER_TICK_COUNT );

	tick1 = tick2 + 1;
	while (tick2 < tick1) {
		tick2 = *(DWORD*)CMOS_PERIOD_TICK_COUNT;
	}

	DWORD ts2 = *(DWORD*)(TIMER_TICK_COUNT );

	DWORD delta = ts2 - ts1;

	return delta;
}


unsigned long GetCmosExactTickCount() {

	DWORD tick1 = *(DWORD*)CMOS_PERIOD_TICK_COUNT;
	DWORD tick2 = tick1;
	while (tick2 == tick1) {
		tick2 = *(DWORD*)CMOS_PERIOD_TICK_COUNT;
	}

	DWORD ts1 = *(DWORD*)(CMOS_EXACT_TICK_COUNT);

	tick1 = tick2 + 1;
	while (tick2 < tick1) {
		tick2 = *(DWORD*)CMOS_PERIOD_TICK_COUNT;
	}

	DWORD ts2 = *(DWORD*)(CMOS_EXACT_TICK_COUNT);

	DWORD delta = ts2 - ts1;

	return delta;
}