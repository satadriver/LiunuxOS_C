
#include "des.h"


bool key[64]; // 64位密钥
bool subKey[16][48]; // 存放16个子密钥

// 初始置换表IP(64位)
int IP[64] = { 58, 50, 42, 34, 26, 18, 10, 2,
			60, 52, 44, 36, 28, 20, 12, 4,
			62, 54, 46, 38, 30, 22, 14, 6,
			64, 56, 48, 40, 32, 24, 16, 8,
			57, 49, 41, 33, 25, 17, 9,  1,
			59, 51, 43, 35, 27, 19, 11, 3,
			61, 53, 45, 37, 29, 21, 13, 5,
			63, 55, 47, 39, 31, 23, 15, 7 };

// 逆置换表IP_1
int IP_1[64] = { 40, 8, 48, 16, 56, 24, 64, 32,
			39, 7, 47, 15, 55, 23, 63, 31,
			38, 6, 46, 14, 54, 22, 62, 30,
			37, 5, 45, 13, 53, 21, 61, 29,
			36, 4, 44, 12, 52, 20, 60, 28,
			35, 3, 43, 11, 51, 19, 59, 27,
			34, 2, 42, 10, 50, 18, 58, 26,
			33, 1, 41,  9, 49, 17, 57, 25 };

int E[48] = { 32,  1,  2,  3,  4,  5,
			4,  5,  6,  7,  8,  9,
			8,  9,  10, 11, 12, 13,
			12, 13, 14, 15, 16, 17,
			16, 17, 18, 19, 20, 21,
			20, 21, 22, 23, 24, 25,
			24, 25, 26, 27, 28, 29,
			28, 29, 30, 31, 32, 1 };

// S盒，每个S盒是 4x16 的置换表，6位 -> 4位，总体上，48位 -> 32位
int S_BOX[8][4][16] = {
	{
		{ 14,4,13,1,2,15,11,8,3,10,6,12,5,9,0,7 },
		{ 0,15,7,4,14,2,13,1,10,6,12,11,9,5,3,8 },
		{ 4,1,14,8,13,6,2,11,15,12,9,7,3,10,5,0 },
		{ 15,12,8,2,4,9,1,7,5,11,3,14,10,0,6,13 }
	},
	{
		{ 15,1,8,14,6,11,3,4,9,7,2,13,12,0,5,10 },
		{ 3,13,4,7,15,2,8,14,12,0,1,10,6,9,11,5 },
		{ 0,14,7,11,10,4,13,1,5,8,12,6,9,3,2,15 },
		{ 13,8,10,1,3,15,4,2,11,6,7,12,0,5,14,9 }
	},
	{
		{ 10,0,9,14,6,3,15,5,1,13,12,7,11,4,2,8 },
		{ 13,7,0,9,3,4,6,10,2,8,5,14,12,11,15,1 },
		{ 13,6,4,9,8,15,3,0,11,1,2,12,5,10,14,7 },
		{ 1,10,13,0,6,9,8,7,4,15,14,3,11,5,2,12 }
	},
	{
		{ 7,13,14,3,0,6,9,10,1,2,8,5,11,12,4,15 },
		{ 13,8,11,5,6,15,0,3,4,7,2,12,1,10,14,9 },
		{ 10,6,9,0,12,11,7,13,15,1,3,14,5,2,8,4 },
		{ 3,15,0,6,10,1,13,8,9,4,5,11,12,7,2,14 }
	},
	{
		{ 2,12,4,1,7,10,11,6,8,5,3,15,13,0,14,9 },
		{ 14,11,2,12,4,7,13,1,5,0,15,10,3,9,8,6 },
		{ 4,2,1,11,10,13,7,8,15,9,12,5,6,3,0,14 },
		{ 11,8,12,7,1,14,2,13,6,15,0,9,10,4,5,3 }
	},
	{
		{ 12,1,10,15,9,2,6,8,0,13,3,4,14,7,5,11 },
		{ 10,15,4,2,7,12,9,5,6,1,13,14,0,11,3,8 },
		{ 9,14,15,5,2,8,12,3,7,0,4,10,1,13,11,6 },
		{ 4,3,2,12,9,5,15,10,11,14,1,7,6,0,8,13 }
	},
	{
		{ 4,11,2,14,15,0,8,13,3,12,9,7,5,10,6,1 },
		{ 13,0,11,7,4,9,1,10,14,3,5,12,2,15,8,6 },
		{ 1,4,11,13,12,3,7,14,10,15,6,8,0,5,9,2 },
		{ 6,11,13,8,1,4,10,7,9,5,0,15,14,2,3,12 }
	},
	{
		{ 13,2,8,4,6,15,11,1,10,9,3,14,5,0,12,7 },
		{ 1,15,13,8,10,3,7,4,12,5,6,11,0,14,9,2 },
		{ 7,11,4,1,9,12,14,2,0,6,10,13,15,3,5,8 },
		{ 2,1,14,7,4,10,8,13,15,12,9,0,3,5,6,11 }
	}
};

