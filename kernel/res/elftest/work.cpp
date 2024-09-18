#include "work.h"
#include <stdio.h>

int g_workfunc_var = 2;

int workfunc(int x,int y){

	unsigned int result = x*y*g_workfunc_var + 1;
	//printf("workfunc result:%d\r\n",result);
	return result;
}
