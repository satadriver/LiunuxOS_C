#pragma once


#include "task.h"


#define FD_SEEK			0x0f
#define FD_RECALIBRATE	7
#define FD_READ			0xe6
#define FD_SPECIFY		7

#define FLOPPY_REG_DOR	0x3f2
#define FD_STATUS		0x3f4
#define FD_DATA			0x3f5
#define FD_DCR			0x3f7

#define STATUS_READY	8
#define STATUS_DIR		0x40

#define SPT				18
#define HPC				2




#pragma pack(1)

typedef struct {
	unsigned int size, sect, head, track, stretch;
	unsigned char gap, rate, spec1;
}floppy_struct;

#pragma pack()


int getCylinder(int lba);

int getHeader(int lba);

int getSector(int lba);



#ifdef DLL_EXPORT
extern "C" int __declspec(dllexport) readFloppySector(int dev, DWORD buf, int secnum, int seccnt);
extern "C" void __declspec(dllexport) FloppyIntProc(LIGHT_ENVIRONMENT* stack);
#else
extern "C" int __declspec(dllimport) readFloppySector(int dev, DWORD buf, int secnum, int seccnt);
extern "C" void __declspec(dllimport) FloppyIntProc(LIGHT_ENVIRONMENT* stack);
#endif


/*
0x3f2          Write            DOR: Digital Output Register
0x3f4          Read             FDC Status: Floppy Disk Status Register
0x3f5          Read/Write       FDC Data: Floppy Disk Data Register
0x3f7          Read             DIR: Digital Input Register
0x3f7          Write            DCR: Disk Control Register

DOR:
7    MOT_EN3    Driver D motor：1-start；0-stop
6    MOT_EN2    Driver C motor：1-start；0-stop
5    MOT_EN1    Driver B motor：1-start；0-stop
4    MOT_EN0    Driver A motor：1-start；0-stop
3    DMA_INT    DMA interrupt； 1 enable; 0-disable
2    RESET		FDC Reset
1    DRV_SEL1	Select driver 1/0
0    DRV_SEL0

FDC Status:
7    RQM    Data ready: FDD ready
6    DIO    Direction: 1 - FDD to CPU; 0 – CPU to FDD
5    NDM    DMA set: 1-not DMA; 0-DMA
4    CB		Controller busy
3    DDB    Driver D busy
2    DCB    Driver C busy
1    DBB    Driver B busy
0    DAB    Driver A busy

FDC Data:

1)  重新校正命令（FD_RECALIBRATE）
软盘启动时调用
阶段    序    D7  D6  D5  D4  D3  D2  D1  D0  说明
cmd      0    0   0   0   0   0   1   1   1   0x07
		 1    0   0   0   0   0   0   US1 US2 Drive no.

执行                                        磁头移动到track0


2)  磁头寻道命令（FD_SEEK）
把磁头定位到制定位置，在读写前执行

阶段    序  D7  D6  D5  D4  D3  D2  D1  D0  说明

cmd      0   0   0   0   0   0   1   1   1   0x0F

		 1   0   0   0   0   0   HD  US1 US2 磁头号、驱动器号

		 2   C                               磁道号

执行										 磁头移动到制定磁道


3)  读扇区数据命令（FD_READ）

阶段    序  D7  D6  D5  D4  D3  D2  D1  D0  说明
cmd     0   MT  MF  SK  0   0   1   1   0   0xE6（MT=MF=SK=1）
		1   0   0   0   0   0   0   US1 US2 驱动器号
		2   C                               磁道号track
		3   H                               磁头号head
		4   R                               起始扇区号start sector
		5   N                               扇区字节数
		6   EOT                             磁道最大扇区号
		7   GPL                             扇区建间隔长度（3）
		8   DTL                             N=0时，制定扇区字节书

执行                                        从软盘读取扇区
结果    1    ST0    状态字节0
		2    ST1    状态字节1
		3    ST2    状态字节2
		4    C		磁道号track
		5    H		磁头号head
		6    R		起始扇区号
		7    N		扇区字节数
注：
MT：多磁道操作。MT=1表示允许多磁道操作
MF：记录方式。MF=1表示选用MFM记录方式，否则是FM记录方式d
SK：是否跳过有删除标志的扇区。SK=1表示跳过。

返回的返回的状态ST0、ST1和ST2的含义如下：
ST0：
位   名称			说明
7-6  ST0_INTR		中断原因。00-正常结束；01-异常结束；10-命令无效；11-软盘驱动器状态改变
5    ST0_SE			寻道操作或重新校正操作结束（seek end）
4    ST0_ECE		设备检查错误（0磁道校正错误）（Equip. Check Error）
3    ST0_NR			软盘未就绪（Not Ready）
2    ST0_HA			磁头地址。中断时磁头地址（Head Address）
1-0  ST0_DS			驱动器号（Driver Select）

ST1：
位    名称    说明
7    ST1_EOC   范文超过磁道最大扇区号（End of Cylinder）
6    Reserve
5    ST1_CRC   CRC校验出错
4    ST1_OR    数据传输超时（Over Run）
3    Reserve
2    ST1_ND    未找到制定扇区（No Data）
1    ST1_WP    写保护（Write Protect）
0    ST1_MAM   未找到扇区地址标志ID（Miss Address Mask）

ST2：
位    名称    说明
7    Reserve
6    ST2_CM		SK=0时，读数据遇到删除标志（Control Mark）
5    ST2_CRC    CRC校验出错
4    ST2_WC		扇区ID信息的磁道号C不不符（Wrong Cylinder）
3    ST2_SEH    检索条件满足（Scan Equal Hit）
2    ST2_SNS    检索条件不满足（Scan Not Satisfied）
1    ST2_BC		磁道坏（Bad Cylinder）
0    ST2_MAM    未找到扇区地址标志ID（Miss Address Mask）

4)  写扇区数据命令(FD_WRITE)

阶段  序  D7  D6  D5  D4  D3  D2  D1  D0  说明
cmd   0   MT  MF  0   0   0   1   0   1   0xC5（MT=MF=1）
	  1   0   0   0   0   0   0   US1 US2 磁头号、驱动器号
	  2   C                               磁道号track
	  3   H                               磁头号head
	  4   R                               起始扇区号start sector
	  5   N                               扇区字节数
	  6   EOT                             磁道最大扇区号
	  7   GPL                             扇区建间隔长度（3）
	  8   DTL                             N=0时，制定扇区字节数

执行                                      向软盘写入扇区

结果  1  ST0   状态字节0
	  2  ST1   状态字节1
	  3  ST2   状态字节2

5)  检测中断状态命令（FD_SENSEI）
阶段  序  D7  D6  D5  D4  D3  D2  D1  D0  说明
cmd   0   0   0   0   0   0   1   1   1   0x08

执行结果
1 ST0  状态字节0
2 磁头所在磁道号

6）  设定驱动器参数命令（FD_SPECIFY）
阶段  序  D7  D6  D5  D4  D3  D2  D1  D0  说明
cmd   0   0   0   0   0   0   1   1   1   0x03
	  1   4-7:SRT单位2ms,0-3:HUT单位32ms  马达速度、磁头卸载时间
	  2   1-7:HLT单位4ms,0:ND             磁头加载时间，非DMA模式

执行                                      设置控制器
结果      无                              无


DIR：
数字输入寄存器
DIR寄存器只有D7位有效，用于表示软盘更换状态，其余用于硬盘控制器。

DCR：
磁盘控制寄存器
DCR仅用D0与D1位，用于表示数据传输率。00-500kpbs, 01-300kpbs, 10-250kpbs。

*/

