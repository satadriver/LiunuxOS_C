#include "def.h"
#include "libc.h"
#include "ac97.h"
#include "device.h"
#include "hardware.h"

#include "math.h"
#include "malloc.h"
#include "file.h"


extern "C" __declspec(dllexport) int g_ac97_exist = 0;

static volatile uint16_t* pcm_out_base;   // 映射到 I/O 空间

// 描述符表 (最多32项, 必须16字节对齐, 这里按32字节对齐)
__declspec(align(32))  ac97_desc_t desc_table[32];


// 读取配置空间双字
static uint32_t pci_read_config_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t addr = (1UL << 31) | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC);
    outportd(PCI_CONFIG_ADDR, addr);
    return inportd(PCI_CONFIG_DATA);
}

// 写入配置空间双字
static void pci_write_config_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t val) {
    uint32_t addr = (1UL << 31) | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC);
    outportd(PCI_CONFIG_ADDR, addr);
    outportd(PCI_CONFIG_DATA, val);
}

// 读取配置空间字（16位）
static uint16_t pci_read_config_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t dword = pci_read_config_dword(bus, slot, func, offset);
    // 偏移量低2位决定是哪个字：0->低16位，2->高16位
    return (uint16_t)((dword >> ((offset & 2) * 8)) & 0xFFFF);
}

// 写入配置空间字（16位）
static void pci_write_config_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t val) {
    uint32_t dword = pci_read_config_dword(bus, slot, func, offset);
    uint32_t mask = 0xFFFFUL << ((offset & 2) * 8);
    uint32_t new_dword = (dword & ~mask) | (((uint32_t)val) << ((offset & 2) * 8));
    pci_write_config_dword(bus, slot, func, offset, new_dword);
}

static uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t addr = (1UL << 31) | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC);
    outportd(PCI_CONFIG_ADDR, addr);
    return inportd(PCI_CONFIG_DATA);
}


static uint32_t pci_write_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset,int cmd) {
    uint32_t addr = (1UL << 31) | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC);
    outportd(PCI_CONFIG_ADDR, addr);

    outportd(PCI_CONFIG_DATA, cmd);
    return 0;
}

// 扫描 AC97 音频设备 (Class=0x04, Subclass=0x01, ProgIF=0x00/0x01)
// 返回: 0成功, -1失败; 并返回总线、设备、功能号及NABMBAR/NAMBA基址
static int find_ac97_device(uint8_t* bus, uint8_t* dev, uint8_t* func,
    uint16_t* nabmbar, uint16_t* namba) {
    for (uint8_t b = 0; b < 0x100; b++) {
        for (uint8_t d = 0; d < 32; d++) {
            for (uint8_t f = 0; f < 8; f++) {
                uint32_t class_rev = pci_read_config(b, d, f, 0x08);
                uint8_t _class = (class_rev >> 24) & 0xFF;
                uint8_t subclass = (class_rev >> 16) & 0xFF;
                uint8_t prog_if = (class_rev >> 8) & 0xFF;
                if (_class == 0x04 && subclass == 0x01 && (prog_if == 0x00 || prog_if == 0x01)) {
                    uint32_t bar0 = pci_read_config(b, d, f, 0x10);   // NABMBAR
                    uint32_t bar1 = pci_read_config(b, d, f, 0x14);   // NAMBA
                    if ((bar0 & 1) == 0) continue;    // 必须为 I/O 空间
                    *nabmbar = (uint16_t)(bar0 & 0xFFFE);
                    *namba = (uint16_t)(bar1 & 0xFFFE);
                    *bus = b; *dev = d; *func = f;
                    return 0;
                }
            }
        }
    }
    return -1;
}



static void ac97_codec_write(uint16_t mixer_base, uint8_t reg, uint16_t val) {
    outportb(mixer_base + 0x00, reg & 0x7F);
    outportw(mixer_base + 0x02, val);
    while (inportb(mixer_base + 0x00) & 0x80);   // 等待完成
}

static uint16_t ac97_codec_read(uint16_t mixer_base, uint8_t reg) {
    outportb(mixer_base + 0x00, reg | 0x80);
    while (inportb(mixer_base + 0x00) & 0x80);
    return inportw(mixer_base + 0x02);
}

