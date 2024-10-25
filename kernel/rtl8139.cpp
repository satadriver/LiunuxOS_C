
#include "rtl8139.h"
#include "pci.h"



int initRTL8139() {

	DWORD regs[256];
	DWORD dev = 0;
	DWORD vd = 0;
	int ret = getPciDevBasePort(regs, 0x0200, &dev, &vd);


	if (vd == 0x3981ec10) {
		WORD port = regs[0] & 0xfff8;
		unsigned char mac[6];
		for (int i = 0; i < 6; i++) {
			mac[i] = regs[i];
		}


	}

	return 0;
}

