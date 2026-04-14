
#include "ml.h"
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

#define		TASK_PREDICTION_TRAIN	(8192)



TaskPredictParam* g_ml_data = 0;

int g_ml_data_cnt = 0;


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

extern "C" __declspec(dllexport) int __kMachineLearning_mlp(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) 
{
	printf("%s %d entry\r\n", __FUNCTION__, __LINE__);

	for (int i = 0; i < 3; i++) {
		TASKCMDPARAMS cmd2;
		__memset((char*)&cmd2, 0, sizeof(TASKCMDPARAMS));
		DWORD ml_addr2 = getAddrFromName(KERNEL_DLL_BASE, "TestThread2");
		__ipiCreateThread((unsigned int)ml_addr2, KERNEL_DLL_BASE, (DWORD)&cmd2, "TestThread2");
	}
	for (int i = 0; i < 3; i++) {
		TASKCMDPARAMS cmd1;
		__memset((char*)&cmd1, 0, sizeof(TASKCMDPARAMS));
		DWORD ml_addr1 = getAddrFromName(KERNEL_DLL_BASE, "TestThread1");
		__ipiCreateThread((unsigned int)ml_addr1, KERNEL_DLL_BASE, (DWORD)&cmd1, "TestThread1");
	}
	for (int i = 0; i < 3; i++) {
		TASKCMDPARAMS cmd3;
		__memset((char*)&cmd3, 0, sizeof(TASKCMDPARAMS));
		DWORD ml_addr3 = getAddrFromName(KERNEL_DLL_BASE, "TestThread3");
		__ipiCreateThread((unsigned int)ml_addr3, KERNEL_DLL_BASE, (DWORD)&cmd3, "TestThread3");
	}
	int imageSize = getSizeOfImage((char*)MAIN_DLL_BASE);
	for(int i = 0; i < 1; ++i) {
		DWORD addr = getAddrFromName(MAIN_DLL_BASE, "TestThread1_main");
		if (addr) 
		{
			__kCreateProcess(MAIN_DLL_SOURCE_BASE, imageSize, "main.dll", "TestThread1_main", 3, 0);
			__sleep(100);
		}
	}

	for (int i = 0; i < 1; ++i) {
		DWORD addr = getAddrFromName(MAIN_DLL_BASE, "TestThread2_main");
		if (addr) 
		{
			__kCreateProcess(MAIN_DLL_SOURCE_BASE, imageSize, "main.dll", "TestThread2_main", 3, 0);
			__sleep(100);
		}
	}
	for (int i = 0; i < 1; ++i) {
		DWORD addr = getAddrFromName(MAIN_DLL_BASE, "TestThread3_main");
		if (addr)
		{
			__kCreateProcess(MAIN_DLL_SOURCE_BASE, imageSize, "main.dll", "TestThread3_main", 3, 0);
			__sleep(100);
		}
	}

	while (g_ml_data_cnt < TASK_PREDICTION_TRAIN) {
		__sleep(1000);
	}

	char szout[256];

	printf("%s %d start\r\n", __FUNCTION__, __LINE__);
	int cnt = 0;
	int i = 0;
	int inSize = sizeof(TaskPredictParam) / sizeof(float) - 1;
	int n_samples = TASK_PREDICTION_TRAIN;
	int outSize = ML_TASK_LIMIT;
	float** x, ** y, max, * x1;
	kad_node_t* t;
	kann_t* ann;
	// construct an MLP with one hidden layers
	t = kann_layer_input(inSize);

	t = kad_relu(kann_layer_dense(t, 64));
	//t = kad_relu(kann_layer_dense(t, 64));
	//t = kad_relu(kann_layer_dense(t, 64));

	//t = kann_layer_cost(t, 1, KANN_C_CEM); // output uses 1-hot encoding
	t = kann_layer_cost(t, outSize, KANN_C_CEM); // output uses 1-hot encoding

	ann = kann_new(t, 0);

	// generate training data
	x = (float**)calloc(n_samples, sizeof(float*));
	y = (float**)calloc(n_samples, sizeof(float*));
	for (i = 0; i < n_samples; ++i) {

		x[i] = (float*)calloc(inSize, sizeof(float));
		__memcpy((char*)x[i], (char*) & g_ml_data[i], sizeof(TaskPredictParam) - sizeof(float));

		y[i] = (float*)calloc(outSize, sizeof(float));
		for (int j = 0; j < outSize; j++) {
			y[i][j] = 0.0;
		}
		int idx  = g_ml_data[i].result;
		y[i][idx] = 1.0;
	}

	// train
	kann_train_fnn1(ann, 0.001f, 64, 50, 10, 0.1f, n_samples, x, y);

	// predict
	n_samples = TASK_PREDICTION_TRAIN;
	x1 = (float*)calloc(inSize, sizeof(float));
	int n_err = 0;
	for (i = 0; i < n_samples; ++i) {
		__memcpy((char*)x1, (char*)&g_ml_data[i], sizeof(TaskPredictParam) - sizeof(float));

		const float* y1 = kann_apply1(ann, x1);

		float max = -1.0;
		int num = 0;
		for (int j = 0; j < outSize; j++) {
			if (y1[j] > max) {
				max = y1[j];
				num = j;
			}
		}
#ifdef _DEBUG
		for (int j = 0; j < 16; j++) {
			printf("%d y:%lf tick:%f user:%f window:%f delta:%f priority:%f max:%f result:%d num:%d\r\n",
				j,y1[j],
				g_ml_data[i].task[j].tick, g_ml_data[i].task[j].user, g_ml_data[i].task[j].window,
				g_ml_data[i].task[j].delta, g_ml_data[i].task[j].priority,max, g_ml_data[i].result,num);
		}
#endif
		if(num != g_ml_data[i].result) {
			n_err++;
		}
	}
	printf("Test error rate: %lf\r\n", 100.0 * n_err / n_samples);
	kann_delete(ann); // TODO: also to free x, y and x1
	return 0;
}

