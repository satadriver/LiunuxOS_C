

#include "apic.h"
#include "hardware.h"
#include "Utils.h"
#include "descriptor.h"

#include "task.h"
#include "cmosAlarm.h"
#include "cmosExactTimer.h"
#include "cmosPeriodTimer.h"
#include "core.h"
#include "device.h"
#include "coprocessor.h"
#include "debugger.h"


int gAllocateAp = 0;

unsigned long g_allocate_ap_lock = 0;

unsigned long g_ap_work_lock = 0;

DWORD * gOicBase = 0;

char * gRcbaBase = 0;

DWORD* gHpetBase = 0;

int g_bsp_id = 0;

int g_test_value = 0;

void enableRcba() {
	outportd(0xcf8, 0x8000f8f0);

	gRcbaBase = (char*)(inportd(0xcfc) & 0xffffc000);
	*gRcbaBase = *gRcbaBase | 1;
}

//bit 9:enable irq 13
//bit 8:enable apic io
void EnableInt13() {
	outportd(0xcf8, 0x8000f8f0);

	gRcbaBase = (char*)(inportd(0xcfc) & 0xffffc000);

	gOicBase = (DWORD*)((DWORD)gRcbaBase + 0x31fe);

	DWORD v = *gOicBase;

	v = (v & 0xffffff00) | 0x200;

	*gOicBase = v;

}

unsigned long ReadIoApicReg(int reg) {

	*(DWORD*)(IO_APIC_BASE) = reg ;
	__asm {
		mfence
	}
	return *(DWORD*)(IO_APIC_BASE+0x10);
}


unsigned long WriteIoApicReg(int reg, unsigned long value) {
	*(DWORD*)(IO_APIC_BASE) = reg ;
	__asm {
		mfence
	}
	*(DWORD*)(IO_APIC_BASE + 0x10) = value;

	return 0;
}



void iomfence() {
	__asm {
		mfence
	}
}

void setIoRedirect(int idx, int id, int vector, int mode) {

	WriteIoApicReg(idx + 1, ((id & 0x0ff) << 24));

	WriteIoApicReg(idx, vector + (mode << 8));

}


unsigned long long GetIoRedirect(int idx) {

	unsigned long long high = ReadIoApicReg(idx + 1);

	unsigned long long low = ReadIoApicReg(idx);

	unsigned long long v = (high << 32) | low;
	return v;
}



void setIoApicID(int id) {

	WriteIoApicReg(0, (id & 0x0ff) << 24);

}


void WaitIcrFree() {
	unsigned int value = 0;
	do {
		value = *(DWORD*)(LOCAL_APIC_BASE + 0x300);
	} while (value & 0x1000);
}


void enableIoApic() {

	outportd(0xcf8, 0x8000f8f0);

	gRcbaBase = (char*)(inportd(0xcfc) & 0xffffc000);

	gOicBase = (DWORD*)((DWORD)gRcbaBase + 0x31fe);

	DWORD v = *gOicBase;
	
	v = (v & 0xffffff00) | 0x100;

	*gOicBase = v;

	char szout[1024];

	__printf(szout,"oic base:%x,value:%x,rcba base:%x\r\n", gOicBase, v,gRcbaBase);
}



DWORD* getOicBase() {
	outportd(0xcf8, 0x8000f8f0);
	DWORD rcba = inportd(0xcfc) & 0xffffc000;

	gOicBase = (DWORD*)(rcba + 0x31fe);

	return gOicBase;
}

char* getRcbaBase() {
	outportd(0xcf8, 0x8000f8f0);
	DWORD base = inportd(0xcfc) & 0xffffc000;

	gRcbaBase = (char*)base;

	return gRcbaBase;
}



int enableHpet() {
	outportd(0xcf8, 0x8000f8f0);

	DWORD addr = inportd(0xcfc) & 0xffffc000;

	gHpetBase = (DWORD*)(addr + 0x3404);

	DWORD v = *gHpetBase;

	v = (v | 0x80) & 0xfffffffc;

	*gHpetBase = v;

	char szout[1024];
	__printf(szout, "hpet base:%x,value:%x\r\n", gHpetBase, v);

	return 0;
}

//enable io apic
void enableIMCR() {
	outportb(0x22, 0x70);
	outportb(0x23, 0x01);
}


int getLVTCount(int n) {
	int cnt =(int) *(long long*)(LOCAL_APIC_BASE + 0x30) >>16;
	return cnt;
}

int getLocalApicVersion() {
	int ver = *(long long*)(LOCAL_APIC_BASE + 0x30) & 0xff;
	return ver;
}


