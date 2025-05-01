#pragma once
#include "def.h"

extern "C" __declspec(dllexport)int ScreenVector(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD runparam);