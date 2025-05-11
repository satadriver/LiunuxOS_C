

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


int gAllocateAp = 0;

unsigned long g_allocate_ap_lock = 0;

#define APIC_CORE_MAX_COUNT	64

DWORD * gApicBase = 0;
DWORD * gSvrBase = 0;
DWORD * gOicBase = 0;
DWORD * gHpetBase = 0;
DWORD * gRcbaBase = 0;

int g_ApNumber = 0;

int g_LocalApicID[APIC_CORE_MAX_COUNT];

int g_IoApicID[APIC_CORE_MAX_COUNT];


void enableRcba() {
	outportd(0xcf8, 0x8000f8f0);

	gRcbaBase = (DWORD*)(inportd(0xcfc) & 0xffffc000);
	*gRcbaBase = *gRcbaBase | 1;
}

void enableFerr() {
	outportd(0xcf8, 0x8000f8f0);

	gRcbaBase = (DWORD*)(inportd(0xcfc) & 0xffffc000);

	gOicBase = (DWORD*)((DWORD)gRcbaBase + 0x31fe);

	DWORD v = *gOicBase;

	v = (v & 0xffffff00) | 0x200;

	*gOicBase = v;

}

void iomfence() {
	__asm {
		mfence
	}
}

void setIoApicID(int id) {
	iomfence();
	*(DWORD*)(0xfec00000) = 0;
	iomfence();
	*(DWORD*)(0xfec00010) = (id & 0x0ff) << 24 ;

}

void setIoRedirect(int idx,int id,int vector,int mode) {
	*(DWORD*)(0xfec00000) = idx ;
	iomfence();
	*(DWORD*)(0xfec00010) = vector + ( mode << 8);
	iomfence();
	*(DWORD*)(0xfec00000) = idx +1;
	iomfence();
	*(DWORD*)(0xfec00010) = ((id & 0x0ff) << 24) ;
}


void enableIoApic() {

	outportd(0xcf8, 0x8000f8f0);

	gRcbaBase = (DWORD*)(inportd(0xcfc) & 0xffffc000);

	gOicBase = (DWORD*)((DWORD)gRcbaBase + 0x31fe);

	DWORD v = *gOicBase;
	
	v = (v & 0xffffff00) | 0x100;

	*gOicBase = v;
}


//fec00000
//fed00000
//fee00000
DWORD* getOicBase() {
	outportd(0xcf8, 0x8000f8f0);
	DWORD rcba = inportd(0xcfc) & 0xffffc000;

	gOicBase = (DWORD*)(rcba + 0x31fe);

	return gOicBase;
}

DWORD* getRcbaBase() {
	outportd(0xcf8, 0x8000f8f0);
	DWORD base = inportd(0xcfc) & 0xffffc000;

	gRcbaBase = (DWORD*)base;

	return gRcbaBase;
}

int enableLocalApic() {

	DWORD high = 0;
	DWORD low = 0;
	int res = 0;
	readmsr(0x1b,&low,&high);
	low = low | 0xc00;
	writemsr(0x1b, low, high);

	gApicBase = (DWORD*)(low & 0xfffff000);

	gSvrBase = (DWORD*)((DWORD)gApicBase + 0xf0);
	*gSvrBase = *gSvrBase | 0x1100;

	int id = getLocalApicID();

	return id;
}

int enableHpet() {
	outportd(0xcf8, 0x8000f8f0);

	DWORD addr = inportd(0xcfc) & 0xffffc000;

	gHpetBase = (DWORD*)(addr + 0x3404);

	DWORD v = *gHpetBase;

	v = (v | 0x80) & 0xfffffffc;

	*gHpetBase = v;

	return 0;
}



void enableIMCR() {
	outportb(0x22, 0x70);
	outportb(0x23, 0x01);
}


int getLVTCount(int n) {
	int cnt = *(long long*)(0xfee00030) >>16;
	return cnt;
}