//cpuid.01h:ebx[31:24]
int getLocalApicID() {
	__asm {
		mov eax,1
		mov ecx,0
		cpuid

		test ebx,0x200000
		jnz _x2apic

		shr ebx,24
		movzx eax,bl
		ret

		_x2apic:
		mov eax,0bh
		mov ecx, 0
		cpuid
		mov eax,edx
	}
}








int initHpet() {
	int res = 0;

	//enableHpet();

	char szout[256];
	
	long long idc = *(long long*)APIC_HPET_BASE;

	HPET_GCAP_ID_REG * hg = (HPET_GCAP_ID_REG*)&idc;
	//the default value is 0x0429b17f=69841279 about 14.318179MHz	
	__printf(szout, "hpet version:%d,count:%d,width:%d,compatable:%d,vender:%x,tick:%x\r\n ", 
		hg->version, hg->count, hg->width, hg->compatable,hg->venderid,hg->tick);
	
	int cnt = hg->count;

	DWORD tick = hg->tick;

	unsigned long long ns = 1000000000000000;

	long long tc =( ns / hg->tick)/100;		// 10ms, 14318179 = 1000ms,0x0429b17f=69841279

	*(long long*)(APIC_HPET_BASE + 0x10) = 0;

	//General Interrupt Status Register
	*(long long*)(APIC_HPET_BASE + 0x20) = 0;
		
	//timer0
	unsigned long long* regs = (unsigned long long*)(APIC_HPET_BASE + 0x100);

	//bit3: writing 1 to this field enables periodic timer and writing 0 enables non - periodic mode.O
	//bit2:Setting this bit to 1 enables triggering of interrupts. Even if this bit is 0, this timer will still set Tn_INT_STS.
	regs[0] = 0x40 + 8 + 4 + 2;
	//Timer N Comparator Value Register
	//compare with main counter to check if an interrupt should be generated.
	regs[1] = tc;
	//regs[2] = 0;

	regs[4] = 4 + 2;
	regs[5] = tc;
	//regs[6] = 0;

	//regs[8] = 0x4;
	//regs[9] = tc;
	//regs[10] = 0;

	//Main Counter Value Register,increase in each interruption
	//Writes to this register should only be done when the counter is halted (ENABLE_CNF = 0
	*(long long*)(APIC_HPET_BASE + 0xf0) = 0;

	//Tn_INT_STS
	//bit0:ENABLE_CNF，0:main counter is halted, timer interrupts are disabled
	//bit1:  "legacy replacement" mapping is enabled。timer -> irq0  cmos ->irq8
	*(long long*)(APIC_HPET_BASE + 0x10) = 3;

	return FALSE;
}



void IoApicRedirect(int pin, int cpu, int vector, int mode) {

	int cpuid = 0;
	if (cpu == -1) {
		__enterSpinlock(&g_allocate_ap_lock);
		int total = *(int*)(AP_TOTAL_ADDRESS);
		int* ids = (int*)AP_ID_ADDRESS;
		cpuid = ids[gAllocateAp];
		gAllocateAp++;
		if (gAllocateAp >= total) {
			gAllocateAp = 0;
		}
		__leaveSpinlock(&g_allocate_ap_lock);
	}
	else {
		cpuid = cpu;
	}

	unsigned long long oldValue = GetIoRedirect(pin);

	char szout[1024];
	__printf(szout, "io apic %d value I64x\r\n ",pin, oldValue);

	setIoRedirect(pin, cpuid, vector, mode);
}


void SetIcr(int cpu,int vector,int mode,int destType) {

	WaitIcrFree();

	if (destType == 0) {
		unsigned int id = cpu << 24;
		*(DWORD*)(LOCAL_APIC_BASE + 0x310) = id;	
		
	}

	iomfence();

	unsigned int v = vector;
	v = v | 0x4000;

	if (mode == 0) {
		
	}
	else {
		v = ((mode & 7) << 8) | v;
	}

	if (destType) {
		v = v | ((destType & 3) << 18);
	}

	*(DWORD*)(LOCAL_APIC_BASE + 0x300) = v;

	v = *(DWORD*)(LOCAL_APIC_BASE + 0x300);
	//char szout[256];
	//__printf(szout, "%s cpu:%x result:%x\r\n", __FUNCTION__, cpu, v);

	return;

}

