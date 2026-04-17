
#include "apic.h"
#include "hardware.h"
#include "Utils.h"
#include "descriptor.h"
#include "algorithm.h"
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
#include "systemService.h"
#include "ml.h"



DWORD * gOicBase = 0;

char * gRcbaBase = 0;

DWORD* gHpetBase = 0;

int g_ini_cmd_lock = 0;

int g_allocate_ap_lock = 0;

int g_ipi_lock[256] = { 0 };
char* g_ipi_buf[256] = { 0 };

int g_apic_int_tag = 0;

LPPROCESS_INFO g_ap_tss_base[256];

unsigned long long g_apic_freq[TASK_LIMIT_TOTAL];

unsigned long long g_timer_cost[TASK_LIMIT_TOTAL];

void enableRcba() {
	outportd(0xcf8, 0x8000f8f0);

	gRcbaBase = (char*)(inportd(0xcfc) & 0xffffc000);
	*gRcbaBase = *gRcbaBase | 1;
}

//bit 9:enable irq 13
//bit 8:enable apic io
void EnableFloatIrq() {
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
	//return;

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


//cpuid.01h:ebx[31:24] or cpuid.04.eax[31:26]
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
	if (tick == 0xffffffff || hg->venderid == 0xffff) {		
		tick = 0x0429b17f;			//not support io apic
	}

	unsigned long long ns = 1000000000000000;

	int freq = 1000 / TASK_TIME_SLICE;

	long long tc =( ns / tick)/ freq;		// 10ms, 14318179 = 1000ms,0x0429b17f=69841279

	*(long long*)(APIC_HPET_BASE + 0x10) = 0;

	//General Interrupt Status Register
	*(long long*)(APIC_HPET_BASE + 0x20) = 0;

	//bit3: writing 1 to this field enables periodic timer and writing 0 enables non - periodic mode.O
	//bit2:Setting this bit to 1 enables triggering of interrupts. Even if this bit is 0, this timer will still set Tn_INT_STS.
	//timer0
	*(unsigned long long*)(APIC_HPET_BASE + 0x100 + 0x20*0) = 0x40 + 8 + 4 + 2;
	//Timer N Comparator Value Register
	//compare with main counter to check if an interrupt should be generated.
	*(unsigned long long*)(APIC_HPET_BASE + 0x108 + 0x20*0) = tc;

	//timer1
	*(unsigned long long*)(APIC_HPET_BASE + 0x100 + 0x20*1) =  4 + 2;
	*(unsigned long long*)(APIC_HPET_BASE + 0x108 + 0x20*1) = ns / tick;

	//Main Counter Value Register,increase in each interruption
	//Writes to this register should only be done when the counter is halted (ENABLE_CNF = 0
	*(long long*)(APIC_HPET_BASE + 0xf0) = 0;

	//Tn_INT_STS
	//bit0:ENABLE_CNF，0:main counter is halted, timer interrupts are disabled
	//bit1:  "legacy replacement" mapping is enabled。timer -> irq0  cmos ->irq8
	*(long long*)(APIC_HPET_BASE + 0x10) = 3;

	return FALSE;
}





extern "C" void __declspec(naked) HpetTimerHandler(LIGHT_ENVIRONMENT * stack) {
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
		sub esp, NATIVE_STACK_LIMIT

		mov eax, KERNEL_MODE_DATA
		mov ds, ax
		mov es, ax
		MOV FS, ax
		MOV GS, AX
		MOV ss, AX
	}

	{
		char szout[256];

		int value = *(int*)(APIC_HPET_BASE + 0x20);
		if (value & 1)
		{
			//__k8254TimerProc();
			//__printf(szout,"hpet timer 0\r\n");
		}
		
		//why not use "else if"?
		if (value & 2) {
			//__printf(szout, "hpet timer 1\r\n");

			unsigned long long next_cnt = *(long long*)(APIC_HPET_BASE + 0xf0);
			unsigned long long period = *(unsigned long long*)(APIC_HPET_BASE + 0x108 + 0x20 * 1);
			next_cnt += period;
			*(unsigned long long*)(APIC_HPET_BASE + 0x108 + 0x20 * 1) = next_cnt;

			__kPeriodTimer();
		}

		*(unsigned __int64*)(APIC_HPET_BASE + 0x20) = 0xff;

		* (DWORD*)(LOCAL_APIC_BASE + 0xB0) = 0;
		*(DWORD*)(IO_APIC_BASE + 0x40) = 0;
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

		jmp HpetTimerHandler
	}
}


void IoApicRedirect(int pin, int cpu, int vector, int mode) {

	static int gAllocateAp;

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
	//__printf(szout, "io apic %d value %I64x\r\n ",pin, oldValue);

	setIoRedirect(pin, cpuid, vector, mode);
}


void SetIcr(int cpu,int vector,int mode,int destType) {

	__enterSpinlock(&g_ini_cmd_lock);

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
	//__printf(szout, "%s cpu:%x result:%x\r\n", __FUNCTION__, cpu, v);

	__leaveSpinlock(&g_ini_cmd_lock);

	return;
}

