
#include "dl.h"
#include "kann-master/kann.h"
#include "apic.h"

#ifdef _DEBUG

#include "math.h"
#include "process.h"
#include "systemService.h"
#include "task.h"
#include "Pe.h"
#include "Thread.h"
#include "utils.h"



#else
#include "math.h"
#include "systemService.h"
#include "task.h"
#include "Pe.h"
#include "Thread.h"
#include "process.h"

#include "libc.h"
#include "malloc.h"
#include "deepLearning.h"
#endif

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




extern "C" __declspec(dllexport) int __kDeepLearning_mlp(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) 
{
	
	DWORD tick1 = *(DWORD*)CMOS_PERIOD_TICK_COUNT;
	//printf("%s %d entry\r\n", __FUNCTION__, __LINE__);
	__asm{cli}
	g_train_complete = 0;
	g_dl_rate = 0.0;
	__asm {sti}

	if (g_dl_ann) {
		free(g_dl_ann);
	}
	if (g_ml_data) {
		free(g_ml_data);
		g_ml_data = 0;
	}
	g_ml_data_cnt = 0;
	if (g_ml_data == 0 && g_ml_data_cnt == 0) {
		g_ml_data = (TaskPredictParam*)__kMalloc(TASK_DISPATCH_SAMPLE * sizeof(TaskPredictParam));
		if (g_ml_data == 0) {
			return 0;
		}
	}

	int max_task = ML_TASK_LIMIT ;

	int sleep_time = 0;

	for (int i = 0; i < max_task/2; i++) {
		break;

		char tn[256];
		__sprintf(tn, "TestThread%d", i);
		TASKCMDPARAMS cmd2;
		__memset((char*)&cmd2, 0, sizeof(TASKCMDPARAMS));
		DWORD ml_addr2 = getAddrFromName(MAIN_DLL_BASE, tn);
		if (ml_addr2) {
			__ipiCreateThread((unsigned int)ml_addr2, MAIN_DLL_SOURCE_BASE, (DWORD)&cmd2, tn);
			__sleep(sleep_time);
		}
	}

	int imageSize = getSizeOfImage((char*)MAIN_DLL_BASE);
	for(int i = 0; i < max_task; ++i) {
		char tn[256];
		__sprintf(tn, "TestProcess_%d", i);

		DWORD addr = getAddrFromName(MAIN_DLL_BASE, tn);
		if (addr) 
		{
			__kCreateProcess(MAIN_DLL_SOURCE_BASE, imageSize, "main.dll", tn, 3, 0);
			__sleep(sleep_time);
		}
	}

	while (g_ml_data_cnt < TASK_DISPATCH_SAMPLE) {
		__sleep(1000);
	}
	DWORD tick2 = *(DWORD*)CMOS_PERIOD_TICK_COUNT;
	DWORD sampleTime = tick2 - tick1;

	char szout[256];

	printf("%s %d start\r\n", __FUNCTION__, __LINE__);
	int cnt = 0;
	int i = 0;
	int inSize = sizeof(TaskPredictParam) / sizeof(float) - 1;
	int n_samples = TASK_DISPATCH_SAMPLE;
	int outSize = ML_TASK_LIMIT;
	float** x, ** y, max, * x1;
	kad_node_t* t;
	
	// construct an MLP with one hidden layers
	t = kann_layer_input(inSize);

	t = kad_relu(kann_layer_dense(t, 64));
	//t = kad_relu(kann_layer_dense(t, 64));
	//t = kad_relu(kann_layer_dense(t, 64));

	//t = kann_layer_cost(t, 1, KANN_C_CEM); // output uses 1-hot encoding
	t = kann_layer_cost(t, outSize, KANN_C_CEM); // output uses 1-hot encoding

	g_dl_ann = kann_new(t, 0);

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
		if(idx != -1)
			y[i][idx] = 1.0;
	}

	// train
	kann_train_fnn1(g_dl_ann, 0.001f, 64, 50, 10, 0.1f, n_samples, x, y);

	// predict
	n_samples = TASK_DISPATCH_SAMPLE;
	x1 = (float*)calloc(inSize, sizeof(float));
	int n_err = 0;
	for (i = 0; i < n_samples; ++i) {
		__memcpy((char*)x1, (char*)&g_ml_data[i], sizeof(TaskPredictParam) - sizeof(float));

		const float* y1 = kann_apply1(g_dl_ann, x1);

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
		if (g_ml_data[i].result != -1) {
			if (num != g_ml_data[i].result) {
				n_err++;
			}
		}
	}

	tick1 = *(DWORD*)CMOS_PERIOD_TICK_COUNT;
	DWORD trainTime = tick1 - tick2;

	double error = 100.0 * n_err / n_samples;
	printf("Task Prediction sample:%d, input dimension:%d, output dimension:%d, error rate: %lf%%, sample seconds:%d, train seconds:%d\r\n", 
		TASK_DISPATCH_SAMPLE, inSize,outSize,error, sampleTime,trainTime);
	g_dl_rate = error;
	//kann_delete(ann); // TODO: also to free x, y and x1
	if (error< TASK_DISPATCH_ERROR_RATE) {
		g_train_complete = 1;
	}

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



