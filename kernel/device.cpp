
#include "def.h"
#include "device.h"
#include "hardware.h"
#include "Utils.h"
#include "keyboard.h"
#include "serialUART.h"

#define PS2_COMMAND_PORT	0x64
#define PS2_DATA_PORT		0x60

#define TIMER_COMMAND_REG	0X43

#define CMOS_NUM_PORT		0X70
#define CMOS_DATA_PORT		0X71



//tr6,tr7
//https://www.kancloud.cn/wizardforcel/intel-80386-ref-manual/123864


//cr4
//https://www.cnblogs.com/ck1020/p/6115200.html

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
	setMouseRate(200);
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


void setMouseRate(int rate) {
	__wait8042Empty();

	outportb(PS2_COMMAND_PORT, 0xf3);

	__wait8042Empty();
	outportb(PS2_DATA_PORT, rate);
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
//d4 d5 mode :11 read read / write low byte first, than read / write high byte
//d1 d2 d3 select work mode
//d0 bcd or binary, 0 = binary, 1 = bcd
void init8254() {

	outportb(TIMER_COMMAND_REG, 0X36);
	outportb(0x40, SYSTEM_TIMER0_FACTOR & 0xff);
	outportb(0x40, (SYSTEM_TIMER0_FACTOR >> 8)&0xff);

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
		mov al, 0x36
		out 43h, al
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
	for (int i = 0; i < 0x1000; i++) {
		v = v * (s++);
	}
	return v;
}

unsigned short getTimerCounter(int num) {

	int cmd = (num << 6) + 0x36;
	outportb(TIMER_COMMAND_REG, cmd);
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

	outportb(PS2_COMMAND_PORT, 0xae);

}

//https://www.cnblogs.com/LinKArftc/p/5735627.html
//83ABh
void getKeyboardID() {

	__wait8042Empty();
	outportb(PS2_DATA_PORT, 0Xf2);

	//__wait8042Full();
	//unsigned char ack = inportw(PS2_DATA_PORT);

	//__wait8042Empty();
	//outportb(PS2_DATA_PORT, 0x20);

	__wait8042Full();
	unsigned char high = inportw(PS2_DATA_PORT);

	__wait8042Full();
	unsigned char low = inportw(PS2_DATA_PORT);

	gKeyboardID = (high << 8) | low;

	char szout[1024];
	__printf(szout, "keyboardid:%x\r\n", gKeyboardID);
}


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

	outportb(CMOS_NUM_PORT, 0X0A);
	outportb(CMOS_DATA_PORT, 0XAA);

	outportb(CMOS_NUM_PORT, 0X0B);
	outportb(CMOS_DATA_PORT, 0X7A);

	outportb(CMOS_NUM_PORT, 0X0D);
	outportb(CMOS_DATA_PORT, 0);
}



void init8259() {

	outportb(0x20, 0x11);
	outportb(0xa0, 0x11);
	outportb(0x21, INTR_8259_MASTER);
	outportb(0xa1, INTR_8259_SLAVE);
	outportb(0x21, 4);
	outportb(0xa1, 2);
	outportb(0x21, 0x1);
	outportb(0xa1, 0x1);

	outportb(0x20, 0x00);
	outportb(0xa0, 0xc0);

	//0: level trigger,1: pulse trigger
	outportb(0x4d0, 0);
	outportb(0x4d1, 0);
}


