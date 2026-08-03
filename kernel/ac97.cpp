#include "def.h"
#include "libc.h"
#include "ac97.h"
#include "device.h"
#include "hardware.h"
#include "math.h"
#include "malloc.h"
#include "file.h"

extern "C" __declspec(dllexport) int g_ac97_exist = 0;

// 描述符表 (最多32项, 16字节对齐)
__declspec(align(16)) ac97_desc_t desc_table[32];

static volatile uint16_t* pcm_out_base;

// ==================== PCI 配置空间访问 ====================
static uint32_t pci_read_config_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t addr = (1UL << 31) | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC);
    outportd(PCI_CONFIG_ADDR, addr);
    return inportd(PCI_CONFIG_DATA);
}

static void pci_write_config_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t val) {
    uint32_t addr = (1UL << 31) | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC);
    outportd(PCI_CONFIG_ADDR, addr);
    outportd(PCI_CONFIG_DATA, val);
}

static uint16_t pci_read_config_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t dword = pci_read_config_dword(bus, slot, func, offset);
    return (uint16_t)((dword >> ((offset & 2) * 8)) & 0xFFFF);
}

static void pci_write_config_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t val) {
    uint32_t dword = pci_read_config_dword(bus, slot, func, offset);
    uint32_t mask = 0xFFFFUL << ((offset & 2) * 8);
    uint32_t new_dword = (dword & ~mask) | (((uint32_t)val) << ((offset & 2) * 8));
    pci_write_config_dword(bus, slot, func, offset, new_dword);
}

// ==================== AC97 Codec 操作 ====================
static void ac97_codec_write(uint16_t mixer_base, uint8_t reg, uint16_t val) {
    outportb(mixer_base + 0x00, reg);
    __sleep(0);  // 添加 I/O 延迟
    outportw(mixer_base + 0x02, val);
    __sleep(0);
    // 等待 Codec 就绪（可选，某些实现需要）
    for (int i = 0; i < 100; i++) {
        if ((inportb(mixer_base + 0x00) & 0x80) == 0) break;
    }
}

static uint16_t ac97_codec_read(uint16_t mixer_base, uint8_t reg) {
    outportb(mixer_base + 0x00, reg);
    __sleep(0);
    return inportw(mixer_base + 0x02);
}

// ==================== 查找 AC97 设备 ====================
static int find_ac97_device(uint8_t* bus, uint8_t* dev, uint8_t* func,
    uint16_t* nabmbar, uint16_t* namba) {
    
    for (uint8_t b = 0; b < 256; b++) {
        for (uint8_t d = 0; d < 32; d++) {
            for (uint8_t f = 0; f < 8; f++) {
                uint32_t class_rev = pci_read_config_dword(b, d, f, 0x08);
                uint8_t _class = (class_rev >> 24) & 0xFF;
                uint8_t subclass = (class_rev >> 16) & 0xFF;
                uint8_t prog_if = (class_rev >> 8) & 0xFF;
                
                // AC97: Class=0x04 (Multimedia), Subclass=0x01 (Audio), ProgIF=0x00
                if (_class == 0x04 && subclass == 0x01 && prog_if == 0x00) {
                    uint32_t bar0 = pci_read_config_dword(b, d, f, 0x10);   // NABMBAR
                    uint32_t bar1 = pci_read_config_dword(b, d, f, 0x14);   // NAMBA
                    
                    // 必须是 I/O 空间
                    if ((bar0 & 1) == 0) continue;
                    
                    *nabmbar = (uint16_t)(bar0 & 0xFFFC);
                    *namba = (uint16_t)(bar1 & 0xFFFC);
                    *bus = b; *dev = d; *func = f;
                    
                    my_printf("Found AC97: bus=%d dev=%d func=%d NABMBAR=%x NAMBA=%x\n",
                        b, d, f, *nabmbar, *namba);
                    return 0;
                }
            }
        }
    }
    return -1;
}

