#pragma once
#include "def.h"
#include "video.h"
#include "window.h"


extern "C" __declspec(dllexport) void initRightMenu(RIGHTMENU * menu, int tid);

extern "C" __declspec(dllexport) int __kDrawWindowsMenu();

extern "C" __declspec(dllexport) int __drawRightMenu(RIGHTMENU* menu);

extern "C" __declspec(dllexport) int __restoreRightMenu(RIGHTMENU* menu);

void initPopupMenu(POPUPMENU* menu);

int __drawLeftMenu(POPUPMENU* menu);

int __restoreLeftMenu(POPUPMENU* menu);


