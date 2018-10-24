#include <stdio.h> 
#include <stdlib.h> 
#include <omp.h>
#include<stdbool.h>


typedef struct {
	int row; 
	int col;
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
		lines = fgetc(file)=='\n'? lines+1:lines; 
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
	{
		fprintf(stderr,"Failure to allocate memory\n");
		return EXIT_FAILURE;
	}
	matrix->count=count;
	fseek(file, 0, SEEK_SET);
	while(fgets(buff, BUFSIZ, file)!=NULL)
	{
		sscanf(buff,"%d %d %f", &row, &col, &val);
		matrix->num_cols=matrix->num_cols<col?col:matrix->num_cols;
		matrix->num_rows= matrix->num_rows<row?row:matrix->num_rows;
		temp.row = row;
		temp.col = col;
		temp.val = val;
		matrix->market[i++]=temp;
	}
	return EXIT_SUCCESS;
}

int matrix_multiplication(Matrix*matrix_a, Matrix*matrix_b, Matrix*matrix_c)
{
	MatrixMarket *mat_a = matrix_a->market;
	MatrixMarket *mat_b = matrix_b->market;
	MatrixMarket *mat_c = (MatrixMarket*)malloc(matrix_a->count*sizeof(MatrixMarket)); 
	if(mat_c==NULL)
	{
		fprintf(stderr,"Failed to allocate memory\n");
		return EXIT_FAILURE;
	}
	int insert=0;
	for(int apos=0;apos<matrix_a->count;)
	{
		int row_to_consider =  mat_a[apos].row;
		for(int bpos =0 ; bpos<matrix_b->count;)
		{
			int col_to_consider = mat_b[bpos].col;
			int temp_a = apos;
			int temp_b = bpos;

			float val = 0.0;
			while(temp_a < matrix_a->count && mat_a[temp_a].row == row_to_consider && temp_b < matrix_b->count && mat_b[temp_b].col == col_to_consider)
			{
				if(mat_a[temp_a].col < mat_b[temp_b].row) temp_a++;
				else if(mat_a[temp_a].col > mat_b[temp_b].row) temp_b++;
				else val += mat_a[temp_a++].val*mat_b[temp_b++].val;
			}
			if(val!=0)
			{
				MatrixMarket temp; 
				temp.col = col_to_consider;
				temp.row = row_to_consider;
				temp.val = val;
				mat_c[insert++] = temp;
			}
			while(bpos < matrix_b->count && mat_b[bpos].col == col_to_consider) bpos++;	
		}
		while(apos < matrix_a->count && mat_a[apos].row == row_to_consider) apos++;
	}
	matrix_c->market=mat_c;
	matrix_c->count=insert;
	return EXIT_SUCCESS;
}

int main(int argc, char* argv[])
{
	FILE*file;
	FILE*file_2;
	bool f_exists;
	if(argc==3)
	{
		file = fopen(argv[1], "r");
		file_2 = fopen(argv[2],"r");
		f_exists = file!=NULL && file_2!=NULL;
	}
	else if(argc==2)
	{
		file = fopen(argv[1], "r");
		f_exists = file!=NULL;
	}
	if(f_exists)
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

		Matrix matrix_c;
		if(matrix_a.num_cols!=matrix_b.num_rows)
		{
			fprintf(stderr, "Invalid dimensions to multiply\n");
			return EXIT_FAILURE;
		}
		matrix_c.num_rows = matrix_a.num_rows;
		matrix_c.num_cols = matrix_b.num_cols;
		//matrix_c.count = matrix_a.count;
		if(matrix_multiplication(&matrix_a,&matrix_b,&matrix_c)==EXIT_FAILURE)
		{	
			return EXIT_FAILURE;
		}
		for(int i=0;i<matrix_c.count;i++)
		{
			printf("%f \n",matrix_c.market[i].val);
		}
		printf("%d x %d matrix created \n", matrix_c.num_rows, matrix_c.num_cols);

	}
	else
	{
		fprintf(stderr, "Invalid file provided \n");
	}
	fclose(file);
	fclose(file_2);
	return 0;
}