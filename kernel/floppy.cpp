#include "floppy.h"
#include "hardware.h"
#include "video.h"
#include "Utils.h"
#include "task.h"
#include "device.h"

floppy_struct floppy_type ={ 2880,18,2,80,0,0x1B,0x00,0xCF }; /* 1.44MB diskette */


//80 18 2
int getCylinder(int lba) {
	return lba / (SPT * HPC);
}

int getHeader(int lba) {
	return  (lba / SPT) % HPC;
}

int getSector(int lba) {
	return lba % SPT + 1;
}


void wait_status() {
	int cnt = 0;
	do {
		__delay();

		int v = inportb(0x3f4);
		if (v & 80) {
			break;
		}
		else {
			__sleep(0);
		}
		cnt++;
	} while (cnt < 3);
}

//https://0cch.com/2013/09/21/floppy-disk-controllere7bc96e7a88b/



extern "C"  __declspec(dllexport)int readFloppySector(int dev,DWORD buf,int secnum,int seccnt) {
	int v = 0;

	int cylinder = getCylinder(secnum);
	int header = getHeader(secnum);
	int sector = getSector(secnum);

	int size = HPC * BYTES_PER_SECTOR * seccnt - 1;

	outportb(0x0d, 0xff);
	outportb(0x0a, 6);
	outportb(0x0c, 0xff);
	outportb(0x04, buf & 0xff);
	outportb(0x04, (buf >>8) & 0xff);
	outportb(0x81, (buf >> 16) & 0xff);
	outportb(0x0c, 0xff);
	outportb(0x5, size && 0xff);
	outportb(0x5, (size >> 8)&0xff);
	outportb(0xa, 0x2);

	outportb(0x3f2, 0x0);
	outportb(0x3f2, 0x0c);

	__sleep(0);

	for (int i = 0; i < 4; i++) {
		wait_status();
		outportb(0x3f5, 0x8);
		wait_status();
		v = inportb(0x3f5);
		wait_status();
		v = inportb(0x3f5);
		wait_status();
	}

	outportb(0x3f7, 0x0);

	wait_status();
	outportb(0x3f5, 3);
	wait_status();
	outportb(0x3f5, 0x0df);
	wait_status();
	outportb(0x3f5, 2);
	wait_status();

	outportb(0x3f2, 0x1c);
	wait_status();

	outportb(0x3f5, 7);
	wait_status();
	outportb(0x3f5, dev);
	wait_status();

	outportb(0x3f5, 8);
	wait_status();
	v = inportb(0x3f5);
	wait_status();
	v = inportb(0x3f5);

	outportb(0x3f2, 4);

	outportb(0x3f2, 0x1c);
	wait_status();

	outportb(0x3f5, 0xf);
	wait_status();
	outportb(0x3f5, 0);
	wait_status();
	outportb(0x3f5, 0);
	wait_status();

	outportb(0x3f5, 8);
	wait_status();
	v = inportb(0x3f5);
	wait_status();
	v = inportb(0x3f5);

	outportb(0x0a, 6);
	outportb(0x0b, 0x46);
	outportb(0x0a, 2);

	wait_status();
	outportb(0x3f5, 0xe6);
	wait_status();
	outportb(0x3f5, 0);
	wait_status();
	outportb(0x3f5, cylinder);
	wait_status();
	outportb(0x3f5, header);
	wait_status();
	outportb(0x3f5, sector);
	wait_status();
	outportb(0x3f5, HPC);
	wait_status();
	outportb(0x3f5, seccnt);
	wait_status();
	outportb(0x3f5, 0x1b);
	wait_status();
	outportb(0x3f5, 0xff);
	wait_status();

	__sleep(0);

	for (int i = 0; i < 7; i++) {
		wait_status();
		v = inportb(0x3f5);
	}

	wait_status();
	outportb(0x3f5, 8);
	wait_status();
	v = inportb(0x3f5);
	wait_status();
	v = inportb(0x3f5);
	wait_status();
	outportb(0x3f2, 4);
	return TRUE;
}



void __declspec(naked) FloppyIntProc(LIGHT_ENVIRONMENT* stack) {

	__asm {

		pushad
		push ds
		push es
		push fs
		push gs
		push ss

		push esp
		sub esp, 4
		push ebp
		mov ebp, esp

		mov eax, KERNEL_MODE_DATA
		mov ds, ax
		mov es, ax
		MOV FS, ax
		MOV GS, AX
	}

	{
		int v1 = inportb(0x3f5);
		int v2 = inportb(0x3f2);
		int v3 = inportb(0x3f4);

		char szout[1024];
		__printf(szout, (char*)"FloppyIntProc!\r\n");

		outportb(0x20, 0x20);

#ifdef APIC_ENABLE
		* (DWORD*)(LOCAL_APIC_BASE + 0xB0) = 0;
		*(DWORD*)(IO_APIC_BASE + 0x40) = 0;
#endif
	}

	__asm {
		mov esp, ebp
		pop ebp
		add esp, 4
		pop esp

		pop ss
		pop gs
		pop fs
		pop es
		pop ds
		popad

		iretd
	}
}


