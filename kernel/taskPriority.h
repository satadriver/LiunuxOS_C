#pragma once

#include "process.h"
#include "algorithm.h"

#define STATIC_PRIORITY				16

#define DYNAMIC_PRIORITY			(4*STATIC_PRIORITY+1)

#define AUTHORITY_PRIORITY			(2*STATIC_PRIORITY+1)

#define WINDOW_PRIORITY				(STATIC_PRIORITY/4)

#define USER_PRIORITY				(STATIC_PRIORITY/4)

#define GRAPH_PRIORITY 				4

#define FILE_PRIORITY				2

#define MOUSE_PRIORITY				1

#define KEYBOARD_PRIORITY			1

unsigned long GetValueFromArray(AlgorithmModel* array, int size, int key);

PROCESS_INFO* GetReadyProcess();