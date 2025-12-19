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
}WINDOWSINFO,*LPWINDOWSINFO;


typedef struct
{
	WINDOWCLASS *window;
	int valid;
}WINDOW_MEMU_ITEM;

typedef struct
{
	POINT pos;
	int status;

	int width;
	int height;
	unsigned int color;
	int cnt;
	unsigned int backGround;
	unsigned int backsize;
	WINDOW_MEMU_ITEM item[LEFTCLICK_MENU_HEIGHT/2/ GRAPHCHAR_HEIGHT];
}POPUPMENU;

#pragma pack()

int getOverlapRect(LPRECT r1, LPRECT r2, LPRECT res);



int insertPopupItem(LPWINDOWCLASS window);

int deletePopupItem(LPWINDOWCLASS window);

int GetFreeWindow();


#ifdef DLL_EXPORT
extern __declspec(dllexport)LPWINDOWSINFO gWindowsList;
extern __declspec(dllexport) POPUPMENU gPopupMenu;
extern "C"  __declspec(dllexport) char* GetVideoBase();

extern "C"  __declspec(dllexport) LPWINDOWSINFO GetProcessTextPos(int** x, int** y);
extern "C" __declspec(dllexport) LPWINDOWSINFO __FindWindow(char* wname);
extern "C" __declspec(dllexport) LPWINDOWSINFO __FindWindowID(DWORD wid);

extern "C" __declspec(dllexport) LPWINDOWSINFO __FindProcessWindow(int tid,int cpu);

extern "C" __declspec(dllexport) LPWINDOWSINFO getTopWindow();
extern "C" __declspec(dllexport) void initWindowList();
extern "C" __declspec(dllexport) DWORD isTopWindow(int wid);

extern "C" __declspec(dllexport) int RemoveWindow(int id);

extern "C" __declspec(dllexport) int InsertWindow(WINDOWCLASS* lpwindow,char * name);

extern "C" __declspec(dllexport) int traversalWindow(char*);

extern "C" __declspec(dllexport) int MaximizeWindow(WINDOWCLASS * window);
extern "C" __declspec(dllexport) int MinimizeWindow(WINDOWCLASS* cwin);

extern "C" __declspec(dllexport) LPWINDOWCLASS insertProcWindow (LPWINDOWCLASS window);

extern "C" __declspec(dllexport) LPWINDOWCLASS removeProcWindow();

extern "C" __declspec(dllexport) int destroyWindows();

extern "C" __declspec(dllexport) LPWINDOWCLASS getWindowFromName(char * winname);
#else
extern __declspec(dllimport)LPWINDOWSINFO gWindowsList;
extern __declspec(dllimport) POPUPMENU gPopupMenu;
extern "C"  __declspec(dllimport) char* GetVideoBase();
extern "C"  __declspec(dllimport) LPWINDOWSINFO GetProcessTextPos(int** x, int** y);
extern "C" __declspec(dllimport) LPWINDOWSINFO __FindProcessWindow(int tid,int cpu);

extern "C" __declspec(dllimport) LPWINDOWSINFO __FindWindow(char* wname);
extern "C" __declspec(dllimport) LPWINDOWSINFO __FindWindowID(DWORD wid);
extern "C" __declspec(dllimport) LPWINDOWSINFO getTopWindow();
extern "C" __declspec(dllimport) void initWindowList();
extern "C" __declspec(dllimport) DWORD isTopWindow(int wid);

extern "C" __declspec(dllimport) int RemoveWindow(int id);

extern "C" __declspec(dllimport) int InsertWindow(WINDOWCLASS*,char * name);

extern "C" __declspec(dllimport) int MaximizeWindow(LPWINDOWCLASS window);

extern "C" __declspec(dllimport) int MinimizeWindow(WINDOWCLASS * cwin);

extern "C" __declspec(dllimport) int traversalWindow(char*);

extern "C" __declspec(dllimport) LPWINDOWCLASS insertProcWindow(LPWINDOWCLASS window);

extern "C" __declspec(dllimport) LPWINDOWCLASS removeProcWindow();

extern "C" __declspec(dllimport) int destroyWindows();

extern "C" __declspec(dllimport) LPWINDOWCLASS getWindowFromName(char * winname);
#endif

