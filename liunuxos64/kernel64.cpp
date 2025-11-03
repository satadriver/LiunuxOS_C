
#include "def.h"
#include "kernel64.h"

//rcx,rdx,r8,r9
//rdi,rsi,rdx,rcx

#define GDT_BASE				0X560000

char* g_bit32Address = 0;
char* g_regebp32 = 0;

extern "C" int __fastcall LiunuxOS64Entry(char* esptop,char* retaddr);

extern "C" int __fastcall LiunuxOS64Leave(int seg32,char * retaddr32,char * gdt32,char * rbp32);


extern "C"  __declspec(dllexport) int Bit64EntryCPP() {

	__kKernelLeave64(g_bit32Address, g_regebp32);

	return 0;
}

extern "C" __declspec(dllexport) int __kKernelLeave64(char* retaddr32, char * regebp32) {

	int ret = 0;

	ret = LiunuxOS64Leave(KERNEL_MODE_CODE, retaddr32, (char*)GDT_BASE, regebp32);
	return ret;
}

extern "C" __declspec(dllexport) int __kKernelEntry64(char * retaddr32,char * ebp32) {
	
	int ret = 0;

	g_bit32Address = retaddr32;
	g_regebp32 = ebp32;

	ret = LiunuxOS64Entry((char*)KERNEL64_STACK_TOP,(char*) Bit64EntryCPP);

	return 64;
}

#ifdef _WINDLL
int __stdcall DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID lpvReserved) {
	return TRUE;
}
#elif defined _WIN32
int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd){

	return 0;

}
#else

#endif