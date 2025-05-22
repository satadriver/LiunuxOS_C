#pragma once


int base64_encode(char* indata, int inlen, char* outdata, int* outlen);

int base64_decode(const char* indata, int inlen, char* outdata, int* outlen);