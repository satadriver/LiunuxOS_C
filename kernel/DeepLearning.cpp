
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
	if (g_ml_data == 0 && g_ml_data_cnt == 0) {
		g_ml_data = (TaskPredictParam*)__kMalloc(TASK_PREDICTION_TRAIN *sizeof(TaskPredictParam));
	}

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



extern "C" __declspec(dllexport) int __kMachineLearning_common(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char szout[256];
	printf("%s %d\r\n", __FUNCTION__, __LINE__);

	int i, k, max_bit = 20, n_samples = 30000, mask = (1 << max_bit) - 1, n_err, max_k;
	float** x, ** y, max, * x1;
	kad_node_t* t;
	kann_t* ann;
	// construct an MLP with one hidden layers
	t = kann_layer_input(max_bit);

	t = kad_relu(kann_layer_dense(t, 64));

	t = kann_layer_cost(t, max_bit + 1, KANN_C_CEM); // output uses 1-hot encoding

	ann = kann_new(t, 0);

	// generate training data
	x = (float**)calloc(n_samples, sizeof(float*));
	y = (float**)calloc(n_samples, sizeof(float*));
	for (i = 0; i < n_samples; ++i) {
		int c, a = kad_rand(0) & (mask >> 1);
		x[i] = (float*)calloc(max_bit, sizeof(float));
		y[i] = (float*)calloc(max_bit + 1, sizeof(float));
		if (x[i] == 0 || y[i] == 0) {
			
		}
		//printf("x[%d]:%x,y[%d]:%x\r\n", i, x[i], i, y[i]);
		for (k = c = 0; k < max_bit; ++k)
			x[i][k] = (float)(a >> k & 1), c += (a >> k & 1);
		y[i][c] = 1.0f; // c is ranged from 0 to max_bit inclusive
	}

	// train
	kann_train_fnn1(ann, 0.001f, 64, 20, 10, 0.1f, n_samples, x, y);

	// predict
	x1 = (float*)calloc(max_bit, sizeof(float));
	for (i = n_err = 0; i < n_samples; ++i) {
		int c, a = kad_rand(0) & (mask >> 1); // generating a new number
		const float* y1;
		for (k = c = 0; k < max_bit; ++k)
			x1[k] = (float)(a >> k & 1), c += (a >> k & 1);
		y1 = kann_apply1(ann, x1);
		for (k = 0, max_k = -1, max = -1.0f; k <= max_bit; ++k) // find the max
			if (max < y1[k]) max = y1[k], max_k = k;
		if (max_k != c) ++n_err;
	}
	printf( "Test error rate: %lf\r\n", 100.0 * n_err / n_samples);
	kann_delete(ann); // TODO: also to free x, y and x1
	return 0;
}
