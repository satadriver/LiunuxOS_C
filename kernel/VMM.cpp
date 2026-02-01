
#include "VMM.h"
#include "def.h"
#include "Utils.h"
#include "malloc.h"
#include "descriptor.h"



//https://blog.csdn.net/qq_41988448/article/details/122791042
//https://www.cnblogs.com/wingsummer/p/15971289.html

VMX_CPU g_VMXCPU;



int IsVTEnabled()
{
    char szout[256];

    ULONG recx = 0;
    __asm {
        mov eax,1
        cpuid
        mov [recx],ecx
    }
    _CPUID_ECX* uCPUID = (_CPUID_ECX*)&recx;
    if (uCPUID->VMX != 1)
    {
        __printf(szout, "CPU not support VT!\r\n");
        return FALSE;
    }


    unsigned long low, high;
    readmsr(MSR_IA32_FEATURE_CONTROL,&low,&high);  
    __int64 v = ((__int64)high << 32) + low;
    IA32_FEATURE_CONTROL_MSR* cmsr = (IA32_FEATURE_CONTROL_MSR*)&v;
    if (cmsr->Lock != 1)
    {
        __printf(szout, "VT not locked!\r\n");
        return FALSE;
    }


    ULONG rcr0,rcr4 = 0;
    __asm {
        mov eax,cr0
        mov [rcr0],eax

        __emit 0x0f
        __emit 0x20
        __emit 0xe0
        //mov eax, cr4


        mov [rcr4],eax
    }
    _CR0 * uCr0 = (_CR0*)&rcr0;
    _CR4* uCr4 = (_CR4*)&rcr4;
    if (uCr0->PE != 1 || uCr0->PG != 1 || uCr0->NE != 1)
    {
        __printf(szout, "PE or PG or NE zero\r\n");
        return FALSE;
    }

    if (uCr4->VMXE == 1)
    {
        
    }
    else {
        __asm {
            //mov eax,cr4
            __emit 0x0f
            __emit 0x20
            __emit 0xe0
            or eax, 0x2000
            //mov cr4, eax
            __emit 0x0f
            __emit 0x22
            __emit 0xe0

        }
    }


    return TRUE;
}

int StartVirtualTechnology()
{
    char szout[1024];

    char* pVMXONRegion;
    
    ULONG uRevId;

   
    if (!IsVTEnabled())
        return 0;

    pVMXONRegion = (char*)__kMalloc(0x1000);

    g_VMXCPU.pVMXONRegion = pVMXONRegion;
    g_VMXCPU.pVMXONRegion_PA = pVMXONRegion;	//获得这块内存的物理地址

    unsigned long low, high;
    readmsr(MSR_IA32_VMX_BASIC, &low, &high);
    __int64 v = ((__int64)high << 32) + low;
    VMX_BASIC_MSR* cmsr = (VMX_BASIC_MSR*)&v;

    uRevId = cmsr->RevId;

    *((PULONG)g_VMXCPU.pVMXONRegion) = uRevId & 0x7fffffff;		//设置版本标识符

    char * vaddr =(char* )pVMXONRegion;
    
    __asm {
        push 0
        push vaddr
        _emit 0xf3;
        _emit 0x0f;
        _emit 0xc7;
        _emit 0x34;
        _emit 0x24;
        add esp,8
    }
   
    DWORD eflags = 0;
    __asm {
        pushfd
        pop [eflags]
    }
    _EFLAGS* uEflags = (_EFLAGS*)&eflags;
    if (uEflags->CF != 0)		//通过CF位判断是否调用成功
    {
        __printf(szout, "vmxon error!\r\n");

    }

    return TRUE;
}

int StopVirtualTechnology()
{
    __asm {
        _emit 0x0f;
        _emit 0x01;
        _emit 0xc4;
    }

    __kFree((unsigned long)g_VMXCPU.pVMXONRegion);		//释放内存

    return 0;
}
