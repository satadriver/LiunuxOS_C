
#include "hardware.h"
#include "video.h"
#include "serialUART.h"
#include "Utils.h"
#include "apic.h"
#include "apic.h"

//https://wiki.osdev.org/Serial_Ports


char gCom1RecvBuf[COM_BUFFER_SIZE];
char * gCom1RecvBufHdr = gCom1RecvBuf;
char * gCom1RecvBufEnd = gCom1RecvBuf;

char gCom2RecvBuf[COM_BUFFER_SIZE];
char * gCom2RecvBufHdr = gCom2RecvBuf;
char * gCom2RecvBufEnd = gCom2RecvBuf;

/*
One of the registers hold what is termed the DLAB or Divisor Latch Access Bit. 
When this bit is set, 
offsets 0 and 1 are mapped to the low and high bytes of the Divisor register for setting the baud rate of the port.
When this bit is clear, offsets 0 and 1 are mapped to their normal registers. 
The DLAB bit only affects port offsets 0 and 1, the other offsets ignore this setting.

The serial controller (UART) has an internal clock which runs at 115200 ticks per second and a clock divisor which is used to control the baud rate.
This is exactly the same type of system used by the Programmable Interrupt Timer (PIT).

In order to set the speed of the port, calculate the divisor required for the given baud rate and program that in to the divisor register.
For example, a divisor of 1 will give 115200 baud, a divisor of 2 will give 57600 baud, 3 will give 38400 baud, etc.

Do not be tempted to use a divisor of 0 to try to get an infinite baud rate, it won't work.
Most serial controllers will generate a unspecified and unpredictable baud rate
(and anyway infinite baud would mean infinite transmission errors as they are proportional.)

To set the divisor to the controller:

Set the most significant bit of the Line Control Register. This is the DLAB bit, and allows access to the divisor registers.
Send the least significant byte of the divisor value to [PORT + 0].
Send the most significant byte of the divisor value to [PORT + 1].
Clear the most significant bit of the Line Control Register.
*/

