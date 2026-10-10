#pragma once

#include "def.h"

#include "deepLearning.h"


extern "C" __declspec(dllexport) int g_dl_proc_tag;


#pragma pack(1)



#pragma pack()



void DlTestProcess(int tag);

extern "C" __declspec(dllexport) int DlRNNTraining(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);

extern "C" __declspec(dllexport) int DlMLPTraining(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param);
