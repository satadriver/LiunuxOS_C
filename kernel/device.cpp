
#include "def.h"
#include "device.h"
#include "hardware.h"
#include "Utils.h"
#include "keyboard.h"
#include "serialUART.h"

#include "mouse.h"



//tr6,tr7
//https://www.kancloud.cn/wizardforcel/intel-80386-ref-manual/123864


//cr4
//https://www.cnblogs.com/ck1020/p/6115200.html

//硬件使用 VIF 和 VIP 来管理 VM86 模式下的中断：
//VIF 替代 IF 控制虚拟中断的允许 / 禁止。
//VIP 标记挂起的中断，触发操作系统介入。
//VM86 程序执行 STI 指令 → 硬件置 VIP=1（而非直接允许中断）。操作系统检测到 VIP = 1 → 模拟中断处理 → 完成后清 VIP 并可能置 VIF = 1。
void enableVME() {
	__asm {
		//mov eax,cr4
		__emit 0x0f
		__emit 0x20
		__emit 0xe0

		or eax,1

		__emit 0x0f
		__emit 0x22
		__emit 0xe0
		//mov cr4 ,eax
	}
}


//CR4.PVI（Protected-mode Virtual Interrupts） 是 x86 架构中 CR4 寄存器 的一个标志位（位 1），
//用于在 保护模式（Protected Mode） 下为 特权级 3（用户态） 的任务提供 硬件虚拟中断支持。
//它的作用与 CR4.VME（Virtual 8086 Mode Extensions）类似，但针对的是保护模式（而非虚拟 8086 模式）。
//允许在 CPL = 3（用户态） 的任务中，使用 硬件虚拟中断机制（类似 EFLAGS.VIF / VIP 在 VM86 模式的作用）。
//当用户态程序执行 STI（允许中断）或 CLI（禁止中断）时，不会直接修改 EFLAGS.IF（真实的中断允许标志），而是修改 EFLAGS.VIF（虚拟中断标志）。
//如果中断发生时 VIF = 0（虚拟中断禁止），CPU 会设置 EFLAGS.VIP = 1（虚拟中断挂起），并触发 #GP（General Protection Fault）异常，由操作系统处理。
void enablePVI() {
	__asm {
		//mov eax, cr4
		__emit 0x0f
		__emit 0x20
		__emit 0xe0

		or eax, 2

		__emit 0x0f
		__emit 0x22
		__emit 0xe0
		//mov cr4 , eax
	}
}

//TSD=0：所有特权级均可执行 RDTSC。TSD = 1：仅内核态（CPL = 0）可执行。
void enableTSD() {
	__asm {
		//mov eax, cr4
		__emit 0x0f
		__emit 0x20
		__emit 0xe0

		and eax, 0xfffffffb

		__emit 0x0f
		__emit 0x22
		__emit 0xe0
		//mov cr4, eax
	}
}

void enableDE() {
	__asm {
		//mov eax, cr4
		__emit 0x0f
		__emit 0x20
		__emit 0xe0

		or eax, 8

		__emit 0x0f
		__emit 0x22
		__emit 0xe0
		//mov cr4, eax
	}
}

void enableMCE() {
	__asm {
		//mov eax,cr4
		__emit 0x0f
		__emit 0x20
		__emit 0xe0

		or eax, 0x40

		__emit 0x0f
		__emit 0x22
		__emit 0xe0
		//mov cr4,eax
	}
}


void enablePCE() {
	__asm {
		//mov eax, cr4
		__emit 0x0f
		__emit 0x20
		__emit 0xe0

		or eax, 0x100		//pce enable rdpmc

		__emit 0x0f
		__emit 0x22
		__emit 0xe0
		//mov cr4, eax
	}
}
	
void enableVMXE() {
	__asm {
		//mov eax,cr4
		__emit 0x0f
		__emit 0x20
		__emit 0xe0

		or eax,0x2000

		__emit 0x0f
		__emit 0x22
		__emit 0xe0
		//mov cr4 ,eax
	}
}

void __wait8042Full() {
	unsigned char status = 0;
	do
	{
		status = inportb(0x64);
	} while ((status & 1) == 0);
}

void __wait8042Empty() {
	unsigned char status = 0;
	do
	{
		status = inportb(0x64);
	} while (status & 2);
}


void initDevices() {

	init8259();
	init8254();
	init8042();
	initCMOS();
	enableMouse();
	//setMouseRate(200);
	enableSpeaker();
	getKeyboardID();
	initSerial();
}


