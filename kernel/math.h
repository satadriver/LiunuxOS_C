#pragma once
#include "def.h"

#ifndef _MATH_H_
#define _MATH_H_


// Visual Studio 中的定义
#define FLT_MAX         3.402823466e+38F        // 最大浮点数
#define FLT_MAX_10_EXP  38                       // 10 的幂的最大指数
#define FLT_MAX_EXP     128                      // 2 的幂的最大指数

#define PI 						(3.141592653589793238462643)
#define E 						(2.7182818284590452353602874713527)
#define SQRT2 					(1.414213562373095145474621858739)
#define DOUBLE_PRECISION_MIN	0.0000001

#define SL_2PI			PI*2
#define SL_PI			PI
#define SL_PI_DIV_2		PI/2

#define LN2			0.69314718055994530941723212145818   // ln(2)
#define EPSILON		1e-15
#define MAX_ITER	100
#define INFINITY	1e308

#define LN10		2.3025850929940456840179914546844  // ln(10)




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
extern "C"  __declspec(dllexport) double __atan(double y, double x, int infNum);
extern "C"  __declspec(dllexport) double _sin(double x);

extern "C"  __declspec(dllexport) double __log(double x);
extern "C"  __declspec(dllexport) double __logn(double base, double exp);
extern "C"  __declspec(dllexport) double __exp(double x);

extern "C" __declspec(dllexport) int abs(int x);
extern "C" __declspec(dllexport) double pown(double x, int n);
extern "C" __declspec(dllexport) double cos(double x);
extern "C" __declspec(dllexport) double sin(double x);
extern "C" __declspec(dllexport) double pow(double a, int b);
extern "C" __declspec(dllexport) double sqrt(double x);

extern "C" __declspec(dllexport) float sqrtf(float x);
extern "C" __declspec(dllexport) float cosf(float x);
extern "C" __declspec(dllexport) float sinf(float x);

extern "C" __declspec(dllexport) float fabsf(float x);
extern "C" __declspec(dllexport) float fabs(float x);

extern "C" __declspec(dllexport) double log(double x);

extern "C" __declspec(dllexport) double exp(double x);

extern "C" __declspec(dllexport) float logf(float x);

extern "C" __declspec(dllexport) float expf(float x);
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
extern "C"  __declspec(dllimport) double __atan(double y, double x, int infNum);
extern "C"  __declspec(dllimport) double _sin(double x);

extern "C"  __declspec(dllimport) double __log(double x);
extern "C"  __declspec(dllimport) double __logn(double base, double exp);
extern "C"  __declspec(dllimport) double __exp(double x);

extern "C" __declspec(dllimport) double abs(double x);
extern "C" __declspec(dllimport) double pown(double x, int n);
extern "C" __declspec(dllimport) double cos(double x);
extern "C" __declspec(dllimport) double sin(double x);
extern "C" __declspec(dllimport) double pow(double a, int b);
extern "C" __declspec(dllimport) double sqrt(double x);

extern "C" __declspec(dllimport) float sqrtf(float x);
extern "C" __declspec(dllimport) float cosf(float x);
extern "C" __declspec(dllimport) float sinf(float x);

extern "C" __declspec(dllimport) float fabsf(float x);
extern "C" __declspec(dllimport) float fabs(float x);

extern "C" __declspec(dllimport) double log(double x);

extern "C" __declspec(dllimport) double exp(double x);

extern "C" __declspec(dllimport) float logf(float x);

extern "C" __declspec(dllimport) float expf(float x);
#endif



#endif

int GetCos(int angle);

int GetSin(int angle);

extern "C" int g_sincos[256];


