#include "Utils.h"
#include "def.h"
#include "Kernel.h"
#include "video.h"
#include "keyboard.h"
#include "mouse.h"
#include "process.h"
#include "task.h"
#include "Pe.h"
#include "ata.h"
#include "fat32/FAT32.h"
#include "fat32/fat32file.h" 
#include "file.h"
#include "NTFS/ntfs.h"
#include "NTFS/ntfsFile.h"
#include "pci.h"
#include "speaker.h"
#include "cmosAlarm.h"
#include "serialUART.h"
#include "floppy.h"
#include "malloc.h"
#include "page.h"
#include "processDOS.h"
#include "gdi.h"
#include "coprocessor.h"
#include "Thread.h"
#include "debugger.h"
#include "descriptor.h"
#include "elf.h"
#include "page.h"
#include "device.h"
#include "core.h"
#include "cmosPeriodTimer.h"
#include "apic.h"
#include "acpi.h"
#include "window.h"
#include "VMM.h"
#include "rtl8139.h"

//#pragma comment(linker, "/ENTRY:DllMain")
//#pragma comment(linker, "/align:512")
//#pragma comment(linker, "/merge:.data=.text")

//https://www.cnblogs.com/ck1020/p/6115200.html

#pragma comment(linker, "/STACK:0x100000")

DWORD gV86VMIEntry = 0;
DWORD gV86VMISize = 0;
DWORD gV86IntProc = 0;
DWORD gKernel16 = 0;
DWORD gKernel32 = 0;
DWORD gKernelData = 0;
DWORD gVideoMode = 0;


int __kernelEntry(LPVESAINFORMATION vesa, DWORD fontbase, DWORD v86ProcessBase, int v86ProcessLen,
	DWORD v86IntBase, DWORD kerneldata, DWORD kernel16, DWORD kernel32) {

	int ret = 0;

	gVideoMode = *(WORD*)((char*)vesa - 2);

	gV86VMIEntry = v86ProcessBase;

	gV86VMISize = v86ProcessLen + 1024;

	gV86IntProc = v86IntBase;

	gKernelData = kerneldata;
	gKernel16 = kernel16;
	gKernel32 = kernel32;
	
	__initTask0((char*)vesa->PhyBasePtr + vesa->OffScreenMemOffset);
	__initVideo(vesa, fontbase);

	char szout[1024];

	initGdt();
	initIDT();
	SetIVTVector();

	initDevices();

	initMemory();

	initPaging();

	initEfer();

	initACPI();

	initCoprocessor();

	initTimer();

	sysEntryInit((DWORD)sysEntry);

	enableVME();
	enablePCE();
	enableMCE();
	enableTSD();
	//enableVMXE();

	initDebugger();

	initWindowList();

	BPCodeStart();

	ret = StartVirtualTechnology();
	if (ret)
	{
		StopVirtualTechnology();
	}

	initDll();

	//initNIC();

	EnterLongMode();

	__asm {
		sti
	}

#ifdef VM86_PROCESS_TASK
	__createDosCodeProc(gV86VMIEntry, gV86VMISize, "V86VMIEntry");
#else

#endif
	
	initFileSystem();

	int imagesize = getSizeOfImage((char*)KERNEL_DLL_SOURCE_BASE);
	DWORD kernelMain = getAddrFromName(KERNEL_DLL_BASE, "__kKernelMain");
	if (kernelMain)
	{
		TASKCMDPARAMS cmd;
		__memset((char*)&cmd, 0, sizeof(TASKCMDPARAMS));
		//__kCreateThread((DWORD)__kSpeakerProc, (DWORD)&cmd, "__kSpeakerProc");
		__kCreateThread((unsigned int)kernelMain, KERNEL_DLL_BASE, (DWORD)&cmd, "__kKernelMain");
		//__kCreateProcess((unsigned int)KERNEL_DLL_SOURCE_BASE, imagesize, "kernel.dll", "__kKernelMain", 3, 0);
	}

	//logFile("__kernelEntry\n");
	
	//ret = loadLibRunFun("c:\\liunux\\main.dll", "__kMainProcess");

	__printf(szout, "Hello world Liunux!Version:%s\r\nPress any key to continue...\r\n",LIUNUXOS_VERSION);

	WINDOWCLASS window;
	initDesktopWindow(&window, "__kKernel", 0,0);

	while (1) 
	{
		int ck = __kGetKbd(window.id)&0xff;
		if (ck==0x1b ) {
			break;
		}
		__sleep(0);
	}

	if (__findProcessFuncName(EXPLORER_TASKNAME) == FALSE)
	{
		__kCreateProcess(MAIN_DLL_SOURCE_BASE, imagesize, "main.dll", EXPLORER_TASKNAME, 3, 0);
	}

	__sleep(1000);

	//AllocateAP(INTR_8259_MASTER + 1);
	//AllocateAP(INTR_8259_SLAVE + 4);

	while (1)
	{
		__asm {
			hlt
		}
	}

	return 0;
}



void __kKernelMain(DWORD retaddr,int pid,char * filename,char * funcname,DWORD param) {

	int ret = 0;

 	char szout[1024];
	__printf(szout, "__kKernelMain task pid:%x,filename:%s,function name:%s\n", pid, filename,funcname);

	char* str = "Hi,how are you?Fine,thank you, and you ? I'm fine too!";

	return;

 	ret = sendUARTData((unsigned char*)str, __strlen(str),COM1PORT);
 
 	unsigned char recvbuf[1024];
 	int recvlen = getCom1Data(recvbuf);
 	if (recvlen > 0)
 	{
 		*(recvbuf + recvlen) = 0;

 		__printf(szout, "com recv data:%s\n", recvbuf);
 	}
	return;
}













#include "servicesProc.h"

#ifdef _DEBUG


#include "math.h"

void mytest(LIGHT_ENVIRONMENT  * stack) {

	return;
}

#endif


#include "pe64.h"
#include <stdio.h>
#ifdef _USRDLL
int __stdcall DllMain( HINSTANCE hInstance,  DWORD fdwReason,  LPVOID lpvReserved) {
	return TRUE;
}
#else
int __stdcall WinMain(  HINSTANCE hInstance,  HINSTANCE hPrevInstance,  LPSTR lpCmdLine,  int nShowCmd )
{
	char* data = new char[0x100000];
	FILE* hf = fopen("liunux64.dll","rb+");
	fseek(hf, 0, 2);
	int fs = ftell(hf);
	fseek(hf, 0, 0);
	fread(data, 1, fs, hf);
	fclose(hf);
	char* exebuf = new char[0x100000];
	char * realbuf = (char*)MemLoadDll64(data,exebuf);
	char* addr2 = (char*)getAddrFromOrd64((char*)realbuf, 1);
	typedef int (*ptrfun)();
	ptrfun mytest1 = (ptrfun) getAddrFromName64(realbuf, "__kMytest64");
	int ret = mytest1();
	

#ifdef _DEBUG
	mytest(0);
#endif
	return TRUE;
}
#endif