

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


int g_debug_tag = 0;

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

	LPPROCESS_INFO target_tss = 0;
	PROCESS_INFO* tss = GetTaskTssBase();
	PROCESS_INFO* process = GetCurrentTaskTssBase();
	LPPROCESS_INFO current = (LPPROCESS_INFO)(tss + process->tid);
	LPPROCESS_INFO ptr = current;
	LPPROCESS_INFO next = 0;

	int cpu = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

	int window[TASK_LIMIT_TOTAL];
	int user[TASK_LIMIT_TOTAL];
	int sleep[TASK_LIMIT_TOTAL];
	AlgorithmModel rate[TASK_LIMIT_TOTAL];
	AlgorithmModel delta[TASK_LIMIT_TOTAL];
	AlgorithmModel level[TASK_LIMIT_TOTAL];

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
				if (ptr->tick_run == 0 || (ptr->param->cmd & TASK_REALTIME)) {
					dynamic = DYNAMIC_PRIORITY;
					ratio = 1.0;
					if (ptr->param->cmd & TASK_REALTIME) {
						ptr->authority = AUTHORITY_PRIORITY;
						ptr->priority = STATIC_PRIORITY;
						ptr->delta = DYNAMIC_PRIORITY;
					}
				}
				else {
					double diff = (double)(ptr->tick_total);
					ratio = ((double)ptr->tick_run) / diff;
					if (ratio > 0.9)
					{
						//ratio = 0.01;
					}
				}
				rate[count].id = ptr->tid;

				double v = ratio * (double)STATIC_PRIORITY;
				rate[count].v = (unsigned long long) v;

				if (g_debug_tag++ % 0x1000 == 0x1000) {
					__printf(szout, "tick_start:%lf, diff:%i64x,tick:%I64x, ratio:%lf\r\n",
						rate[count].v, ptr->tick_total, ptr->tick_run, ratio);
				}
				sleep[count] = (ptr->sleep );
				window[count] = (ptr->window == 0 ? 0 : WINDOW_PRIORITY);

				user[count] = (ptr->level == 0 ? USER_PRIORITY : 0);

				delta[count].v = dynamic;
				delta[count].id = ptr->tid;

				level[count].id = ptr->tid;
				level[count].v = 0;

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
		int target_id = 0;

		//QuickSort(tickc, 0, count - 1);
		//for (int i = 0; i < count; i++) {
		//	tickc[i].v = STATIC_PRIORITY / (count - i);
		//}
	
		for (int i = 0; i < count; i++) {
			int pid = level[i].id;
			level[i].v += rate[i].v;

			level[i].v += (window[i] + user[i]);
			
			level[i].v += delta[i].v;
			level[i].v += tss[pid].priority;
			level[i].v += tss[pid].authority;
		}

		QuickSort(level, 0, count - 1);
		target_id = level[count - 1].id;
		target_tss = tss + target_id;

		extern int g_train_complete;

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
			tp.task[i].usage = tick_ratio;
			tp.task[i].user = user_ratio;
			tp.task[i].window = window_ratio;
			tp.task[i].delta = delta_ratio;
			tp.task[i].priority = priority_ratio;
			tp.task[i].authority = authority_r;
			tp.task[i].sleep = (float)(tss[pid].sleep)*1.0;
		}

		if (num == ML_TASK_LIMIT) {
			int index = -1;
			for (int i = 0; i < count; i++) {
				if (rate[i].id == target_id) {
					index = i;
					break;
				}
			}
			
			if (index >= ML_TASK_LIMIT) {
				int pid = target_id;
				float tick_ratio = (float)GetValueFromArray(rate, count, pid) / (float)STATIC_PRIORITY;
				float user_ratio = (float)(tss[pid].level == 0 ? USER_PRIORITY : 0) / (float)STATIC_PRIORITY;
				float window_ratio = (float)(tss[pid].window ? WINDOW_PRIORITY : 0) / (float)STATIC_PRIORITY;
				float delta_ratio = (float)GetValueFromArray(delta, count, pid) / (float)STATIC_PRIORITY;
				float priority_ratio = (float)(tss[pid].priority) / (float)STATIC_PRIORITY;
				float authority_r = (float)tss[pid].authority / (float)STATIC_PRIORITY;

				int ri = __random(0) % ML_TASK_LIMIT;
				tp.task[ri].usage = tss[pid].tick_run/ tss[pid].tick_total;
				tp.task[ri].user = user_ratio;
				tp.task[ri].window = window_ratio;
				tp.task[ri].delta = delta_ratio;
				tp.task[ri].priority = priority_ratio;
				tp.task[ri].authority = authority_r;
				tp.task[ri].sleep = (float)(tss[pid].sleep) * 1.0;
				tp.result = ri;
			}
			else {

			}
		}
		else {
			for (int i = num; i < ML_TASK_LIMIT; i++) {
				tp.task[i].usage = 0.0;
				tp.task[i].user = 0.0;
				tp.task[i].window = 0.0;
				tp.task[i].delta = 0.0;
				tp.task[i].priority = 0.0;
				tp.task[i].authority = 0.0;
				tp.task[i].sleep = 1.0;
			}
		}

#ifndef _DEBUG
		if (g_debug_tag++ % 0x1000 == 0x1000) {
			for (int i = 0; i < ML_TASK_LIMIT; i++) {
				__printf(szout, "%d:  %f   %f   %f   %f  %f result:%d\r\n",
					i, tp.task[i].usage, tp.task[i].user, tp.task[i].window, tp.task[i].delta, tp.task[i].priority, tp.result);
			}
		}
#endif
		if (g_train_complete == 0) {
			SaveMlData(&tp);
		}

		if (g_train_complete) {
			int seq = TaskSwitchPrediction(&tp);
			if (seq >= 0 && seq < count) {
				target_id = rate[seq].id;
				if (g_debug_tag++ % 0x100 == 0) {
					int cpu = *(int*)(LOCAL_APIC_BASE + 0x20) >> 24;
					LPPROCESS_INFO p = GetTaskTssBaseId(cpu);
					LPPROCESS_INFO tp = p + target_id;
					__printf(szout, "TaskSwitchPrediction seq:%d,count:%d tid:%x cpu:%x function:%s filename:%s\r\n",
						seq, count, target_id, cpu, tp->funcname, tp->filename);
				}
			}
			else {
				__printf(szout, "TaskSwitchPrediction seq:%d,count:%d error\r\n", seq, count);
			}

			target_tss = tss + target_id;
		}

		for (int i = 0; i < count; i++) {
			int tid = delta[i].id;
			if (target_id != delta[i].id) {

				tss[tid].delta += 1;
				
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