void initTextModeDevices() {

	init8259();
	init8254();
	init8042();
	initCMOS();
	//enableMouse();
	//setMouseRate(200);
	enableSpeaker();
	getKeyboardID();

	initSerial();
}

/*
To enable the Intellimouse Z-axis extension, you have to set some magic into the sample rate:

set_mouse_rate(200);  // see the example above
set_mouse_rate(100);
set_mouse_rate(80);
mouseid = identify(); // see Get Device ID, 0xF2
*/

void enableMouseZAxis() {
	setMouseSampleRate(200);
	setMouseSampleRate(100);
	setMouseSampleRate(80);
	gMouseID = getMouseID();
}



void insert8042Key(char key) {
	__wait8042Empty();
	outportb(PS2_COMMAND_PORT, 0xd2);

	__wait8042Empty();
	outportb(PS2_DATA_PORT, key);
}


void insert8042Mouse(LPMOUSEINFO mouse) {
	__wait8042Empty();
	outportb(PS2_COMMAND_PORT, 0xd3);

	__wait8042Empty();
	outportb(PS2_DATA_PORT, mouse->status);

	__wait8042Empty();
	outportb(PS2_COMMAND_PORT, 0xd3);

	__wait8042Empty();
	outportb(PS2_DATA_PORT, mouse->x);

	__wait8042Empty();
	outportb(PS2_COMMAND_PORT, 0xd3);

	__wait8042Empty();
	outportb(PS2_DATA_PORT, mouse->y);
}

void setMouseResolution(int res) {

	__wait8042Empty();
	outportb(PS2_COMMAND_PORT, 0xd4);

	__wait8042Empty();
	outportb(PS2_DATA_PORT, 0xe8);

	__wait8042Empty();
	outportb(PS2_COMMAND_PORT, 0xd4);

	__wait8042Empty();
	outportb(PS2_DATA_PORT, res);
}



void setMouseScale() {
	__wait8042Empty();
	outportb(PS2_COMMAND_PORT, 0xd4);

	__wait8042Empty();
	outportb(PS2_DATA_PORT, 0xe6);
}






void setMouseSampleRate(int rate) {

	__wait8042Empty();
	outportb(PS2_COMMAND_PORT, 0xd4);

	__wait8042Empty();
	outportb(PS2_DATA_PORT, 0xf3);

	__wait8042Empty();
	outportb(PS2_COMMAND_PORT, 0xd4);

	__wait8042Empty();
	outportb(PS2_DATA_PORT, rate);

}

//https://www.cnblogs.com/LinKArftc/p/5735627.html
//83ABh
void getKeyboardID() {
	int c = 0;

	__wait8042Empty();
	outportb(PS2_DATA_PORT, 0Xf2);

	__wait8042Full();
	char c1 = inportb(PS2_DATA_PORT);

	__wait8042Full();
	char c2 = inportb(PS2_DATA_PORT);

	__wait8042Full();
	unsigned char high = inportb(PS2_DATA_PORT);

	__wait8042Full();
	unsigned char low = inportb(PS2_DATA_PORT);

	gKeyboardID = (high << 8) | low;

	char szout[256];
	__printf(szout, "keyboardid:%x\r\n", gKeyboardID);
}


int getMouseID() {

	int c = 0;

	__wait8042Empty();
	outportb(PS2_COMMAND_PORT, 0xd4);

	__wait8042Empty();
	outportb(PS2_DATA_PORT, 0xf2);

	int c1 = 0;
	//__wait8042Full();
	c1 = inportb(PS2_DATA_PORT);

	return c1 ;

}


void disableMouse() {
	__wait8042Empty();

	outportb(PS2_COMMAND_PORT, 0xa7);

	__wait8042Empty();

	outportb(PS2_COMMAND_PORT, 0xd4);

	__wait8042Empty();

	outportb(PS2_DATA_PORT, 0xf5);
}

void enableMouse() {
	__wait8042Empty();

	outportb(PS2_COMMAND_PORT, 0xa8);

	__wait8042Empty();

	outportb(PS2_COMMAND_PORT, 0xd4);

	__wait8042Empty();

	outportb(PS2_DATA_PORT, 0xf4);
}