int InitIoApicRte() {
	
	IoApicRedirect(0x10, g_bsp_id, APIC_LVTLINT0_VECTOR|0x10000, 0);

	IoApicRedirect(0x12, g_bsp_id, INTR_8259_MASTER + 1, 0);

	IoApicRedirect(0x14, g_bsp_id, APIC_HPETTIMER_VECTOR, 0);

	IoApicRedirect(0x16, g_bsp_id, INTR_8259_MASTER + 3, 0);
	IoApicRedirect(0x18, g_bsp_id, INTR_8259_MASTER + 4, 0);
	IoApicRedirect(0x1a, g_bsp_id, INTR_8259_MASTER + 5, 0);
	IoApicRedirect(0x1c, g_bsp_id, INTR_8259_MASTER + 6, 0);
	IoApicRedirect(0x1e, g_bsp_id, INTR_8259_MASTER + 7, 0);

	IoApicRedirect(0x20, g_bsp_id, APIC_LVTTIMER_VECTOR, 0);
	//IoApicRedirect(0x20, g_bsp_id, APIC_HPETTIMER_VECTOR, 0);

	IoApicRedirect(0x22, g_bsp_id, INTR_8259_SLAVE + 1, 0);
	IoApicRedirect(0x24, g_bsp_id, INTR_8259_SLAVE + 2, 0);
	IoApicRedirect(0x26, g_bsp_id, INTR_8259_SLAVE + 3, 0);
	IoApicRedirect(0x28, g_bsp_id, INTR_8259_SLAVE + 4, 0);
	IoApicRedirect(0x2a, g_bsp_id, INTR_8259_SLAVE + 5, 0);
	IoApicRedirect(0x2c, g_bsp_id, INTR_8259_SLAVE + 6, 0);
	IoApicRedirect(0x2e, g_bsp_id, INTR_8259_SLAVE + 7, 0);

	return 0;
}



extern "C" void __declspec(naked) IPIIntHandler(LIGHT_ENVIRONMENT * stack) {
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

		mov eax, KERNEL_MODE_DATA
		mov ds, ax
		mov es, ax
		MOV FS, ax
		MOV GS, AX
	}

	{
		char szout[256];
		
		__printf(szout, "IPIIntHandler\r\n");
		

		*(DWORD*)(LOCAL_APIC_BASE + 0xb0) = 0;

		__enterSpinlock(&g_ap_work_lock);
		g_test_value++;
		__leaveSpinlock(&g_ap_work_lock);

		__printf(szout, "function:%s value:%d\r\n",__FUNCTION__, g_test_value);
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


int g_lvt_timer = 0;

extern "C" void __declspec(naked) LVTTimerIntHandler(LIGHT_ENVIRONMENT* stack) {
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

		mov eax, KERNEL_MODE_DATA
		mov ds, ax
		mov es, ax
		MOV FS, ax
		MOV GS, AX
	}

	{
		char szout[256];
		__printf(szout, "%s\r\n", __FUNCTION__);

		*(DWORD*)(LOCAL_APIC_BASE + 0xb0) = 0;

		*(DWORD*)(LOCAL_APIC_BASE + 390) = 0;

		int intInSec = (1000 / TASK_TIME_SLICE);

		/*
		int v = 0x10000;
		*(DWORD*)(LOCAL_APIC_BASE + 0x320) = v;

		v = 0x0d;
		*(DWORD*)(LOCAL_APIC_BASE + 0x3E0) = v;

		
		v = 1000000 / intInSec;
		*(DWORD*)(LOCAL_APIC_BASE + 0x380) = v;

		v = APIC_LVTTIMER_VECTOR | 0x20000;
		*(DWORD*)(LOCAL_APIC_BASE + 0x320) = v;
		*/

		g_lvt_timer++;
		if (g_lvt_timer >= intInSec) {
			g_lvt_timer = 0;
			__kPeriodTimer();
		}
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

		clts
		iretd
	}
}



extern "C" void __declspec(naked) LVTTemperatureIntHandler(LIGHT_ENVIRONMENT* stack) {
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

		mov eax, KERNEL_MODE_DATA
		mov ds, ax
		mov es, ax
		MOV FS, ax
		MOV GS, AX
	}

	{
		*(DWORD*)(LOCAL_APIC_BASE + 0xb0) = 0;

		char szout[256];
		__printf(szout, "%s\r\n", __FUNCTION__);
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

		clts
		iretd
	}
}



extern "C" void __declspec(naked) LVTErrorIntHandler(LIGHT_ENVIRONMENT* stack) {
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

		mov eax, KERNEL_MODE_DATA
		mov ds, ax
		mov es, ax
		MOV FS, ax
		MOV GS, AX
	}

	{
		*(DWORD*)(LOCAL_APIC_BASE + 0xb0) = 0;

		*(DWORD*)(LOCAL_APIC_BASE + 0x370) = 0;
		DWORD error = *(DWORD*)(LOCAL_APIC_BASE + 0x370);

		char szout[256];
		__printf(szout, "%s error:%x\r\n", __FUNCTION__,error);
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

		clts
		iretd
	}
}


