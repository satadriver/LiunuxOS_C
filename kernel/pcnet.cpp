#include "def.h"
#include "hardware.h"
#include "pci.h"

#define outd outportd
#define outw outportw

#define ind inportd
#define inw inportw

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