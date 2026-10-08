#pragma once

#include "process.h"
#include "algorithm.h"

#ifdef DLL_EXPORT
extern "C" __declspec(dllexport)  int g_tp_cache;
extern "C" __declspec(dllexport)  unsigned long long g_tp_error1;
extern "C" __declspec(dllexport)  unsigned long long g_tp_error2;
extern "C" __declspec(dllexport)  unsigned long long g_tp_error3;
extern "C" __declspec(dllexport)  unsigned long long g_task_pre_hit;
extern "C" __declspec(dllexport)  unsigned long long g_task_pre_total;
extern "C" __declspec(dllexport)  unsigned long long g_task_pre_cost ;
extern "C" __declspec(dllexport) unsigned long long g_task_other_hit;
extern "C" __declspec(dllexport) unsigned long long g_task_dl_hit ;
#else
extern "C" __declspec(dllimport)  int g_tp_cache;
extern "C" __declspec(dllimport)  unsigned long long g_tp_error1;
extern "C" __declspec(dllimport)  unsigned long long g_tp_error2;
extern "C" __declspec(dllimport)  unsigned long long g_tp_error3;
extern "C" __declspec(dllimport)  unsigned long long g_task_pre_hit;
extern "C" __declspec(dllimport)  unsigned long long g_task_pre_total;
extern "C" __declspec(dllimport)  unsigned long long g_task_pre_cost;
extern "C" __declspec(dllimport) unsigned long long g_task_other_hit;
extern "C" __declspec(dllimport) unsigned long long g_task_dl_hit;
#endif

#define STATIC_PRIORITY				16

#define DYNAMIC_PRIORITY			(4*STATIC_PRIORITY+1)

#define AUTHORITY_PRIORITY			(2*STATIC_PRIORITY+1)

#define WINDOW_PRIORITY				(STATIC_PRIORITY/4)

#define USER_PRIORITY				(STATIC_PRIORITY/4)

#define GRAPH_PRIORITY 				4

#define FILE_PRIORITY				2

#define MOUSE_PRIORITY				1

#define KEYBOARD_PRIORITY			1

#define DELTA_UNIT_PRIORITY			3

#define PREDICTION_PRIORITY			(STATIC_PRIORITY)

#define TASK_PREDICTION_BUF_SIZE	16

unsigned long GetValueFromArray(AlgorithmModel* array, int size, int key);

PROCESS_INFO* GetReadyProcess();

void InitTaskScheduleBuf();

int PredictionTask();