int getLocalApicVersion() {
	int ver = *(long long*)(0xfee00030) & 0xff;
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


extern "C" void __declspec(dllexport) __kBspInitProc() {

	enableLocalApic();

	int id = getLocalApicID();

	g_LocalApicID[g_ApNumber] = id;

	enableIoApic();

	setIoApicID(g_ApNumber);

	setIoRedirect(0x12, id, INTR_8259_MASTER + 1, 0);
	setIoRedirect(0x14, id, INTR_8259_MASTER , 0);
	setIoRedirect(0x16, id, INTR_8259_MASTER + 3, 0);
	setIoRedirect(0x18, id, INTR_8259_MASTER + 4, 0);
	setIoRedirect(0x1a, id, INTR_8259_MASTER + 5, 0);
	setIoRedirect(0x1c, id, INTR_8259_MASTER + 6, 0);
	setIoRedirect(0x1e, id, INTR_8259_MASTER + 7, 0);

	setIoRedirect(0x20, id, INTR_8259_SLAVE + 0, 0);
	setIoRedirect(0x22, id, INTR_8259_SLAVE + 1, 0);
	setIoRedirect(0x24, id, INTR_8259_SLAVE + 2, 0);
	setIoRedirect(0x26, id, INTR_8259_SLAVE + 3, 0);
	setIoRedirect(0x28, id, INTR_8259_SLAVE + 4, 0);
	setIoRedirect(0x2a, id, INTR_8259_SLAVE + 5, 0);
	setIoRedirect(0x2c, id, INTR_8259_SLAVE + 6, 0);
	setIoRedirect(0x2e, id, INTR_8259_SLAVE + 7, 0);

	setIoRedirect(0x30, id, INTR_8259_SLAVE + 8, 0);

	*(DWORD*)0xfee00300 = 0xc4500;	//发送 INIT IPI, 使所有 processor 执行 INIT
	iomfence();

	DWORD addr = AP_INIT_ADDRESS >> 12;

	*(DWORD*)0xfee00300 = 0xc4600 | addr;	//发送 Start - up IPI，AP的起始物理地址为0x1B000
	iomfence();

	*(DWORD*)0xfee00300 = 0xc4600 | addr;	//再次发送 Start - up IPI，AP的起始物理地址为0x1B000
	iomfence();

	enableIMCR();

	outportb(0x21, 0xff);
	outportb(0xa1, 0xff);

	*(DWORD*)0xfee00350 = 0x10000;

	initHpet();
}


int initHpet() {
	int res = 0;

	enableHpet();

	long long id = *(long long*)APIC_HPET_BASE;

	HPET_GCAP_ID_REG * gcap = (HPET_GCAP_ID_REG*)&id;
	if (gcap->tick == 0x0429b17f) {
		int cnt = gcap->count;

		DWORD tick = gcap->tick;

		long long total = 143182;		// 14318179 = 1000ms,0x0429b17f

		*(long long*)(APIC_HPET_BASE + 0x10) = 3;

		*(long long*)(APIC_HPET_BASE + 0x20) = 0;

		long long* regs = (long long*)(APIC_HPET_BASE + 0x100);	

		for (int i = 0; i < cnt; i++) {
			if (i == 0) {
				regs[i] = 0x40 + 8 + 4 + 2 - 2;
			}
			else if (i == 2) {
				regs[i] = 0x1000 + 0x40 + 4 + 2 - 2;
			}
			else if (i % 2 == 0) {
				regs[i] = 0x40 + 2 - 2;
			}
			else {
				regs[i] = total;
			}
			i += 2;
		}

		*(long long*)(APIC_HPET_BASE + 0xf0) = 0;

		return TRUE;
	}

	return FALSE;
}







extern "C" void __declspec(naked) HpetInterrupt(LIGHT_ENVIRONMENT * stack) {
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
		LPPROCESS_INFO process = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
		char szout[1024];

		int tag = *(int*)(APIC_HPET_BASE + 0x20);
		if (tag & 1) {
			__kTaskSchedule((LIGHT_ENVIRONMENT*)stack);
			*(long*)(APIC_HPET_BASE + 0x108) = 0;
		}
		else if (tag & 2) {
			DWORD c = *(long*)(APIC_HPET_BASE + 0xf0);
			DWORD cmp = *(long*)(APIC_HPET_BASE + 0x128);
		}

		*(long *)0xFEE000B0 = 0;
	}

	__asm {
#ifdef SINGLE_TASK_TSS
		mov eax, dword ptr ds : [CURRENT_TASK_TSS_BASE + PROCESS_INFO.tss.cr3]
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

		clts
		iretd

		jmp HpetInterrupt
	}
}



