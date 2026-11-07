
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

#define TASK_SAMPLE_FRACTOR		0X10

int g_tp_cache_enable = 0;

unsigned long long g_tp_error_cnt = 0;
unsigned long long g_tp_corrects[16];

int g_tp_lock[256];

unsigned long long g_task_reason_cost = 0;
unsigned long long g_task_pre_total = 0;
unsigned long long g_task_pre_cache_hit = 0;
unsigned long long g_task_other_hit = 0;
unsigned long long g_task_pre_hit = 0;
unsigned long long g_task_train_hit = 0;

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
			g_task_pre_cache_hit++;
			return target_tss;
		}
	}

	int cnt = 0;

	double alloc_max = 0.01;
	double mem_max = 0.01;
	float sleep_max = 0.001;

	float delay_delta = 0.01;
	double delay_max = 0.01;
	
	double delta_max = 0.01;
	double authority_max = 0.01;

	PROCESS_INFO * ptr_delay_max = 0;

	int count = 0;

	int sleep_count = 0;

	unsigned long long ticknow = __krdtsc();

	PROCESS_INFO* cur = GetCurrentTaskTssBase();
	cur->delta =0;
	cur->priority = cur->priority / 2;
	cur->delay >>= 1;

#ifdef TASK_SWITCH_ARRAY
	PROCESS_INFO* tss = GetTaskTssBase();
	LPPROCESS_INFO prev = (LPPROCESS_INFO)(tss + cur->tid);
	LPPROCESS_INFO ptr = prev;
	do {
		if (ptr->status == TASK_TERMINATE) {
			ptr->status = TASK_OVER;
		}
		else if (ptr->status == TASK_RUN && cpu != ptr->cpuid) {
			__printf(szout, "%s %d status:%x cpu:%d cpuid:%d\r\n", __FUNCTION__, __LINE__, ptr->status,cpu,ptr->cpuid);
		}
		else if (ptr->status == TASK_RUN && cpu == ptr->cpuid) {
			if (ptr->sleep) {
				sleep_count++;
				if (ptr != prev)
					ptr->sleep--;
			}
			else {
				int dynamic = ptr->delta;
				double ratio = STATIC_PRIORITY;
				double cratio = STATIC_PRIORITY;
				if ((ptr->tick_total == 0 || g_cpu_tick[cpu] == 0) || ptr->tick_run == 0 || (ptr->param->cmd & TASK_REALTIME)) {
					dynamic = DYNAMIC_PRIORITY;
				}
				else {
					ratio = (double)(ptr->tick_total) / (ptr->tick_run);
					cratio = (double)g_cpu_tick[cpu] / (ptr->tick_run);
					if (ratio > STATIC_PRIORITY)
						ratio = STATIC_PRIORITY;
					if (cratio > STATIC_PRIORITY)
						cratio = STATIC_PRIORITY;
				}
				g_rate_buf[cpu][count].id = ptr->tid;
				g_crate_buf[cpu][count].id = ptr->tid;
				g_rate_buf[cpu][count].fv = ratio;
				g_crate_buf[cpu][count].fv = cratio;

				ptr->delta = dynamic;

				g_level_buf[cpu][count].id = ptr->tid;
				g_level_buf[cpu][count].fv = 0;

				if (ptr->alloc_times + ptr->free_times > alloc_max)
					alloc_max = ptr->alloc_times + ptr->free_times;

				if (*(ptr->lpvasize) > mem_max)
					mem_max = *(ptr->lpvasize);

				if ((ticknow - ptr->prev_tick) > delay_max) {
					delay_max = ticknow - ptr->prev_tick;
					ptr_delay_max = ptr;
				}
				ptr_delay_max->delay += delay_delta;
				delay_delta += STATIC_PRIORITY;

				if (ptr->delta > delta_max)
					delta_max = ptr->delta;

				if (ptr->authority > authority_max)
					authority_max = ptr->authority;

				if (ptr->sleep_total / (ptr->counter + 1) > sleep_max)
					sleep_max = ptr->sleep_total / (ptr->counter + 1);

				count++;
			}
		}

		ptr++;
		if (ptr - tss >= TASK_LIMIT_TOTAL) {
			ptr = tss;
		}
	} while (ptr != prev);

