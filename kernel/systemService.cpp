#include "systemService.h"
#include "task.h"
#include "hardware.h"
#include "task.h"
#include "mouse.h"
#include "keyboard.h"
#include "Utils.h"
#include "core.h"
#include "VM86.h"
#include "apic.h"

DWORD __declspec(naked) ServiceEntry(LIGHT_ENVIRONMENT* stack) {

	__asm {
		pushad
		push ds
		push es
		push fs
		push gs
		push ss

		push esp
		sub esp, 4
		push ebp
		mov ebp, esp
		sub esp, NATIVE_STACK_LIMIT
	}

	__asm {

		push dword ptr ss:[ebp + 8]
		push edi
		push eax

		mov eax, KERNEL_MODE_DATA
		mov ds, ax
		mov es, ax
		MOV FS, ax
		MOV GS, AX
		mov ss,ax

		call __kServicesProc
		add esp, 12

		mov edx,stack
		mov [edx + LIGHT_ENVIRONMENT.eax],eax		//may be error?  warning: "."应用于非 UDT 类型
	}

	__asm {
		mov esp, ebp
		pop ebp
		add esp, 4
		pop esp

		pop ss
		pop gs
		pop fs
		pop es
		pop ds
		popad

		iretd
	}
}




DWORD __declspec(dllexport) __kServicesProc(DWORD num, DWORD * params, LIGHT_ENVIRONMENT* stack) {

	DWORD r = 0;
	switch (num)
	{
		case SVC_KBD_OUTPUT:
		{
			r = __kGetKbd(params[0]);
			break;
		}
		case SVC_KBD_INPUT:
		{
			__kPutKbd((unsigned char)params[0], 0);
			break;
		}
		case SVC_MOUSE_OUTPUT:
		{
			r= __kGetMouse((LPMOUSEINFO)params[0], params[1]);
			break;
		}
		case SVC_GRAPH_CHAR_OUTPUT:
		{
			r = __drawGraphChars((char*)params[0], params[1]);
			break;
		}
		case SVC_RANDOM:
		{
			r = __random((unsigned long)params[0]);
			break;
		}
		case SVC_SLEEP:
		{
			sleep(params);

			break;
		}
		case SVC_TURNON_SCREEN:
		{
			__turnonScreen();
			break;
		}
		case SVC_TURNOFF_SCREEN:
		{
			__turnoffScreen();
			break;
		}
		case SVC_CPU_MANUFACTORY:
		{
			r = __cputype(params);
			break;
		}
		case SVC_TIMESTAMP:
		{
			r = __timestamp(params);
			break;
		}
		case SVC_SWITCH_SCREEN:
		{
			__switchScreen();
			break;
		}
		case SVC_CPUINFO:
		{
			r = __cpuinfo(params);
			break;
		}
		case SVC_DRAW_MOUSE:
		{
			__kRestoreMouse();
			__kRefreshMouseBackup();
			__kDrawMouse();
			break;
		}
		case SVC_RESTORE_MOUSE:
		{

			__kRestoreMouse();
			break;
		}
		case SVC_SET_VIDEOMODE:
		{
			break;
		}
		case SVC_YIELD:
		{
			yield(stack);
			break;
		}
		case SVC_IPI_CREATEPROC:
		{
			IpiCreateProcess(params[0], params[1], (char*)params[2], (char*)params[3], params[4], params[5]);
			break;
		}
		case SVC_IPI_CREATETHREAD:
		{
			IpiCreateThread((char*)params[0], (char*)params[1], params[2], (char*)params[3]);
			break;
		}
		default: {
			break;
		}
	}
	return r;
}




extern "C"  __declspec(dllexport)void __ipiCreateProcess(DWORD base, int size, char* module, char* func, int level, unsigned long p) {

	__asm {
		push edi
		mov eax, SVC_IPI_CREATEPROC
		lea edi,base
		int 0x80
		pop edi
	}
}
extern "C"  __declspec(dllexport)void __ipiCreateThread(DWORD addr, char* module, unsigned long p,char * func ) {

	__asm {
		push edi
		mov eax, SVC_IPI_CREATETHREAD
		lea edi, addr
		int 0x80
		pop edi
	}
}





