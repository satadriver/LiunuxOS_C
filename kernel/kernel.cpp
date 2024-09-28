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


int __kernelEntry(LPVESAINFORMATION vesa, DWORD fontbase, DWORD v86ProcessBase, int v86ProcessLen,
	DWORD v86IntBase, DWORD kerneldata, DWORD kernel16, DWORD kernel32) {

	int ret = 0;

	gV86VMIEntry = v86ProcessBase;

	gV86VMISize = v86ProcessLen + 1024;

	gV86IntProc = v86IntBase;

	gKernelData = kerneldata;
	gKernel16 = kernel16;
	gKernel32 = kernel32;

	__initVideo(vesa, fontbase);

	char szout[1024];

	initGdt();
	initIDT();

	initDevices();

	initMemory();

	initPaging();

	__initTask();

	initDll();

	initEfer();

	initACPI();

	initCoprocessor();

	initTimer();

	sysEntryInit((DWORD)sysEntry);

	enableVME();
	enablePCE();
	enableMCE();
	enableTSD();

	initDebugger();

	__asm {
		in al, 0x60
		sti
	}

#ifdef SINGLE_TASK_TSS
	__createDosCodeProc(gV86VMIEntry, gV86VMISize, "V86VMIEntry");
#else
	__createDosCodeProc(gV86VMIEntry, gV86VMISize, "V86VMIEntry");
#endif

	__printf(szout, "Hello world of Liunux!\r\n");

	initFileSystem();

	int imagesize = getSizeOfImage((char*)KERNEL_DLL_SOURCE_BASE);
	DWORD kernelMain = getAddrFromName(KERNEL_DLL_BASE, "__kKernelMain");
	if (kernelMain)
	{
		TASKCMDPARAMS cmd;
		__memset((char*)&cmd, 0, sizeof(TASKCMDPARAMS));
		//__kCreateThread((DWORD)__kSpeakerProc, (DWORD)&cmd, "__kSpeakerProc");
		//__kCreateThread((unsigned int)kernelMain, KERNEL_DLL_BASE, (DWORD)&cmd, "__kKernelMain");
		//__kCreateProcess((unsigned int)KERNEL_DLL_SOURCE_BASE, imagesize, "kernel.dll", "__kKernelMain", 3, 0);
	}

	//logFile("__kernelEntry\n");
	
	//ret = loadLibRunFun("c:\\liunux\\main.dll", "__kMainProcess");

	//__kGetKbd(0);

	while (1)
	{
		if (__findProcessFuncName("__kExplorer") == FALSE)
		{
			__printf(szout, "__kCreateProcess __kExplorer before\r\n");
			__kCreateProcess(MAIN_DLL_SOURCE_BASE, imagesize, "main.dll", "__kExplorer", 3, 0);
			__printf(szout, "__kCreateProcess __kExplorer end\r\n");
		}

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














#ifdef _DEBUG

#include <string>

#include <winternl.h>
#include <winsock.h>
#include <Windows.h>
#pragma comment(lib,"ws2_32.lib")

using namespace std;

typedef DWORD(WINAPI* NTQUERYSYSTEMINFORMATION)(DWORD, PVOID, DWORD, PDWORD);
typedef struct _SYSTEM_HANDLE_INFORMATION
{
    ULONG ProcessId;
    UCHAR ObjectTypeNumber;
    UCHAR Flags;
    USHORT Handle;
    PVOID Object;
    ACCESS_MASK GrantedAccess;
}SYSTEM_HANDLE_INFORMATION, * PSYSTEM_HANDLE_INFORMATION;
#define STATUS_INFO_LENGTH_MISMATCH 0x004
typedef struct _SYSTEM_HANDLE_INFORMATION_EX
{
    ULONG NumberOfHandles;
    SYSTEM_HANDLE_INFORMATION Information[165536];
}SYSTEM_HANDLE_INFORMATION_EX, * PSYSTEM_HANDLE_INFORMATION_EX;
#define SystemHandleInformation 0x10  // 16
typedef struct _FILE_NAME_INFORMATION {
    ULONG FileNameLength;
    WCHAR FileName[256];
} FILE_NAME_INFORMATION, * PFILE_NAME_INFORMATION;
typedef struct _NM_INFO
{
    HANDLE   hFile;
    FILE_NAME_INFORMATION Info;
} NM_INFO, * PNM_INFO;
typedef enum _RFILE_INFORMATION_CLASS {
    FileDirectoryInformation1 = 1,
    FileFullDirectoryInformation,
    FileBothDirectoryInformation,
    FileBasicInformation,
    FileStandardInformation,
    FileInternalInformation,
    FileEaInformation,
    FileAccessInformation,
    FileNameInformation,
    FileRenameInformation,
    FileLinkInformation,
    FileNamesInformation,
    FileDispositionInformation,
    FilePositionInformation,
    FileFullEaInformation,
    FileModeInformation,
    FileAlignmentInformation,
    FileAllInformation,
    FileAllocationInformation,
    FileEndOfFileInformation,
    FileAlternateNameInformation,
    FileStreamInformation,
    FilePipeInformation,
    FilePipeLocalInformation,
    FilePipeRemoteInformation,
    FileMailslotQueryInformation,
    FileMailslotSetInformation,
    FileCompressionInformation,
    FileObjectIdInformation,
    FileCompletionInformation,
    FileMoveClusterInformation,
    FileQuotaInformation,
    FileReparsePointInformation,
    FileNetworkOpenInformation,
    FileAttributeTagInformation,
    FileTrackingInformation,
    FileIdBothDirectoryInformation,
    FileIdFullDirectoryInformation,
    FileValidDataLengthInformation,
    FileShortNameInformation,
    FileIoCompletionNotificationInformation,
    FileIoStatusBlockRangeInformation,
    FileIoPriorityHintInformation,
    FileSfioReserveInformation,
    FileSfioVolumeInformation,
    FileHardLinkInformation,
    FileProcessIdsUsingFileInformation,
    FileNormalizedNameInformation,
    FileNetworkPhysicalNameInformation,
    FileIdGlobalTxDirectoryInformation,
    FileIsRemoteDeviceInformation,
    FileUnusedInformation,
    FileNumaNodeInformation,
    FileStandardLinkInformation,
    FileRemoteProtocolInformation,
    FileRenameInformationBypassAccessCheck,
    FileLinkInformationBypassAccessCheck,
    FileVolumeNameInformation,
    FileIdInformation,
    FileIdExtdDirectoryInformation,
    FileReplaceCompletionInformation,
    FileHardLinkFullIdInformation,
    FileIdExtdBothDirectoryInformation,
    FileMaximumInformation
} RFILE_INFORMATION_CLASS, * PRFILE_INFORMATION_CLASS;
typedef NTSTATUS(WINAPI* ZWQUERYINFORMATIONFILE)(HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, RFILE_INFORMATION_CLASS);
char* GetFileName(HMODULE hNtDll, PNM_INFO lpParameter)
{
    char str[1024];
    PNM_INFO         NmInfo = (PNM_INFO)lpParameter;
    IO_STATUS_BLOCK IoStatus;
    ZWQUERYINFORMATIONFILE ZwQueryInformationFile =
        (ZWQUERYINFORMATIONFILE)GetProcAddress(hNtDll, "ZwQueryInformationFile");
    ZwQueryInformationFile(NmInfo->hFile, &IoStatus, &NmInfo->Info, 256, RFILE_INFORMATION_CLASS::FileNameInformation);
    if (NmInfo->Info.FileNameLength != 0)
    {
        
        int strsize = WideCharToMultiByte(CP_ACP,0, NmInfo->Info.FileName, NmInfo->Info.FileNameLength / sizeof(WCHAR),str,sizeof(str),0,0);
        str[strsize] = 0;
    }
    return str;
}




void mytest(LIGHT_ENVIRONMENT  * stack) {
    WSADATA wsa;
    WSAStartup(0x0202, &wsa);

    HMODULE hNtDll = LoadLibraryA("ntdll.dll");
    NTQUERYSYSTEMINFORMATION NtQuerySystemInformation =
        (NTQUERYSYSTEMINFORMATION)GetProcAddress(hNtDll, "ZwQuerySystemInformation");
    ULONG cbBuffer = sizeof(SYSTEM_HANDLE_INFORMATION_EX);
    LPVOID pBuffer = (LPVOID)malloc(cbBuffer);
    auto id = GetCurrentProcessId();
    if (pBuffer)
    {
        NtQuerySystemInformation(SystemHandleInformation, pBuffer, cbBuffer, NULL);
        PSYSTEM_HANDLE_INFORMATION_EX pInfo = (PSYSTEM_HANDLE_INFORMATION_EX)pBuffer;
        for (ULONG r = 0; r < pInfo->NumberOfHandles; r++)
        {
            if (pInfo->Information[r].ObjectTypeNumber)
            {
                NM_INFO nmInfo = { 0 };
                nmInfo.hFile = (HANDLE)pInfo->Information[r].Handle;
                char* fileName = GetFileName(hNtDll, &nmInfo);
                printf("find file:%s\r\n", fileName);
            }
        }

        free(pBuffer);
    }
    FreeModule(hNtDll);

	return;
}

#endif

#ifdef _USRDLL
int __stdcall DllMain( HINSTANCE hInstance,  DWORD fdwReason,  LPVOID lpvReserved) {
	return TRUE;
}
#else
int __stdcall WinMain(  HINSTANCE hInstance,  HINSTANCE hPrevInstance,  LPSTR lpCmdLine,  int nShowCmd )
{
	LIGHT_ENVIRONMENT stack = { 0 };
#ifdef _DEBUG
	mytest(&stack);
#endif
	return TRUE;
}
#endif