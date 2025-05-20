#include "debugger.h"
#include "Utils.h"
#include "video.h"
#include "task.h"
#include "device.h"
#include "core.h"
#include "VM86.h"
#include "servicesProc.h"

//VME bit0
//虚拟8086模式扩展（中的位0）置1时则在虚拟8086模式下，启用中断和异常处理扩展。置0时禁用扩展功能。
//虚拟模式扩展的应用是通过减少虚拟8086监控程序对8086程序执行过程中出现的中断和异常的处理，并且重定向中断和异常到8086程序的处理程序，
//从而改进虚拟8086模式下应用程序的性能。对于虚拟中断标志（VIF vip）它也提供了硬件支持来改进在多任务及多处理器环境下执行8086程序的可靠性

// Protected - Mode Virtual Interrupts(PVI) Bit.Bit1.
//Setting PVI to 1 enables support for protected -mode virtual interrupts.Clearing PVI to 0 disablesthis support.
//When PVI = 1, hardware support of two bits in the rFLAGS register, VIF and VIP, isenabled.
// Only the STI and CLI instructions are affected by enabling PVI.Unlike the case when CR0.VME = 1,
//the interrupt - redirection bitmap in the TSS cannot be used for selective INTn interception.
// PVI enhancements are also supported in long mode.See“Virtual Interrupts” on page 251 for more
// information on using PVI.

//tsd stamp disable bit
//de debugging extensions bit 

// PSE bit4
// 页尺寸扩展(中的位4)置1时页大小为4M字节，置0时页大小为4K字节

// PAE bit5
// 物理地址扩展(中的位5)置1时启用分页机制来引用36位物理地址；置0时只可引用32位地址。

// MCE bit6
// 启用机器检测(中的位6)置1时启用机器检测(machine - check)异常，置0时禁用机器检测异常

// PGE bit7
// 启用全局页(中的位7)(在P6系列处理器中引入)置1时启用全局页，置0时禁用全局页。
// 全局页这一特征能够使那些经常被使用或共享的页对所有的用户标志为全局的(通过页目录或者页表项中的第8位 - 全局标志来实现)。
// 在任务切换或者往CR3寄存器写时，全局页并不从TLB中刷新。当启用全局页这一特征时，在设置PGE标志之前，
// 必须先启用分页机制(通过设置CR0中的PG标志)。如果将这个顺序颠倒了，可能会影响程序的正确性以及处理器的性能会受损

// PCE bit8
// 启用性能监测计数器(中的位8)置1时，允许RDPMC指令执行，不论程序运行哪个特权级别。置0时RDPMC指令只能运行在0级特权上。

//bit 9
// OSFXSR 操作系统对FXSAVE和FXRSTOR指令的支持(中的位9)置1时，这一标志具有下列
// 功能：(1)表明操作系统支持FXSAVE和FXRSTOR指令
// (2)启用FXSAVE和FXRSTOR指令来保存和恢复XMM和MXCSR寄存器连同x87 FPU和MMX寄存器的内容
// (3)允许处理器执行除了PAUSE、PREFETCHh、SFENCE、LFENCE，MFENCE、MOVNTI和CLFLUSH指令之外的任何SSE和SSE2指令。
// 如果这一标志置0，则FXSAVE和FXRSTOR指令保存和恢复x87 FPU和MMX寄存器的内容，但可能不保存和恢复XMM和MXCSR寄存器的内容。
// 另外，如果这一标志置0，当处理器企图执行除了PAUSE、PREFETCHh、SFENCE、LFENCE、MFENCE、 MOVNTI和CLFLUSH指令之外的任何SSE和SSE2指令时，
// 都将会产生一个非法操作码异常(#UD)，操作系统必须正确地设置这一标志。
// 注意：
// CPUID特征标志FXSR、SSE和SSE2(位24、25、26)分别表示在特定的IA - 32处理器上，是否具有FXSAVE / FXRESTOR指令，SSE扩展以及SSE2扩展。
// OSFXSR位则为操作系统启用这些特征提供了途径以及并指明了操作系统是否支持这些特征。