void sleep(DWORD * params) {
	int sleeptime = params[0];
	int interval = 1000 / (OSCILLATE_FREQUENCY / SYSTEM_TIMER0_FACTOR);
	DWORD times = sleeptime / interval;
	DWORD mod = sleeptime % interval;
	if (mod != 0)
	{
		times++;
	}

	if (times == 0) {
		times = 1;
	}

	enter_task_array_lock();

	LPPROCESS_INFO proc = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	int tid = proc->tid;
	LPPROCESS_INFO tss = (LPPROCESS_INFO)GetTaskTssBase();
	LPPROCESS_INFO cur_tss = tss + tid;
	
	cur_tss->sleep += times ;
	proc->sleep = cur_tss->sleep;

	leave_task_array_lock();

	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	long long t1 = 0;

	DWORD low1 = 0;
	DWORD high1 = 0;
	readmsr(0xe7, &low1, &high1);
	if (low1 == 0 && high1== 0) {
		t1 = __krdtsc();
	}
	else {
		t1 = high1;
		t1 = t1 << 32;
		t1 += low1;
	}

	while(1)
	{
		
		__asm {
			hlt
		}

		if (cur_tss->sleep == 0)
		{
			break;
		}
		else {
			continue;
		}
	}

	unsigned long long t2 = 0;
	if (low1 == 0 && high1 == 0) {
		t2 = __krdtsc();
		g_cpu_sleep[id] += (t2 - t1);

		g_cpu_active[id] = t2 - g_cpu_sleep[id];
	}
	else {
		DWORD low2 = 0;
		DWORD high2 = 0;
		readmsr(0xe7, &low2, &high2);
		t2 = high2;
		t2 = (t2 << 32) + low2;
		
		unsigned long long tick = __krdtsc();

		g_cpu_active[id] = t2;
		g_cpu_sleep[id] = tick - t2;
	}

}



DWORD g_random_seed = 0;

DWORD __random(DWORD r) {

	const int u = 65537;
	const int v = 997;
	const int w = 9973;

	const int base = 0x10000;

	if (g_random_seed) {
		
	}
	else {
		if (r) {
			g_random_seed = r;
		}
		else {
			g_random_seed = *((DWORD*)TIMER0_TICK_COUNT);
		}
	}
	g_random_seed = (w * g_random_seed*v + u)% 0xffffffff;
	return g_random_seed;



}

DWORD __random_old(DWORD init) {
	unsigned __int64 t = init;
	if (t == 0) {
		DWORD dt = *((DWORD*)TIMER0_TICK_COUNT);
		t = dt;
	}
	t = (t *( 7 *7*7*7*7 )) % 0xffffffff;
	return (DWORD)t;
}



void __turnoffScreen() {

	outportb(0x3c4, 1);
	int r = inportb(0x3c5);
	if ( (r & 0x20) == 0) {
		outportb(0x3c5, r | 0x20);
	}
}


void __turnonScreen() {

	outportb(0x3c4, 1);
	int r = inportb(0x3c5);
	if (r & 0x20 ) {
		outportb(0x3c5, 0);
	}
}


void __switchScreen() {
	outportb(0x3c4, 1);
	int r = inportb(0x3c5);
	if (r & 0x20) {
		outportb(0x3c5, 0);
	}
	else {
		outportb(0x3c5, 0x20);
	}
}



DWORD	__cputype(unsigned long * params) {

	__asm{
		mov edi,params
		mov eax, 0
		; must use .586 or above
		; dw 0a20fh
		mov ecx,0
		cpuid
		; ebx:edx:ecx = intel or else
		mov ds : [edi] , ebx
		mov ds : [edi + 4] , edx
		mov ds : [edi + 8] , ecx
		mov dword ptr ds : [edi + 12] , 0
	}
}





DWORD __cpuinfo(unsigned long* params) {
	__asm {
		mov edi,params

		mov     eax, 80000000h
		mov		ecx,0
		; dw 0a20fh
		cpuid
		cmp     eax, 80000004h
		jb      __cpuinfoEnd

		mov     eax, 80000002h
		mov		ecx, 0
		; dw 0a20fh
		cpuid
		mov     dword ptr[edi], eax
		mov     dword ptr[edi + 4], ebx
		mov     dword ptr[edi + 8], ecx
		mov     dword ptr[edi + 12], edx

		mov     eax, 80000003h
		mov		ecx, 0
		; dw 0a20fh
		cpuid
		mov     dword ptr[edi + 16], eax
		mov     dword ptr[edi + 20], ebx
		mov     dword ptr[edi + 24], ecx
		mov     dword ptr[edi + 28], edx

		mov     eax, 80000004h
		mov		ecx, 0
		; dw 0a20fh
		cpuid	
		mov     dword ptr[edi + 32], eax
		mov     dword ptr[edi + 36], ebx
		mov     dword ptr[edi + 40], ecx
		mov     dword ptr[edi + 44], edx

		mov     dword ptr[edi + 48], 0

		__cpuinfoEnd:
	}
}

unsigned int getcpuFreq() {
	unsigned int f1 = 0;
	unsigned int f2 = 0;
	__asm {
		rdtsc
		mov [f1],eax
		mov [f1 + 4],edx
		xor eax,eax
		rdtsc
		mov[f2], eax
		mov[f2 + 4], edx
	}
	return f2 - f1;
}


//https://www.felixcloutier.com/x86/cpuid
// eax: Processor Base Frequency (in MHz)
// ebx: Maximum Frequency (in MHz)
// ecx: Bus (Reference) Frequency (in MHz)
unsigned __int64 __cpuFreq(DWORD* cpu,DWORD* max,DWORD* bus) {
	__asm {

		mov eax, 0x16
		mov ecx, 0
		cpuid
		mov [cpu],eax
		mov [max],ebx
		mov [bus],ecx
		//why use ret will make error?
		//ret 
	}
}