// ==================== 初始化 AC97 Codec ====================
static void init_ac97_codec(uint16_t mixer_base, uint32_t sample_rate) {
    my_printf("Initializing AC97 codec at %x\n", mixer_base);
    
    // 1. 全局复位
    ac97_codec_write(mixer_base, AC97_RESET, 0x0000);
    for (volatile int i = 0; i < 100000; i++);  // 延时等待复位完成
    
    // 2. 设置最大音量（AC97: 0x0000 = 0dB = 最大音量, bit15 = 静音）
    // 0x1F1F = -46.5dB，接近静音！必须改为 0x0000
    ac97_codec_write(mixer_base, AC97_MASTER_VOL, 0x0000);      // 主音量最大
    ac97_codec_write(mixer_base, AC97_HEADPHONE_VOL, 0x0000);  // 耳机音量最大
    ac97_codec_write(mixer_base, AC97_PCM_OUT_VOL, 0x0000);     // PCM 音量最大
    ac97_codec_write(mixer_base, AC97_MONO_VOL, 0x0000);       // 单声道最大
    
    my_printf("Master vol: %x\n", ac97_codec_read(mixer_base, AC97_MASTER_VOL));
    my_printf("PCM vol: %x\n", ac97_codec_read(mixer_base, AC97_PCM_OUT_VOL));
    
    // 3. 检测并设置采样率
    uint16_t ext_id = ac97_codec_read(mixer_base, AC97_EXTENDED_ID);
    my_printf("Extended ID: %x\n", ext_id);
    
    if (ext_id & EID_VRA) {
        my_printf("VRA supported, setting sample rate to %d\n", sample_rate);
        ac97_codec_write(mixer_base, AC97_PCM_FRONT_DAC_RATE, (uint16_t)sample_rate);
        
        // 启用 VRA
        uint16_t ext_stat = ac97_codec_read(mixer_base, AC97_EXTENDED_STAT);
        ext_stat |= EST_VRA;
        ac97_codec_write(mixer_base, AC97_EXTENDED_STAT, ext_stat);
        
        // 验证实际采样率
        uint16_t actual = ac97_codec_read(mixer_base, AC97_PCM_FRONT_DAC_RATE);
        my_printf("Actual sample rate: %d\n", actual);
    } else {
        my_printf("VRA not supported, using fixed 48000Hz\n");
        // 固定 48kHz，确保传入数据也是 48kHz
    }
}

// ==================== NABM 控制器操作 ====================
static void ac97_reset_controller(uint16_t base_port) {
    my_printf("Resetting NABM controller at %x\n", base_port);
    
    // 1. 停止 DMA
    outportb(base_port + PI_CR, 0x00);
    __sleep(0);
    
    // 2. 复位 FIFO
    outportb(base_port + PI_CR, CR_RFC);
    __sleep(0);
    
    // 3. 等待复位完成
    for (int i = 0; i < 1000; i++) {
        if ((inportb(base_port + PI_CR) & CR_RFC) == 0) break;
    }
    
    // 4. 清除状态寄存器（写1清除）
    outportw(base_port + PI_SR, 0xFFFF);
    __sleep(0);
    
    my_printf("Controller reset done, CR=%x SR=%x\n",
        inportb(base_port + PI_CR), inportw(base_port + PI_SR));
}

static void ac97_dma_start(uint16_t base_port, uint32_t phys_desc, int num_desc) {
    my_printf("Starting DMA: desc=%x count=%d\n", phys_desc, num_desc);
    
    // 1. 确保停止
    outportb(base_port + PI_CR, 0x00);
    __sleep(0);
    
    // 2. 设置描述符表物理地址
    outportd(base_port + PI_BDBAR, phys_desc);
    __sleep(0);
    
    // 3. 设置最后有效索引
    outportb(base_port + PI_LVI, (uint8_t)(num_desc - 1));
    __sleep(0);
    
    // 4. 启动 DMA（只需要 CR_RPBM = 0x01）
    outportb(base_port + PI_CR, CR_RPBM);
    __sleep(0);
    
    my_printf("DMA started, CR=%x SR=%x CIV=%d\n",
        inportb(base_port + PI_CR),
        inportw(base_port + PI_SR),
        inportb(base_port + PI_CIV));
}