extern "C" void __declspec(naked) LVTPerformanceIntHandler(LIGHT_ENVIRONMENT* stack) {
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

		mov eax, KERNEL_MODE_DATA
		mov ds, ax
		mov es, ax
		MOV FS, ax
		MOV GS, AX
	}

	{
		*(DWORD*)(LOCAL_APIC_BASE + 0xb0) = 0;

		char szout[256];
		__printf(szout, "%s\r\n", __FUNCTION__);
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

		clts
		iretd
	}
}

extern "C" void __declspec(naked) LVTLint1Handler(LIGHT_ENVIRONMENT* stack) {
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

		mov eax, KERNEL_MODE_DATA
		mov ds, ax
		mov es, ax
		MOV FS, ax
		MOV GS, AX
	}

	{
		*(DWORD*)(LOCAL_APIC_BASE + 0xb0) = 0;

		char szout[256];
		__printf(szout, "%s\r\n", __FUNCTION__);
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

		clts
		iretd
	}
}


extern "C" void __declspec(naked) LVTLint0Handler(LIGHT_ENVIRONMENT* stack) {
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

		mov eax, KERNEL_MODE_DATA
		mov ds, ax
		mov es, ax
		MOV FS, ax
		MOV GS, AX
	}

	{
		*(DWORD*)(LOCAL_APIC_BASE + 0xb0) = 0;

		char szout[256];
		__printf(szout, "%s\r\n", __FUNCTION__);
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

		clts
		iretd
	}
}

extern "C" void __declspec(naked) LVTCMCIHandler(LIGHT_ENVIRONMENT* stack) {
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

		mov eax, KERNEL_MODE_DATA
		mov ds, ax
		mov es, ax
		MOV FS, ax
		MOV GS, AX
	}

	{
		*(DWORD*)(LOCAL_APIC_BASE + 0xb0) = 0;

		char szout[256];
		__printf(szout, "%s\r\n", __FUNCTION__);
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

		clts
		iretd
	}
}

//https://blog.csdn.net/weixin_46645613/article/details/120406002
//https://zhuanlan.zhihu.com/p/406213995
//https://zhuanlan.zhihu.com/p/678582090
//https://www.zhihu.com/question/594531181/answer/2982337869








int g_cmos_timer = 0;


extern "C" void __declspec(naked) HpetTimer0Handler(LIGHT_ENVIRONMENT * stack) {
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

		mov eax, KERNEL_MODE_DATA
		mov ds, ax
		mov es, ax
		MOV FS, ax
		MOV GS, AX

		cli
	}

	{
		char szout[256];
		//__printf(szout, "HpetInterrupt\r\n");

		//LPPROCESS_INFO process = (LPPROCESS_INFO)GetCurrentTaskTssBase();

		int value = *(int*)(APIC_HPET_BASE + 0x20);
		if (value & 1)
		{
			__kTaskSchedule((LIGHT_ENVIRONMENT*)stack);
			*(long*)(APIC_HPET_BASE + 0x20) = value & 0xfffffffe;
			*(unsigned long*)(APIC_HPET_BASE + 0xf0) = 0;
			
			*(unsigned long*)(APIC_HPET_BASE + 0xf4) = 0;

			g_cmos_timer++;
			if (g_cmos_timer == (1000 / TASK_TIME_SLICE)) {
				g_cmos_timer = 0;
				
				//__kPeriodTimer();
			}
		}

		if (value & 2) {
			*(long*)(APIC_HPET_BASE + 0x20) = value & 0xfffffffd;
			*(unsigned long*)(APIC_HPET_BASE + 0xf0) = 0;

			*(unsigned long*)(APIC_HPET_BASE + 0xf4) = 0;

			g_cmos_timer++;
			if (g_cmos_timer == (1000 / TASK_TIME_SLICE)) {
				g_cmos_timer = 0;

			}

			
			long long idc = *(long long*)APIC_HPET_BASE;

			HPET_GCAP_ID_REG* hg = (HPET_GCAP_ID_REG*)&idc;
			unsigned long long ns = 1000000000000000;

			long long tc = (ns / hg->tick) / 100;	
			unsigned long long* regs = (unsigned long long*)(APIC_HPET_BASE + 0x100);
			*(long long*)(APIC_HPET_BASE + 0x10) = 0;
			regs[4] = 4 + 2;
			regs[5] = tc;
			*(long long*)(APIC_HPET_BASE + 0x10) = 3;
		}

		*(DWORD*)(LOCAL_APIC_BASE + 0xB0) = 0;
		*(DWORD*)(IO_APIC_BASE + 0x40) = 0;

		DWORD base = APIC_HPET_BASE;
		unsigned __int64* gintr_sta = (unsigned __int64*)(base + 0x20);
		*gintr_sta = 0xff;
	}

	__asm {
#ifdef SINGLE_TASK_TSS
		call GetCurrentTaskTssBase
		mov edx, eax
		mov eax, dword ptr ds : [edx + PROCESS_INFO.tss.cr3]
		mov cr3, eax
#endif

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
#ifdef SINGLE_TASK_TSS
		mov esp, dword ptr ss : [esp - 20]
#endif	
		sti
		clts
		iretd

		jmp HpetTimer0Handler
	}
}

