

#include "apic.h"
#include "hardware.h"
#include "Utils.h"
#include "descriptor.h"

#include "task.h"
#include "cmosAlarm.h"
#include "cmosExactTimer.h"
#include "cmosPeriodTimer.h"
#include "core.h"
#include "device.h"
#include "coprocessor.h"
#include "debugger.h"
#include "Pe.h"
#include "peVirtual.h"
#include "Thread.h"

int gAllocateAp = 0;

DWORD * gOicBase = 0;

char * gRcbaBase = 0;

DWORD* gHpetBase = 0;

int g_lvt_timer = 0;

unsigned long g_allocate_ap_lock = 0;

int g_ipi_lock = 0;

int g_apic_int_tag = 0;

LPPROCESS_INFO g_ap_tss_base[256];



void enableRcba() {
	outportd(0xcf8, 0x8000f8f0);

	gRcbaBase = (char*)(inportd(0xcfc) & 0xffffc000);
	*gRcbaBase = *gRcbaBase | 1;
}

//bit 9:enable irq 13
//bit 8:enable apic io
void EnableInt13() {
	outportd(0xcf8, 0x8000f8f0);

	gRcbaBase = (char*)(inportd(0xcfc) & 0xffffc000);

	gOicBase = (DWORD*)((DWORD)gRcbaBase + 0x31fe);

	DWORD v = *gOicBase;

	v = (v & 0xffffff00) | 0x200;

	*gOicBase = v;

}

unsigned long ReadIoApicReg(int reg) {

	*(DWORD*)(IO_APIC_BASE) = reg ;
	__asm {
		mfence
	}
	return *(DWORD*)(IO_APIC_BASE+0x10);
}


unsigned long WriteIoApicReg(int reg, unsigned long value) {
	*(DWORD*)(IO_APIC_BASE) = reg ;
	__asm {
		mfence
	}
	*(DWORD*)(IO_APIC_BASE + 0x10) = value;

	return 0;
}



void iomfence() {
	__asm {
		mfence
	}
}

void setIoRedirect(int idx, int id, int vector, int mode) {
	WriteIoApicReg(idx, vector + (mode << 8));
	iomfence();
	WriteIoApicReg(idx + 1, ((id & 0x0ff) << 24));
	


}


unsigned long long GetIoRedirect(int idx) {

	unsigned long long high = ReadIoApicReg(idx + 1);

	unsigned long long low = ReadIoApicReg(idx);

	unsigned long long v = (high << 32) | low;
	return v;
}



void setIoApicID(int id) {

	WriteIoApicReg(0, (id & 0x0ff) << 24);

}


void WaitIcrFree() {
	unsigned int value = 0;
	do {
		value = *(DWORD*)(LOCAL_APIC_BASE + 0x300);
	} while (value & 0x1000);
}


void enableIoApic() {

	outportd(0xcf8, 0x8000f8f0);

	gRcbaBase = (char*)(inportd(0xcfc) & 0xffffc000);

	gOicBase = (DWORD*)((DWORD)gRcbaBase + 0x31fe);

	DWORD v = *gOicBase;
	
	v = (v & 0xffffff00) | 0x100;

	*gOicBase = v;

	char szout[256];

	__printf(szout,"oic base:%x,value:%x,rcba base:%x\r\n", gOicBase, v,gRcbaBase);
}



DWORD* getOicBase() {
	outportd(0xcf8, 0x8000f8f0);
	DWORD rcba = inportd(0xcfc) & 0xffffc000;

	gOicBase = (DWORD*)(rcba + 0x31fe);

	return gOicBase;
}

char* getRcbaBase() {
	outportd(0xcf8, 0x8000f8f0);
	DWORD base = inportd(0xcfc) & 0xffffc000;

	gRcbaBase = (char*)base;

	return gRcbaBase;
}



int enableHpet() {
	outportd(0xcf8, 0x8000f8f0);

	DWORD addr = inportd(0xcfc) & 0xffffc000;

	gHpetBase = (DWORD*)(addr + 0x3404);

	DWORD v = *gHpetBase;

	v = (v | 0x80) & 0xfffffffc;

	*gHpetBase = v;

	char szout[256];
	__printf(szout, "hpet base:%x,value:%x\r\n", gHpetBase, v);

	return 0;
}

//enable io apic
void enableIMCR() {
	outportb(0x22, 0x70);
	outportb(0x23, 0x01);
}


int getLVTCount(int n) {
	int cnt =(int) *(long long*)(LOCAL_APIC_BASE + 0x30) >>16;
	return cnt;
}

int getLocalApicVersion() {
	int ver = *(long long*)(LOCAL_APIC_BASE + 0x30) & 0xff;
	return ver;
}


//cpuid.01h:ebx[31:24]
int getLocalApicID() {
	__asm {
		mov eax,1
		mov ecx,0
		cpuid

		test ebx,0x200000
		jnz _x2apic

		shr ebx,24
		movzx eax,bl
		ret

		_x2apic:
		mov eax,0bh
		mov ecx, 0
		cpuid
		mov eax,edx
	}
}








