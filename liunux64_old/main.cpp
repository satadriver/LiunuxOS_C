
#include "def.h"
#include "main.h"

DWORD gV86VMIEntry = 0;
DWORD gV86VMISize = 0;
DWORD gV86IntProc = 0;
DWORD gKernel16 = 0;
DWORD gKernel32 = 0;
DWORD gKernelData = 0;
DWORD gVideoMode = 0;

extern "C" __declspec(dllexport) int __kMytest64() {
	return 64;
}



extern "C" __declspec(dllexport) int __kKernelEntry64() {
	return 64;
}


int __stdcall DllMain( char* hInstance, int fdwReason,  int lpvReserved) {
	return 1;
}