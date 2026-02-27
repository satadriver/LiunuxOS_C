#ifndef _MATH_C_
#define _MATH_C_



#include "math.h"
#include "def.h"

extern "C"  __declspec(dllexport) double __abs(double x)
{
	if (x < 0)
	{
		return -x;
	}
	return x;
}



//only when n > 0
extern "C"  __declspec(dllexport) double __pown(double x, int n)
{
	if (n <= 0) {
		return 0;
	}
	double r = 1.0;
	for (int i = 0; i < n; ++i)
	{
		r *= x;
	}
	return r;
}


extern "C"  __declspec(dllexport) double __pow(double a, int b)
{
	double r = a;
	if (b > 0)
	{
		while (--b)
			r *= a;
	}
	else if (b < 0)
	{
		while (++b)
			r *= a;
		r = 1.0 / r;
	}
	else
		r = 1;
	return r;
}



extern "C"  __declspec(dllexport) double __sqrt(double x)
{
	if (x < 0)
	{
		return -1.0;
	}
	if (x == 0)
	{
		return 0.0;
	}
	double x0, x1;
	x0 = x;
	x1 = x / 2.0;
	while (__abs(x0 - x1) > DOUBLE_PRECISION_MIN)
	{
		x0 = x1;
		x1 = (x0 + (x / x0)) / 2;
	}
	return x1;
}

extern "C"  __declspec(dllexport)double _sqrt(double x)
{
	double xhalf = 0.5f * x;
	int i = *(int*)&x;
	i = 0x5f375a86 - (i >> 1);
	x = *(float*)&i;
	x = x * (1.5f - xhalf * x * x);
	x = x * (1.5f - xhalf * x * x);
	x = x * (1.5f - xhalf * x * x);
	return 1 / x;
}



extern "C"  __declspec(dllexport) double __sin(double x)
{
	while (x > PI)
	{
		x -= PI * 2.0;
	}

	while (x < -PI)
	{
		x += PI * 2.0;
	}

	const double B = 1.2732395447;
	const double C = -0.4052847346;
	const double P = 0.2310792853;		//0.225; 
	double y = B * x + C * x * __abs(x);
	y = P * (y * __abs(y) - y) + y;
	return y;
}






extern "C"  __declspec(dllexport) double __cos(double x)
{
	double Q = PI/2.0;

	x += Q;
	if (x > PI)
		x -= 2.0 * PI;

	return(__sin(x));
}



//1 + 3 + 5 + ... + (2n - 1) = (1 + (2n - 1))*(n / 2) = n ^ 2
extern "C"  __declspec(dllexport) DWORD __sqrtInteger(DWORD i) {
	DWORD root = 0;
	__asm {
		MOV eax, i
		MOV EBX, 1
		MOV ECX, 1
		_S_LOOP:
		SUB EAX, EBX
		JC _END; 有借位为止
		INC EBX; 修改为3、5、7...
		INC EBX
		INC ECX; n加1
		JMP _S_LOOP
		_END :
		MOV root, ECX
	}
	return root;
}



extern "C"  __declspec(dllexport) double __acos(double x)
{
	if (x > 1 || x < -1) {
		return 0.0;
	}

	double L, R, Mid;

	L = 0; 
	
	R = PI;

	Mid = (L + R) / 2.0;

	while (1)
	{
		if (__abs(__cos(Mid) - x) < DOUBLE_PRECISION_MIN) //近似值相等
		{
			break;
		}

		else if (x< __cos(Mid)) {

			L = Mid;
		}
		else if (x > __cos(Mid)) { //x在左侧

			R = Mid;
		}
		Mid = (L + R) / 2.0;

	}

	return Mid;

}