// OSXMMEXCPT bit10
// 操作系统支持未屏蔽的SIMD浮点异常(中的位10)，表明操作系统通过异常处理程序支持非屏蔽的SIMD浮点异常的处理，
// 该异常处理程序在SIMD浮点异常产生时被调用。
// 操作系统必须正确的设置这一标志，如果这一标志没有设置，当处理器检测到非屏蔽SIMD浮点异常时，将会产生一个非法操作码异常(#UD)

//bit10 OSXMMEXCPT 操作系统支持未屏蔽的SIMD浮点异常

// bit 18 XSAVE and Extended States(OSXSAVE) Bit.Bit18.If this bit is set to 1 then the operating system
// supports the XGETBV, XSETBV, XSAVE and XRSTOR instructions.The processor will also be able
// to execute XGETBV and XSETBV instructions in order toread and write XCR0.Also, if set, the
// XSAVE and XRSTOR instructions can save and restore thex87 FPU state(including MMX registers),
// the SSE state(YMM / XMM registers and MXCSR), alongwith other processor extended states
// enabled in XCR0.


// 从启动到进入长模式
// 设置实模式下的栈，注意启动扇区被bios读取到0x7c00位置
// 利用int13, ah = 02h读取内核到指定位置，注意设备chs的限制
// 关闭中断，开启a20，设置32位gdt
// cr0.pe[0] = 1，进入保护模式
// 长跳转至32位启动代码，16位实模式结束
// 设置32位段，设置保护模式下的栈
// 准备4kbx(2 - 4)大小的空间，存放页表，分别对应1gb、2mb、4kb的页面大小，存放pml4e、pdpe、pde和pte。注意2mb下pde是页表，而1gb下pdpe就是页表了。
// 所有的页表项和目录项都是64位，共512项，正好4kb。其内容就是64位物理地址，而低n位被挪作他用。
// 禁用cr0.pg[31]、cr0.em[2]，启用cr0.mp[1]，禁用分页，并启用sse
// 启用.osfxsr[9]、.osxmmexcpt[10]、.pae[5]、.pge[7]，启用sse，pae和pge
// 将pml4装入cr3
// 启用efer.lme[8]开启长模式
// 启用cr0.pg[31]开启分页，进入64位兼容模式
// 设置64位gdt，进入64位长模式
/*长跳转至64位启动代码，保护模式结束*/

//TF(bit 8) [Trap flag]   将该位设置为1以允许单步调试模式，清零则禁用该模式




void initDebugger() {
	enableDE();

	__asm {
		//mov eax,cr4
		__emit 0x0f
		__emit 0x20
		__emit 0xe0

		//or eax, 8		//DE

		//mov cr4,eax
		__emit 0x0f
		__emit 0x22
		__emit 0xe0

		mov eax,0
		mov dr7,eax
		mov dr6,eax
	}
}