int initHpet() {
	int res = 0;

	//enableHpet();

	char szout[256];
	
	long long idc = *(long long*)APIC_HPET_BASE;

	HPET_GCAP_ID_REG * hg = (HPET_GCAP_ID_REG*)&idc;
	//the default value is 0x0429b17f=69841279 about 14.318179MHz
	__printf(szout, "hpet version:%d,count:%d,width:%d,compatable:%d,vender:%x,tick:%x\r\n ", 
		hg->version, hg->count, hg->width, hg->compatable,hg->venderid,hg->tick);
	
	int cnt = hg->count;

	DWORD tick = hg->tick;

	unsigned long long ns = 1000000000000000;

	long long tc =( ns / hg->tick)/100;		// 10ms, 14318179 = 1000ms,0x0429b17f=69841279

	*(long long*)(APIC_HPET_BASE + 0x10) = 0;

	//General Interrupt Status Register
	*(long long*)(APIC_HPET_BASE + 0x20) = 0;
		
	//timer0
	unsigned long long* regs = (unsigned long long*)(APIC_HPET_BASE + 0x100);

	//bit3: writing 1 to this field enables periodic timer and writing 0 enables non - periodic mode.O
	//bit2:Setting this bit to 1 enables triggering of interrupts. Even if this bit is 0, this timer will still set Tn_INT_STS.
	regs[0] = 0x40 + 8 + 4 + 2;
	//Timer N Comparator Value Register
	//compare with main counter to check if an interrupt should be generated.
	regs[1] = tc;
	//regs[2] = 0;

	regs[4] = 4 + 2;
	regs[5] = tc;
	//regs[6] = 0;

	//regs[8] = 0x4;
	//regs[9] = tc;
	//regs[10] = 0;

	//Main Counter Value Register,increase in each interruption
	//Writes to this register should only be done when the counter is halted (ENABLE_CNF = 0
	*(long long*)(APIC_HPET_BASE + 0xf0) = 0;

	//Tn_INT_STS
	//bit0:ENABLE_CNF，0:main counter is halted, timer interrupts are disabled
	//bit1:  "legacy replacement" mapping is enabled。timer -> irq0  cmos ->irq8
	*(long long*)(APIC_HPET_BASE + 0x10) = 3;

	return FALSE;
}



void IoApicRedirect(int pin, int cpu, int vector, int mode) {

	int cpuid = 0;
	if (cpu == -1) {
		//__enterSpinlock(&g_allocate_ap_lock);
		int total = *(int*)(CPU_TOTAL_ADDRESS);
		int* ids = (int*)CPU_ID_ADDRESS;
		cpuid = ids[gAllocateAp];
		gAllocateAp++;
		if (gAllocateAp >= total) {
			gAllocateAp = 0;
		}
		//__leaveSpinlock(&g_allocate_ap_lock);
	}
	else {
		cpuid = cpu;
	}

	unsigned long long oldValue = GetIoRedirect(pin);

	char szout[256];
	__printf(szout, "io apic %d value %I64x\r\n ",pin, oldValue);

	setIoRedirect(pin, cpuid, vector, mode);
}


void SetIcr(int cpu,int vector,int mode,int destType) {

	WaitIcrFree();

	if (destType == 0) {
		unsigned int id = cpu << 24;
		*(DWORD*)(LOCAL_APIC_BASE + 0x310) = id;	
		iomfence();
	}

	unsigned int v = vector;
	v = v | 0x4000;

	if (mode == 0) {
		
	}
	else {
		v = ((mode & 7) << 8) | v;
	}

	if (destType) {
		v = v | ((destType & 3) << 18);
	}

	*(DWORD*)(LOCAL_APIC_BASE + 0x300) = v;

	v = *(DWORD*)(LOCAL_APIC_BASE + 0x300);
	char szout[256];
	__printf(szout, "%s cpu:%x result:%x\r\n", __FUNCTION__, cpu, v);

	return;

}

int InitIoApicRte() {
	
	int bsp_id = *(int*)(CPU_ID_ADDRESS);

	IoApicRedirect(0x10, bsp_id, 0x10000, 0);

	IoApicRedirect(0x12, bsp_id, INTR_8259_MASTER + 1, 0);

	//IoApicRedirect(0x14, bsp_id, APIC_HPETTIMER_VECTOR, 0);

	IoApicRedirect(0x14, bsp_id, APIC_LVTTIMER_VECTOR, 0);

	IoApicRedirect(0x16, bsp_id, INTR_8259_MASTER + 3, 0);
	IoApicRedirect(0x18, bsp_id, INTR_8259_MASTER + 4, 0);
	IoApicRedirect(0x1a, bsp_id, INTR_8259_MASTER + 5, 0);
	IoApicRedirect(0x1c, bsp_id, INTR_8259_MASTER + 6, 0);
	IoApicRedirect(0x1e, bsp_id, INTR_8259_MASTER + 7, 0);

	//IoApicRedirect(0x20, bsp_id, APIC_LVTTIMER_VECTOR, 0);
	//IoApicRedirect(0x20, bsp_id, APIC_HPETTIMER_VECTOR, 0);

	IoApicRedirect(0x22, bsp_id, INTR_8259_SLAVE + 1, 0);
	IoApicRedirect(0x24, bsp_id, INTR_8259_SLAVE + 2, 0);
	IoApicRedirect(0x26, bsp_id, INTR_8259_SLAVE + 3, 0);
	IoApicRedirect(0x28, bsp_id, INTR_8259_SLAVE + 4, 0);
	IoApicRedirect(0x2a, bsp_id, INTR_8259_SLAVE + 5, 0);
	IoApicRedirect(0x2c, bsp_id, INTR_8259_SLAVE + 6, 0);
	IoApicRedirect(0x2e, bsp_id, INTR_8259_SLAVE + 7, 0);

	return 0;
}





