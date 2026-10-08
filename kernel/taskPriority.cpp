

#include "def.h"
#include "process.h"
#include "apic.h"
#include "hardware.h"
#include "Utils.h"
#include "descriptor.h"
#include "algorithm.h"
#include "task.h"
#include "cmosAlarm.h"
#include "cmosExactTimer.h"
#include "cmosPeriodTimer.h"
#include "core.h"
#include "device.h"
#include "coprocessor.h"
#include "debugger.h"
#include "Pe.h"
#include "peVirtual.h"
#include "Thread.h"
#include "systemService.h"
#include "deeplearning.h"
#include "apicTimer.h"
#include "taskPriority.h"



int g_tp_cache = 0;

unsigned long long g_tp_error1 = 0;

unsigned long long g_tp_error2 = 0;
unsigned long long g_tp_error3 = 0;

int g_tp_lock[256];

unsigned long long g_task_pre_total = 0;
unsigned long long g_task_pre_hit = 0;
unsigned long long g_task_pre_cost = 0;

unsigned long long g_task_other_hit = 0;
unsigned long long g_task_dl_hit = 0;

LPPROCESS_INFO g_task_predict_buf[256][TASK_PREDICTION_BUF_SIZE];


//double* g_mem_buf[256] ;
//double* g_alloc_buf[256];
AlgorithmModel* g_rate_buf[256];
AlgorithmModel* g_crate_buf[256];
AlgorithmModel* g_level_buf[256];

TaskPredictParam* g_tpp_buf[256];


unsigned long GetValueFromArray(AlgorithmModel* array, int size, int key) {
	for (int i = 0; i < size; i++) {
		if (array[i].id == key) {
			return (unsigned long)array[i].v;
		}
	}
	return 0;
}

void InitTaskScheduleBuf() {
	int cpu = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	int allocSize = TASK_LIMIT_TOTAL * sizeof(double);
	unsigned long alignSize = 0;
	if (g_level_buf[cpu] == 0) {

		allocSize = TASK_LIMIT_TOTAL * sizeof(AlgorithmModel);
		g_level_buf[cpu] = (AlgorithmModel*)__kProcessMalloc(allocSize, &alignSize, 0, cpu, 0,
			PAGE_READWRITE | PAGE_USERPRIVILEGE | PAGE_PRESENT | 0x80000000);
	}
	if (g_crate_buf[cpu] == 0) {
		allocSize = TASK_LIMIT_TOTAL * sizeof(AlgorithmModel);
		g_crate_buf[cpu] = (AlgorithmModel*)__kProcessMalloc(allocSize, &alignSize, 0, cpu, 0,
			PAGE_READWRITE | PAGE_USERPRIVILEGE | PAGE_PRESENT | 0x80000000);
	}
	if (g_rate_buf[cpu] == 0) {
		allocSize = TASK_LIMIT_TOTAL * sizeof(AlgorithmModel);
		g_rate_buf[cpu] = (AlgorithmModel*)__kProcessMalloc(allocSize, &alignSize, 0, cpu, 0,
			PAGE_READWRITE | PAGE_USERPRIVILEGE | PAGE_PRESENT | 0x80000000);
	}

	if (g_tpp_buf[cpu] == 0) {
		allocSize = sizeof(TaskPredictParam);
		g_tpp_buf[cpu] = (TaskPredictParam*)__kProcessMalloc(allocSize, &alignSize, 0, cpu, 0,
			PAGE_READWRITE | PAGE_USERPRIVILEGE | PAGE_PRESENT | 0x80000000);
	}
}

