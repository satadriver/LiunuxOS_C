#pragma once
#include "def.h"

#ifndef _MATH_H_
#define _MATH_H_

#define PI 		(3.141592653589793238462643)
#define E 		(2.7182818284590452353602874)
#define SQRT2 	(1.414213562373095145474621858739)

extern "C" __declspec(dllexport) DWORD __sqrtInteger(DWORD i);

double abs(double x);
double pown(double x, int n);
double cos(double x);
double sin(double x);
double pow(double a, int b);
double sqrt(double x);
double __sqrt(double a);

#endif

int GetCos(int angle);

int GetSin(int angle);

extern "C" int g_sincos[256];
