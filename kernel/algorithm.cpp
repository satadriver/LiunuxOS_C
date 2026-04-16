
#include "algorithm.h"
#include "Utils.h"

void BubbleSort_ull(AlgorithmModel* arr, int count) {
	for (int i = count - 1; i > 0; i--) {
		for (int j = 0; j < i; j++) {
			double low = arr[j].fv;
			double high = arr[j + 1].fv;
			if (low > high) {
				double v = arr[j].fv;
				unsigned long long id = arr[j].id;
				arr[j].fv = arr[j + 1].fv;
				arr[j].id = arr[j + 1].id;
				arr[j + 1].fv = v;
				arr[j + 1].id = id;
			}
		}
	}

	char szout[256];
	for (int i = 0; i < count; i++) {
		//__printf(szout, "cpu:%d,active:%i64x\r\n",(int) arr[i].id, arr[i].v);
	}
}

void BubbleSort(unsigned int* arr, int count) {
	for (int i = count - 1; i > 0 ; i--) {
		for (int j = 0; j < i; j++) {
			unsigned int low = arr[j] & 0x00ffffff;
			unsigned int high = arr[j + 1] & 0x00ffffff;
			if (low > high) {
				unsigned int temp = arr[j];
				arr[j] = arr[j + 1];
				arr[j + 1] = temp;
			}
		}
	}
}

int partition(AlgorithmModel* data, int low, int high) {
	unsigned long long  pivot = data[low].v;
	unsigned long long  id = data[low].id;
	while (low < high)
	{
		while (low < high && data[high].v >= pivot) // 从右向左找第一个小于x的数
			high--;
		if (low < high)
			data[low++] = data[high];
		while (low < high && data[low].v < pivot) // 从左向右找第一个大于等于x的数
			low++;
		if (low < high)
			data[high--] = data[low];
	}
	data[low].v = pivot;
	data[low].id = id;
	return low;
}

void QuickSort(AlgorithmModel* s, int low, int high)
{
	if (low < high)
	{
		int pivot = partition(s, low, high);
		QuickSort(s, low, pivot - 1);
		QuickSort(s, pivot + 1, high);
	}
}




void swap(unsigned long* a, unsigned long* b) {
	unsigned long t = *a;
	*a = *b;
	*b = t;
}

int GetPivot(unsigned long data[], int left, int right) {
	if(right - left <= 1) {
		return 0xffffffff;
	}
	else if (right - left == 2)
	{
		if(data[left] > data[left + 1]) {
			
			unsigned long t = data[left];
			data[left] = data[left + 1];
			data[left + 1] = t;
		}
		else {
		}
		return 0xffffffff;
	}
	else if (right - left >= 3)
	{
		int min = 0;
		int max = 1;
		int mid = 0;
		if(data[0] > data[1]) {
			max = data[0];
			min = data[1];
		}
		else {
		}

		if (data[2] > data[max]) {
			mid = max;
			max = 2;

		}
		if (data[2] < data[min]) {
			mid = min;
			min = 2;
		}

		if (right - left == 3) {
			swap(data + min, data + mid);
			swap(data + mid, data + max);
			return 0xffffffff;
		}
		return mid;
	}
	else {

	}
	return 0;
}


int FastSort(int data[], int left, int right) {
	if (right - left <= 0)
	{
		return 0;
	}

	int low = left;
	int high = right;

	int mid = (low + high)/2;

	int v = data[mid];

	unsigned long tmp = 0;

	while (low < high)
	{
		while ((low < high) && (data[high] >= v)){
			high--;
		}

		while ((low < high) && (data[low] < v) ) {
			low++;
		}

		if (low < high) {
			tmp = data[high];
			data[high] = data[low];
			data[low] = tmp;
		}
	}

	FastSort(data, left, low - 1);
	FastSort(data, low , right);

	return 0;
}