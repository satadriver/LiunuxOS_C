#pragma once

#include "process.h"

int WaitOrKey(int s, int wid, int key);

#ifdef DLL_EXPORT

extern "C"  __declspec(dllexport) int __pausePid(int pid);

extern "C"  __declspec(dllexport) int __resumePid(int pid);

extern "C"  __declspec(dllexport) int __resumeTid(int tid);

extern "C"  __declspec(dllexport) int __pauseTid(int tid);

extern "C"  __declspec(dllexport) int __terminateByFileName(char* filename);

extern "C"  __declspec(dllexport) int __terminateByFuncName(char* funcname);

extern "C"  __declspec(dllexport) PROCESS_INFO* __findProcessByTid(int tid);

extern "C"  __declspec(dllexport) PROCESS_INFO* __findProcessByPid(int pid);

extern "C"  __declspec(dllexport) PROCESS_INFO* __findProcessFileName(char* filename);

extern "C"  __declspec(dllexport) PROCESS_INFO* __findProcessFuncName(const char* funcname);

#else


extern "C"  __declspec(dllimport) int __pausePid(int pid);

extern "C"  __declspec(dllimport) int __resumePid(int pid);

extern "C"  __declspec(dllimport) int __resumeTid(int tid);

extern "C"  __declspec(dllimport) int __pauseTid(int tid);

extern "C"  __declspec(dllimport) int __terminateByFileName(char* filename);

extern "C"  __declspec(dllimport) int __terminateByFuncName(char* funcname);

extern "C"  __declspec(dllimport) PROCESS_INFO* __findProcessByTid(int tid);

extern "C"  __declspec(dllimport) PROCESS_INFO* __findProcessByPid(int pid);

extern "C"  __declspec(dllimport) PROCESS_INFO* __findProcessFileName(char* filename);

extern "C"  __declspec(dllimport) PROCESS_INFO* __findProcessFuncName(const char* funcname);

#endif


//CF(bit 0) [Carry flag]   
//若算术操作产生的结果在最高有效位(most-significant bit)发生进位或借位则将其置1，反之清零。
//这个标志指示无符号整型运算的溢出状态，这个标志同样在多倍精度运算(multiple-precision arithmetic)中使用

//SF(bit 7) [Sign flag]   
//该标志被设置为有符号整型的最高有效位。(0指示结果为正，反之则为负) 

//OF(bit 11) [Overflow flag]   
//如果整型结果是较大的正数或较小的负数，并且无法匹配目的操作数时将该位置1，反之清零。这个标志为带符号整型运算指示溢出状态

//OF是有符号数运算结果的标志
//OF标志：这个标志有点复杂，其结果是CF标志和次最高位是否发生进位（如果进位是1，没进位是0）进行异或的结果
//OF只对有符号数运算有意义，CF对无符号数运算有意义
//MOV AX,858F
//SUB AX,7869

// IOPL是I/O保护机制中的关键之一，它位于EFLAGS寄存器的第12、13位。指令in、ins、out、outs、cli、sti只有在CPL<= IOPL时才能执行。
//这些指令被称为I/O敏感指令，如果特权级低的指令视图访问这些I/O敏感指令将会导致常规保护错误(#GP)
//可以改变IOPL的指令只有popfl和iret指令，但只有运行在特权级0的程序才能将其改变