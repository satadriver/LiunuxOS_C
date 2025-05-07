
#include "textMode.h"
#include "file.h"
#include "core.h"
#include "device.h"
#include "task.h"
#include "servicesProc.h"
#include "serialUART.h"
#include "coprocessor.h"
#include "Pe.h"
#include "cmosPeriodTimer.h"
#include "debugger.h"
#include "hardware.h"
#include "Utils.h"
#include "VM86.h"
#include "textMode.h"
#include "mouse.h"
#include "keyboard.h"
#include "pci.h"
#include "hardware.h"
#include "kernel.h"
#include "processDOS.h"
#include "cmosPeriodTimer.h"



LPVESAINFORMATION glpVesaInfo;

char* gTxtBuf = 0;

int gTxtOffset = 0;

DWORD gFont = 0;

int runcmd(char * cmd) {
	int res = 0;

	if (__strcmp(cmd, "windows") == 0) {

		char szout[1024];

		res = v86Process(0x4f02, 0, 0, 0x4112, 0, 0, 0, 0, 0x10);

		VESAINFORMATION vesaInfo;
		vesaInfo.BitsPerPixel = 24;
		vesaInfo.YRes = 480;
		vesaInfo.XRes = 640;
		vesaInfo.PhyBasePtr = (char*)0xe0000000;
		vesaInfo.OffScreenMemOffset = 0;
		vesaInfo.BytesPerScanLine = vesaInfo.XRes * (vesaInfo.BitsPerPixel>>3);

		__initVideo(glpVesaInfo, gFontBase);

		enableMouse();
		setMouseRate(200);
		/*
		VesaSimpleInfo vsi[64];
		res = getVideoMode(vsi);
		{
			for (int idx = 0; idx < res; idx++) {
				__sprintf(szout, "mode:%d,width:%d,height:%d,bytesPerPixel:%d,base:%x,offset:%x,size:%x\r\n",
					vsi[idx].mode, vsi[idx].x, vsi[idx].y, vsi[idx].bpp, vsi[idx].base, vsi[idx].offset, vsi[idx].size);
				outputStr(szout, OUTPUT_TEXTMODE_COLOR);
			}
		}*/

#ifdef SINGLE_TASK_TSS
		__createDosCodeProc(gV86VMIEntry,gV86VMISize, "V86VMIEntry");
#else
		__createDosCodeProc(gV86VMIEntry, gV86VMISize,"V86VMIEntry");
#endif

		int imagesize = getSizeOfImage((char*)MAIN_DLL_SOURCE_BASE);
		__kCreateProcessFromAddrFunc(MAIN_DLL_SOURCE_BASE, imagesize, "__kExplorer", 3, 0);

		while (1)
		{
			if (__findProcessFuncName("__kExplorer") == FALSE)
			{
				__kCreateProcess(MAIN_DLL_SOURCE_BASE, imagesize, "main.dll", "__kExplorer", 3, 0);
			}

			__asm {
				hlt
			}
		}

	}
	else if (__strcmp(cmd, "cpuinfo") == 0) {

		char cpu[1024];
		getCpuInfo(cpu);
		__strcat(cpu, "\r\n");
		outputStr(cpu, OUTPUT_TEXTMODE_COLOR);
	}
	else if (__strcmp(cmd, "cputype") == 0) {

		char cpu[1024];
		getCpuType(cpu);
		__strcat(cpu, "\r\n");
		outputStr(cpu, OUTPUT_TEXTMODE_COLOR);
	}
	else if (__strcmp(cmd, "timestamp") == 0) {

	}
	else if (__strcmp(cmd, "date") == 0) {

		DATETIME dt;
		__getDateTime(&dt);
		char datetime[1024];
		__sprintf(datetime, "year:%d,month:%d,day:%d,hour:%d,minute:%d,second:%d\r\n",
			dt.year, dt.month, dt.dayInMonth, dt.hour, dt.minute, dt.second);
		outputStr(datetime, OUTPUT_TEXTMODE_COLOR);
	}
	
	return 0;
}

int outputStr(char* str,char color) {
	int size = __strlen(str);
	for (int i = 0; i < size; i++) {
		outputChar(str[i], color);
	}
	return 0;
}


int outputChar(char c,char color) {

	if (c == 0x0a ) {
		gTxtOffset = (gTxtOffset / LINE_SIZE) * LINE_SIZE + LINE_SIZE;
		
	}
	else if (c == 0x0d) {
		//int mod = gTxtOffset % LINE_SIZE;
		//if (mod) {
		//	gTxtOffset -= mod;
		//}
		//gTxtOffset += LINE_SIZE;

		gTxtOffset = (gTxtOffset / LINE_SIZE) * LINE_SIZE ;			
	}
	else if (c == 0x08) {
		gTxtOffset -= 2;
		if (gTxtOffset <= 0) {
			gTxtOffset = 0;
		}
		*(gTxtBuf + gTxtOffset) = 0;
		*(gTxtBuf + gTxtOffset + 1) = 0;
	}
	else {
		*(gTxtBuf + gTxtOffset) = c;
		gTxtOffset++;
		*(gTxtBuf + gTxtOffset) = color;
		gTxtOffset++;
	}

	setScreenPos(gTxtOffset );

	setCursor(gTxtOffset/2);
	
	return 0;
}



