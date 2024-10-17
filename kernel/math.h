#pragma once
#include "def.h"

#ifndef _MATH_H_
#define _MATH_H_

#define PI 						(3.141592653589793238462643)
#define E 						(2.7182818284590452353602874)
#define SQRT2 					(1.414213562373095145474621858739)
#define DOUBLE_PRECISION_MIN	0.0000001



#ifdef DLL_EXPORT
extern "C" __declspec(dllexport) DWORD __sqrtInteger(DWORD i);
extern "C"  __declspec(dllexport) double __abs(double x);
extern "C"  __declspec(dllexport) double __pown(double x, int n);
extern "C"  __declspec(dllexport) double __cos(double x);
extern "C"  __declspec(dllexport) double __sin(double x);
extern "C"  __declspec(dllexport) double __pow(double a, int b);
extern "C"  __declspec(dllexport) double __sqrt(double x);

extern "C"  __declspec(dllexport) double _sqrt(double x);

extern "C"  __declspec(dllexport) double __acos(double x);
extern "C"  __declspec(dllexport) double __asin(double x);

#else
extern "C" __declspec(dllimport) DWORD __sqrtInteger(DWORD i);
extern "C"  __declspec(dllimport) double __abs(double x);
extern "C"  __declspec(dllimport) double __pown(double x, int n);
extern "C"  __declspec(dllimport) double __cos(double x);
extern "C"  __declspec(dllimport) double __sin(double x);
extern "C"  __declspec(dllimport) double __pow(double a, int b);
extern "C"  __declspec(dllimport) double __sqrt(double a);

extern "C"  __declspec(dllimport) double _sqrt(double x);

extern "C"  __declspec(dllimport) double __acos(double x);
extern "C"  __declspec(dllimport) double __asin(double x);
#endif



#endif

int GetCos(int angle);

int GetSin(int angle);

extern "C" int g_sincos[256];


