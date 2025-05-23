
#include "def.h"

//rcx,rdx,r8,r9
//rdi,rsi,rdx,rcx

extern "C" int __fastcall LiunuxOS64Entry(char* esptop,char* retaddr);


extern "C"  __declspec(dllexport) int Bit64EntryCPP() {
	while (1) {
		//break;
	}
	return 0;
}

extern "C" __declspec(dllexport) int __kKernelEntry64() {
	
	int ret = 0;

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