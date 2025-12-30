
#include "def.h"
#include "kernel64.h"

//rcx,rdx,r8,r9
//rdi,rsi,rdx,rcx



unsigned long g_bit32Address = 0;
unsigned long g_regEsp32 = 0;

extern "C" __declspec(dllexport) int  LiunuxOS64Entry(char* esptop,char* retaddr);

extern "C" __declspec(dllexport) int  LiunuxOS64Leave(int seg32, unsigned long retaddr32,char * gdt32, unsigned long rbp32);


extern "C"  __declspec(dllexport)  int Bit64EntryCPP() {

	__kKernelLeave64(g_bit32Address, g_regEsp32);

	return 0;
}

extern "C" __declspec(dllexport) int __kKernelLeave64(unsigned long retaddr32, unsigned long regebp32) {

	int ret = 0;

	ret = LiunuxOS64Leave(KERNEL_MODE_CODE, retaddr32, (char*)GDT_BASE, regebp32);
	return ret;
}

extern "C" __declspec(dllexport) int __kKernelEntry64(unsigned long retaddr32,unsigned long  esp32) {
	
	int ret = 0;

	g_bit32Address = retaddr32;
	g_regEsp32 = esp32;

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