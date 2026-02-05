
#include "queue.h"
#include "Utils.h"

using namespace MyRingQueue;

int g_queue_lock = 0;

int MyRingQueue::init(RingQueue* q,char* base,int size) {
	q->size = size;
	q->base = base;
	q->head = base;
	q->tail = base;
	return TRUE;
}

int MyRingQueue::push(RingQueue* q, char* value,int size) {

	__enterSpinlock(&g_queue_lock);

	RingQueueData* data = (RingQueueData*)(q->tail);

	__memcpy(data->data, value, size);
	data->valid = TRUE;

	int overflow = 0;

	char* next = q->tail + sizeof(RingQueueData);
	if (next >= q->base + (q->size) * sizeof(RingQueueData))
	{
		next = q->base;
	}

	if (next == q->head) {
		overflow = 1;
	}
	else {
		q->tail = next;
	}
	
	__leaveSpinlock(&g_queue_lock);

	return overflow;
}

char * MyRingQueue::pop(RingQueue* q) {
	char* v = 0;
	if (q->head == q->tail) {
		return v;
	}

	__enterSpinlock(&g_queue_lock);

	char* ptr = (char*) q->head;
	do {
		RingQueueData* data = (RingQueueData*)ptr;
		if (data->valid) {
			v = data->data;
			break;
		}

		ptr += sizeof(RingQueueData);
		if (ptr >= q->base + q->size * sizeof(RingQueueData) ) {
			ptr = q->base;
		}

	} while (ptr != (char*)q->tail);

	q->head = ptr;

	__leaveSpinlock(&g_queue_lock);
	return v;
}


//useless function
int MyRingQueue::insert(RingQueue* q, char* v,int size) {

	return 0;
}


int MyRingQueue::remove(RingQueue* q, char* v,int size) {

	int result = 0;

	__enterSpinlock(&g_queue_lock);

	char* ptr = q->tail;
	do {
		RingQueueData* data = (RingQueueData*)ptr;

		if (__memcmp(data->data,v,size) == 0 ) {
			__memset(data->data, 0, sizeof(data->data));
			data->valid = 0;
			result = TRUE;
			break;
		}

		ptr += sizeof(RingQueueData);
		if (ptr >= q->base + q->size * sizeof(RingQueueData)) {
			ptr = q->base;
		}
	} while (ptr != q->tail);

	__leaveSpinlock(&g_queue_lock);

	return result;
}