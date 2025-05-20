

extern "C" __declspec(dllexport) int __kMytest64() {
	return 64;
}



extern "C" __declspec(dllexport) int __kKernelEntry64() {
	return 64;
}


int __stdcall DllMain( char* hInstance, int fdwReason,  int lpvReserved) {
	return 1;
}