PROCESS_INFO* GetReadyProcess() {

	char szout[256];

	g_task_pre_total++;

	int cpu = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

	LPPROCESS_INFO target_tss = 0;

	int ret = __GetSpinlock(&g_tp_lock[cpu]);
	if (ret) {
		for (int i = TASK_PREDICTION_BUF_SIZE -1; i >= 0; i--) {
			if (g_task_predict_buf[cpu][i]) {
				target_tss = g_task_predict_buf[cpu][i];
				g_task_predict_buf[cpu][i] = 0;
				break;
			}
		}	
		__leaveSpinlock(&g_tp_lock[cpu]);
		if (target_tss) {
			g_task_pre_hit++;
			return target_tss;
		}
	}

	PROCESS_INFO* tss = GetTaskTssBase();
	PROCESS_INFO* process = GetCurrentTaskTssBase();
	LPPROCESS_INFO current = (LPPROCESS_INFO)(tss + process->tid);
	LPPROCESS_INFO ptr = current;
	LPPROCESS_INFO next = 0;

	//AlgorithmModel rate[TASK_LIMIT_TOTAL];
	//AlgorithmModel crate[TASK_LIMIT_TOTAL];
	//int window[TASK_LIMIT_TOTAL];
	//int user[TASK_LIMIT_TOTAL];
	//int sleep[TASK_LIMIT_TOTAL];
	//double mem[TASK_LIMIT_TOTAL];
	//double alloc[TASK_LIMIT_TOTAL];
	//int delta[TASK_LIMIT_TOTAL];
	//AlgorithmModel level[TASK_LIMIT_TOTAL];

	double total_alloc = 0.0;
	double total_mem = 0.0;
	double total_sleep = 0.0;
	int count = 0;
	do {
		ptr++;
		if (ptr - tss >= TASK_LIMIT_TOTAL) {
			ptr = tss;
		}

		if (ptr == 0 || ptr == current) {
			break;
		}

		if (cpu != ptr->cpuid) {
			continue;
		}

		if (ptr->status == TASK_TERMINATE) {
			ptr->status = TASK_OVER;
			continue;
		}
		else if (ptr->status == TASK_RUN) {
			if (ptr->sleep) {
				ptr->sleep--;
			}
			else {
				int dynamic = ptr->delta;
				double ratio = 0.0;
				double cratio = 0.0;
				if (ptr->tick_run == 0 || (ptr->param->cmd & TASK_REALTIME)) {
					dynamic = DYNAMIC_PRIORITY;
					ratio = 0.01;
					cratio = 0.01;
				}
				else {
					double diff = (double)(ptr->tick_total);
					ratio = ((double)ptr->tick_run) / diff;
					diff = g_cpu_tick[cpu];
					cratio = ((double)ptr->tick_run) / (double)diff;
				}
				g_rate_buf[cpu][count].id = ptr->tid;
				g_crate_buf[cpu][count].id = ptr->tid;
				double v = 1.0 / ratio ;
				if (v > STATIC_PRIORITY/2)
					v = STATIC_PRIORITY/2;
				g_rate_buf[cpu][count].v = (unsigned long long) v;
				v = 1.0 / cratio ;
				if (v > STATIC_PRIORITY/2)
					v = STATIC_PRIORITY/2;
				g_crate_buf[cpu][count].v = (unsigned long long) v;

				//window[count] = (ptr->window == 0 ? 0 : WINDOW_PRIORITY);
				//user[count] = (ptr->level == 0 ? USER_PRIORITY : 0);
				//delta[count] = dynamic;
				ptr->delta = dynamic;
				g_level_buf[cpu][count].id = ptr->tid;
				g_level_buf[cpu][count].v = 0;

				//sleep[count] = ptr->sleep;
				//g_mem_buf[cpu][count] = (*ptr->lpvasize);
				//g_alloc_buf[cpu][count] = ptr->alloc_times;
				total_alloc += ptr->alloc_times;
				total_mem += (*ptr->lpvasize);
				total_sleep += ptr->sleep;

				count++;

				if (next == 0) {
					next = ptr;
				}
			}
		}
		else if (ptr->status == TASK_OVER) {
			continue;
		}
		else if (ptr->status == TASK_SUSPEND) {
			continue;
		}
	} while (TRUE);

	if (count == 1) {
		target_tss = next;
		g_task_other_hit++;
	}
	else if (count <= 0) {
		target_tss = current;
		g_task_other_hit++;
	}
	else if (count > 1) {
		int target_id = -1;
		
		for (int i = 0; i < count; i++) {
			int pid = g_level_buf[cpu][i].id;

			double alloc_ratio = tss[pid].alloc_times / total_alloc;
			//g_alloc_buf[cpu][i] = alloc_ratio;

			double mem_ratio = (*tss[pid].lpvasize) / total_mem;
			//g_mem_buf[cpu][i] = mem_ratio;
			
			//if (g_train_complete == 0) 
			{
				g_level_buf[cpu][i].v += g_rate_buf[cpu][i].v;
				g_level_buf[cpu][i].v += g_crate_buf[cpu][i].v;
				int window_priority = (tss[pid].window == 0 ? 0 : WINDOW_PRIORITY);
				int user_priority = (tss[pid].level == 0 ? USER_PRIORITY : 0);
				g_level_buf[cpu][i].v += (window_priority + user_priority);
				g_level_buf[cpu][i].v += tss[pid].delta;
				
				g_level_buf[cpu][i].v += tss[pid].priority;
				g_level_buf[cpu][i].v += tss[pid].authority;
				g_level_buf[cpu][i].v += STATIC_PRIORITY * alloc_ratio;
				g_level_buf[cpu][i].v += STATIC_PRIORITY * mem_ratio;

				g_level_buf[cpu][i].v += (tss[pid].sleep / total_sleep) * STATIC_PRIORITY;
			}
		}	
	
		//if (g_dl_train_complete == 0) 
		{
			QuickSort(g_level_buf[cpu], 0, count - 1);
			target_id = g_level_buf[cpu][count - 1].id;
		}

		//TaskPredictParam tp;
		g_tpp_buf[cpu]->result = -1;

		int num = 0;
		if (count >= ML_TASK_LIMIT) {
			num = ML_TASK_LIMIT;
		}
		else {
			num = count;
		}

		for (int i = 0; i < num; i++) {
			int pid = g_rate_buf[cpu][i].id;
			float tick_ratio = (float)GetValueFromArray(g_rate_buf[cpu], count, pid) / (float)STATIC_PRIORITY;
			float user_ratio = (float)(tss[pid].level == 0 ? USER_PRIORITY : 0) / (float)STATIC_PRIORITY;
			float window_ratio = (float)(tss[pid].window ? WINDOW_PRIORITY : 0) / (float)STATIC_PRIORITY;
			float delta_ratio = (float)tss[pid].delta / (float)STATIC_PRIORITY;
			float priority_ratio = (float)(tss[pid].priority) / (float)STATIC_PRIORITY;
			float authority_r = (float)tss[pid].authority / (float)STATIC_PRIORITY;

			if (pid == target_id) {
				g_tpp_buf[cpu]->result = i;
			}
			g_tpp_buf[cpu]->task[i].tickrate = tick_ratio;
			g_tpp_buf[cpu]->task[i].cpurate = (float)g_crate_buf[cpu][i].fv/ (float)STATIC_PRIORITY;
			g_tpp_buf[cpu]->task[i].user = user_ratio;
			g_tpp_buf[cpu]->task[i].window = window_ratio;
			g_tpp_buf[cpu]->task[i].delta = delta_ratio;
			g_tpp_buf[cpu]->task[i].priority = priority_ratio;
			g_tpp_buf[cpu]->task[i].authority = authority_r;
			g_tpp_buf[cpu]->task[i].sleep = (double)tss[pid].sleep/ (double)total_sleep;
			g_tpp_buf[cpu]->task[i].mem = *tss[pid].lpvasize / total_mem ;
			g_tpp_buf[cpu]->task[i].alloc = tss[pid].alloc_times/total_alloc;
		}

		if (num == ML_TASK_LIMIT) {
			if (count != ML_TASK_LIMIT) {
				__printf("%s %d counter:%d exceed\r\n", __FUNCTION__, __LINE__, count);
				int index = -1;
				for (int i = 0; i < count; i++) {
					if (g_rate_buf[cpu][i].id == target_id) {
						index = i;
						break;
					}
				}
				if (index >= ML_TASK_LIMIT) {
					int pid = g_rate_buf[cpu][index].id;
					float tick_ratio = (float)GetValueFromArray(g_rate_buf[cpu], count, pid) / (float)STATIC_PRIORITY;
					float user_ratio = (float)(tss[pid].level == 0 ? USER_PRIORITY : 0) / (float)STATIC_PRIORITY;
					float window_ratio = (float)(tss[pid].window ? WINDOW_PRIORITY : 0) / (float)STATIC_PRIORITY;
					float delta_ratio = (float)tss[pid].delta / (float)STATIC_PRIORITY;
					float priority_ratio = (float)(tss[pid].priority) / (float)STATIC_PRIORITY;
					float authority_r = (float)tss[pid].authority / (float)STATIC_PRIORITY;

					int ri = __random(0) % ML_TASK_LIMIT;
					g_tpp_buf[cpu]->task[ri].tickrate = tick_ratio;
					g_tpp_buf[cpu]->task[ri].cpurate = (float)g_crate_buf[cpu][index].fv / (float)STATIC_PRIORITY;
					g_tpp_buf[cpu]->task[ri].user = user_ratio;
					g_tpp_buf[cpu]->task[ri].window = window_ratio;
					g_tpp_buf[cpu]->task[ri].delta = delta_ratio;
					g_tpp_buf[cpu]->task[ri].priority = priority_ratio;
					g_tpp_buf[cpu]->task[ri].authority = authority_r;
					g_tpp_buf[cpu]->task[ri].sleep = (double)tss[pid].sleep / (double)total_sleep;
					g_tpp_buf[cpu]->task[ri].mem = (double)(*tss[pid].lpvasize) / (double)total_mem;
					g_tpp_buf[cpu]->task[ri].alloc = (double)tss[pid].alloc_times / (double)total_alloc;
					g_tpp_buf[cpu]->result = ri;

					g_rate_buf[cpu][ri].id = pid;
					g_rate_buf[cpu][ri].v = g_rate_buf[cpu][index].v;
				}
			}
		}
		else {
			for (int i = num; i < ML_TASK_LIMIT; i++) {
				g_tpp_buf[cpu]->task[i].tickrate = 1.0;
				g_tpp_buf[cpu]->task[i].cpurate = 1.0;
				g_tpp_buf[cpu]->task[i].user = 0.0;
				g_tpp_buf[cpu]->task[i].window = 0.0;
				g_tpp_buf[cpu]->task[i].delta = 0.0;
				g_tpp_buf[cpu]->task[i].priority = 0.0;
				g_tpp_buf[cpu]->task[i].authority = 0.0;
				g_tpp_buf[cpu]->task[i].sleep = 1.0;
				g_tpp_buf[cpu]->task[i].mem = 0.0;
				g_tpp_buf[cpu]->task[i].alloc = 0.0;
			}
		}

		if (g_dl_train_complete == 0) {
			CollectDlSample(g_tpp_buf[cpu]);
			g_task_other_hit++;
		}
		else {
			int old_h1 = g_level_buf[cpu][count - 1].id;
			int old_h2 = g_level_buf[cpu][count - 2].id;

			int seq = TaskSchedulePredict(g_tpp_buf[cpu]);
			if (seq >= 0 && seq < count) {
				if (g_dl_tp_mix) {
					int id = g_rate_buf[cpu][seq].id;
					for (int i = 0; i < count; i++) {
						if (id == g_level_buf[cpu][i].id) {
							g_level_buf[cpu][i].v += PREDICTION_PRIORITY;
							break;
						}
					}
					QuickSort(g_level_buf[cpu], 0, count - 1);
					target_id = g_level_buf[cpu][count - 1].id;
				}
				else {
					target_id = g_rate_buf[cpu][seq].id;
				}

				if (old_h1 != target_id){
					g_tp_error1++;
					if (count >= 2 && target_id != old_h2) {
						g_tp_error2++;
					}
				}
				g_task_dl_hit++;
			}
			else {
				target_id = next->tid;	
				g_task_other_hit++;
				g_tp_error3++;
			}
		}

		for (int i = 0; i < count; i++) {
			int tid = g_rate_buf[cpu][i].id;
			if (target_id != g_rate_buf[cpu][i].id) {
				tss[tid].delta += DELTA_UNIT_PRIORITY;
				if (tss[tid].delta > DYNAMIC_PRIORITY) {
					tss[tid].delta = DYNAMIC_PRIORITY;
				}
			}
			else {
				tss[tid].delta = 0;
			}
		}

		target_tss = tss + target_id;
	}

	target_tss->delta = 0;
	target_tss->authority = target_tss->authority/2;

	return target_tss;
}






