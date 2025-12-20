

//#include<stdio.h>
//#include<string.h>
//#include "rc4.h"
#include "def.h"
#include "Utils.h"

#define KEY_SIZE 1024

/*初始化函数*/
void rc4_init(unsigned char* s, unsigned char* key, unsigned long Len)
{
    int i = 0, j = 0;
    char k[KEY_SIZE] = { 0 };
    unsigned char tmp = 0;
    for (i = 0; i < KEY_SIZE; i++)
    {
        s[i] = i;
        k[i] = key[i % Len];
    }

    for (i = 0; i < KEY_SIZE; i++)
    {
        j = (j + s[i] + k[i]) % KEY_SIZE;

        tmp = s[i];
        s[i] = s[j];//交换s[i]和s[j]
        s[j] = tmp;
    }
}

/*加解密*/
void rc4_crypt(unsigned char* s, unsigned char* Data, unsigned long Len)
{
    int i = 0, j = 0, t = 0;
    unsigned long k = 0;
    unsigned char tmp;

    for (k = 0; k < Len; k++)
    {

        j = (j + s[i]) % KEY_SIZE;

        tmp = s[i];
        s[i] = s[j];//交换s[x]和s[y]
        s[j] = tmp;

        t = (s[i] + s[j]) % KEY_SIZE;
        Data[k] ^= s[t];

        i = (i + 1) % KEY_SIZE;
    }
}

int rc4_test()
{
    char szout[256];

    unsigned char s[KEY_SIZE] = { 0 }, s2[KEY_SIZE] = { 0 };//S-box

    char key[KEY_SIZE] = { "this is the key to init RC4 key" };

    char pData[512] = "[Data]这是一段用来测试RC4加解密算法的数据，此段数据会被用来加密和解密、并被输出显示、以便验证RC4加解密算法的正确性[Data]";

    unsigned long datalen = __strlen(pData);

    __printf(szout,"加解密测试数据:\"%s\"\n\n", pData);

    __printf(szout,"RC4密钥:\"%s\",长度:%d\n\n", key, __strlen(key));

    rc4_init(s, (unsigned char*)key, __strlen(key));

    __printf(szout,"S-box初始化数据:\r\n");
    for (int i = 0; i < KEY_SIZE; i++)
    {
        __printf(szout,"%02X", s[i]);
        if (i && (i + 1) % 16 == 0)
            __printf(szout,"\r\n");
    }
    __printf(szout,"\n\n");

    for (int i = 0; i < KEY_SIZE; i++)//用s2[i]暂时保留经过初始化的s[i]
    {
        s2[i] = s[i];
    }


    rc4_crypt(s, (unsigned char*)pData, datalen);//加密
    __printf(szout,"RC4加密后的数据:%s\r\n\r\n", pData);


    rc4_init(s2, (unsigned char*)key, __strlen(key));//初始化密钥
    rc4_crypt(s2, (unsigned char*)pData, datalen);//解密
    __printf(szout,"RC4解密后的数据:%s\r\n\r\n", pData);

    return 0;
}


