

//不要忘了头文件，否则即使声明函数导出，没有包含也不会导出
#include "main.h"
#include "utils.h"
#include "mouse.h"
#include "gdi/jpeg.h"
#include "dl.h"
#include "mainUtils.h"

#pragma comment(lib,"../release/kernel.lib")



//https://wiki.osdev.org/





#ifdef _USRDLL
int __stdcall DllMain(unsigned int hInstance, unsigned int fdwReason, unsigned int lpvReserved) {

	return 1;
}
#else
int __stdcall WinMain(unsigned int hInstance, unsigned int hPrevInstance, char * lpCmdLine, int nShowCmd)
{
	unsigned long long total = 0;
	HeapAllocTest(0x100, 0x10000, &total);
#ifdef _DEBUG

	//testjpeg();
#endif
	return 1;
}
#endif


