#pragma once

#include "def.h"

#pragma pack(1)

struct TaskPredictParam {
	float tick;
	float user;
	float window;
	float delta;
	float priority;
	float result;
};

#pragma pack()

extern "C" __declspec(dllexport) TaskPredictParam * g_ml_data ;

int SaveData(float tick, float user, float window, float delta, float priority);

extern "C" __declspec(dllexport) int __kMachineLearning(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);