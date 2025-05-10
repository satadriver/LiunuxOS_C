#pragma once
#include "def.h"

#define MOUSE_POS_LIMIT				256

#define MOUSE_SHOW_COLOR			0x9f9f9f9f
#define MOUSE_BORDER_WIDTH			2
#define MOUSE_SHOW_RATIO			40
#define MOUSE_BORDER_COLOR			0

#define MOUSE_GRANULARITY			GRAPHCHAR_HEIGHT

#pragma pack (1)
typedef struct
{
	//; bit 6 - 7 两个比特位必须为0
	//; bit 5: if deltaY is negtive, then set to 1, else set to 0
	//; bit 4: if deltaX is negtive, then set to 1, else set to 0
	//; bit 3  always set to 1, means than this mouse packet data is valid
	//; bit 2	middle click
	//; bit 1  right click
	//; bit 0  left click
	int status;
	int x;
	int y;
	int z;
	int scroll;
}MOUSEINFO, *LPMOUSEINFO;

typedef struct {
	MOUSEINFO	mintrData;
	int			mouseWidth;
	int			mouseHeight;
	int			mouseX;
	int			mouseY;
	int			mouseZ;
	MOUSEINFO	mouseBuf[MOUSE_POS_LIMIT];
	int			mouseBufHdr;
	int			mouseBufTail;
	int			bInvalid;
	
	DWORD		mouseColor;
	DWORD		mouseCoverData[0x4000];
}MOUSEDATA,*LPMOUSEDATA;

#pragma pack()



void mousetest();

extern "C" void MouseIntProc();




int isGeometryBorder(int x, int y);

int isGeometryMouse(int x, int y);

#ifdef DLL_EXPORT
extern "C" __declspec(dllexport) DWORD gMouseID;
extern "C"  __declspec(dllexport) void __initMouse(int x, int y);
extern "C"  __declspec(dllexport) DWORD gMouseTest;

extern "C"  __declspec(dllexport) int getmouse(LPMOUSEINFO lpinfo,int id);
extern "C"  __declspec(dllexport) void drawMouse();
extern "C"  __declspec(dllexport) void invalidMouse();
extern "C"  __declspec(dllexport) void restoreMouse();

extern "C"  __declspec(dllexport) void __kMouseProc();

extern "C"  __declspec(dllexport) int GetMousePacket(LPMOUSEINFO lpmouse);

extern "C"  __declspec(dllexport) int __kGetMouse(LPMOUSEINFO lpmouse, int wid);

extern "C"  __declspec(dllexport) void __kRefreshMouseBackup();

extern "C"  __declspec(dllexport) void __kRestoreMouse();

extern "C"  __declspec(dllexport) void __kDrawMouse();

extern "C"  __declspec(dllexport) void insertMouse(MOUSEINFO* info);
#else
extern "C" __declspec(dllimport) DWORD gMouseID;
extern "C"  __declspec(dllimport) void __initMouse(int x, int y);
extern "C"  __declspec(dllimport) DWORD gMouseTest;

extern "C"  __declspec(dllimport) int getmouse(LPMOUSEINFO lpinfo, int id);
extern "C"  __declspec(dllimport) void invalidMouse();
extern "C"  __declspec(dllimport) void restoreMouse();
extern "C"  __declspec(dllimport) void drawMouse();

extern "C"  __declspec(dllimport) void __kMouseProc();
extern "C"  __declspec(dllimport) int GetMousePacket(LPMOUSEINFO lpmouse);
extern "C"  __declspec(dllimport) int __kGetMouse(LPMOUSEINFO lpmouse, int wid);
extern "C"  __declspec(dllimport) void __kRefreshMouseBackup();
extern "C"  __declspec(dllimport) void __kRestoreMouse();
extern "C"  __declspec(dllimport) void __kDrawMouse();

extern "C"  __declspec(dllimport) void insertMouse(MOUSEINFO * info);
#endif