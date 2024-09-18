#pragma once

#include "def.h"
#include "ListEntry.h"
#include "video.h"



#define WINDOW_LIST_BUF_SIZE	0X10000

#pragma pack(1)

typedef struct  _WINDOWSINFO
{
	LIST_ENTRY list;
	DWORD valid;
	DWORD id;
	DWORD *cursorX;
	DWORD *cursorY;
	DWORD cursorColor;
	char windowname[WINDOW_NAME_LIMIT];
}WINDOWSINFO,*LPWINDOWSINFO;

#pragma pack()



LPWINDOWSINFO checkWindowExist(char * wname);
LPWINDOWSINFO checkWindowExist(DWORD wid);
LPWINDOWSINFO getFreeWindow();

int getOverlapRect(LPRECT r1, LPRECT r2, LPRECT res);

#ifdef DLL_EXPORT
extern "C" __declspec(dllexport) DWORD getTopWindow();
extern "C" __declspec(dllexport) void initWindowList();
extern "C" __declspec(dllexport) DWORD isTopWindow(int wid);

extern "C" __declspec(dllexport) int removeWindow(int id);

extern "C" __declspec(dllexport) int addWindow(int valid, DWORD *x, DWORD *y,int color,char * name);

extern "C" __declspec(dllexport) LPWINDOWCLASS insertProcWindow (LPWINDOWCLASS window);


extern "C" __declspec(dllexport) LPWINDOWCLASS removeProcWindow();

extern "C" __declspec(dllexport) int destroyWindows();

extern "C" __declspec(dllexport) LPWINDOWCLASS getWindowFromName(char * winname);
#else
extern "C" __declspec(dllimport) DWORD getTopWindow();
extern "C" __declspec(dllimport) void initWindowList();
extern "C" __declspec(dllimport) DWORD isTopWindow(int wid);

extern "C" __declspec(dllimport) int removeWindow(int id);

extern "C" __declspec(dllimport) int addWindow(int valid, DWORD *x, DWORD *y, int color,char * name);

extern "C" __declspec(dllimport) LPWINDOWCLASS insertProcWindow(LPWINDOWCLASS window);


extern "C" __declspec(dllimport) LPWINDOWCLASS removeProcWindow();

extern "C" __declspec(dllimport) int destroyWindows();

extern "C" __declspec(dllimport) LPWINDOWCLASS getWindowFromName(char * winname);
#endif