/*
61h NMI Status and Control Register
bit3:IOCHK NMI Enable(INE) : When set, IOCHK# NMIs are disabledand cleared.When cleared, IOCHK# NMIs are enabled.
bit2:SERR# NMI Enable(SNE) : When set, SERR# NMIs are disabledand cleared.When cleared, SERR# NMIs are enabled.
bit1:Speaker Data Enable(SDE) : When this bit is a 0, the SPKR output is a 0.
When this bit is a 1, the SPKR output is equivalent to the Counter 2 OUT signal value.
bit 0:Timer Counter 2 Enable(TC2E) : When cleared, counter 2 counting is disabled.When set, counting is enabled.
*/
void enableSpeaker() {

	outportb(0x61, 3);
}

#define TIMER1_DIVIDE_FREQ  1193
#define TIMER2_DIVIDE_FREQ  1193

//d6 d7 select timer, 00 = 40h, 01 = 41h, 02 = 42h
//d4 d5 mode :11 read read / write low byte first, than read / write high byte.00 lock value
//d1 d2 d3 select work mode,mode 2 or mode 3 is repeat periodic circle timer,others is not periodic circle
//d0 bcd or binary, 0 = binary, 1 = bcd
void init8254() {

	unsigned long freq = OSCILLATE_FREQUENCY;
	unsigned long slice = TASK_TIME_SLICE;
	unsigned circle = 1000 / slice;
	unsigned long v = freq / circle;
	if (v >= 0x10000) {
		v = 0;
	}
	char szout[256];
	__printf(szout,"8254 tick %d per second\r\n", v);

	outportb(TIMER_COMMAND_REG, 0X36);	//36 or 34 is periodic
	//outportb(0x40, SYSTEM_TIMER0_FACTOR & 0xff);
	//outportb(0x40, (SYSTEM_TIMER0_FACTOR >> 8)&0xff);

	outportb(0x40, v & 0xff);
	outportb(0x40, (v >> 8) & 0xff);

	outportb(TIMER_COMMAND_REG, 0X76);
	outportb(0x41, TIMER1_DIVIDE_FREQ & 0xff);
	outportb(0x41, (TIMER1_DIVIDE_FREQ >>8)&0xff);

	outportb(TIMER_COMMAND_REG, 0Xb6);
	outportb(0x42, TIMER2_DIVIDE_FREQ & 0xff);
	outportb(0x42, (TIMER2_DIVIDE_FREQ >> 8) & 0xff);
}


void waitInterval(int v) {
	
	unsigned short interval = 1000/ (OSCILLATE_FREQUENCY / TIMER1_DIVIDE_FREQ);
	unsigned short times = (v *1 )/ interval;
	unsigned short mod =( v * 1) % interval;
	if (mod != 0)
	{
		times++;
	}

	if (times == 0) {
		times = 1;
	}

	unsigned short v0 = getTimerCounter(1);
	do {
		unsigned short v1 = getTimerCounter(1);
		if (v1 - v0 < times) {
			continue;
		}
		else {
			break;
		}
	} while (1);

	return;
}

void waitInterval2(int cnt) {
	unsigned short v0 = getTimerCounter(2);
	unsigned short v1 = getTimerCounter(2);
	//while (v0 == v1) 
	{
		v1 = getTimerCounter(2);
	}
	do {
		v1 = getTimerCounter(2);
	} while ((v1 - v0) != cnt && (v0 - v1) != cnt);
	return;
}


void waitInterval1(int cnt) {
	unsigned short v0 = getTimerCounter(1) ;
	unsigned short v1 = getTimerCounter(1);
	//while (v0 == v1) 
	{
		v1 = getTimerCounter(1);
	}
	do {
		v1 = getTimerCounter(1);
	} while ((v1 - v0) != cnt && (v0 - v1) != cnt);
	return;
}

void waitInterval0(unsigned short cnt) {
	unsigned short v0 = getTimer0Counter();
	unsigned short v1 = getTimer0Counter();
	//while (v0 == v1) 
	{
		v1 = getTimer0Counter();
	}

	do {
		v1 = getTimer0Counter();
	} while ( (v1 - v0) != cnt && (v0 - v1) != cnt);
	return;
}

unsigned short getTimer0Counter() {

	__asm {
		mov al, 0x6
		out 43h, al

		//mov al, 0x36
		//out 43h, al

		in al, 40h
		mov ah, al
		in al, 40h
		xchg ah, al
		movzx eax, ax
	}
}

int delay() {
	int v = 1;
	int s = 1;
	for (int i = 0; i < 0x100; i++) {
		v = v * (s++);
	}
	return v;
}

void __delay() {
	for (int i = 0; i < 0x10; i++) {
		__asm {
			nop
		}
	}
}



