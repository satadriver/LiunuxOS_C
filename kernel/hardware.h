#pragma once

unsigned int inportbs(unsigned short port);
unsigned int inportb(unsigned short port);
unsigned int inportw(unsigned short port);
unsigned int inportd(unsigned int port);

void outportb(unsigned short port, unsigned char value);
void outportw(unsigned short port, unsigned short value);
void outportd(unsigned short port, unsigned int value);