int InitIoApicRte() {
	
	int bsp_id = *(int*)(CPU_ID_ADDRESS);

	IoApicRedirect(0x10, bsp_id, 0x10000, 0);

	IoApicRedirect(0x12, bsp_id, INTR_8259_MASTER + 1, 0);

	IoApicRedirect(0x14, bsp_id, APIC_HPETTIMER_VECTOR , 0);

	IoApicRedirect(0x16, bsp_id, INTR_8259_MASTER + 3, 0);
	IoApicRedirect(0x18, bsp_id, INTR_8259_MASTER + 4, 0);
	IoApicRedirect(0x1a, bsp_id, INTR_8259_MASTER + 5, 0);
	IoApicRedirect(0x1c, bsp_id, INTR_8259_MASTER + 6, 0);
	IoApicRedirect(0x1e, bsp_id, INTR_8259_MASTER + 7, 0);

	IoApicRedirect(0x20, bsp_id, APIC_HPETTIMER_VECTOR, 0);

	IoApicRedirect(0x22, bsp_id, INTR_8259_SLAVE + 1, 0);
	IoApicRedirect(0x24, bsp_id, INTR_8259_SLAVE + 2, 0);
	IoApicRedirect(0x26, bsp_id, INTR_8259_SLAVE + 3, 0);
	IoApicRedirect(0x28, bsp_id, INTR_8259_SLAVE + 4, 0);
	IoApicRedirect(0x2a, bsp_id, INTR_8259_SLAVE + 5, 0);
	IoApicRedirect(0x2c, bsp_id, INTR_8259_SLAVE + 6, 0);
	IoApicRedirect(0x2e, bsp_id, INTR_8259_SLAVE + 7, 0);

	IoApicRedirect(0x30, bsp_id, 0x10000, 0);
	IoApicRedirect(0x32, bsp_id, 0x10000 + 1, 0);
	IoApicRedirect(0x34, bsp_id, 0x10000 + 2, 0);
	IoApicRedirect(0x36, bsp_id, 0x10000 + 3, 0);
	IoApicRedirect(0x38, bsp_id, 0x10000 + 4, 0);
	IoApicRedirect(0x3a, bsp_id, 0x10000 + 5, 0);
	IoApicRedirect(0x3c, bsp_id, 0x10000 + 6, 0);
	IoApicRedirect(0x3e, bsp_id, 0x10000 + 7, 0);

	return 0;
}





int IpiCreateThread(char* addr,  char* module, unsigned long p, char* funname)
{
	int ret = 0;
	int id = GetIdleProcessor();

	__enterSpinlock(&g_ipi_lock[id]);

	IPI_MSG_PARAM* msg = (IPI_MSG_PARAM*)g_ipi_buf[id];
	for (int i = 0; i < IPI_MSG_LIMIT; i++) {
		if (msg[i].valid == 0) {

			msg[i].cmd = IPI_CREATETHREAD;
			msg[i].valid = 1;
			msg[i].id = id;
			IPI_CREATETHREAD_PARAM* subparam = (IPI_CREATETHREAD_PARAM*)msg[i].param;
			subparam->addr = (DWORD)addr;
			subparam->module = module;
			__strcpy(subparam->funcname, funname);
			if (p) {
				subparam->lpparam = subparam->params;
				__memcpy(subparam->params, (char*)p, sizeof(TASKCMDPARAMS));
			}
			else {
				subparam->lpparam = 0;
				__memset(subparam->params, 0, sizeof(TASKCMDPARAMS));
			}

			__leaveSpinlock(&g_ipi_lock[id]);
			SetIcr(id, APIC_IPI_VECTOR, 0, 0);
#ifdef _DEBUG
			char szout[256];
			__printf(szout, "%s cpu:%d module:%x function:%s\r\n", __FUNCTION__, id, subparam->module, subparam->funcname);
#endif
			return TRUE;
		}
	}

	__leaveSpinlock(&g_ipi_lock[id]);
	
	return 0;
}



