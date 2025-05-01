#pragma once

#include "def.h"
#define OUTPUT_INFO_COLOR	0Xff0000

extern "C" __declspec(dllexport) int TrajectoryBall(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD runparam);