unsigned short getTimerCounter(int num) {
	int cmd = (num << 6) + 0x6;
	outportb(TIMER_COMMAND_REG, cmd);

	//cmd = (num << 6) + 0x36;
	//outportb(TIMER_COMMAND_REG, cmd);
	unsigned short low = inportb(0x40 + num);
	unsigned short high = inportb(0x40 + num);
	return low + (high << 8);
}


void init8042() {

	__wait8042Empty();
	outportb(PS2_COMMAND_PORT, 0xad);

	__wait8042Empty();
	outportb(PS2_COMMAND_PORT, 0x60);

	__wait8042Empty();
	outportb(PS2_DATA_PORT, 0X47);

	__wait8042Empty();
	outportb(PS2_COMMAND_PORT, 0x20);

	__wait8042Full();
	int c = inportb(PS2_DATA_PORT);

	__wait8042Empty();
	outportb(PS2_COMMAND_PORT, 0xae);

	char szout[256];
	__printf(szout, "8042 init value:%d\r\n", c);

}


//编程RTC时，禁用NMI和其它中断是“极其必要的”。 这是因为如果发生中断，RTC可能会处于 “未定义” (不工作) 状态。
//这通常不是什么大事，但是有两个问题： RTC从不由BIOS初始化，它由电池备份。 因此，即使是冷重启也可能不足以使RTC脱离未定义的状态!
// 
//frequency =  32768 >> (rate-1); 该设置必须是介于1到15之间的值
/*
0001 = 3.90625 ms
0010 = 7.8125 ms
0011 = 122.070 µs
0100 = 244.141 µs
0101 = 488.281 µs
0110 = 976.5625 µs
0111 = 1.953125 ms
1000 = 3.90625 ms
1001 = 7.8125 ms
1010 = 15.625 ms
1011 = 31.25 ms
1100 = 62.5 ms
1101 = 125 ms
1110 = 250 ms
1111= 500 ms
*/
void initCMOS() {

	outportb(CMOS_NUM_PORT, 0X0A|0x80);
	//delay();
	//int v = inportb(CMOS_DATA_PORT);
	//while ( (v & 0x80) == 0) 
	{
	}
	outportb(CMOS_DATA_PORT, 0X2A);

	outportb(CMOS_NUM_PORT, 0X0B | 0x80);
	//delay();
	outportb(CMOS_DATA_PORT, 0X7A);

	outportb(CMOS_NUM_PORT, 0X0D | 0x80);
	outportb(CMOS_DATA_PORT, 0);
}



void init8259() {

	outportb(0x20, 0x11);
	outportb(0xa0, 0x11);
	outportb(0x21, INTR_8259_MASTER);
	outportb(0xa1, INTR_8259_SLAVE);
	outportb(0x21, 4);
	outportb(0xa1, 2);
	outportb(0x21, 0x11);
	outportb(0xa1, 0x1);

	outportb(0x21, 0x40);	//ocw1
	outportb(0xa1, 0xc0);

	//outportb(0x20, 0x20);	//ocw2
	//outportb(0xa0, 0x20);

	//0: level trigger,1: pulse trigger
	outportb(0x4d0, 0);
	outportb(0x4d1, 0);
}




#define PIC1			0x20		/* IO base address for master PIC */
#define PIC2			0xA0		/* IO base address for slave PIC */
#define PIC1_COMMAND	PIC1
#define PIC1_DATA		(PIC1+1)
#define PIC2_COMMAND	PIC2
#define PIC2_DATA		(PIC2+1)

#define PIC_READ_IRR    0x0a    /* OCW3 irq ready next CMD read */
#define PIC_READ_ISR    0x0b    /* OCW3 irq service next CMD read */

/* Helper func */
static uint16_t __pic_get_irq_reg(int ocw3)		//ocw3
{
	/* OCW3 to PIC CMD to get the register values.  PIC2 is chained, and
	 * represents IRQs 8-15.  PIC1 is IRQs 0-7, with 2 being the chain */
	outportb(PIC1_COMMAND, ocw3);
	outportb(PIC2_COMMAND, ocw3);
	return (inportb(PIC2_COMMAND) << 8) | inportb(PIC1_COMMAND);
}

/* Returns the combined value of the cascaded PICs irq request register */
uint16_t pic_get_irr(void)
{
	return __pic_get_irq_reg(PIC_READ_IRR);
}

/* Returns the combined value of the cascaded PICs in-service register */
uint16_t pic_get_isr(void)
{
	return __pic_get_irq_reg(PIC_READ_ISR);
}