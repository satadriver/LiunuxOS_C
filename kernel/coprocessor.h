#pragma once

#include "def.h"

void enableSSE();

void enableAVX();

void initCoprocessor();

#ifdef DLL_EXPORT
extern "C" __declspec(dllexport) void __kCoprocessor();
#else

extern "C" __declspec(dllimport) void __kCoprocessor();
#endif