#else
	LPPROCESS_INFO tss[256];
	PROCESS_INFO_LIST* prevlist = GetRemoteTaskListTid(cpu,cur->tid,0);
	LPPROCESS_INFO prev = &prevlist->proc;
	PROCESS_INFO_LIST* list = prevlist;
	do {
		if (list == 0)
			break;
		LPPROCESS_INFO ptr = &list->proc;
		if (ptr->status == TASK_RUN) {
			if (ptr->sleep) {
				sleep_count++;
				if(ptr != prev)
					ptr->sleep--;
			}
			else {
				int dynamic = ptr->delta;
				double ratio = STATIC_PRIORITY;
				double cratio = STATIC_PRIORITY;
				if ( (ptr->tick_total == 0 || g_cpu_tick[cpu] == 0)|| ptr->tick_run == 0 || (ptr->param->cmd & TASK_REALTIME)) {
					dynamic = DYNAMIC_PRIORITY;
				}
				else {
					ratio =  (double)(ptr->tick_total)/ (ptr->tick_run);
					cratio = (double)g_cpu_tick[cpu]/ (ptr->tick_run);
					if (ratio > STATIC_PRIORITY)
						ratio = STATIC_PRIORITY;
					if (cratio > STATIC_PRIORITY)
						cratio = STATIC_PRIORITY;
				}
				g_rate_buf[cpu][count].id = ptr->tid;
				g_crate_buf[cpu][count].id = ptr->tid;
				g_rate_buf[cpu][count].fv = ratio;
				g_crate_buf[cpu][count].fv = cratio;

				ptr->delta = dynamic;

				g_level_buf[cpu][count].id = ptr->tid;
				g_level_buf[cpu][count].fv = 0.0;

				if(ptr->alloc_times+ptr->free_times > alloc_max)
					alloc_max = ptr->alloc_times + ptr->free_times;

				if (*(ptr->lpvasize) > mem_max)
					mem_max = *(ptr->lpvasize);

				tss[count] = ptr;

				if ((ticknow - ptr->prev_tick) > delay_max) {
					delay_max = ticknow - ptr->prev_tick;
					ptr_delay_max = ptr;
				}
				ptr_delay_max->delay += delay_delta;		
				delay_delta += STATIC_PRIORITY;

				if (ptr->delta > delta_max)
					delta_max = ptr->delta;

				if( ptr->sleep_total / (ptr->counter + 1) > sleep_max)
					sleep_max = ptr->sleep_total / (ptr->counter + 1);

				if (ptr->authority > authority_max)
					authority_max = ptr->authority;

				count++;
			}
		}
		else if (ptr->status == TASK_TERMINATE || ptr->status == TASK_OVER) {
			ptr->status = TASK_OVER;
		}
		list = (PROCESS_INFO_LIST*)list->list.next;
		if (cnt++ > 0x1000) {
			__printf(szout, "%s %d error\r\n",__FUNCTION__, __LINE__);
			break;
		}
	} while (list && list != prevlist);