// 初始化 Codec：复位、最大音量、设置采样率（尝试可变速率）
static void init_ac97_codec(uint16_t mixer_base, uint32_t sample_rate) {
    ac97_codec_write(mixer_base, AC97_RESET, 0);
    // 设置主音量和 PCM 音量为最大 (0x1F1F)
    ac97_codec_write(mixer_base, AC97_MASTER_VOL, 0x1F1F);
    ac97_codec_write(mixer_base, AC97_PCM_OUT_VOL, 0x1F1F);
    // 检测是否支持可变速率 (VRA)
    uint16_t ext_id = ac97_codec_read(mixer_base, AC97_EXTENDED_ID);
    if (ext_id & (1 << 3)) {
        // 写入采样率
        ac97_codec_write(mixer_base, AC97_PCM_FRONT_DAC_RATE, (uint16_t)sample_rate);
        // 启用 VRA 位
        uint16_t ext_stat = ac97_codec_read(mixer_base, AC97_EXTENDED_STAT);
        ext_stat |= (1 << 3);
        ac97_codec_write(mixer_base, AC97_EXTENDED_STAT, ext_stat);
    }
    // 若不支持 VRA，硬件固定为 48kHz，调用者需保证 sample_rate 为 48000
}




// 启动 DMA 传输
static void ac97_dma_start(int num_desc) {
    // 描述符表物理地址 (这里使用线性地址，假定恒等映射)
    uint32_t phys_desc = (uint32_t)desc_table;
    outportd((uint32_t)pcm_out_base + PI_INDEX, phys_desc);
    outportb((uint32_t)pcm_out_base + PI_LVI, (uint8_t)(num_desc - 1));

    // 复位控制器和 FIFO
    uint16_t cr = inportw((uint32_t)pcm_out_base + PI_CR);
    cr |= CR_RPBM | CR_RFC;
    outportw((uint32_t)pcm_out_base + PI_CR, cr);
    cr &= ~(CR_RPBM | CR_RFC);
    cr |= CR_LD;                // 加载描述符
    outportw((uint32_t)pcm_out_base + PI_CR, cr);
    cr |= CR_RD;                // 启动运行
    outportw((uint32_t)pcm_out_base + PI_CR, cr);
}

// 轮询等待传输完成 (不使用中断)
static void ac97_dma_wait_complete(int num_desc) {

    while (1) {
        uint16_t sr = inportw((uint32_t)pcm_out_base + PI_SR);
        if (sr & 0x0002) {          // 描述符完成标志
            outportw((uint32_t)pcm_out_base + PI_SR, sr);   // 写1清除
            uint8_t civ = inportb((uint32_t)pcm_out_base + PI_CIV);
            if (civ == (uint8_t)(num_desc - 1))
                break;
        }
    }
}

/***************************** WAV 文件解析 *****************************/
// 简易内存比较 (避免依赖库未实现)
static int my_memcmp_new(const void* s1, const void* s2, size_t n) {
    uint8_t* a = ( uint8_t * )s1, * b = (uint8_t * )s2;
    for (size_t i = 0; i < n; i++)
        if (a[i] != b[i]) return (a[i] < b[i]) ? -1 : 1;
    return 0;
}

// 解析 WAV 头部, 提取 PCM 数据和参数
static int parse_wav(const uint8_t* wav_data, uint32_t wav_size,
    uint8_t** pcm_out, uint32_t* pcm_size,
    uint16_t* channels, uint32_t* sample_rate, uint16_t* bits) {
    if (wav_size < 44) return -1;

    // RIFF 头
    const uint8_t* riff = wav_data;
    if (my_memcmp(riff, "RIFF", 4) != 0 || my_memcmp(riff + 8, "WAVE", 4) != 0)
        return -1;

    WAV_FILE_HEADER* wav = (WAV_FILE_HEADER * )(wav_data );
    const uint8_t* ptr = wav_data + 12;   // 跳过 RIFF 主块
    // 查找 fmt 子块
    while (ptr + 8 <= wav_data + wav_size) {
        if (my_memcmp(ptr, "fmt ", 4) == 0) {
            uint32_t fmt_size = *(uint32_t*)(ptr + 4);
            if (fmt_size >= 16) {
                *channels = *(uint16_t*)(ptr + 8 + 2);
                
                *bits = *(uint16_t*)(ptr + 8 + 14);
                *sample_rate = *(uint32_t*)(ptr + 8 + 4);

                *channels = wav->channels;
                *sample_rate = wav->samplespersec;
                *bits = wav->bitspersample;
                
                if (*bits != 16 || *channels != 2)   // 只支持 16位立体声
                    return -1;
                ptr += 8 + fmt_size;
                break;
            }
        }
        ptr += 8 + *(uint32_t*)(ptr + 4);
    }

    // 查找 data 子块
    while (ptr + 8 <= wav_data + wav_size) {
        if (my_memcmp(ptr, "data", 4) == 0) {
            *pcm_size = *(uint32_t*)(ptr + 4);
            *pcm_out = (uint8_t*)(ptr + 8);
            return 0;
        }
        ptr += 8 + *(uint32_t*)(ptr + 4);
    }
    return -1;
}

