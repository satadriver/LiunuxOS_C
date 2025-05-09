#pragma once

#define PS2_COMMAND_PORT	0x64
#define PS2_DATA_PORT		0x60

#define TIMER_COMMAND_REG	0X43

#define CMOS_NUM_PORT		0X70
#define CMOS_DATA_PORT		0X71

void initDevices();
void init8254();
void initCMOS();
void enableMouse();
void init8042();

void init8259();

void initTextModeDevices();

void enableVME();

void enablePVI();

void enableTSD();

void enableDE();
void enableVMXE();
void enableMCE();
void enablePCE();

void enableSpeaker();
void getKeyboardID();

void __wait8042Empty();

void __wait8042Full();

#define __waitPs2Out __wait8042Full
#define __waitPs2In __wait8042Empty

void setMouseRate(int rate);

int getMouseID();

void disableMouse();

int delay();

void __delay();

unsigned short getTimer0Counter();

unsigned short getTimerCounter(int num);

void waitInterval2(int cnt);

void waitInterval1(int cnt);

void waitInterval0(unsigned short v);


static uint16_t __pic_get_irq_reg(int ocw3);

/* Returns the combined value of the cascaded PICs irq request register */
uint16_t pic_get_irr(void);
/* Returns the combined value of the cascaded PICs in-service register */
uint16_t pic_get_isr(void);