extern "C"  __declspec(dllexport) double __asin(double x)
{
	if (x > 1 || x < -1) {
		return 0.0;
	}

	double L, R, Mid;

	L = 0;

	R = PI;

	Mid = (L + R) / 2.0;

	while (1)
	{
		if (__abs(__sin(Mid) - x) < DOUBLE_PRECISION_MIN) //近似值相等
		{
			break;
		}

		else if (x < __sin(Mid)) {

			L = Mid;
		}
		else if (x > __sin(Mid)) { //x在左侧

			R = Mid;
		}
		Mid = (L + R) / 2.0;

	}

	return Mid;

}


extern "C"  __declspec(dllexport) double __asin_old(double x)		//taylor expandation error
{
	if (x > 1 || x < -1) {
		return 0.0;
	}
	int n = 1; 	

	double u, ans;
	ans = x;	
	u = x;
	while (__abs(u) >= DOUBLE_PRECISION_MIN) {
		if (__abs(u) < DOUBLE_PRECISION_MIN)
			break;
		u = u * (2.0 * n - 1) * (2.0 * n - 1) * x * x / ((2.0 * n) * (2.0 * n + 1));
		n++;
		ans = ans + u;
	}

	return ans;
}

//https://blog.csdn.net/simitwsnyx/article/details/45890281



extern "C"  __declspec(dllexport) double _sin(double x)
{
	int sign = 1;
	int itemCnt = 4;
	int k = 0;
	double result = 0.0f;
	double tx = 0.0f;
	int factorial = 1;
	if (x < 0)
	{
		x = -x;
		sign *= -1;
	}
	while (x >= SL_2PI)
	{
		x -= SL_2PI;
	}
	if (x > SL_PI)
	{
		x -= SL_PI;
		sign *= -1;
	}
	if (x > SL_PI_DIV_2)
	{
		x = SL_PI - x;
	}
	tx = x;
	for (k = 0; k < itemCnt; k++)
	{
		if (k % 2 == 0)
		{
			result += (tx / factorial);
		}
		else
		{
			result -= (tx / factorial);
		}
		tx *= (x * x);
		factorial *= (2 * (k + 1));
		factorial *= (2 * (k + 1) + 1);
	}
	if (1 == sign)
		return result;
	else
		return -result;
}

extern "C"  __declspec(dllexport) double __atan(double y, double x, int infNum)
{
	int i;
	double z = y / x, sum = 0.0f, temp;
	double del = z / infNum;

	for (i = 0; i < infNum; i++)
	{
		z = i * del;
		temp = 1 / (z * z + 1) * del;
		sum += temp;
	}

	if (x > 0)
	{
		return sum;
	}
	else if (y >= 0 && x < 0)
	{
		return sum + PI;
	}
	else if (y < 0 && x < 0)
	{
		return sum - PI;
	}
	else if (y > 0 && x == 0)
	{
		return PI / 2;
	}
	else if (y < 0 && x == 0)
	{
		return -1 * PI / 2;
	}
	else
	{
		return 0;
	}
}



int g_sincos[256] =
{ 0, 6, 13, 19, 25, 31, 38, 44, 50, 56,
 62, 68, 74, 80, 86, 92, 98, 104, 109, 115,
 121, 126, 132, 137, 142, 147, 152, 157, 162, 167,
 172, 177, 181, 185, 190, 194, 198, 202, 206, 209,
 213, 216, 220, 223, 226, 229, 231, 234, 237, 239,
 241, 243, 245, 247, 248, 250, 251, 252, 253, 254,
 255, 255, 256, 256, 256, 256, 256, 255, 255, 254,
 253, 252, 251, 250, 248, 247, 245, 243, 241, 239,
 237, 234, 231, 229, 226, 223, 220, 216, 213, 209,
 206, 202, 198, 194, 190, 185, 181, 177, 172, 167,
 162, 157, 152, 147, 142, 137, 132, 126, 121, 115,
 109, 104, 98, 92, 86, 80, 74, 68, 62, 56,
 50, 44, 38, 31, 25, 19, 13, 6, 0, -6,
 - 13, -19, -25, -31, -38, -44, -50, -56, -62, -68,
 - 74, -80, -86, -92, -98, -104, -109, -115, -121, -126,
 - 132, -137, -142, -147, -152, -157, -162, -167, -172, -177,
 - 181, -185, -190, -194, -198, -202, -206, -209, -213, -216,
 - 220, -223, -226, -229, -231, -234, -237, -239, -241, -243,
 - 245, -247, -248, -250, -251, -252, -253, -254, -255, -255,
 - 256, -256, -256, -256, -256, -255, -255, -254, -253, -252,
 - 251, -250, -248, -247, -245, -243, -241, -239, -237, -234,
 - 231, -229, -226, -223, -220, -216, -213, -209, -206, -202,
 - 198, -194, -190, -185, -181, -177, -172, -167, -162, -157,
 - 152, -147, -142, -137, -132, -126, -121, -115, -109, -104,
 - 98, -92, -86, -80, -74, -68, -62, -56, -50, -44,
 - 38, -31, -25, -19, -13, -6 };




