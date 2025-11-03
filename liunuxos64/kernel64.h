#pragma once



extern "C" __declspec(dllexport) int __kKernelLeave64(char* retaddr32, char* regebp32);

extern "C" __declspec(dllexport) int __kKernelEntry64(char* retaddr32, char* ebp32);

