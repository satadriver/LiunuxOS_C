

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


extern "C" __declspec(dllexport) int __kMachineLearning(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	char szout[256];
	printf("%s %d\r\n", __FUNCTION__, __LINE__);

	int i, k, max_bit = 20, n_samples = 30000, mask = (1 << max_bit) - 1, n_err, max_k;
	float** x, ** y, max, * x1;
	kad_node_t* t;
	kann_t* ann;
	// construct an MLP with one hidden layers
	t = kann_layer_input(max_bit);
	printf("%s %d\r\n", __FUNCTION__, __LINE__);
	t = kad_relu(kann_layer_dense(t, 64));
	printf("%s %d\r\n", __FUNCTION__, __LINE__);
	t = kann_layer_cost(t, max_bit + 1, KANN_C_CEM); // output uses 1-hot encoding
	printf("%s %d\r\n", __FUNCTION__, __LINE__);
	ann = kann_new(t, 0);
	printf("%s %d\r\n", __FUNCTION__, __LINE__);
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
	printf("%s %d\r\n", __FUNCTION__, __LINE__);
	// train
	kann_train_fnn1(ann, 0.001f, 64, 50, 10, 0.1f, n_samples, x, y);
	printf("%s %d\r\n", __FUNCTION__, __LINE__);
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