int IpiCreateThread(char* addr,  char* module, unsigned long p, char* funname)
{
	__enterSpinlock(&g_allocate_ap_lock);
	int ret = 0;
	int id = GetIdleProcessor();
	IPI_MSG_PARAM* msg = (IPI_MSG_PARAM*)IPI_MSG_BASE;

	msg[id].cmd = IPI_CREATETHREAD;

	msg[id].pc = 4;
	IPI_CREATETHREAD_PARAM* subparam = (IPI_CREATETHREAD_PARAM*)msg[id].param;
	subparam->addr = (DWORD)addr;
	subparam->module = module;

	__strcpy(subparam->funcname, funname);

	subparam->params = (DWORD)p;
	char szout[256];
	__printf(szout, "%s cpu:%d module:%x function:%s\r\n", __FUNCTION__, id, subparam->module, subparam->funcname);
	
	__leaveSpinlock(&g_allocate_ap_lock);

	SetIcr(id, APIC_IPI_VECTOR, 0, 0);

	return 0;
}

int IpiCreateProcess(DWORD base, int size, char* module, char* func, int level, unsigned long p)
{
	__enterSpinlock(&g_allocate_ap_lock);
	int ret = 0;
	int id = GetIdleProcessor();
	IPI_MSG_PARAM* msg = (IPI_MSG_PARAM*)IPI_MSG_BASE;

	msg[id].cmd = IPI_CREATEPROCESS;

	msg[id].pc = 6;
	IPI_CREATEPROCESS_PARAM* subparam = (IPI_CREATEPROCESS_PARAM*)msg[id].param;
	subparam->base = (DWORD)base;
	subparam->size = size;

	__strcpy((char*)subparam->module, module);

	__strcpy(subparam->funcname, func);
	subparam->level = level;
	subparam->params = (DWORD)p;
	char szout[256];
	__printf(szout, "%s cpu:%d base:%x size:%x module:%s addr:%p function:%s addr:%p level:%d param:%x\r\n", __FUNCTION__,
		id, subparam->base, subparam->size, subparam->module, &subparam->module, subparam->funcname, &subparam->funcname, subparam->level, subparam->params);

	//SetIcr(0, APIC_IPI_VECTOR, 0, 3);
	__leaveSpinlock(&g_allocate_ap_lock);
	SetIcr(id, APIC_IPI_VECTOR, 0, 0);

	return 0;
}
extern "C" void __declspec(naked) IPIIntHandler(LIGHT_ENVIRONMENT * stack) {
	__asm {
		cli
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
		mov ss,ax	
	}

	{
		char szout[256];

		__enterSpinlock(&g_allocate_ap_lock);

		IPI_MSG_PARAM* msg =(IPI_MSG_PARAM * )IPI_MSG_BASE;
		int id = *(int*)(LOCAL_APIC_BASE + 0x20) >> 24;
		int cmd = msg[id].cmd;
		
		//__printf(szout,"cpu:%d %s %d cmd:%d\r\n",id, __FUNCTION__,__LINE__,cmd);

		if (cmd == IPI_CREATEPROCESS) {
			msg[id].cmd = 0;

			IPI_CREATEPROCESS_PARAM* subparam = (IPI_CREATEPROCESS_PARAM*)msg[id].param;
			DWORD base = (DWORD)subparam->base;
			DWORD size = subparam->size;
			char *  module =subparam->module;
			char* funcname = subparam->funcname;
			int level = subparam->level;
			char* p = (char*)subparam->params;
			
			__printf(szout, "%s module:%s addr:%p function:%s addr:%p\r\n", __FUNCTION__, module,&subparam->module, funcname,&subparam->funcname);

			if (__findProcessFileName(subparam->funcname) == FALSE)
			{
				//__kCreateProcess(base, size, (subparam->module), (subparam->funcname), level, (unsigned long)p);
				//__kCreateProcess(MAIN_DLL_SOURCE_BASE, 0x100000, (char*)"main.dll", (char*)"__kConsole", 3, 0);
				__kCreateProcess(subparam->base, subparam->size, (subparam->module), (subparam->funcname), subparam->level, (unsigned long)subparam->params);
			}
		}
		else if (cmd == IPI_CREATETHREAD) {
			msg[id].cmd = 0;

			IPI_CREATETHREAD_PARAM* subparam = (IPI_CREATETHREAD_PARAM*)msg[id].param;
			DWORD module = (DWORD)subparam->module;
			DWORD addr = subparam->addr;
			char* funcname = subparam->funcname;
			char* p = (char*)subparam->params;

			__printf(szout, "%s module:%s function:%s\r\n", __FUNCTION__, module, funcname);

			if (__findProcessFileName(funcname) == FALSE)
			{
				__kCreateThread((DWORD)addr, (DWORD)subparam->module,(unsigned long)p ,subparam->funcname );
			}
		}
		else {

		}

		__leaveSpinlock(&g_allocate_ap_lock);
		
		g_ipi_lock++;
		*(DWORD*)(LOCAL_APIC_BASE + 0xb0) = 0;	
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



extern "C" void __declspec(naked) LVTTimerIntHandler(LIGHT_ENVIRONMENT* stack) {
	__asm {
		cli

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
		MOV ss, AX
	}

	{
		char szout[256];
		//__printf(szout, "LVTTimerIntHandlers esp local value:%x\r\n",szout);
		//__printf(szout, "entry %s\r\n", __FUNCTION__);

		g_lvt_timer++;
		
		//__kTaskSchedule((LIGHT_ENVIRONMENT*)stack);
		LPPROCESS_INFO next = SingleTssSchedule(stack);
		//*(DWORD*)(LOCAL_APIC_BASE + 390) = 0;

		//__enterSpinlock(&g_ap_work_lock);
	
		//__leaveSpinlock(&g_ap_work_lock);

		*(DWORD*)(LOCAL_APIC_BASE + 0xb0) = 0;
		if (g_apic_int_tag) {
			*(DWORD*)(IO_APIC_BASE + 0x40) = 0;
		}
	}

	__asm {
#ifdef SINGLE_TASK_TSS
		call GetCurrentTaskTssBase
		mov edx, eax
		mov eax, dword ptr ds : [edx + PROCESS_INFO.tss.cr3]
		mov cr3, eax
#endif
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

#ifdef SINGLE_TASK_TSS
		mov esp, dword ptr ss : [esp - 20]
#endif	

		iretd

		jmp LVTTimerIntHandler
	}
}



extern "C" void __declspec(naked) LVTTemperatureIntHandler(LIGHT_ENVIRONMENT* stack) {
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
		MOV ss, AX
	}

	{
		*(DWORD*)(LOCAL_APIC_BASE + 0xb0) = 0;

		char szout[256];
		__printf(szout, "%s\r\n", __FUNCTION__);
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

		clts
		iretd
	}
}



extern "C" void __declspec(naked) LVTErrorIntHandler(LIGHT_ENVIRONMENT* stack) {
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
		MOV ss, AX
	}

	{
		*(DWORD*)(LOCAL_APIC_BASE + 0xb0) = 0;

		*(DWORD*)(LOCAL_APIC_BASE + 0x370) = 0;
		DWORD error = *(DWORD*)(LOCAL_APIC_BASE + 0x370);

		char szout[256];
		__printf(szout, "%s error:%x\r\n", __FUNCTION__,error);
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

		clts
		iretd
	}
}


