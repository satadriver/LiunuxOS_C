#include "mppx.h"

void mppx(char * str,int len) {
	int l = len - 1;
	int times = len - 1;

	for (int k = 0; k < times; k++)
	{

		for (int i = 0; i < l; i++)
		{
			if (str[i] > str[i+1])
			{
				char t = str[i+1];
				str[i + 1] = str[i];
				str[i] = t;
			}
		}

		l--;
	}
}