extern "C" __declspec(dllexport) int __kTextModeEntry(LPVESAINFORMATION vesa, DWORD fontbase, DWORD v86ProcessBase, int v86ProcessLen,
	DWORD v86IntBase, DWORD kerneldata, DWORD kernel16, DWORD kernel32) {

	int ret = 0;

	__initTask0((char*)vesa->PhyBasePtr + vesa->OffScreenMemOffset);

	gV86VMIEntry = v86ProcessBase;

	gV86VMISize = v86ProcessLen;

	gV86IntProc = v86IntBase;

	gKernelData = kerneldata;
	gKernel16 = kernel16;
	gKernel32 = kernel32;
	gFont = fontbase;
	glpVesaInfo = vesa;

 	DWORD svgaregs[16];
 	DWORD svgadev = 0;
 	DWORD svgairq = 0;
 	ret = getPciDevBasePort(svgaregs, 0x0300, &svgadev, &svgairq);
	for (int i = 0; i < 4; i++) {
		if (svgaregs[i] && (svgaregs[i] & 1) == 0)
		{
			gTxtBuf = (char*)(svgaregs[i] & 0xfffffff0);
		}
	}

	gTxtBuf = (char*)TEXTMODE_BASE;

	gTxtOffset = 0;

	initGdt();
	initIDT();

	initTextModeDevices();

	initMemory();

	initPaging();	

	initDll();

	initEfer();

	initCoprocessor();

	initTimer();

	__asm {
		in al, 60h
		sti	
	}

	initFileSystem();

	initDebugger();

	WINDOWCLASS window;
	window.showMode = 0;

	char cmd[1024];
	char* lpcmd = cmd;
	while (1)
	{
		unsigned int asc = __kGetKbd(0) & 0xff;
		//unsigned int asc = __getchar(0);
		//res = isScancodeAsc(asc);
		if (asc)
		{
			__sleep(0);

			outputChar(asc, INPUT_TEXTMODE_COLOR);
			*lpcmd= asc;
			lpcmd++;
			if (lpcmd - cmd >= sizeof(cmd)) {
				lpcmd = cmd;
			}
			else if (asc == 0x0a) {
				lpcmd--;
				*lpcmd = 0;
				lpcmd = cmd;
				runcmd(cmd);
			}
			else if (asc == 0x08) {
				lpcmd--;
				*lpcmd = 0;
				lpcmd--;
				if (lpcmd <= cmd) {
					lpcmd = cmd;
				}
				*lpcmd = 0;
			}
		}		
	}

	return 0;
}

//https://wiki.osdev.org/VGA_Hardware#Port_0x3C0
//http://www.osdever.net/FreeVGA/vga/portidx.htm

//光标在屏幕上的位置保存在显卡内部的两个光标寄存器中，
//索引寄存器的端口号是0x03d4。通过给索引寄存器写入索引值读取对应的显卡内部寄存器的值。
//两个 8 位光标寄存器，索引值分别是 14（0x0e）和 15（0x0f），分别存储光标位置的高 8 位和低 8 位。
//指定了索引寄存器的值之后，通过数据端口0x03d5读取数据。
int setCursor(int pos) {

	outportb(0x3d4, 0x0e);
	//int h = inportb(0x3d5);
	outportb(0x3d5, (pos>>8)&0xff);

	outportb(0x3d4, 0x0f);
	outportb(0x3d5, pos  & 0xff);
	//int h = inportb(0x3d5);
	return 0;
}

int clearTextScreen() {
	for (int i = 0; i < TEXTMODE_BUF_SIZE; i++) {
		*((char*)TEXTMODE_BASE + i) = 0;
	}
	return 0;
}

int setScreenPos(int offset) {

	if ( (offset < TEXTMODE_BUF_SIZE)) {
		int line = offset / (LINE_SIZE );
		int mod = offset % (LINE_SIZE );
		if (mod) {
			line++;
		}
		line++;
		offset = (line * (LINE_SIZE ) - (LINE_SIZE * ROW_CHAR_COUNT))/2;
		if (offset < 0) {
			offset = 0;
		}
	}
	else{
		offset = 0;
		gTxtOffset = offset;
		clearTextScreen();
	}

	outportb(0x3d4, 0x0c);
	outportb(0x3d5, (offset >> 8) & 0xff);

	outportb(0x3d4, 0x0d);
	outportb(0x3d5, offset & 0xff);

	return 0;
}