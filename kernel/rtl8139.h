#pragma once


#include "def.h"




// RTL8139 PCI �豸��Ϣ
#define RTL8139_VENDOR_ID  0x10EC
#define RTL8139_DEVICE_ID  0x8139

// PCI ���ÿռ���ʶ˿�
#define PCI_CONFIG_ADDRESS  0x0CF8
#define PCI_CONFIG_DATA     0x0CFC

// RTL8139 �Ĵ���ƫ����
#define IDR0        0x00   // ����ID/�豸ID
#define COMMAND     0x37   // ����Ĵ���
#define IMR         0x3C   // �ж����μĴ���
#define RCR         0x44   // �������üĴ���
#define CONFIG1     0x52   // ���üĴ���1
#define TSAD        0x20   // ����״̬�Ĵ�������
#define TSD         0x10   // ��������������
#define RBSTART     0x30   // ���ջ�������ʼ��ַ
#define CR          0x37   // ����Ĵ���
#define CAPR        0x38   // ��ǰ���ݰ���ȡָ��

int rtl8139_init(uint16_t io_base);

int initNIC();