int AllocateApTask(int intnum) {

	if(intnum < 0 || intnum > 255) {
		return -1;
	}

	int res = -1;
	int cpuId = 0;

	__enterSpinlock(&g_allocate_ap_lock);

	int total = *(int*)(AP_TOTAL_ADDRESS);
	if (total > 0) {

		int idx = gAllocateAp;
		int* ids = (int*)AP_ID_ADDRESS;
		int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
		
		do {
			cpuId = ids[gAllocateAp];
			if (id != cpuId) {

				SetIcr(cpuId, intnum,0,0);

				res = gAllocateAp;		
			}

			gAllocateAp++;
			if (gAllocateAp >= total) {
				gAllocateAp = 0;
			}
			if (res >= 0) {
				break;
			}
		} while (gAllocateAp != idx);
	}

	__leaveSpinlock(&g_allocate_ap_lock);

	char szout[256];
	unsigned long v = *(DWORD*)(LOCAL_APIC_BASE + 0x300);
	__printf(szout, "%s index:%x,id:%x result:%x\r\n",__FUNCTION__, res, cpuId,v);
	return res;
}






int ActiveApTask(int intnum) {

	//return 0;

	if (intnum < 0 || intnum > 255) {
		return -1;
	}

	__enterSpinlock(&g_allocate_ap_lock);

	int total = *(int*)(AP_TOTAL_ADDRESS);
	if (total > 0) {
		
		int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
		int* ids = (int*)AP_ID_ADDRESS;
		for (int idx = 0; idx < total; idx++)
		{
			int cpuId = ids[idx];
			if (id != cpuId) {

				SetIcr(cpuId, intnum,0,0);
			}	
		} 
	}

	__leaveSpinlock(&g_allocate_ap_lock);

	//char szout[256];
	//__printf(szout, "AllocateApTask index:%x,id:%x\r\n", res, cpuId);
	return 0;
}


int IsApicSupported() {

	int res = 0;

	__asm {
		mov eax, 1
		cpuid
		test edx, (1 << 9)
		jz __no_apic
		mov[res], 1

		__no_apic:
	}
	return res;
}

int enableLocalApic() {

	unsigned long origin = 0;

	DWORD low = 0;
	DWORD high = 0;
	readmsr(0x1b,&low,&high);
	origin = low;

	high = 0;
	low = LOCAL_APIC_BASE|0x800;
	writemsr(0x1b, low, high);

	*(DWORD*)(LOCAL_APIC_BASE + 0xf0) = 0x100 | APIC_SPURIOUS_VECTOR;

	readmsr(0x1b, &low, &high);
	char szout[256];
	__printf(szout, "local apic value:%x,origin value:%x\r\n", low, origin);
	return 0;
}


void DisableInt() {
	__asm {
		mov al, 0xFF
		out 0xA1, al
		out 0x21, al
	}
	enableIMCR();
}



//https://blog.csdn.net/weixin_46645613/article/details/120406002
//https://zhuanlan.zhihu.com/p/406213995
//https://zhuanlan.zhihu.com/p/678582090
//https://www.zhihu.com/question/594531181/answer/2982337869


// Local APIC within processor而 IO APIC within chipset(一般都在南桥里). 
//LINT0 and LINT1是local APIC的两个 input pins. 一般都和 INTR & NMI shared共享.
//即 pin name is: LINT0/INTR & LINT1/NMI. 
//当 IO APIC被设成 "bypass mode"(即不靠IO APIC传递 interrupts),则 8259的 INTR会接到 local APIC的 LINT0；
//而 chipset内的 NMI logic 也会接到 local APIC的 LINT1.
//ExtInt代表接受cpu直接接受8259中断信号，而不是io apic

int DisableLocalApicLVT() {

	unsigned long v = 0;
	v = APIC_LVTTEMPERATURE_VECTOR;
	*(DWORD*)(LOCAL_APIC_BASE + 0x330) = v | 0x10000;

	v = APIC_LVTPERFORMANCE_VECTOR;
	*(DWORD*)(LOCAL_APIC_BASE + 0x340) = v | 0x10000;

	v = APIC_LVTLINT0_VECTOR;
	*(DWORD*)(LOCAL_APIC_BASE + 0x350) = v | 0x10000;

	v = APIC_LVTLINT1_VECTOR;
	*(DWORD*)(LOCAL_APIC_BASE + 0x360) = v | 0x10000;

	v = APIC_LVTERROR_VECTOR;
	*(DWORD*)(LOCAL_APIC_BASE + 0x370) = v | 0x10000;

	v = APIC_LVTCMCI_VECTOR;
	*(DWORD*)(LOCAL_APIC_BASE + 0x2f0) = v | 0x10000;

	return 0;

}


