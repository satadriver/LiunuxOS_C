
#include "parallel.h"
#include "Utils.h"
#include "hardware.h"


//Here the LPT_Base_Address can be:
//LPT1: 0x378 (or occasionally 0x3BC) (IRQ 7)
//LPT2 : 0x278 (IRQ 6)
//LPT3 : 0x3BC (IRQ 5)


//https://wiki.osdev.org/Parallel_port

int ParallelPrinter(char* data,int size) {

    unsigned char lControl;

    for (int i = 0; i < size; i++) {
        // Wait for the printer to be receptive
        while ((inportb(0x379) & 0x80 ) == 0)
        {
            __sleep(0);
        }

        // Now put the data onto the data lines
        outportb(0x378, data[i]);

        // Now pulse the strobe line to tell the printer to read the data
        lControl = inportb(0x37A);
        outportb(0x37A, lControl | 1);
        __sleep(0);
        outportb(0x37A, lControl);

    }
	return 0;
}