#include"stdafx.h"
#include<iostream>
#include<ctime>
#include"mpi.h"

#define _CRT_SECURE_NO_WARNINGS
typedef unsigned char byte;
using namespace std;


byte findAverage(byte* arr, int size) {
	double tmp = 0;
	for (int i = 0; i < size; i++) {
		tmp += arr[i];
	}
	return (byte)(tmp / (double)size);
}


byte findAverage(byte* arr, int* weigth, int size) {
	double tmp = 0;
	for (int i = 0; i < size; i++) {
		tmp += arr[i] * weigth[i];
	}
	return (byte)(tmp / (double)size);
}

void printPicturetMatrix(byte** arr, const int cols, const int rows) {
	for (int y = 0; y < rows; y++) {
		for (int x = 0; x < cols; x++) {
			if (arr[x][y] > 99)
				printf("%i ", arr[x][y]);
			else if (arr[x][y] < 10)
				printf("%i   ", arr[x][y]);
			else printf("%i  ", arr[x][y]);
		}
		printf("\n");
	}
}

byte** generateRandomPicture(const int cols, const int rows) {
	byte** array = new byte*[cols];
	for (int i = 0; i < cols; i++)
	{
		array[i] = new byte[rows];
		for (int j = 0; j < rows; j++)
		{
			array[i][j] = rand() % 255;
		}
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
		curr += cols;
	}
	return arr;
}

byte* correctContrast(byte* arr, int size, byte avg) {
	byte delta = avg / 10;
	for (int i = 0; i < size; i++)
	{
		if (arr[i] < delta) arr[i] = 0;
		else if (arr[i] >= 255 - delta) arr[i] = 255;
		else if (arr[i] < avg) arr[i] -= delta;
		else if (arr[i] > avg) arr[i] += delta;
	}
	return arr;
}

int main(int argc, char* argv[]) {
	byte** picture;
	int* offset;
	int* amount;
	byte *recvbuf;
	byte avg;
	int pictureSize[2];
	int columnNumber, rowNumber;
	int numproc, rank;
	double startTime, endTime;
	MPI_Status status;
	MPI_Request request;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numproc);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	amount = new int[numproc];
	offset = new int[numproc];
	

	if (rank == 0) {

		printf("Enter number of colums: ");
		cin >> columnNumber;

		printf("Enter number of rows: ");
		cin >> rowNumber;

		int matrixSize[] = { columnNumber,rowNumber };
		MPI_Bcast(matrixSize, 2, MPI_INT, 0, MPI_COMM_WORLD);


		srand(time(nullptr));
		picture = generateRandomPicture(columnNumber, rowNumber);
		printPicturetMatrix(picture, columnNumber, rowNumber);
		printf("\n");
		if (numproc == 1) {
			avg = 0;
			for (int i = 0; i < columnNumber; i++)
				avg += findAverage(picture[i], rowNumber) / columnNumber;
			for (int i = 0; i < columnNumber; i++)
				picture[i] = correctContrast(picture[i], rowNumber, avg);
			printPicturetMatrix(picture, columnNumber, rowNumber);
		MPI_Finalize();
		return 0;
		}
	

		amount = generateAmountArray(columnNumber, rowNumber, numproc);
		offset = generateOffsetArray(columnNumber, rowNumber, numproc, amount);
		startTime = MPI_Wtime();

		MPI_Bcast(amount, numproc, MPI_INT, 0, MPI_COMM_WORLD);
		MPI_Bcast(offset, numproc, MPI_INT, 0, MPI_COMM_WORLD);

		int calculatingRank = 1;
		for (int i = amount[0]; i < columnNumber; i++) {
				if (i == offset[calculatingRank] + amount[calculatingRank])	calculatingRank++;
				
			MPI_Isend(picture[i], rowNumber, MPI_BYTE, calculatingRank, i, MPI_COMM_WORLD, &request);
			MPI_Request_free(&request);
		}

		avg = 0;
		double tmp = 0;
		for (int i = 0; i < amount[0]; i++)
			tmp += findAverage(picture[i], rowNumber);
		avg = tmp / amount[0];

		int *recvCount = new int[numproc];
		byte *recvarr = new byte[numproc];
		int* recvdispl = new int[numproc];
		for (int j = 0; j < numproc; j++) {
			recvarr[j] = 0;
			recvdispl[j] = j;
			recvCount[j] = 1;
		}

		MPI_Gatherv(&avg, 1, MPI_BYTE, recvarr, recvCount, recvdispl, MPI_BYTE, 0, MPI_COMM_WORLD);
		delete[] recvCount, recvdispl;
		avg = findAverage(recvarr, numproc);
		delete[] recvarr;
		MPI_Bcast(&avg, 1, MPI_BYTE, 0, MPI_COMM_WORLD);
		

		for (int i = 0; i < amount[rank]; i++)
			picture[i] = correctContrast(picture[i], rowNumber, avg);

		int sendingRank = 1;
		for (int i = amount[0]; i < columnNumber; i++) {
			if (i == offset[sendingRank] + amount[sendingRank])	sendingRank++;
			MPI_Recv(picture[i], rowNumber, MPI_BYTE, sendingRank, i, MPI_COMM_WORLD, &status);
		}
		cout << "Corrected matrix " << endl << endl;
		printPicturetMatrix(picture, columnNumber, rowNumber);
		
	}
	else {
		MPI_Bcast(pictureSize, 2, MPI_INT, 0, MPI_COMM_WORLD);
		columnNumber = pictureSize[0];
		rowNumber = pictureSize[1];

		MPI_Bcast(amount, numproc, MPI_INT, 0, MPI_COMM_WORLD);
		MPI_Bcast(offset, numproc, MPI_INT, 0, MPI_COMM_WORLD);

		picture = new byte*[columnNumber];
		avg = 0;
		for (int i = 0; i < amount[rank] ; i++)
		{
			picture[i] = new byte[rowNumber];
			MPI_Recv(picture[i], rowNumber, MPI_BYTE, 0, i + offset[rank], MPI_COMM_WORLD,&status);
			avg += findAverage(picture[i], rowNumber) / amount[rank];
		}
		
		MPI_Gatherv(&avg, 1, MPI_BYTE, nullptr, nullptr, nullptr, MPI_BYTE, 0, MPI_COMM_WORLD);
		MPI_Bcast(&avg, 1, MPI_BYTE, 0, MPI_COMM_WORLD);

		for(int i = 0;i < amount[rank];i++){

			picture[i] = correctContrast(picture[i], rowNumber, avg);

			MPI_Isend(picture[i], rowNumber, MPI_BYTE, 0, offset[rank] + i, MPI_COMM_WORLD, &request);
			MPI_Request_free(&request);
		}
	}

	if (rank == 0) {
		endTime = MPI_Wtime();
		printf("\nProgram completed in %f \n", endTime - startTime);
	}
	delete[]  offset, amount,picture;

	MPI_Finalize();
	return 0;
}