#endif
	if ( count < 2) {
		g_task_other_hit++;
#ifdef TASK_SWITCH_ARRAY
		target_tss = tss + g_rate_buf[cpu][0].id;
#else
		target_tss = tss[0];
#endif
		if (count <= 0 && sleep_count == 0) {
			__printf(szout, "%s %d cpu:%d tid:%d status:%d error\r\n",__FUNCTION__, __LINE__, cpu, target_tss->tid, target_tss->status);
		}
	}
	else if (count >= 2) {
		for (int i = 0; i < count; i++) {
#ifdef TASK_SWITCH_ARRAY
			int pid = g_level_buf[cpu][i].id;
			//if (g_train_complete == 0) 
			{
				g_level_buf[cpu][i].fv += (g_rate_buf[cpu][i].fv );
				g_level_buf[cpu][i].fv += (g_crate_buf[cpu][i].fv );
				int window_priority = (tss[pid].window == 0 ? 0 : WINDOW_PRIORITY);
				int user_priority = (tss[pid].level == 0 ? USER_PRIORITY : 0);
				g_level_buf[cpu][i].fv += (window_priority + user_priority);
				g_level_buf[cpu][i].fv += tss[pid].delta;
				g_level_buf[cpu][i].fv += tss[pid].priority;
				g_level_buf[cpu][i].fv += tss[pid].authority;
				g_level_buf[cpu][i].fv += STATIC_PRIORITY *((tss[pid].alloc_times + tss[pid].free_times) / alloc_max);
				g_level_buf[cpu][i].fv += STATIC_PRIORITY * ((*tss[pid].lpvasize) / mem_max);
				float sleep_ratio = (float)(tss[pid].sleep_total / (tss[pid].counter + 1)) / (float)sleep_max;
				g_level_buf[cpu][i].fv += sleep_ratio * STATIC_PRIORITY;
				g_level_buf[cpu][i].fv += tss[pid].delay;
			}
#else
			{
				g_level_buf[cpu][i].fv += (g_rate_buf[cpu][i].fv );
				g_level_buf[cpu][i].fv += (g_crate_buf[cpu][i].fv );
				int window_priority = (tss[i]->window == 0 ? 0 : WINDOW_PRIORITY);
				int user_priority = (tss[i]->level == 0 ? USER_PRIORITY : 0);
				g_level_buf[cpu][i].fv += (window_priority + user_priority);
				g_level_buf[cpu][i].fv += tss[i]->delta;
				g_level_buf[cpu][i].fv += tss[i]->priority;
				g_level_buf[cpu][i].fv += tss[i]->authority;
				g_level_buf[cpu][i].fv += STATIC_PRIORITY * ((tss[i]->alloc_times + tss[i]->free_times) / alloc_max);
				g_level_buf[cpu][i].fv += STATIC_PRIORITY * (*(tss[i]->lpvasize) / mem_max);
				float sleep_ratio = (tss[i]->sleep_total / (tss[i]->counter + 1)) / sleep_max;
				g_level_buf[cpu][i].fv += sleep_ratio * STATIC_PRIORITY;
				g_level_buf[cpu][i].fv += tss[i]->delay;
#endif
			}
		}

		int target_id = -1;
		//if (g_dl_train_complete == 0) 
		{
			QuickSortd(g_level_buf[cpu], 0, count - 1);
			target_id = g_level_buf[cpu][count - 1].id;
		}

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
			if (pid == target_id) {
				g_tpp_buf[cpu]->result = i;
			}
			float user_ratio = (float)(tss[pid].level == 0 ? USER_PRIORITY : 0) / (float)STATIC_PRIORITY;
			float window_ratio = (float)(tss[pid].window ? WINDOW_PRIORITY : 0) / (float)STATIC_PRIORITY;
			g_tpp_buf[cpu]->task[i].delay = tss[pid].delay / delay_max;
			g_tpp_buf[cpu]->task[i].tickrate = g_rate_buf[cpu][i].fv / STATIC_PRIORITY;
			g_tpp_buf[cpu]->task[i].cpurate = g_crate_buf[cpu][i].fv / STATIC_PRIORITY;
			g_tpp_buf[cpu]->task[i].user = user_ratio;
			g_tpp_buf[cpu]->task[i].window = window_ratio;
			g_tpp_buf[cpu]->task[i].delta = tss[pid].delta / delta_max;
			g_tpp_buf[cpu]->task[i].priority = (float)(tss[pid].priority) / (float)STATIC_PRIORITY;
			g_tpp_buf[cpu]->task[i].authority = tss[pid].authority / authority_max;
			float sleep_ratio = (tss[pid].sleep_total / (tss[pid].counter + 1)) / sleep_max;
			g_tpp_buf[cpu]->task[i].sleep = sleep_ratio;
			g_tpp_buf[cpu]->task[i].mem = *tss[pid].lpvasize / mem_max ;
			g_tpp_buf[cpu]->task[i].alloc = ( tss[pid].alloc_times + tss[pid].free_times )/alloc_max;
#else
			if (g_rate_buf[cpu][i].id == target_id) {
				g_tpp_buf[cpu]->result = i;
			}
			float user_ratio = (float)(tss[i]->level == 0 ? USER_PRIORITY : 0) / (float)STATIC_PRIORITY;
			float window_ratio = (float)(tss[i]->window ? WINDOW_PRIORITY : 0) / (float)STATIC_PRIORITY;
			g_tpp_buf[cpu]->task[i].delay = tss[i]->delay / delay_max;
			g_tpp_buf[cpu]->task[i].tickrate = g_rate_buf[cpu][i].fv / STATIC_PRIORITY;
			g_tpp_buf[cpu]->task[i].cpurate = g_crate_buf[cpu][i].fv / STATIC_PRIORITY;
			g_tpp_buf[cpu]->task[i].user = user_ratio;
			g_tpp_buf[cpu]->task[i].window = window_ratio;
			g_tpp_buf[cpu]->task[i].delta = tss[i]->delta / delta_max;
			g_tpp_buf[cpu]->task[i].priority = (float)(tss[i]->priority) / (float)STATIC_PRIORITY;
			g_tpp_buf[cpu]->task[i].authority = tss[i]->authority / authority_max;
			float sleep_ratio = (tss[i]->sleep_total / (tss[i]->counter + 1)) / sleep_max;
			g_tpp_buf[cpu]->task[i].sleep = sleep_ratio;
			g_tpp_buf[cpu]->task[i].mem = *tss[i]->lpvasize / mem_max;
			g_tpp_buf[cpu]->task[i].alloc = (tss[i]->alloc_times + tss[i]->free_times) / alloc_max;
#endif
		}

		if (num == ML_TASK_LIMIT) {
			if (count != ML_TASK_LIMIT) {
				__printf("%s %d counter:%d exceed\r\n", __FUNCTION__, __LINE__, count);

				if (index >= ML_TASK_LIMIT) {
					int ri = __random(0) % ML_TASK_LIMIT;
					g_tpp_buf[cpu]->result = ri;
#ifdef TASK_SWITCH_ARRAY	
					int pid = g_rate_buf[cpu][index].id;
					float user_ratio = (float)(tss[pid].level == 0 ? USER_PRIORITY : 0) / (float)STATIC_PRIORITY;
					float window_ratio = (float)(tss[pid].window ? WINDOW_PRIORITY : 0) / (float)STATIC_PRIORITY;				
					g_tpp_buf[cpu]->task[ri].tickrate = g_rate_buf[cpu][index] / STATIC_PRIORITY;
					g_tpp_buf[cpu]->task[ri].cpurate = g_crate_buf[cpu][index].fv / STATIC_PRIORITY;
					g_tpp_buf[cpu]->task[ri].user = user_ratio;
					g_tpp_buf[cpu]->task[ri].window = window_ratio;
					g_tpp_buf[cpu]->task[ri].delta = tss[pid].delta / delta_max;
					g_tpp_buf[cpu]->task[ri].priority = (float)(tss[pid].priority) / (float)STATIC_PRIORITY;;
					g_tpp_buf[cpu]->task[ri].authority = tss[pid].authority / authority_max;
					float sleep_ratio = (tss[pid].sleep_total / (tss[pid].counter + 1)) /sleep_max;
					g_tpp_buf[cpu]->task[ri].sleep = sleep_ratio;
					g_tpp_buf[cpu]->task[ri].mem = (*tss[pid].lpvasize) / mem_max;
					g_tpp_buf[cpu]->task[ri].alloc =  (tss[pid].alloc_times + tss[pid].free_times)/(double)alloc_max;
					g_tpp_buf[cpu]->task[ri].delay = tss[pid].delay / delay_max;
					
					g_rate_buf[cpu][ri].id = pid;
					g_rate_buf[cpu][ri].fv = g_rate_buf[cpu][index].fv;
#else

					float user_ratio = (float)(tss[index]->level == 0 ? USER_PRIORITY : 0) / (float)STATIC_PRIORITY;
					float window_ratio = (float)(tss[index]->window ? WINDOW_PRIORITY : 0) / (float)STATIC_PRIORITY;
					g_tpp_buf[cpu]->task[ri].tickrate = g_rate_buf[cpu][index].fv / STATIC_PRIORITY;
					g_tpp_buf[cpu]->task[ri].cpurate = g_crate_buf[cpu][index].fv / STATIC_PRIORITY;
					g_tpp_buf[cpu]->task[ri].user = user_ratio;
					g_tpp_buf[cpu]->task[ri].window = window_ratio;
					g_tpp_buf[cpu]->task[ri].delta = tss[index]->delta / delta_max;
					g_tpp_buf[cpu]->task[ri].priority = (float)(tss[index]->priority) / (float)STATIC_PRIORITY;
					g_tpp_buf[cpu]->task[ri].authority = tss[index]->authority / authority_max;
					float sleep_ratio = (tss[index]->sleep_total / (tss[index]->counter + 1)) / sleep_max;
					g_tpp_buf[cpu]->task[ri].sleep = sleep_ratio;
					g_tpp_buf[cpu]->task[ri].mem = (*tss[index]->lpvasize) / mem_max;
					g_tpp_buf[cpu]->task[ri].alloc = (tss[index]->alloc_times + tss[index]->free_times)/alloc_max;
					g_tpp_buf[cpu]->task[ri].delay = tss[index]->delay / delay_max;
					g_rate_buf[cpu][ri].id = g_rate_buf[cpu][index].id;
					g_rate_buf[cpu][ri].fv = g_rate_buf[cpu][index].fv;
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
				g_tpp_buf[cpu]->task[i].delay = 0.0;
			}
		}

		if (g_dl_train_complete == 0  ) { 
			if ((count > ML_TASK_LIMIT / g_cpu_total) && (g_task_pre_total % TASK_SAMPLE_FRACTOR == 0)) {
#ifdef MODEL_TYPE_RNN
				CollectRNNSample(g_tpp_buf[cpu], count);
#else
				CollectDlSample(g_tpp_buf[cpu], count);
#endif			
			}
			g_task_train_hit++;
		}
		else {
			int seq = TaskSchedulePredict(g_tpp_buf[cpu]);
			if (seq >= 0 && seq < count) {
				if (g_td_mix_enable) {
					int id = g_rate_buf[cpu][seq].id;
					for (int i = 0; i < count; i++) {
						if (id == g_level_buf[cpu][i].id) {
							g_level_buf[cpu][i].fv += PREDICTION_PRIORITY;
							break;
						}
					}
					QuickSortd(g_level_buf[cpu], 0, count - 1);
					target_id = g_level_buf[cpu][count - 1].id;
				}
				else {
					target_id = g_rate_buf[cpu][seq].id;
				}
				PreMultipleHitRate(count, 3, cpu, target_id);
				g_task_pre_hit++;
			}
			else {
				target_id = g_rate_buf[cpu][0].id;
				g_task_other_hit++;
				g_tp_error_cnt++;
			}
			//__printf(szout, "%s %d cpu:%d seq:%d ,target_id:%d\r\n",__FUNCTION__, __LINE__, cpu, seq, target_id);
		}
#ifdef TASK_SWITCH_ARRAY
		for (int i = 0; i < count; i++) {
			int tid = g_rate_buf[cpu][i].id;
			if (target_id != g_rate_buf[cpu][i].id) {
				tss[tid].delta += STATIC_PRIORITY;
				if (tss[tid].delta > DYNAMIC_PRIORITY) {
					//tss[tid].delta = DYNAMIC_PRIORITY;
				}
			}
		}
		target_tss = tss + target_id;
#else
		int target_idx = -1;
		for (int i = 0; i < count; i++) {
			if (target_id != g_rate_buf[cpu][i].id) {
				tss[i]->delta += STATIC_PRIORITY;
				if (tss[i]->delta > DYNAMIC_PRIORITY) {
					//tss[tid].delta = DYNAMIC_PRIORITY;
				}
			}
			else {
				target_idx = i;
			}
		}

		target_tss = tss[target_idx];
		if (target_idx==-1)
		{
			__printf(szout, "%s %d cpu:%d target_idx:%d target_id:%d function:%s\r\n",
				__FUNCTION__, __LINE__, cpu, target_idx, target_id, target_tss->funcname);
		}

		//priorityTaskTest(count, cpu, target_id, target_idx, tss);
#endif	
	}

	target_tss->delta = 0;
	target_tss->authority = target_tss->authority/2;
	target_tss->delay >>= 1;

	return target_tss;
}


