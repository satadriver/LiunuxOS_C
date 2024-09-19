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

unsigned short getTimer0Counter();

unsigned short getTimerCounter(int num);

void waitInterval2(int cnt);

void waitInterval1(int cnt);

void waitInterval0(unsigned short v);