// P置换，32位 -> 32位
int P[32] = { 16,  7, 20, 21,
			29, 12, 28, 17,
			1,  15, 23, 26,
			5,  18, 31, 10,
			2,  8,  24, 14,
			32, 27, 3,  9,
			19, 13, 30, 6,
			22, 11, 4,  25 };

/*------------------下面是生成子密钥用到的表-----------------*/

// PC-1置换表，64位 -> 56位，去掉密钥中的8个校验位
int PC_1[56] = { 57, 49, 41, 33, 25, 17, 9,
			 1, 58, 50, 42, 34, 26, 18,
			10,  2, 59, 51, 43, 35, 27,
			19, 11,  3, 60, 52, 44, 36,
			63, 55, 47, 39, 31, 23, 15,
			 7, 62, 54, 46, 38, 30, 22,
			14,  6, 61, 53, 45, 37, 29,
			21, 13,  5, 28, 20, 12,  4 };

// PC-2压缩置换表，56位 -> 48位
int PC_2[48] = { 14, 17, 11, 24,  1,  5,
			3,  28, 15,  6, 21, 10,
			23, 19, 12,  4, 26,  8,
			16,  7, 27, 20, 13,  2,
			41, 52, 31, 37, 47, 55,
			30, 40, 51, 45, 33, 48,
			44, 49, 39, 56, 34, 53,
			46, 42, 50, 36, 29, 32 };

// 每轮左移的位数
int shiftBits[16] = { 1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1 };


/**
 * Feistel 轮函数，用于进行16次迭代
 * @param R      32位数据
 * @param K      48位子密钥
 * @param output 32位输出
 */
void Feistel(bool* R, bool* K, bool* output) {
	// 1) 将32位的串R作E-扩展，成为48位的串
	int i;
	bool expandR[48];
	for (i = 0; i < 48; ++i)
	{
		expandR[i] = R[E[i] - 1];
	}

	// 2) expandR 与 K 异或
	for (i = 0; i < 48; ++i)
	{
		expandR[i] = expandR[i] == K[i] ? 0 : 1;
	}

	// 3) 用S-盒分别对8个组进行 6-4 转换
	bool temp[32];
	for (i = 0; i < 8; ++i)
	{
		int j = i * 6;
		int row = expandR[j] * 2 + expandR[j + 5];
		int col = expandR[j + 1] * 8 + expandR[j + 2] * 4 + expandR[j + 3] * 2 + expandR[j + 4];
		int num = S_BOX[i][row][col];
		j = i * 4;
		int k;
		for (k = 3; k >= 0; --k)
		{
			temp[j + k] = num % 2;
			num /= 2;
		}
	}

	// 4) P-置换
	for (i = 0; i < 32; ++i)
	{
		output[i] = temp[P[i] - 1];
	}

	return;
}

/**
 * 对56位密钥的前后部分进行左移
 * @param A     56位密钥
 * @param shift 偏移量
 */
void LeftShift(bool* A, int shift) {
	int temp0 = A[0], temp1 = A[1];
	int i;
	for (i = 0; i < 26; ++i) {
		A[i] = A[i + shift];
	}
	if (shift == 1) {
		A[26] = A[27];
		A[27] = temp0;
	}
	else if (shift == 2) {
		A[26] = temp0;
		A[27] = temp1;
	}
}

/**
 * 生成16个48位的子密钥
 */
void GenerateSubKeys() {
	bool realKey[56];
	bool left[28];
	bool right[28];
	int i;

	// 对密钥的56个非校验位实行PC-1置换
	for (i = 0; i < 56; ++i)
	{
		realKey[i] = key[PC_1[i] - 1];
	}

	// 生成子密钥并保存
	for (i = 0; i < 16; ++i)
	{
		int j;
		// 提取realKey的前28位和后28位
		for (j = 0; j < 28; ++j)
		{
			left[j] = realKey[j];
		}
		for (j = 0; j < 28; ++j)
		{
			right[j] = realKey[j + 28];
		}
		// 左移
		LeftShift(left, shiftBits[i]);
		LeftShift(right, shiftBits[i]);
		// 恢复
		for (j = 0; j < 28; ++j)
		{
			realKey[j] = left[j];
		}
		for (j = 0; j < 28; ++j)
		{
			realKey[j] = right[j + 28];
		}
		// PC-2压缩置换
		for (j = 0; j < 48; ++j)
		{
			subKey[i][j] = realKey[PC_2[j] - 1];
		}
	}
}

