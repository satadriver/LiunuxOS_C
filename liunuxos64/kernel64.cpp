
#include "def.h"

//rcx,rdx,r8,r9
//rdi,rsi,rdx,rcx

extern "C" int LiunuxOS64Entry(unsigned long long esptop);

extern "C" __declspec(dllexport) int __kKernelEntry64() {
	int ret = 0;
	ret = LiunuxOS64Entry(KERNEL64_STACK_TOP);

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