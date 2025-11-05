

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
void EnableFloatError() {
	outportd(0xcf8, 0x8000f8f0);

	gRcbaBase = (char*)(inportd(0xcfc) & 0xffffc000);

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








int initHpet() {
	int res = 0;

	enableHpet();

	long long id = *(long long*)APIC_HPET_BASE;

	HPET_GCAP_ID_REG * gcap = (HPET_GCAP_ID_REG*)&id;
	if (gcap->tick == 0x0429b17f) 
	{
		int cnt = gcap->count;

		DWORD tick = gcap->tick;

		long long total = 143182;		// 14318179 = 1000ms,0x0429b17f

		*(long long*)(APIC_HPET_BASE + 0x10) = 3;

		*(long long*)(APIC_HPET_BASE + 0x20) = 0;

		long long* regs = (long long*)(APIC_HPET_BASE + 0x100);	
		*(long long*)(APIC_HPET_BASE + 0x100 + 4) = 0;

		for (int i = 0; i < cnt; i++) {
			if (i == 0) {
				regs[i] = 0x40 + 8 + 4 + 2 - 2;
				//break;
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

		*(long long*)(APIC_HPET_BASE + 0x100 + 8) = 143182;

		*(long long*)(APIC_HPET_BASE + 0x100 + 12) = 0;

		*(long long*)(APIC_HPET_BASE + 0xf0) = 0;

		*(long long*)(APIC_HPET_BASE + 0xf0 + 4) = 0;

		char szout[256];
		__printf(szout, "hpet counter:%x\r\n", gcap->tick);

	}

	return FALSE;
}



void IoApicRedirect(int idx, int idnum, int vector, int mode) {

	int id = 0;
	if (idnum == -1) {
		__enterSpinlock(&g_allocate_ap_lock);
		int total = *(int*)(AP_TOTAL_ADDRESS);
		int* ids = (int*)AP_ID_ADDRESS;
		id = ids[gAllocateAp];
		gAllocateAp++;
		if (gAllocateAp >= total) {
			gAllocateAp = 0;
		}
		__leaveSpinlock(&g_allocate_ap_lock);
	}
	else {
		id = idnum;
	}

	*(DWORD*)(0xfec00000) = idx;
	iomfence();
	*(DWORD*)(0xfec00010) = vector + (mode << 8);
	iomfence();
	*(DWORD*)(0xfec00000) = idx + 1;
	iomfence();
	*(DWORD*)(0xfec00010) = ((id & 0x0ff) << 24);
}

int InitIoApic() {
	
	IoApicRedirect(0x14, g_bsp_id, INTR_8259_MASTER, 0xa0);
	IoApicRedirect(0x12, g_bsp_id, INTR_8259_MASTER + 1, 0);
	IoApicRedirect(0x16, g_bsp_id, INTR_8259_MASTER + 3, 0);
	IoApicRedirect(0x18, g_bsp_id, INTR_8259_MASTER + 4, 0);
	IoApicRedirect(0x1a, g_bsp_id, INTR_8259_MASTER + 5, 0);
	IoApicRedirect(0x1c, g_bsp_id, INTR_8259_MASTER + 6, 0);
	IoApicRedirect(0x1e, g_bsp_id, INTR_8259_MASTER + 7, 0);

	IoApicRedirect(0x20, g_bsp_id, INTR_8259_SLAVE + 0, 0);
	IoApicRedirect(0x22, g_bsp_id, INTR_8259_SLAVE + 1, 0);
	IoApicRedirect(0x24, g_bsp_id, INTR_8259_SLAVE + 2, 0);
	IoApicRedirect(0x26, g_bsp_id, INTR_8259_SLAVE + 3, 0);
	IoApicRedirect(0x28, g_bsp_id, INTR_8259_SLAVE + 4, 0);
	IoApicRedirect(0x2a, g_bsp_id, INTR_8259_SLAVE + 5, 0);
	IoApicRedirect(0x2c, g_bsp_id, INTR_8259_SLAVE + 6, 0);
	IoApicRedirect(0x2e, g_bsp_id, INTR_8259_SLAVE + 7, 0);

	IoApicRedirect(0x30, 0, INTR_8259_SLAVE + 8, 0);

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
		*(DWORD*)0xFEE000B0 = 0;

		g_test_value++;

		char szout[256];
		__printf(szout, "IPIIntHandler\r\n");
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
		LPPROCESS_INFO process = (LPPROCESS_INFO)GetCurrentTaskTssBase();
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

		*(long*)0xFEE000B0 = 0;
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

		clts
		iretd

		jmp HpetInterrupt
	}
}

int AllocateApTask(int intnum) {

	if(intnum < 0 || intnum > 255) {
		return -1;
	}

	int res = -1;

	int total = *(int*)(AP_TOTAL_ADDRESS);
	int* ids = (int*)AP_ID_ADDRESS;
	if (total > 0) {
		__enterSpinlock(&g_allocate_ap_lock);

		while (true)
		{
			int value = *(DWORD*)(LOCAL_APIC_BASE + 0x300);
			if (value & 0x1000) {
				__sleep(0);
			}
			else {
				break;
			}
		}

		int currendptr = gAllocateAp;
		int currentid = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
		do {				
			if (currentid != ids[gAllocateAp]) {
				break;
			}

			gAllocateAp++;
			if (gAllocateAp >= total) {
				gAllocateAp = 0;
			}

			if (currendptr == gAllocateAp) {
				__leaveSpinlock(&g_allocate_ap_lock);
				return -1;
			}
		} while (1);

		unsigned int id = ids[gAllocateAp] << 24;
		*(DWORD*)(LOCAL_APIC_BASE + 0x310) = id;

		int v = 0x000 | intnum;
		*(DWORD*)(LOCAL_APIC_BASE + 0x300) = v;

		res = gAllocateAp;

		gAllocateAp++;
		if (gAllocateAp >= total) {
			gAllocateAp = 0;
		}

		__leaveSpinlock(&g_allocate_ap_lock);

		char szout[256];
		__printf(szout, "AllocateApTask index:%x,id:%x\r\n", res, id);
		return res;
	}

	return -1;
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

	char* apic_base = 0;
	unsigned long origin = 0;

	__asm {
		mov ecx, 0x1B
		rdmsr
		mov[origin], eax
		mov eax, LOCAL_APIC_BASE
		wrmsr

		mov edi, eax
		mov dword ptr ds : [edi + 0xF0] , 0x1FF
		mov dword ptr  ds : [edi + 0x80] , 0
		mov dword ptr ds : [edi + 0x350] , 0x10000
		mov dword ptr  ds : [edi + 0x360] , 0x10000

		//mov dword[edi + 0x320], 0x40 | (1 << 17)

		mov al, 0xFF
		out 0xA1, al
		out 0x21, al

		mov ecx, 0x1B
		rdmsr
		mov[apic_base], eax
		or eax, 0x800
		wrmsr
	}

	enableIMCR();

	char szout[256];
	__printf(szout, "local apic base:%x,origin value:%x\r\n", apic_base, origin);
	return 0;
}



//https://blog.csdn.net/weixin_46645613/article/details/120406002
//https://zhuanlan.zhihu.com/p/406213995
//https://zhuanlan.zhihu.com/p/678582090
//https://www.zhihu.com/question/594531181/answer/2982337869





extern "C" void __declspec(dllexport) __kApInitProc() {
	__asm {
		cli
	}
	if (1) {
		//enableLocalApic();	
	}
	char szout[1024];

	*(DWORD*)(LOCAL_APIC_BASE + 0xf0) = 0x1ff;

	__enterSpinlock(&g_allocate_ap_lock);
	unsigned int value = *(DWORD*)(LOCAL_APIC_BASE + 0x20);
	int id = value >> 24;

	int seq = *(int*)AP_TOTAL_ADDRESS;
	int* apids = (int*)AP_ID_ADDRESS;
	apids[seq] = id;
	*(int*)(AP_TOTAL_ADDRESS) = seq + 1;

	__asm {
		mov eax, KTASK_STACK_SIZE
		mov ecx, seq
		inc ecx
		mul ecx
		add eax, AP_KSTACK_BASE
		sub eax, STACK_TOP_DUMMY
		//mov esp,eax
		//mov ebp,eax
	}

	__printf(szout, "ap origin value:%x,id:%d\r\n", value, id);

	int tsssize = (sizeof(PROCESS_INFO) + 0xfff) & 0xfffff000;
	initKernelTss((TSS*)AP_TSS_BASE + tsssize * seq, AP_STACK0_BASE + TASK_STACK0_SIZE * (seq + 1) - STACK_TOP_DUMMY,
		AP_KSTACK_BASE + KTASK_STACK_SIZE * (seq + 1) - STACK_TOP_DUMMY, 0, PDE_ENTRY_VALUE, 0);

	makeTssDescriptor(AP_TSS_BASE + tsssize * seq, 3, sizeof(TSS) - 1, (TssDescriptor*)(GDT_BASE + AP_TSS_DESCRIPTOR + seq * sizeof(TssDescriptor)));

	PROCESS_INFO* process = (PROCESS_INFO*)(AP_TSS_BASE + tsssize * seq);
	//process->tss.cr3 = PDE_ENTRY_VALUE;
	process->showX = 0;
	process->showY = GRAPHCHAR_HEIGHT * seq*8;
	DescriptTableReg gdtbase;
	__asm {
		sgdt gdtbase
	}

	gdtbase.addr = GDT_BASE;
	gdtbase.size = AP_TSS_DESCRIPTOR + (seq + 1) * sizeof(TssDescriptor) - 1;

	short ltr_offset = AP_TSS_DESCRIPTOR + (seq) * sizeof(TssDescriptor);

	__leaveSpinlock(&g_allocate_ap_lock);

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
	}

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

	__asm {sti}

	while (1) {
		__asm {
			hlt
		}
		__printf(szout, "ap id:%d wake\r\n", id);
	}
}


void BPCodeStart() {

	int ret = 0;
	char szout[256];

	ret = IsApicSupported();
	if(ret == 0){
		return;
	}

	*(DWORD*)(LOCAL_APIC_BASE + 0xf0) = 0x1ff;

	__asm{cli}

	*(int*)(AP_TOTAL_ADDRESS) = 0;
	//enableLocalApic();

	//enableIoApic();

#ifdef APIC_ENABLE
	
	InitIoApic();
	initHpet();
#endif
	//in bsp the bit 8 of LOCAL_APIC_BASE is set
	g_bsp_id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;	

	int ioapic_id = *(DWORD*)(IO_APIC_BASE) >> 24;

	int ioapic_ver = *(DWORD*)(IO_APIC_BASE + 1) & 0xff;
	
	__printf(szout, "bsp id:%d,io apic id:%x,version:%x\r\n", g_bsp_id,ioapic_id,ioapic_ver);

	DWORD v = 0xc4500;
	*(DWORD*)(LOCAL_APIC_BASE + 0x300) = v;

	__asm {sti}

	__sleep(1000);

	v = 0xc4600 | (AP_INIT_ADDRESS >> 12);
	*(DWORD*)(LOCAL_APIC_BASE + 0x300) = v;

#ifdef APIC_ENABLE
	//setIoApicID(g_bsp_id);
#endif

	if (1) {
		
		__sleep(1000);
		int * ids = (int*)AP_ID_ADDRESS;
		int cnt = *(int*)(AP_TOTAL_ADDRESS);
		for (int i = 0; i < cnt; i++) {

			
			__printf(szout, "ap num:%d, id:%d\r\n", i,ids[i]);

			unsigned int id = ids[i] << 24;
			*(DWORD*)(LOCAL_APIC_BASE + 0x310) = id;
			int v = 0x81;
			*(DWORD*)(LOCAL_APIC_BASE + 0x300) = v;
		}

		//AllocateApTask(0xff);

		__printf(szout, "bsp id:%d init:%d ok\r\n", g_bsp_id, g_test_value);
	}

	return;
}




LPPROCESS_INFO GetCurrentTaskTssBase(){

	int seq = *(int*)AP_TOTAL_ADDRESS;
	if (seq == 0) {
		LPPROCESS_INFO process = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
		return process;
	}
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20);
	id = id >> 24;
	
	//__enterSpinlock(&g_allocate_ap_lock);
	if(id == g_bsp_id){
		LPPROCESS_INFO process = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
		//__leaveSpinlock(&g_allocate_ap_lock);
		return process;
	}

	int* apids = (int*)AP_ID_ADDRESS;
	for(int i = 0;i<seq;i ++){
		if(apids[i] == id){
			//__leaveSpinlock(&g_allocate_ap_lock);
			int tsssize = (sizeof(PROCESS_INFO) + 0xfff) & 0xfffff000;
			LPPROCESS_INFO process = (LPPROCESS_INFO)(AP_TSS_BASE + tsssize * i);
			return process;
		}
	}

	LPPROCESS_INFO process = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
	return process;

}