#pragma once

#include "def.h"

#define DOS_SYSTIMER_ADDR		0X46C







extern "C" __declspec(dllexport) void __k8254TimerProc();

int __kAdd8254Timer(DWORD addr, DWORD delay, DWORD param1, DWORD param2, DWORD param3, DWORD param4);

void __kRemove8254Timer(int n);

void init8254Timer();