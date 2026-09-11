
#include <Windows.h>

int __strd2i(char* istr) {
	int len = strlen(istr);
	if (len <= 0)
	{
		return 0;
	}

	int negtive = 0;
	int k = 0;
	if (istr[0] == '-')
	{
		negtive = 1;
		k++;
	}
	else if (istr[0] == '+')
	{
		k++;
	}

	unsigned int ret = 0;
	for (; k < len; k++)
	{
		int v = istr[k] - 0x30;
		if (v >= 0 && v <= 9)
		{
			ret = ret * 10 + v;
		}
		else {
			break;
		}
	}

	if (negtive)
	{
		ret = -ret;
	}
	return ret;
}

double strlf2lf(char* str) {
	int len = strlen(str);
	if (len > 32) {
		return 0.0;
	}
	int npos = 0;
	char strz[64];
	char strf[64];
	int zl = 0;
	int fl = 0;
	int neg = 0;
	for (int i = 0; i < len; i++) {
		if (str[i] == ' ') {

		}
		else if (str[i] >= '0' && str[i] <= '9') {
			if (npos) {
				strf[fl++] = str[i];
			}
			else {
				strz[zl++] = str[i];
			}
		}
		else if (str[i] == '-') {
			if (zl || fl) {
				return 0.0;
			}
			neg = 1;
		}
		else if (str[i] == '.') {
			npos = i;
		}
		else {
			return 0.0;
		}
	}

	strz[zl] = 0;
	strf[fl] = 0;

	int vz = __strd2i(strz);
	double fz = __strd2i(strf);
	
	for (int i = 0; i < fl; i++)
	{
		fz /= 10;
	}
	double v = vz + fz ;
	if (neg)
		v = -v;
	return v;
}
