#include"stdafx.h"
#include<iostream>
#include<ctime>
#include"mpi.h"

#define _CRT_SECURE_NO_WARNINGS

using namespace std;

int findMaxInColumn(int* arr, int height) {
	int max;
	max = arr[0];
	for (int i = 1; i < height; i++) {
		if (max < arr[i])
			max = arr[i];
	}
	return max;
}

int* findMaxforEachColumn(int* arr, int width, int height) {
	int* max = new int[width];
	for (int i = 0; i < width; i++) {
		max[i] = arr[i*height];
	}
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			if (max[i] < arr[j + i * height])
				max[i] = arr[j + i * height];
		}
	}
	return max;
}

void PrintMaximums(int* maxArr, int size) {
	for (int i = 0; i < size; i++)
		printf("Maximum for colum: %i is: %i \n", i + 1, maxArr[i]);
}

void printMatrix(int* arr, const int cols, const int rows) {
	for (int y = 0; y < rows; y++) {
		for (int x = 0; x < cols; x++) {
			if (arr[y + x * rows] > 99)
				printf("%i ", arr[y + x * rows]);
			else if (arr[y + x * rows] < 10)
				printf("%i   ", arr[y + x * rows]);
			else printf("%i  ", arr[y + x * rows]);
		}
		printf("\n");
	}
}

int* generateRandMatrix(const int cols, const int rows, int a, int b) {
	const int size = cols * rows;
	int* array = new int[size];
	if (a >= b)
		b = a + 1;
	for (int i = 0; i < size; i++) {
		array[i] = rand() % (b - a) + a;
	}

	return array;
}

int* generateOffsetArray(const int cols, const int rows, int numproc, int* amountArray) {
	int* arr = new int[numproc];
	int currOffset = 0;
	arr[0] = 0;
	for (int i = 1; i < numproc; i++) {
		currOffset = arr[i - 1];
		arr[i] = currOffset + amountArray[i - 1];
	}
	return arr;
}

int* generateAmountArray(const int cols, const int rows, int numproc) {
	const int size = cols*rows;
	int curr = 0;
	int i = 0;
	int* arr = new int[numproc];
	for (int j = 0; j < numproc; j++)
		arr[j] = 0;
	while (curr < size) {
		arr[i++ % numproc]++;
		curr += rows;
	}
	return arr;
}

int main(int argc, char* argv[]) {
	int* arr;
	int* max;
	int* offset;
	int* amount;
	int columnNumber;
	int rowNumber;
	int *recvbuf;
	int matrixSize[2];

	int numproc, rank;
	double startTime, endTime;
	MPI_Status status;
	MPI_Request request;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numproc);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	amount = new int[numproc];
	offset = new int[numproc];

	MPI_Datatype columnType;

	if (rank == 0) {
		int a;
		int b;

		printf("Enter number of colums: ");
		cin >> columnNumber;

		printf("Enter number of rows: ");
		cin >> rowNumber;

		int matrixSize[] = { columnNumber,rowNumber };
		MPI_Bcast(matrixSize, 2, MPI_INT, 0, MPI_COMM_WORLD);

		printf("Enter min: ");
		cin >> a;
		printf("Enter max: ");
		cin >> b;

		srand(time(nullptr));
		arr = generateRandMatrix(columnNumber, rowNumber, a, b);
		printMatrix(arr, columnNumber, rowNumber);
		printf("\n");
		if (numproc == 1) {
			startTime = MPI_Wtime();
			max = findMaxforEachColumn(arr, columnNumber, rowNumber);
			PrintMaximums(max, columnNumber);
			endTime = MPI_Wtime();
			printf("\n Program completed in %f \n", endTime - startTime);


			delete[] arr;
			delete[] max;

			MPI_Finalize();
			return 0;
		}
		max = new int[columnNumber];
		amount = generateAmountArray(columnNumber, rowNumber, numproc);
		offset = generateOffsetArray(columnNumber, rowNumber, numproc, amount);
		startTime = MPI_Wtime();

		MPI_Bcast(amount, numproc, MPI_INT, 0, MPI_COMM_WORLD);
		MPI_Bcast(offset, numproc, MPI_INT, 0, MPI_COMM_WORLD);


		MPI_Type_vector(rowNumber, 1, 1, MPI_INT, &columnType);

		MPI_Type_commit(&columnType);
		recvbuf = new int[amount[0] * rowNumber];
		for (int j = 0; j < amount[0] * rowNumber; j++)
			recvbuf[j] = 0;

		MPI_Scatterv(arr, amount, offset, columnType, recvbuf, amount[rank], columnType, 0, MPI_COMM_WORLD);
		delete[] arr;
	}
	else {
		MPI_Bcast(matrixSize, 2, MPI_INT, 0, MPI_COMM_WORLD);
		columnNumber = matrixSize[0];
		rowNumber = matrixSize[1];

		MPI_Bcast(amount, numproc, MPI_INT, 0, MPI_COMM_WORLD);

		MPI_Bcast(offset, numproc, MPI_INT, 0, MPI_COMM_WORLD);

		recvbuf = new int[amount[rank] * rowNumber];
		for (int j = 0; j < amount[rank] * rowNumber; j++)
			recvbuf[j] = 0;


		MPI_Type_vector(rowNumber, 1, 1, MPI_INT, &columnType);
		MPI_Type_commit(&columnType);
		arr = new int[10];


		MPI_Scatterv(arr, amount, offset, columnType, recvbuf, amount[rank], columnType, 0, MPI_COMM_WORLD);
	}
	const int currAmount = amount[rank];
	max = new int[currAmount];
	for (int i = 0; i < currAmount; i++) {
		max[i] = findMaxInColumn(recvbuf + i*rowNumber, rowNumber);
	}
	int *result = new int[columnNumber];
	MPI_Gatherv(max, currAmount, MPI_INT,
		result, amount, offset, MPI_INT,
		0, MPI_COMM_WORLD);

	if (rank == 0) {
		PrintMaximums(result, columnNumber);
		endTime = MPI_Wtime();
		printf("\nProgram completed in %f \n", endTime - startTime);
	}
	delete[] max, offset, amount, recvbuf, result;

	MPI_Finalize();
	return 0;
}