extern "C" __declspec(dllexport) int __kDeepLearning_rnn(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param)
{
	int i, c, seed = 11, n_h_layers = 1, n_h_neurons = 64, mini_size = 64, max_epoch = 30, to_apply = 0, norm = 1, n_threads = 1;
	float lr = 0.01f, dropout = 0.2f;
	kann_t* ann = 0;
	char* fn_in = 0, * fn_out = 0;

	int inSize = sizeof(TaskPredictParam) / sizeof(float) - 1;
	int blockSize = sizeof(TaskPredictParam) - sizeof(float);

	int n_samples = TASK_DISPATCH_SAMPLE;
	int outSize = 1;

	kad_node_t* t;
	int rnn_flag = KANN_RNN_VAR_H0;
	if (norm) rnn_flag |= KANN_RNN_NORM;
	bit_data_t *d = (bit_data_t*)malloc(sizeof(bit_data_t));
	d->n_in = inSize;
	d->m = TASK_DISPATCH_SAMPLE;
	d->n = TASK_DISPATCH_SAMPLE;
	d->ulen = 32;
	d->x = (unsigned long long*)malloc(inSize*sizeof(float) * TASK_DISPATCH_SAMPLE);
	d->y = (unsigned long long*)malloc(sizeof(float)* TASK_DISPATCH_SAMPLE);

	if (g_ml_data == 0) {
		g_ml_data = (TaskPredictParam*)malloc(TASK_DISPATCH_SAMPLE *sizeof(TaskPredictParam));
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




extern "C" __declspec(dllexport) int TestThread0(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	
	while (g_train_complete==0) {
		__sleep(0);
	}

	return 0;
}

extern "C" __declspec(dllexport) int TestThread1(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param)
{
	float f1 = PI;
	while (g_train_complete == 0) {
		f1 = sin(f1/3);
		if(f1 < 0.00001f && f1 > -0.00001f) {
			f1 = PI;
		}
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestThread2(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestThread3(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestThread4(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestThread5(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestThread6(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestThread7(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestThread8(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {

	while (g_train_complete == 0) {
		__sleep(0);
	}

	return 0;
}

extern "C" __declspec(dllexport) int TestThread9(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param)
{
	float f1 = PI;
	while (g_train_complete == 0) {
		f1 = sin(f1 / 3);
		if (f1 < 0.00001f && f1 > -0.00001f) {
			f1 = PI;
		}
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestThread10(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestThread11(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestThread12(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestThread13(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}
extern "C" __declspec(dllexport) int TestThread14(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestThread15(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}























extern "C" __declspec(dllexport) int TestProcess0(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestProcess1(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];
	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);

	}

	return 0;
}

extern "C" __declspec(dllexport) int TestProcess2(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param)
{
	float f1 = PI;
	while (g_train_complete == 0) {
		f1 = __sinf(f1 / 3);
		if (f1 < 0.00001f && f1 > -0.00001f) {
			f1 = PI;
		}
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestProcess3(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestProcess4(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestProcess5(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestProcess6(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestProcess7(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestProcess8(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestProcess9(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestProcess10(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestProcess11(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestProcess12(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestProcess13(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestProcess14(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestProcess15(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}


extern "C" __declspec(dllexport) int TestProcess16(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestProcess17(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];
	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);

	}

	return 0;
}

extern "C" __declspec(dllexport) int TestProcess18(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param)
{
	float f1 = PI;
	while (g_train_complete == 0) {
		f1 = __sinf(f1 / 3);
		if (f1 < 0.00001f && f1 > -0.00001f) {
			f1 = PI;
		}
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestProcess19(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestProcess20(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestProcess21(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestProcess22(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestProcess23(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestProcess24(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestProcess25(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestProcess26(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestProcess27(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestProcess28(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestProcess29(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestProcess30(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}

extern "C" __declspec(dllexport) int TestProcess31(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char buf[1024];

	while (g_train_complete == 0) {
		DWORD tick = __random(0);
		__memset(buf, (unsigned char)tick, sizeof(buf));
		__sleep(0);
	}
	return 0;
}




#define DEFINE_TEST_PROCESS(N) \
    extern "C" __declspec(dllexport) void TestProcess_##N() { \
        while(g_train_complete == 0 && g_dl_rate == 0.0){__sleep(0); } \
    }

DEFINE_TEST_PROCESS(0)
DEFINE_TEST_PROCESS(1)
DEFINE_TEST_PROCESS(2)
DEFINE_TEST_PROCESS(3)
DEFINE_TEST_PROCESS(4)
DEFINE_TEST_PROCESS(5)
DEFINE_TEST_PROCESS(6)
DEFINE_TEST_PROCESS(7)
DEFINE_TEST_PROCESS(8)
DEFINE_TEST_PROCESS(9)
DEFINE_TEST_PROCESS(10)
DEFINE_TEST_PROCESS(11)
DEFINE_TEST_PROCESS(12)
DEFINE_TEST_PROCESS(13)
DEFINE_TEST_PROCESS(14)
DEFINE_TEST_PROCESS(15)
DEFINE_TEST_PROCESS(16)
DEFINE_TEST_PROCESS(17)
DEFINE_TEST_PROCESS(18)
DEFINE_TEST_PROCESS(19)
DEFINE_TEST_PROCESS(20)
DEFINE_TEST_PROCESS(21)
DEFINE_TEST_PROCESS(22)
DEFINE_TEST_PROCESS(23)
DEFINE_TEST_PROCESS(24)
DEFINE_TEST_PROCESS(25)
DEFINE_TEST_PROCESS(26)
DEFINE_TEST_PROCESS(27)
DEFINE_TEST_PROCESS(28)
DEFINE_TEST_PROCESS(29)
DEFINE_TEST_PROCESS(30)
DEFINE_TEST_PROCESS(31)
DEFINE_TEST_PROCESS(32)
DEFINE_TEST_PROCESS(33)
DEFINE_TEST_PROCESS(34)
DEFINE_TEST_PROCESS(35)
DEFINE_TEST_PROCESS(36)
DEFINE_TEST_PROCESS(37)
DEFINE_TEST_PROCESS(38)
DEFINE_TEST_PROCESS(39)
DEFINE_TEST_PROCESS(40)
DEFINE_TEST_PROCESS(41)
DEFINE_TEST_PROCESS(42)
DEFINE_TEST_PROCESS(43)
DEFINE_TEST_PROCESS(44)
DEFINE_TEST_PROCESS(45)
DEFINE_TEST_PROCESS(46)
DEFINE_TEST_PROCESS(47)
DEFINE_TEST_PROCESS(48)
DEFINE_TEST_PROCESS(49)
DEFINE_TEST_PROCESS(50)
DEFINE_TEST_PROCESS(51)
DEFINE_TEST_PROCESS(52)
DEFINE_TEST_PROCESS(53)
DEFINE_TEST_PROCESS(54)
DEFINE_TEST_PROCESS(55)
DEFINE_TEST_PROCESS(56)
DEFINE_TEST_PROCESS(57)
DEFINE_TEST_PROCESS(58)
DEFINE_TEST_PROCESS(59)
DEFINE_TEST_PROCESS(60)
DEFINE_TEST_PROCESS(61)
DEFINE_TEST_PROCESS(62)
DEFINE_TEST_PROCESS(63)