void PreMultipleHitRate(int count,int limit,int cpu,int target_id) {
	int min = count > limit ? limit : count;
	
	for (int i = 0; i < min; i++) {
		if (target_id == g_level_buf[cpu][count - 1 - i].id) {
			g_tp_corrects[i]++;
		}
	}
}


int g_priority_test_cnt = 0;


void priorityTaskTest(int count,int cpu,int target_id,int target_idx, LPPROCESS_INFO *tss) {
	char szout[1024];
	g_priority_test_cnt++;
	if (g_priority_test_cnt % 0x100 == 0) {
		
		__printf(szout, "%s %d cpu:%d,tid:%d target_id:%d,target_idx:%d,function:%s\r\n",
			__FUNCTION__, __LINE__, cpu,tss[target_idx]->tid, target_id, target_idx, tss[target_idx]->funcname);
	}
	if (g_priority_test_cnt % 0x200 == 0) {
		for (int i = 0; i < count; i++) {
			__printf(szout, "%d id:%d score:%lf    \r\n", i, (DWORD)g_level_buf[cpu][i].id, g_level_buf[cpu][i].fv);
		}
	}
}


int PredictionTask() {

	if (g_tp_cache_enable == 0|| g_dl_train_complete == 0 || g_dl_rate == 0.0) {
		return 0;
	}

	char szout[256];

	LPPROCESS_INFO target_tss = 0;

	PROCESS_INFO* cur = GetCurrentTaskTssBase();
	cur->delay >= 1;
	cur->priority >>= 1;
	cur->delta = 0;

	AlgorithmModel rate[TASK_LIMIT_TOTAL];
	AlgorithmModel crate[TASK_LIMIT_TOTAL];
	AlgorithmModel level[TASK_LIMIT_TOTAL];

	int cpu = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

	float alloc_max = 0.01;
	float mem_max = 0.01;
	float sleep_max = 0.001;
	//float run_max = 0.1;
	//__asm{cli}
	float delay_delta = 0.01;
	double delay_max = 0.01;
	PROCESS_INFO * ptr_delay_max = 0;
	double delta_max = 0.01;
	double authority_max = 0.01;
	unsigned long long ticknow = __krdtsc();

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
				if (cpu == ptr->cpuid) {
					if (ptr->status == TASK_TERMINATE) {
						ptr->status = TASK_OVER;
					}
					else if (ptr->status == TASK_RUN) {
						if (ptr->sleep) {
							//ptr->sleep--;
						}
						else {
							int dynamic = ptr->delta;

							double ratio = STATIC_PRIORITY;
							double cratio = STATIC_PRIORITY;
							if ((ptr->tick_total == 0 || g_cpu_tick[cpu] == 0) || ptr->tick_run == 0 || (ptr->param->cmd & TASK_REALTIME)) {
								dynamic = DYNAMIC_PRIORITY;
							}
							else {
								ratio = (double)(ptr->tick_total) / (ptr->tick_run);
								cratio = (double)g_cpu_tick[cpu] / (ptr->tick_run);
								if (ratio > STATIC_PRIORITY)
									ratio = STATIC_PRIORITY;
								if (cratio > STATIC_PRIORITY)
									cratio = STATIC_PRIORITY;
							}
							rate[count].id = ptr->tid;
							crate[count].id = ptr->tid;
							rate[count].fv = ratio;
							crate[count].fv = cratio;

							ptr->delta = dynamic;

							if (ptr->alloc_times + ptr->free_times > alloc_max)
								alloc_max = ptr->alloc_times + ptr->free_times;

							if (*(ptr->lpvasize) > mem_max)
								mem_max = *(ptr->lpvasize);

							if ((ticknow - ptr->prev_tick) > delay_max) {
								delay_max = ticknow - ptr->prev_tick;
								ptr_delay_max = ptr;
							}
							ptr_delay_max->delay += delay_delta;
							delay_delta += STATIC_PRIORITY;

							if (ptr->delta > delta_max)
								delta_max = ptr->delta;
							if (ptr->authority > authority_max)
								authority_max = ptr->authority;

							if (ptr->sleep_total / (ptr->counter + 1) > sleep_max)
								sleep_max = ptr->sleep_total / (ptr->counter + 1);

							count++;
						}
					}
				}
				ptr++;
				if (ptr - tss >= TASK_LIMIT_TOTAL) {
					ptr = tss;
				}
			} while (ptr != proc);