/***************************** 主播放函数 *****************************/
// 参数: wav_buffer - WAV 文件在内存中的基址 (物理地址, 恒等映射)
//       wav_size   - 文件大小 (字节)
// 返回值: 0 成功, -1 失败
int ac97_play_wav(const uint8_t* wav_buffer, uint32_t wav_size) {
    uint8_t* pcm_data;
    uint32_t pcm_len;
    uint16_t channels, bits;
    uint32_t sample_rate;

    my_printf("%s %d\r\n", __FUNCTION__, __LINE__);

    // 1. 解析 WAV 文件
    if (parse_wav(wav_buffer, wav_size, &pcm_data, &pcm_len,&channels, &sample_rate, &bits) != 0)
        return -1;

    my_printf("pcm:%x size:%x channels:%d sample rate:%d bits:%d\r\n", pcm_data, pcm_len, channels, sample_rate, bits);

    // 2. 查找 AC97 硬件
    uint8_t bus, dev, func;
    uint16_t nabmbar, namba;
    if (find_ac97_device(&bus, &dev, &func, &nabmbar, &namba) != 0)
        return -1;

    my_printf("bus:%x dev:%x func:%d nabmbar:%x namba:%x\r\n", bus, dev, func, nabmbar, namba);

    uint16_t cmd = pci_read_config_word(bus, dev, func, 0x04);
    cmd |= 0x0004;   // Bus Master Enable
    pci_write_config_word(bus, dev, func, 0x04, cmd);



    // 3. 初始化总线主控 (PCM输出通道)
    pcm_out_base = (volatile uint16_t*)(nabmbar + PCM_OUT_BASE);
    outportw((uint32_t)pcm_out_base + PI_CR, CR_RPBM | CR_RFC);   // 复位停止

    // 4. 初始化 AC97 Codec
    init_ac97_codec(namba, sample_rate);

    // 5. 准备 DMA 描述符 (每个块最大 4096 字节, 符合 ICH 限制)
    int num_desc = (pcm_len + 4095) / 4096;
    if (num_desc > 32) num_desc = 32;      // 硬件最多支持 32 个描述符
    if (num_desc == 0) return -1;

    uint32_t offset = 0;
    for (int i = 0; i < num_desc; i++) {
        uint32_t block = pcm_len - offset;
        if (block > 4096) block = 4096;
        desc_table[i].addr = (uint32_t)(pcm_data + offset);
        desc_table[i].ctrl_len = (block - 1) | (((i == num_desc - 1) ? 1UL : 0) << 31);
        offset += block;
    }



    uint16_t base_port = nabmbar + PCM_OUT_BASE;  // 通常 nabmbar 就是基址

    // 复位 DMA
    outportw(base_port + PI_CR, CR_RPBM | CR_RFC);
    outportw(base_port + PI_CR, 0);

    // 设置描述符表地址
    outportd(base_port + PI_INDEX, (uint32_t)desc_table);
    outportb(base_port + PI_LVI, (uint8_t)(num_desc - 1));

    // 启动 DMA
    uint16_t cr = inportw(base_port + PI_CR);
    cr |= CR_LD;
    outportw(base_port + PI_CR, cr);
    cr |= CR_RD;
    outportw(base_port + PI_CR, cr);



    // 6. 启动 DMA 播放
    ac97_dma_start(num_desc);

    // 7. 轮询等待完成
    ac97_dma_wait_complete(num_desc);
	__sleep(0);   // 这里简单睡眠，实际使用中应使用更合适的同步机制

    // 8. 停止 DMA 控制器
    outportw((uint32_t)pcm_out_base + PI_CR, 0);
    return 0;
}


int ac97_play_wav_file(char *filename) {

    char* filedata = (char*)__kMalloc(0x4000000);
    if (filedata == 0) {
        return 0;
    }
    int filesize = readFile(filename, &filedata);
    if (filesize) {
		int cnt = filesize / 0x100000;
        for(int i = 0; i < cnt; i++) {
			__memcpy((char*)ISA_DMA_BUFFER, filedata + i * 0x100000, 0x100000);
            ac97_play_wav((const unsigned char*)ISA_DMA_BUFFER, 0x100000);
		}
        int mod = filesize % 0x100000;
        if (mod) {
            __memcpy((char*)ISA_DMA_BUFFER, filedata + cnt * 0x100000, mod);
            ac97_play_wav((const unsigned char*)ISA_DMA_BUFFER, mod);
        }
    }
    return 0;
}