extern "C" void __declspec(dllexport) __kApInitProc_old() {
	DescriptTableReg idtbase;
	idtbase.size = 256 * sizeof(SegDescriptor) - 1;
	idtbase.addr = IDT_BASE;

	initKernelTss((TSS*)AP_TSS_BASE + 0x4000*g_ApNumber, AP_STACK0_BASE + TASK_STACK0_SIZE* (g_ApNumber+1) - STACK_TOP_DUMMY,
		AP_KSTACK_BASE + KTASK_STACK_SIZE*(g_ApNumber + 1)- STACK_TOP_DUMMY, 0, PDE_ENTRY_VALUE, 0);
	makeTssDescriptor(AP_TSS_BASE + 0x4000 * g_ApNumber, 3, sizeof(TSS) - 1, 
		(TssDescriptor*)(GDT_BASE + AP_TSS_DESCRIPTOR + sizeof(TssDescriptor) * g_ApNumber));

	DescriptTableReg gdtbase;
	__asm {
		sgdt gdtbase
	}

	DWORD lptssdesc = (DWORD)(AP_TSS_DESCRIPTOR + sizeof(TssDescriptor) * g_ApNumber);

	gdtbase.addr = GDT_BASE;
	gdtbase.size = AP_TSS_DESCRIPTOR + sizeof(TssDescriptor) * g_ApNumber - 1;
	__asm{
		lgdt gdtbase

		mov eax, lptssdesc
		ltr ax

		mov ax, ldtSelector
		lldt ax

		lidt idtbase
	}

	enableLocalApic();

	int id = getLocalApicID();

	g_LocalApicID[g_ApNumber+1] = id;

	enableIoApic();

	setIoApicID(g_ApNumber+1);

	g_ApNumber++;

	char szout[1024];
	__printf(szout, "idt base:%x,size:%x\r\n", idtbase.addr, idtbase.size);

	*(DWORD*)0xFEE00310 = 0;
	*(DWORD*)0xFEE00300 = 0x4030;

	__asm {
		sti
		hlt
	}
}

/*
短跳转（Short Jmp，只能跳转到256字节的范围内），对应机器码：EB
近跳转（Near Jmp，可跳至同一段范围内的地址），对应机器码：E9
近跳转（Near call，可跳至同一段范围内的地址），对应机器码：E8
远跳转（Far Jmp，可跳至任意地址），对应机器码： EA
远跳转（Far call，可跳至任意地址），对应机器码： 9A
ff 15 call
ff 25 call
*/




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
		*(DWORD*)0xFEE000B0 = 0;
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

//int g_ap_count = 0;

//int g_ap_ids[256] = { 0 };

//https://blog.csdn.net/weixin_46645613/article/details/120406002
//https://zhuanlan.zhihu.com/p/406213995
//https://zhuanlan.zhihu.com/p/678582090
//https://www.zhihu.com/question/594531181/answer/2982337869

extern "C" void __declspec(dllexport) __kApInitProc() {
	__asm {
		cli
	}

	char szout[1024];

	__enterSpinlock(&g_allocate_ap_lock);
	int id = *(DWORD*)0xFEE00020;
	id = id >> 24;
	int seq = *(int*)AP_TOTAL_ADDRESS;
	int* apids = (int*)AP_ID_ADDRESS;
	apids[seq] = id;
	*(int*)AP_TOTAL_ADDRESS = seq + 1;
	__leaveSpinlock(&g_allocate_ap_lock);

	__printf(szout, "AP:%d %s entry\r\n", id,__FUNCTION__);

	int tsssize = (sizeof(PROCESS_INFO) + 0xfff) & 0xfffff;
	initKernelTss((TSS*)AP_TSS_BASE + tsssize * seq, AP_STACK0_BASE + TASK_STACK0_SIZE * (seq + 1) - STACK_TOP_DUMMY,
		AP_KSTACK_BASE + KTASK_STACK_SIZE * (seq + 1) - STACK_TOP_DUMMY, 0, PDE_ENTRY_VALUE, 0);

	makeTssDescriptor(AP_TSS_BASE + tsssize * seq, 3, sizeof(TSS) - 1,(TssDescriptor*)(GDT_BASE + AP_TSS_DESCRIPTOR ));

	DescriptTableReg gdtbase;
	__asm {
		sgdt gdtbase
	}

	gdtbase.addr = GDT_BASE;
	gdtbase.size  +=  sizeof(TssDescriptor);

	__asm {
		//do not use lgdt lpgdt,why?
		lgdt gdtbase

		mov ax, AP_TSS_DESCRIPTOR
		ltr ax

		mov eax, PDE_ENTRY_VALUE
		mov cr3, eax

		mov eax, cr0
		or eax, 0x80000000
		mov cr0, eax	
	}

	DescriptTableReg idtbase;
	idtbase.size = 256 * sizeof(SegDescriptor) - 1;
	idtbase.addr = IDT_BASE;
	__asm {
		//不要使用 lidt lpidt,why?
		lidt idtbase
	}

	initCoprocessor();

	enableVME();
	enablePCE();
	enableMCE();
	enableTSD();

	DWORD v = *(DWORD*)0xFEE000F0;
	v = v | 0x100;
	*(DWORD*)0xFEE000F0 = v;

	__printf(szout, "AP:%d %s complete\r\n", id, __FUNCTION__);

	__asm{sti}

	while (1) {
		__asm {
			
			hlt
		}
	}
}



