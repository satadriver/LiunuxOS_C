#pragma once


#include "def.h"




// RTL8139 PCI 设备信息
#define RTL8139_VENDOR_ID  0x10EC
#define RTL8139_DEVICE_ID  0x8139

// PCI 配置空间访问端口
#define PCI_CONFIG_ADDRESS  0x0CF8
#define PCI_CONFIG_DATA     0x0CFC

// RTL8139 寄存器偏移量
#define IDR0        0x00   // 厂商ID/设备ID
#define COMMAND     0x37   // 命令寄存器
#define IMR         0x3C   // 中断屏蔽寄存器
#define RCR         0x44   // 接收配置寄存器
#define CONFIG1     0x52   // 配置寄存器1
#define TSAD        0x20   // 传输状态寄存器数组
#define TSD         0x10   // 传输描述符数组
#define RBSTART     0x30   // 接收缓冲区起始地址
#define CR          0x37   // 命令寄存器
#define CAPR        0x38   // 当前数据包读取指针

int rtl8139_init(uint16_t io_base);

int initNIC();