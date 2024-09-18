#pragma once

#include "def.h"

#pragma pack(1)

typedef struct {
	char * head;
	char * tail;
	char * base;
	int size;
}CycleQueue;

#pragma pack()




int initQueye(CycleQueue* q, char* base, int size);

int inQueue(CycleQueue* q, char* value);

char* outQueue(CycleQueue* q);