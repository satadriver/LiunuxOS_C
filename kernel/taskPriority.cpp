
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


double GetCpuRatio(int cpu) {
	double cpu_total = 0.01;
	for (int n = 0; n < g_cpu_total; n++) {
		int cpuid = g_cpu_id[n];
		cpu_total += g_cpu_tick[cpuid];
	}
	double cpu_rate = g_cpu_tick[cpu];
	if (cpu_rate == 0.0)
		cpu_rate = 0.01;
	double cpu_ratio = (double)cpu_total / cpu_rate;
	if (cpu_ratio > STATIC_PRIORITY)
		cpu_ratio = STATIC_PRIORITY;
	return cpu_ratio;
}

PROCESS_INFO* PrioritySchedule(int sleep) {
	
	char szout[256];
	//__printf(szout, "%s %d\r\n", __FUNCTION__, __LINE__);
	g_task_pre_total++;

	int cpu = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

	PROCESS_INFO* cur = GetCurrentTaskTssBase();
	cur->counter++;
	if (cur->sleep)
		cur->sleep--;

	cur->delta = cur->delta >> 1;
	cur->priority = cur->priority >> 1;
	cur->delay = cur->delay >> 1;

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
			target_tss->counter++;
			if (target_tss->sleep)
				target_tss->sleep--;

			target_tss->delta = cur->delta >> 1;
			target_tss->priority = cur->priority >> 1;
			target_tss->delay = cur->delay >> 1;
			return target_tss;
		}
	}
	else {
		return 0;
	}

	int cnt = 0;

	double alloc_max = 0.01;
	double mem_max = 0.01;
	float sleep_max = 0.001;

	float delay_delta = 0.01;
	double delay_max = 0.01;
	PROCESS_INFO* ptr_delay_max = 0;
	
	double delta_max = 0.01;
	double authority_max = 0.01;

	int count = 0;

	int sleep_count = 0;

	double cpu_ratio = GetCpuRatio(cpu);

	double task_total = 0.01;

	AlgorithmModel task_ratio[256];

	AlgorithmModel level_buf[256];

	TaskPredictParam tpp_buf;

	LPPROCESS_INFO tss_sleep[256];

	unsigned long long ticknow = __krdtsc();

