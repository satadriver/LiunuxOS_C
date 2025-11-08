#pragma once



extern "C" __declspec(dllexport) int __kKernelLeave64(unsigned long retaddr32, unsigned long regebp32);

extern "C" __declspec(dllexport) int __kKernelEntry64(unsigned long retaddr32, unsigned long ebp32);

extern "C" __declspec(dllexport) int  LiunuxOS64Entry(char* esptop, char* retaddr);

extern "C" __declspec(dllexport) int  LiunuxOS64Leave(int seg32, unsigned long retaddr32, char* gdt32, unsigned long rbp32);

