
#include "queue.h"


int initQueye(CycleQueue * q,char* base,int size) {
	q->size = size;
	q->base = base;
	q->head = base;
	q->tail = base;
	return TRUE;
}

int inQueue(CycleQueue* q, char* value) {

	char* tmp = q->tail + 1;
	if (tmp  >= q->base + q->size) {
		tmp = q->base;
	}
	if (tmp == q->head) {
		return 0;
	}

	*(char**)(q->tail) = value;

	q->tail++;

	if (q->tail  >= q->base + q->size) {
		
		q->tail = q->base;
	}
	return TRUE;
}

char * outQueue(CycleQueue* q) {
	if (q->head == q->tail) {
		return 0;
	}

	char * v = *(char**)(q->head);

	q->head++;
	if (q->head >= q->base + q->size) {

		q->head = q->base;
	}
	return v;
}