#ifdef TASK_SWITCH_ARRAY
	PROCESS_INFO* tss = GetTaskTssBase();
	LPPROCESS_INFO prev = (LPPROCESS_INFO)(tss + cur->tid);
	LPPROCESS_INFO ptr = prev;
	do {
		if (ptr->status == TASK_TERMINATE) {
			ptr->status = TASK_OVER;
		}
		else if ( cpu != ptr->cpuid) {
			__printf(szout, "%s %d task:%s tid:%d pid:%d status:%x cpu:%d cpuid:%d\r\n", __FUNCTION__, __LINE__,ptr->funcname,ptr->tid,ptr->pid, ptr->status,cpu,ptr->cpuid);
		}
		else if (ptr->status == TASK_RUN && cpu == ptr->cpuid) {
			if (ptr->sleep) {
				tss_sleep[sleep_count++] = ptr;
				if (sleep){
					ptr->sleep--;
				}
			}
			else {
				ptr->counter++;

				if (ptr->tick_total == 0 || g_cpu_tick[cpu] == 0 || ptr->tick_run == 0 ||ptr->param->cmd & TASK_REALTIME ) {
					ptr->delta = DYNAMIC_PRIORITY;
				}
				
				task_ratio[count].fv = 0.0;
				task_ratio[count].id = ptr->tid;

				task_total += ptr->tick_run;

				level_buf[count].id = ptr->tid;
				level_buf[count].fv = 0.0;

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
	if(prevlist == 0) {
		//__printf(szout, "%s %d task:%s cpu:%d tid:%d pid:%d status:%d error\r\n", __FUNCTION__, __LINE__, cur->funcname, cpu, cur->tid, cur->pid, cur->status);
		prevlist = (PROCESS_INFO_LIST * )GetTaskListId(cpu)->list.next;
	}
	LPPROCESS_INFO prev = &prevlist->proc;
	PROCESS_INFO_LIST* list = prevlist;
	do {
		if (list == 0)
			break;
		LPPROCESS_INFO ptr = &list->proc;
		if (ptr&&ptr->status == TASK_RUN) {
			if (ptr->sleep) {
				tss_sleep[sleep_count++] = ptr;
				if (sleep) {
					ptr->sleep--;
				}
			}
			else {
				ptr->counter++;

				if ( ptr->tick_total == 0 || g_cpu_tick[cpu] == 0|| ptr->tick_run == 0 || ptr->param->cmd & TASK_REALTIME) {
					ptr->delta = DYNAMIC_PRIORITY;
				}

				task_total += ptr->tick_run;

				task_ratio[count].fv = 0.0;
				task_ratio[count].id = ptr->tid;

				level_buf[count].id = ptr->tid;
				level_buf[count].fv = 0.0;

				if(ptr->alloc_times+ptr->free_times > alloc_max)
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

				if( ptr->sleep_total / (ptr->counter + 1) > sleep_max)
					sleep_max = ptr->sleep_total / (ptr->counter + 1);

				if (ptr->authority > authority_max)
					authority_max = ptr->authority;

				tss[count++] = ptr;
			}
		}
		else if (ptr->status == TASK_TERMINATE || ptr->status == TASK_OVER) {
			ptr->status = TASK_OVER;
		}
		list = (PROCESS_INFO_LIST*)list->list.next;
		if (cnt++ > 0x1000) {
			__printf(szout, "%s %d task:%s tid:%d pid:%d cpu:%d error\r\n",__FUNCTION__, __LINE__,cur->funcname,cur->tid,cur->pid,cur->cpuid);
			break;
		}
	} while (list && list != prevlist);
#endif
	if (count <= 0) {
		g_task_other_hit++;
		if (sleep_count <= 0 ) {
			__printf(szout, "%s %d task:%s cpu:%d tid:%d pid:%d status:%d error\r\n", __FUNCTION__, __LINE__, cur->funcname, cpu, cur->tid, cur->pid, cur->status);
			return 0;
		}
		else if (cur->status == TASK_OVER) {
			target_tss = tss_sleep[0];
		}
		else {
			return 0;
		}
	}
	else if ( count == 1) {
		g_task_other_hit++;
#ifdef TASK_SWITCH_ARRAY
		target_tss = tss + task_ratio[0].id;
#else
		target_tss = tss[0];
#endif
	}
	else if (count >= 2) {
		for (int i = 0; i < count; i++) {
#ifdef TASK_SWITCH_ARRAY
			int pid = level_buf[i].id;
			//if (g_train_complete == 0) 
			{
				task_ratio[i].fv = task_total / (tss[pid].tick_run + 0.01);
				if (task_ratio[i].fv > STATIC_PRIORITY)
					task_ratio[i].fv = STATIC_PRIORITY;
				level_buf[i].fv += task_ratio[i].fv;

				level_buf[i].fv += cpu_ratio;

				int window_priority = (tss[pid].window == 0 ? 0 : WINDOW_PRIORITY);
				int user_priority = (tss[pid].level == 0 ? USER_PRIORITY : 0);
				level_buf[i].fv += (window_priority + user_priority);
				level_buf[i].fv += tss[pid].delta;
				level_buf[i].fv += tss[pid].priority;
				level_buf[i].fv += tss[pid].authority;
				level_buf[i].fv += STATIC_PRIORITY *((tss[pid].alloc_times + tss[pid].free_times) / alloc_max);
				level_buf[i].fv += STATIC_PRIORITY * ((*tss[pid].lpvasize) / mem_max);
				float sleep_ratio = (float)(tss[pid].sleep_total / (tss[pid].counter + 1)) / (float)sleep_max;
				level_buf[i].fv += sleep_ratio * STATIC_PRIORITY;
				level_buf[i].fv += tss[pid].delay;
			}
#else
			{
				task_ratio[i].fv = task_total / (tss[i]->tick_run + 1);
				if(task_ratio[i].fv > STATIC_PRIORITY)
					task_ratio[i].fv = STATIC_PRIORITY;
				level_buf[i].fv += task_ratio[i].fv;

				level_buf[i].fv += cpu_ratio;

				int window_priority = (tss[i]->window == 0 ? 0 : WINDOW_PRIORITY);
				int user_priority = (tss[i]->level == 0 ? USER_PRIORITY : 0);
				level_buf[i].fv += (window_priority + user_priority);
				level_buf[i].fv += tss[i]->delta;
				level_buf[i].fv += tss[i]->priority;
				level_buf[i].fv += tss[i]->authority;
				level_buf[i].fv += STATIC_PRIORITY * ((tss[i]->alloc_times + tss[i]->free_times) / alloc_max);
				level_buf[i].fv += STATIC_PRIORITY * (*(tss[i]->lpvasize) / mem_max);
				float sleep_ratio = (tss[i]->sleep_total / (tss[i]->counter + 1)) / sleep_max;
				level_buf[i].fv += sleep_ratio * STATIC_PRIORITY;
				level_buf[i].fv += tss[i]->delay;
			}
#endif
		}

		int target_id = -1;
		//if (g_dl_train_complete == 0) 
		{
			QuickSortd(level_buf, 0, count - 1);
			target_id = level_buf[count - 1].id;
		}

		tpp_buf.result = -1;

		int target_idx = -1;
		for (int i = 0; i < count; i++) {
			if (task_ratio[i].id == target_id) {
				target_idx = i;
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
			int pid = task_ratio[i].id;
			if (pid == target_id) {
				tpp_buf.result = i;
			}
			float user_ratio = (float)(tss[pid].level == 0 ? USER_PRIORITY : 0) / (float)STATIC_PRIORITY;
			float window_ratio = (float)(tss[pid].window ? WINDOW_PRIORITY : 0) / (float)STATIC_PRIORITY;
			tpp_buf.task[i].delay = tss[pid].delay / delay_max;

			tpp_buf.task[i].tickrate = task_ratio[i].fv / STATIC_PRIORITY;

			tpp_buf.task[i].cpurate = cpu_ratio / STATIC_PRIORITY;

			tpp_buf.task[i].user = user_ratio;
			tpp_buf.task[i].window = window_ratio;
			tpp_buf.task[i].delta = tss[pid].delta / delta_max;
			tpp_buf.task[i].priority = (float)(tss[pid].priority) / (float)STATIC_PRIORITY;
			tpp_buf.task[i].authority = tss[pid].authority / authority_max;
			float sleep_ratio = (tss[pid].sleep_total / (tss[pid].counter + 1)) / sleep_max;
			tpp_buf.task[i].sleep = sleep_ratio;
			tpp_buf.task[i].mem = *tss[pid].lpvasize / mem_max ;
			tpp_buf.task[i].alloc = ( tss[pid].alloc_times + tss[pid].free_times )/alloc_max;
#else
			if (task_ratio[i].id == target_id) {
				tpp_buf.result = i;
			}
			float user_ratio = (float)(tss[i]->level == 0 ? USER_PRIORITY : 0) / (float)STATIC_PRIORITY;
			float window_ratio = (float)(tss[i]->window ? WINDOW_PRIORITY : 0) / (float)STATIC_PRIORITY;
			tpp_buf.task[i].delay = tss[i]->delay / delay_max;

			tpp_buf.task[i].tickrate = task_ratio[i].fv / STATIC_PRIORITY;

			tpp_buf.task[i].cpurate = cpu_ratio / STATIC_PRIORITY;

			tpp_buf.task[i].user = user_ratio;
			tpp_buf.task[i].window = window_ratio;
			tpp_buf.task[i].delta = tss[i]->delta / delta_max;
			tpp_buf.task[i].priority = (float)(tss[i]->priority) / (float)STATIC_PRIORITY;
			tpp_buf.task[i].authority = tss[i]->authority / authority_max;
			float sleep_ratio = (tss[i]->sleep_total / (tss[i]->counter + 1)) / sleep_max;
			tpp_buf.task[i].sleep = sleep_ratio;
			tpp_buf.task[i].mem = *tss[i]->lpvasize / mem_max;
			tpp_buf.task[i].alloc = (tss[i]->alloc_times + tss[i]->free_times) / alloc_max;
#endif
		}

		if (num == ML_TASK_LIMIT) {
			if (count != ML_TASK_LIMIT) {
				__printf("%s %d counter:%d exceed\r\n", __FUNCTION__, __LINE__, count);

				if (target_idx >= ML_TASK_LIMIT) {
					int ri = __random(0) % ML_TASK_LIMIT;
					tpp_buf.result = ri;
#ifdef TASK_SWITCH_ARRAY	
					int pid = task_ratio[target_idx].id;
					float user_ratio = (float)(tss[pid].level == 0 ? USER_PRIORITY : 0) / (float)STATIC_PRIORITY;
					float window_ratio = (float)(tss[pid].window ? WINDOW_PRIORITY : 0) / (float)STATIC_PRIORITY;				
					tpp_buf.task[ri].tickrate = task_ratio[target_idx].fv / STATIC_PRIORITY;
					tpp_buf.task[ri].cpurate = cpu_ratio / STATIC_PRIORITY;
					tpp_buf.task[ri].user = user_ratio;
					tpp_buf.task[ri].window = window_ratio;
					tpp_buf.task[ri].delta = tss[pid].delta / delta_max;
					tpp_buf.task[ri].priority = (float)(tss[pid].priority) / (float)STATIC_PRIORITY;;
					tpp_buf.task[ri].authority = tss[pid].authority / authority_max;
					float sleep_ratio = (tss[pid].sleep_total / (tss[pid].counter + 1)) /sleep_max;
					tpp_buf.task[ri].sleep = sleep_ratio;
					tpp_buf.task[ri].mem = (*tss[pid].lpvasize) / mem_max;
					tpp_buf.task[ri].alloc =  (tss[pid].alloc_times + tss[pid].free_times)/(double)alloc_max;
					tpp_buf.task[ri].delay = tss[pid].delay / delay_max;
					
					task_ratio[ri].id = pid;
					task_ratio[ri].fv = task_ratio[target_idx].fv;
#else

					float user_ratio = (float)(tss[target_idx]->level == 0 ? USER_PRIORITY : 0) / (float)STATIC_PRIORITY;
					float window_ratio = (float)(tss[target_idx]->window ? WINDOW_PRIORITY : 0) / (float)STATIC_PRIORITY;
					tpp_buf.task[ri].tickrate = task_ratio[target_idx].fv / STATIC_PRIORITY;
					tpp_buf.task[ri].cpurate = cpu_ratio / STATIC_PRIORITY;
					tpp_buf.task[ri].user = user_ratio;
					tpp_buf.task[ri].window = window_ratio;
					tpp_buf.task[ri].delta = tss[target_idx]->delta / delta_max;
					tpp_buf.task[ri].priority = (float)(tss[target_idx]->priority) / (float)STATIC_PRIORITY;
					tpp_buf.task[ri].authority = tss[target_idx]->authority / authority_max;
					float sleep_ratio = (tss[target_idx]->sleep_total / (tss[target_idx]->counter + 1)) / sleep_max;
					tpp_buf.task[ri].sleep = sleep_ratio;
					tpp_buf.task[ri].mem = (*tss[target_idx]->lpvasize) / mem_max;
					tpp_buf.task[ri].alloc = (tss[target_idx]->alloc_times + tss[target_idx]->free_times)/alloc_max;
					tpp_buf.task[ri].delay = tss[target_idx]->delay / delay_max;
					task_ratio[ri].id = task_ratio[target_idx].id;
					task_ratio[ri].fv = task_ratio[target_idx].fv;
#endif
				}
			}
		}
		else {
			for (int i = num; i < ML_TASK_LIMIT; i++) {
				tpp_buf.task[i].tickrate = 0.0;
				tpp_buf.task[i].cpurate = 0.0;
				tpp_buf.task[i].user = 0.0;
				tpp_buf.task[i].window = 0.0;
				tpp_buf.task[i].delta = 0.0;
				tpp_buf.task[i].priority = 0.0;
				tpp_buf.task[i].authority = 0.0;
				tpp_buf.task[i].sleep = 0.0;
				tpp_buf.task[i].mem = 0.0;
				tpp_buf.task[i].alloc = 0.0;
				tpp_buf.task[i].delay = 0.0;
			}
		}

		if (g_dl_train_complete == 0  ) { 
			if (sleep || (count >= ML_TASK_LIMIT / g_cpu_total  && g_task_pre_total % TASK_SAMPLE_FRACTOR == 0) )
			{
#ifdef MODEL_TYPE_RNN
				CollectRNNSample(&tpp_buf, count);
#else
				CollectDlSample(&tpp_buf, count);
#endif			
			}
			g_task_train_hit++;
		}
		else {
			int seq = TaskSchedulePredict(&tpp_buf);
			if (seq >= 0 && seq < count) {
				if (g_td_mix_enable) {
					int id = task_ratio[seq].id;
					for (int i = 0; i < count; i++) {
						if (id == level_buf[i].id) {
							level_buf[i].fv += PREDICTION_PRIORITY;
							break;
						}
					}
					QuickSortd(level_buf, 0, count - 1);
					target_id = level_buf[count - 1].id;
				}
				else {
					target_id = task_ratio[seq].id;
				}
				PreMultipleHitRate(count, 3, level_buf, target_id);
				g_task_pre_hit++;
			}
			else {
				target_id = task_ratio[0].id;
				g_task_other_hit++;
				g_tp_error_cnt++;
			}
			//__printf(szout, "%s %d cpu:%d seq:%d ,target_id:%d\r\n",__FUNCTION__, __LINE__, cpu, seq, target_id);
		}
#ifdef TASK_SWITCH_ARRAY
		for (int i = 0; i < count; i++) {
			int tid = task_ratio[i].id;
			if (target_id != task_ratio[i].id) {
				tss[tid].delta += STATIC_PRIORITY;
				if (tss[tid].delta > DYNAMIC_PRIORITY) {

				}
			}
		}
		target_tss = tss + target_id;
#else
		for (int i = 0; i < count; i++) {
			if (target_id != task_ratio[i].id) {
				tss[i]->delta += STATIC_PRIORITY;
				if (tss[i]->delta > DYNAMIC_PRIORITY) {

				}
			}
			else {
				target_idx = i;
			}
		}
		target_tss = tss[target_idx];
		//__printf(szout, "%s %d cpu:%d target_idx:%d target_id:%d function:%s\r\n",__FUNCTION__, __LINE__, cpu, target_idx, target_id, target_tss->funcname);
		//priorityTaskTest(count, cpu, target_id, target_idx, tss);
#endif
	}
	if (target_tss) {
		target_tss->delta >>=1;
		target_tss->authority = target_tss->authority / 2;
		target_tss->delay >>= 1;
	}

	return target_tss;
}


void PreMultipleHitRate(int count,int limit, AlgorithmModel* level_buf,int target_id) {
	int min = count > limit ? limit : count;
	
	for (int i = 0; i < min; i++) {
		if (target_id == level_buf[count - 1 - i].id) {
			g_tp_corrects[i]++;
		}
	}
}


int g_priority_test_cnt = 0;


void priorityTaskTest(int count,int cpu,int target_id,int target_idx, LPPROCESS_INFO *tss, AlgorithmModel* level_buf) {
	char szout[1024];
	g_priority_test_cnt++;
	if (g_priority_test_cnt % 0x100 == 0) {
		
		__printf(szout, "%s %d cpu:%d,tid:%d target_id:%d,target_idx:%d,function:%s\r\n",
			__FUNCTION__, __LINE__, cpu,tss[target_idx]->tid, target_id, target_idx, tss[target_idx]->funcname);
	}
	if (g_priority_test_cnt % 0x200 == 0) {
		for (int i = 0; i < count; i++) {
			__printf(szout, "%d id:%d score:%lf    \r\n", i, (DWORD)level_buf[i].id, level_buf[i].fv);
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

	int cpu = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

	double cpu_ratio = GetCpuRatio(cpu);

	AlgorithmModel task_ratio[256];

	AlgorithmModel level[TASK_LIMIT_TOTAL];

	double task_total = 0.01;
	
	float alloc_max = 0.01;
	float mem_max = 0.01;
	float sleep_max = 0.001;

	//__asm{cli}
	float delay_delta = 0.01;
	double delay_max = 0.01;
	PROCESS_INFO * ptr_delay_max = 0;

	double delta_max = 0.01;
	double authority_max = 0.01;

	unsigned long long ticknow = __krdtsc();

	int ret = __GetSpinlock(cur->lplock);
	if (ret == 0)
		return 0;

	__enterSpinlock(&g_tp_lock[cpu]);

	PROCESS_INFO* tss_sleep[256];
	int sleep_count = 0;

	int total = 0;

	for (int num = 0; num < TASK_PREDICTION_BUF_SIZE; num++) {

		if (g_task_predict_buf[cpu][num] == 0) {

			int count = 0;
#ifdef TASK_SWITCH_ARRAY
			PROCESS_INFO* tss = GetTaskTssBase();
			LPPROCESS_INFO prev = (LPPROCESS_INFO)(tss + cur->tid);
			LPPROCESS_INFO ptr = prev;
			do {
				if (cpu == ptr->cpuid) {
					if (ptr->status == TASK_TERMINATE) {
						ptr->status = TASK_OVER;
					}
					else if (ptr->status == TASK_RUN) {
						if (ptr->sleep) {
							tss_sleep[sleep_count++] = ptr;
						}
						else {
							if (ptr->tick_total == 0 || g_cpu_tick[cpu] == 0 || ptr->tick_run == 0 ||ptr->param->cmd & TASK_REALTIME) {
								ptr->delta = DYNAMIC_PRIORITY;
							}

							task_ratio[count].fv = 0.0;
							task_ratio[count].id = ptr->tid;

							task_total += ptr->tick_run;

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
			} while (ptr != prev);
#else
			PROCESS_INFO* tss[256];		
			PROCESS_INFO_LIST* prevlist = (PROCESS_INFO_LIST*)GetRemoteTaskListTid(cpu,cur->tid,0);
			if (prevlist == 0)
				prevlist = (PROCESS_INFO_LIST*)GetTaskListId(cpu)->list.next;
			PROCESS_INFO* prev = (PROCESS_INFO*)&prevlist->proc;
			PROCESS_INFO_LIST* list = prevlist;
			ptr_delay_max = prev;
			do {
				if (list == 0)
					break;
				LPPROCESS_INFO ptr = &list->proc;
				if (ptr->status == TASK_TERMINATE) {
					ptr->status = TASK_OVER;
				}
				else if (ptr->status == TASK_RUN) {
					if (ptr->sleep) {
						tss_sleep[sleep_count++]=ptr;
					}
					else {
						if (ptr->tick_total == 0 || g_cpu_tick[cpu] == 0 || ptr->tick_run == 0 || (ptr->param->cmd & TASK_REALTIME)) {
							ptr->delta = DYNAMIC_PRIORITY;
						}

						task_ratio[count].fv = 0.0;
						task_ratio[count].id = ptr->tid;

						level[count].id = ptr->tid;
						level[count].fv = 0;

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

						tss[count++] = ptr;
					}
				}
				list = (PROCESS_INFO_LIST*)list->list.next;
			} while (list && list != prevlist);
#endif
			if (count < 0) {
				g_task_other_hit++;
				if (sleep_count <= 0) {
					__printf(szout, "%s %d task:%s cpu:%d tid:%d status:%d error\r\n", __FUNCTION__, __LINE__, cur->funcname, cpu, cur->tid, cur->status);
					target_tss = 0;
				}
				else if(cur->status == TASK_OVER) {
					target_tss = tss_sleep[0];
				}
				else {
					target_tss = prev;
				}
			}
			else if (count == 1) {
#ifdef TASK_SWITCH_ARRAY
				target_tss = tss + task_ratio[0].id;
#else
				target_tss = tss[0];
#endif
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
					int pid = task_ratio[i].id;
					float user_ratio = (float)(tss[pid].level == 0 ? USER_PRIORITY : 0) / (float)STATIC_PRIORITY;
					float window_ratio = (float)(tss[pid].window ? WINDOW_PRIORITY : 0) / (float)STATIC_PRIORITY;

					task_ratio[i].fv = task_total / (tss[pid].tick_run + 1);
					if (task_ratio[i].fv > STATIC_PRIORITY)
						task_ratio[i].fv = STATIC_PRIORITY;

					tp.task[i].tickrate = task_ratio[i].fv / STATIC_PRIORITY;
					tp.task[i].cpurate = cpu_ratio / STATIC_PRIORITY;

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
					task_ratio[i].fv = task_total / (tss[i]->tick_run + 1);
					if (task_ratio[i].fv > STATIC_PRIORITY)
						task_ratio[i].fv = STATIC_PRIORITY;

					float user_ratio = (float)(tss[i]->level == 0 ? USER_PRIORITY : 0) / (float)STATIC_PRIORITY;
					float window_ratio = (float)(tss[i]->window ? WINDOW_PRIORITY : 0) / (float)STATIC_PRIORITY;

					tp.task[i].tickrate = task_ratio[i].fv / STATIC_PRIORITY;
					tp.task[i].cpurate = cpu_ratio / STATIC_PRIORITY;
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

						for (int i = 0; i < count; i++) {
#ifdef TASK_SWITCH_ARRAY
							int pid = task_ratio[i].id;
							level[i].id = task_ratio[i].id ;
							level[i].fv += task_ratio[i].fv ;
							level[i].fv += cpu_ratio ;
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
							level[i].id = task_ratio[i].id;
							level[i].fv += task_ratio[i].fv  ;
							level[i].fv += cpu_ratio ;
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
							if (task_ratio[i].id == target_id) {
								index = i;
								break;
							}
						}

						if (index >= ML_TASK_LIMIT) {
							int ri = __random(0) % ML_TASK_LIMIT;
							tp.result = ri;
#ifdef TASK_SWITCH_ARRAY
							int pid = task_ratio[index].id;
							float user_ratio = (float)(tss[pid].level == 0 ? USER_PRIORITY : 0) / (float)STATIC_PRIORITY;
							float window_ratio = (float)(tss[pid].window ? WINDOW_PRIORITY : 0) / (float)STATIC_PRIORITY;				
							tp.task[ri].tickrate = task_ratio[index].fv / STATIC_PRIORITY;
							tp.task[ri].cpurate = cpu_ratio / STATIC_PRIORITY;
							tp.task[ri].user = user_ratio;
							tp.task[ri].window = window_ratio;
							tp.task[ri].delta = tss[pid].delta /delta_max;
							tp.task[ri].priority = (float)(tss[pid].priority) / (float)STATIC_PRIORITY;;
							tp.task[ri].authority = tss[pid].authority /authority_max;;
							float sleep_ratio = (tss[pid].sleep_total / (tss[pid].counter + 1)) /sleep_max;
							tp.task[ri].sleep = sleep_ratio;
							tp.task[ri].mem = (*(tss[pid].lpvasize) / mem_max);
							tp.task[ri].alloc = (tss[pid].alloc_times + tss[pid].free_times) / alloc_max;
							tp.task[ri].delay = tss[pid].delay;				
							task_ratio[ri].id = pid;
							task_ratio[ri].fv = task_ratio[index].fv;
#else

							float user_ratio = (float)(tss[index]->level == 0 ? USER_PRIORITY : 0) / (float)STATIC_PRIORITY;
							float window_ratio = (float)(tss[index]->window ? WINDOW_PRIORITY : 0) / (float)STATIC_PRIORITY;
							tp.task[ri].tickrate = task_ratio[index].fv / STATIC_PRIORITY;
							tp.task[ri].cpurate = cpu_ratio / STATIC_PRIORITY;
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
							task_ratio[ri].id = task_ratio[index].id;
							task_ratio[ri].fv = task_ratio[index].fv;
#endif
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
						tp.task[i].sleep = 0.0;
						tp.task[i].mem = 0.0;
						tp.task[i].alloc = 0.0;
						tp.task[i].delay = 0.0;
					}
				}		
				
				int seq = TaskSchedulePredict(&tp);
				if (seq >= 0 && seq < count) {
					target_id = task_ratio[seq].id;
					total++;
					PreMultipleHitRate(count, 3, level, target_id);
					g_task_pre_hit++;
				}
				else {
					target_id = task_ratio[0].id;
					g_tp_error_cnt++;
					g_task_other_hit++;
				}
#ifdef TASK_SWITCH_ARRAY
				target_tss = tss + target_id;
				for (int i = 0; i < count; i++) {
					int tid = task_ratio[i].id;
					if (target_id != task_ratio[i].id) {
						tss[tid].delta += STATIC_PRIORITY;
						if (tss[tid].delta > DYNAMIC_PRIORITY) {

						}
					}
				}
#else
				int target_idx = -1;
				for (int i = 0; i < count; i++) {
					if (target_id != task_ratio[i].id) {
						tss[i]->delta += STATIC_PRIORITY;
						if (tss[i]->delta > DYNAMIC_PRIORITY) {

						}
					}
					else {
						target_idx = i;
					}
				}
				target_tss = tss[target_idx];
#endif
			}

			g_task_predict_buf[cpu][num] = target_tss;
		}
	}

	__leaveSpinlock(&g_tp_lock[cpu]);

	__leaveSpinlock(cur->lplock);
	//__asm{sti}
	return total;
}
