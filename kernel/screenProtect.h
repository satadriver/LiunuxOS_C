#pragma once
#include "video.h"

void pauseBreak();


extern int gScreenProtectWindowID ;

extern "C" __declspec(dllexport) void refreshScreenColor();

extern "C" __declspec(dllexport) void refreshScreenColor2();

extern "C" __declspec(dllexport) void refreshScreenColor3();
#ifdef DLL_EXPORT

extern "C" __declspec(dllexport) int initScreenProtect();
extern "C" __declspec(dllexport) int stopScreenProtect();

extern "C" __declspec(dllexport) int __kPrintScreen();

extern "C" __declspec(dllexport) void __kScreenProtect(int p1, int p2, int p3, int p4);

extern "C" __declspec(dllexport) void initVectorGraph();

extern "C" __declspec(dllexport)  void VectorGraph(DWORD p1, DWORD p2, DWORD p3, DWORD p4);

extern "C" __declspec(dllexport) void initTrajectory();

extern "C" __declspec(dllexport) void TrajectoryProc(DWORD p1, DWORD p2, DWORD p3, DWORD p4);
#else
extern "C" __declspec(dllimport) void initVectorGraph();
extern "C" __declspec(dllimport) void refreshScreenColor();
extern "C" __declspec(dllimport) int initScreenProtect();
extern "C" __declspec(dllimport) int stopScreenProtect();

extern "C" __declspec(dllimport) int __kPrintScreen();

extern "C" __declspec(dllimport) void __kScreenProtect(int p1, int p2, int p3, int p4);

extern "C" __declspec(dllimport) void initTrajectory();
#endif


