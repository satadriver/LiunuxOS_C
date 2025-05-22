#pragma once

#pragma once

#define uint32_t unsigned int

#define uint8_t unsigned char

int aesTest();

#pragma pack(1)

typedef struct {
    uint32_t eK[44], dK[44];    // encKey, decKey
    int Nr; // 10 rounds
}AesKey;

int aesEncrypt(const uint8_t* key, uint32_t keyLen, const uint8_t* pt, uint8_t* ct, uint32_t len);

int aesDecrypt(const uint8_t* key, uint32_t keyLen, const uint8_t* ct, uint8_t* pt, uint32_t len);

#pragma pack()
