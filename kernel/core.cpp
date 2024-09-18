#include "core.h"
#include "def.h"
#include "process.h"
#include "Utils.h"
#include "Pe.h"
#include "malloc.h"
#include "page.h"
#include "video.h"
#include "Utils.h"
#include "Kernel.h"
#include "exception.h"
#include "debugger.h"
#include "keyboard.h"
#include "serialUART.h"
#include "mouse.h"
#include "v86.h"
#include "servicesProc.h"
#include "task.h"
#include "vectorRoutine.h"
#include "descriptor.h"
#include "floppy.h"
#include "parallel.h"
#include "soundBlaster/sbPlay.h"
#include "apic.h"


void makeDataSegDescriptor(DWORD base, int dpl, int bit, int direction, int w, SegDescriptor* descriptor) {
	descriptor->present = 1;
	descriptor->avl = 0;
	descriptor->unused = 0;
	descriptor->system = 1;
	descriptor->code = 0;
	descriptor->r_w = w;
	descriptor->access = 0;
	if (bit == 16) {
		descriptor->granularity = 0;
		descriptor->db = 0;
	}
	else {
		descriptor->granularity = 1;
		descriptor->db = 1;
	}

	if (direction) {
		descriptor->ext_conform = 1;
	}
	else {
		descriptor->ext_conform = 0;
	}

	descriptor->baseLow = base & 0xffff;
	descriptor->baseMid = (base >> 16) & 0xff;
	descriptor->baseHigh = (base >> 24) & 0xff;
	descriptor->dpl = dpl;
	descriptor->len = 0xffff;
	descriptor->lenHigh = 0xf;
}

void makeCodeSegDescriptor(DWORD base, int dpl, int bit, int conforming, int r, SegDescriptor* descriptor) {
	descriptor->present = 1;
	descriptor->avl = 0;
	descriptor->unused = 0;
	descriptor->system = 1;
	descriptor->code = 1;
	descriptor->r_w = r;
	descriptor->access = 0;
	if (bit == 16) {
		descriptor->granularity = 0;
		descriptor->db = 0;
	}
	else {
		descriptor->granularity = 1;
		descriptor->db = 1;
	}

	if (conforming) {
		descriptor->ext_conform = 1;
	}
	else {
		descriptor->ext_conform = 0;
	}
	descriptor->baseLow = base & 0xffff;
	descriptor->baseMid = (base >> 16) & 0xff;
	descriptor->baseHigh = (base >> 24) & 0xff;
	descriptor->dpl = dpl;
	descriptor->len = 0xffff;
	descriptor->lenHigh = 0xf;
}

void makeTssDescriptor(DWORD base, int dpl,  int size, TssDescriptor* descriptor) {
	descriptor->present = 1;
	descriptor->dpl = dpl;
	descriptor->system = 0;
	descriptor->type = TSS_DESCRIPTOR;

	descriptor->avl = 0;
	descriptor->unused = 0;
	descriptor->db = 0;
	descriptor->granularity = 0;
	descriptor->baseLow = base & 0xffff;
	descriptor->baseMid = (base >> 16) & 0xff;
	descriptor->baseHigh = (base >> 24) & 0xff;
	descriptor->len = size & 0xffff;
	descriptor->lenHigh = (size >> 16) & 0xf;
}


void makeLDTDescriptor(DWORD base, int dpl, int size, TssDescriptor* descriptor) {
	descriptor->present = 1;
	descriptor->dpl = dpl;
	descriptor->system = 0;
	descriptor->type = LDT_DESCRIPTOR;

	descriptor->avl = 0;
	descriptor->unused = 0;
	descriptor->db = 0;
	descriptor->granularity = 0;
	descriptor->baseLow = base & 0xffff;
	descriptor->baseMid = (base >> 16) & 0xff;
	descriptor->baseHigh = (base >> 24) & 0xff;
	descriptor->len = size & 0xffff;
	descriptor->lenHigh = (size >> 16) & 0xf;
}

