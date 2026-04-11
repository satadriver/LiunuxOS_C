#include <Windows.h>
#include <stdio.h>
#include "main.h"

int __dump(char* src, int len, int lowercase, unsigned char* dstbuf) {
	if (len >= 0x1000)
	{
		*dstbuf = 0;
		return FALSE;
	}

	int no = 55;
	if (lowercase)
	{
		no = 87;
	}


	int lineno = 0;
	unsigned char* dst = dstbuf;

	char szlineno[16];
	memset(szlineno, 0x20, 16);
	int lnl = sprintf(szlineno, (char*)"%d.", lineno);
	//*(szlineno + lnl) = 0x20;
	//__strcpy((char*)dst, szlineno);
	//dst += lnl;
	for (int k = 0; k < 16; k++)
	{
		if (szlineno[k] == 0)
		{
			szlineno[k] = 0x20;
		}
	}
	memcpy((char*)dst, szlineno, 8);
	dst += 8;


	for (int i = 0; i < len; i++)
	{
		if ((i != 0) && (i % 16 == 0))
		{
			*dst = '\n';
			dst++;
			lineno++;

			memset(szlineno, 0x20, 16);
			lnl = sprintf(szlineno, (char*)"%d.", lineno);
			//*(szlineno + lnl) = 0x20;
			//__strcpy((char*)dst, szlineno);
			//dst += lnl;
			for (int k = 0; k < 16; k++)
			{
				if (szlineno[k] == 0)
				{
					szlineno[k] = 0x20;
				}
			}
			memcpy((char*)dst, szlineno, 8);
			dst += 8;
		}
		else if ((i != 0) && (i % 8 == 0))
		{
			*dst = ' ';
			dst++;
			*dst = ' ';
			dst++;
		}

		unsigned char c = src[i];
		unsigned char h = (c & 0x0f0) >> 4;
		unsigned char l = c & 0x0f;
		if (h >= 0 && h <= 9)
		{
			h += 48;
		}
		else if (h >= 10 && h <= 15)
		{
			h += no;
		}

		*dst = h;
		dst++;

		if (l >= 0 && l <= 9)
		{
			l += 48;
		}
		else if (l >= 10 && l <= 15)
		{
			l += no;
		}

		*dst = l;
		dst++;

		*dst = ' ';
		dst++;
	}

	*dst = '\n';
	dst++;

	*dst = 0;
	dst++;

	return dst - dstbuf;
}




int lf2strlf(double f, char* buf) {
	unsigned long long i = (unsigned long long)f;

	int len = __i64ToStrd64(i, buf);
	buf[len] = '.';
	len++;

	double s = f - i;
	if (s < 0) {
		s = -s;
	}

	double tf = s;
	int pos = 0;

	for (int p = 0; p < 4; p++) {
		tf = tf * 10;
		unsigned long long ti = (unsigned long long)tf;
		if (ti) {
			tf = tf - ti;
			pos = p;
		}
	}

	for (int k = 0; k < pos + 1; k++) {
		s = s * 10;
		unsigned long long t = (unsigned long long)s;
		s = s - t;
		int sublen = __i64ToStrd64(t, buf + len);
		len += sublen;
	}

	buf[len] = 0;
	return len;
}


char* swapStr(char* dst, char* src) {
	int len = strlen(src);

	for (int i = len - 1, j = 0; i >= 0; i--, j++) {
		dst[j] = src[i];
	}
	dst[len] = 0;
	return dst;
}

int f2strf(float d, char* buf) {
	return lf2strlf(d, buf);
}

int __i64ToStrd64(__int64 v, char* buf) {
	char strd[256];
	*strd = 0;

	int start = 0;
	if (v < 0) {
		v = -v;
		buf[0] = '-';
		start = 1;
	}

	int len = 0;
	__int64 h = v;
	do {
		__int64 i = h % 10;

		h = h / 10;

		strd[len] = (unsigned char)i + '30';
		len++;

	} while (h);

	strd[len] = 0;

	swapStr(buf + start, strd);

	return len + start;
}

int __i2strh(unsigned int n, int lowercase, unsigned char* buf) {
	buf[0] = 0x30;
	buf[1] = 'X';

	int no = 55;
	if (lowercase)
	{
		no = 87;
		buf[1] = 'x';
	}

	int b = 24;

	int empty = 0;

	unsigned char* dst = buf + 2;

	for (int i = 0; i < 4; i++)
	{
		unsigned char c = n >> b;

		unsigned char h = (c & 0x0f0) >> 4;
		unsigned char l = c & 0x0f;

		unsigned char tmp = h;

		if (h >= 0 && h <= 9)
		{
			h += 48;
		}
		else if (h >= 10 && h <= 15)
		{
			h += no;
		}

		if (empty) {
			*dst = h;
			dst++;
		}
		else {
			if (tmp) {
				empty = TRUE;
				*dst = h;
				dst++;
			}
			else {

			}
		}

		tmp = l;
		if (l >= 0 && l <= 9)
		{
			l += 48;
		}
		else if (l >= 10 && l <= 15)
		{
			l += no;
		}

		if (empty) {
			*dst = l;
			dst++;
		}
		else {
			if (tmp) {
				empty = TRUE;
				*dst = l;
				dst++;
			}
			else {

			}
		}

		b -= 8;
	}

	if (dst - buf == 2) {
		buf[2] = 0x30;
		buf[3] = 0;
		return 3;
	}
	else {
		*(dst) = 0;
		return dst - buf;
	}
}

int __i2stru(unsigned int h, char* strd) {

	memset(strd, 0, 11);

	unsigned int divid = 1000000000;

	int cnt = 0;

	for (int i = 0; i < 10; i++)
	{
		unsigned int d = h / divid;
		if (d)
		{
			*strd = d + 0x30;

			strd++;

			cnt++;

			h = h % divid;
		}
		else if (cnt) {
			*strd = 0x30;
			strd++;
			cnt++;
		}

		divid = divid / 10;
	}

	if (cnt == 0)
	{
		*strd = 0x30;
		return 1;
	}
	return cnt;
}


int __i2strd(int h, char* strd) {
	int n = h;
	int len = 0;
	if (h < 0) {
		strd[0] = '-';
		n = -h;
		len = 1;
	}
	else {
	}
	int sublen = __i2stru(n, strd + len);
	return len + sublen;
}


int __strh2i(unsigned char* str) {
	int ret = 0;

	int len = strlen((char*)str);
	for (int i = 0; i < len; i++)
	{
		ret = ret << 4;

		unsigned char c = str[i];
		if (c >= '0' && c <= '9')
		{
			c = c - 48;
		}
		else if (c >= 'A' && c <= 'F')
		{
			c = c - 55;
		}
		else if (c >= 'a' && c <= 'f')
		{
			c = c - 87;
		}
		else {
			return 0;
		}

		ret += c;
	}

	return ret;
}


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

	int ret = 0;
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

int main(int argc, char* argv[])
{
	char buf[1024];

	double db = 34567890156.1234890;

	long long ul = 0x123456789012345L;

	lf2strlf(db, buf);

	__i64ToStrd64(ul, buf);
	return FALSE;
}


