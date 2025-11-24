#include "servicesProc.h"
#include "task.h"
#include "hardware.h"
#include "task.h"
#include "mouse.h"
#include "keyboard.h"
#include "Utils.h"
#include "core.h"
#include "VM86.h"
#include "apic.h"

DWORD __declspec(naked) servicesProc(LIGHT_ENVIRONMENT* stack) {

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
	}

	__asm {
		push edi
		push eax

		mov eax, KERNEL_MODE_DATA
		mov ds, ax
		mov es, ax
		MOV FS, ax
		MOV GS, AX
		mov ss,ax
		call __kServicesProc
		add esp, 8

		mov edx,stack
		mov [edx + LIGHT_ENVIRONMENT.eax],eax		//may be error?  warning: "."应用于非 UDT 类型
	}
#ifdef APIC_ENABLE
	* (DWORD*)(LOCAL_APIC_BASE + 0xB0) = 0;
#endif

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

DWORD __declspec(dllexport) __kServicesProc(DWORD num, DWORD * params) {

	DWORD r = 0;
	switch (num)
	{
		case KBD_OUTPUT:
		{
			r = __kGetKbd(params[0]);
			break;
		}
		case KBD_INPUT:
		{
			__kPutKbd((unsigned char)params[0], 0);
			break;
		}
		case MOUSE_OUTPUT:
		{
			r= __kGetMouse((LPMOUSEINFO)params[0], params[1]);
			break;
		}
		case GRAPH_CHAR_OUTPUT:
		{
			r = __drawGraphChars((char*)params[0], params[1]);
			break;
		}
		case RANDOM:
		{
			r = __random((unsigned long)params[0]);
			break;
		}
		case SLEEP:
		{
			sleep(params);

			break;
		}
		case TURNON_SCREEN:
		{
			__turnonScreen();
			break;
		}
		case TURNOFF_SCREEN:
		{
			__turnoffScreen();
			break;
		}
		case CPU_MANUFACTORY:
		{
			r = __cputype(params);
			break;
		}
		case TIMESTAMP:
		{
			r = __timestamp(params);
			break;
		}
		case SWITCH_SCREEN:
		{
			__switchScreen();
			break;
		}
		case CPUINFO:
		{
			r = __cpuinfo(params);
			break;
		}
		case DRAW_MOUSE:
		{
			__kRestoreMouse();
			__kRefreshMouseBackup();
			__kDrawMouse();
			break;
		}
		case RESTORE_MOUSE:
		{

			__kRestoreMouse();
			break;
		}
		case SET_VIDEOMODE:
		{
			break;
		}

		default: {
			break;
		}
	}
	return r;
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

	LPPROCESS_INFO proc = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	int tid = proc->tid;
	LPPROCESS_INFO tss = (LPPROCESS_INFO)TASKS_TSS_BASE;
	LPPROCESS_INFO cur_tss = tss + tid;
	
	cur_tss->sleep += times ;
	proc->sleep = cur_tss->sleep;
	while(1)
	{
		__asm {
			sti
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



//MSR 是CPU 的一组64 位寄存器，可以分别通过RDMSR 和WRMSR 两条指令进行读和写的操作，前提要在ECX 中写入MSR 的地址。
//MSR 的指令必须执行在level 0 或实模式下。
//RDMSR    读模式定义寄存器。对于RDMSR 指令，将会返回相应的MSR 中64bit 信息到(EDX：EAX)寄存器中
//WRMSR    写模式定义寄存器。对于WRMSR 指令，把要写入的信息存入(EDX：EAX)中，执行写指令后，即可将相应的信息存入ECX 指定的MSR 中

//通过DTS获取温度并不是直接得到CPU的实际温度，而是两个温度的差。
//第一个叫做Tjmax，这个Intel叫TCC activation temperature，
//意思是当CPU温度达到或超过这个值时，就会触发相关的温度控制电路，系统此时会采取必要的动作来降低CPU的温度，或者直接重启或关机。
//所以CPU的温度永远不会超过这个值。这个值一般是100℃或85℃（也有其他值），对于具体的处理器来说就是一个固定的值。
//第二个就是DTS获取的CPU温度相对Tjmax的偏移值，暂且叫Toffset，那CPU的实际温度就是：currentTemp=Tjmax-Toffset
int __readTemperature(DWORD* tjunction) {
	unsigned int Tjunction = 0;
	DWORD temp = 0;
	__asm {
		mov eax, 0
		cpuid
		cmp eax, 6
		jb _tmpQuit

		mov eax, 6
		cpuid
		test eax, 2
		jz _tmpQuit

		mov ecx, 0x1A2		//eax中16~23位就是Tjmax的值
		rdmsr
		test eax, 0x40000000
		jnz _tmp85
		//mov eax, 100
		mov Tjunction, eax
		jmp _getdts
		_tmp85 :
		//mov eax, 85
		mov Tjunction, eax

			_getdts :
		mov ebx, eax
			and ebx, 0x00ff0000
			shr ebx, 16

			//mov ebx,eax

			mov ecx, 0x19C		//eax中16~22（注意这里是7位）位就是Toffset的值
			rdmsr
			and eax, 0x007f0000
			shr eax, 16
			sub ebx, eax
			mov eax, ebx
			mov temp, eax
			_tmpQuit :
	}

	*tjunction = Tjunction;

	char szout[1024];
	__printf(szout, (char*)"tjmax:%x,temprature:%x\r\n", Tjunction, temp);


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

#ifdef APIC_ENABLE
	* (DWORD*)(LOCAL_APIC_BASE + 0xB0) = 0;
#endif

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