void makeTaskGateDescriptor(DWORD selector, int dpl, TaskGateDescriptor* descriptor) {

	descriptor->type = TASKGATE_DESCRIPTOR;
	descriptor->system = 0;
	descriptor->dp1 = dpl;
	descriptor->present = 1;
	descriptor->selector = selector;
	descriptor->unused1 = 0;
	descriptor->unused2 = 0;
	descriptor->unused3 = 0;
}



void makeCallGateDescriptor(DWORD base, DWORD selector, int dpl, int paramcnt, CallGateDescriptor* descriptor) {
	descriptor->present = 1;
	descriptor->paramCnt = paramcnt;
	descriptor->system = 0;
	descriptor->type = CALLGATE_DESCRIPTOR;
	descriptor->dpl = dpl;
	descriptor->selector = selector;
	descriptor->baseLow = base & 0xffff;
	descriptor->baseHigh = (base >> 16) & 0xffff;
}

void makeIntGateDescriptor(DWORD base, DWORD selector, int dpl, IntTrapGateDescriptor* descriptor) {
	descriptor->present = 1;
	descriptor->system = 0;
	descriptor->type = INTGATE_DESCRIPTOR;
	descriptor->dpl = dpl;
	descriptor->unused = 0;
	descriptor->selector = selector;
	descriptor->baseLow = base & 0xffff;

	descriptor->baseHigh = (base >> 16) & 0xffff;
}

void makeTrapGateDescriptor(DWORD base, DWORD selector, int dpl, IntTrapGateDescriptor* descriptor) {
	descriptor->present = 1;
	descriptor->system = 0;
	descriptor->type = TRAPGATE_DESCRIPTOR;
	descriptor->unused = 0;
	descriptor->dpl = dpl;
	descriptor->selector = selector;
	descriptor->baseLow = base & 0xffff;
	descriptor->baseHigh = (base >> 16) & 0xffff;
}

//http://www.rcollins.org/articles/vme1/

/*
The TSS has been extended to include a 32-byte interrupt redirection bit map. 
32-bytes is exactly 256 bits, one bit for each software interrupt which can be invoked via the INT-n instruction. 
This bit map resides immediately below the I/O permission bit map (see Figure 1). 
The definition of the I/O Base field in the TSS is therefore extended and dual purpose.
Not only does the I/O Base field point to the base of the I/O permission bit map, 
but also to the end (tail) of the interrupt redirection bit map.
This structure behaves exactly like the I/O permission bit map, except that it controls software interrupts.
When its corresponding bit is set, an interrupt will fault to the Ev86 monitor. 
When its bit is clear, the Ev86 task will service the interrupt without ever leaving Ev86 mode.
*/
/*
Use "Virtual Mode Extensions", which will allow you to give the TSS a "interrupt redirection bitmap", 
telling which interrupt should be processed in virtual mode using the IVT and 
which should be processed in protected mode using the IDT. VME aren't available on QEMU, though.
*/

/*
Enhanced v86 mode was designed to eliminate many of these problems, 
and significantly enhance the performance of v86 tasks running at all IOPL levels. 
When running in Enhanced virtual-8086 mode (Ev86) at IOPL=3, CLI and STI still modify IF. 
This behavior hasn't changed. Running at IOPL<3 has changed. CLI, STI, 
and all other IF-sensitive instructions no longer unconditionally fault to the Ev86 monitor. 
Instead, IF-sensitive instructions clear and set a virtual version of the interrupt flag in the EFLAGS register 
called VIF.[5] Clearing VIF does not block external interrupts, 
as clearing IF does. Instead, IF-sensitive instructions clear and set a virtual version of the interrupt flag called VIF.
VIF does not control external interrupts as IF does.
*/