int InitLocalApicCmci() {
	int v = APIC_LVTCMCI_VECTOR;
	*(DWORD*)(LOCAL_APIC_BASE + 0x2f0) = v;
	return 0;
}


int InitLocalApicErr() {
	int v = APIC_LVTERROR_VECTOR;
	*(DWORD*)(LOCAL_APIC_BASE + 0x370) = v;
	return 0;
}

int InitLocalApicTimer() {

	int v = 0;

	v = 0x10000;
	*(DWORD*)(LOCAL_APIC_BASE + 0x320) = v;

	v = 0x03;
	*(DWORD*)(LOCAL_APIC_BASE + 0x3E0) = v;

	v = APIC_LVTTIMER_VECTOR | 0x20000;
	*(DWORD*)(LOCAL_APIC_BASE + 0x320) = v;

	v = 1000000 *16/ (1000 / TASK_TIME_SLICE);
	*(DWORD*)(LOCAL_APIC_BASE + 0x380) = v;

	return 0;
}


extern "C" void __declspec(dllexport) __kApInitProc() {
	char* reg_esp = 0;
	int ret = 0;
	__asm {
		//cli
		mov ds:[reg_esp],esp
	}

	char szout[1024];
	enableLocalApic();
	*(DWORD*)(LOCAL_APIC_BASE + 0xf0) = 0x100| APIC_SPURIOUS_VECTOR;
	*(DWORD*)(LOCAL_APIC_BASE + 0x80) = 0;
	*(DWORD*)(LOCAL_APIC_BASE + 0xd0) = 0;
	*(DWORD*)(LOCAL_APIC_BASE + 0xe0) = 0;

	int lint0 = *(DWORD*)(LOCAL_APIC_BASE + 0x350);
	int lint1 = *(DWORD*)(LOCAL_APIC_BASE + 0x360);

	int localapic_ver = *(DWORD*)(LOCAL_APIC_BASE + 0x30);

	//*(DWORD*)(LOCAL_APIC_BASE + 0x350) = 0x700;
	//*(DWORD*)(LOCAL_APIC_BASE + 0x360) = 0x400;

#ifdef IO_APIC_ENABLE
	__asm {cli}

	ret = DisableLocalApicLVT();

	ret = InitLocalApicTimer();

	__asm {sti}
#endif

	//ret = InitLocalApicTimer();

	unsigned int cpuid = *(DWORD*)(LOCAL_APIC_BASE + 0x20)>>24;

	//__enterLock(&g_allocate_ap_lock);
	__enterSpinlock(&g_allocate_ap_lock);

	//__printf(szout, "cpu:%d enter lock\r\n", cpuid);

	int seq = *(int*)AP_TOTAL_ADDRESS;
	int* apids = (int*)AP_ID_ADDRESS;
	apids[seq] = cpuid;
	*(int*)(AP_TOTAL_ADDRESS) = seq + 1;

	int ioapic_id = ReadIoApicReg(0) >> 24;

	int ioapic_ver = ReadIoApicReg(1) & 0xff;

	//WriteIoApicReg(0, cpuid << 24);

	unsigned long stacktop = (unsigned long)(AP_KSTACK_BASE + KTASK_STACK_SIZE * (cpuid + 1) - STACK_TOP_DUMMY);
	unsigned long stack0top = (unsigned long)(AP_STACK0_BASE + TASK_STACK0_SIZE * (cpuid + 1) - STACK_TOP_DUMMY);

	int tssSize = (sizeof(PROCESS_INFO) + 0xfff) & 0xfffff000;
	//tssSize = sizeof(PROCESS_INFO);
	LPPROCESS_INFO process = (LPPROCESS_INFO)(AP_TASK_TSS_BASE + tssSize * cpuid);
	initKernelTss((TSS*)&process->tss, stack0top,stacktop, 0, PDE_ENTRY_VALUE, 0);

	makeTssDescriptor((unsigned long)process, 3, sizeof(TSS) - 1,(TssDescriptor*)(GDT_BASE + AP_TSS_SELECTOR + cpuid * sizeof(TssDescriptor)));

	char procname[64];
	__sprintf(procname, "apic_process_%d", cpuid);

	TASKRESULT freeTss;
	__getFreeTask(&freeTss);
	int tid =freeTss.number;

	process->tss.cr3 = PDE_ENTRY_VALUE;
	__strcpy(process->filename, (char*)procname);
	__strcpy(process->funcname, (char*)procname);	
	process->cpuid = cpuid;
	process->tid = tid;
	process->pid = tid;
	process->espbase = (unsigned long)stacktop;
	process->level = 0;
	process->counter = 0;
	process->vaddr = 0;
	process->vasize = 0;
	process->moduleaddr = (DWORD)0;
	process->videoBase = (char*)gGraphBase;
	process->showX = 0;
	process->showY = GRAPHCHAR_HEIGHT * cpuid * 16+256;
	process->window = 0;
	process->sleep = 0;
	process->copyMap = 0;
	process->status = TASK_RUN;

	__memcpy((char*)freeTss.lptss, (char*)process, sizeof(PROCESS_INFO));

	DescriptTableReg gdtbase;
	__asm {
		sgdt gdtbase
	}

	gdtbase.addr = GDT_BASE;
	gdtbase.size = AP_TSS_SELECTOR + (cpuid + 1) * sizeof(TssDescriptor) - 1;

	short ltr_offset = AP_TSS_SELECTOR + (cpuid) * sizeof(TssDescriptor);

	__asm {
		//do not use lgdt lpgdt,why?
		lgdt gdtbase

		mov ax, ltr_offset
		ltr ax

		mov eax, PDE_ENTRY_VALUE
		mov cr3, eax

		mov eax, cr0
		or eax, 0x80000000
		mov cr0, eax

		//mov ax, KERNEL_MODE_CODE
		//mov ds,ax
		//mov es,ax
		//mov fs,ax
		//mov gs,ax
		//mov ss,ax
	}

	//AdjustApIDT();

	DescriptTableReg idtbase;
	idtbase.size = 256 * sizeof(SegDescriptor) - 1;
	idtbase.addr = IDT_BASE;
	__asm {
		lidt idtbase
	}

	initCoprocessor();
	enableVME();
	enablePCE();
	enableMCE();
	enableTSD();

	initDebugger();

	EnableSyscall();

	sysEntryInit((DWORD)sysEntry);

	__asm {sti}

	__printf(szout, "ap id:%d version:%x init complete.esp:%x lint0:%x lint1:%x tid:%d io apic id:%x version:%x\r\n", 
		cpuid, localapic_ver, reg_esp,lint0,lint1,tid,ioapic_id, ioapic_ver);

	__leaveSpinlock(&g_allocate_ap_lock);
	//__leaveLock(&g_allocate_ap_lock);

	//__asm{int APIC_IPI_VECTOR}
#ifdef TASK_SWITCH_ARRAY

#else
	InsertTaskList(tid);
#endif

	while (1) {
		__asm {
			hlt
		}
#if 0
		uint32_t irr4 = *(DWORD*)(LOCAL_APIC_BASE+0x140);
		if (irr4 & (1 << 1)) {
			__printf(szout,"GOT IPI in IRR! 0x%x\n", irr4);
		}
#endif
		//unsigned int cpuid = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
		//__printf(szout, "ap id:%d wake\r\n", cpuid);
	}
}





