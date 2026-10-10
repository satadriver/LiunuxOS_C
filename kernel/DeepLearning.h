#pragma once

#include "def.h"
#include "kann-master/kann.h"

#define		TASK_DISPATCH_SAMPLE		(4096)

#define		ML_TASK_LIMIT				32

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

extern "C" __declspec(dllexport) int g_dl_tp_mix;

extern "C" __declspec(dllexport) int g_dl_lock;

extern "C" __declspec(dllexport) TaskPredictParam * g_dl_data;

extern "C" __declspec(dllexport) int g_dl_train_complete;

extern "C" __declspec(dllexport) int g_dl_data_cnt;

extern "C" __declspec(dllexport) double g_dl_rate;

extern "C" __declspec(dllexport) kann_t* g_dl_ann ;

extern "C" __declspec(dllexport) int CollectDlSample(TaskPredictParam*);

extern "C" __declspec(dllexport) int TaskSchedulePredict(TaskPredictParam* tp);

#else
extern "C" __declspec(dllimport) int g_dl_tp_mix;

extern "C" __declspec(dllimport) int g_dl_lock;

extern "C" __declspec(dllimport) TaskPredictParam * g_dl_data;

extern "C" __declspec(dllimport) int g_dl_train_complete;

extern "C" __declspec(dllimport) int g_dl_data_cnt;

extern "C" __declspec(dllimport) double g_dl_rate;

extern "C" __declspec(dllimport) kann_t * g_dl_ann;

extern "C" __declspec(dllimport) int CollectDlSample(TaskPredictParam*);

extern "C" __declspec(dllimport) int TaskSchedulePredict(TaskPredictParam* tp);

#endif
