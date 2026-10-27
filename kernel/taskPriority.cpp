

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

unsigned long long g_tp_correct0 = 0;
unsigned long long g_tp_correct1 = 0;
unsigned long long g_tp_correct2 = 0;
unsigned long long g_tp_correct3 = 0;
unsigned long long g_tp_correct4 = 0;

int g_tp_lock[256];

unsigned long long g_task_pre_total = 0;
unsigned long long g_task_pre_hit = 0;
unsigned long long g_task_pre_cost = 0;

unsigned long long g_task_other_hit = 0;
unsigned long long g_task_dl_hit = 0;

LPPROCESS_INFO g_task_predict_buf[256][TASK_PREDICTION_BUF_SIZE];


AlgorithmModel* g_rate_buf[256];
AlgorithmModel* g_crate_buf[256];
AlgorithmModel* g_level_buf[256];
TaskPredictParam* g_tpp_buf[256];

void InitTaskScheduleBuf() {
	int cpu = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	int allocSize = MAX_CPU_COUNT * sizeof(double);
	if (g_level_buf[cpu] == 0) {
		allocSize = MAX_CPU_COUNT * sizeof(AlgorithmModel);
		g_level_buf[cpu] = (AlgorithmModel*)__kProcessMalloc(allocSize, 0, 0, cpu, 0,
			PAGE_READWRITE | PAGE_USERPRIVILEGE | PAGE_PRESENT | 0x80000000);
	}
	if (g_crate_buf[cpu] == 0) {
		allocSize = MAX_CPU_COUNT * sizeof(AlgorithmModel);
		g_crate_buf[cpu] = (AlgorithmModel*)__kProcessMalloc(allocSize, 0, 0, cpu, 0,
			PAGE_READWRITE | PAGE_USERPRIVILEGE | PAGE_PRESENT | 0x80000000);
	}
	if (g_rate_buf[cpu] == 0) {
		allocSize = MAX_CPU_COUNT * sizeof(AlgorithmModel);
		g_rate_buf[cpu] = (AlgorithmModel*)__kProcessMalloc(allocSize, 0, 0, cpu, 0,
			PAGE_READWRITE | PAGE_USERPRIVILEGE | PAGE_PRESENT | 0x80000000);
	}

	if (g_tpp_buf[cpu] == 0) {
		allocSize = sizeof(TaskPredictParam);
		g_tpp_buf[cpu] = (TaskPredictParam*)__kProcessMalloc(allocSize, 0, 0, cpu, 0,
			PAGE_READWRITE | PAGE_USERPRIVILEGE | PAGE_PRESENT | 0x80000000);
	}
}


unsigned long GetValueFromArray(AlgorithmModel* array, int size, int key) {
	for (int i = 0; i < size; i++) {
		if (array[i].id == key) {
			return (unsigned long)array[i].v;
		}
	}
	return 0;
}