int IpiCreateProcess(DWORD base, int size, char* fn, char* func, int level, unsigned long p)
{
	int ret = 0;

	int id = 0;
	if (base) {
		int petype = getPeType(base);
		if (petype == DOS_EXE_FILE || petype == DOS_COM_FILE)
		{
			id = 0;
		}
		else {
			id = GetIdleProcessor();
		}
	}
	else {
		id = GetIdleProcessor();
	}

	__enterSpinlock(&g_ipi_lock[id]);

	IPI_MSG_PARAM* msg = (IPI_MSG_PARAM*)g_ipi_buf[id];
	for (int i = 0; i < IPI_MSG_LIMIT; i++) {
		if (msg[i].valid == 0) {
			msg[i].cmd = IPI_CREATEPROCESS;
			msg[i].valid = 1;
			msg[i].id = id;
			IPI_CREATEPROCESS_PARAM* subparam = (IPI_CREATEPROCESS_PARAM*)msg[i].param;
			subparam->base = (DWORD)base;
			subparam->size = size;
			__strcpy((char*)subparam->filename, fn);
			__strcpy(subparam->funcname, func);
			subparam->level = level;
			if (p) {
				subparam->lpparam = subparam->params;
				__memcpy(subparam->params, (char*)p, sizeof(TASKCMDPARAMS));
			}
			else {
				subparam->lpparam = 0;
				__memset(subparam->params, 0, sizeof(TASKCMDPARAMS));
			}

			__leaveSpinlock(&g_ipi_lock[id]);
			SetIcr(id, APIC_IPI_VECTOR, 0, 0);

#ifdef _DEBUG
			char szout[256];
			__printf(szout, "%s cpu:%d base:%x size:%x module:%s addr:%p function:%s addr:%p level:%d param:%x\r\n", 
				__FUNCTION__, id, subparam->base, subparam->size, subparam->base, &subparam->base, subparam->funcname, &subparam->funcname, subparam->level, subparam->params);
#endif

			return TRUE;
		}
	}

	__leaveSpinlock(&g_ipi_lock[id]);
	
	return 0;
}


extern "C" void __declspec(naked) IPIIntHandler(LIGHT_ENVIRONMENT * stack) {
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
		sub esp, NATIVE_STACK_LIMIT

		mov eax, KERNEL_MODE_DATA
		mov ds, ax
		mov es, ax
		MOV fs, ax
		MOV gs, ax
		mov ss, ax
	}

	{
		char szout[256];
		int cpu = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
		int ret = __enterSpinlock(&g_ipi_lock[cpu]);
		//if (ret) 
		{
			IPI_MSG_PARAM* msg = (IPI_MSG_PARAM*)g_ipi_buf[cpu];
			for (int i = 0; i < IPI_MSG_LIMIT; i++) {
				if (msg[i].valid && msg[i].id == cpu) {

					int cmd = msg[i].cmd;

					msg[i].valid = 0;

					//__printf(szout,"cpu:%d %s %d cmd:%d\r\n",id, __FUNCTION__,__LINE__,cmd);

					if (cmd == IPI_CREATEPROCESS) {
						IPI_CREATEPROCESS_PARAM* subparam = (IPI_CREATEPROCESS_PARAM*)msg[i].param;
						DWORD base = (DWORD)subparam->base;
						DWORD size = subparam->size;
						char* fn = subparam->filename;
						char* funcname = subparam->funcname;
						int level = subparam->level;
						char* p = (char*)subparam->lpparam;

						//__printf(szout, "%s module:%x addr:%p function:%s addr:%p\r\n", __FUNCTION__, module,&subparam->module, funcname,&subparam->funcname);

						if (__findProcessFileName(subparam->funcname) == FALSE)
						{
							//__kCreateProcess(base, size, (subparam->module), (subparam->funcname), level, (unsigned long)p);
						}
						__kCreateProcess(base, size, fn, funcname, level, (unsigned long)p);
					}
					else if (cmd == IPI_CREATETHREAD) {

						IPI_CREATETHREAD_PARAM* subparam = (IPI_CREATETHREAD_PARAM*)msg[i].param;
						DWORD module = (DWORD)subparam->module;
						DWORD addr = subparam->addr;
						char* funcname = subparam->funcname;
						char* p = (char*)subparam->lpparam;

						//__printf(szout, "%s module:%x function:%s\r\n", __FUNCTION__, module, funcname);

						if (__findProcessFileName(funcname) == FALSE)
						{

						}
						__kCreateThread((DWORD)addr, (DWORD)module, (unsigned long)p, funcname);
					}
					else if (cmd == IPI_SWITCHTASK) {

					}
					else if (cmd == IPI_TASKSWITCH) {
						//__kTaskSchedule((LIGHT_ENVIRONMENT*)stack);
					}
					else {

					}
				}
			}
			__leaveSpinlock(&g_ipi_lock[cpu]);
		}
		
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
		jmp IPIIntHandler
	}
}



extern "C" void __declspec(naked) LVTTimerIntHandler(LIGHT_ENVIRONMENT* stack) {
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
		sub esp, NATIVE_STACK_LIMIT

		mov eax, KERNEL_MODE_DATA
		mov ds, ax
		mov es, ax
		MOV FS, ax
		MOV GS, AX	
		MOV ss, AX

#ifndef SINGLE_TASK_TSS
		clts
#endif
	}

	{
		char szout[256];

		//__printf(szout, "LVTTimerIntHandlers esp local value:%x\r\n", szout);

		__kTaskSchedule((LIGHT_ENVIRONMENT*)stack);

		//LPPROCESS_INFO next = SingleTssSchedule(stack);

		//*(DWORD*)(LOCAL_APIC_BASE + 390) = 0;

		*(DWORD*)(LOCAL_APIC_BASE + 0xB0) = 0;
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
		sub esp, NATIVE_STACK_LIMIT

		mov eax, KERNEL_MODE_DATA
		mov ds, ax
		mov es, ax
		MOV FS, ax
		MOV GS, AX
		MOV ss, AX
	}

	{
		char szout[256];
		
		DWORD low = 0;
		DWORD high = 0;
		readmsr(0x19c, &low, &high);

		__printf(szout, "%s local apic temperature low:%x,high:%x\r\n", __FUNCTION__, low,high);

		low = 0x1f + 0x8000 + 0xc00000;
		writemsr(0x19c, low, high);

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
		sub esp, NATIVE_STACK_LIMIT

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
		sub esp, NATIVE_STACK_LIMIT

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
		sub esp, NATIVE_STACK_LIMIT

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

		iretd
	}
}

