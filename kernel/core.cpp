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
#include "core.h"
#include "servicesProc.h"
#include "task.h"
#include "vectorRoutine.h"
#include "descriptor.h"
#include "floppy.h"
#include "parallel.h"
#include "soundBlaster/sbPlay.h"
#include "apic.h"
#include "file.h"
#include "pe64.h"
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
	descriptor->selector = (USHORT)selector;
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
	descriptor->selector = (USHORT)selector;
	descriptor->baseLow = base & 0xffff;
	descriptor->baseHigh = (base >> 16) & 0xffff;
}

void makeIntGateDescriptor(DWORD base, DWORD selector, int dpl, IntTrapGateDescriptor* descriptor) {
	descriptor->present = 1;
	descriptor->system = 0;
	descriptor->type = INTGATE_DESCRIPTOR;
	descriptor->dpl = dpl;
	descriptor->unused = 0;
	descriptor->selector = (USHORT)selector;
	descriptor->baseLow = base & 0xffff;

	descriptor->baseHigh = (base >> 16) & 0xffff;
}

void makeTrapGateDescriptor(DWORD base, DWORD selector, int dpl, IntTrapGateDescriptor* descriptor) {
	descriptor->present = 1;
	descriptor->system = 0;
	descriptor->type = TRAPGATE_DESCRIPTOR;
	descriptor->unused = 0;
	descriptor->dpl = dpl;
	descriptor->selector = (USHORT)selector;
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

	tss->intMap[31] = 0xff;
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

	initKernelTss((TSS*)GetCurrentTaskTssBase(),TASKS_STACK0_BASE + TASK_STACK0_SIZE - STACK_TOP_DUMMY,KERNEL_TASK_STACK_TOP, 0, PDE_ENTRY_VALUE, 0);
	makeTssDescriptor((unsigned long)GetCurrentTaskTssBase(), 3, sizeof(TSS) - 1, (TssDescriptor*)(GDT_BASE + kTssTaskSelector));

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

	makeTrapGateDescriptor((DWORD)HypervisorInjectException, KERNEL_MODE_CODE, 3, descriptor + 28);
	makeTrapGateDescriptor((DWORD)VMMCommException, KERNEL_MODE_CODE, 3, descriptor + 29);
	makeTrapGateDescriptor((DWORD)SecurityException, KERNEL_MODE_CODE, 3, descriptor + 30);

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

	makeTrapGateDescriptor((DWORD)servicesProc, KERNEL_MODE_CODE, 3, descriptor + 0x80);

	makeTrapGateDescriptor((DWORD)IPIIntHandler, KERNEL_MODE_CODE, 3, descriptor + 0x81);

	makeTrapGateDescriptor((DWORD)vm86IntProc, KERNEL_MODE_CODE, 3, descriptor + 0xfe);
	
	makeTaskGateDescriptor((DWORD)kTssV86Selector, 3, (TaskGateDescriptor*)(descriptor + 0xff));

	makeTrapGateDescriptor((DWORD)ApicSpuriousHandler, KERNEL_MODE_CODE, 3, descriptor + 0xff);

	
#ifdef APIC_ENABLE
	
#endif

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

void InitIdt64() {

	IntTrapGate64Descriptor* descriptor = (IntTrapGate64Descriptor*)IDT64_BASE_ADDR;
	for (int i = 0; i < 256; i++)
	{
		makeTrapGate64Descriptor((DWORD)AnonymousException, KERNEL_MODE_CODE, 3, descriptor + i);
	}

	makeTrapGate64Descriptor((DWORD)DivideError, KERNEL_MODE_CODE, 3, descriptor + 0);
	makeTrapGate64Descriptor((DWORD)DebugTrap, KERNEL_MODE_CODE, 3, descriptor + 1);

	makeTrapGate64Descriptor((DWORD)NmiInterrupt, KERNEL_MODE_CODE, 3, descriptor + 2);

	makeTrapGate64Descriptor((DWORD)BreakPointTrap, KERNEL_MODE_CODE, 3, descriptor + 3);

	makeTrapGate64Descriptor((DWORD)OverflowException, KERNEL_MODE_CODE, 3, descriptor + 4);
	makeTrapGate64Descriptor((DWORD)BoundRangeExceed, KERNEL_MODE_CODE, 3, descriptor + 5);
	makeTrapGate64Descriptor((DWORD)UndefinedOpcode, KERNEL_MODE_CODE, 3, descriptor + 6);
	makeTrapGate64Descriptor((DWORD)DeviceUnavailable, KERNEL_MODE_CODE, 3, descriptor + 7);
	makeTrapGate64Descriptor((DWORD)DoubleFault, KERNEL_MODE_CODE, 3, descriptor + 8);

	makeTrapGate64Descriptor((DWORD)CoprocSegOverrun, KERNEL_MODE_CODE, 3, (descriptor + 9));

	//makeTrapGate64Descriptor((DWORD)kTssExceptSelector, 3, (descriptor + 10));

	makeTrapGate64Descriptor((DWORD)SegmentUnpresent, KERNEL_MODE_CODE, 3, descriptor + 11);
	makeTrapGate64Descriptor((DWORD)StackSegFault, KERNEL_MODE_CODE, 3, descriptor + 12);
	makeTrapGate64Descriptor((DWORD)GeneralProtection, KERNEL_MODE_CODE, 3, descriptor + 13);
	makeTrapGate64Descriptor((DWORD)PageFault, KERNEL_MODE_CODE, 3, descriptor + 14);
	makeTrapGate64Descriptor((DWORD)AnonymousException, KERNEL_MODE_CODE, 3, descriptor + 15);
	makeTrapGate64Descriptor((DWORD)FloatPointError, KERNEL_MODE_CODE, 3, descriptor + 16);
	makeTrapGate64Descriptor((DWORD)AlignmentCheck, KERNEL_MODE_CODE, 3, descriptor + 17);
	makeTrapGate64Descriptor((DWORD)MachineCheck, KERNEL_MODE_CODE, 3, descriptor + 18);
	makeTrapGate64Descriptor((DWORD)SIMDException, KERNEL_MODE_CODE, 3, descriptor + 19);
	makeTrapGate64Descriptor((DWORD)VirtualizationException, KERNEL_MODE_CODE, 3, descriptor + 20);
	makeTrapGate64Descriptor((DWORD)CtrlProtectException, KERNEL_MODE_CODE, 3, descriptor + 21);

	makeTrapGate64Descriptor((DWORD)HypervisorInjectException, KERNEL_MODE_CODE, 3, descriptor + 28);
	makeTrapGate64Descriptor((DWORD)VMMCommException, KERNEL_MODE_CODE, 3, descriptor + 29);
	makeTrapGate64Descriptor((DWORD)SecurityException, KERNEL_MODE_CODE, 3, descriptor + 30);


	makeIntGate64Descriptor((DWORD)TimerInterrupt, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_MASTER + 0);

	makeIntGate64Descriptor((DWORD)KeyboardIntProc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_MASTER + 1);
	makeIntGate64Descriptor((DWORD)SlaveIntProc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_MASTER + 2);
	makeIntGate64Descriptor((DWORD)__kCom2Proc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_MASTER + 3);
	makeIntGate64Descriptor((DWORD)__kCom1Proc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_MASTER + 4);
	makeIntGate64Descriptor((DWORD)Parallel2IntProc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_MASTER + 5);
	//makeIntGate64Descriptor((DWORD)SoundInterruptProc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_MASTER + 5);
	makeIntGate64Descriptor((DWORD)Parallel1IntProc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_MASTER + 7);
	makeIntGate64Descriptor((DWORD)FloppyIntProc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_MASTER + 6);

	makeIntGate64Descriptor((DWORD)CmosInterrupt, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_SLAVE + 0);
	makeIntGate64Descriptor((DWORD)Slave1IntProc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_SLAVE + 1);
	makeIntGate64Descriptor((DWORD)NetcardIntProc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_SLAVE + 2);
	makeIntGate64Descriptor((DWORD)USBIntProc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_SLAVE + 3);
	makeIntGate64Descriptor((DWORD)MouseIntProc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_SLAVE + 4);
	makeIntGate64Descriptor((DWORD)CoprocessorIntProc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_SLAVE + 5);
	makeIntGate64Descriptor((DWORD)IDEMasterIntProc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_SLAVE + 6);
	makeIntGate64Descriptor((DWORD)IDESlaveIntProc, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_SLAVE + 7);

	//makeIntGate64Descriptor((DWORD)IPIIntHandler, KERNEL_MODE_CODE, 3, descriptor + INTR_8259_SLAVE + 8);

	makeTrapGate64Descriptor((DWORD)servicesProc, KERNEL_MODE_CODE, 3, descriptor + 0x80);

	//makeTrapGateDescriptor((DWORD)vm86IntProc, KERNEL_MODE_CODE, 3, descriptor + 0xfe);

	//makeTaskGate64Descriptor((DWORD)kTssV86Selector, 3, (TaskGateDescriptor*)(descriptor + 0xff));
#ifdef APIC_ENABLE
	makeTrapGate64Descriptor((DWORD)ApicSpuriousHandler, KERNEL_MODE_CODE, 3, descriptor + 0xff);
#endif

	DescriptTableReg idtbase;
	idtbase.size = 256 * sizeof(SegDescriptor) - 1;
	idtbase.addr = IDT_BASE;
	char szout[1024];
	__printf(szout, (char*)"idt base:%x,size:%x\r\n", idtbase.addr, idtbase.size);
	__asm {
		//不要使用 lidt lpidt,why?
		lidt idtbase
	}
}


int InitGdt64() {
	QWORD* gdt = (QWORD*)GDT64_BASE_ADDR;
	gdt[0] = 0;
	gdt[1] = 0x00209a0000000000;
	gdt[2] = 0x0020920000000000;
	gdt[3] = 0x0020fa0000000000;
	gdt[4] = 0x0020f20000000000;
	gdt[5] = 0x00cffa0000000000;
	gdt[6] = 0x00cff20000000000;
	gdt[7] = 0x0000000000000000;
	short tss_offset = 8 * sizeof (SegDescriptor);

	initTss64((TSS64_DATA*)TSS64_BASE_ADDRESS, TSS64_STACK0_BASE+sizeof(TASK_STACK0_SIZE) - STACK_TOP_DUMMY);
	makeTss64Descriptor(TSS64_BASE_ADDRESS, 3, sizeof(TSS64_DATA)-1,(Tss64Descriptor*)(GDT64_BASE_ADDR + tss_offset));

	return tss_offset + sizeof(Tss64Descriptor);
}

int Is64Supported() {
	int v = 0;
	__asm {
		mov eax, 0x80000000
		cpuid
		cmp eax, 0x80000001
		jb __no_long_mode

		mov eax, 0x80000001
		cpuid
		test edx, 1 << 29; 测试LM位
		jz __no_long_mode
		mov[v], 1
		__no_long_mode:
	}
	return v;
}


void EnablePage64() {
	__asm {
		mov eax, PDE64_ENTRY_VALUE
		mov cr3, eax

		mov eax, cr0
		or eax, 0x80000000
		mov cr0, eax
	}
}

void SetLongMode() {
	__asm {
		mov ecx, 0xC0000080; EFER MSR
		rdmsr
		or eax, 1 << 8; 设置LME位
		wrmsr
	}
}

void InitPAE() {
	__asm {
		__emit 0x0f
		__emit 0x20
		__emit 0xe0
		//mov eax, cr4

		//mov eax, cr4
		or eax, 1 << 5; 设置PAE位
		//mov cr4, eax

		//mov cr4, eax
		__emit 0x0f
		__emit 0x22
		__emit 0xe0
	}
}

void DisablePAE() {
	__asm {
		__emit 0x0f
		__emit 0x20
		__emit 0xe0
		//mov eax, cr4
		//mov eax, cr4
		and eax, 0xffffffdf; 清除PAE位
		//mov cr4, eax
		//mov cr4, eax
		__emit 0x0f
		__emit 0x22
		__emit 0xe0
	}
}

void EnterLongMode() {
	int ret = Is64Supported();
	if (ret) {
		char* databuf = (char*)__kMalloc(0x100000);
		ret = readFile(LIUNUX_BASE_PATH "liunuxos64.dll", &databuf);
		char* realbuf = (char*)MemLoadDll64((char*)databuf, (char*)KERNEL64_DLL_BASE);
		typedef int (*ptrfunction)();
		ptrfunction kernel64Entry = (ptrfunction)getAddrFromName64(realbuf, "__kKernelEntry64");
		char szout[1024];
		__printf(szout, "__kKernelEntry64:%x\r\n", kernel64Entry);

		__asm {
			cli

			mov eax, cr0
			and eax, 0x7fffffff
			mov cr0, eax
		}

		int gdtlen = InitGdt64();

		//InitIdt64();

		InitPage64((QWORD*) PDE64_ENTRY_VALUE);

		InitPAE();

		unsigned char g_jmpstub[16];
		g_jmpstub[0] = 0xea;
		g_jmpstub[5] = KERNEL_MODE_CODE;
		g_jmpstub[6] = 0;	
		* (DWORD*)(g_jmpstub + 1) =(DWORD) kernel64Entry;

		DWORD kernel64Entry32 = (DWORD)kernel64Entry;
		//EnablePage64();

		//SetLongMode();

		DescriptTableReg gdtbase;
		gdtbase.size = gdtlen - 1;
		gdtbase.addr = GDT64_BASE_ADDR;

		short tr_offset = gdtlen - sizeof(Tss64Descriptor);

		unsigned long oldEbp = 0;
		unsigned long oldEsp = 0;

		__asm {

			lea eax, __bit64EntryOffset
			mov edx, kernel64Entry32
			mov  ds : [eax] , edx

			mov eax, PDE64_ENTRY_VALUE
			mov cr3, eax

			mov ecx, 0xC0000080; EFER MSR
			rdmsr
			or eax, 1 << 8; 设置LME位
			wrmsr

			mov eax, cr0
			or eax, 0x80000000
			mov cr0, eax

			lgdt gdtbase
			mov ax, tr_offset
			ltr ax

			pushad

			lea ecx, __win64_leave
			mov edx,esp

			_emit 0xea
		__bit64EntryOffset:
			_emit 0
			_emit 0
			_emit 0
			_emit 0
			_emit 8
			_emit 0	

			push dword ptr KERNEL_MODE_CODE
			push dword ptr kernel64Entry32
			retf

			lea eax, g_jmpstub
			jmp eax

		__win64_leave:
			mov eax, cr0
			and eax, 7fffffffh
			mov cr0, eax

			mov ecx, 0C0000080h
			rdmsr
			and eax, 0fffffeffh
			wrmsr

			__emit 0x0f
			__emit 0x20
			__emit 0xe0
			//mov eax, cr4
			//mov eax, cr4
			and eax, 0xffffffdf
			//mov cr4, eax
			//mov cr4, eax
			__emit 0x0f
			__emit 0x22
			__emit 0xe0
			
			mov ax, KERNEL_MODE_CODE
			mov ss,ax
			mov ds,ax
			mov es,ax
			mov fs,ax
			mov gs,ax

			mov eax, PDE_ENTRY_VALUE
			mov cr3, eax

			mov eax, cr0
			or eax, 0x80000000
			mov cr0, eax

			mov ax, kTssTaskSelector
			ltr ax

			popad
		}
	}
}





void makeTss64Descriptor(QWORD base, int dpl, int size, Tss64Descriptor* descriptor) {
	descriptor->tss32.present = 1;
	descriptor->tss32.dpl = dpl;
	descriptor->tss32.system = 0;
	descriptor->tss32.type = TSS_DESCRIPTOR;

	descriptor->tss32.avl = 0;
	descriptor->tss32.unused = 0;
	descriptor->tss32.db = 0;
	descriptor->tss32.granularity = 0;
	descriptor->tss32.baseLow = base & 0xffff;
	descriptor->tss32.baseMid = (base >> 16) & 0xff;
	descriptor->tss32.baseHigh = (base >> 24) & 0xff;
	descriptor->tss32.len = size & 0xffff;
	descriptor->tss32.lenHigh = (size >> 16) & 0xf;

	descriptor->baseHigh32 = base >> 32;
}


void makeLDT64Descriptor(QWORD base, int dpl, int size, Tss64Descriptor* descriptor) {
	descriptor->tss32.present = 1;
	descriptor->tss32.dpl = dpl;
	descriptor->tss32.system = 0;
	descriptor->tss32.type = LDT_DESCRIPTOR;

	descriptor->tss32.avl = 0;
	descriptor->tss32.unused = 0;
	descriptor->tss32.db = 0;
	descriptor->tss32.granularity = 0;
	descriptor->tss32.baseLow = base & 0xffff;
	descriptor->tss32.baseMid = (base >> 16) & 0xff;
	descriptor->tss32.baseHigh = (base >> 24) & 0xff;
	descriptor->tss32.len = size & 0xffff;
	descriptor->tss32.lenHigh = (size >> 16) & 0xf;

	descriptor->baseHigh32 = base >> 32;
}





void makeCallGate64Descriptor(QWORD base, DWORD selector, int dpl, int paramcnt, CallGate64Descriptor* descriptor) {
	descriptor->cg32.present = 1;
	descriptor->cg32.paramCnt = paramcnt;
	descriptor->cg32.system = 0;
	descriptor->cg32.type = CALLGATE_DESCRIPTOR;
	descriptor->cg32.dpl = dpl;
	descriptor->cg32.selector = (USHORT)selector;
	descriptor->cg32.baseLow = base & 0xffff;
	descriptor->cg32.baseHigh = (base >> 16) & 0xffff;

	descriptor->baseHigh32 = base >> 32;
}

void makeIntGate64Descriptor(QWORD base, DWORD selector, int dpl, IntTrapGate64Descriptor* descriptor) {
	descriptor->itg32.present = 1;
	descriptor->itg32.system = 0;
	descriptor->itg32.type = INTGATE_DESCRIPTOR;
	descriptor->itg32.dpl = dpl;
	descriptor->itg32.unused = 0;
	descriptor->itg32.selector = (USHORT)selector;
	descriptor->itg32.baseLow = base & 0xffff;

	descriptor->itg32.baseHigh = (base >> 16) & 0xffff;

	descriptor->baseHigh32 = base >> 32;
}

void makeTrapGate64Descriptor(QWORD base, DWORD selector, int dpl, IntTrapGate64Descriptor* descriptor) {
	descriptor->itg32.present = 1;
	descriptor->itg32.system = 0;
	descriptor->itg32.type = TRAPGATE_DESCRIPTOR;
	descriptor->itg32.unused = 0;
	descriptor->itg32.dpl = dpl;
	descriptor->itg32.selector = (USHORT)selector;
	descriptor->itg32.baseLow = base & 0xffff;
	descriptor->itg32.baseHigh = (base >> 16) & 0xffff;

	descriptor->baseHigh32 = base >> 32;
}


void initTss64(TSS64_DATA* tss, QWORD rsp) {

	__memset((char*)tss, 0, sizeof(TSS64_DATA));

	tss->iomap= sizeof(TSS64_DATA);
	tss->rsp0 = rsp;
	tss->ioend = 0xff;
}