void initV86Tss(TSS* tss, DWORD esp0, DWORD ip,DWORD cs, DWORD cr3,DWORD ldt) {

	__memset((char*)tss, 0, sizeof(TSS));

	tss->iomapEnd = 0xff;
	tss->iomapOffset = OFFSETOF(TSS, iomapOffset) + SIZEOFMEMBER(TSS, intMap);

	tss->eflags = 0x223202;

	tss->ds = cs;
	tss->es = cs;
	tss->fs = cs;
	tss->gs = cs;

	tss->ss = cs;
	tss->esp = V86_STACK_SIZE - STACK_TOP_DUMMY;

	tss->esp0 = esp0;
	tss->ss0 = KERNEL_MODE_STACK;

	tss->eip = ip;
	tss->cs = cs;

	tss->cr3 = cr3;
	tss->ldt = ldt;
}


void initKernelTss(TSS* tss, DWORD esp0, DWORD reg_esp, DWORD eip, DWORD cr3, DWORD ldt) {

	__memset((char*)tss, 0, sizeof(TSS));

	tss->iomapEnd = 0xff;
	tss->iomapOffset = OFFSETOF(TSS, iomapOffset) + SIZEOFMEMBER(TSS, intMap);

	tss->eflags = 0x203202;

	tss->ds = KERNEL_MODE_DATA;
	tss->es = KERNEL_MODE_DATA;
	tss->fs = KERNEL_MODE_DATA;
	tss->gs = KERNEL_MODE_DATA;

	tss->ss = KERNEL_MODE_DATA;
	tss->esp = reg_esp;

	tss->esp0 = esp0;
	tss->ss0 = KERNEL_MODE_STACK;

	tss->eip = eip;
	tss->cs = KERNEL_MODE_CODE;

	tss->ldt = ldt;
	tss->cr3 = cr3;
}



void initGdt() {

	DescriptTableReg gdtbase;
	__asm {
		sgdt gdtbase;
	}

	char szout[1024];
	__printf(szout, "gdt base:%x,size:%x\r\n", gdtbase.addr, gdtbase.size);

	__memset((char*)GDT_BASE, 0, sizeof(SegDescriptor) * 8192);
	__memcpy((char*)GDT_BASE, (char*)gdtbase.addr, gdtbase.size + 1);

	SegDescriptor* gdt = (SegDescriptor*)GDT_BASE;
	makeCodeSegDescriptor(0, 0, 32, 0, 1, gdt + 1);
	makeDataSegDescriptor(0, 0, 32, 0, 1, gdt + 2);
	makeCodeSegDescriptor(0, 3, 32, 0, 1, gdt + 3);
	makeDataSegDescriptor(0, 3, 32, 0, 1, gdt + 4);

	makeCallGateDescriptor((DWORD)__kCallGateProc, KERNEL_MODE_CODE, 3, 2, (CallGateDescriptor*)(GDT_BASE + callGateSelector));

	makeLDTDescriptor(LDT_BASE, 3, 0x27, (TssDescriptor*)(GDT_BASE + ldtSelector));

	__memset((char*)LDT_BASE, 0, sizeof(SegDescriptor) * 8192);
	SegDescriptor* ldt = (SegDescriptor*)LDT_BASE;
	//ldt sequence number start form 1 not like gdt starting from 0
	makeCodeSegDescriptor(0, 0, 32, 0, 1, ldt + 0);
	makeDataSegDescriptor(0, 0, 32, 0, 1, ldt + 1);
	makeCodeSegDescriptor(0, 3, 32, 0, 1, ldt + 2);
	makeDataSegDescriptor(0, 3, 32, 0, 1, ldt + 3);

	initKernelTss((TSS*)CURRENT_TASK_TSS_BASE,TASKS_STACK0_BASE + TASK_STACK0_SIZE - STACK_TOP_DUMMY,KERNEL_TASK_STACK_TOP, 0, PDE_ENTRY_VALUE, 0);
	makeTssDescriptor(CURRENT_TASK_TSS_BASE, 3, sizeof(TSS) - 1, (TssDescriptor*)(GDT_BASE + kTssTaskSelector));

	initKernelTss((TSS*)INVALID_TSS_BASE, TSSEXP_STACK0_TOP, TSSEXP_STACK_TOP, (DWORD)InvalidTss, PDE_ENTRY_VALUE, 0);
	makeTssDescriptor((DWORD)INVALID_TSS_BASE, 3, sizeof(TSS) - 1, (TssDescriptor*)(GDT_BASE + kTssExceptSelector));

	initKernelTss((TSS*)TIMER_TSS_BASE, TSSTIMER_STACK0_TOP, TSSTIMER_STACK_TOP, (DWORD)TimerInterrupt, PDE_ENTRY_VALUE, 0);
	makeTssDescriptor((DWORD)TIMER_TSS_BASE, 3, sizeof(TSS) - 1, (TssDescriptor*)(GDT_BASE + kTssTimerSelector));

	initV86Tss((TSS*)V86_TSS_BASE, TSSV86_STACK0_TOP, gV86IntProc,gKernel16 , PDE_ENTRY_VALUE, 0);
	makeTssDescriptor((DWORD)V86_TSS_BASE, 3, sizeof(TSS) - 1, (TssDescriptor*)(GDT_BASE + kTssV86Selector));


	gdtbase.addr = GDT_BASE;
	__asm {
		//do not use lgdt lpgdt,why?
		lgdt gdtbase

		mov ax, kTssTaskSelector
		ltr ax

		mov ax, ldtSelector
		lldt ax
	}
}


