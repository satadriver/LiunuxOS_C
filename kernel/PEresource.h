#pragma once

#include "def.h"

extern "C" __declspec(dllexport) int getResFromID(DWORD module, int id, DWORD type, DWORD * offset, DWORD * size);

extern "C" __declspec(dllexport) int getResFromName(DWORD module, const char *name,DWORD type, DWORD * offset, DWORD * size);
