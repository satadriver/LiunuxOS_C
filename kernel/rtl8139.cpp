
#include "rtl8139.h"
#include "pci.h"
#include "hardware.h"
#include "Utils.h"

/*
ffff:0 f000:e05b
Master booter��������Ҫ����Щ���飺
���MAGIC��Signature���Ƿ�Ϊ�Ϸ�ֵ��ʮ������55 AA����
�� �Լ��ƶ�������λ�ã�һ����0x0600������0x7C00��0x7c00+512K�Ŀռ��ó�����
�Ա����boot sector����װ�����λ�ã��������ܺ�ֱ�Ӵ�����ֱ��װ��boot sector������һ��
*/

unsigned char gMac[6] = { 0 };

int g_nic_iobase = 0;

int g_nic_dev = 0;

// ���ջ�������С (8KB)
#define RX_BUF_SIZE 8192
uint8_t rx_buffer8139[RX_BUF_SIZE];


void GetNicMac() {
	for (int i = 0; i < 6; i++) {
		gMac[i] = inportb(g_nic_iobase + i);
	}
}

int initNIC() {
    char szout[1024];
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

// ��ʼ�� RTL8139 ����
int rtl8139_init(uint16_t io_base) {
    char szout[1024];

    outportd(0xcf8, g_nic_dev + 4);
    DWORD v = inportd(0xcfc);
    v = v | 5;
    outportd(0xcfc, v);

    outportb(io_base + 0x52, 0x0);

    // 1. ��λ����
    outportb(io_base + 0x37, 0x10);
    while ((inportb(io_base + 0x37) & 0x10) != 0)
    {

    }

    // 2. ���ý��ջ�����
    uint32_t rx_buf_phys = (uint32_t)rx_buffer8139;  // �����������ַ
    outportd(io_base + RBSTART, rx_buf_phys);

    // 3. ���ý��տ��ƼĴ��� (RCR)
    // ���ܹ㲥�����ಥ������ȷ�������ý���
    outportd(io_base + RCR, 0x00008F);

    // 4. �����ж�����Ĵ��� (IMR)
    outportw(io_base + IMR, 0x0005);  // ���ý���OK�ͷ���OK�ж�

    // 5. ���÷��ͺͽ���
    outportb(io_base + CR, 0x0C);  // ����TE(����ʹ��)��RE(����ʹ��)λ

    __printf(szout,"RTL8139 initialized successfully at I/O base %x\n", io_base);
    return 0;
}