/*
__asm {
		; MASM的宏应该不陌生吧，就不做解释了。
	outb macro port:req, b:req
		mov dx, port
		mov al, b
		out dx, al
	endm

	inb macro port:req
		mov dx, port
		in al, dx
	endm

	wait_status macro
		inb 3f4h
	@@:
		test al, 80h
		jz @B
	endm

	; ISA DMA 初始化部分， 由于ISA DMA不是这篇文章的重点
	; 所以这里只说明大概的用途。
	outb 0dh, 0ffh      ; 重置DMA控制器
	outb 0ah, 6h        ; 选择设置2号DMA通道
	outb 0ch, 0ffh      ; 重置Flipflop寄存器
	outb 4h, 0h         ; 设置DMA的物理地址，需要设置两次
	outb 4h, 0f0h       ; 我这里设置的是0xf000
	outb 81h, 0h        ; 设置地址24bit的高8bit为0，也就是0x00f000
	outb 0ch, 0ffh      ; 重置Flipflop寄存器
	outb 5h, 0ffh       ; 设置物理内存大小，其大小为Length-1
	outb 5h, 0fh        ; 我这里的内存大小是0x1000，所以设置为0xfff
	outb 0ah, 2h        ; 选择清除2号DMA通道

	; FDC的初始化过程
	outb 3f2h, 0h       ; 1.重置数字输出寄存器
	outb 3f2h, 0ch
	call WaitIrq

	mov ecx, 4
	check_int:
	wait_status
	outb 3f5h, 8h       ; 发送8号命令，该命令清除控制器触发的中断
	wait_status         ; 并且返回结果，这里重复4次，是为了清除4个软驱的状态
	inb 3f5h
	wait_status
	inb 3f5h
	wait_status
	loop check_int

	outb 3f7h, 0h       ; 2.设置传输速度为500kb/s
	wait_status
	outb 3f5h, 3h       ; 3.设置FDC里面三个时钟以及DMA。
	wait_status
	outb 3f5h, 0dfh
	wait_status
	outb 3f5h, 2h

	outb 3f2h, 1ch      ; 开启软驱电动机
	wait_status
	outb 3f5h, 7h       ; 发送校验命令
	wait_status
	outb 3f5h, 0h       ; 选择0号软驱
	wait_status
	outb 3f5h, 8h       ; 发送清楚中断命令，获得结果
	wait_status
	inb 3f5h
	wait_status
	inb 3f5h
	outb 3f2h, 4h       ; 关闭电动机

	; 接下来是软盘的读取操作
	outb 3f2h, 1ch      ; 开启电动机
	wait_status
	outb 3f5h, 0fh      ; 4.发送0f寻道命令
	wait_status
	outb 3f5h, 0h
	wait_status
	outb 3f5h, 0h
	wait_status
	outb 3f5h, 8h       ; 发送清楚中断命令，获得结果
	wait_status
	inb 3f5h
	wait_status
	inb 3f5h

	; 设置ISA DMA为读取
	outb 0ah, 6h
	outb 0bh, 46h
	outb 0ah, 2h

	wait_status
	outb 3f5h, 0e6h     ; 5.发送读取扇区命令
	wait_status
	outb 3f5h, 0h       ; 设置磁头和驱动器号
	wait_status
	outb 3f5h, 0h       ; 设置磁道
	wait_status
	outb 3f5h, 0h       ; 设置磁头
	wait_status
	outb 3f5h, 1h       ; 设置扇区号
	wait_status
	outb 3f5h, 2h       ; 设置扇区大小
	wait_status
	outb 3f5h, 8h       ; 设置读取扇区数量
	wait_status
	outb 3f5h, 1bh      ; 设置磁盘为3.5英寸
	wait_status
	outb 3f5h, 0ffh     ; 设置读取长度，总是0xff

	call WaitIrq

	mov ecx, 7
	loop_ret:
	wait_status
	inb 3f5h
	loop loop_ret

	wait_status
	outb 3f5h, 8h       ; 发送清楚中断命令，获得结果
	wait_status
	inb 3f5h
	wait_status
	inb 3f5h
	outb 3f2h, 4h       ; 关闭电动机
}
*/