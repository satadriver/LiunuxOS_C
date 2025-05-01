#pragma once
#include "video.h"

void pauseBreak();


extern int gScreenProtectWindowID ;


#ifdef DLL_EXPORT
extern "C" __declspec(dllexport) void SpiralBallVideo();

extern "C" __declspec(dllexport) void DiamondVideo();

extern "C" __declspec(dllexport) void CubeVideo();

extern "C" __declspec(dllexport) void EllipseVideo();

extern "C" __declspec(dllexport) int initScreenProtect();

extern "C" __declspec(dllexport) int stopScreenProtect();

extern "C" __declspec(dllexport) int __kPrintScreen();

extern "C" __declspec(dllexport) void __kScreenProtect(int p1, int p2, int p3, int p4);

extern "C" __declspec(dllexport) void initSquareVideo();

extern "C" __declspec(dllexport)  void SquareVideo(DWORD p1, DWORD p2, DWORD p3, DWORD p4);

extern "C" __declspec(dllexport) void initTrajectory();

extern "C" __declspec(dllexport) void TrajectoryVideo(DWORD p1, DWORD p2, DWORD p3, DWORD p4);
#else
extern "C" __declspec(dllimport) void SpiralBallVideo();

extern "C" __declspec(dllimport) void DiamondVideo();

extern "C" __declspec(dllimport) void CubeVideo();

extern "C" __declspec(dllimport) void initSquareVideo();

extern "C" __declspec(dllimport)  void SquareVideo(DWORD p1, DWORD p2, DWORD p3, DWORD p4);

extern "C" __declspec(dllimport) void EllipseVideo();

extern "C" __declspec(dllimport) int initScreenProtect();

extern "C" __declspec(dllimport) int stopScreenProtect();

extern "C" __declspec(dllimport) int __kPrintScreen();

extern "C" __declspec(dllimport) void __kScreenProtect(int p1, int p2, int p3, int p4);

extern "C" __declspec(dllimport) void initTrajectory();
#endif


