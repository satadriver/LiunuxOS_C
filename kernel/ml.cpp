

#include "kann-master/kann.h"
#include "math.h"

extern "C" __declspec(dllexport) int __kMachineLearning(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {

	kann_t* ann;
	kad_node_t* t;
	t = kann_layer_input(784); // for MNIST

	//kad_node_t* knt = kann_layer_dense(t, 64);
	//knt = kad_relu(knt); // a 64-neuron hidden layer with ReLU activation


	//t = kann_layer_cost(t, 10, KANN_C_CEM); // softmax output + multi-class cross-entropy cost
	//ann = kann_new(t, 0);                   // compile the network and collate variables

	char szout[256];

	double x = 100.0;
	for (int i = 0; i < 1; i++) {
		x = log(x);
	}
	
	double y = 100.0;
	for (int i = 0; i < 3; i++) {
		y = sqrt(y);
	}


	__printf(szout,"%s %d x:%lf y:%lf\r\n", __FUNCTION__, __LINE__,x,y);




	return 0;
}