void __declspec(naked) BreakPointTrap(LIGHT_ENVIRONMENT* stack) {
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
		mov ss,ax
	}

	{
		__kBreakPoint(stack);
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

void __declspec(naked) DebugTrap(LIGHT_ENVIRONMENT* stack) {
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
		mov ss,ax
	}

	{
		__kDebugger(stack);
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

//Software-generated exceptions are not influenced by the IR bit map. INT1,[1] INT3, INTO, 
//and BOUND[2] are not subject to the IR bit map. Instead, these opcodes will always invoke the protected mode exception handler.[3]

void __kBreakPoint(LIGHT_ENVIRONMENT* stack) {

	char szout[1024];

#ifdef VM86_PROCESS_TASK

#else
	if (stack->eflags & 0x20000)
	{
		__kVm86IntProc();
		return;
	}
#endif

	int len = 0;

	unsigned char code = *(unsigned char*)stack->eip;
	if (code == 0xcc) {
		stack->eip++;
		__printf(szout, (char*)"int3(0xcc) is exception\r\n");
	}
	else {
		//__printf(szout, "int3(0xcc) is trap\r\n");
	}

	DWORD eflags = 0;
	__asm {
		//cpu clear IF when enter interruptions
		pushfd
		pop ss:[eflags]
	}

	if (stack->eflags & 0x20000)
	{
		len = __printf(szout,
			(char*)"BreakPoint eax:%x,ecx:%x,edx:%x,ebx:%x,esp0:%x,ebp:%x,esi:%x,edi:%x,eip:%x,vm86 cs:%x,current eflags:%x,stack eflags:%x,vm86 esp:%x,vm86 ss:%x,vm86 ds:%x,vm86 es:%x,vm86 fs:%x,vm86 gs:%x\n",
			stack->eax, stack->ecx, stack->edx, stack->ebx, stack->esp, stack->ebp, stack->esi, stack->edi, 
			stack->eip, stack->cs, eflags, stack->eflags, stack->esp3, stack->ss3,stack->ds_v86, stack->es_v86, stack->fs_v86, stack->gs_v86);
	}
	else if (stack->cs & 3)
	{
		len = __printf(szout,
			(char*)"BreakPoint eax:%x,ecx:%x,edx:%x,ebx:%x,esp:%x,ebp:%x,esi:%x,edi:%x,eip:%x,cs:%x,current eflags:%x,stack eflags:%x,esp3:%x,ss3:%x,ds:%x,es:%x,fs:%x,gs:%x,ss:%x\n",
			stack->eax, stack->ecx, stack->edx, stack->ebx, stack->esp, stack->ebp, stack->esi, stack->edi, 
			stack->eip, stack->cs, eflags, stack->eflags, stack->esp3, stack->ss3,
			stack->ds, stack->es, stack->fs, stack->gs,stack->ss);
	}
	else {
		len = __printf(szout,
			(char*)"BreakPoint eax:%x,ecx:%x,edx:%x,ebx:%x,esp:%x,ebp:%x,esi:%x,edi:%x,eip:%x,cs:%x,current eflags:%x,stack eflags:%x,ds:%x,es:%x,fs:%x,gs:%x,ss:%x\n",
			stack->eax, stack->ecx, stack->edx, stack->ebx, stack->esp, stack->ebp, stack->esi, stack->edi,stack->eip, stack->cs, eflags, stack->eflags,
			stack->ds, stack->es, stack->fs, stack->gs, stack->ss);
	}

	return;
}

//DR0 - DR3
//Contain linear addresses of up to 4 breakpoints.If paging is enabled, they are translated to physical addresses.

// DR6 It permits the debugger to determine which debug conditions have occurred.
//Bits 0 through 3 indicates, when set, that it's associated breakpoint condition was met when a debug exception was generated.
//Bit 13 indicates that the next instruction in the instruction stream accesses one of the debug registers.
//Bit 14 indicates(when set) that the debug exception was triggered by the single - step execution mode(enabled with TF bit in EFLAGS).
//Bit 15 indicates(when set) that the debug instruction resulted from a task switch where T flag in the TSS of target task was set.
//Bit 16 indicates(when clear) that the debug exception or breakpoint exception occured inside an RTM region.


void __kDebugger(LIGHT_ENVIRONMENT* stack) {

	char szout[1024];
	int len = 0;

	DWORD reg_dr6=0;
	__asm {
		mov eax,dr6
		mov [reg_dr6],eax
	}

	if (reg_dr6 & 0x2000)	//BD
	{
		__asm {
			mov eax,dr7
			and eax,0xffffdfff
			mov dr7,eax
		}
		len = __printf(szout, (char*)"BD BreakPoint eip:%x,cs:%x\r\n",stack->eip,stack->cs);

	}
	
	if (reg_dr6 & 0x4000)	//BS
	{
		len = __printf(szout, (char*)"Single Step BreakPoint eip:%x,cs:%x\r\n", stack->eip, stack->cs);

		//__kBreakPoint(stack);

		DWORD eflags = stack->eflags;
		if (eflags & 0x100)
		{
			eflags = eflags & 0xfffffeff;
			stack->eflags = eflags;
		}
	}
	
	if (reg_dr6 & 0x8000)	//BT
	{
		len = __printf(szout, (char*)"TSS Trap BreakPoint eip:%x,cs:%x\r\n", stack->eip, stack->cs);

	}

	DWORD addr = 0;
	DWORD bptype = 0;
	DWORD bplen = 0;
	if (reg_dr6 & 1)
	{
		__asm {
			mov eax, dr0
			mov[addr], eax

			mov eax, dr7
			and eax,0x30000
			mov [bptype],eax

			mov eax,dr7
			and eax,0xc0000
			mov [bplen],eax
		}
	}

	if (reg_dr6 & 2)
	{
		__asm {
			mov eax, dr1
			mov[addr], eax

			mov eax, dr7
			and eax, 0x300000
			mov[bptype], eax

			mov eax, dr7
			and eax, 0xc00000
			mov[bplen], eax
		}
	}

	if (reg_dr6 & 4)
	{
		__asm {
			mov eax, dr2
			mov[addr], eax

			mov eax, dr7
			and eax, 0x3000000
			mov[bptype], eax

			mov eax, dr7
			and eax, 0xc000000
			mov[bplen], eax
		}
	}

	if (reg_dr6 & 8)
	{
		__asm {
			mov eax, dr3
			mov[addr], eax

			mov eax, dr7
			and eax, 0x30000000
			mov[bptype], eax

			mov eax, dr7
			and eax, 0xc0000000
			mov[bplen], eax
		}
	}

	if ( (reg_dr6 & 0x2000) || (reg_dr6 & 0x4000) || (reg_dr6 & 0x8000) )
	{

	}
	
	if((reg_dr6 & 1) || (reg_dr6 & 2) || (reg_dr6 & 4) || (reg_dr6 & 8)){

		len = __printf(szout, (char*)"breakpoint address:%x,type:%d,bplen:%d\r\n",addr, bptype,bplen);

	}

	__asm {
		mov eax, 0
		mov[reg_dr6], eax
	}

	//RF 标志的主要功能是许可从调试异常（调试断点引发的）后面的那个指令开始继续执行。
	//调试软件必须在用IRETD 指令返回到被中断程序之前，将栈中的EFLAGES 映象中的该位置为1，以阻止指令断点产生另外的调试异常。

	//调试中断会在执行指令前触发，在返回并成功执行断点指令之后，处理器会自动清零该位，从而许可继续产生指令断点故障。
	//从中断返回的时候，如果不置RF的话，会再次进入调试中断，RF就是为了防止重复进入调试中断而使用的。

	DWORD eflags = stack->eflags;
	if ( (eflags & 0x10000) == 0)
	{
		//eflags = eflags & 0xfffeffff;
		eflags = eflags | 0x10000;
		stack->eflags = eflags;
	}
}

/*
dr7:

bit 0:L0
bit 1:G0

bit 2:L1
bit 3:G1

bit 4:L2
bit 5:G2

bit 6:L3
bit 7:G3

bit 13:GD

bit 17-16: rw/0
bit 19-18:len/0

bit 21-20: rw/1
bit 23-22:len/1

bit 25-24: rw/2
bit 27-26:len/2

bit 29-28: rw/3
bit 31-30:len/3
*/

//rw:
//00: exe
//01: data write
//10: I/O
//11: read write data

//len:
//00: 1 byte
//01: 2 byte
//10: 8 byte
//11: 4 byte

int dataWBreakPoint(unsigned int * addr,int len) {
	
	DWORD reg_dr7 = 0;
	__asm {
		mov eax,dr7
		mov reg_dr7,eax
	}

	DWORD mask = 0;
	DWORD value = 0;
	if ( (reg_dr7 & 1) == 0)
	{
		mask  = 0xf0000;
		value = 0x10001;
		__asm {
			mov eax, addr
			mov dr0, eax
		}
	}else if ((reg_dr7 & 4) == 0)
	{
		mask  = 0xf00000;
		value = 0x100004;
		__asm {
			mov eax, addr
			mov dr1, eax
		}
	}
	else if ((reg_dr7 & 0x10) == 0)
	{
		mask  = 0xf000000;
		value = 0x1000010;
		__asm {
			mov eax, addr
			mov dr2, eax
		}
	}
	else if ((reg_dr7 & 0x40) == 0)
	{
		mask  = 0xf0000000;
		value = 0x10000040;
		__asm {
			mov eax, addr
			mov dr3, eax
		}
	}

	__asm {
		mov eax, dr7
		mov ecx,mask
		not ecx
		and eax,ecx

		or eax,value
		mov dr7, eax
	}
}

int dataRWBreakPoint(unsigned int * addr, int len) {
	DWORD reg_dr7 = 0;
	__asm {
		mov eax, dr7
		mov reg_dr7, eax
	}

	DWORD mask = 0;
	DWORD value = 0;
	if ((reg_dr7 & 1) == 0)
	{
		mask  = 0xf0000;
		value = 0x30001;
		__asm {
			mov eax, addr
			mov dr0, eax
		}
	}
	else if ((reg_dr7 & 4) == 0)
	{
		mask  = 0xf00000;
		value = 0x300004;
		__asm {
			mov eax, addr
			mov dr1, eax
		}
	}
	else if ((reg_dr7 & 0x10) == 0)
	{
		mask  = 0xf000000;
		value = 0x3000010;
		__asm {
			mov eax, addr
			mov dr2, eax
		}
	}
	else if ((reg_dr7 & 0x40) == 0)
	{
		mask  = 0xf0000000;
		value = 0x30000040;
		__asm {
			mov eax, addr
			mov dr3, eax
		}
	}

	__asm {
		mov eax, dr7
		mov ecx, mask
		not ecx
		and eax, ecx
		or eax, value
		mov dr7, eax
	}
}

int ioBreakPoint(unsigned int * addr, int len) {
	DWORD reg_dr7 = 0;
	__asm {
		mov eax, dr7
		mov reg_dr7, eax
	}

	int mask = 0;
	DWORD value = 0;
	if ((reg_dr7 & 1) == 0)
	{
		mask  = 0xf0000;
		value = 0x20001;
		__asm {
			mov eax, addr
			mov dr0, eax
		}
	}
	else if ((reg_dr7 & 4) == 0)
	{
		mask  = 0xf00000;
		value = 0x200004;
		__asm {
			mov eax, addr
			mov dr1, eax
		}
	}
	else if ((reg_dr7 & 0x10) == 0)
	{
		mask  = 0xf000000;
		value = 0x2000010;
		__asm {
			mov eax, addr
			mov dr2, eax
		}
	}
	else if ((reg_dr7 & 0x40) == 0)
	{
		mask  = 0xf0000000;
		value = 0x20000040;
		__asm {
			mov eax, addr
			mov dr3, eax
		}
	}

	__asm {
		mov eax, dr7
		mov ecx, mask
		not ecx
		and eax, ecx
		or eax, value
		mov dr7, eax
	}
}

int codeBreakPoint(unsigned int * addr,int len) {
	DWORD reg_dr7 = 0;
	__asm {
		mov eax, dr7
		mov reg_dr7, eax
	}

	DWORD mask = 0;
	DWORD value = 0;
	if ((reg_dr7 & 1) == 0)
	{
		mask  = 0xf0000;
		value = 0x00001;
		__asm {
			mov eax, addr
			mov dr0, eax
		}
	}
	else if ((reg_dr7 & 4) == 0)
	{
		mask  = 0xf00000;
		value = 0x000004;
		__asm {
			mov eax, addr
			mov dr1, eax
		}
	}
	else if ((reg_dr7 & 0x10) == 0)
	{
		mask  = 0xf000000;
		value = 0x0000010;
		__asm {
			mov eax, addr
			mov dr2, eax
		}
	}
	else if ((reg_dr7 & 0x40) == 0)
	{
		mask  = 0xf0000000;
		value = 0x00000040;
		__asm {
			mov eax, addr
			mov dr3, eax
		}
	}

	__asm {
		mov eax, dr7
		mov ecx, mask
		not ecx
		and eax, ecx
		or eax, value
		mov dr7, eax
	}
}



void __enableBreakPoint() {

	__asm {
		//_emit 0xcd
		//_emit 03

		int 3	//int 3 is different with int3 in linux gcc
	}
}

int enableSingleStep() {
	//single step
	__asm {
		pushfd
		bts dword ptr ss : [esp], 8
		popfd
	}
}


int disableSingleStep() {
	//single step
	__asm {
		pushfd
		btr dword ptr ss : [esp], 8
		popfd
	}
}


int enableGdDebugger() {

	__asm {
		mov eax, dr7
		bts eax, 13
		mov dr7, eax
	}
}

int disableGdDebugger() {

	__asm {
		mov eax, dr7
		btr eax, 13
		mov dr7, eax
	}
}

//only be effective in ring0
void enableOverflow() {
	__asm {
		pushfd
		or ss:[esp],0x800
		popfd

		_emit 0xce
	}
}


//int 1
void enableDebugger() {
	__asm {
		_emit 0xF1
	}
}


