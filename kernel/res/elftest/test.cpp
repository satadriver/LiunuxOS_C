#include <stdio.h>


int g_int = 2;

//extern "C" __declspec(dllexport) int __testfunction(int p);


extern "C" int __testfunction(int p)
{
	
	int k = g_int*p;
	for(int i = 1;i <= 5;i ++)
	{
		k = k * i;
	}
	
	//printf("result:%d\r\n",k);
	return k;
}


int main(){

	return __testfunction(5);
}	
