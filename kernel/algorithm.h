#pragma once


#pragma pack(1)


struct AlgorithmModel {
	unsigned long long v;
	unsigned long long id;
};


#pragma pack()


void BubbleSort_ull(AlgorithmModel* arr, int count);