typedef struct {
	int n_in, ulen;
	int n, m;
	uint64_t* x, * y;
} bit_data_t;

static void train(kann_t* ann, bit_data_t* d, float lr, int mini_size, int max_epoch, const char* fn, int n_threads)
{
	float** x, ** y, * r, best_cost = 1e30f;
	int epoch, j, n_var, * shuf;
	kann_t* ua;

	n_var = kann_size_var(ann);
	r = (float*)calloc(n_var, sizeof(float));
	x = (float**)malloc(d->ulen * sizeof(float*));
	y = (float**)malloc(d->ulen * sizeof(float*));
	for (j = 0; j < d->ulen; ++j) {
		x[j] = (float*)calloc(mini_size * d->n_in, sizeof(float));
		y[j] = (float*)calloc(mini_size * 2, sizeof(float));
	}
	shuf = (int*)calloc(d->n, sizeof(int));
	kann_shuffle(d->n, shuf);

	ua = kann_unroll(ann, d->ulen);
	kann_set_batch_size(ua, mini_size);
	kann_mt(ua, n_threads, mini_size);
	kann_feed_bind(ua, KANN_F_IN, 0, x);
	kann_feed_bind(ua, KANN_F_TRUTH, 0, y);
	kann_switch(ua, 1);
	for (epoch = 0; epoch < max_epoch; ++epoch) {
		double cost = 0.0;
		int tot = 0, tot_base = 0, n_cerr = 0;
		for (j = 0; j < d->n - mini_size; j += mini_size) {
			int i, b, k;
			for (k = 0; k < d->ulen; ++k) {
				for (b = 0; b < mini_size; ++b) {
					int s = shuf[j + b];
					for (i = 0; i < d->n_in; ++i)
						x[k][b * d->n_in + i] = (float)(d->x[s * d->n_in + i] >> k & 1);
					y[k][b * 2] = y[k][b * 2 + 1] = 0.0f;
					y[k][b * 2 + (d->y[s] >> k & 1)] = 1.0f;
				}
			}
			cost += kann_cost(ua, 0, 1) * d->ulen * mini_size;
			n_cerr += kann_class_error(ua, &k);
			tot_base += k;
			//kad_check_grad(ua->n, ua->v, ua->n-1);
			kann_RMSprop(n_var, lr, 0, 0.9f, ua->g, ua->x, r);
			tot += d->ulen * mini_size;
		}
		if (cost < best_cost) {
			best_cost = cost;
			if (fn) kann_save(fn, ann);
		}
		fprintf(stderr, "epoch: %d; cost: %g (class error: %.2f%%)\n", epoch + 1, cost / tot, 100.0f * n_cerr / tot_base);
	}

	for (j = 0; j < d->ulen; ++j) {
		free(y[j]); free(x[j]);
	}
	free(y); free(x); free(r); free(shuf);
}



extern "C" __declspec(dllexport) int __kMachineLearning_rnn(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param)
{
	int i, c, seed = 11, n_h_layers = 1, n_h_neurons = 64, mini_size = 64, max_epoch = 30, to_apply = 0, norm = 1, n_threads = 1;
	float lr = 0.01f, dropout = 0.2f;
	kann_t* ann = 0;
	char* fn_in = 0, * fn_out = 0;

	int inSize = sizeof(TaskPredictParam) / sizeof(float) - 1;
	int blockSize = sizeof(TaskPredictParam) - sizeof(float);

	int n_samples = TASK_PREDICTION_TRAIN;
	int outSize = 1;

	kad_node_t* t;
	int rnn_flag = KANN_RNN_VAR_H0;
	if (norm) rnn_flag |= KANN_RNN_NORM;
	bit_data_t *d = (bit_data_t*)malloc(sizeof(bit_data_t));
	d->n_in = inSize;
	d->m = TASK_PREDICTION_TRAIN;
	d->n = TASK_PREDICTION_TRAIN;
	d->ulen = 32;
	d->x = (unsigned long long*)malloc(inSize*sizeof(float) * TASK_PREDICTION_TRAIN);
	d->y = (unsigned long long*)malloc(sizeof(float)* TASK_PREDICTION_TRAIN);

	if (g_ml_data == 0) {
		g_ml_data = (TaskPredictParam*)malloc(TASK_PREDICTION_TRAIN *sizeof(TaskPredictParam));
	}

	for (i = 0; i < n_samples; ++i) {

		__memcpy((char*)d->x + i* blockSize, (char*)&g_ml_data[i], blockSize);

		int idx = g_ml_data[i].result;
		d->y[i] = idx*1.0;
	}

	t = kann_layer_input(d->n_in);
	for (i = 0; i < n_h_layers; ++i) {
		t = kann_layer_gru(t, n_h_neurons, rnn_flag);
		t = kann_layer_dropout(t, dropout);
	}
	ann = kann_new(kann_layer_cost(t, 2, KANN_C_CEM), 0);
	
	train(ann, d, lr, mini_size, max_epoch, fn_out, n_threads);

	return 0;
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


extern "C" __declspec(dllexport) int TestThread1(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	
	while (1) {
		__sleep(0);
		__asm {
			//hlt
		}
	}

	return 0;
}
extern "C" __declspec(dllexport) int TestThread2(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param)
{
	float f1 = PI;
	while (1) {
		f1 = sinf(f1/3);
		if(f1 < 0.00001f && f1 > -0.00001f) {
			f1 = PI;
		}
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestThread3(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (1) {
		DWORD tick = __random(0);
		memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}