extern "C" void __declspec(naked) LVTPerformanceIntHandler(LIGHT_ENVIRONMENT* stack) {
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
		MOV ss, AX
	}

	{
		*(DWORD*)(LOCAL_APIC_BASE + 0xb0) = 0;

		char szout[256];
		__printf(szout, "%s\r\n", __FUNCTION__);
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

		clts
		iretd
	}
}



extern "C" void __declspec(naked) LVTCMCIHandler(LIGHT_ENVIRONMENT* stack) {
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
		MOV ss, AX
	}

	{
		*(DWORD*)(LOCAL_APIC_BASE + 0xb0) = 0;

		char szout[256];
		__printf(szout, "%s\r\n", __FUNCTION__);
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

		clts
		iretd
	}
}

//https://blog.csdn.net/weixin_46645613/article/details/120406002
//https://zhuanlan.zhihu.com/p/406213995
//https://zhuanlan.zhihu.com/p/678582090
//https://www.zhihu.com/question/594531181/answer/2982337869








int g_cmos_timer = 0;


extern "C" void __declspec(naked) HpetTimer0Handler(LIGHT_ENVIRONMENT * stack) {
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
		MOV ss, AX
		//cli
	}

	{
		char szout[256];
		//__printf(szout, "HpetInterrupt\r\n");

		//LPPROCESS_INFO process = (LPPROCESS_INFO)GetCurrentTaskTssBase();

		int value = *(int*)(APIC_HPET_BASE + 0x20);
		//if (value & 1)
		{
			__kTaskSchedule((LIGHT_ENVIRONMENT*)stack);
			//*(long*)(APIC_HPET_BASE + 0x20) = value & 0xfffffffe;
			//*(unsigned long*)(APIC_HPET_BASE + 0xf0) = 0;
			
			//*(unsigned long*)(APIC_HPET_BASE + 0xf4) = 0;

			g_cmos_timer++;
			if (g_cmos_timer == (1000 / TASK_TIME_SLICE)) {
				g_cmos_timer = 0;
				
				//__kPeriodTimer();
			}
		}

		/*
		if (value & 2) {
			*(long*)(APIC_HPET_BASE + 0x20) = value & 0xfffffffd;
			*(unsigned long*)(APIC_HPET_BASE + 0xf0) = 0;

			*(unsigned long*)(APIC_HPET_BASE + 0xf4) = 0;

			g_cmos_timer++;
			if (g_cmos_timer == (1000 / TASK_TIME_SLICE)) {
				g_cmos_timer = 0;

			}

			
			long long idc = *(long long*)APIC_HPET_BASE;

			HPET_GCAP_ID_REG* hg = (HPET_GCAP_ID_REG*)&idc;
			unsigned long long ns = 1000000000000000;

			long long tc = (ns / hg->tick) / 100;	
			unsigned long long* regs = (unsigned long long*)(APIC_HPET_BASE + 0x100);
			*(long long*)(APIC_HPET_BASE + 0x10) = 0;
			regs[4] = 4 + 2;
			regs[5] = tc;
			*(long long*)(APIC_HPET_BASE + 0x10) = 3;
		}
		*/
		*(DWORD*)(LOCAL_APIC_BASE + 0xB0) = 0;
		*(DWORD*)(IO_APIC_BASE + 0x40) = 0;

		DWORD base = APIC_HPET_BASE;
		unsigned __int64* gintr_sta = (unsigned __int64*)(base + 0x20);
		*gintr_sta = 0xff;
	}

	__asm {
#ifdef SINGLE_TASK_TSS
		call GetCurrentTaskTssBase
		mov edx, eax
		mov eax, dword ptr ds : [edx + PROCESS_INFO.tss.cr3]
		mov cr3, eax
#endif

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
#ifdef SINGLE_TASK_TSS
		mov esp, dword ptr ss : [esp - 20]
#endif	

		//clts
		iretd

		jmp HpetTimer0Handler
	}
}

