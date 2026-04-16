#pragma once


#pragma pack(1)


struct AlgorithmModel {
	union {
		unsigned long long v;
		double fv;
	};
	
	unsigned long long id;
};


#pragma pack()

void swap(unsigned long* a, unsigned long* b);

void BubbleSort(unsigned int* arr, int count);

void BubbleSort_ull(AlgorithmModel* arr, int count);

void QuickSort(AlgorithmModel* s, int low, int high);

int fastSort(int data[], int left, int right);

void quickSort(int s[], int low, int high);