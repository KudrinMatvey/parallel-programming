#include"stdafx.h"
#include<iostream>
#include<ctime>
#include"mpi.h"

#define _CRT_SECURE_NO_WARNINGS
using namespace std;

int* generateOffsetArray(int* amountArray, const int numproc) {
	int* arr = new int[numproc];
	int currOffset = 0;
	arr[0] = 0;
	for (int i = 1; i < numproc; i++) {
		currOffset = arr[i - 1];
		arr[i] = currOffset + amountArray[i - 1];
	}
	return arr;
}

int* generateAmountArray(const int elementsNumber, const int numproc) {
	int curr = 0;
	int* arr = new int[numproc];
	for (int j = 0; j < numproc; j++)
		arr[j] = elementsNumber / numproc;
	arr[numproc - 1] += elementsNumber % numproc;
	return arr;
}

int* generateRandomArray(const int elementsNumber, const int min, const int max) {
	int* arr = new int[elementsNumber];
	for (int j = 0; j < elementsNumber; j++)
		arr[j] = rand() % (max - min);
	return arr;
}

int* sort(int* array, int size) {
	int* res = new int[size];
	for (int i = 0; i < size; i++) {
		res[i] = 0;
	}

	if (size == 1) {
		return array;
	} else	if (size == 2) {
		
		if (array[0] > array[1]) {
			res[0] = array[1];
			res[1] = array[0];
		}
		else {
			res[0] = array[0];
			res[1] = array[1];
		}

	}
	else {
		int* sz = generateAmountArray(size, 2);
		int size1 = sz[0];
		int size2 = sz[1];
		delete[] sz;

		int* arr1 = new int[size1];
		int* arr2 = new int[size2];

		for (int i = 0; i < size1; i++) {
			arr1[i] = array[i];
			arr2[i] = array[i + size1];
		}
		if (size1 != size2) {
			arr2[size2 - 1] = array[size - 1];
		}

		arr1 = sort(arr1, size1);
		arr2 = sort(arr2, size2);

		int c1 = 0;
		int c2 = 0;
		int i = 0;
		while (c1 + c2 < size) {
			if (c1 < size1&& c2 < size2) {
				if (arr1[c1] < arr2[c2])
				{
					res[i++] = arr1[c1++];
				} else if (arr1[c1] > arr2[c2])
				{
					res[i++] = arr2[c2++];
				}
				else if (arr1[c1] == arr2[c2]) {
					res[i++] = arr1[c1++];
					res[i++] = arr2[c2++];
				}
			}
			else if(c1 == size1){
				while (c2 < size2)
				{
					res[i++] = arr2[c2++];
				}
			}
			else if (c2 == size2) {
				while (c1 < size1)
				{
					res[i++] = arr1[c1++];
				}
			}

		}
	} return res;
}

int* combineArrays(int* array, int* amount, int* offset, int arraySize, int numproc) {
	int * res = new int[arraySize];
	for (int i = 0; i < arraySize; i++)
	{
		res[i] = 0;
	}
	int* counters = new int[numproc];
	for (int i = 0; i < numproc; i++)
	{
		counters[i] = 0;
	}
	int i = 0;
	while (i < arraySize) {
		int currmin = 100000;
		int currrank = 0;
		for (int j = 0; j < numproc; j++)
		{
			if (counters[j] < amount[j]) {
				if (array[offset[j] + counters[j]] < currmin) {
					currmin = array[offset[j] + counters[j]];
					currrank = j;
				}
			}
		}
		res[i++] = currmin;
		counters[currrank]++;
	}
	return res;

}


int main(int argc, char* argv[]) {
	int* array;
	int* amountArray;
	int* offsetArray;
	int* recvbuf;
	int numproc, rank;

	double starttime, endtime;
	int elementsNumber;
	MPI_Status status;
	MPI_Request request;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numproc);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if (rank == 0) {

		printf("Enter number of elements: ");
		cin >> elementsNumber;
		int a, b;
		int* copy;
		copy = new int[elementsNumber];
		printf("Enter min: ");
		cin >> a;
		printf("Enter max: ");
		cin >> b;
		printf("Generated array: ");
		srand(time(nullptr));
		array = generateRandomArray(elementsNumber, a, b);

		for (int i = 0; i < elementsNumber ; i++)
		{
			copy[i] = array[i];
			printf("%i ", array[i]);
		}
		cout << endl;

		amountArray = generateAmountArray(elementsNumber, numproc);
		offsetArray = generateOffsetArray(amountArray, numproc);

		starttime = MPI_Wtime();
		MPI_Bcast(amountArray, numproc, MPI_INT, 0, MPI_COMM_WORLD);
		MPI_Bcast(offsetArray, numproc, MPI_INT, 0, MPI_COMM_WORLD);
		recvbuf = new int[amountArray[0]];

		MPI_Scatterv(array, amountArray, offsetArray, MPI_INT, recvbuf, amountArray[0], MPI_INT, 0, MPI_COMM_WORLD);
		
		recvbuf = sort(recvbuf, amountArray[0]);


		MPI_Gatherv(recvbuf, amountArray[0], MPI_INT, array,amountArray,offsetArray,MPI_INT, 0, MPI_COMM_WORLD);
		array = combineArrays(array, amountArray, offsetArray, elementsNumber, numproc);
		endtime = MPI_Wtime();

		printf("Sorted: ");
		for (int i = 0; i < elementsNumber; i++) {
			printf("%i ", array[i]);
		}

		printf("\nCompleted in %d", endtime - starttime);
		starttime = MPI_Wtime();		
		array = sort(copy, elementsNumber);
		endtime = MPI_Wtime();

		printf("\nSorted linear: ");
		for (int i = 0; i < elementsNumber; i++) {
			printf("%i ", array[i]);
		}
		printf("\nCompleted in %d", endtime - starttime);
	}
	else {
		amountArray = new int[numproc];
		offsetArray = new int[numproc];

		MPI_Bcast(amountArray, numproc, MPI_INT, 0, MPI_COMM_WORLD);
		MPI_Bcast(offsetArray, numproc, MPI_INT, 0, MPI_COMM_WORLD);
		recvbuf = new int[amountArray[rank]];
		array = new int[amountArray[rank]];

		MPI_Scatterv(array, amountArray, offsetArray, MPI_INT, recvbuf, amountArray[rank], MPI_INT, 0, MPI_COMM_WORLD);
		recvbuf = sort(recvbuf, amountArray[rank]);
	
		MPI_Gatherv(recvbuf, amountArray[rank], MPI_INT, nullptr, nullptr, nullptr, MPI_INT, 0, MPI_COMM_WORLD);
	}
	MPI_Finalize();
	return 0;
}