
#include "../kernel/core.h"
#include "vectorroutine.h"
#include "mouse.h"
#include "keyboard.h"
#include "debugger.h"
#include "utils.h"
#include "floppy.h"
#include "servicesProc.h"
#include "serialUART.h"

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