static void ac97_dma_wait_complete(uint16_t base_port, int num_desc) {
    my_printf("Waiting for DMA completion...\n");
    
    while (1) {
        uint16_t sr = inportw(base_port + PI_SR);
        uint8_t civ = inportb(base_port + PI_CIV);
        uint8_t lvi = inportb(base_port + PI_LVI);
        
        // 检查 DMA 是否停止
        if (sr & SR_DCH) {
            my_printf("DMA halted (SR=%x)\n", sr);
            break;
        }
        
        // 检查是否到达最后一个描述符
        if (civ == lvi && (sr & SR_LVBCI)) {
            my_printf("Last buffer completed (CIV=%d LVI=%d SR=%x)\n", civ, lvi, sr);
            outportw(base_port + PI_SR, SR_LVBCI);  // 清除标志
            break;
        }
        
        // 检查 FIFO 错误
        if (sr & SR_FIFOE) {
            my_printf("FIFO error (SR=%x)\n", sr);
            outportw(base_port + PI_SR, SR_FIFOE);
        }
    }
    
    // 停止 DMA
    outportb(base_port + PI_CR, 0x00);
    __sleep(0);
}

// ==================== WAV 解析 ====================
static int mymemcmp(const void* s1, const void* s2, size_t n) {
    const uint8_t* a = (const uint8_t*)s1;
    const uint8_t* b = (const uint8_t*)s2;
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i]) return (a[i] < b[i]) ? -1 : 1;
    }
    return 0;
}

static int parse_wav(const uint8_t* wav_data, uint32_t wav_size,
    uint8_t** pcm_out, uint32_t* pcm_size,
    uint16_t* channels, uint32_t* sample_rate, uint16_t* bits) {
    
    if (wav_size < 44) return -1;
    
    const WAV_FILE_HEADER* wav = (const WAV_FILE_HEADER*)wav_data;
    
    // 验证 RIFF/WAVE 头
    if (my_memcmp(wav->riff, "RIFF", 4) != 0 ||
        my_memcmp(wav->wave, "WAVE", 4) != 0) {
        my_printf("Invalid WAV header\n");
        return -1;
    }
    
    // 验证 fmt 块
    if (my_memcmp(wav->fmt, "fmt ", 4) != 0) {
        my_printf("Invalid fmt chunk\n");
        return -1;
    }
    
    *channels = wav->channels;
    *sample_rate = wav->samplespersec;
    *bits = wav->bitspersample;
    
    my_printf("WAV: channels=%d rate=%d bits=%d format=%d\n",
        *channels, *sample_rate, *bits, wav->format);
    
    // 只支持 16位立体声 PCM
    if (wav->format != 1 || *bits != 16 || *channels != 2) {
        my_printf("Unsupported format, need 16-bit stereo PCM\n");
        return -1;
    }
    
    // 查找 data 块
    const uint8_t* ptr = wav_data + 12 + 8 + wav->fmtsize;
    while (ptr + 8 <= wav_data + wav_size) {
        if (my_memcmp(ptr, "data", 4) == 0) {
            *pcm_size = *(uint32_t*)(ptr + 4);
            *pcm_out = (uint8_t*)(ptr + 8);
            my_printf("PCM data: addr=%x size=%d\n", *pcm_out, *pcm_size);
            return 0;
        }
        uint32_t chunk_size = *(uint32_t*)(ptr + 4);
        ptr += 8 + chunk_size;
    }
    
    my_printf("data chunk not found\n");
    return -1;
}

