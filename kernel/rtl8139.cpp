
#include "rtl8139.h"
#include "pci.h"
#include "hardware.h"

unsigned char gMac[6] = { 0 };

int g_nic_iobase = 0;

int g_nic_dev = 0;

// ���ջ�������С (8KB)
#define RX_BUF_SIZE 8192
uint8_t rx_buffer[RX_BUF_SIZE];


void GetNicMac() {
	for (int i = 0; i < 6; i++) {
		gMac[i] = inportb(g_nic_iobase + i);
	}
}

int initNIC() {

	DWORD regs[16];
	DWORD dev = 0;
	DWORD vd = 0;
	int ret = getPciDevBasePort(regs, 0x0200, &dev, &vd);
    g_nic_dev = dev;
	if (vd == 0x802910ec) {
		g_nic_iobase = regs[0] & 0xfff8;
        GetNicMac();
        rtl8139_init(g_nic_iobase);
	}

	return 0;
}

// ��ʼ�� RTL8139 ����
int rtl8139_init(uint16_t io_base) {

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
    uint32_t rx_buf_phys = (uint32_t)rx_buffer;  // �����������ַ
    outportd(io_base + RBSTART, rx_buf_phys);

    // 3. ���ý��տ��ƼĴ��� (RCR)
    // ���ܹ㲥�����ಥ������ȷ�������ý���
    outportd(io_base + RCR, 0x00008F);

    // 4. �����ж�����Ĵ��� (IMR)
    outportw(io_base + IMR, 0x0005);  // ���ý���OK�ͷ���OK�ж�

    // 5. ���÷��ͺͽ���
    outportb(io_base + CR, 0x0C);  // ����TE(����ʹ��)��RE(����ʹ��)λ

    //printf("RTL8139 initialized successfully at I/O base 0x%04X\n", io_base);
    return 0;
}





