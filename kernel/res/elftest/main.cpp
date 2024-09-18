#include <stdio.h>
#include <dlfcn.h>

#include <stdlib.h>
#include <cerrno>

#include "work.h"
#include "function.h"

//extern "C" void _init(void){
//	printf("_init\r\n");
//}



void my_init(void) __attribute__((constructor)); 

void my_fini(void) __attribute__((destructor));  


int main(){

	int result = 0;
	
	g_function_c = g_function_c*2;
	result = function_c(8,9);
	printf("function_c result:%x\r\n",result);
	
	
	
	g_workfunc_var = g_workfunc_var*2;
	
	result = workfunc(10,11);
	printf("workfunction result:%x\r\n",result);
	
	void * h = dlopen("/home/ljg/work/elftest/test.so",RTLD_NOW);
	if(h ){
		
		typedef int (*ptrfunction)(int p);
		ptrfunction lpfunc = (ptrfunction)dlsym(h,"__testfunction");
		if(lpfunc){
			int result = lpfunc(4);
			printf("lpfunc(4):%d\r\n",result);
		}else{
			printf("dlsym __testfunction error:%s\r\n",dlerror());
		}
		dlclose(h);
	}else{
		printf("dlopen test.so error:%s\r\n",dlerror());
	}
	return 0;
}






void out_msg(const char *m)
{  
 
    printf("    Our_Func \n");  
 
}  
 
void my_init(void)  
{  
 
    printf("\n    Init \n");  
 
}  
void my_fini(void)  
{  
 
    printf("    Fini \n");  
 
}