void BPCodeStart() {

	int ret = 0;
	char szout[256];

	ret = IsApicSupported();
	if(ret == 0){
		return;
	}

	//__asm {cli}
	enableLocalApic();

	enableIoApic();

#if 0
	enableRcba();
#endif
	//__asm {sti}

	int lint0 = *(DWORD*)(LOCAL_APIC_BASE + 0x350);
	int lint1 = *(DWORD*)(LOCAL_APIC_BASE + 0x360);

	*(DWORD*)(LOCAL_APIC_BASE + 0xf0) = 0x100| APIC_SPURIOUS_VECTOR;

	*(DWORD*)(LOCAL_APIC_BASE + 0x80) = 0;
	*(DWORD*)(LOCAL_APIC_BASE + 0xd0) = 0;
	*(DWORD*)(LOCAL_APIC_BASE + 0xe0) = 0;
	*(int*)(AP_TOTAL_ADDRESS) = 0;

	//in bsp the bit 8 of LOCAL_APIC_BASE is set
	g_bsp_id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;	

	int ioapic_id = ReadIoApicReg(0) >> 24;

	int ioapic_ver = ReadIoApicReg(1) & 0xff;

	int localapic_ver = *(DWORD*)(LOCAL_APIC_BASE+0x30);

	//WriteIoApicReg(0, g_bsp_id << 24);

	DWORD v = 0;

#if 0
	WaitIcrFree();
	v = 0xc4500;
	*(DWORD*)(LOCAL_APIC_BASE + 0x300) = v;
#else
	SetIcr(0, 0, 5, 3);
#endif
	__sleep(0);

#if 0
	WaitIcrFree();
	v = 0xc4600 | (AP_INIT_ADDRESS >> 12);
	*(DWORD*)(LOCAL_APIC_BASE + 0x300) = v;
#else
	SetIcr(0, AP_INIT_ADDRESS >> 12, 6, 3);
#endif
	__sleep(0);

#if 0
	WaitIcrFree();
	v = 0xc4600 | (AP_INIT_ADDRESS >> 12);
	*(DWORD*)(LOCAL_APIC_BASE + 0x300) = v;
	__sleep(0);
#else
	SetIcr(0, AP_INIT_ADDRESS >> 12, 6, 3);
#endif
	__sleep(0);

	int* ids = (int*)AP_ID_ADDRESS;
	int cnt = *(int*)(AP_TOTAL_ADDRESS);
	for (int i = 0; i < cnt; i++) {
#if 0
		SetIcr(ids[i], APIC_IPI_VECTOR, 0,0);
		v = *(DWORD*)(LOCAL_APIC_BASE + 0x300);
		__printf(szout, "%s index:%x,id:%x result:%x\r\n", __FUNCTION__, i, ids[i], v);
#else
		AllocateApTask(APIC_IPI_VECTOR);
#endif
	}

	//InitLocalApicErr();

	//InitLocalApicCmci();

	//ret = InitLocalApicTimer();

#ifdef IO_APIC_ENABLE
	__asm {cli}
	initHpet();
	DisableInt();
		
	ret = DisableLocalApicLVT();

	InitIoApicRte();

	__asm {sti}
#endif

	__sleep(0);
	//AllocateApTask(2);

	__printf(szout, "bsp id:%d version:%x lock:%d init complete. lint0:%x lint1:%x io apic id:%x version:%x\r\n", 
		g_bsp_id, localapic_ver, g_test_value,lint0,lint1, ioapic_id, ioapic_ver);

	return;
}