int AllocateAP(int vn) {

	int total = *(int*)(AP_TOTAL_ADDRESS);
	int* ids = (int*)AP_ID_ADDRESS;
	if (total > 0) {

		
		__enterSpinlock(&g_allocate_ap_lock);

		while (true)
		{
			int value = *(DWORD*)0xFEE00300;
			if (value & 0x1000) {
				__sleep(0);
			}
			else {
				break;
			}
		}


		int id = ids[gAllocateAp]<<24;
		*(DWORD*)0xFEE00310 = id;
			
		int v = 0x4000|vn;
		*(DWORD*)0xFEE00300 = v;
		

		gAllocateAp++;
		if (gAllocateAp >= total) {
			gAllocateAp = 0;
		}

		__leaveSpinlock(&g_allocate_ap_lock);
		return gAllocateAp;
		
	}

	return 0;
}



void BPCodeStart() {

	*(int*)AP_TOTAL_ADDRESS = 0;

	//enableLocalApic();

	int bpid = *(DWORD*)0xFEE00020 >>24;
	char szout[256];
	__printf(szout, "bp id:%d\r\n", bpid);

	DWORD v = *(DWORD*)0xFEE000F0;
	v = v | 0x100;
	*(DWORD*)0xFEE000F0 = v;

	v = 0xc4500;
	*(DWORD*)0xFEE00300 = v;

	__int64 tmp = 0;
	for (int i = 0; i < 0x100000; i++) {
		tmp += i;
	}

	v = 0xc4600 | (AP_INIT_ADDRESS >> 12);
	*(DWORD*)0xFEE00300 = v;

	tmp = 1;
	for (int i = 0; i < 0x100000; i++) {
		tmp = tmp *i;
	}
	//return;

	int c = *(int*)AP_TOTAL_ADDRESS;
	if (c > 0) {
		enableIoApic();

		int* ids = (int*)AP_ID_ADDRESS;
		int id = ids[0];
		setIoRedirect(0x12, id, INTR_8259_MASTER + 1, 0);
		setIoRedirect(0x14, id, INTR_8259_MASTER, 0);
		setIoRedirect(0x16, id, INTR_8259_MASTER + 3, 0);
		setIoRedirect(0x18, id, INTR_8259_MASTER + 4, 0);
		setIoRedirect(0x1a, id, INTR_8259_MASTER + 5, 0);
		setIoRedirect(0x1c, id, INTR_8259_MASTER + 6, 0);
		setIoRedirect(0x1e, id, INTR_8259_MASTER + 7, 0);

		setIoRedirect(0x20, id, INTR_8259_SLAVE + 0, 0);
		setIoRedirect(0x22, id, INTR_8259_SLAVE + 1, 0);
		setIoRedirect(0x24, id, INTR_8259_SLAVE + 2, 0);
		setIoRedirect(0x26, id, INTR_8259_SLAVE + 3, 0);
		setIoRedirect(0x28, id, INTR_8259_SLAVE + 4, 0);
		setIoRedirect(0x2a, id, INTR_8259_SLAVE + 5, 0);
		setIoRedirect(0x2c, id, INTR_8259_SLAVE + 6, 0);
		setIoRedirect(0x2e, id, INTR_8259_SLAVE + 7, 0);

		setIoRedirect(0x30, id, INTR_8259_SLAVE + 8, 0);

	}

	return;
}