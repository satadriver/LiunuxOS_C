
#include "rtl8139.h"
#include "pci.h"
#include "hardware.h"
#include "Utils.h"

/*
ffff:0 f000:e05b
Master booter最起码需要做这些事情：
检测MAGIC（Signature）是否为合法值（十六进制55 AA）；
将 自己移动到其它位置（一般是0x0600），将0x7C00到0x7c00+512K的空间让出来，
以备其后将boot sector程序装入这个位置，这样才能和直接从软盘直接装入boot sector程序相一致
*/

unsigned char gMac[6] = { 0 };

int g_nic_iobase = 0;

int g_nic_dev = 0;

// 接收缓冲区大小 (8KB)
#define RX_BUF_SIZE 8192
uint8_t rx_buffer8139[RX_BUF_SIZE];


void GetNicMac() {
	for (int i = 0; i < 6; i++) {
		gMac[i] = inportb(g_nic_iobase + i);
	}
}

int initNIC() {
    char szout[256];
	DWORD regs[16];
	DWORD dev = 0;
	DWORD vd = 0;
	int ret = getPciDevBasePort(regs, 0x0200, &dev, &vd);
    g_nic_dev = dev;
	if (vd == 0x813910ec) 
    {
		g_nic_iobase = regs[0] & 0xfff8;
        GetNicMac();
        __printf(szout, "RTL8139 mac address: %x-%x-%x-%x-%x-%x\r\n",
            gMac[0], gMac[1], gMac[2], gMac[3], gMac[4], gMac[5]);
        rtl8139_init(g_nic_iobase);
	}

	return 0;
}

// 初始化 RTL8139 网卡
int rtl8139_init(uint16_t io_base) {
    char szout[256];

    outportd(0xcf8, g_nic_dev + 4);
    DWORD v = inportd(0xcfc);
    v = v | 5;
    outportd(0xcfc, v);

    outportb(io_base + 0x52, 0x0);

    // 1. 软复位网卡
    outportb(io_base + 0x37, 0x10);
    while ((inportb(io_base + 0x37) & 0x10) != 0)
    {

    }

    // 2. 配置接收缓冲区
    uint32_t rx_buf_phys = (uint32_t)rx_buffer8139;  // 假设是物理地址
    outportd(io_base + RBSTART, rx_buf_phys);

    // 3. 配置接收控制寄存器 (RCR)
    // 接受广播包、多播包、正确包，启用接收
    outportd(io_base + RCR, 0x00008F);

    // 4. 配置中断掩码寄存器 (IMR)
    outportw(io_base + IMR, 0x0005);  // 启用接收OK和发送OK中断

    // 5. 启用发送和接收
    outportb(io_base + CR, 0x0C);  // 设置TE(发送使能)和RE(接收使能)位

    __printf(szout,"RTL8139 initialized successfully at I/O base %x\n", io_base);
    return 0;
}





