

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

//#define PREDICTION_DEEPLEARNING

#define TASK_PREDICTION_BUF_SIZE	16

int g_tp_debug = 0;

int g_tp_lock[256];

unsigned long long g_task_pre_total = 0;
unsigned long long g_task_pre_hit = 0;
unsigned long long g_task_pre_cost = 0;

unsigned long long g_task_other_hit = 0;
unsigned long long g_task_dl_hit = 0;


LPPROCESS_INFO g_prediction_task[256][TASK_PREDICTION_BUF_SIZE];



unsigned long GetValueFromArray(AlgorithmModel* array, int size, int key) {
	for (int i = 0; i < size; i++) {
		if (array[i].id == key) {
			return (unsigned long)array[i].v;
		}
	}
	return 0;
}



PROCESS_INFO* GetReadyProcess() {

	char szout[256];

	g_task_pre_total++;

	int cpu = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	LPPROCESS_INFO target_tss = 0;

	int ret = __GetSpinlock(&g_tp_lock[cpu]);
	if (ret) {
		for (int i = TASK_PREDICTION_BUF_SIZE -1; i >= 0; i--) {
			if (g_prediction_task[cpu][i]) {
				target_tss = g_prediction_task[cpu][i];
				g_prediction_task[cpu][i] = 0;
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

	AlgorithmModel rate[TASK_LIMIT_TOTAL];
	AlgorithmModel crate[TASK_LIMIT_TOTAL];
	
	int window[TASK_LIMIT_TOTAL];

	int user[TASK_LIMIT_TOTAL];
	int sleep[TASK_LIMIT_TOTAL];

	double mem[TASK_LIMIT_TOTAL];

	double alloc[TASK_LIMIT_TOTAL];

	AlgorithmModel delta[TASK_LIMIT_TOTAL];

	AlgorithmModel level[TASK_LIMIT_TOTAL];

	double total_malloc = 0.0;

	double total_mem = 0.0;

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
					ratio = 1.0;
					cratio = 1.0;
				}
				else {
					double diff = (double)(ptr->tick_total);
					ratio = ((double)ptr->tick_run) / diff;
					diff = g_cpu_tick[cpu];
					cratio = ((double)ptr->tick_run) / (double)diff;
				}
				rate[count].id = ptr->tid;
				crate[count].id = ptr->tid;
				double v = 1.0 / ratio ;
				if (v > STATIC_PRIORITY/2)
					v = STATIC_PRIORITY/2;
				rate[count].v = (unsigned long long) v;
				v = 1.0 / cratio ;
				if (v > STATIC_PRIORITY/2)
					v = STATIC_PRIORITY/2;
				crate[count].v = (unsigned long long) v;
				//__printf(szout, "tick_start:%lf, diff:%i64x,tick:%I64x, ratio:%lf\r\n",rate[count].v, ptr->tick_total, ptr->tick_run, ratio);
				window[count] = (ptr->window == 0 ? 0 : WINDOW_PRIORITY);

				user[count] = (ptr->level == 0 ? USER_PRIORITY : 0);

				delta[count].v = dynamic;
				delta[count].id = ptr->tid;

				level[count].id = ptr->tid;
				level[count].v = 0;

				sleep[count] = ptr->sleep;

				mem[count] = (*ptr->lpvasize);
				alloc[count] = ptr->alloc_times;

				total_malloc += ptr->alloc_times;

				total_mem += (*ptr->lpvasize);

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
		//__printf(szout, "%s %d count:%d\r\n", __FUNCTION__, __LINE__);
	}
	else if (count > 1) {
		int target_id = -1;
		
		for (int i = 0; i < count; i++) {
			double alloc_ratio =  alloc[i] / total_malloc;
			alloc[i] = alloc_ratio;		

			double mem_ratio = mem[i] / total_mem;
			mem[i] = mem_ratio;
			
			//if (g_train_complete == 0) 
			{
				level[i].v += rate[i].v;
				level[i].v += crate[i].v;

				level[i].v += (window[i] + user[i]);
				int pid = level[i].id;
				level[i].v += delta[i].v;
				level[i].v += tss[pid].priority;
				level[i].v += tss[pid].authority;
				level[i].v += STATIC_PRIORITY * alloc_ratio;
				level[i].v += STATIC_PRIORITY * mem_ratio;
			}
		}	
	
		if (g_train_complete == 0) 
		{
			QuickSort(level, 0, count - 1);
			target_id = level[count - 1].id;
			target_tss = tss + target_id;
		}
		//extern int g_train_complete;

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
			float delta_ratio = (float)GetValueFromArray(delta, count, pid) / (float)STATIC_PRIORITY;
			float priority_ratio = (float)(tss[pid].priority) / (float)STATIC_PRIORITY;
			float authority_r = (float)tss[pid].authority / (float)STATIC_PRIORITY;

			if (pid == target_id) {
				tp.result = i;
			}
			tp.task[i].tickrate = tick_ratio;
			tp.task[i].cpurate = crate[i].fv;
			tp.task[i].user = user_ratio;
			tp.task[i].window = window_ratio;
			tp.task[i].delta = delta_ratio;
			tp.task[i].priority = priority_ratio;
			tp.task[i].authority = authority_r;
			tp.task[i].sleep = tss[pid].sleep;
			tp.task[i].mem = mem[i];
			tp.task[i].alloc = alloc[i];
		}

		if (num == ML_TASK_LIMIT) {
			if (count != ML_TASK_LIMIT) {
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
					float delta_ratio = (float)GetValueFromArray(delta, count, pid) / (float)STATIC_PRIORITY;
					float priority_ratio = (float)(tss[pid].priority) / (float)STATIC_PRIORITY;
					float authority_r = (float)tss[pid].authority / (float)STATIC_PRIORITY;

					int ri = __random(0) % ML_TASK_LIMIT;
					tp.task[ri].tickrate = tick_ratio;
					tp.task[ri].cpurate = crate[index].fv;
					tp.task[ri].user = user_ratio;
					tp.task[ri].window = window_ratio;
					tp.task[ri].delta = delta_ratio;
					tp.task[ri].priority = priority_ratio;
					tp.task[ri].authority = authority_r;
					tp.task[ri].sleep = tss[pid].sleep;
					tp.task[ri].mem = mem[index];
					tp.task[ri].alloc = alloc[index];
					tp.result = ri;

					rate[ri].id = pid;
					rate[ri].v = rate[index].v;
				}
			}
		}
		else {
			for (int i = num; i < ML_TASK_LIMIT; i++) {
				tp.task[i].tickrate = 0.0;
				tp.task[i].cpurate = 0.0;
				tp.task[i].user = 0.0;
				tp.task[i].window = 0.0;
				tp.task[i].delta = 0.0;
				tp.task[i].priority = 0.0;
				tp.task[i].authority = 0.0;
				tp.task[i].sleep = -1.0;
				tp.task[i].mem = 0.0;
				tp.task[i].alloc = 0.0;
			}
		}

		if (g_train_complete == 0) {
			SaveMlData(&tp);
		}
		else {
			int seq = TaskSwitchPrediction(&tp);
			if (seq >= 0 && seq < count) {
#ifdef PREDICTION_DEEPLEARNING
				target_id = rate[seq].id;
#else
				int id = rate[seq].id;
				for (int i = 0; i < count; i++) {
					if (id == level[i].id) {
						level[i].v += PREDICTION_PRIORITY;
						break;
					}
				}
				QuickSort(level, 0, count - 1);
				target_id = level[count - 1].id;
				target_tss = tss + target_id;
				g_task_dl_hit++;
#endif
				if (g_tp_debug++ % 0x100 == 0) {
					LPPROCESS_INFO p = GetTaskTssBaseId(cpu);
					LPPROCESS_INFO tp = p + target_id;
					//__printf(szout, "%s %d seq:%d,count:%d, tid:%x, cpu:%x, function:%s, cost:%i64x\r\n",
					//	__FUNCTION__,__LINE__,seq, count, target_id, cpu, tp->funcname, g_task_pre_cost);
				}
			}
			else {
				target_id = next->tid;	
				//__printf(szout, "%s %d seq:%d,count:%d error,cost:%i64x\r\n", 
				//	__FUNCTION__, __LINE__, seq, count, g_task_pre_cost);
				g_task_other_hit++;
			}

			target_tss = tss + target_id;
		}

		for (int i = 0; i < count; i++) {
			int tid = delta[i].id;
			if (target_id != delta[i].id) {
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
	target_tss->authority = target_tss->authority/2;

	return target_tss;
}






int PredictionTask() {

	//return 0;

	char szout[256];
	if (g_train_complete == 0 || g_dl_rate == 0.0) {
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

	int window[TASK_LIMIT_TOTAL];
	int authority[TASK_LIMIT_TOTAL];

	int user[TASK_LIMIT_TOTAL];
	int sleep[TASK_LIMIT_TOTAL];

	double mem[TASK_LIMIT_TOTAL];

	double alloc[TASK_LIMIT_TOTAL];

	AlgorithmModel delta[TASK_LIMIT_TOTAL];

	double total_malloc = 0.0;

	double total_mem = 0.0;

	//__asm{cli}
	__enterSpinlock(&g_tp_lock[cpu]);

	int total = 0;

	for (int num = 0; num < TASK_PREDICTION_BUF_SIZE; num++) {

		if (g_prediction_task[cpu][num] == 0) {
			total++;

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
							ratio = 1.0;
							cratio = 1.0;
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

						window[count] = (ptr->window == 0 ? 0 : WINDOW_PRIORITY);

						user[count] = (ptr->level == 0 ? USER_PRIORITY : 0);

						delta[count].v = dynamic;
						delta[count].id = ptr->tid;

						sleep[count] = ptr->sleep;

						mem[count] = (*ptr->lpvasize);
						alloc[count] = ptr->alloc_times;

						total_malloc += ptr->alloc_times;

						total_mem += (*ptr->lpvasize);

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
				//__printf(szout, "%s %d count:%d\r\n", __FUNCTION__, __LINE__);
			}
			else if (count > 1) {

				for (int i = 0; i < count; i++) {
					double alloc_ratio = alloc[i] / total_malloc;
					alloc[i] = alloc_ratio;

					double mem_ratio = mem[i] / total_mem;
					mem[i] = mem_ratio;
				}

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
					float delta_ratio = (float)GetValueFromArray(delta, count, pid) / (float)STATIC_PRIORITY;
					float priority_ratio = (float)(tss[pid].priority) / (float)STATIC_PRIORITY;
					float authority_r = (float)tss[pid].authority / (float)STATIC_PRIORITY;

					tp.task[i].tickrate = tick_ratio;
					tp.task[i].cpurate = crate[i].fv;
					tp.task[i].user = user_ratio;
					tp.task[i].window = window_ratio;
					tp.task[i].delta = delta_ratio;
					tp.task[i].priority = priority_ratio;
					tp.task[i].authority = authority_r;
					tp.task[i].sleep = tss[pid].sleep;
					tp.task[i].mem = mem[i];
					tp.task[i].alloc = alloc[i];
				}

				if (num == ML_TASK_LIMIT) {
					if (count != ML_TASK_LIMIT) {
						//__printf("%s %d counter:%d exceed\r\n", __FUNCTION__, __LINE__, count);
						AlgorithmModel level[TASK_LIMIT_TOTAL];
						for (int i = 0; i < count; i++) {
							level[i].id = rate[i].id;
							level[i].v += rate[i].v;
							level[i].v += crate[i].v;

							level[i].v += (window[i] + user[i]);
							int pid = level[i].id;
							level[i].v += delta[i].v;
							level[i].v += tss[pid].priority;
							level[i].v += tss[pid].authority;
							level[i].v += STATIC_PRIORITY * mem[i];
							level[i].v += STATIC_PRIORITY * alloc[i];
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
							float delta_ratio = (float)GetValueFromArray(delta, count, pid) / (float)STATIC_PRIORITY;
							float priority_ratio = (float)(tss[pid].priority) / (float)STATIC_PRIORITY;
							float authority_r = (float)tss[pid].authority / (float)STATIC_PRIORITY;

							int ri = __random(0) % ML_TASK_LIMIT;
							tp.task[ri].tickrate = tick_ratio;
							tp.task[ri].cpurate = crate[index].fv;
							tp.task[ri].user = user_ratio;
							tp.task[ri].window = window_ratio;
							tp.task[ri].delta = delta_ratio;
							tp.task[ri].priority = priority_ratio;
							tp.task[ri].authority = authority_r;
							tp.task[ri].sleep = tss[pid].sleep;
							tp.task[ri].mem = mem[index];
							tp.task[ri].alloc = alloc[index];
							tp.result = ri;

							rate[ri].id = pid;
							rate[ri].v = rate[index].v;
						}
					}
				}
				else {
					for (int i = num; i < ML_TASK_LIMIT; i++) {
						tp.task[i].tickrate = 0.0;
						tp.task[i].cpurate = 0.0;
						tp.task[i].user = 0.0;
						tp.task[i].window = 0.0;
						tp.task[i].delta = 0.0;
						tp.task[i].priority = 0.0;
						tp.task[i].authority = 0.0;
						tp.task[i].sleep = -1.0;
						tp.task[i].mem = 0.0;
						tp.task[i].alloc = 0.0;
					}
				}		
				
				int seq = TaskSwitchPrediction(&tp);

				if (seq >= 0 && seq < count) {
					target_id = rate[seq].id;
					//g_task_dl_hit++;
					if (g_tp_debug++ % 0x100 == 0) {
						LPPROCESS_INFO p = GetTaskTssBaseId(cpu);
						LPPROCESS_INFO tp = p + target_id;
						//__printf(szout, "%s %d seq:%d,count:%d, tid:%x, cpu:%x, function:%s, cost:%i64x\r\n",
						//	__FUNCTION__, __LINE__, seq, count, target_id, cpu, tp->funcname, g_task_pre_cost);
					}
				}
				else {
					//__printf(szout, "%s %d seq:%d,count:%d, error,cost:%i64x\r\n",
					//	__FUNCTION__, __LINE__, seq, count, g_task_pre_cost);
					target_id = next->tid;
					
				}

				target_tss = tss + target_id;

				for (int i = 0; i < count; i++) {
					int tid = delta[i].id;
					if (target_id != delta[i].id) {
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

			g_prediction_task[cpu][num] = target_tss;
		}
	}

	__leaveSpinlock(&g_tp_lock[cpu]);
	//__asm{sti}
	return total;
}