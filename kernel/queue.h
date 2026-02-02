#pragma once

#include "def.h"



#pragma pack(1)

typedef struct {
	char * head;
	char * tail;
	char * base;
	int size;
}RingQueue;

#pragma pack()


namespace MyRingQueue{

	int init(RingQueue* q, char* base, int size);

	int push(RingQueue* q, char* value);

	char* pop(RingQueue* q);
}