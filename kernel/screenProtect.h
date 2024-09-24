#pragma once
#include "video.h"

void pauseBreak();


extern int gScreenProtectWindowID ;



#ifdef DLL_EXPORT
extern "C" __declspec(dllexport) void initVectorGraph();
extern "C" __declspec(dllexport) void refreshScreenColor();
extern "C" __declspec(dllexport) int initScreenProtect();
extern "C" __declspec(dllexport) int stopScreenProtect();

extern "C" __declspec(dllexport) int __kPrintScreen();

extern "C" __declspec(dllexport) void __kScreenProtect(int p1, int p2, int p3, int p4);


#else
extern "C" __declspec(dllimport) void initVectorGraph();
extern "C" __declspec(dllimport) void refreshScreenColor();
extern "C" __declspec(dllimport) int initScreenProtect();
extern "C" __declspec(dllimport) int stopScreenProtect();

extern "C" __declspec(dllimport) int __kPrintScreen();

extern "C" __declspec(dllimport) void __kScreenProtect(int p1, int p2, int p3, int p4);

#endif


