#pragma once


#pragma once

#include "process.h"
#include "def.h"

#define SYSTEM_DESCRIPTOR		0
#define STORAGE_DESCRIPTOR		1

#define LDT_DESCRIPTOR			2
#define TASKGATE_DESCRIPTOR		5
#define TSS_DESCRIPTOR			9
#define CALLGATE_DESCRIPTOR		12
#define INTGATE_DESCRIPTOR		14
#define TRAPGATE_DESCRIPTOR		15

#pragma pack(1)

typedef struct {
	WORD size;
	DWORD addr;
}DescriptTableReg;




struct SegType { //conforming code: jump from low pr ivite code to high private code without changing CPL
	union {
		struct {
			BYTE code : 1; //code or data segment
			BYTE ext_conform : 1; //expansion-direction or conforming code. 1:downward 0:upward
			BYTE r_w : 1; //code readable or data writable
			BYTE access : 1;
		}split;
		BYTE integral : 4;
	}u;
};


struct SegDescriptor {
	WORD len;
	WORD baseLow;
	BYTE baseMid;
	BYTE access : 1;
	BYTE r_w : 1; //code readable or data writable
	BYTE ext_conform : 1; //expansion-direction or conforming code. 1:downward 0:upward
	BYTE code : 1; //code or data segment
	BYTE system : 1; //1:data or code 0:system
	BYTE dpl : 2;
	BYTE present : 1;
	BYTE lenHigh : 4;
	BYTE avl : 1; //0
	BYTE unused : 1; //0
	BYTE db : 1; //operand width
	BYTE granularity : 1; //0:byte 1:4096
	BYTE baseHigh;

};


struct TssDescriptor {
	WORD len;
	WORD baseLow;
	BYTE baseMid;
	BYTE type : 4;
	BYTE system : 1; //1:data or code 0:system
	BYTE dpl : 2;
	BYTE present : 1;
	BYTE lenHigh : 4;
	BYTE avl : 1; //0
	BYTE unused : 1; //0
	BYTE db : 1; //operand width
	BYTE granularity : 1; //0:byte 1:4096
	BYTE baseHigh;

};


#define LdtDescriptor TssDescriptor



struct CallGateDescriptor {
	WORD baseLow;
	WORD selector;
	BYTE paramCnt;
	BYTE type : 4;
	BYTE system : 1; //1:data or code 0:system
	BYTE dpl : 2;
	BYTE present : 1;
	WORD baseHigh;
};


struct IntTrapGateDescriptor {
	WORD baseLow;
	WORD selector;
	BYTE unused;
	BYTE type : 4;
	BYTE system : 1; //1:data or code 0:system
	BYTE dpl : 2;
	BYTE present : 1;
	WORD baseHigh;
};



struct TaskGateDescriptor {
	WORD unused1;
	WORD selector;
	BYTE unused2;
	BYTE type : 4;
	BYTE system : 1; //1:data or code 0:system
	BYTE dp1 : 2;
	BYTE present : 1;
	WORD unused3;
};

struct IntTrapGate64Descriptor {
	struct IntTrapGateDescriptor itg32;
	QWORD baseHigh32;
};


struct CallGate64Descriptor {
	struct CallGateDescriptor cg32;
	QWORD baseHigh32;
};

#define Ldt64Descriptor Tss64Descriptor

struct Tss64Descriptor {

	struct TssDescriptor tss32; 
	
	QWORD baseHigh32;
};



#pragma pack()


void EnterLongMode();

void initGdt();

void initIDT();

void initV86Tss(TSS* tss, DWORD esp0,  DWORD ip,DWORD cs, DWORD cr3, DWORD ldt);

void initKernelTss(TSS* tss, DWORD esp0, DWORD esp, DWORD eip, DWORD cr3, DWORD ldt);

void makeDataSegDescriptor(DWORD base, int dpl, int bit, int direction, int w, SegDescriptor* descriptor);

void makeCodeSegDescriptor(DWORD base, int dpl, int bit, int conforming, int r, SegDescriptor* descriptor);

void makeTssDescriptor(DWORD base, int dpl,  int size, TssDescriptor* descriptor);

void makeLDTDescriptor(DWORD base, int dpl, int size, TssDescriptor* descriptor);

void makeTaskGateDescriptor(DWORD selector, int dpl, TaskGateDescriptor* descriptor);

void makeCallGateDescriptor(DWORD base, DWORD selector, int dpl, int paramcnt, CallGateDescriptor* descriptor);

void makeIntGateDescriptor(DWORD base, DWORD selector, int dpl, IntTrapGateDescriptor* descriptor);

void makeTrapGateDescriptor(DWORD base, DWORD selector, int dpl, IntTrapGateDescriptor* descriptor);


void makeTss64Descriptor(QWORD base, int dpl, int size, Tss64Descriptor* descriptor);

void makeLDT64Descriptor(QWORD base, int dpl, int size, Tss64Descriptor* descriptor);

void makeCallGate64Descriptor(QWORD base, DWORD selector, int dpl, int paramcnt, CallGate64Descriptor* descriptor);

void makeIntGate64Descriptor(QWORD base, DWORD selector, int dpl, IntTrapGate64Descriptor* descriptor);

void makeTrapGate64Descriptor(QWORD base, DWORD selector, int dpl, IntTrapGate64Descriptor* descriptor);

void initTss64(TSS64_DATA* tss, QWORD rsp);

#ifdef DLL_EXPORT
extern "C" __declspec(dllexport) void initEfer();

#else
extern "C" __declspec(dllimport) void initEfer();

#endif



