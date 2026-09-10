
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



int g_train_complete = 0;

TaskPredictParam* g_ml_data = 0;

int g_ml_data_cnt = 0;

kann_t* g_dl_ann = 0;


int SaveMlData(TaskPredictParam * tp)
{

	if (g_ml_data != 0 && g_ml_data_cnt < TASK_PREDICTION_TRAIN) {
		__memcpy((char*)&g_ml_data[g_ml_data_cnt], (char*)tp,sizeof(TaskPredictParam));

		if (g_ml_data_cnt % 100 == 0) {

		}

		g_ml_data_cnt++;
	}

	return g_ml_data_cnt;
}





int TaskSwitchPrediction(TaskPredictParam* tp) {
	int inSize = sizeof(TaskPredictParam) / sizeof(float) - 1;
	int n_samples = TASK_PREDICTION_TRAIN;
	int outSize = ML_TASK_LIMIT;
	
	int n_err = 0;

	const float* y1 = kann_apply1(g_dl_ann, (float*)tp);

	float max = -1.0;
	int num = 0;
	for (int j = 0; j < outSize; j++) {
		if (y1[j] > max) {
			max = y1[j];
			num = j;
		}
	}
	return num;
}




