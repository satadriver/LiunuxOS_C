#include "function.h"

int g_function_c = 4;

int function_c(int x,int y){

	int result = x * y *g_function_c * 4;
	
	return result;
}
