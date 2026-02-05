#pragma once

#include "def.h"



#pragma pack(1)

typedef struct {
	char * head;
	char * tail;
	char * base;
	int size;
}RingQueue;



typedef struct {
	int valid;
	char data[64];
}RingQueueData;

#pragma pack()


namespace MyRingQueue{

	int init(RingQueue* q, char* base, int size);

	int push(RingQueue* q, char* value,int size);

	char* pop(RingQueue* q);

	int insert(RingQueue* q, char* v,int size);
	int remove(RingQueue* q, char* v,int size);
}