void initIDT() {

#ifdef _DEBUG
	SegDescriptor* gdt = (SegDescriptor*)new char[0x10000];
	IntTrapGateDescriptor* descriptor = (IntTrapGateDescriptor*)new char[0x10000];
#else

	IntTrapGateDescriptor* descriptor = (IntTrapGateDescriptor*)IDT_BASE;
#endif

	for (int i = 0; i < 256; i++)
	{
		makeTrapGateDescriptor((DWORD)AnonymousException, KERNEL_MODE_CODE, 3, descriptor + i);
	}

	makeTrapGateDescriptor((DWORD)DivideError, KERNEL_MODE_CODE, 3, descriptor + 0);
	makeTrapGateDescriptor((DWORD)DebugTrap, KERNEL_MODE_CODE, 3, descriptor + 1);

	makeTrapGateDescriptor((DWORD)NmiInterrupt, KERNEL_MODE_CODE, 3, descriptor + 2);

	makeTrapGateDescriptor((DWORD)BreakPointTrap, KERNEL_MODE_CODE, 3, descriptor + 3);

	makeTrapGateDescriptor((DWORD)OverflowException, KERNEL_MODE_CODE, 3, descriptor + 4);
	makeTrapGateDescriptor((DWORD)BoundRangeExceed, KERNEL_MODE_CODE, 3, descriptor + 5);
	makeTrapGateDescriptor((DWORD)UndefinedOpcode, KERNEL_MODE_CODE, 3, descriptor + 6);
	makeTrapGateDescriptor((DWORD)DeviceUnavailable, KERNEL_MODE_CODE, 3, descriptor + 7);
	makeTrapGateDescriptor((DWORD)DoubleFault, KERNEL_MODE_CODE, 3, descriptor + 8);

	makeTrapGateDescriptor((DWORD)CoprocSegOverrun, KERNEL_MODE_CODE, 3, (descriptor + 9));

	makeTaskGateDescriptor((DWORD)kTssExceptSelector, 3, (TaskGateDescriptor*)(descriptor + 10));

	makeTrapGateDescriptor((DWORD)SegmentUnpresent, KERNEL_MODE_CODE, 3, descriptor + 11);
	makeTrapGateDescriptor((DWORD)StackSegFault, KERNEL_MODE_CODE, 3, descriptor + 12);
	makeTrapGateDescriptor((DWORD)GeneralProtection, KERNEL_MODE_CODE, 3, descriptor + 13);
	makeTrapGateDescriptor((DWORD)PageFault, KERNEL_MODE_CODE, 3, descriptor + 14);
	makeTrapGateDescriptor((DWORD)AnonymousException, KERNEL_MODE_CODE, 3, descriptor + 15);
	makeTrapGateDescriptor((DWORD)FloatPointError, KERNEL_MODE_CODE, 3, descriptor + 16);
	makeTrapGateDescriptor((DWORD)AlignmentCheck, KERNEL_MODE_CODE, 3, descriptor + 17);
	makeTrapGateDescriptor((DWORD)MachineCheck, KERNEL_MODE_CODE, 3, descriptor + 18);
	makeTrapGateDescriptor((DWORD)SIMDException, KERNEL_MODE_CODE, 3, descriptor + 19);
	makeTrapGateDescriptor((DWORD)VirtualizationException, KERNEL_MODE_CODE, 3, descriptor + 20);
	makeTrapGateDescriptor((DWORD)CtrlProtectException, KERNEL_MODE_CODE, 3, descriptor + 21);

#ifdef SINGLE_TASK_TSS
	makeIntGateDescriptor((DWORD)TimerInterrupt, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_MASTER + 0);
#else
	makeTaskGateDescriptor((DWORD)kTssTimerSelector, 3, (TaskGateDescriptor*)(descriptor + INTR_8259_MASTER + 0));
#endif

	makeIntGateDescriptor((DWORD)KeyboardIntProc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_MASTER + 1);
	makeIntGateDescriptor((DWORD)SlaveIntProc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_MASTER + 2);
	makeIntGateDescriptor((DWORD)__kCom2Proc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_MASTER + 3);
	makeIntGateDescriptor((DWORD)__kCom1Proc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_MASTER + 4);
	makeIntGateDescriptor((DWORD)Parallel2IntProc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_MASTER + 5);
	makeIntGateDescriptor((DWORD)SoundInterruptProc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_MASTER + 5);
	makeIntGateDescriptor((DWORD)Parallel1IntProc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_MASTER + 7);
	makeIntGateDescriptor((DWORD)FloppyIntProc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_MASTER + 6);

	makeIntGateDescriptor((DWORD)CmosInterrupt, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_SLAVE + 0);
	makeIntGateDescriptor((DWORD)Slave1IntProc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_SLAVE + 1);
	makeIntGateDescriptor((DWORD)NetcardIntProc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_SLAVE + 2);
	makeIntGateDescriptor((DWORD)USBIntProc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_SLAVE + 3);
	makeIntGateDescriptor((DWORD)MouseIntProc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_SLAVE + 4);
	makeIntGateDescriptor((DWORD)CoprocessorIntProc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_SLAVE + 5);
	makeIntGateDescriptor((DWORD)IDEMasterIntProc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_SLAVE + 6);
	makeIntGateDescriptor((DWORD)IDESlaveIntProc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_SLAVE + 7);

	makeTrapGateDescriptor((DWORD)IPIIntHandler, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_SLAVE + 8);

	makeTrapGateDescriptor((DWORD)servicesProc, KERNEL_MODE_CODE, 3, descriptor + 0x80);

	makeTaskGateDescriptor((DWORD)kTssV86Selector, 3, (TaskGateDescriptor*)(descriptor + 0xff));

	DescriptTableReg idtbase;
	idtbase.size = 256 * sizeof(SegDescriptor) - 1;
	idtbase.addr = IDT_BASE;
	char szout[1024];
	__printf(szout, "idt base:%x,size:%x\r\n", idtbase.addr, idtbase.size);
	__asm {
		//不要使用 lidt lpidt,why?
		lidt idtbase
	}
}