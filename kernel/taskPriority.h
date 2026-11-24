#pragma once

#include "process.h"
#include "algorithm.h"

#ifdef DLL_EXPORT
extern "C" __declspec(dllexport)  int g_tp_cache_enable;
extern "C" __declspec(dllexport)  unsigned long long g_tp_error_cnt;
extern "C" __declspec(dllexport) unsigned long long g_tp_corrects[16];
extern "C" __declspec(dllexport)  unsigned long long g_task_pre_cache_hit;
extern "C" __declspec(dllexport)  unsigned long long g_task_pre_total;
extern "C" __declspec(dllexport)  unsigned long long g_task_reason_cost;
extern "C" __declspec(dllexport) unsigned long long g_task_other_hit;
extern "C" __declspec(dllexport) unsigned long long g_task_pre_hit ;
extern "C" __declspec(dllexport) unsigned long long g_task_train_hit;
#else
extern "C" __declspec(dllimport)  int g_tp_cache_enable;
extern "C" __declspec(dllimport)  unsigned long long g_tp_error_cnt;
extern "C" __declspec(dllimport) unsigned long long g_tp_corrects[16];
extern "C" __declspec(dllimport)  unsigned long long g_task_pre_cache_hit;
extern "C" __declspec(dllimport)  unsigned long long g_task_pre_total;
extern "C" __declspec(dllimport)  unsigned long long g_task_reason_cost;
extern "C" __declspec(dllimport) unsigned long long g_task_other_hit;
extern "C" __declspec(dllimport) unsigned long long g_task_pre_hit;
extern "C" __declspec(dllimport) unsigned long long g_task_train_hit;
#endif

//#define MODEL_TYPE_RNN				
//#define MODEL_TYPE_MLP	

#define STATIC_PRIORITY				16

#define DYNAMIC_PRIORITY			(STATIC_PRIORITY)

#define AUTHORITY_PRIORITY			(STATIC_PRIORITY)

#define WINDOW_PRIORITY				(STATIC_PRIORITY)

#define USER_PRIORITY				(STATIC_PRIORITY/2)

#define RATIO_PRIORITY 				8

#define GRAPH_PRIORITY 				STATIC_PRIORITY

#define FILE_PRIORITY				8

#define MOUSE_PRIORITY				4

#define KEYBOARD_PRIORITY			4

#define DELTA_UNIT_PRIORITY			4

#define PREDICTION_PRIORITY			(STATIC_PRIORITY)

#define TASK_PREDICTION_BUF_SIZE	16


#define TASK_SAMPLE_FRACTOR			0X10



double GetCpuRatio(int cpu);

PROCESS_INFO* PrioritySchedule(int sleep);

int PrefetchTaskCache();

void PreMultipleHitRate(int count, int limit, AlgorithmModel* level_buf,  int target_id);

void priorityTaskTest(int count, int cpu, int target_id, int target_idx, LPPROCESS_INFO* tss, AlgorithmModel* level_buf);