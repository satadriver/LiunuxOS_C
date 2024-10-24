#pragma once



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

void enableMCE();
void enablePCE();

void enableSpeaker();
void getKeyboardID();

void __wait8042Empty();

void __wait8042Full();

#define __waitPs2Out __wait8042Full
#define __waitPs2In __wait8042Empty

void setMouseRate(int rate);

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