PROCESS_INFO* PrioritySchedule() {
	
	char szout[256];
	//__printf(szout, "%s %d\r\n", __FUNCTION__, __LINE__);
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

	double total_alloc = 0.0;
	double total_mem = 0.0;
	double total_sleep = 0.0;
	int count = 0;

	PROCESS_INFO* cur = GetCurrentTaskTssBase();
#ifdef TASK_SWITCH_ARRAY
	PROCESS_INFO* tss = GetTaskTssBase();
	LPPROCESS_INFO prev = (LPPROCESS_INFO)(tss + cur->tid);
	LPPROCESS_INFO ptr = prev;
	LPPROCESS_INFO next = 0;
	do {
		if (ptr->status == TASK_TERMINATE) {
			ptr->status = TASK_OVER;
		}
		else if (ptr->status == TASK_RUN && cpu != ptr->cpuid) {
			__printf(szout, "%s %d status:%x cpu:%d cpuid:%d\r\n", __FUNCTION__, __LINE__, ptr->status,cpu,ptr->cpuid);
		}
		else if (ptr->status == TASK_RUN && cpu == ptr->cpuid) {
			if (ptr->sleep) {
				ptr->sleep--;
			}
			else {
				int dynamic = ptr->delta;
				double ratio = 1.0;
				double cratio = 1.0;
				if ((ptr->tick_run == 0 || ptr->tick_total == 0 || g_cpu_tick[cpu] == 0) || (ptr->param->cmd & TASK_REALTIME)) {
					dynamic = DYNAMIC_PRIORITY;
					ratio = 0.01;
					cratio = 0.01;
				}
				else {
					double diff = (ptr->tick_total);
					if(diff != 0.0)
						ratio = (ptr->tick_run) / diff;
					diff = g_cpu_tick[cpu];
					if (diff != 0.0)
						cratio = (ptr->tick_run) / diff;
				}
				g_rate_buf[cpu][count].id = ptr->tid;
				g_crate_buf[cpu][count].id = ptr->tid;
				double v = 1.0 / ratio ;
				if (v > STATIC_PRIORITY)
					v = STATIC_PRIORITY;
				g_rate_buf[cpu][count].v = (unsigned long long) v;
				v = 1.0 / cratio ;
				if (v > STATIC_PRIORITY)
					v = STATIC_PRIORITY;
				g_crate_buf[cpu][count].v = (unsigned long long) v;

				ptr->delta = dynamic;
				g_level_buf[cpu][count].id = ptr->tid;
				g_level_buf[cpu][count].v = 0;

				total_alloc += ptr->alloc_times;
				total_mem += (*ptr->lpvasize);
				total_sleep += ptr->sleep;

				count++;

				if (next == 0 && ptr != prev) {
					next = ptr;
				}
			}
		}

		ptr++;
		if (ptr - tss >= TASK_LIMIT_TOTAL) {
			ptr = tss;
		}

		if (ptr == prev) {
			break;
		}
	} while (TRUE);
	//__printf(szout, "%s %d\r\n", __FUNCTION__, __LINE__);
#else
	LPPROCESS_INFO tss[256];
	PROCESS_INFO_LIST* prevlist = GetRemoteTaskListTid(cpu,cur->tid);
	LPPROCESS_INFO prev = &prevlist->proc;
	PROCESS_INFO_LIST* nextlist = 0;
	LPPROCESS_INFO next = 0;
	PROCESS_INFO_LIST* ptr = prevlist;
	do {
		if (ptr == 0)
			break;
		LPPROCESS_INFO p = &ptr->proc;
		if (p->status == TASK_RUN) {
			if (p->sleep) {
				p->sleep--;
			}
			else {
				int dynamic = p->delta;
				double ratio = 1.0;
				double cratio = 1.0;
				if ( (p->tick_run == 0 || p->tick_total == 0 || g_cpu_tick[cpu] == 0) || (p->param->cmd & TASK_REALTIME)) {
					dynamic = DYNAMIC_PRIORITY;
					ratio = 0.01;
					cratio = 0.01;
				}
				else {
					double diff =(p->tick_total);
					if (diff != 0.0)
						ratio = (p->tick_run) / diff;
					diff = g_cpu_tick[cpu];
					if (diff != 0.0)
						cratio = (p->tick_run) / diff;
				}
				g_rate_buf[cpu][count].id = p->tid;
				g_crate_buf[cpu][count].id = p->tid;
				double v = 1.0 / ratio;
				if (v > STATIC_PRIORITY)
					v = STATIC_PRIORITY;
				g_rate_buf[cpu][count].v = (unsigned long long) v;
				v = 1.0 / cratio;
				if (v > STATIC_PRIORITY)
					v = STATIC_PRIORITY;
				g_crate_buf[cpu][count].v = (unsigned long long) v;

				p->delta = dynamic;
				g_level_buf[cpu][count].id = p->tid;
				g_level_buf[cpu][count].v = 0;

				total_alloc += p->alloc_times;
				total_mem += (*p->lpvasize);
				total_sleep += p->sleep;

				tss[count++] = p;

				if (next == 0 && p != prev) {
					next = p;
				}		
			}
		}

		nextlist = (PROCESS_INFO_LIST*)ptr->list.next;
		if (p->status == TASK_TERMINATE || p->status == TASK_OVER) {
			p->status = TASK_OVER;
			if (p->pid == p->tid) {
				//__kExitProcess(ptr);
			}
		}
		ptr = nextlist;
	} while (ptr && ptr != prevlist);
#endif
	if (target_tss == 0)
		target_tss = prev;

	if (count == 1) {
		target_tss = prev;
		g_task_other_hit++;
	}
	else if (count <= 0) {
		target_tss = prev;
		return target_tss;
	}
	else if (count == 2) {
		target_tss = next;
		g_task_other_hit++;
	}
	else if (count > 2) {
		
		if(total_sleep == 0)
			total_sleep = 1;

#ifdef TASK_SWITCH_ARRAY
		for (int i = 0; i < count; i++) {
			int pid = g_level_buf[cpu][i].id;
			double alloc_ratio = tss[pid].alloc_times / total_alloc;
			double mem_ratio = (*tss[pid].lpvasize) / total_mem;
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
#else
		for (int i = 0; i < count; i++) {
			double alloc_ratio = tss[i]->alloc_times / total_alloc;
			double mem_ratio = (* (tss[i]->lpvasize)) / total_mem;
			//if (g_train_complete == 0) 
			{
				g_level_buf[cpu][i].v += g_rate_buf[cpu][i].v;
				g_level_buf[cpu][i].v += g_crate_buf[cpu][i].v;
				int window_priority = (tss[i]->window == 0 ? 0 : WINDOW_PRIORITY);
				int user_priority = (tss[i]->level == 0 ? USER_PRIORITY : 0);
				g_level_buf[cpu][i].v += (window_priority + user_priority);
				g_level_buf[cpu][i].v += tss[i]->delta;
				g_level_buf[cpu][i].v += tss[i]->priority;
				g_level_buf[cpu][i].v += tss[i]->authority;
				g_level_buf[cpu][i].v += STATIC_PRIORITY * alloc_ratio;
				g_level_buf[cpu][i].v += STATIC_PRIORITY * mem_ratio;
				g_level_buf[cpu][i].v += (tss[i]->sleep / total_sleep) * STATIC_PRIORITY;
			}
		}
#endif
		int target_id = -1;
		//if (g_dl_train_complete == 0) 
		{
			QuickSort(g_level_buf[cpu], 0, count - 1);
			target_id = g_level_buf[cpu][count - 1].id;
		}

		//TaskPredictParam tp;
		g_tpp_buf[cpu]->result = -1;

		int index = -1;
		for (int i = 0; i < count; i++) {
			if (g_rate_buf[cpu][i].id == target_id) {
				index = i;
				break;
			}
		}

		int num = 0;
		if (count >= ML_TASK_LIMIT) {
			num = ML_TASK_LIMIT;
		}
		else {
			num = count;
		}

		for (int i = 0; i < num; i++) {
#ifdef TASK_SWITCH_ARRAY
			int pid = g_rate_buf[cpu][i].id;
			float tick_ratio = (float)GetValueFromArray(g_rate_buf[cpu], count, pid) / (float)STATIC_PRIORITY;
			float user_ratio = (float)(tss[pid].level == 0 ? USER_PRIORITY : 0) / (float)STATIC_PRIORITY;
			float window_ratio = (float)(tss[pid].window ? WINDOW_PRIORITY : 0) / (float)STATIC_PRIORITY;
			float delta_ratio = (float)tss[pid].delta / (float)DYNAMIC_PRIORITY;
			float priority_ratio = (float)(tss[pid].priority) / (float)STATIC_PRIORITY;
			float authority_r = (float)tss[pid].authority / (float)AUTHORITY_PRIORITY;

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
#else
			float tick_ratio = (float)g_rate_buf[cpu][i].fv / (float)STATIC_PRIORITY;
			float user_ratio = (float)(tss[i]->level == 0 ? USER_PRIORITY : 0) / (float)STATIC_PRIORITY;
			float window_ratio = (float)(tss[i]->window ? WINDOW_PRIORITY : 0) / (float)STATIC_PRIORITY;
			float delta_ratio = (float)tss[i]->delta / (float)DYNAMIC_PRIORITY;
			float priority_ratio = (float)(tss[i]->priority) / (float)STATIC_PRIORITY;
			float authority_r = (float)tss[i]->authority / (float)AUTHORITY_PRIORITY;

			if (g_rate_buf[cpu][i].id == target_id) {
				g_tpp_buf[cpu]->result = i;
			}
			g_tpp_buf[cpu]->task[i].tickrate = tick_ratio;
			g_tpp_buf[cpu]->task[i].cpurate = (float)g_crate_buf[cpu][i].fv / (float)STATIC_PRIORITY;
			g_tpp_buf[cpu]->task[i].user = user_ratio;
			g_tpp_buf[cpu]->task[i].window = window_ratio;
			g_tpp_buf[cpu]->task[i].delta = delta_ratio;
			g_tpp_buf[cpu]->task[i].priority = priority_ratio;
			g_tpp_buf[cpu]->task[i].authority = authority_r;
			g_tpp_buf[cpu]->task[i].sleep = (double)tss[i]->sleep / (double)total_sleep;
			g_tpp_buf[cpu]->task[i].mem = *tss[i]->lpvasize / total_mem;
			g_tpp_buf[cpu]->task[i].alloc = tss[i]->alloc_times / total_alloc;
#endif
		}

		if (num == ML_TASK_LIMIT) {
			if (count != ML_TASK_LIMIT) {
				__printf("%s %d counter:%d exceed\r\n", __FUNCTION__, __LINE__, count);

				if (index >= ML_TASK_LIMIT) {
#ifdef TASK_SWITCH_ARRAY
					int pid = g_rate_buf[cpu][index].id;
					float tick_ratio = (float)GetValueFromArray(g_rate_buf[cpu], count, pid) / (float)STATIC_PRIORITY;
					float user_ratio = (float)(tss[pid].level == 0 ? USER_PRIORITY : 0) / (float)STATIC_PRIORITY;
					float window_ratio = (float)(tss[pid].window ? WINDOW_PRIORITY : 0) / (float)STATIC_PRIORITY;
					float delta_ratio = (float)tss[pid].delta / (float)DYNAMIC_PRIORITY;
					float priority_ratio = (float)(tss[pid].priority) / (float)STATIC_PRIORITY;
					float authority_r = (float)tss[pid].authority / (float)AUTHORITY_PRIORITY;

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
#else
					float tick_ratio = (float)g_rate_buf[cpu][index ].fv/ (float)STATIC_PRIORITY;
					float user_ratio = (float)(tss[index]->level == 0 ? USER_PRIORITY : 0) / (float)STATIC_PRIORITY;
					float window_ratio = (float)(tss[index]->window ? WINDOW_PRIORITY : 0) / (float)STATIC_PRIORITY;
					float delta_ratio = (float)tss[index]->delta / (float)DYNAMIC_PRIORITY;
					float priority_ratio = (float)(tss[index]->priority) / (float)STATIC_PRIORITY;
					float authority_r = (float)tss[index]->authority / (float)AUTHORITY_PRIORITY;

					int ri = __random(0) % ML_TASK_LIMIT;
					g_tpp_buf[cpu]->task[ri].tickrate = tick_ratio;
					g_tpp_buf[cpu]->task[ri].cpurate = (float)g_crate_buf[cpu][index].fv / (float)STATIC_PRIORITY;
					g_tpp_buf[cpu]->task[ri].user = user_ratio;
					g_tpp_buf[cpu]->task[ri].window = window_ratio;
					g_tpp_buf[cpu]->task[ri].delta = delta_ratio;
					g_tpp_buf[cpu]->task[ri].priority = priority_ratio;
					g_tpp_buf[cpu]->task[ri].authority = authority_r;
					g_tpp_buf[cpu]->task[ri].sleep = (double)tss[index]->sleep / (double)total_sleep;
					g_tpp_buf[cpu]->task[ri].mem = (double)(*tss[index]->lpvasize) / (double)total_mem;
					g_tpp_buf[cpu]->task[ri].alloc = (double)tss[index]->alloc_times / (double)total_alloc;
					g_tpp_buf[cpu]->result = ri;

					g_rate_buf[cpu][ri].id = g_rate_buf[cpu][index].id;
					g_rate_buf[cpu][ri].v = g_rate_buf[cpu][index].v;
#endif
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
#ifdef MODEL_TYPE_RNN
			CollectRNNSample(g_tpp_buf[cpu]);		
#else
			CollectDlSample(g_tpp_buf[cpu]);
#endif
			g_task_other_hit++;
		}
		else {
			int old_h1 = g_level_buf[cpu][count - 1].id;
			int old_h2 = 0;
			if(count >= 2)
				old_h2 = g_level_buf[cpu][count - 2].id;
			int old_h3 = 0;
			if(count >= 3)
				old_h3=g_level_buf[cpu][count - 3].id;
			int old_h4= 0;
			if(count >= 4)
				old_h4=g_level_buf[cpu][count - 4].id;

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

				if (old_h1 == target_id) {
					g_tp_correct1++;
				}
				else if (count >= 2 && target_id == old_h2) {
					g_tp_correct2++;
				}
				else if (count >= 3 && target_id == old_h3) {
					g_tp_correct3++;
				}
				else if (count >= 4 && target_id == old_h4) {
					g_tp_correct4++;
				}
				g_task_dl_hit++;
			}
			else {
				target_id = next->tid;
				g_task_other_hit++;
				g_tp_correct0++;
			}
		}
#ifdef TASK_SWITCH_ARRAY
		for (int i = 0; i < count; i++) {
			int tid = g_rate_buf[cpu][i].id;
			if (target_id != g_rate_buf[cpu][i].id) {
				tss[tid].delta += DELTA_UNIT_PRIORITY;
				if (tss[tid].delta > DYNAMIC_PRIORITY) {
					//tss[tid].delta = DYNAMIC_PRIORITY;
				}
			}
			else {
				tss[tid].delta = 0;
			}
		}
		target_tss = tss + target_id;
#else
		for (int i = 0; i < count; i++) {
			if (target_id != g_rate_buf[cpu][i].id) {
				tss[i]->delta += DELTA_UNIT_PRIORITY;
				if (tss[i]->delta > DYNAMIC_PRIORITY) {
					//tss[tid].delta = DYNAMIC_PRIORITY;
				}
			}
			else {
				tss[i]->delta = 0;
			}
		}
		target_tss = tss[index];
#endif	
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
	PROCESS_INFO* cur = GetCurrentTaskTssBase();
	LPPROCESS_INFO next = 0;

	AlgorithmModel rate[TASK_LIMIT_TOTAL];
	AlgorithmModel crate[TASK_LIMIT_TOTAL];
	int cpu = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

	double total_alloc = 0.0;
	double total_mem = 0.0;
	double total_sleep = 0.0;
	//__asm{cli}
	__enterSpinlock(&g_tp_lock[cpu]);

	int total = 0;

	for (int num = 0; num < TASK_PREDICTION_BUF_SIZE; num++) {

		if (g_task_predict_buf[cpu][num] == 0) {

			int count = 0;
#ifdef TASK_SWITCH_ARRAY
			PROCESS_INFO* tss = GetTaskTssBase();
			LPPROCESS_INFO proc = (LPPROCESS_INFO)(tss + cur->tid);
			LPPROCESS_INFO ptr = proc;
			do {
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
						if ((ptr->tick_run == 0 || ptr->tick_total == 0 || g_cpu_tick[cpu] == 0) || (ptr->param->cmd & TASK_REALTIME)) {
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

						ptr++;
						if (ptr - tss >= TASK_LIMIT_TOTAL) {
							ptr = tss;
						}
						if (ptr == proc) {
							break;
						}

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
#else
			PROCESS_INFO* tss[256];

			PROCESS_INFO_LIST* proclist = GetTaskListTid(cur->tid);
			LPPROCESS_INFO proc = &proclist->proc;
			PROCESS_INFO_LIST* list = (PROCESS_INFO_LIST*)GetTaskListId(cpu);
			PROCESS_INFO_LIST* hdr = (PROCESS_INFO_LIST*)list->list.next;
			PROCESS_INFO_LIST* ptr = hdr;
			do {
				if (ptr == 0)
					break;
				LPPROCESS_INFO p = &ptr->proc;
				if (p->status == TASK_TERMINATE) {
					p->status = TASK_OVER;
				}
				else if (p->status == TASK_RUN) {
					if (p->sleep) {
						//p->sleep--;
					}
					else {
						int dynamic = p->delta;
						double ratio = 0.0;
						double cratio = 0.0;
						if ((p->tick_run == 0 || p->tick_total == 0 || g_cpu_tick[cpu] == 0)|| (p->param->cmd & TASK_REALTIME)) {
							dynamic = DYNAMIC_PRIORITY;
							ratio = 0.01;
							cratio = 0.01;
						}
						else {
							double diff = (double)(p->tick_total);
							ratio = ((double)p->tick_run) / diff;
							diff = g_cpu_tick[cpu];
							cratio = ((double)p->tick_run) / (double)diff;
						}
						g_rate_buf[cpu][count].id = p->tid;
						g_crate_buf[cpu][count].id = p->tid;
						double v = 1.0 / ratio;
						if (v > STATIC_PRIORITY)
							v = STATIC_PRIORITY;
						g_rate_buf[cpu][count].v = (unsigned long long) v;
						v = 1.0 / cratio;
						if (v > STATIC_PRIORITY)
							v = STATIC_PRIORITY;
						g_crate_buf[cpu][count].v = (unsigned long long) v;

						p->delta = dynamic;
						g_level_buf[cpu][count].id = p->tid;
						g_level_buf[cpu][count].v = 0;

						total_alloc += p->alloc_times;
						total_mem += (*p->lpvasize);
						total_sleep += p->sleep;

						tss[count] = p;
						count++;

						if (next == 0) {
							next = p;
						}
					}
				}
				ptr = (PROCESS_INFO_LIST*)ptr->list.next;
			} while (ptr && ptr != hdr);
#endif

			if (count == 1) {
				target_tss = proc;
			}
			else if (count == 2) {
				target_tss = next;
			}
			else if (count > 2) {
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
#ifdef TASK_SWITCH_ARRAY
					int pid = rate[i].id;
					float tick_ratio = (float)GetValueFromArray(rate, count, pid) / (float)STATIC_PRIORITY;
					float user_ratio = (float)(tss[pid].level == 0 ? USER_PRIORITY : 0) / (float)STATIC_PRIORITY;
					float window_ratio = (float)(tss[pid].window ? WINDOW_PRIORITY : 0) / (float)STATIC_PRIORITY;
					float delta_ratio = (float)tss[pid].delta / (float)DYNAMIC_PRIORITY;
					float priority_ratio = (float)(tss[pid].priority) / (float)STATIC_PRIORITY;
					float authority_r = (float)tss[pid].authority / (float)AUTHORITY_PRIORITY;

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
#else
					int pid = rate[i].id;
					float tick_ratio = (float)GetValueFromArray(rate, count, pid) / (float)STATIC_PRIORITY;
					float user_ratio = (float)(tss[pid]->level == 0 ? USER_PRIORITY : 0) / (float)STATIC_PRIORITY;
					float window_ratio = (float)(tss[pid]->window ? WINDOW_PRIORITY : 0) / (float)STATIC_PRIORITY;
					float delta_ratio = (float)tss[pid]->delta / (float)DYNAMIC_PRIORITY;
					float priority_ratio = (float)(tss[pid]->priority) / (float)STATIC_PRIORITY;
					float authority_r = (float)tss[pid]->authority / (float)AUTHORITY_PRIORITY;

					tp.task[i].tickrate = tick_ratio;
					tp.task[i].cpurate = crate[i].fv / (float)STATIC_PRIORITY;
					tp.task[i].user = user_ratio;
					tp.task[i].window = window_ratio;
					tp.task[i].delta = delta_ratio;
					tp.task[i].priority = priority_ratio;
					tp.task[i].authority = authority_r;
					tp.task[i].sleep = tss[pid]->sleep / total_sleep;
					tp.task[i].mem = *(tss[pid]->lpvasize) / (double)total_mem;
					tp.task[i].alloc = tss[pid]->alloc_times / total_alloc;
#endif
				}

				if (num == ML_TASK_LIMIT) {
					if (count != ML_TASK_LIMIT) {
						__printf("%s %d counter:%d exceed\r\n", __FUNCTION__, __LINE__, count);
						AlgorithmModel level[TASK_LIMIT_TOTAL];
						for (int i = 0; i < count; i++) {
#ifdef TASK_SWITCH_ARRAY
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
#else
							int pid = rate[i].id;
							level[i].id = rate[i].id;
							level[i].v += rate[i].v;
							level[i].v += crate[i].v;
							int windowlevel = (tss[pid]->window == 0 ? 0 : WINDOW_PRIORITY);
							int userlevel = (tss[pid]->level == 0 ? USER_PRIORITY : 0);
							level[i].v += (windowlevel + userlevel);

							level[i].v += tss[pid]->delta;
							level[i].v += tss[pid]->priority;
							level[i].v += tss[pid]->authority;
							level[i].v += STATIC_PRIORITY * (*(tss[pid]->lpvasize) / (double)total_mem);
							level[i].v += STATIC_PRIORITY * (tss[pid]->alloc_times / total_alloc);
							level[i].v += (tss[pid]->sleep / total_sleep) * STATIC_PRIORITY;
#endif
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
#ifdef TASK_SWITCH_ARRAY
							int pid = rate[index].id;
							float tick_ratio = (float)GetValueFromArray(rate, count, pid) / (float)STATIC_PRIORITY;
							float user_ratio = (float)(tss[pid].level == 0 ? USER_PRIORITY : 0) / (float)STATIC_PRIORITY;
							float window_ratio = (float)(tss[pid].window ? WINDOW_PRIORITY : 0) / (float)STATIC_PRIORITY;
							float delta_ratio = (float)tss[pid].delta / (float)DYNAMIC_PRIORITY;
							float priority_ratio = (float)(tss[pid].priority) / (float)STATIC_PRIORITY;
							float authority_r = (float)tss[pid].authority / (float)AUTHORITY_PRIORITY;

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
#else
							int pid = rate[index].id;
							float tick_ratio = (float)GetValueFromArray(rate, count, pid) / (float)STATIC_PRIORITY;
							float user_ratio = (float)(tss[pid]->level == 0 ? USER_PRIORITY : 0) / (float)STATIC_PRIORITY;
							float window_ratio = (float)(tss[pid]->window ? WINDOW_PRIORITY : 0) / (float)STATIC_PRIORITY;
							float delta_ratio = (float)tss[pid]->delta / (float)DYNAMIC_PRIORITY;
							float priority_ratio = (float)(tss[pid]->priority) / (float)STATIC_PRIORITY;
							float authority_r = (float)tss[pid]->authority / (float)AUTHORITY_PRIORITY;

							int ri = __random(0) % ML_TASK_LIMIT;
							tp.task[ri].tickrate = tick_ratio;
							tp.task[ri].cpurate = crate[index].fv / (float)STATIC_PRIORITY;
							tp.task[ri].user = user_ratio;
							tp.task[ri].window = window_ratio;
							tp.task[ri].delta = delta_ratio;
							tp.task[ri].priority = priority_ratio;
							tp.task[ri].authority = authority_r;
							tp.task[ri].sleep = tss[pid]->sleep / total_sleep;
							tp.task[ri].mem = (*(tss[pid]->lpvasize) / (double)total_mem);
							tp.task[ri].alloc = (tss[pid]->alloc_times / total_alloc);
							tp.result = ri;

							rate[ri].id = pid;
							rate[ri].v = rate[index].v;
#endif
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
#ifdef TASK_SWITCH_ARRAY
				target_tss = tss + target_id;
				for (int i = 0; i < count; i++) {
					int tid = rate[i].id;
					if (target_id != rate[i].id) {
						tss[tid].delta += DELTA_UNIT_PRIORITY;
						if (tss[tid].delta > DYNAMIC_PRIORITY) {
							//tss[tid].delta = DYNAMIC_PRIORITY;
						}
					}
					else {
						tss[tid].delta = 0;
					}
				}
#else
				target_tss = tss[ target_id];
				for (int i = 0; i < count; i++) {
					int tid = rate[i].id;
					if (target_id != rate[i].id) {
						tss[tid]->delta += DELTA_UNIT_PRIORITY;
						if (tss[tid]->delta > DYNAMIC_PRIORITY) {
							//tss[tid].delta = DYNAMIC_PRIORITY;
						}
					}
					else {
						tss[tid]->delta = 0;
					}
				}
#endif
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