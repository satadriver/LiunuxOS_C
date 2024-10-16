#pragma once


#include "def.h"

extern "C" __declspec(dllexport)int __kClock(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD runparam);