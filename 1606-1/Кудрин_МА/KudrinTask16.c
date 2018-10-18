#include<mpi.h>
#include<iostream>

using namespace std;

int findMaxInColumn(int* arr,int height) {
	int max;
	max = arr[0];
	for (int i = 1; i < height; i++){
		if (max < arr[i])
		max = arr[i];
	}
	return max;
}

int* findMaxforEachColumn(int* arr,int width, int height){
	int* max = new int [width];
	for(int i = 0; i < width; i++){
		max[i] = arr[i*height];
	}
	for(int i = 0; i < width; i++){
		for (int j = 0; j < height; j++){
			if (max[i] < arr[j + i*height])
				max[i] = arr[j + i*height];
		}
	}
	return max;	
}

void PrintMaximums(int* maxArr, int size) {
	for (int i = 0; i < size; i++)
		printf("Maximum for colum: %i is: %i \n",i + 1,maxArr[i]);
}

void printMatrix(int* arr, const int cols,const int rows) {
	for (int y = 0; y < rows; y++) {
		for (int x = 0; x < cols; x++) {
			if(arr[y + x*rows] > 99)
				printf("%i ",arr[y + x*rows]);
			else if(arr[y + x*rows] < 10)
				printf("%i   ",arr[y + x*rows]);
			else printf("%i  ",arr[y + x*rows]);
		}
		printf("\n");
	}
}

int* generateRandMatrix(const int cols, const int rows) {
	const int size = cols*rows;
	int* array = new int [size];
	
	for (int i = 0; i < size; i++) {
		array[i] = rand() % 1000 ;
	}

	return array;
}

int main(int argc, char* argv[]) {

	int* arr;
	int* max; 
	int numproc,rank; 
	
	MPI_Status status;
	MPI_Request request;
	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD,&numproc); // -   
	MPI_Comm_rank(MPI_COMM_WORLD,&rank); // 
	
	double startTime,endTime;

	if(rank == 0){
		int columnNumber;
		int rowNumber;
		printf("Enter number of rows: ");
		scanf("%i",&rowNumber);
		printf("Enter number of colums: ");
		scanf("%i",&columnNumber);
		srand(time(nullptr));
		int* arr = generateRandMatrix(columnNumber,rowNumber);	
		printMatrix(arr, columnNumber, rowNumber);
		printf("\n");
		max = new int[columnNumber];
		
		if(numproc == 1){
			startTime=MPI_Wtime();
			max = findMaxforEachColumn(arr,columnNumber,rowNumber);
			PrintMaximums(max,columnNumber);
			endTime=MPI_Wtime();
			printf("\n Program completed in %f \n",endTime - startTime);
		return 0 ; 
		}
		
		int matrixSize[] = {columnNumber,rowNumber};
		MPI_Bcast(matrixSize, 2, MPI_INT, 0, MPI_COMM_WORLD);
		
		startTime=MPI_Wtime();
		
		for(int i = 0; i < columnNumber;i++){
			MPI_Isend(arr + rowNumber*i,rowNumber,MPI_INT,i % (numproc - 1) + 1,i,MPI_COMM_WORLD, &request);
			MPI_Request_free(&request);
		}
		
		for(int i = 0;i < columnNumber;i++){
			MPI_Recv(max + i,1,MPI_INT,i % (numproc - 1) + 1,i ,MPI_COMM_WORLD, &status);			
		}
		
		endTime = MPI_Wtime();
		
		printf("\n");							
		PrintMaximums(max,columnNumber);
		printf("\n Program completed in %f \n",endTime - startTime);	
		
							
	}  else {
		int matrixSize[2];	
		MPI_Bcast(matrixSize, 2, MPI_INT, 0, MPI_COMM_WORLD);
		int columnNumber = matrixSize[0];
		int rowNumber = matrixSize[1];
		
		for(int i = 0;i <= columnNumber  / (numproc - 1) ;i++){
			int column = rank + i * ( numproc - 1) - 1 ;
			if(column < columnNumber){		
				arr = new int[rowNumber];
				MPI_Recv(arr, rowNumber, MPI_INT, 0, column, MPI_COMM_WORLD, &status);
				max = new int[1];
				max[0] = findMaxInColumn(arr,rowNumber);
				MPI_Send(max,1,MPI_INT, 0,column,MPI_COMM_WORLD);
			}
		}
	}


MPI_Finalize();

return 0;
}