#else
			PROCESS_INFO* tss[256];
			PROCESS_INFO_LIST* root = (PROCESS_INFO_LIST*)GetTaskListId(cpu);
			PROCESS_INFO_LIST* hdr = (PROCESS_INFO_LIST*)root->list.next;
			PROCESS_INFO_LIST* list = hdr;
			do {
				if (list == 0)
					break;
				LPPROCESS_INFO ptr = &list->proc;
				if (ptr->status == TASK_TERMINATE) {
					ptr->status = TASK_OVER;
				}
				else if (ptr->status == TASK_RUN) {
					if (ptr->sleep) {
						//p->sleep--;
					}
					else {
						int dynamic = ptr->delta;

						double ratio = STATIC_PRIORITY;
						double cratio = STATIC_PRIORITY;
						if ((ptr->tick_total == 0 || g_cpu_tick[cpu] == 0) || ptr->tick_run == 0 || (ptr->param->cmd & TASK_REALTIME)) {
							dynamic = DYNAMIC_PRIORITY;
						}
						else {
							ratio = (double)(ptr->tick_total) / (ptr->tick_run);
							cratio = (double)g_cpu_tick[cpu] / (ptr->tick_run);
							if (ratio > STATIC_PRIORITY)
								ratio = STATIC_PRIORITY;
							if (cratio > STATIC_PRIORITY)
								cratio = STATIC_PRIORITY;
						}
						rate[count].id = ptr->tid;
						crate[count].id = ptr->tid;
						rate[count].fv = ratio;
						crate[count].fv =  cratio;

						ptr->delta = dynamic;

						level[count].id = ptr->tid;
						level[count].fv = 0;

						if (ptr->alloc_times + ptr->free_times > alloc_max)
							alloc_max = ptr->alloc_times + ptr->free_times;

						if (*(ptr->lpvasize) > mem_max)
							mem_max = *(ptr->lpvasize);

						tss[count] = ptr;

						if ((ticknow - ptr->prev_tick) > delay_max) {
							delay_max = ticknow - ptr->prev_tick;
							ptr_delay_max = ptr;
						}
						ptr_delay_max->delay += delay_delta;
						delay_delta += STATIC_PRIORITY;

						if (ptr->delta > delta_max)
							delta_max = ptr->delta;

						if (ptr->authority > authority_max)
							authority_max = ptr->authority;

						if (ptr->sleep_total / (ptr->counter + 1) > sleep_max)
							sleep_max = ptr->sleep_total / (ptr->counter + 1);

						count++;
					}
				}
				list = (PROCESS_INFO_LIST*)list->list.next;
			} while (list && list != hdr);
