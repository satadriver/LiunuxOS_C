#pragma once

#include "def.h"

// 基址偏移（相对于 NABMBAR）
#define PCM_OUT_BASE          0x00      // PCM输出通道基址
#define BMI_PI_BASE           0x10      // 输入基址
#define PI_INDEX              0x00      // 描述符指针
#define PI_CIV                0x02      // 当前索引值
#define PI_LVI                0x03      // 最后有效索引
#define PI_SR                 0x04      // 状态寄存器
#define PI_PICB               0x05      // 字节计数
#define PI_PIV                0x07      // 预加载索引
#define PI_CR                 0x0f      // 控制寄存器
// 控制寄存器位
#define CR_IOCE               0x0001    // 中断完成使能
#define CR_FEIE               0x0002    // FIFO错误中断使能
#define CR_LVBIE              0x0004    // 最后有效缓冲中断使能
#define CR_RR                 0x0008    // 重跑
#define CR_RPBM               0x0010    // 复位总线主控
#define CR_RFC                0x0020    // 复位FIFO
#define CR_LD                 0x0080    // 加载描述符表
#define CR_RD                 0x0100    // 运行位

/***************************** 总线主控 DMA 描述符 (Intel ICH 规范) *****************************/
typedef struct {
    uint32_t addr;       // 缓冲区物理地址（低32位）
    uint32_t ctrl_len;   // bit31=最后一块, bit30=中断使能, 低17位=字节数-1
}  ac97_desc_t;



// PCM 输出通道寄存器偏移 (相对于 NABMBAR)
#define PCM_OUT_BASE   0x00
#define PI_INDEX       0x00   // 描述符表基址 (DWORD)
#define PI_CIV         0x02   // 当前索引 (BYTE)
#define PI_LVI         0x03   // 最后有效索引 (BYTE)
#define PI_SR          0x04   // 状态寄存器 (WORD)
#define PI_PICB        0x05   // 当前字节计数 (WORD)
#define PI_CR          0x0F   // 控制寄存器 (WORD)

// 控制寄存器位
#define CR_RPBM   (1 << 4)    // 复位总线主控
#define CR_RFC    (1 << 5)    // 复位 FIFO
#define CR_LD     (1 << 7)    // 加载描述符
#define CR_RD     (1 << 8)    // 运行 DMA

/***************************** PCI 配置空间访问 *****************************/
#define PCI_CONFIG_ADDR  0xCF8
#define PCI_CONFIG_DATA  0xCFC

/***************************** AC97 Codec 操作 (通过 Mixer 基址) *****************************/
#define AC97_RESET           0x00
#define AC97_MASTER_VOL      0x02
#define AC97_PCM_OUT_VOL     0x18
#define AC97_EXTENDED_ID     0x28
#define AC97_EXTENDED_STAT   0x2A
#define AC97_PCM_FRONT_DAC_RATE 0x2C

#pragma pack(1)

typedef struct {
    char riff[4];          //0 "RIFF"
    unsigned int size;     //4 文件大小
    char wave[4];          //8 "WAVE"
    char fmt[4];           //12 "fmt "
    unsigned int fmtsize;  //16 fmt字段大小
    unsigned short format; //20 音频格式
    unsigned short channels; //22 通道数
    unsigned int samplespersec; //24 采样率
    unsigned int bytestpersec; //28 每秒字节数
    unsigned short blockalign; //32 每个通道的字节数
    unsigned short bitspersample; //34 采样大小
    char data[4];          // "data"
    unsigned int datasize; // 数据大小
} WAV_FILE_HEADER;

#pragma pack()


#ifdef DLL_EXPORT

extern "C" __declspec(dllexport) int g_ac97_exist;

extern "C" __declspec(dllexport) int ac97_play_wav(const uint8_t* wav_buffer, uint32_t wav_size);
extern "C" __declspec(dllexport)int ac97_play_wav_file(char* filename);
#else

extern "C" __declspec(dllimport) int g_ac97_exist ;

extern "C" __declspec(dllimport) int ac97_play_wav(const uint8_t* wav_buffer, uint32_t wav_size);
extern "C" __declspec(dllimport)int ac97_play_wav_file(char* filename);
#endif