
#include "pci.h"
#include "video.h"
#include "Utils.h"
#include "hardware.h"





//bit31:valid
//bit 16-23:bus no		device=1 is pci bridge
//bit 11-15:device no
//bit 8-10:function no
//bit 2-6: register no
//bit0-2:0
DWORD makePciAddr(int bus, int dev, int func, int reg) {

	DWORD v = (reg & 0xfc) + ((func&7) << 8) + ((dev&0x1f) << 11) + ((bus&0xff) << 16) + 0x80000000;
	return v;
}


int getNetcard(DWORD* regs, DWORD* dev, DWORD* irq) {
	return getPciDevBasePort(regs, 0x0200, dev, irq);
}

int getSvga(DWORD* regs, DWORD* dev, DWORD* irq) {
	return getPciDevBasePort(regs, 0x0300, dev, irq);
}

int getSoundcard(DWORD* regs, DWORD* dev, DWORD* irq) {
	return getPciDevBasePort(regs, 0x0401, dev, irq);
}

int getSmbus(DWORD* regs, DWORD* dev, DWORD* irq) {
	return getPciDevBasePort(regs, 0x0c05, dev, irq);
}

int getUsb(DWORD* regs, DWORD* dev, DWORD* irq) {
	return getPciDevBasePort(regs, 0x0c03, dev, irq);
}

int getLPC(DWORD* regs, DWORD* dev, DWORD* irq) {
	return getPciDevBasePort(regs, 0x0601, dev, irq);
}


int getNorthBridge(DWORD* regs, DWORD* dev, DWORD* irq) {
	return getPciDevBasePort(regs, 0x0600, dev, irq);
}


int GetPciReg(int bus, int dev, int fun,int reg) {

	DWORD pci = makePciAddr(bus, dev, fun, reg);

	outportd(0xcf8, pci);
	DWORD v = inportd(0xcfc);
	if (v && v != 0xffffffff)
	{
		return v;
	}

	return 0;
}

int SetPciReg(int bus, int dev, int fun, int reg,DWORD v) {

	DWORD pci = makePciAddr(bus, dev, fun, reg);

	outportd(0xcf8, pci);

	outportd(0xcfc,v);

	return 0;
}

int getPciDevBasePort(DWORD* baseregs, WORD devClsVender, DWORD* dev, DWORD* vd) {
	//__asm{cli}
	int cnt = 0;

	DWORD* lpdst = (DWORD*)baseregs;

	for (int bdf = 0x80000008; bdf <= 0x80fff808; bdf += 0x100)			//offset 8,read class type,vender type
	{
		outportd(0xcf8, bdf);
		DWORD v = inportd(0xcfc);
		if (v && v != 0xffffffff)
		{
			v = v >> 16;
			if (v == devClsVender)
			{
				*dev = bdf&0x00ffff00;

				outportd(0xcf8, bdf&0xffffff00);
				*vd = v;

				DWORD v = inportd(0xcfc);
				int baseregidx = (bdf & 0xffffff00) + 0x10;

				for (int i = 0; i < 4; i++)
				{
					outportd(0xcf8, baseregidx);
					v = inportd(0xcfc);
					if (v && v != 0xffffffff) {
						*lpdst = v;
						lpdst++;
						baseregidx += 4;

						cnt++;
					}
					else {
						break;
					}
				}

				baseregidx = (bdf & 0xffffff00) + 0x40;
				outportd(0xcf8, baseregidx);
				v = inportd(0xcfc);
				if (v && v != 0xffffffff) {
					//*irqpin = v;
				}
			}
		}
	}

	//__asm{sti}
	return cnt;
}




int listpci(DWORD* dst) {
	int cnt = 0;

	DWORD* lpdst = (DWORD*)dst;

	//__asm{cli}

	for (int bdf = 0x80000008; bdf <= 0x80fff808; bdf += 0x100)			//offset 8,read class type,vender type
	{
		outportd(0xcf8, bdf);
		DWORD v = inportd(0xcfc);
		if (v && v != 0xffffffff)
		{
			v = v >> 16;

			*lpdst = v;
			lpdst++;
			*lpdst = bdf;
			lpdst++;

			
			//if ( (v == 0x200) ||  (v== 0x0300) || (v== 0x0401) || (v == 0x0c03) || (v == 0x0c05) ) 
			{
				int baseregidx = (bdf & 0xffffff00) + 0x00;	//0x10
				int regcnt = 0;
				DWORD regs[0x40];
				for (int i = 0; i < 8; i++)
				{
					outportd(0xcf8, baseregidx);
					DWORD r = inportd(0xcfc);
					if (r && r != 0xffffffff) {

						baseregidx += 4;
						regs[i] = r;
						regcnt++;
					}
					else {
						break;
					}
				}

				if (regcnt ) {
					char szout[256];
					__printf(szout, "dev:%x,type:%x, regs:%x,%x,%x,%x,%x,%x,%x,%x\n", 
						bdf, v, regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], regs[7]);
				}
			}
			
			cnt++;
		}
	}

	//__asm{sti}
	return cnt;
}








void showAllPciDevs() {
	unsigned long devbuf[2048];
	int cnt = listpci(devbuf);
	if (cnt > 0)
	{
		for (int i = 0; i < cnt; )
		{
			char szout[256];
			__printf(szout, "\npci type:%x,device:%x\n", devbuf[i], devbuf[i + 1]);

			i += 2;
		}
	}
}





int showPciDevs() {

	char szout[256];
	__printf(szout, ( char*)"\n\npci devices:\n");

	int ret = 0;

	DWORD svgaregs[64];
	DWORD svgadev = 0;
	DWORD svgairq = 0;
	ret = getSvga(svgaregs, &svgadev, &svgairq);
	if (ret)
	{
		__printf(szout, "svga regs:%x,%x,%x,%x,dev:%x,irq:%x\n", svgaregs[0], svgaregs[1], svgaregs[2], svgaregs[3], svgadev, svgairq);

	}

	DWORD usbregs[64];
	DWORD usbdev = 0;
	DWORD usbirq = 0;
	ret = getUsb(usbregs, &usbdev, &usbirq);
	if (ret)
	{
		__printf(szout, "usb regs:%x,%x,%x,%x,dev:%x,irq:%x\n", usbregs[0], usbregs[1], usbregs[2], usbregs[3],usbdev, usbirq);

	}

	DWORD netregs[64];
	DWORD netdev = 0;
	DWORD netirq = 0;
	ret = getNetcard(netregs, &netdev, &netirq);
	if (ret)
	{
		__printf(szout, "netcard regs:%x,%x,%x,%x,dev:%x,irq:%x\n", netregs[0], netregs[1], netregs[2], netregs[3],netdev, netirq);

	}

	DWORD smbusregs[64];
	DWORD smbusdev = 0;
	DWORD smbusirq = 0;
	ret = getSmbus(smbusregs, &smbusdev, &smbusirq);
	if (ret)
	{
		__printf(szout, "smbus regs:%x,%x,%x,%x,dev:%x,irq:%x\n", smbusregs[0], smbusregs[1], smbusregs[2], smbusregs[3], smbusdev, smbusirq);
	}

	DWORD soundregs[64];
	DWORD sounddev = 0;
	DWORD soundirq = 0;
	ret = getSoundcard(soundregs, &sounddev, &soundirq);
	if (ret)
	{
		__printf(szout, "sound card regs:%x,%x,%x,%x,dev:%x,irq:%x\n", soundregs[0], soundregs[1], soundregs[2], soundregs[3], sounddev, soundirq);
	}
	return 0;
}