int AllocateApTask(int intnum) {

	if(intnum < 0 || intnum > 255) {
		return -1;
	}

	int res = -1;
	int cpuId = 0;

	int total = *(int*)(CPU_TOTAL_ADDRESS);
	if (total > 0) {

		int idx = gAllocateAp;
		int* ids = (int*)CPU_ID_ADDRESS;
		int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
		
		do {
			cpuId = ids[gAllocateAp];
			if (id != cpuId) {

				SetIcr(cpuId, intnum,0,0);

				res = gAllocateAp;		
			}

			gAllocateAp++;
			if (gAllocateAp >= total) {
				gAllocateAp = 0;
			}
			if (res >= 0) {
				break;
			}
		} while (gAllocateAp != idx);
	}

	char szout[256];
	unsigned long v = *(DWORD*)(LOCAL_APIC_BASE + 0x300);
	__printf(szout, "%s index:%x,id:%x result:%x\r\n",__FUNCTION__, res, cpuId,v);
	return res;
}






int ActiveApTask(int intnum) {
#ifndef IPI_TASK_SWITCH
	return 0;
#endif
	if (intnum < 0 || intnum > 255) {
		return -1;
	}

	//__enterSpinlock(&g_allocate_ap_lock);

	int total = *(int*)(CPU_TOTAL_ADDRESS);
	if (total > 0) {
		
		SetIcr(0, intnum, 0, 3);
		/*
		int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
		int* ids = (int*)AP_ID_ADDRESS;
		for (int idx = 0; idx < total; idx++)
		{
			int cpuId = ids[idx];
			if (id != cpuId) {

				SetIcr(cpuId, intnum,0,0);
			}	
		}
		*/
	}

	//__leaveSpinlock(&g_allocate_ap_lock);

	//char szout[256];
	//__printf(szout, "AllocateApTask index:%x,id:%x\r\n", res, cpuId);
	return 0;
}


int IsApicSupported() {

	int res = 0;

	__asm {
		mov eax, 1
		cpuid
		test edx, (1 << 9)
		jz __no_apic
		mov[res], 1

		__no_apic:
	}
	return res;
}

int enableLocalApic() {

	unsigned long origin = 0;

	DWORD low = 0;
	DWORD high = 0;
	readmsr(0x1b,&low,&high);
	origin = low;

	high = 0;
	low = LOCAL_APIC_BASE|0x800;
	if (origin & 0x100) {
		low |= 0x100;
	}
	writemsr(0x1b, low, high);

	*(DWORD*)(LOCAL_APIC_BASE + 0xf0) = 0x100 | APIC_SPURIOUS_VECTOR;

	readmsr(0x1b, &low, &high);
	char szout[256];
	__printf(szout, "local apic value:%x,origin value:%x\r\n", low, origin);
	return 0;
}


void DisableInt() {
	__asm {
		mov al, 0xFF
		out 0xA1, al
		out 0x21, al
	}
	enableIMCR();
}



//https://blog.csdn.net/weixin_46645613/article/details/120406002
//https://zhuanlan.zhihu.com/p/406213995
//https://zhuanlan.zhihu.com/p/678582090
//https://www.zhihu.com/question/594531181/answer/2982337869


// Local APIC within processor而 IO APIC within chipset(一般都在南桥里). 
//LINT0 and LINT1是local APIC的两个 input pins. 一般都和 INTR & NMI shared共享.
//即 pin name is: LINT0/INTR & LINT1/NMI. 
//当 IO APIC被设成 "bypass mode"(即不靠IO APIC传递 interrupts),则 8259的 INTR会接到 local APIC的 LINT0；
//而 chipset内的 NMI logic 也会接到 local APIC的 LINT1.
//ExtInt代表接受cpu直接接受8259中断信号，而不是io apic

int DisableLocalApicLVT() {

	unsigned long v = 0;
	v = APIC_LVTTEMPERATURE_VECTOR;
	*(DWORD*)(LOCAL_APIC_BASE + 0x330) = v | 0x10000;

	v = APIC_LVTPERFORMANCE_VECTOR;
	*(DWORD*)(LOCAL_APIC_BASE + 0x340) = v | 0x10000;

	v = APIC_LVTERROR_VECTOR;
	*(DWORD*)(LOCAL_APIC_BASE + 0x370) = v | 0x10000;

	v = APIC_LVTCMCI_VECTOR;
	*(DWORD*)(LOCAL_APIC_BASE + 0x2f0) = v | 0x10000;

	return 0;

}


int InitLocalApicCmci() {
	int v = APIC_LVTCMCI_VECTOR;
	*(DWORD*)(LOCAL_APIC_BASE + 0x2f0) = v;
	return 0;
}


int InitLocalApicErr() {
	int v = APIC_LVTERROR_VECTOR;
	*(DWORD*)(LOCAL_APIC_BASE + 0x370) = v;
	return 0;
}

