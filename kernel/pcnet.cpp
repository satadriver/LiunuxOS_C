#include "def.h"
#include "hardware.h"
#include "pci.h"
#include "Utils.h"

#define outb outportb
#define outd outportd
#define outw outportw
#define outl outd

#define inb inportb
#define ind inportd
#define inw inportw
#define inl ind

DWORD io_base = 0;

void writeRAP32(uint32_t val)
{
    outd(io_base + 0x14, val);
}

void writeRAP16(uint16_t val)
{
    outw(io_base + 0x12, val);
}

uint32_t readCSR32(uint32_t csr_no)
{
    writeRAP32(csr_no);
    return ind(io_base + 0x10);
}

uint16_t readCSR16(uint16_t csr_no)
{
    writeRAP32(csr_no);
    return inw(io_base + 0x10);
}

void writeCSR32(uint32_t csr_no, uint32_t val)
{
    writeRAP32(csr_no);
    outd(io_base + 0x10, val);
}

void writeCSR16(uint16_t csr_no, uint16_t val)
{
    writeRAP16(csr_no);
    outw(io_base + 0x10, val);
}












uint32_t readBCR32(uint32_t csr_no)
{
    writeRAP32(csr_no);
    return ind(io_base + 0x1c);
}

uint16_t readBCR16(uint16_t csr_no)
{
    writeRAP32(csr_no);
    return inw(io_base + 0x16);
}

void writeBCR32(uint32_t csr_no, uint32_t val)
{
    writeRAP32(csr_no);
    outd(io_base + 0x1c, val);
}

void writeBCR16(uint16_t csr_no, uint16_t val)
{
    writeRAP16(csr_no);
    outw(io_base + 0x16, val);
}


void ResetPcnet() {
    ind(io_base + 0x18);
    inw(io_base + 0x14);
}


void SWSTYLE() {
    uint32_t csr58 = readCSR32(58);
    csr58 &= 0xFF00;  // SWSTYLE is 8 bits (7:0)
    csr58 |= 2;
    writeCSR32(58, csr58);
}


void ASEL() {
    uint32_t bcr2 = readBCR32(2);
    bcr2 |= 0x2;
    writeBCR32(2, bcr2);
}

//bus:00,dev:3,function:00

void EnablePcnet() {
    SetPciReg(0, 3, 0, 4, 5);
}




#define PCNET_VENDOR_ID  0x1022
#define PCNET_DEVICE_ID  0x2000

struct pcnet_desc {
    uint32_t addr;
    uint16_t length;
    uint16_t flags;
};

struct pcnet_desc tx_ring[1], rx_ring[1];

uint8_t tx_buffer[1514+1000];
uint8_t rx_buffer[1514+1000];


// PCI ���ÿռ��д��ͨ�� IO �˿� 0xCF8/0xCFC��
uint32_t pci_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (1 << 31) | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC);
    outl(0xCF8, address);
    return inl(0xCFC);
}

// ���� AMD PCnet ����
int find_pcnet() {
    char szout[1024];
    for (int bus = 0; bus < 256; bus++) {
        for (int slot = 0; slot < 32; slot++) {
            uint32_t vendor_device = pci_read(bus, slot, 0, 0);
            uint16_t vendor = vendor_device & 0xFFFF;
            uint16_t device = vendor_device >> 16;

            if (vendor == PCNET_VENDOR_ID && device == PCNET_DEVICE_ID) {
                uint32_t port = pci_read(bus, slot, 0, 0x10);
                __printf(szout,"Found PCnet at %x:%x,bar:%x\n", bus, slot, port);
                return port & 0xfff0;
            }
        }
    }
    __printf(szout,"PCnet not found!\n");
    return 0;
}





// ͨ�� CSR0 ���͸�λ����
void pcnet_reset(uint16_t iobase) {
    outw(iobase + 0x14, 0x0004); // д�� CSR0 �� RST λ
    (inw(iobase + 0x14) & 0x0004);
    //while ((inw(iobase + 0x14) & 0x0004))
    {
        //break;
        //; // �ȴ���λ���
        
    }
}


