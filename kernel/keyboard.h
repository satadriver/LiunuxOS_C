
#include "def.h"
#ifndef KEYBOARD_H_H_H
#define KEYBOARD_H_H_H

#define KEYBORAD_BUF_SIZE		1024

#define SHIFTLEFT_SET_FLAG 		1
#define SHIFTRIGHT_SET_FLAG 	2
#define CTRLLEFT_SET_FLAG 		4
#define CTRLRIGHT_SET_FLAG 		8
#define ALTLEFT_SET_FLAG		0x10
#define ALTRIGHT_SET_FLAG		0x20
#define SCROLLLOCK_SET_FLAG 	0x40
#define NUMSLOCK_SET_FLAG		0x80
#define CAPSLOCK_SET_FLAG 		0x100
#define INSERT_SET_FLAT			0x200


#define BC_BACK				8
#define VK_BACK				0x08   //Backspace ¼ü
#define BC_TAB				8
#define VK_TAB				0x09   //Tab ¼ü
#define BC_ESCAPE			1
#define VK_ESCAPE			0X1B   //Tab ¼ü

#define BC_F1 0X3B
#define VK_F1 112

#define BC_F2 0X3C
#define VK_F2 113

#define BC_F3 0X3D
#define VK_F3 114

#define BC_F4 0X3E
#define VK_F4 115

#define BC_F5 0X3F
#define VK_F5 116

#define BC_F6 0X40
#define VK_F6 117

#define BC_F7 0X41
#define VK_F7 118

#define BC_F8 0X42
#define VK_F8 119

#define BC_F9 0X43
#define VK_F9 120

#define BC_F10 0X44
#define VK_F10 121

#define BC_F11 0X57
#define VK_F11 122

#define BC_F12 0X58
#define VK_F12 123

#define BC_INSERT 0X52
#define VK_INSERT 45

#define BC_HOME 0X47
#define VK_HOME 36

#define BC_PRIOR 0X49
#define VK_PRIOR 33

#define BC_NEXT 0X51
#define VK_NEXT 34

#define BC_END 0X4F
#define VK_END 35

#define BC_DELETE 0X53
#define VK_DELETE 46

#define BC_LEFT 0X4B
#define VK_LEFT 37

#define BC_UP 0X48
#define VK_UP 38

#define BC_RIGHT 0X4D
#define VK_RIGHT 39

#define BC_DOWN 0X50
#define VK_DOWN 40

#define BC_CAPSLOCK 0X3A
#define VK_CAPSLOCK 20

#define BC_NUMSLOCK 0X45
#define VK_NUMSLOCK 144

#define BC_PAUSE_BREAK 0X37
#define VK_PAUSE_BREAK 19

#define BC_SCROLLLOCK 0X46
#define VK_SCROLLLOCK 145

#define BC_LSHIFT			0X2A
#define VK_LSHIFT			0xA0
#define BC_RSHIFT			0X36
#define VK_RSHIFT			0xA1

#define BC_CONTROL			0X1D
#define VK_CONTROL			17

#define VK_LCONTROL			0xA2
#define VK_RCONTROL			0xA3
#define VK_LMENU			0xA4
#define VK_RMENU			0xA5

#define VK_PRINT			0X2A

#define BC_MENU 0X38
#define VK_MENU 18

#define BC_APPS 0X46
#define VK_APPS 93

#define BC_RWIN 0X46
#define VK_RWIN 92

#define BC_LWIN 0X46
#define VK_LWIN 91



#pragma pack(1)

typedef struct {

	unsigned int kbdBuf[KEYBORAD_BUF_SIZE];
	unsigned int kbdStatusBuf[KEYBORAD_BUF_SIZE];
	unsigned int kbdBufHdr;
	unsigned int kbdBufTail;
	unsigned int kbdStatus;
	unsigned int kbdLedStatus;
}KBDBUFDATA,*LPKBDBUFDATA;

#pragma pack()






void kbdtest();

extern "C" void KeyboardIntProc();

void __kKbdLed(unsigned char cmd);

int numsLockProc(unsigned int c);

int isScancodeAsc(unsigned char c);

#ifdef DLL_EXPORT

extern "C" __declspec(dllexport)WORD gKeyboardID;

extern "C"  __declspec(dllexport) DWORD gKbdTest;

extern "C"  __declspec(dllexport) unsigned int __kGetKbd(int wid);

extern "C"  __declspec(dllexport) unsigned int __getchar(int id);

extern "C"  __declspec(dllexport) int __putchar(char * s);

extern "C"  __declspec(dllexport) void __kKeyboardProc();

extern "C"  __declspec(dllexport) void __kPutKbd(unsigned char s,int wid);
#else

extern "C" __declspec(dllimport) WORD gKeyboardID;

extern "C"  __declspec(dllimport) DWORD gKbdTest;
extern "C"  __declspec(dllimport) unsigned int __kGetKbd(int wid);

extern "C"  __declspec(dllimport) unsigned int __getchar(int id);

extern "C"  __declspec(dllimport) int __putchar(char * s);

extern "C"  __declspec(dllimport) void __kKeyboardProc();

extern "C"  __declspec(dllimport) void __kPutKbd(unsigned char s,int wid);
#endif

#endif