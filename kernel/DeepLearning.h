#pragma once

#include "def.h"
#include "kann-master/kann.h"

#define		TASK_DISPATCH_SAMPLE		(4096)

#define		ML_TASK_LIMIT				16

#define		TASK_DISPATCH_ERROR_RATE	(20.0)		

#pragma pack(1)

struct TaskSwitchSample {
	float tickrate;
	float cpurate;
	float mem;
	float alloc;
	float user;
	float window;
	float delta;
	float priority;
	float authority;
	float sleep;
};

struct TaskPredictParam {

	TaskSwitchSample task[ML_TASK_LIMIT];
	int result;
};

#pragma pack()

#ifdef DLL_EXPORT

extern "C" __declspec(dllexport) TaskPredictParam * g_ml_data;

extern "C" __declspec(dllexport) int g_train_complete;

extern "C" __declspec(dllexport) int g_ml_data_cnt;

extern "C" __declspec(dllexport) double g_dl_rate;

extern "C" __declspec(dllexport) kann_t* g_dl_ann ;

extern "C" __declspec(dllexport) int SaveMlData(TaskPredictParam*);

extern "C" __declspec(dllexport) int __kDeepLearning_rnn(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);

extern "C" __declspec(dllexport) int TaskSwitchPrediction(TaskPredictParam* tp);

#else
extern "C" __declspec(dllimport) TaskPredictParam * g_ml_data;

extern "C" __declspec(dllimport) int g_train_complete;

extern "C" __declspec(dllimport) int g_ml_data_cnt;

extern "C" __declspec(dllimport) double g_dl_rate;

extern "C" __declspec(dllimport) kann_t * g_dl_ann;

extern "C" __declspec(dllimport) int SaveMlData(TaskPredictParam*);

extern "C" __declspec(dllimport) int __kDeepLearning_rnn(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);

extern "C" __declspec(dllimport) int TaskSwitchPrediction(TaskPredictParam* tp);

#endif
