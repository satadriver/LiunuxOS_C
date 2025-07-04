#pragma once
#ifndef _DES_H_
#define _DES_H_

//#include <stdbool.h>









void Feistel(bool* R, bool* K, bool* output);

void LeftShift(bool* A, int shift);

void GenerateSubKeys();

void encrypt(bool* plain, bool* cipher);

void decrypt(bool* cipher, bool* plain);

void BytesToBits(char* s, bool* bitset);

void BitsToBytes(bool* bitset, char* s);

#endif