//CPU_freq = tsc_freq * (aperf_t1 - aperf_t0) / (mperf_t1 - mperf_t0)

unsigned __int64 getCpuFreq() {
	unsigned int mperf_var_lo;

	unsigned int mperf_var_hi;

	unsigned int aperf_var_lo;

	unsigned int aperf_var_hi;

	unsigned __int64 maxfreq;

	__asm {
		; read MPERF
		mov ecx, 0xe7
		rdmsr
		mov mperf_var_lo, eax
		mov mperf_var_hi, edx

		; read APERF
		mov ecx, 0xe8
		rdmsr
		mov aperf_var_lo, eax
		mov aperf_var_hi, edx

		; read maxfreq
		mov ecx,0xce
		rdmsr
		; bits 8:15
		mov dword ptr  [maxfreq],eax
		mov dword ptr [maxfreq+4],edx
	}
	if (aperf_var_hi != aperf_var_lo && mperf_var_hi != mperf_var_lo) {
		return ( maxfreq) * (aperf_var_hi - aperf_var_lo) / (mperf_var_hi - mperf_var_lo);
	}
	return 0;
}

unsigned __int64 __krdtsc() {
	__asm {

		rdtsc
		//why use ret will make error?
		//ret 
	}
}


DWORD __timestamp(unsigned long* params) {

	__asm {
		mov edi,params
		; must use .586 or above
		rdtsc
		; edx:eax = time stamp
		mov ds : [edi] , eax
		mov ds : [edi + 4] , edx
		mov dword ptr ds : [edi + 8] , 0
	}
}


//通过DTS获取温度并不是直接得到CPU的实际温度，而是两个温度的差。
//第一个叫做Tjmax，这个Intel叫TCC activation temperature，
//意思是当CPU温度达到或超过这个值时，就会触发相关的温度控制电路，系统此时会采取必要的动作来降低CPU的温度，或者直接重启或关机。
//所以CPU的温度永远不会超过这个值。这个值一般是100℃或85℃（也有其他值），对于具体的处理器来说就是一个固定的值。
//第二个就是DTS获取的CPU温度相对Tjmax的偏移值，暂且叫Toffset，那CPU的实际温度就是：currentTemp=Tjmax-Toffset
int CpuTemperature(DWORD* lptj) {
	unsigned int tjmax = 0;
	DWORD temp_offset = 0;

	DWORD low = 0;
	DWORD high = 0;
	int result = 0;
	readmsr(0x1a2, &low, &high);

	tjmax = (low & 0xff0000) >> 16;

	DWORD low2 = 0;
	DWORD high2 = 0;

	readmsr(0x19c, &low2, &high2);
	temp_offset = (low2 & 0x7f0000) >> 16;

	int temp = tjmax - temp_offset;

	*lptj = tjmax;

	char szout[256];
	__printf(szout, (char*)"tjmax:%x,temperature:%x\r\n", tjmax, temp);

	return temp;
}



int __kVm86IntProc() {

	V86_INT_PARAMETER* params = (V86_INT_PARAMETER*)V86_INT_ADDRESS;

	TssDescriptor* lptssd = (TssDescriptor*)(GDT_BASE + params->tr);
	if ((lptssd->type & 2)) {
		lptssd->type = lptssd->type & 0x0d;
	}

	unsigned char code[16];
	code[0] = 0xea;
	code[1] = 0;
	code[2] = 0;
	code[3] = 0;
	code[4] = 0;
	*(WORD*)(code+5) = (WORD)(params->tr);
	
	__asm {
		LEA EAX,[code]
		JMP EAX

		//_emit 0xea
		//_emit 0
		//_emit 0
		//_emit 0
		//_emit 0

		//_emit kTssTaskSelector
		//_emit 0
	}
	return 0;
}

DWORD __declspec(naked) vm86IntProc(LIGHT_ENVIRONMENT* stack) {

	__asm {
		pushad
		push ds
		push es
		push fs
		push gs
		push ss

		push esp
		sub esp, 4
		push ebp
		mov ebp, esp
		sub esp, NATIVE_STACK_LIMIT
	}

	__asm {
		mov eax, KERNEL_MODE_DATA
		mov ds, ax
		mov es, ax
		MOV FS, ax
		MOV GS, AX
		mov ss, ax

		call __kVm86IntProc

		mov edx, stack
		mov[edx + LIGHT_ENVIRONMENT.eax], eax		//may be error?  warning: "."应用于非 UDT 类型
	}



	__asm {
		mov esp, ebp
		pop ebp
		add esp, 4
		pop esp

		pop ss
		pop gs
		pop fs
		pop es
		pop ds
		popad

		iretd
	}
}