int IsBspProcessor() {
	unsigned int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20);
	id = id >> 24;
	if (id == g_bsp_id) {
		return 1;
	}
	return 0;
}





LPPROCESS_INFO GetCurrentTaskTssBase(){
	char szout[1024];
	int cnt = *(int*)AP_TOTAL_ADDRESS;
	if (cnt == 0) {
		LPPROCESS_INFO process = (LPPROCESS_INFO)BSP_TASK_TSS_BASE;
		return process;
	}
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20);
	id = id >> 24;
	
	//__enterSpinlock(&g_allocate_ap_lock);
	if(id == g_bsp_id){
		LPPROCESS_INFO process = (LPPROCESS_INFO)BSP_TASK_TSS_BASE;
		//__leaveSpinlock(&g_allocate_ap_lock);
		return process;
	}

	int* apids = (int*)AP_ID_ADDRESS;
	for(int i = 0;i< cnt;i ++){
		if(apids[i] == id){
			//__leaveSpinlock(&g_allocate_ap_lock);
			int tsssize = (sizeof(PROCESS_INFO) + 0xfff) & 0xfffff000;
			LPPROCESS_INFO process = (LPPROCESS_INFO)(AP_TASK_TSS_BASE + tsssize * id);
			return process;
		}
	}

	__printf(szout, "%s error\r\n", __FUNCTION__);
	return 0;
}



void BubbleSort(int* arr, int count) {
	for (int i = 0; i < count - 1; i++) {
		for (int j = 0; j < count - i - 1; j++) {
			int low = arr[j] & 0x00ffffff;
			int high = arr[j + 1] & 0x00ffffff;
			if (low > high) {
				int temp = arr[j];
				arr[j] = arr[j + 1];
				arr[j + 1] = temp;
			}
		}
	}
}


int GetIdleProcessor() {
	char szout[1024];

	int cpuStatus[256];
	__memset((char*)cpuStatus, 0, 256);

	int total = 0;

	int cnt = *(int*)AP_TOTAL_ADDRESS;
	if (cnt <= 0) {
		return g_bsp_id;
	}

	LPPROCESS_INFO proc = (LPPROCESS_INFO)TASKS_TSS_BASE;
	for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
		if (proc[i].status == TASK_RUN ) {
			int cpuid = proc[i].cpuid;
			if(cpuid < 0 || cpuid >=256){
				__printf(szout, "%s cpuid error:%d\r\n",__FUNCTION__, cpuid);
				break;
			}
			unsigned int c = cpuStatus[cpuid] & 0xffffff;
			unsigned int num = cpuid<<24;
			c++;
			cpuStatus[cpuid] = (num) | (c);
			total++;
		}
	}

	if (total) {
		BubbleSort(cpuStatus, cnt+1);
	}

	unsigned int c = cpuStatus[0] & 0xffffff;
	unsigned int num = ( cpuStatus[0] & 0xff000000) >> 24;
	return num;
}