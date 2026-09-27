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

void BubbleSort(AlgorithmModel* arr, int count);

void BubbleSortd(AlgorithmModel* arr, int count);

void QuickSort(AlgorithmModel* s, int low, int high);