void initSerialUART(int port) {

	outportb(port + 1, 0x00);    // Disable all interrupts

	//brd = 1843200/(16*baud)
	outportb(port + 3, 0x80);    // Enable DLAB (set baud rate divisor)
	outportb(port + 0, 0x01);    // Set divisor to 3 (lo byte) 115200 baud
	outportb(port + 1, 0x00);    // (high byte)
	outportb(port + 3, 0x03);    // 8 bits, no parity, one stop bit

	outportb(port + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold

									 //bit3:modem status change interrupt
									 //bit2:connection status change interrupt
									 //bit1:send buf empty interrupt
									 //bit0:recv buf full interrupt
	outportb(port + 1, 0x0f);    // enable all interrupts
								 
	//outportb(port + 4, 0x0B);    // IRQs enabled, RTS/DSR set

	outportb(port + 4, 0x1f);    // loop
}

//Transmitter Holding Register Empty: send ok
//Received Data Available: The Interrupt Trigger Level is used to configure how much data must be received in the FIFO Receive buffer 
//before triggering a Received Data Available Interrupt
//Receiver Line Status: check Line Status register


void initSerial() {
	initSerialUART(COM1PORT);
	initSerialUART(COM2PORT);
}


int sendUARTData(unsigned char* data, int size, int port) {
	for (int i = 0; i < size; )
	{
		unsigned char c = inportb(port + 5);
		if (c & 0x20)
		{
			outportb(port, *(data + i));

			i++;
		}
		else {
			__sleep(0);
		}
	}
	return size;
}


int getCom1Data(unsigned char * data) {
	if (gCom1RecvBufEnd == gCom1RecvBufHdr)
	{
		return 0;
	}

	int cnt = 0;
	while (gCom1RecvBufEnd != gCom1RecvBufHdr)
	{
		*(data + cnt)= * gCom1RecvBufEnd;
		gCom1RecvBufEnd++;
		if (gCom1RecvBufEnd >= gCom1RecvBuf + COM_BUFFER_SIZE)
		{
			gCom1RecvBufEnd = gCom1RecvBuf;
		}
		cnt++;
	}
	return cnt;
}



int getCom2Data(unsigned char * data) {
	
	if (gCom2RecvBufEnd == gCom2RecvBufHdr)
	{
		return 0;
	}

	int cnt = 0;
	while (gCom2RecvBufEnd != gCom2RecvBufHdr)
	{
		*(data + cnt) = *gCom2RecvBufEnd;
		gCom2RecvBufEnd++;
		if (gCom2RecvBufEnd >= gCom2RecvBuf + COM_BUFFER_SIZE)
		{
			gCom2RecvBufEnd = gCom2RecvBuf;
		}
		cnt++;
	}

	return cnt;
}



extern "C"  __declspec(naked) void __kCom1Proc(LIGHT_ENVIRONMENT * stack) {

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
		char szout[1024];
		unsigned char status = inportb(COM1PORT + 2);
		//__printf(szout, "com1 IRR:%x\r\n", status);
		if (status & 1)
		{

		}
		else{
			if ((status & 6) == 6) //	Receiver Line Status
			{			
				status = inportb(COM1PORT + 5);
				__printf(szout,(char*) "com1 Receiver Line Status interrupt\r\n");
			}
			else if ((status & 6) == 4)		//	Received Data Available
			{
				unsigned char c = inportb(COM1PORT + 5);
				if (c & 1)
				{
					*gCom1RecvBufHdr = inportb(COM1PORT);
					gCom1RecvBufHdr++;
					if (gCom1RecvBufHdr >= gCom1RecvBuf + COM_BUFFER_SIZE)
					{
						gCom1RecvBufHdr = gCom1RecvBuf;
					}
				}

				//__printf(szout, "com1 Received Data Available interrupt\r\n");
			}
			else if ((status & 6) == 2)		//Transmitter Holding Register Empty
			{
				unsigned char c = inportb(COM1PORT + 5);
				if (c & 1)
				{
					*gCom1RecvBufHdr = inportb(COM1PORT);
					gCom1RecvBufHdr++;
					if (gCom1RecvBufHdr >= gCom1RecvBuf + COM_BUFFER_SIZE)
					{
						gCom1RecvBufHdr = gCom1RecvBuf;
					}
				}
				//__printf(szout, "com1 Transmitter Holding Register Empty interrupt\r\n");
			}
			else if ((status & 6) == 0)		//Modem Status
			{
				status = inportb(COM1PORT + 6);
				__printf(szout, (char*)"com1 Modem Status interrupt\r\n");
			}
			else if ((status & 8) == 8)
			{
				status = inportb(COM1PORT + 6);
				__printf(szout, (char*)"com1 timeout interrupt\r\n");
			}
		}
		
		EOICommand(INTR_8259_MASTER + 2);
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



extern "C"  __declspec(naked) void __kCom2Proc(LIGHT_ENVIRONMENT * stack) {
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
		char szout[1024];

		unsigned char status = inportb(COM2PORT + 2);
		//__printf(szout, "com2 IRR:%x\r\n",status);
		if ((status & 1) == 0)
		{
			if ((status & 6) == 6)		//	Receiver Line Status
			{
				status = inportb(COM2PORT + 5);
				//__printf(szout, "com2 Receiver Line Status interrupt\r\n");
			}
			else if ((status & 6) == 4)		//	Received Data Available
			{
				unsigned char c = inportb(COM2PORT + 5);
				if (c & 1)
				{
					*gCom2RecvBufHdr = inportb(COM2PORT);
					gCom2RecvBufHdr++;
					if (gCom2RecvBufHdr >= gCom2RecvBuf + COM_BUFFER_SIZE)
					{
						gCom2RecvBufHdr = gCom2RecvBuf;
					}
				}
				//__printf(szout, "com2 Received Data Available interrupt\r\n");
			}
			else if ((status & 6) == 2)	//Transmitter Holding Register Empty
			{
				unsigned char c = inportb(COM2PORT + 5);
				if (c & 1)
				{
					*gCom2RecvBufHdr = inportb(COM2PORT);
					gCom2RecvBufHdr++;
					if (gCom2RecvBufHdr >= gCom2RecvBuf + COM_BUFFER_SIZE)
					{
						gCom2RecvBufHdr = gCom2RecvBuf;
					}
				}
				//__printf(szout, "com2 Transmitter Holding Register Empty interrupt\r\n");
			}
			else if ((status & 6) == 0)	//Modem Status
			{
				status = inportb(COM2PORT + 6);
				//__printf(szout, "com2 Modem Status interrupt\r\n");
			}
			else if ((status & 8) == 8) 
			{
				status = inportb(COM2PORT + 6);
				//__printf(szout, "com2 timeout interrupt\r\n");
			}
		}
		
		EOICommand(INTR_8259_MASTER + 3);

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