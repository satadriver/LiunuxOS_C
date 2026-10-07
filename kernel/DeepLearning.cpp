
#include "DeepLearning.h"
#include "kann-master/kann.h"
#include "apic.h"

#ifdef _DEBUG
#include "process.h"
#include "math.h"
#include "systemService.h"
#include "task.h"
#include "Pe.h"
#include "Thread.h"

#include "libc.h"
#include "malloc.h"
#else
#include "math.h"
#include "systemService.h"
#include "task.h"
#include "Pe.h"
#include "Thread.h"
#include "process.h"
#include "taskPriority.h"
#include "libc.h"
#include "malloc.h"

#endif

//#include <windows.h>

#define sqrt __sqrt
#define sqrtf __sqrtf
#define exp __exp
#define expf __expf
#define log __log
#define sin __sin
#define cos __cos
#define sinf __sinf
#define cosf __cosf
#define fabs __fabs
#define fabsf __fabsf
#define logf __logf


#define malloc my_malloc
#define free my_free
#define realloc my_realloc
#define calloc my_calloc

#define memcpy my_memcpy
#define memset	my_memset

#define abort my_abort

#define printf my_printf
#define fprintf my_fprintf

#define fread my_fread
#define fopen my_fopen
#define fwrite my_fwrite
#define fclose my_fclose
#define strcmp my_strcmp
#define strcat my_strcat
#define strlen my_strlen
#define strcpy my_strcpy
#define strncmp my_strncmp

#define wcslen my_wcslen
#define wcscmp my_wcscmp
#define wcscat my_wcscat
#define wcsstr my_wcsstr
#define wcscpy my_wcscpy

#define fputc my_fputc
#define fgetc my_fgetc
#define fgets my_fgets
#define fputs my_fputs


// to compile and run: gcc -O2 this-prog.c kann.c kautodiff.c -lm && ./a.out

int g_dl_tp_mix = 0;

int g_dl_lock = 0;

int g_dl_train_complete = 0;

TaskPredictParam* g_dl_data = 0;

int g_dl_data_cnt = 0;

kann_t* g_dl_ann = 0;

double g_dl_rate = 0.0;


int CollectDlSample(TaskPredictParam * tp)
{
	__enterSpinlock(&g_dl_lock);

	if (g_dl_data != 0 && g_dl_data_cnt < TASK_DISPATCH_SAMPLE) {
		__memcpy((char*)&g_dl_data[g_dl_data_cnt], (char*)tp,sizeof(TaskPredictParam));

		g_dl_data_cnt++;
	}

	__leaveSpinlock(&g_dl_lock);

	//printf("%s %d g_dl_data_cnt:%d\r\n", __FUNCTION__, __LINE__, g_dl_data_cnt);

	DWORD pos = (gVideoHeight - GRAPHCHAR_HEIGHT * 3) * gVideoWidth * gBytesPerPixel + (gVideoWidth / 2) * gBytesPerPixel;
	char szout[256];
	__sprintf(szout, (char*)"g_dl_data_cnt:%x ", g_dl_data_cnt);
	__drawGraphChar((char*)szout, 0, pos, TASKBARCOLOR);

	return g_dl_data_cnt;
}




int TaskSchedulePredict(TaskPredictParam* tp) {
	
	unsigned long long tick1 = __krdtsc();
	//int inSize = sizeof(TaskPredictParam) / sizeof(float) - 1;
	//int n_samples = TASK_DISPATCH_SAMPLE;
	if (g_dl_train_complete == 0 || g_dl_rate == 0.0) {
		return -1;
	}
	int outSize = ML_TASK_LIMIT;
	
	//int n_err = 0;
	__enterSpinlock(&g_dl_lock);

	const float* y1 = kann_apply1(g_dl_ann, (float*)tp);

	float max = -1.0;
	int num = -1;
	for (int j = 0; j < outSize; j++) {
		if (y1[j] >= max) {
			max = y1[j];
			num = j;
		}
	}
	unsigned long long tick2 = __krdtsc();
	g_task_pre_cost = (g_task_pre_cost + tick2 - tick1) / 2;

	__leaveSpinlock(&g_dl_lock);
	return num;
}