int InitLocalApicTimer() {

	int v = 0;

	v = 0x10000;
	*(DWORD*)(LOCAL_APIC_BASE + 0x320) = v;

	iomfence();

	v = 0x03;
	*(DWORD*)(LOCAL_APIC_BASE + 0x3E0) = v;

	iomfence();

	v = APIC_LVTTIMER_VECTOR | 0x20000 ;
	*(DWORD*)(LOCAL_APIC_BASE + 0x320) = v;

	iomfence();

	unsigned long long lv = 100000000;
	lv = lv /16 / (1000 / TASK_TIME_SLICE);
	*(DWORD*)(LOCAL_APIC_BASE + 0x380) = (DWORD)lv;

	DWORD cnt1 = *(DWORD*)(LOCAL_APIC_BASE + 0x390);

	DWORD cnt2 = *(DWORD*)(LOCAL_APIC_BASE + 0x390);

	char szout[256];
	__printf(szout, "cnt1:%x cnt2:%x\r\n", cnt1,cnt2);

	return 0;
}


extern "C" void __declspec(dllexport) __kApInitProc() {
	char* reg_esp = 0;
	int ret = 0;
	__asm {
		//cli
		mov ds:[reg_esp],esp
	}

	char szout[256];

	int lint0 = *(DWORD*)(LOCAL_APIC_BASE + 0x350);
	int lint1 = *(DWORD*)(LOCAL_APIC_BASE + 0x360);

	enableLocalApic();

	*(DWORD*)(LOCAL_APIC_BASE + 0x80) = 0;
	*(DWORD*)(LOCAL_APIC_BASE + 0x350) = 0x10000;
	*(DWORD*)(LOCAL_APIC_BASE + 0x360) = 0x10000;

	int localapic_ver = *(DWORD*)(LOCAL_APIC_BASE + 0x30);

	unsigned int cpuid = *(DWORD*)(LOCAL_APIC_BASE + 0x20)>>24;

	//__enterLock(&g_allocate_ap_lock);
	__enterSpinlock(&g_allocate_ap_lock);

	//__printf(szout, "cpu:%d enter lock\r\n", cpuid);

	int seq = *(int*)CPU_TOTAL_ADDRESS;
	int* apids = (int*)CPU_ID_ADDRESS;
	apids[seq] = cpuid;
	*(int*)(CPU_TOTAL_ADDRESS) = seq + 1;

	int ioapic_id = ReadIoApicReg(0) >> 24;

	int ioapic_ver = ReadIoApicReg(1) & 0xff;

	SetTaskTssBase();
	InitTaskArray();
	TASKRESULT freeTss;
	__getFreeTask(&freeTss, cpuid, 0);
	int tid = freeTss.number;
	char * lpgdt = InitGdt();
	unsigned long stacktop = (unsigned long)(AP_KSTACK_BASE + KTASK_STACK_SIZE * (cpuid + 1) - STACK_TOP_DUMMY);
	unsigned long stack0top = (unsigned long)(TASKS_STACK0_BASE + TASK_STACK0_SIZE * (TASK_LIMIT_TOTAL * cpuid + 0 + 1) - STACK_TOP_DUMMY);
	char procname[64];
	__sprintf(procname, "apic_process_%d", cpuid);

	//__printf(szout, "pid:%d cpu:%s\r\n", freeTss.number, procname);
	LPPROCESS_INFO process = GetCurrentTaskTssBase();
	process->tss.cr3 = PDE_ENTRY_VALUE;
	__strcpy(process->filename, (char*)procname);
	__strcpy(process->funcname, (char*)procname);	
	process->cpuid = cpuid;
	process->tid = tid;
	process->pid = tid;
	process->espbase = (unsigned long)stacktop;
	process->level = 0;
	process->counter = 0;
	process->vaddr = 0;
	process->vasize = 0;
	process->moduleaddr = (DWORD)0;
	process->videoBase = (char*)gGraphBase;
	process->showX = 0;
	process->showY = GRAPHCHAR_HEIGHT * cpuid * 16+256;
	process->window = 0;
	process->sleep = 0;
	process->copyMap = 0;
	process->status = TASK_RUN;

	__memcpy((char*)freeTss.lptss, (char*)process, sizeof(PROCESS_INFO));

	char * lpidt = InitIDT();

	__asm {
		mov eax, PDE_ENTRY_VALUE
		mov cr3, eax

		mov eax, cr0
		or eax, 0x80000000
		mov cr0, eax

		//mov ax, KERNEL_MODE_CODE
		//mov ds,ax
		//mov es,ax
		//mov fs,ax
		//mov gs,ax
		//mov ss,ax
	}

	short tr_new;
	DescriptTableReg idtbase_new;
	DescriptTableReg gdtbase_new;
	__asm {
		sgdt gdtbase_new
		sidt idtbase_new
		str ax
		mov tr_new, ax
	}
	unsigned long long tssdesc = *(unsigned long long*)(GDT_BASE + cpuid * 0x10000 + kTssTaskSelector );
	__printf(szout, "ap id:%d process:%x tss:%i64x idt:%x size:%x gdt:%x size:%d ltr:%x\r\n",
		cpuid,process, tssdesc, idtbase_new.addr, idtbase_new.size,gdtbase_new.addr, gdtbase_new.size, tr_new);

	initCoprocessor();
	enableVME();
	enablePCE();
	enableMCE();
	enableTSD();

	initDebugger();
	EnableSyscall();
	sysEntryInit((DWORD)sysEntry);

	//InitLocalApicErr();

	//InitLocalApicCmci();
#ifdef IO_APIC_ENABLE
	__asm {cli}

	//ret = DisableLocalApicLVT();

	__asm {sti}
#endif

	//*(DWORD*)(LOCAL_APIC_BASE + 0x350) = 0x700;
	
	__printf(szout, "ap id:%d version:%x init complete.esp:%x esp:%x esp0:%x lint0:%x lint1:%x tid:%d io apic id:%x version:%x\r\n", 
		cpuid, localapic_ver, reg_esp,stacktop,stack0top,lint0,lint1,tid,ioapic_id, ioapic_ver);

	__leaveSpinlock(&g_allocate_ap_lock);
	//__leaveLock(&g_allocate_ap_lock);

	//__asm{int APIC_IPI_VECTOR}
#ifdef TASK_SWITCH_ARRAY

#else
	InitTaskList();
	InsertTaskList_First(0);
#endif

#ifndef IPI_TASK_SWITCH
	//ret = InitLocalApicTimer();
#endif

	__asm {sti}

	int imagesize = getSizeOfImage((char*)KERNEL_DLL_BASE);
	DWORD kernelMain = getAddrFromName(KERNEL_DLL_BASE, "__kKernelMain");
	//if (kernelMain)
	{
		TASKCMDPARAMS cmd;
		__memset((char*)&cmd, 0, sizeof(TASKCMDPARAMS));
		//__kCreateThread((DWORD)__kSpeakerProc, (DWORD)&cmd, "__kSpeakerProc");
		//__kCreateThread((unsigned int)kernelMain, KERNEL_DLL_BASE, (DWORD)&cmd, "__kKernelMain");
		//__kCreateProcess((unsigned int)KERNEL_DLL_SOURCE_BASE, imagesize, "kernel.dll", "__kKernelMain", 3, 0);
	}

	while (1) {
		__asm {
			hlt
		}
#if 0
		uint32_t irr4 = *(DWORD*)(LOCAL_APIC_BASE+0x140);
		if (irr4 & (1 << 1)) {
			__printf(szout,"GOT IPI in IRR! 0x%x\n", irr4);
		}
#endif
		//unsigned int cpuid = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
		//__printf(szout, "ap id:%d wake\r\n", cpuid);
	}
}





