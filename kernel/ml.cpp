
#include "ml.h"
#include "kann-master/kann.h"
#ifdef _DEBUG

#include "math.h"
#include "libc.h"
#else
#include "math.h"
#endif
//#include <windows.h>

#include "math.h"
#include "libc.h"
#include "malloc.h"
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


#include <stdlib.h>
#define malloc mymalloc
#define free myfree
#define realloc __realloc
#define calloc __calloc

// to compile and run: gcc -O2 this-prog.c kann.c kautodiff.c -lm && ./a.out

#define		TASK_PREDICTION_TRAIN	(1024)
#define		TASK_PREDICTION_TEST	(1024)
#define		TASK_PREDICTION_COUNT	(TASK_PREDICTION_TRAIN+TASK_PREDICTION_TEST)

TaskPredictParam* g_ml_data = 0;

int g_ml_data_cnt = 0;



int SaveMlData(float tick, float user, float window, float delta, float priority,
	float ntick, float nuser, float nwindow, float ndelta, float npriority, float result)
{
	if (g_ml_data == 0 && g_ml_data_cnt == 0) {
		g_ml_data = (TaskPredictParam*)__kMalloc(TASK_PREDICTION_COUNT *sizeof(TaskPredictParam));
	}
	if (g_ml_data != 0) {
		if (g_ml_data_cnt < TASK_PREDICTION_COUNT) {
			g_ml_data[g_ml_data_cnt].tick = tick;
			g_ml_data[g_ml_data_cnt].user = user;
			g_ml_data[g_ml_data_cnt].window = window;
			g_ml_data[g_ml_data_cnt].delta = delta;
			g_ml_data[g_ml_data_cnt].priority = priority;

			g_ml_data[g_ml_data_cnt].result = result;

			g_ml_data[g_ml_data_cnt].ntick = ntick;
			g_ml_data[g_ml_data_cnt].nuser = nuser;
			g_ml_data[g_ml_data_cnt].nwindow = nwindow;
			g_ml_data[g_ml_data_cnt].ndelta = ndelta;
			g_ml_data[g_ml_data_cnt].npriority = npriority;

			if(g_ml_data_cnt % 100 == 0) {
				printf("%s %d cnt:%d tick:%f user:%f window:%f delta:%f priority:%f ntick:%f nuser:%f nwindow:%f ndelta:%f npriority:%f result:%f\r\n", 
					__FUNCTION__, __LINE__, g_ml_data_cnt, tick, user, window, delta, priority, 
					ntick, nuser, nwindow, ndelta, npriority, result);
			}

			g_ml_data_cnt++;
		}
	}

	return g_ml_data_cnt;
}

extern "C" __declspec(dllexport) int __kMachineLearning(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) 
{
	printf("%s %d entry\r\n", __FUNCTION__, __LINE__);
	while (g_ml_data_cnt < TASK_PREDICTION_COUNT) {
		__sleep(100);
	}

	char szout[256];

	printf("%s %d start\r\n", __FUNCTION__, __LINE__);
	int cnt = 0;
	int i = 0;
	int inSize = ( sizeof(TaskPredictParam) / sizeof(float) - 1) ;
	int n_samples = TASK_PREDICTION_TRAIN;
	int outSize = 1;
	float** x, ** y, max, * x1;
	kad_node_t* t;
	kann_t* ann;
	// construct an MLP with one hidden layers
	t = kann_layer_input(inSize);

	t = kad_relu(kann_layer_dense(t, 64));
	//t = kad_relu(kann_layer_dense(t, 64));
	//t = kad_relu(kann_layer_dense(t, 64));

	//t = kann_layer_cost(t, 1, KANN_C_CEM); // output uses 1-hot encoding
	t = kann_layer_cost(t, 1, KANN_C_CEB); // output uses 1-hot encoding

	ann = kann_new(t, 0);

	// generate training data
	x = (float**)calloc(n_samples, sizeof(float*));
	y = (float**)calloc(n_samples, sizeof(float*));
	for (i = 0; i < n_samples; ++i) {

		x[i] = (float*)calloc(inSize, sizeof(float));
		__memcpy((char*)x[i], (char*) & g_ml_data[i], sizeof(TaskPredictParam) - sizeof(float));

		y[i] = (float*)calloc(outSize, sizeof(float));
		float v = (int)g_ml_data[i].result;
		y[i][0] = v;		
	}

	// train
	kann_train_fnn1(ann, 0.001f, 64, 10, 10, 0.1f, n_samples, x, y);

	// predict
	n_samples = TASK_PREDICTION_COUNT;
	x1 = (float*)calloc(inSize, sizeof(float));
	int n_err = 0;
	for (i = TASK_PREDICTION_TEST; i < n_samples; ++i) {
		__memcpy((char*)x1, (char*)&g_ml_data[i], sizeof(TaskPredictParam) - sizeof(float));

		const float* y1;
		y1 = kann_apply1(ann, x1);

		if (*y1 - g_ml_data[i].result >= 1e-6 || *y1 - g_ml_data[i].result <= -1e-6)
			++n_err;

		if (cnt++ % 10 == 0) {
			printf("predict:%f, truth:%f\r\n", *y1, g_ml_data[i].result);
		}
	}
	printf("Test error rate: %lf\r\n", 100.0 * n_err / n_samples);
	kann_delete(ann); // TODO: also to free x, y and x1
	return 0;
}

extern "C" __declspec(dllexport) int __kMachineLearning_old(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
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


