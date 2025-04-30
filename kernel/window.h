#pragma once

#include "def.h"
#include "ListEntry.h"
#include "video.h"



#define WINDOW_LIST_BUF_SIZE	0X10000

#pragma pack(1)

typedef struct  _WINDOWSINFO
{
	LIST_ENTRY list;
	WINDOWCLASS * window;
	DWORD valid;
	DWORD id;

}WINDOWSINFO,*LPWINDOWSINFO;

#pragma pack()




LPWINDOWSINFO getFreeWindow();

int getOverlapRect(LPRECT r1, LPRECT r2, LPRECT res);



#ifdef DLL_EXPORT
extern "C"  __declspec(dllexport) char* GetVideoBase();

extern "C"  __declspec(dllexport) LPWINDOWSINFO GetProcessTextPos(int** x, int** y);
extern "C" __declspec(dllexport) LPWINDOWSINFO __FindWindow(char* wname);
extern "C" __declspec(dllexport) LPWINDOWSINFO __FindWindowID(DWORD wid);
extern "C" __declspec(dllexport) LPWINDOWCLASS getProcessWindow(int pid);

extern "C" __declspec(dllexport) DWORD getTopWindow();
extern "C" __declspec(dllexport) void initWindowList();
extern "C" __declspec(dllexport) DWORD isTopWindow(int wid);

extern "C" __declspec(dllexport) int removeWindow(int id);

extern "C" __declspec(dllexport) int addWindow(DWORD lpwindow,char * name);

extern "C" __declspec(dllexport) LPWINDOWCLASS insertProcWindow (LPWINDOWCLASS window);


extern "C" __declspec(dllexport) LPWINDOWCLASS removeProcWindow();

extern "C" __declspec(dllexport) int destroyWindows();

extern "C" __declspec(dllexport) LPWINDOWCLASS getWindowFromName(char * winname);
#else
extern "C"  __declspec(dllimport) char* GetVideoBase();
extern "C"  __declspec(dllimport) LPWINDOWSINFO GetProcessTextPos(int** x, int** y);
extern "C" __declspec(dllimport) LPWINDOWCLASS getProcessWindow(int pid);
extern "C" __declspec(dllimport) LPWINDOWSINFO __FindWindow(char* wname);
extern "C" __declspec(dllimport) LPWINDOWSINFO __FindWindowID(DWORD wid);
extern "C" __declspec(dllimport) DWORD getTopWindow();
extern "C" __declspec(dllimport) void initWindowList();
extern "C" __declspec(dllimport) DWORD isTopWindow(int wid);

extern "C" __declspec(dllimport) int removeWindow(int id);

extern "C" __declspec(dllimport) int addWindow(DWORD ,char * name);

extern "C" __declspec(dllimport) LPWINDOWCLASS insertProcWindow(LPWINDOWCLASS window);


extern "C" __declspec(dllimport) LPWINDOWCLASS removeProcWindow();

extern "C" __declspec(dllimport) int destroyWindows();

extern "C" __declspec(dllimport) LPWINDOWCLASS getWindowFromName(char * winname);
#endif