// ==================== 主播放函数 ====================
int ac97_play_wav(const uint8_t* wav_buffer, uint32_t wav_size) {
    my_printf("=== AC97 Play WAV ===\n");
    
    uint8_t* pcm_data;
    uint32_t pcm_len;
    uint16_t channels, bits;
    uint32_t sample_rate;
    
    // 1. 解析 WAV
    if (parse_wav(wav_buffer, wav_size, &pcm_data, &pcm_len, &channels, &sample_rate, &bits) != 0) {
        return -1;
    }
    
    // 2. 查找 AC97 硬件
    uint8_t bus, dev, func;
    uint16_t nabmbar, namba;
    if (find_ac97_device(&bus, &dev, &func, &nabmbar, &namba) != 0) {
        my_printf("AC97 device not found\n");
        return -1;
    }
    
    g_ac97_exist = 1;
    
    // 3. 启用 Bus Master
    uint16_t cmd = pci_read_config_word(bus, dev, func, 0x04);
    if ((cmd & 0x04) == 0) {
        cmd |= 0x04;
        pci_write_config_word(bus, dev, func, 0x04, cmd);
        my_printf("Bus Master enabled\n");
    }
    
    // 4. 初始化控制器和 Codec
    uint16_t base_port = nabmbar + PCM_OUT_BASE;
    ac97_reset_controller(base_port);
    init_ac97_codec(namba, sample_rate);
    
    // 5. 准备 DMA 描述符
    // 注意：每个描述符最大 65536 字节（len 字段 16位，值 = 字节数-1）
    // ICH 限制每个缓冲区不超过 65536 字节
    const uint32_t MAX_BLOCK = 65536;
    int num_desc = (pcm_len + MAX_BLOCK - 1) / MAX_BLOCK;
    if (num_desc > 32) num_desc = 32;
    if (num_desc == 0) return -1;
    
    my_printf("Creating %d descriptors for %d bytes\n", num_desc, pcm_len);
    
    // 清零描述符表
    for (int i = 0; i < 32; i++) {
        desc_table[i].addr = 0;
        desc_table[i].len = 0;
        desc_table[i].ctrl = 0;
    }
    
    uint32_t offset = 0;
    for (int i = 0; i < num_desc; i++) {
        uint32_t block = pcm_len - offset;
        if (block > MAX_BLOCK) block = MAX_BLOCK;
        
        // 关键：addr 必须是物理地址！
        // 如果 OS 使用恒等映射，线性地址 = 物理地址
        // 否则需要转换
        desc_table[i].addr = (uint32_t)(pcm_data + offset);
        desc_table[i].len = (uint16_t)(block - 1);  // ICH 规范：长度-1
        desc_table[i].ctrl = (i == num_desc - 1) ? DESC_BUP : 0;  // 最后一块
        
        my_printf("Desc[%d]: addr=%x len=%d ctrl=%x\n",
            i, desc_table[i].addr, desc_table[i].len, desc_table[i].ctrl);
        
        offset += block;
    }
    
    // 6. 启动 DMA 播放
    ac97_dma_start(base_port, (uint32_t)desc_table, num_desc);
    
    // 7. 等待完成
    ac97_dma_wait_complete(base_port, num_desc);
    
    my_printf("Playback complete\n");
    return 0;
}

// ==================== 文件播放 ====================
int ac97_play_wav_file(char* filename) {
    my_printf("Playing file: %s\n", filename);
    
    // 分配 64MB 缓冲区
    char* filedata = (char*)__kMalloc(0x4000000);
    if (filedata == 0) {
        my_printf("Memory allocation failed\n");
        return -1;
    }
    
    int filesize = readFile(filename, &filedata);
    if (filesize <= 0) {
        my_printf("File read failed: %d\n", filesize);
        return -1;
    }
    
    my_printf("File size: %d bytes\n", filesize);
    
    // 分块播放，每块 1MB
    const int BLOCK_SIZE = 0x100000;
    int cnt = filesize / BLOCK_SIZE;
    int mod = filesize % BLOCK_SIZE;
    
    for (int i = 0; i < cnt; i++) {
        my_printf("Playing block %d/%d\n", i + 1, cnt + (mod ? 1 : 0));
        __memcpy((char*)ISA_DMA_BUFFER, filedata + i * BLOCK_SIZE, BLOCK_SIZE);
        ac97_play_wav((const uint8_t*)ISA_DMA_BUFFER, BLOCK_SIZE);
    }
    
    if (mod) {
        my_printf("Playing final block: %d bytes\n", mod);
        __memcpy((char*)ISA_DMA_BUFFER, filedata + cnt * BLOCK_SIZE, mod);
        ac97_play_wav((const uint8_t*)ISA_DMA_BUFFER, mod);
    }
    
    my_printf("File playback complete\n");
    return 0;
}