void BPCodeStart() {

	int ret = 0;
	char szout[256];

	ret = IsApicSupported();
	if(ret == 0){
		return;
	}

	__asm {cli}
	enableLocalApic();

	//enableIoApic();

#if 0
	enableRcba();
#endif

	int lint0 = *(DWORD*)(LOCAL_APIC_BASE + 0x350);
	int lint1 = *(DWORD*)(LOCAL_APIC_BASE + 0x360);

	*(DWORD*)(LOCAL_APIC_BASE + 0x80) = 0;

	*(DWORD*)(LOCAL_APIC_BASE + 0x350) = 0x700;
	*(DWORD*)(LOCAL_APIC_BASE + 0x360) = 0x400;

	__asm {sti}

	//in bsp the bit 8 of LOCAL_APIC_BASE is set
	int cpu = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

	*(DWORD*)CPU_ID_ADDRESS = cpu;

	*(int*)(CPU_TOTAL_ADDRESS) = 1;

	int ioapic_id = ReadIoApicReg(0) >> 24;

	int ioapic_ver = ReadIoApicReg(1) & 0xff;

	int localapic_ver = *(DWORD*)(LOCAL_APIC_BASE+0x30);

	//WriteIoApicReg(0, g_bsp_id << 24);

	DWORD v = 0;

#if 0
	WaitIcrFree();
	v = 0xc4500;
	*(DWORD*)(LOCAL_APIC_BASE + 0x300) = v;
#else
	SetIcr(0, 0, 5, 3);
#endif
	__sleep(0);

#if 0
	WaitIcrFree();
	v = 0xc4600 | (AP_INIT_ADDRESS >> 12);
	*(DWORD*)(LOCAL_APIC_BASE + 0x300) = v;
#else
	SetIcr(0, AP_INIT_ADDRESS >> 12, 6, 3);
#endif
	__sleep(0);

#if 0
	WaitIcrFree();
	v = 0xc4600 | (AP_INIT_ADDRESS >> 12);
	*(DWORD*)(LOCAL_APIC_BASE + 0x300) = v;
	__sleep(0);
#else
	SetIcr(0, AP_INIT_ADDRESS >> 12, 6, 3);
#endif
	__sleep(0);

	int* ids = (int*)CPU_ID_ADDRESS;
	int cnt = *(int*)(CPU_TOTAL_ADDRESS);
	for (int i = 0; i < cnt; i++) {
#if 0
		SetIcr(ids[i], APIC_IPI_VECTOR, 0,0);
		v = *(DWORD*)(LOCAL_APIC_BASE + 0x300);
		__printf(szout, "%s index:%x,id:%x result:%x\r\n", __FUNCTION__, i, ids[i], v);
#else
		AllocateApTask(APIC_IPI_VECTOR);	
#endif
	}
	//SetIcr(0, APIC_IPI_VECTOR, 0, 3);

	//InitLocalApicErr();

	//InitLocalApicCmci();
	
#ifdef IO_APIC_ENABLE
	__asm {cli}
	
	//initHpet();
	//InitIoApicRte();
	//DisableInt();
	//ret = DisableLocalApicLVT();
	
	g_apic_int_tag = 1;
	__asm {sti}
#endif

	//ret = InitLocalApicTimer();

	__sleep(0);
	//AllocateApTask(2);

	__printf(szout, "bsp id:%d version:%x ipi:%d apic timer:%x init complete. lint0:%x lint1:%x io apic id:%x version:%x\r\n", 
		cpu, localapic_ver, g_ipi_lock, g_lvt_timer,lint0,lint1, ioapic_id, ioapic_ver);

	return;
}


