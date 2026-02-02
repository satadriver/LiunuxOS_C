
#include "queue.h"

using namespace MyRingQueue;

int MyRingQueue::init(RingQueue* q,char* base,int size) {
	q->size = size;
	q->base = base;
	q->head = base;
	q->tail = base;
	return TRUE;
}

int MyRingQueue::push(RingQueue* q, char* value) {

	*(char**)(q->tail) = value;

	int overflow = 0;

	char* next = q->tail + sizeof(char*);
	if (next >= q->base + (q->size) * sizeof(char*))
	{
		next = q->base;
	}

	if (next == q->head) {
		overflow = 1;
	}

	if (overflow == 0) {
		q->tail = next;
	}
	
	return overflow;
}

char * MyRingQueue::pop(RingQueue* q) {
	if (q->head == q->tail) {
		return 0;
	}

	char * v = *(char**)(q->head);

	q->head += sizeof(char*);
	if (q->head >= q->base + q->size * sizeof(char*) ) {

		q->head = q->base;
	}

	return v;
}