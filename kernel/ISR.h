#pragma once
#pragma once
#include "def.h"
#include "task.h"


#pragma pack(1)

typedef struct {

	DWORD		eip;
	DWORD		cs;
	DWORD		eflags;
}ExceptionStack0;

typedef struct {

	ExceptionStack0 stack0;
	DWORD		esp3;
	DWORD		ss3;
}ExceptionStack3;

typedef struct {

	ExceptionStack3 stack3;
	DWORD		gs;
	DWORD		fs;
	DWORD		ds;
	DWORD		es;
}ExceptionStackV86;

typedef struct {
	DWORD		errcode;
	ExceptionStack0 stack0;
}ExceptionErrorStack0;

typedef struct {
	DWORD		errcode;
	ExceptionStack3 stack3;
}ExceptionErrorStack3;

typedef struct {
	DWORD		errcode;
	ExceptionStackV86 stackv86;

}ExceptionErrorStackV86;



#pragma pack()



void  Com1IntProc(LIGHT_ENVIRONMENT* stack);
void  Com2IntProc(LIGHT_ENVIRONMENT* stack);

void  Parallel2IntProc(LIGHT_ENVIRONMENT* stack);
void  Parallel1IntProc(LIGHT_ENVIRONMENT* stack);

void  FloppyDiskIntProc(LIGHT_ENVIRONMENT* stack);
void  SlaveIntProc(LIGHT_ENVIRONMENT* stack);

void  NetcardIntProc(LIGHT_ENVIRONMENT* stack);
void  USBIntProc(LIGHT_ENVIRONMENT* stack);

void  CoprocessorIntProc(LIGHT_ENVIRONMENT* stack);
void  IDEMasterIntProc(LIGHT_ENVIRONMENT* stack);

void  IDESlaveIntProc(LIGHT_ENVIRONMENT* stack);

void Slave1IntProc(LIGHT_ENVIRONMENT* stack);

extern "C" void TimerInterrupt(LIGHT_ENVIRONMENT * stack);

extern "C" void  CmosInterrupt(LIGHT_ENVIRONMENT * stack);

void DivideError(LIGHT_ENVIRONMENT* stack);

void NmiInterrupt(LIGHT_ENVIRONMENT* stack);

void  OverflowException(LIGHT_ENVIRONMENT* stack);

void BoundRangeExceed(LIGHT_ENVIRONMENT* stack);

void UndefinedOpcode(LIGHT_ENVIRONMENT* stack);

void  DeviceNotAvailable(LIGHT_ENVIRONMENT* stack);

void  DoubleFault(LIGHT_ENVIRONMENT* stack);

void  CoprocSegOverrun(LIGHT_ENVIRONMENT* stack);

void  InvalidTss(LIGHT_ENVIRONMENT* stack);

void  SegmentUnpresent(LIGHT_ENVIRONMENT* stack);

void  StackSegFault(LIGHT_ENVIRONMENT* stack);

void  GeneralProtection(LIGHT_ENVIRONMENT* stack);

void  PageFault(LIGHT_ENVIRONMENT* stack);

void AnonymousException(LIGHT_ENVIRONMENT* stack);

void FloatPointError(LIGHT_ENVIRONMENT* stack);

void  AlignmentCheck(LIGHT_ENVIRONMENT* stack);

void  MachineCheck(LIGHT_ENVIRONMENT* stack);

void SIMDException(LIGHT_ENVIRONMENT* stack);

void VirtualizationException(LIGHT_ENVIRONMENT* stack);

void CtrlProtectException(LIGHT_ENVIRONMENT* stack);

void HypervisorInjectException(LIGHT_ENVIRONMENT* stack);
void VMMCommException(LIGHT_ENVIRONMENT* stack);
void SecurityException(LIGHT_ENVIRONMENT* stack);

void ApicSpuriousHandler(LIGHT_ENVIRONMENT* stack);