#pragma once

#include "def.h"

#pragma pack(1)

struct TaskPredictParam {
	float tick;
	float user;
	float window;
	float delta;
	float priority;
	float status;
	float result;
};

#pragma pack()

extern "C" __declspec(dllexport) TaskPredictParam * g_ml_data ;


int SaveMlData(float tick, float user, float window, float delta, float priority, float status,float result);

extern "C" __declspec(dllexport) int __kMachineLearning(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);