#endif
			if (count < 2) {
#ifdef TASK_SWITCH_ARRAY
				target_tss = tss + rate[0].id;
#else
				target_tss = tss[0];
#endif
				if (count <= 0 || target_tss->status == TASK_TERMINATE || target_tss->status == TASK_OVER) {
					__printf(szout, "%s %d cpu:%d tid:%d status:%d error\r\n",__FUNCTION__, __LINE__, cpu, target_tss->tid, target_tss->status);
				}
				g_task_other_hit++;
			}
			else if (count >= 2) {
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
					float user_ratio = (float)(tss[pid].level == 0 ? USER_PRIORITY : 0) / (float)STATIC_PRIORITY;
					float window_ratio = (float)(tss[pid].window ? WINDOW_PRIORITY : 0) / (float)STATIC_PRIORITY;
					tp.task[i].tickrate = rate[i].fv / STATIC_PRIORITY;
					tp.task[i].cpurate = crate[i].fv / STATIC_PRIORITY;
					tp.task[i].user = user_ratio;
					tp.task[i].window = window_ratio;
					tp.task[i].delta = tss[pid].delta / delta_max;
					tp.task[i].priority = (float)(tss[pid].priority) / (float)STATIC_PRIORITY;;
					tp.task[i].authority = tss[pid].authority /authority_max;;
					float sleep_ratio =(tss[pid].sleep_total / (tss[pid].counter + 1)) / sleep_max;
					tp.task[i].sleep = sleep_ratio;
					tp.task[i].mem = *(tss[pid].lpvasize) / mem_max;
					tp.task[i].alloc = (tss[pid].alloc_times + tss[pid].free_times )/ alloc_max;
					tp.task[i].delay = tss[pid].delay/delay_max;
#else
					float user_ratio = (float)(tss[i]->level == 0 ? USER_PRIORITY : 0) / (float)STATIC_PRIORITY;
					float window_ratio = (float)(tss[i]->window ? WINDOW_PRIORITY : 0) / (float)STATIC_PRIORITY;
					tp.task[i].tickrate = rate[i].fv / STATIC_PRIORITY;
					tp.task[i].cpurate = crate[i].fv / STATIC_PRIORITY;
					tp.task[i].user = user_ratio;
					tp.task[i].window = window_ratio;
					tp.task[i].delta = tss[i]->delta / delta_max;
					tp.task[i].priority = (float)(tss[i]->priority) / (float)STATIC_PRIORITY;
					tp.task[i].authority = tss[i]->authority / authority_max;
					float sleep_ratio = (tss[i]->sleep_total / (tss[i]->counter + 1)) / sleep_max;
					tp.task[i].sleep = sleep_ratio;
					tp.task[i].mem = *(tss[i]->lpvasize) / mem_max;
					tp.task[i].alloc = (tss[i]->alloc_times + tss[i]->free_times )/ alloc_max;
					tp.task[i].delay = tss[i]->delay / delay_max;
#endif
				}

				if (num == ML_TASK_LIMIT) {
					if (count != ML_TASK_LIMIT) {
						__printf("%s %d counter:%d exceed\r\n", __FUNCTION__, __LINE__, count);
						AlgorithmModel level[TASK_LIMIT_TOTAL];
						for (int i = 0; i < count; i++) {
#ifdef TASK_SWITCH_ARRAY
							int pid = rate[i].id;
							level[i].id = rate[i].id ;
							level[i].fv += rate[i].fv ;
							level[i].fv += crate[i].fv ;
							int windowlevel = (tss[pid].window == 0 ? 0 : WINDOW_PRIORITY);
							int userlevel = (tss[pid].level == 0 ? USER_PRIORITY : 0);
							level[i].fv += (windowlevel + userlevel);
							level[i].fv += tss[pid].delta;
							level[i].fv += tss[pid].priority;
							level[i].fv += tss[pid].authority;
							level[i].fv += STATIC_PRIORITY * (*(tss[pid].lpvasize) / (double)mem_max);
							level[i].fv += STATIC_PRIORITY * ((tss[pid].alloc_times + tss[pid].free_times)/alloc_max);
							float sleep_ratio = (float)(tss[pid].sleep_total / (tss[pid].counter + 1)) / (float)sleep_max;
							level[i].fv += sleep_ratio * STATIC_PRIORITY;
							level[i].fv += tss[pid].delay;
#else
							level[i].id = rate[i].id;
							level[i].fv += rate[i].fv  ;
							level[i].fv += crate[i].fv ;
							int windowlevel = (tss[i]->window == 0 ? 0 : WINDOW_PRIORITY);
							int userlevel = (tss[i]->level == 0 ? USER_PRIORITY : 0);
							level[i].fv += (windowlevel + userlevel);
							level[i].fv += tss[i]->delta;
							level[i].fv += tss[i]->priority;
							level[i].fv += tss[i]->authority;
							level[i].fv += STATIC_PRIORITY * (*(tss[i]->lpvasize) / (double)mem_max);
							level[i].fv += STATIC_PRIORITY * ((tss[i]->alloc_times + tss[i]->free_times)/ alloc_max);
							float sleep_ratio = (float)(tss[i]->sleep_total / (tss[i]->counter + 1)) / (float)sleep_max;
							level[i].fv += sleep_ratio * STATIC_PRIORITY;
							level[i].fv += tss[i]->delay;
#endif
						}
						QuickSortd(level, 0, count - 1);
						target_id = level[count - 1].id;

						int index = -1;
						for (int i = 0; i < count; i++) {
							if (rate[i].id == target_id) {
								index = i;
								break;
							}
						}

						if (index >= ML_TASK_LIMIT) {
							int ri = __random(0) % ML_TASK_LIMIT;
							tp.result = ri;
#ifdef TASK_SWITCH_ARRAY
							int pid = rate[index].id;
							float user_ratio = (float)(tss[pid].level == 0 ? USER_PRIORITY : 0) / (float)STATIC_PRIORITY;
							float window_ratio = (float)(tss[pid].window ? WINDOW_PRIORITY : 0) / (float)STATIC_PRIORITY;				
							tp.task[ri].tickrate = rate[index].fv / STATIC_PRIORITY;
							tp.task[ri].cpurate = crate[index].fv / STATIC_PRIORITY;
							tp.task[ri].user = user_ratio;
							tp.task[ri].window = window_ratio;
							tp.task[ri].delta = tss[pid].delta /delta_max;
							tp.task[ri].priority = (float)(tss[pid].priority) / (float)STATIC_PRIORITY;;
							tp.task[ri].authority = tss[pid].authority /authority_max;;
							float sleep_ratio = (tss[pid].sleep_total / (tss[pid].counter + 1)) /sleep_max;
							tp.task[ri].sleep = sleep_ratio;
							tp.task[ri].mem = (*(tss[pid].lpvasize) / mem_max);
							tp.task[ri].alloc = (tss[pid].alloc_times + tss[pid].free_times) / alloc_max);
							tp.task[ri].delay = tss[pid].delay;				
							rate[ri].id = pid;
							rate[ri].fv = rate[index].fv;
#else

							float user_ratio = (float)(tss[index]->level == 0 ? USER_PRIORITY : 0) / (float)STATIC_PRIORITY;
							float window_ratio = (float)(tss[index]->window ? WINDOW_PRIORITY : 0) / (float)STATIC_PRIORITY;
							tp.task[ri].tickrate = rate[index].fv / STATIC_PRIORITY;
							tp.task[ri].cpurate = crate[index].fv / STATIC_PRIORITY;
							tp.task[ri].user = user_ratio;
							tp.task[ri].window = window_ratio;
							tp.task[ri].delta = tss[index]->delta / delta_max;
							tp.task[ri].priority = (float)(tss[index]->priority) / (float)STATIC_PRIORITY;
							tp.task[ri].authority = tss[index]->authority / authority_max;
							float sleep_ratio = (tss[index]->sleep_total / (tss[index]->counter + 1)) / sleep_max;
							tp.task[ri].sleep = sleep_ratio;
							tp.task[ri].mem = (*(tss[index]->lpvasize) / mem_max);
							tp.task[ri].alloc = (tss[index]->alloc_times + tss[index]->free_times) / alloc_max;
							tp.task[ri].delay = tss[index]->delay;
							rate[ri].id = rate[index].id;
							rate[ri].fv = rate[index].fv;
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
						tp.task[i].delay = 0.0;
					}
				}		
				
				int seq = TaskSchedulePredict(&tp);
				if (seq >= 0 && seq < count) {
					target_id = rate[seq].id;	
					total++;
					PreMultipleHitRate(count, 3, cpu, target_id);
					g_task_pre_hit++;
				}
				else {
					target_id = rate[0].id;
					g_tp_error_cnt++;
					g_task_other_hit++;
				}
#ifdef TASK_SWITCH_ARRAY
				target_tss = tss + target_id;
				for (int i = 0; i < count; i++) {
					int tid = rate[i].id;
					if (target_id != rate[i].id) {
						tss[tid].delta += STATIC_PRIORITY;
						if (tss[tid].delta > DYNAMIC_PRIORITY) {
							//tss[tid].delta = DYNAMIC_PRIORITY;
						}
					}
				}
#else
				target_tss = tss[seq];
				for (int i = 0; i < count; i++) {
					if (target_id != rate[i].id) {
						tss[i]->delta += STATIC_PRIORITY;
						if (tss[i]->delta > DYNAMIC_PRIORITY) {
							//tss[tid].delta = DYNAMIC_PRIORITY;
						}
					}
				}
#endif
			}

			target_tss->delta = 0;
			target_tss->delay >>= 1;
			target_tss->authority = target_tss->authority / 2;

			g_task_predict_buf[cpu][num] = target_tss;
		}
	}

	__leaveSpinlock(&g_tp_lock[cpu]);
	//__asm{sti}
	return total;
}