/**
 * DES加密
 * @param plain  明文
 * @param cipher 加密得到的密文
 */
void encrypt(bool* plain, bool* cipher) {
	bool temp[64];
	bool left[32];
	bool right[32];
	bool newLeft[32];
	int i, round;


	// 1) 初始置换IP
	for (i = 0; i < 64; ++i)
	{
		temp[i] = plain[IP[i] - 1];
	}
	// 2) 16轮迭代
	for (i = 0; i < 32; ++i)
	{
		left[i] = temp[i];
	}
	for (i = 0; i < 32; ++i)
	{
		right[i] = temp[i + 32];
	}
	for (round = 0; round < 16; ++round)
	{
		for (i = 0; i < 32; ++i)
		{
			newLeft[i] = right[i];
		}
		bool fresult[32];
		Feistel(right, subKey[round], fresult);
		for (i = 0; i < 32; ++i)
		{
			right[i] = left[i] == fresult[i] ? 0 : 1;
		}
		for (i = 0; i < 32; ++i)
		{
			left[i] = newLeft[i];
		}
	}
	// 3) 交换置换
	for (i = 0; i < 32; ++i)
	{
		temp[i] = right[i];
	}
	for (i = 0; i < 32; ++i)
	{
		temp[i + 32] = left[i];
	}
	// 4) 逆置换
	for (i = 0; i < 64; ++i)
	{
		cipher[i] = temp[IP_1[i] - 1];
	}
	return;
}

/**
 * DES解密
 * @param cipher 密文
 * @param plain  解密得到的明文
 */
void decrypt(bool* cipher, bool* plain) {
	bool temp[64];
	bool left[32];
	bool right[32];
	bool newLeft[32];
	int i, round;


	// 1) 初始置换IP
	for (i = 0; i < 64; ++i)
	{
		temp[i] = cipher[IP[i] - 1];
	}
	// 2) 16轮迭代
	for (i = 0; i < 32; ++i)
	{
		left[i] = temp[i];
	}
	for (i = 0; i < 32; ++i)
	{
		right[i] = temp[i + 32];
	}
	for (round = 0; round < 16; ++round)
	{
		for (i = 0; i < 32; ++i)
		{
			newLeft[i] = right[i];
		}
		bool fresult[32];
		Feistel(right, subKey[15 - round], fresult);
		for (i = 0; i < 32; ++i)
		{
			right[i] = left[i] == fresult[i] ? 0 : 1;
		}
		for (i = 0; i < 32; ++i)
		{
			left[i] = newLeft[i];
		}
	}
	// 3) 交换置换
	for (i = 0; i < 32; ++i)
	{
		temp[i] = right[i];
	}
	for (i = 0; i < 32; ++i)
	{
		temp[i + 32] = left[i];
	}
	// 4) 逆置换
	for (i = 0; i < 64; ++i)
	{
		plain[i] = temp[IP_1[i] - 1];
	}
	return;
}

/**
 * 将8个字节转换成64位
 * @param s      8个字节的char数组
 * @param bitset 位数组
 */
void BytesToBits(char* s, bool* bitset) {
	int i, j;
	for (i = 0; i < 8; ++i)
	{
		for (j = 0; j < 8; ++j)
		{
			bitset[8 * i + j] = (s[i] >> j) & 1;
		}
	}
	return;
}

void BitsToBytes(bool* bitset, char* s) {
	int i, j;
	for (i = 0; i < 8; ++i)
	{
		for (j = 0; j < 8; ++j)
		{
			s[i] |= ((int)bitset[8 * i + j]) << j;
		}
	}
	s[8] = '\0';
	return;
}




#if 0
int main() {
	char plainStr[9];
	char keyStr[9];
	char resultStr[9];

	// 输入明文和密钥
	printf("请输入明文和密钥（格式均为8个字符组成的字符串）\n");
	printf("明文：");
	scanf("%s", plainStr);
	printf("密钥：");
	scanf("%s", keyStr);

	bool plain[64];
	bool cipher[64];
	bool result[64];

	// 将明文和密钥转换为位数组
	BytesToBits(plainStr, plain);
	BytesToBits(keyStr, key);

	// 生成16个48位的子密钥
	GenerateSubKeys();

	// DES加密
	encrypt(plain, cipher);
	printf("你的明文已被加密\n");

	printf("请输入密钥来进行解密：");
	scanf("%s", keyStr);
	BytesToBits(keyStr, key);
	GenerateSubKeys();

	// DES解密
	decrypt(cipher, result);

	// 将解密结果转换char数组
	BitsToBytes(result, resultStr);

	printf("解密得到的明文为：%s\n", resultStr);
	printf("如果你输入了正确的密钥，则该结果与原明文一样；否则，该结果将不同与原明文\n");

	return 0;
}
#endif