//https://blog.csdn.net/weixin_46645613/article/details/120406002
//https://zhuanlan.zhihu.com/p/406213995
//https://zhuanlan.zhihu.com/p/678582090
//https://www.zhihu.com/question/594531181/answer/2982337869










int AllocateApTask(int intnum) {
	static int gAllocateAp = 0;

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

	return 0;

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

int InitApicThermalMonitor() {

	DWORD tj = 0;
	CpuTemperature(&tj);

	int threshHold1 = tj - 10;

	int threshHold2 = tj - 30;
	DWORD low = 0x1f + (threshHold1 << 8) + (0x8000) + (threshHold2 << 16) + 0xc00000;
	DWORD high = 0;

	//exception ?
	//writemsr(0x19b, low, high);

	unsigned long v = 0;
	v = APIC_LVTTEMPERATURE_VECTOR;
	*(DWORD*)(LOCAL_APIC_BASE + 0x330) = v ;
	return 0;
}


int InitApicPerformMonitor() {
	int v = APIC_LVTPERFORMANCE_VECTOR;
	*(DWORD*)(LOCAL_APIC_BASE + 0x340) = v ;
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




unsigned long long ApicTimerFreq() {
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	return g_apic_freq[id] ;
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

	unsigned long long lv = APICTIMER_FREQ;

	//lv = lv /16 / (1000 / TASK_TIME_SLICE);
	*(DWORD*)(LOCAL_APIC_BASE + 0x380) = (DWORD)0;
	*(DWORD*)(LOCAL_APIC_BASE + 0x380) = (DWORD)0xffffffff;

	unsigned long long freq = 0;
	int times = 8;
	unsigned  long long ticks = 0;
	unsigned  long long tick;
	for (int i = 0; i < times; i++) {
		freq += GetApicTimerFreq(&tick);
		ticks += tick;
	}
	freq = freq / times;
	ticks = ticks / times;

	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	g_apic_freq[id] = freq / (1000 / TASK_TIME_SLICE);
	g_timer_cost[id] = ticks;

	//1 / frequency * counter = time cost in one period
	//counter = time * frequency
	freq = freq /(1000 / TASK_TIME_SLICE);

	*(DWORD*)(LOCAL_APIC_BASE + 0x380) = (DWORD)0;

	*(DWORD*)(LOCAL_APIC_BASE + 0x380) = (DWORD)freq;

	char szout[256];
	__printf(szout, "apic timer init value:%I64x\r\n", freq);

	return 0;
}


extern "C" void __declspec(dllexport) __kApInitProc() {
	char* reg_esp = 0;
	char* reg_ebp = 0;
	int ret = 0;
	__asm {
		mov ds:[reg_esp],esp
		mov ds : [reg_ebp] , ebp
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

	g_cpu_start_tick[cpuid] = __krdtsc();

	g_ipi_buf[cpuid] = (char*)__kMalloc(sizeof(IPI_MSG_PARAM) * IPI_MSG_LIMIT);
	__memset(g_ipi_buf[cpuid], 0, sizeof(IPI_MSG_PARAM) * IPI_MSG_LIMIT);

	//__enterLock(&g_allocate_ap_lock);
	__enterSpinlock(&g_allocate_ap_lock);

	//__printf(szout, "cpu:%d enter lock\r\n", cpuid);

	int seq = *(int*)CPU_TOTAL_ADDRESS;
	int* apids = (int*)CPU_ID_ADDRESS;
	apids[seq] = cpuid;
	*(int*)(CPU_TOTAL_ADDRESS) = seq + 1;

	int ioapic_id = ReadIoApicReg(0) >> 24;

	int ioapic_ver = ReadIoApicReg(1) & 0xff;

	SetTaskVideoBase((char*)gGraphBase);

	char* lpgdt = InitGdt();

	char * lpidt = InitIDT();

	char procname[64];
	__sprintf(procname, "Apic_Process_%d", cpuid);
	int mytid = __initTask0((char*)LIUNUX_KERNEL32_DLL, procname,0, GRAPHCHAR_HEIGHT * cpuid * 16 + 256);

	EnablePaging32((char*)PDE_ENTRY_VALUE);

	short tr_new;
	DescriptTableReg idtbase_new;
	DescriptTableReg gdtbase_new;
	__asm {
		sgdt gdtbase_new
		sidt idtbase_new
		str ax
		mov tr_new, ax
	}
	LPPROCESS_INFO process = GetCurrentTaskTssBase();
	char* gdt = GetGdtBase();
	unsigned long long tssdesc = *(unsigned long long*)(gdt + kTssTaskSelector );
	__printf(szout, "ap id:%d process:%x tss:%i64x idt:%x size:%x gdt:%x size:%d ltr:%x\r\n",
		cpuid,process, tssdesc, idtbase_new.addr, idtbase_new.size,gdtbase_new.addr, gdtbase_new.size, tr_new);
	
	initCoprocessor();
	EnableSyscall();
	SysenterInit((DWORD)SysenterEntry);
	enableVME();
	enablePCE();
	enableMCE();
	enableTSD();
	initDebugger();

	__leaveSpinlock(&g_allocate_ap_lock);
	//__leaveLock(&g_allocate_ap_lock);

	InitLocalApicErr();

	InitLocalApicCmci();

	//*(DWORD*)(LOCAL_APIC_BASE + 0x350) = 0x700;

	//__asm{int APIC_IPI_VECTOR}

	ret = InitLocalApicTimer();

	InitPm();

	GetCpuRate();

	InitApicThermalMonitor();

	InitApicPerformMonitor();

	//int imageSize = getSizeOfImage((char*)MAIN_DLL_SOURCE_BASE);
	//__kCreateProcess(MAIN_DLL_SOURCE_BASE, imageSize, (char*)"main.dll", (char*)"__DummyProcess", 3, 0);

	__asm {sti}

	char* reg_esp_new = 0;
	__asm {
		mov ds : [reg_esp_new] , esp
	}
	int tid = process->tid;
	unsigned long stacktop = (unsigned long)process->tss.esp;
	unsigned long stack0top = (unsigned long)process->tss.esp0;

	DWORD reg_cr0 = 0;
	DWORD reg_cr4 = 0;
	__asm {
		mov eax, cr0
		mov[reg_cr0], eax

		//mov eax,cr4
		__emit 0x0f
		__emit 0x20
		__emit 0xe0
		mov[reg_cr4], eax
		__emit 0x0f
		__emit 0x22
		__emit 0xe0
	}

	__printf(szout, "ap id:%d version:%x cr0:%x cr4:%x init complete.esp:%x,ebp:%x, esp_new:%x,esp top:%x esp0:%x lint0:%x lint1:%x tid:%d io apic id:%x version:%x\r\n",
		cpuid, localapic_ver, reg_cr0, reg_cr4,reg_esp, reg_ebp, reg_esp_new, stacktop, stack0top, lint0, lint1, tid, ioapic_id, ioapic_ver);

	AdjustApicTimer();

	while (1) {
		__sleep(0);
		__asm {
			//hlt
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
	__memset((char*)IPI_MSG_BASE, 0, 0x10000);
	__asm {cli}

	enableLocalApic();

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
	g_ipi_buf[cpu] = (char*)__kMalloc(sizeof(IPI_MSG_PARAM) * IPI_MSG_LIMIT);
	__memset(g_ipi_buf[cpu], 0, sizeof(IPI_MSG_PARAM) * IPI_MSG_LIMIT);

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
	//SetIcr(0, AP_INIT_ADDRESS >> 12, 6, 3);
#endif
	__sleep(0);

	int* ids = (int*)CPU_ID_ADDRESS;
	int cnt = *(int*)(CPU_TOTAL_ADDRESS);
	for (int i = 0; i < cnt; i++) {
#if 1
		//SetIcr(ids[i], APIC_IPI_VECTOR, 0,0);

		//v = *(DWORD*)(LOCAL_APIC_BASE + 0x300);
		//__printf(szout, "%s index:%x,id:%x result:%x\r\n", __FUNCTION__, i, ids[i], v);
#else
		AllocateApTask(APIC_IPI_VECTOR);	
#endif
	}
	//SetIcr(0, APIC_IPI_VECTOR, 0, 3);

	InitLocalApicErr();

	InitLocalApicCmci();

	InitApicThermalMonitor();

	InitApicPerformMonitor();

	//ret = DisableLocalApicLVT();

#ifdef IO_APIC_ENABLE
	enableIoApic();

#if 0
	enableRcba();
#endif

	__asm {cli}
	DisableInt();
	initHpet();
	InitIoApicRte();
	g_apic_int_tag = 1;
	__asm {sti}
#endif

	//ret = InitLocalApicTimer();

	g_cpu_start_tick[cpu] = __krdtsc();

	DWORD reg_cr0 = 0;
	DWORD reg_cr4 = 0;
	__asm {
		mov eax,cr0
		mov [reg_cr0],eax

		//mov eax,cr4
		__emit 0x0f
		__emit 0x20
		__emit 0xe0
		mov[reg_cr4], eax
		__emit 0x0f
		__emit 0x22
		__emit 0xe0
	}
	__printf(szout, "bsp id:%d cr0:%x cr4:%x. version:%x init complete. lint0:%x lint1:%x io apic id:%x version:%x\r\n", 
		cpu,reg_cr0,reg_cr4, localapic_ver,lint0,lint1, ioapic_id, ioapic_ver);

	AdjustApicTimer();

	return;
}


int IsBspProcessor() {
	unsigned int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20)>>24;

	int bspid = *(int*)CPU_ID_ADDRESS;
	if (id == bspid) {
		return 1;
	}
	return 0;
}


#ifdef _DEBUG
#include <stdio.h>
#include <stdlib.h>

LPPROCESS_INFO g_proc_info = 0;

LPPROCESS_INFO DebugCreateProcess() {
	if (g_proc_info == 0) {
		g_proc_info = (LPPROCESS_INFO)malloc(0x10000);
		__memset((char*)g_proc_info, 0, sizeof(PROCESS_INFO));
		int debug_heap_size = 0x100000;
		g_proc_info->fast_heap = (char*)malloc(debug_heap_size);
		g_proc_info->fast_heap_large = 0;
		//g_proc_info->fast_heap_large = (char*)malloc(debug_heap_size);

		int total = 4;
		g_proc_info->lpHeapBase = &g_proc_info->heapBase;
		for (int num = 0; num < total; num++) {
			char * buf = (char*)malloc(debug_heap_size<< num);
			if (buf) {
				g_proc_info->lpHeapBase[num] = buf;
			}
			else {
				break;
			}
		}
		g_proc_info->heapCnt = total;

		g_proc_info->lpHeapCnt = &g_proc_info->heapCnt;
		g_proc_info->heapsize = debug_heap_size;
		g_proc_info->heap_lock = 0;
		g_proc_info->lpheap_lock = &g_proc_info->heap_lock;
		g_proc_info->va_size = 0;
		g_proc_info->lpvasize = &g_proc_info->va_size;
	}
	return 0;
}

#endif

LPPROCESS_INFO GetCurrentTaskTssBase() {
#ifdef _DEBUG
	if (g_proc_info == 0) {
		DebugCreateProcess();
	}
	LPPROCESS_INFO tss = g_proc_info;
	return g_proc_info;
#else

#endif
	char szout[256];

	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

	int tssSize = (sizeof(PROCESS_INFO) + 0xfff) & 0xfffff000;
	LPPROCESS_INFO process = (LPPROCESS_INFO)(TASK_TSS_BASE + tssSize * id);
	return process;
	/*
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
	*/
}


int GetTssSize() {
	int tssSize = (sizeof(PROCESS_INFO) + 0xfff) & 0xfffff000;
	return tssSize;
}

LPPROCESS_INFO GetTaskTssBase() {
	
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

	LPPROCESS_INFO proc = (LPPROCESS_INFO)g_ap_tss_base[id];
	if (proc == 0) {
		proc = SetTaskTssBase();
		g_ap_tss_base[id] = proc;
	}
	return proc;
}

LPPROCESS_INFO SetTaskTssBase() {
	char szout[256];
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	if (id == 0) {
		int tssSize = (sizeof(PROCESS_INFO) + 0xfff) & 0xfffff000;
		unsigned long size = tssSize * TASK_LIMIT_TOTAL;
		g_ap_tss_base[id] = (LPPROCESS_INFO)(AP_TASK_TSS_ARRAY + id * size);
	}
	else {
		int tssSize = (sizeof(PROCESS_INFO) + 0xfff) & 0xfffff000;
		unsigned long size = tssSize* TASK_LIMIT_TOTAL;
		char * buf =(char*) __kProcessMalloc(size, &size, 0, id, 0, PAGE_READWRITE | PAGE_USERPRIVILEGE | PAGE_PRESENT);
		g_ap_tss_base[id] = (LPPROCESS_INFO)buf;
		if (buf == 0) {
			__printf(szout,"%s %d error\r\n", __FUNCTION__, __LINE__);
		}
	}
	
	return (LPPROCESS_INFO)g_ap_tss_base[id];
}

LPPROCESS_INFO GetTaskTssBaseId(int id) {

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








int GetIdleProcessor() {

	int* ids = (int*)CPU_ID_ADDRESS;
	int counter = *(int*)(CPU_TOTAL_ADDRESS);
	AlgorithmModel times[TASK_LIMIT_TOTAL];
	unsigned long long tick = __krdtsc();
	for (int i = 0; i < counter; i++) {
		int id = ids[i];	
		double cpu_diff = tick - g_cpu_start_tick[id];
		double cpu_ratio = (double)g_cpu_tick[id] / cpu_diff;
		//times[i].v = g_cpu_tick[id];
		//__memcpy((char*)&times[i].v,(char*) & cpu_ratio, sizeof(double));
		times[i].fv = cpu_ratio;
		times[i].id = id;
	}

	BubbleSort_ull(times, counter);
	
	return (int)times[0].id;

	/*
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
	*/
}


unsigned long GetValueFromArray(AlgorithmModel* array,int size,int key) {
	for(int i = 0; i < size; i++) {
		if(array[i].id == key) {
			return (unsigned long)array[i].v;
		}
	}
	return 0;
}

int g_debug_tag = 0;


int GetCongestion(int * procs) {
	int id = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;
	unsigned long long tick = __krdtsc();
	double cpu_diff = tick - g_cpu_start_tick[id];
	double cpu_ratio = (double)g_cpu_tick[id] / cpu_diff;

	int max_tid = -1;
	double max_ratio = 0.0;

	int n = 0;

	int cnt = 0;

	if (cpu_ratio > 0.5) {
		LPPROCESS_INFO tss = GetTaskTssBase();
		for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {
			if (tss[i].status == TASK_RUN) {
				n++;

				double proc_cpu_ratio = (double)tss[i].tick / cpu_diff;
				if (proc_cpu_ratio > max_ratio) {
					max_ratio = proc_cpu_ratio;
					max_tid = tss[i].tid;
				}

				double proc_diff = tss[i].tick_total;
				double proc_ratio = (double)tss[i].tick / proc_diff;
				if (proc_ratio > 0.5) {
					procs[cnt++] = tss[i].tid;
				}
			}
		}
	}

	return cnt;
}



PROCESS_INFO * GetReadyProcess() {

	char szout[256];

	LPPROCESS_INFO target_tss = 0;
	PROCESS_INFO* tss = GetTaskTssBase();
	PROCESS_INFO* process = GetCurrentTaskTssBase();
	LPPROCESS_INFO current = (LPPROCESS_INFO)(tss + process->tid);
	LPPROCESS_INFO ptr = current;
	LPPROCESS_INFO next = 0;
	
	AlgorithmModel tickc[TASK_LIMIT_TOTAL];
	int cpu = *(DWORD*)(LOCAL_APIC_BASE + 0x20) >> 24;

	int window[TASK_LIMIT_TOTAL];

	int user[TASK_LIMIT_TOTAL];

	AlgorithmModel delta[TASK_LIMIT_TOTAL];

	AlgorithmModel level[TASK_LIMIT_TOTAL];

	int count = 0;
	do {
		ptr++;
		if (ptr - tss >= TASK_LIMIT_TOTAL) {
			ptr = tss;
		}

		if (ptr == 0 || ptr == current) {
			break;
		}

		if (cpu != ptr->cpuid) {
			continue;
		}

		if (ptr->status == TASK_TERMINATE) {
			ptr->status = TASK_OVER;
			continue;
		}
		else if (ptr->status == TASK_RUN) {
			if (ptr->sleep) {
				ptr->sleep--;
			}
			else {
				double ratio = 0.0;
				if (ptr->tick == 0) {
					ratio = 1.0;
				}
				else {
					double diff = (double)(ptr->tick_total);
					ratio = ((double)ptr->tick) / diff;
					if (ratio > 0.9) 
					{
						//ratio = 0.01;
					}
				}
				tickc[count].id = ptr->tid;
				__memcpy((char*)&tickc[count].v, (char*)&ratio, sizeof(double));

				if (g_debug_tag++ % 0x1000 == 0x1000) {
					__printf(szout, "tick_start:%lf, diff:%i64x,tick:%I64x, ratio:%lf\r\n",
						tickc[count].v, ptr->tick_total, ptr->tick, ratio);
				}

				window[count] = (ptr->window == 0 ? 0 : WINDOW_PRIORITY);

				user[count] = (ptr->level == 0 ? USER_PRIORITY : 0);

				delta[count].v = ptr->delta;
				delta[count].id = ptr->tid;

				level[count].id = ptr->tid;
				level[count].v = 0;

				count++;

				if (next == 0) {
					next = ptr;
				}
			}
		}
		else if (ptr->status == TASK_OVER) {
			continue;
		}
		else if (ptr->status == TASK_SUSPEND) {
			continue;
		}
	} while (TRUE);

	if (count == 1) {
		return next;
	}
	else if (count <= 0) {
		target_tss = current;
		//__printf(szout, "%s %d count:%d\r\n", __FUNCTION__, __LINE__);
	}
	else if (count > 1) {
		int target_id = 0;

		QuickSort(tickc, 0, count - 1);
		for (int i = 0; i < count; i++) {
			tickc[i].v = STATIC_PRIORITY / (count - i);
		}

		for (int i = 0; i < count; i++) {
			for (int j = 0; j < count; j++) {
				if (level[i].id == tickc[j].id) {
					level[i].v += tickc[j].v;
					break;
				}
			}
			level[i].v += (window[i] + user[i]);
			int pid = level[i].id;
			level[i].v += tss[pid].delta;
			level[i].v += tss[pid].priority;
			level[i].v += tss[pid].authority;
		}

		QuickSort(level, 0, count - 1);

		QuickSort(delta, 0, count - 1);
		if (delta[count - 1].v > DYNAMIC_PRIORITY) {
			target_id = delta[count - 1].id;
		}
		else
			target_id = level[count - 1].id;
		
		target_tss = tss + target_id;

		extern int g_train_complete;

		TaskPredictParam tp;
		int times = count / ML_TASK_LIMIT;
		int mod = count % ML_TASK_LIMIT;
		int n = 0;
		for ( n = 0; n < times; n++) {
			int idx = n * ML_TASK_LIMIT;
			for (int i = idx; i <  idx+ ML_TASK_LIMIT; i++) {
				int pid = tickc[i].id;
				float tick_ratio = (float)GetValueFromArray(tickc, count, pid) / (float)STATIC_PRIORITY;
				float user_ratio = (float)(tss[pid].level == 0 ? USER_PRIORITY : 0) / (float)STATIC_PRIORITY;
				float window_ratio = (float)(tss[pid].window ? WINDOW_PRIORITY : 0) / (float)STATIC_PRIORITY;
				float delta_ratio = (float)GetValueFromArray(delta, count, pid) / (float)STATIC_PRIORITY;
				float priority_ratio = (float)(tss[pid].priority) / (float)STATIC_PRIORITY;
				float authority_r = (float)tss[pid].authority / (float)STATIC_PRIORITY;

				if (pid == target_id) {
					tp.result = i% ML_TASK_LIMIT;
				}
				tp.task[i % ML_TASK_LIMIT].tick = tick_ratio;
				tp.task[i % ML_TASK_LIMIT].user = user_ratio;
				tp.task[i % ML_TASK_LIMIT].window = window_ratio;
				tp.task[i % ML_TASK_LIMIT].delta = delta_ratio;
				tp.task[i % ML_TASK_LIMIT].priority = priority_ratio;
				tp.task[i % ML_TASK_LIMIT].authority = authority_r;
			}
			if (g_train_complete == 0) {
				SaveMlData(&tp);
			}		
		}
		
		n = times * ML_TASK_LIMIT;
		for (int i = n; i < n + mod; i++) {
			int pid = tickc[i].id;
			float tick_ratio = (float)GetValueFromArray(tickc, count, pid) / (float)STATIC_PRIORITY;
			float user_ratio = (float)(tss[pid].level == 0 ? USER_PRIORITY : 0) / (float)STATIC_PRIORITY;
			float window_ratio = (float)(tss[pid].window ? WINDOW_PRIORITY : 0) / (float)STATIC_PRIORITY;
			float delta_ratio = (float)GetValueFromArray(delta, count, pid) / (float)STATIC_PRIORITY;
			float priority_ratio = (float)(tss[pid].priority) / (float)STATIC_PRIORITY;
			float authority_r = (float)tss[pid].authority / (float)STATIC_PRIORITY;

			if (pid == target_id) {
				tp.result = i % ML_TASK_LIMIT;
			}
			tp.task[i % ML_TASK_LIMIT].tick = tick_ratio;
			tp.task[i % ML_TASK_LIMIT].user = user_ratio;
			tp.task[i % ML_TASK_LIMIT].window = window_ratio;
			tp.task[i % ML_TASK_LIMIT].delta = delta_ratio;
			tp.task[i % ML_TASK_LIMIT].priority = priority_ratio;
			tp.task[i % ML_TASK_LIMIT].authority = authority_r;
		}

		for (int i = n+mod; i < n+ML_TASK_LIMIT; i++) {
			tp.task[i % ML_TASK_LIMIT].tick = 0.0;
			tp.task[i % ML_TASK_LIMIT].user = 0.0;
			tp.task[i % ML_TASK_LIMIT].window = 0.0;
			tp.task[i % ML_TASK_LIMIT].delta = 0.0;
			tp.task[i % ML_TASK_LIMIT].priority = 0.0;
			tp.task[i % ML_TASK_LIMIT].authority = 0.0;
		}
#ifndef _DEBUG
		if (g_debug_tag++ % 0x1000 == 0x1000) {
			for (int i = 0; i < ML_TASK_LIMIT; i++) {
				__printf(szout, "%d:  %f   %f   %f   %f  %f result:%d\r\n", 
					i, tp.task[i].tick, tp.task[i].user, tp.task[i].window, tp.task[i].delta, tp.task[i].priority,tp.result);
			}
		}
#endif
		if (g_train_complete == 0) {
			SaveMlData(&tp);
		}
		
		if (g_train_complete) {
			int seq = TaskSwitchPrediction(&tp);
			if (seq >= 0 && seq < count) {

				target_id = tickc[seq].id;
			}
			else {
				target_id = 0;
				__printf(szout, "TaskSwitchPrediction seq:%d,count:%d\r\n", seq,count);
			}
			
			target_tss = tss + target_id;
		}

		for (int i = 0; i < count; i++) {
			int tid = delta[i].id;
			if (target_id != delta[i].id) {
				if (tid == next->tid) {
					tss[tid].delta += 1;
				}
				else {
					tss[tid].delta += 1;
				}

				if (tss[tid].delta > DYNAMIC_PRIORITY) {
					tss[tid].delta = DYNAMIC_PRIORITY;
				}
			}
			else {
				tss[tid].delta = 0;
			}
		}

	}

	target_tss->delta = 0;
	target_tss->authority = 0;

	return target_tss;
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