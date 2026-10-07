#pragma once

#include "def.h"

#include "deepLearning.h"



#pragma pack(1)



#pragma pack()


extern "C" __declspec(dllexport) int DlRNNTraining(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);


extern "C" __declspec(dllexport) int DlMLPTraining(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);
