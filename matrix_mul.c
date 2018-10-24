#include <stdio.h> 
#include <stdlib.h> 
#include <omp.h>
#include<stdbool.h>


typedef struct {
	int i; 
	int j;
	float val; 
}MatrixMarket;

typedef struct {
	int num_rows; 
	int num_cols;
	int count; 
	MatrixMarket*market;
} Matrix;

int getLineCount(FILE*file)
{
	int lines = 0;
	while(!feof(file))
	{
		lines = fgetc(file)=='\n'? lines+1:lines; 
	}
	return lines; 
}

int createMatrixFromFile(Matrix*matrix, FILE*file)
{
	char buff[BUFSIZ];
	int row;
	int col; 
	float val;
	int i=0;
	int count = getLineCount(file);
	MatrixMarket temp;
	matrix->market = (MatrixMarket*)malloc(count*sizeof(MatrixMarket)); 
	if(matrix->market==NULL)
		return EXIT_FAILURE;
	matrix->count=count;
	fseek(file, 0, SEEK_SET);
	while(fgets(buff, BUFSIZ, file)!=NULL)
	{
		sscanf(buff,"%d %d %f", &row, &col, &val);
		if(matrix->num_cols<col)
			matrix->num_cols=col;
		if(matrix->num_rows<row)
			matrix->num_rows=row;
		temp.i = row;
		temp.j = col;
		temp.val = val;
		matrix->market[i++]=temp;
	}
	return EXIT_SUCCESS;
}

int main(int argc, char* argv[])
{
	FILE*file;
	FILE*file_2;
	bool fexists;
	if(argc==3)
	{
		file = fopen(argv[1], "r");
		file_2 = fopen(argv[2],"r");
		fexists = file!=NULL && file_2!=NULL;
	}
	else if(argc==2)
	{
		file = fopen(argv[1], "r");
		fexists = file!=NULL;
	}
	if(fexists)
	{
		
		Matrix matrix_a;
		matrix_a.num_rows = 0;
		matrix_a.num_cols = 0;

		if(createMatrixFromFile(&matrix_a, file)==EXIT_FAILURE)
		{
			fprintf(stderr, "FAILED  \n");
			return EXIT_FAILURE;
		}
		for(int i=0;i<matrix_a.count;i++)
		{
			printf("%f \n",matrix_a.market[i].val);
		}
		printf("%d valid entries \n", matrix_a.count);
		printf("%d x %d matrix created \n", matrix_a.num_rows, matrix_a.num_cols);
		if(argc!=3)
			return EXIT_FAILURE;

		Matrix matrix_b;
		matrix_b.num_rows = 0;
		matrix_b.num_cols = 0;

		if(createMatrixFromFile(&matrix_b, file_2)==EXIT_FAILURE)
		{
			fprintf(stderr, "FAILED  \n");
			return EXIT_FAILURE;
		}

		for(int i=0;i<matrix_b.count;i++)
		{
			printf("%f \n",matrix_b.market[i].val);
		}
		printf("%d valid entries \n", matrix_b.count);
		printf("%d x %d matrix created \n", matrix_b.num_rows, matrix_b.num_cols);

		Matrix matrix_b;
		if(matrix_a.num_cols!=matrix_b.num_rows)
		{
			fprintf(stderr, "Invalid dimensions to multiply\n");
			return EXIT_FAILURE;
		}
	}
	else
	{
		fprintf(stderr, "Invalid file provided \n");
	}
	fclose(file);
	fclose(file_2);
}