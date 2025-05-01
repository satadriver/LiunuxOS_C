#pragma once
#pragma once
#include "video.h"
#include "def.h"

#define SPIRAL_SMALL_CIRCLE_SIZE	48
#define SPIRAL_SMALL_CIRCLE_SIZE2	36
#define SPIRAL_SMALL_CIRCLE_SIZE3	24
#define AXIS_COLOR					0

extern "C" __declspec(dllexport) int SpiralBall(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD runparam);