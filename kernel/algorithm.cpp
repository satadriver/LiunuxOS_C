
#include "algorithm.h"
#include "Utils.h"

void BubbleSort_ull(AlgorithmModel* arr, int count) {
	for (int i = count - 1; i > 0; i--) {
		for (int j = 0; j < i; j++) {
			unsigned long long  low = arr[j].v;
			unsigned long long high = arr[j + 1].v;
			if (low > high) {
				unsigned long long v = arr[j].v;
				unsigned long long id = arr[j].id;
				arr[j].v = arr[j + 1].v;
				arr[j].id = arr[j + 1].id;
				arr[j + 1].v = v;
				arr[j + 1].id = id;
			}
		}
	}

	char szout[256];
	for (int i = 0; i < count; i++) {
		//__printf(szout, "cpu:%d,active:%i64x\r\n",(int) arr[i].id, arr[i].v);
	}
}