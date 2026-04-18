#pragma once

#include "def.h"

#define		TASK_PREDICTION_TRAIN	(4096)

#define ML_TASK_LIMIT				16

#pragma pack(1)

struct TaskSwitchSample {
	float tick;
	float user;
	float window;
	float delta;
	float priority;
	float authority;
};

struct TaskPredictParam {

	TaskSwitchSample task[ML_TASK_LIMIT];
	int result;
};

#pragma pack()

extern "C" __declspec(dllexport) TaskPredictParam * g_ml_data ;


int SaveMlData(TaskPredictParam*);
extern "C" __declspec(dllexport) int __kMachineLearning_rnn(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);

int TaskSwitchPrediction(TaskPredictParam* tp);
extern "C" __declspec(dllexport) int __kMachineLearning_mlp(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);

extern "C" __declspec(dllexport) int TestThread3(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);
extern "C" __declspec(dllexport) int TestThread2(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);

extern "C" __declspec(dllexport) int TestThread1(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);

extern "C" __declspec(dllexport) int TestThread0(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);
extern "C" __declspec(dllexport) int TestThread4(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);

extern "C" __declspec(dllexport) int TestThread5(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);
extern "C" __declspec(dllexport) int TestThread6(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);
extern "C" __declspec(dllexport) int TestThread7(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);

extern "C" __declspec(dllexport) int TestThread8(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);
extern "C" __declspec(dllexport) int TestThread9(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);
extern "C" __declspec(dllexport) int TestThread10(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);

extern "C" __declspec(dllexport) int TestThread11(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);
extern "C" __declspec(dllexport) int TestThread12(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);
extern "C" __declspec(dllexport) int TestThread13(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);

extern "C" __declspec(dllexport) int TestThread14(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);
extern "C" __declspec(dllexport) int TestThread15(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);
