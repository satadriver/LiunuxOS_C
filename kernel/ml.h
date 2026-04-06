#pragma once

#include "def.h"

#pragma pack(1)

struct TaskSwitchSample {
	float tick;
	float user;
	float window;
	float delta;
	float priority;
};

struct TaskPredictParam {

	TaskSwitchSample task[16];
	int result;
};

#pragma pack()

extern "C" __declspec(dllexport) TaskPredictParam * g_ml_data ;


int SaveMlData(TaskPredictParam*);

extern "C" __declspec(dllexport) int __kMachineLearning(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);

extern "C" __declspec(dllexport) int TestThread3(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);
extern "C" __declspec(dllexport) int TestThread2(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);

extern "C" __declspec(dllexport) int TestThread1(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);