// ���� MAC ��ַ��ʾ����00:11:22:33:44:55��
void pcnet_set_mac(uint16_t iobase, uint8_t mac[6]) {
    outw(iobase + 0x12, 0x0000); // ѡ�� CSR1
    for (int i = 0; i < 6; i++) {
        outb(iobase + 0x10, mac[i]); // д�� MAC ��ַ
    }
}

// ��ʼ�� CSR �Ĵ���
void pcnet_init_csr(uint16_t iobase) {
    outw(iobase + 0x12, 0x0002); // ѡ�� CSR2
    outw(iobase + 0x10, 0x0000); // ����ж�״̬

    outw(iobase + 0x12, 0x0003); // ѡ�� CSR3
    outw(iobase + 0x10, 0x0000); // �������߿�ȣ�16-bit��

    outw(iobase + 0x12, 0x0004); // ѡ�� CSR4
    outw(iobase + 0x10, 0x0115); // �����Զ�ѡ��ȫ˫��
}




void pcnet_init_desc(uint16_t iobase) {
    // ����������
    tx_ring[0].addr = (uint32_t)&tx_buffer;
    tx_ring[0].length = 0;
    tx_ring[0].flags = 0x8000; // OWN λ���������ƣ�

    // ����������
    rx_ring[0].addr = (uint32_t)&rx_buffer;
    rx_ring[0].length = 0;
    rx_ring[0].flags = 0x8000; // OWN λ

    // ����������������ַ
    outw(iobase + 0x12, 0x0001); // ѡ�� CSR1
    outw(iobase + 0x10, (uint32_t)&tx_ring >> 16);
    outw(iobase + 0x12, 0x0008); // ѡ�� CSR8
    outw(iobase + 0x10, (uint32_t)&tx_ring & 0xFFFF);
    // �������ý�����������ַ��CSR9/CSR10��
}


void pcnet_start(uint16_t iobase) {
    outw(iobase + 0x12, 0x0000); // ѡ�� CSR0
    outw(iobase + 0x10, 0x0102); // �����жϡ�����������STRT + INEA��
}

void pcnet_send(uint16_t iobase, uint8_t* data, uint16_t len) {
    __memcpy((char*)tx_buffer,(char*) data, len);
    tx_ring[0].length = len;
    tx_ring[0].flags = 0x8000; // �������� OWN λ

    outw(iobase + 0x12, 0x0000); // ѡ�� CSR0
    outw(iobase + 0x10, 0x0008); // �������ͣ�TDMD��
}

void pcnet_poll_rx(uint16_t iobase) {
    char szout[1024];
    if (!(rx_ring[0].flags & 0x8000)) { // OWN λ�������������ʾ������
        uint16_t len = rx_ring[0].length;
        __printf(szout,"Received %d bytes\n", len);
        // ���� rx_buffer ����...
        rx_ring[0].flags = 0x8000; // �黹������������
    }
}

/*
uint32_t bar0 = pci_read(bus, slot, 0, 0x10) & 0xFFFFFFFC;
if (bar0 & 1) {
    // I/O �˿�ģʽ
    uint16_t iobase = bar0 & 0xFFFC;
    printf("Using I/O Port: 0x%x\n", iobase);
}
else {
    // MMIO ģʽ
    printf("Using MMIO: 0x%x\n", bar0);
}
*/


int pcnetInit() {
    
    uint16_t iobase = find_pcnet(); // ���� PCI BAR0 ���ص� I/O �˿�
    if (iobase == 0) {
        return 0;
    }
    pcnet_reset(iobase);
    //pcnet_set_mac(iobase,  (unsigned char*)"\x12\x23\x34\x45\x56\x67");
    pcnet_init_csr(iobase);
    pcnet_init_desc(iobase);
    pcnet_start(iobase);

    // ���Ͳ������ݣ�ARP ����
    uint8_t arp_packet[100] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  };
    pcnet_send(iobase, arp_packet, sizeof(arp_packet));

    //while (1) 
    {
        pcnet_poll_rx(iobase);
        //break;
    }
    char szout[1024];

    __printf(szout, "pcnetInit end\n");
    return 0;
}