int GetCos(int angle) {
	int idx = (angle + 64)&0xff;
	return g_sincos[idx];
}

int GetSin(int angle) {
	return g_sincos[angle];
}



/**
 * 最简单的log实现 - 使用变换和级数
 * 原理：ln(x) = ln(m * 2^k) = ln(m) + k * ln(2)
 * 其中 m 在 [1, 2) 区间
 */
double __log_test(double x) {
	// 检查输入
	if (x <= 0) {
		return -999999;  // 错误标记
	}

	// 特殊情况
	if (x == 1.0) {
		return 0.0;
	}

	// 步骤1：将x归一化到 [1, 2) 区间
	int k = 0;
	double m = x;

	// 如果x >= 2，不断除以2
	while (m >= 2.0) {
		m = m / 2.0;
		k++;
	}

	// 如果x < 1，不断乘以2
	while (m < 1.0) {
		m = m * 2.0;
		k--;
	}

	// 现在 m 在 [1, 2) 区间
	// 步骤2：计算 ln(m) 使用变换 y = (m-1)/(m+1)
	// ln(m) = 2 * [y + y³/3 + y⁵/5 + ...]

	double y = (m - 1.0) / (m + 1.0);
	double y2 = y * y;
	double term = y;
	double ln_m = 0.0;

	// 计算级数
	for (int n = 1; n <= 20; n += 2) {
		ln_m = ln_m + term / n;
		term = term * y2;

		// 如果项太小就停止
		if (term < 1e-15 || term > -1e-15) {
			break;
		}
	}

	ln_m = 2.0 * ln_m;

	// 步骤3：组合结果 ln(x) = ln(m) + k * ln(2)
	return ln_m + k * LN2;
}


double __log(double x) {
	if (x <= 0) return 0;
	if (x == 1) return 0;

	// 归一化
	int k = 0;
	double m = x;
	while (m >= 2) { m /= 2; k++; }
	while (m < 1) { m *= 2; k--; }

	// 计算 ln(m)
	double y = (m - 1) / (m + 1);
	double y2 = y * y;
	double sum = y;
	double term = y;

	// 计算级数直到项足够小
	for (int n = 3; n <= 19; n += 2) {
		term = term * y2;
		sum = sum + term / n;

		if (__abs(term / n) < 1e-15) {
			break;
		}
	}

	return 2 * sum + k * LN2;
}









double __exp(double x) {
	// 处理负数: e^(-x) = 1/e^x
	if (x < 0) {
		return 1.0 / __exp(-x);
	}

	double result = 1.0;  // 第一项: 1
	double term = 1.0;    // 当前项的值

	// 计算级数的每一项
	for (int n = 1; n <= 256; n++) {
		term = term * x / n;  // 第n项 = 第(n-1)项 * x / n
		result = result + term;

		// 如果当前项非常小，可以提前结束
		if (term < 1e-15) {
			break;
		}
	}

	return result;
}

#endif