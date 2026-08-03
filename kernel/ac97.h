#pragma once

#include "def.h"

// ==================== NABM 寄存器偏移（相对于 NABMBAR）====================
// ICH4+ 规范，VirtualBox ICH9 兼容模式
#define PCM_OUT_BASE    0x00

#define PI_BDBAR        0x00   // Buffer Descriptor Base Address Register (DWORD)
#define PI_CIV          0x04   // Current Index Value (BYTE)
#define PI_LVI          0x05   // Last Valid Index (BYTE)
#define PI_SR           0x06   // Status Register (WORD)
#define PI_PICB         0x08   // Position In Current Buffer (WORD)
#define PI_PIV          0x0A   // Prefetched Index Value (BYTE)
#define PI_CR           0x0B   // Control Register (BYTE) - ICH0-3
// 注意：ICH4+ 控制寄存器扩展到 16位，偏移仍为 0x0B，但写入 16位值

// ==================== 控制寄存器位（标准 Intel ICH 定义）====================
#define CR_RPBM         0x01   // bit0: Run/Pause Bus Master（启动 DMA，最关键）
#define CR_RFC          0x02   // bit1: Reset FIFO Control
#define CR_RR           0x04   // bit2: Reset Registers
#define CR_LVBIE        0x04   // bit2: Last Valid Buffer Interrupt Enable (与 RR 共享位，不同版本)
// 注意：ICH 规范中 bit2 在不同版本有不同定义，这里用 CR_RR

// 状态寄存器位（写1清除）
#define SR_FIFOE        0x10   // bit4: FIFO Error
#define SR_BCIS         0x08   // bit3: Buffer Completion Interrupt Status
#define SR_LVBCI        0x04   // bit2: Last Valid Buffer Completion Interrupt
#define SR_CELV         0x02   // bit1: Current Equals Last Valid
#define SR_DCH          0x01   // bit0: DMA Controller Halted

// ==================== DMA 描述符（Intel ICH 规范，8字节对齐）====================
typedef struct {
    uint32_t addr;       // 缓冲区物理地址（低32位）
    uint16_t len;        // 缓冲区长度 - 1（字节数）
    uint16_t ctrl;       // bit15=中断，bit14=最后一块，bit13=保留...
} ac97_desc_t;

// 描述符控制位
#define DESC_IOC        0x8000   // bit15: Interrupt On Completion
#define DESC_BUP        0x4000   // bit14: Buffer Underrun Policy (最后一块)

// ==================== PCI 配置空间 ====================
#define PCI_CONFIG_ADDR  0xCF8
#define PCI_CONFIG_DATA  0xCFC

// ==================== AC97 Codec 寄存器 ====================
#define AC97_RESET              0x00
#define AC97_MASTER_VOL         0x02
#define AC97_HEADPHONE_VOL      0x04
#define AC97_MONO_VOL           0x06
#define AC97_PCM_OUT_VOL        0x18
#define AC97_EXTENDED_ID        0x28
#define AC97_EXTENDED_STAT      0x2A
#define AC97_PCM_FRONT_DAC_RATE 0x2C
#define AC97_PCM_SURR_DAC_RATE  0x2E
#define AC97_PCM_LFE_DAC_RATE   0x30

// Extended ID 位
#define EID_VRA                 0x0001   // Variable Rate Audio
#define EID_DRA                 0x0002   // Double Rate Audio
#define EID_SPDIF               0x0004   // S/PDIF
#define EID_VRM                 0x0008   // Variable Rate Mic
#define EID_CDAC                0x0040   // Center DAC
#define EID_SDAC                0x0080   // Surround DAC
#define EID_LDAC                0x0100   // LFE DAC
#define EID_AMAP                0x0200   // Audio Mixer/ADC

// Extended Status 位
#define EST_VRA                 0x0001
#define EST_DRA                 0x0002

#pragma pack(1)

typedef struct {
    char riff[4];                   // "RIFF"
    unsigned int size;              // 文件大小 - 8
    char wave[4];                   // "WAVE"
    char fmt[4];                    // "fmt "
    unsigned int fmtsize;           // 16 或 18
    unsigned short format;          // 1 = PCM
    unsigned short channels;        // 1 或 2
    unsigned int samplespersec;     // 采样率
    unsigned int bytestpersec;      // 每秒字节数
    unsigned short blockalign;      // 每样本字节数
    unsigned short bitspersample;   // 8 或 16
    char data[4];                   // "data"
    unsigned int datasize;          // 数据大小
} WAV_FILE_HEADER;

#pragma pack()

#ifdef DLL_EXPORT
extern "C" __declspec(dllexport) int g_ac97_exist;
extern "C" __declspec(dllexport) int ac97_play_wav(const uint8_t* wav_buffer, uint32_t wav_size);
extern "C" __declspec(dllexport) int ac97_play_wav_file(char* filename);
#else
extern "C" __declspec(dllimport) int g_ac97_exist;
extern "C" __declspec(dllimport) int ac97_play_wav(const uint8_t* wav_buffer, uint32_t wav_size);
extern "C" __declspec(dllimport) int ac97_play_wav_file(char* filename);
#endif