int PredictionTask() {

	if (g_tp_cache == 0) {
		return 0;
	}

	char szout[256];
	if (g_dl_train_complete == 0 || g_dl_rate == 0.0) {
		return 0;
	}
	LPPROCESS_INFO target_tss = 0;
	PROCESS_INFO* tss = GetTaskTssBase();
	PROCESS_INFO* process = GetCurrentTaskTssBase();
	LPPROCESS_INFO current = (LPPROCESS_INFO)(tss + process->tid);
	LPPROCESS_INFO ptr = current;
	LPPROCESS_INFO next = 0;

	AlgorithmModel rate[TASK_LIMIT_TOTAL];
	AlgorithmModel crate[TASK_LIMIT_TOTAL];
	int cpu = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

	//int window[TASK_LIMIT_TOTAL];
	//int user[TASK_LIMIT_TOTAL];
	//double sleep[TASK_LIMIT_TOTAL];
	//double mem[TASK_LIMIT_TOTAL];
	//double alloc[TASK_LIMIT_TOTAL];
	//int delta[TASK_LIMIT_TOTAL];

	double total_alloc = 0.0;
	double total_mem = 0.0;
	double total_sleep = 0.0;
	//__asm{cli}
	__enterSpinlock(&g_tp_lock[cpu]);

	int total = 0;

	for (int num = 0; num < TASK_PREDICTION_BUF_SIZE; num++) {

		if (g_task_predict_buf[cpu][num] == 0) {

			int count = 0;
			do {
				ptr++;
				if (ptr - tss >= TASK_LIMIT_TOTAL) {
					ptr = tss;
				}

				if (ptr == 0 || ptr == current) {
					break;
				}

				if (cpu != ptr->cpuid) {
					continue;
				}

				if (ptr->status == TASK_TERMINATE) {
					ptr->status = TASK_OVER;
					continue;
				}
				else if (ptr->status == TASK_RUN) {
					if (ptr->sleep) {
						//ptr->sleep--;
					}
					else {
						int dynamic = ptr->delta;
						double ratio = 0.0;
						double cratio = 0.0;
						if (ptr->tick_run == 0 || (ptr->param->cmd & TASK_REALTIME)) {
							dynamic = DYNAMIC_PRIORITY;
							ratio = 0.01;
							cratio = 0.01;
						}
						else {
							double diff = (double)(ptr->tick_total);
							ratio = ((double)ptr->tick_run) / diff;
							diff = g_cpu_tick[cpu];
							cratio = ((double)ptr->tick_run) / (double)diff;
						}
						rate[count].id = ptr->tid;
						crate[count].id = ptr->tid;
						double v = 1.0 / ratio;
						if (v > STATIC_PRIORITY / 2)
							v = STATIC_PRIORITY / 2;
						rate[count].v = (unsigned long long) v;
						v = 1.0 / cratio;
						if (v > STATIC_PRIORITY / 2)
							v = STATIC_PRIORITY / 2;
						crate[count].v = (unsigned long long) v;

						ptr->delta = dynamic;

						total_alloc += ptr->alloc_times;
						total_mem += (*ptr->lpvasize);
						total_sleep += ptr->sleep;

						count++;

						if (next == 0) {
							next = ptr;
						}
					}
				}
				else if (ptr->status == TASK_OVER) {
					continue;
				}
				else if (ptr->status == TASK_SUSPEND) {
					continue;
				}
			} while (TRUE);

			if (count == 1) {
				target_tss = next;
			}
			else if (count <= 0) {
				target_tss = current;
			}
			else if (count > 1) {
				int target_id = -1;
				TaskPredictParam tp;
				tp.result = -1;

				int num = 0;
				if (count >= ML_TASK_LIMIT) {
					num = ML_TASK_LIMIT;
				}
				else {
					num = count;
				}

				for (int i = 0; i < num; i++) {
					int pid = rate[i].id;
					float tick_ratio = (float)GetValueFromArray(rate, count, pid) / (float)STATIC_PRIORITY;
					float user_ratio = (float)(tss[pid].level == 0 ? USER_PRIORITY : 0) / (float)STATIC_PRIORITY;
					float window_ratio = (float)(tss[pid].window ? WINDOW_PRIORITY : 0) / (float)STATIC_PRIORITY;
					float delta_ratio = (float)tss[pid].delta / (float)STATIC_PRIORITY;
					float priority_ratio = (float)(tss[pid].priority) / (float)STATIC_PRIORITY;
					float authority_r = (float)tss[pid].authority / (float)STATIC_PRIORITY;

					tp.task[i].tickrate = tick_ratio;
					tp.task[i].cpurate = crate[i].fv / (float)STATIC_PRIORITY;
					tp.task[i].user = user_ratio;
					tp.task[i].window = window_ratio;
					tp.task[i].delta = delta_ratio;
					tp.task[i].priority = priority_ratio;
					tp.task[i].authority = authority_r;
					tp.task[i].sleep = tss[pid].sleep/total_sleep;
					tp.task[i].mem = *(tss[pid].lpvasize) / (double)total_mem;
					tp.task[i].alloc = tss[pid].alloc_times / total_alloc;
				}

				if (num == ML_TASK_LIMIT) {
					if (count != ML_TASK_LIMIT) {
						__printf("%s %d counter:%d exceed\r\n", __FUNCTION__, __LINE__, count);
						AlgorithmModel level[TASK_LIMIT_TOTAL];
						for (int i = 0; i < count; i++) {
							int pid = rate[i].id;
							level[i].id = rate[i].id;
							level[i].v += rate[i].v;
							level[i].v += crate[i].v;
							int windowlevel = (tss[pid].window == 0 ? 0 : WINDOW_PRIORITY);
							int userlevel = (tss[pid].level == 0 ? USER_PRIORITY : 0);
							level[i].v += (windowlevel + userlevel);
	
							level[i].v += tss[pid].delta;
							level[i].v += tss[pid].priority;
							level[i].v += tss[pid].authority;
							level[i].v += STATIC_PRIORITY * (*(tss[pid].lpvasize) / (double)total_mem);
							level[i].v += STATIC_PRIORITY * (tss[pid].alloc_times / total_alloc);
							level[i].v += (tss[pid].sleep / total_sleep) * STATIC_PRIORITY;
						}
						QuickSort(level, 0, count - 1);
						target_id = level[count - 1].id;

						int index = -1;
						for (int i = 0; i < count; i++) {
							if (rate[i].id == target_id) {
								index = i;
								break;
							}
						}
						if (index >= ML_TASK_LIMIT) {
							int pid = rate[index].id;
							float tick_ratio = (float)GetValueFromArray(rate, count, pid) / (float)STATIC_PRIORITY;
							float user_ratio = (float)(tss[pid].level == 0 ? USER_PRIORITY : 0) / (float)STATIC_PRIORITY;
							float window_ratio = (float)(tss[pid].window ? WINDOW_PRIORITY : 0) / (float)STATIC_PRIORITY;
							float delta_ratio = (float)tss[pid].delta / (float)STATIC_PRIORITY;
							float priority_ratio = (float)(tss[pid].priority) / (float)STATIC_PRIORITY;
							float authority_r = (float)tss[pid].authority / (float)STATIC_PRIORITY;

							int ri = __random(0) % ML_TASK_LIMIT;
							tp.task[ri].tickrate = tick_ratio;
							tp.task[ri].cpurate = crate[index].fv / (float)STATIC_PRIORITY;
							tp.task[ri].user = user_ratio;
							tp.task[ri].window = window_ratio;
							tp.task[ri].delta = delta_ratio;
							tp.task[ri].priority = priority_ratio;
							tp.task[ri].authority = authority_r;
							tp.task[ri].sleep = tss[pid].sleep/ total_sleep;
							tp.task[ri].mem = (*(tss[pid].lpvasize) / (double)total_mem);
							tp.task[ri].alloc = (tss[pid].alloc_times / total_alloc);
							tp.result = ri;

							rate[ri].id = pid;
							rate[ri].v = rate[index].v;
						}
					}
				}
				else {
					for (int i = num; i < ML_TASK_LIMIT; i++) {
						tp.task[i].tickrate = 1.0;
						tp.task[i].cpurate = 1.0;
						tp.task[i].user = 0.0;
						tp.task[i].window = 0.0;
						tp.task[i].delta = 0.0;
						tp.task[i].priority = 0.0;
						tp.task[i].authority = 0.0;
						tp.task[i].sleep = 1.0;
						tp.task[i].mem = 0.0;
						tp.task[i].alloc = 0.0;
					}
				}		
				
				int seq = TaskSchedulePredict(&tp);
				if (seq >= 0 && seq < count) {
					target_id = rate[seq].id;
					total++;
					//g_tp_error1++;
				}
				else {
					target_id = next->tid;		
					//g_tp_error2++;
				}
				target_tss = tss + target_id;

				for (int i = 0; i < count; i++) {
					int tid = rate[i].id;
					if (target_id != rate[i].id) {
						tss[tid].delta += DELTA_UNIT_PRIORITY;
						if (tss[tid].delta > DYNAMIC_PRIORITY) {
							tss[tid].delta = DYNAMIC_PRIORITY;
						}
					}
					else {
						tss[tid].delta = 0;
					}
				}
			}

			target_tss->delta = 0;
			target_tss->authority = target_tss->authority / 2;

			g_task_predict_buf[cpu][num] = target_tss;
		}
	}

	__leaveSpinlock(&g_tp_lock[cpu]);
	//__asm{sti}
	return total;
}