int IsBspProcessor() {
	unsigned int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20)>>24;
	id = id >> 24;
	int bspid = *(int*)CPU_ID_ADDRESS;
	if (id == bspid) {
		return 1;
	}
	return 0;
}

LPPROCESS_INFO GetTaskTssBaseSelected(int id) {

	return (LPPROCESS_INFO)g_ap_tss_base[id];
}

LPPROCESS_INFO GetCurrentTaskTssBase() {
	char szout[256];

	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

	int tsssize = (sizeof(PROCESS_INFO) + 0xfff) & 0xfffff000;
	LPPROCESS_INFO process = (LPPROCESS_INFO)(AP_TASK_TSS_BASE + tsssize * id);
	return process;

	int cnt = *(int*)CPU_TOTAL_ADDRESS;
	if (cnt)
	{
		int* apids = (int*)CPU_ID_ADDRESS;
		for (int i = 0; i < cnt; i++) {
			if (apids[i] == id)
			{

				LPPROCESS_INFO process = (LPPROCESS_INFO)(AP_TASK_TSS_BASE + tsssize * id);
				return process;
			}
		}
	}

	__printf(szout, "%s error,cpu:%d,ap count:%d\r\n", __FUNCTION__, id, cnt);
	return 0;
}

LPPROCESS_INFO GetTaskTssBase() {
	
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

	return (LPPROCESS_INFO)g_ap_tss_base[id];
}

LPPROCESS_INFO SetTaskTssBase() {
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	g_ap_tss_base[id] = (LPPROCESS_INFO)(AP_TASK_TSS_ARRAY + id * 0x400000);

	return (LPPROCESS_INFO)g_ap_tss_base[id];
}




int GetCpu(int* out, int size) {
	int cnt = *(int*)(CPU_TOTAL_ADDRESS);
	if (cnt > size) {
		return 0;
	}
	int* cpus = (int*)CPU_ID_ADDRESS;
	for (int i = 0; i < cnt; i++) {
		out[i] = cpus[i];
	}
	return cnt;
}


void BubbleSort(unsigned int* arr, int count) {
	for (int i = 0; i < count - 1; i++) {
		for (int j = 0; j < count - i - 1; j++) {
			unsigned int low = arr[j] & 0x00ffffff;
			unsigned int high = arr[j + 1] & 0x00ffffff;
			if (low > high) {
				unsigned int temp = arr[j];
				arr[j] = arr[j + 1];
				arr[j + 1] = temp;
			}
		}
	}
}





int GetIdleProcessor() {

	//return 1;

	int counter = *(int*)(CPU_TOTAL_ADDRESS);
	int* ids = (int*)CPU_ID_ADDRESS;
	
	gAllocateAp++;
	if (gAllocateAp >= counter) {
		gAllocateAp = 0;

	}
	int cpuid = ids[gAllocateAp];
	return cpuid;

	char szout[256];

	unsigned int cpuStatus[256];
	__memset((char*)cpuStatus, 0, 256);

	int total = 0;
	int bspid = *(int*)CPU_ID_ADDRESS;
	int cnt = *(int*)CPU_TOTAL_ADDRESS;
	if (cnt <= 1) {
		return bspid;
	}
	
	int* apids = (int*)CPU_ID_ADDRESS;

	for (int num = 0; num < cnt; num++) {
		
		for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
			int cpuid = apids[num];
			LPPROCESS_INFO proc = (LPPROCESS_INFO)g_ap_tss_base[cpuid];
			if (proc[i].status == TASK_RUN) {
				
				if (cpuid < 0 || cpuid >= 256 || cpuid != proc[i].cpuid) {
					__printf(szout, "%s cpuid:%d error\r\n", __FUNCTION__, cpuid);
					break;
				}
				unsigned int c = cpuStatus[cpuid] & 0xffffff;
				unsigned int num = cpuid << 24;
				c++;
				cpuStatus[cpuid] = (num) | (c);
				total++;
			}
		}
	}

	if (total) {
		BubbleSort(cpuStatus, cnt );
	}

	for (int i = 0; i < cnt ; i++) {
		unsigned int c = cpuStatus[i] & 0xffffff;
		unsigned int num = (cpuStatus[i] & 0xff000000) >> 24;
		if (num == bspid) {
			if (i < (cnt + 1) / 2) {
				int n1 = cpuStatus[i] & 0xffffff;
				int n2 = cpuStatus[i + 1] & 0xffffff;
				if (n2 > 2 * n1) {
					return num;
				}
				else {

				}
			}
			else {
				return num;
			}
		}
		else {
			return num;
		}
	}

	return bspid;
}








void EOICommand(int pin) {
	if (g_apic_int_tag) {
		*(DWORD*)(LOCAL_APIC_BASE + 0xB0) = 0;
		*(DWORD*)(IO_APIC_BASE + 0x40) = 0;
	}
	else {
		if ((pin >= INTR_8259_MASTER) && (pin < INTR_8259_MASTER + 8)) {
			outportb(0x20, 0x20);
		}
		else if ((pin >= INTR_8259_SLAVE) && (pin < INTR_8259_SLAVE + 8)) {
			outportb(0x20, 0x20);
			outportb(0xa0, 0x20);
		}
	}

}