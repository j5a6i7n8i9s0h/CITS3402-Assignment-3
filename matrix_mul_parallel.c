#include <stdio.h> 
#include <stdlib.h> 
#include <stdbool.h>
#include <omp.h>
#include <mpi.h>
#include <time.h>

#define MASTER 0
#define FROM_MASTER 1 /* setting a message type */
#define FROM_WORKER 2 /* setting a message type */
// sort -k1 -k2 -n on files pre run

typedef struct {
	int row; 
	int col;
	float val; 
} MatrixMarket;

typedef struct {
	int count; 
	MatrixMarket *market;
} Matrix;

int getLineCount(FILE *file)
{
	int lines = 0;
	while(!feof(file))
		lines = (fgetc(file) == '\n') ? lines+1 : lines; 
	return lines; 
}

void clearMatrixFromMemory(Matrix mat)
{
	free(mat.market);
}

void createMatrixFromFile(Matrix *matrix, FILE *file, int count)
{
	char buff[BUFSIZ];
	int row;
	int col; 
	float val;
	int i = 0;
	MatrixMarket temp;
	matrix->market = (MatrixMarket*)malloc(count*sizeof(MatrixMarket)); 
	if(matrix->market == NULL)
	{
		fprintf(stderr,"Failure to allocate memory\n");
		MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
		exit(EXIT_FAILURE);
	}
	matrix->count = count;
	fseek(file, 0, SEEK_SET);
	while(fgets(buff, BUFSIZ, file) != NULL)
	{
		sscanf(buff,"%d %d %f", &row, &col, &val);
		temp.row = row;
		temp.col = col;
		temp.val = val;
		matrix->market[i++] = temp;
	}
}

int main(int argc, char* argv[])
{
	int numtasks, taskid, numworkers, source, dest, mtype;
	int  rows, averow, extra, insert, offset,i, rc;
	MPI_Status status;
	if(argc != 3)
	{
		fprintf(stderr, "The program requires three arguements\n");
		return EXIT_FAILURE;
	}


	FILE *file, *file_2;
	bool f_exists;

	file = fopen(argv[1], "r");
	file_2 = fopen(argv[2],"r");
	f_exists = file != NULL && file_2 != NULL;

	if(!f_exists)
	{
		fprintf(stderr, "Invalid file(s) provided \n");
		fclose(file);
		fclose(file_2);
		return EXIT_FAILURE;

	}
	
	clock_t begin = clock();


	int file_1_lines = getLineCount(file);
	fseek(file, 0, SEEK_SET);
	int file_2_lines = getLineCount(file_2);
	fseek(file_2, 0, SEEK_SET);
	int max_lines = 4 * ((file_1_lines>file_2_lines)? file_1_lines: file_2_lines);

	MatrixMarket *mat_c = (MatrixMarket*)malloc(max_lines*sizeof(MatrixMarket)); 
	MatrixMarket *mat_c_temp = (MatrixMarket*)malloc(max_lines*sizeof(MatrixMarket)); 
	MatrixMarket *mat_a = (MatrixMarket*)malloc(file_1_lines*sizeof(MatrixMarket)); 
	MatrixMarket *mat_b = (MatrixMarket*)malloc(file_2_lines*sizeof(MatrixMarket)); 
	if(mat_c == NULL || mat_c_temp == NULL || mat_a==NULL || mat_b==NULL)
	{
		fprintf(stderr,"Failed to allocate memory\n");
		MPI_Abort(MPI_COMM_WORLD, rc);
		exit(EXIT_FAILURE);
	}
	// ______________________________________________ // 
	MPI_Init(&argc, &argv);
	const int nitems=3;
    int          blocklengths[3] = {1,1,1};
    MPI_Datatype types[3] = {MPI_INT, MPI_INT, MPI_FLOAT};
    MPI_Datatype mpi_matrix_market;
    MPI_Aint     offsets[3];

    offsets[0] = offsetof(MatrixMarket, row);
    offsets[1] = offsetof(MatrixMarket, col);
    offsets[2] = offsetof(MatrixMarket, val);

    MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_matrix_market);
    MPI_Type_commit(&mpi_matrix_market);

	MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
	MPI_Comm_rank(MPI_COMM_WORLD, &taskid);
	numworkers = numtasks - 1; 
	if (numtasks < 2 ) {
		printf("Need at least two MPI tasks. Quitting...\n");
		MPI_Abort(MPI_COMM_WORLD, rc);
		exit(EXIT_FAILURE);
	}
	if(taskid == MASTER)
	{
		Matrix matrix_a;
		Matrix matrix_b;
		Matrix matrix_c;
		matrix_c.count = 0;

		createMatrixFromFile(&matrix_a, file, file_1_lines);
		createMatrixFromFile(&matrix_b, file_2, file_2_lines);
		mat_a = matrix_a.market;
		mat_b = matrix_b.market;
		averow = matrix_a.count/numworkers;
		extra = matrix_a.count % numworkers;
		offset = 0 ;
		mtype = FROM_MASTER;
		for(dest = 1; dest<=numworkers; dest++)
		{
			rows = (dest <= extra)? averow +1: averow;
			MPI_Send(&offset, 1, MPI_INT, dest, mtype, MPI_COMM_WORLD); 
			MPI_Send(&rows, 1, MPI_INT, dest, mtype, MPI_COMM_WORLD);
			MPI_Send(&mat_a[offset], rows, mpi_matrix_market, dest, mtype, MPI_COMM_WORLD);
			MPI_Send(&mat_b[0], file_2_lines, mpi_matrix_market, dest, mtype, MPI_COMM_WORLD);
			offset +=rows;
		}
		mtype = FROM_WORKER;
		for(i=1; i <= numworkers; ++i)
		{
			source =i; 
			MPI_Recv(&offset, 1, MPI_INT, source, mtype, MPI_COMM_WORLD, &status);
			MPI_Recv(&rows, 1, MPI_INT, source, mtype, MPI_COMM_WORLD, &status);
			MPI_Recv(&insert, 1, MPI_INT, source, mtype, MPI_COMM_WORLD,&status);
			MPI_Recv(mat_c_temp, insert, mpi_matrix_market, source, mtype, MPI_COMM_WORLD, &status);
			if(insert>0){
				for(int in=0;in<insert;in++)
				{
					for(int items=0; items<max_lines;items++)
					{
						if(matrix_c.count==items)
						{
							mat_c[matrix_c.count++] = mat_c_temp[in];
							break;
						}
						if(mat_c_temp[in].row == mat_c[items].row && mat_c_temp[in].col == mat_c[items].col)
						{
							mat_c[items].val+=mat_c_temp[in].val;
							break;
						}
					}
				}
			}
			printf("Received results from task %d\n",source);
		}
		matrix_c.market=mat_c;
		for(i=0;i<matrix_c.count;i++)
		{
			printf("%d %d %f \n", matrix_c.market[i].row, matrix_c.market[i].col, matrix_c.market[i].val);
		}

		clock_t end = clock();
		printf("Time: %f ms \n", (float)(end-begin)*1000/CLOCKS_PER_SEC);
	}

	if(taskid > MASTER)
	{
		mtype = FROM_MASTER;
		MPI_Recv(&offset, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD, &status);
		MPI_Recv(&rows, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD, &status);
		MPI_Recv(mat_a, rows, mpi_matrix_market, MASTER, mtype, MPI_COMM_WORLD, &status);
		MPI_Recv(mat_b, file_2_lines, mpi_matrix_market, MASTER, mtype, MPI_COMM_WORLD, &status);
		int apos, row_to_consider;
		insert = 0; // number of elements to be inserted
		for(apos = 0; apos < rows; )
		{
			row_to_consider =  mat_a[apos].row;
			for(int bpos = 0; bpos < file_2_lines; )
			{
				int col_to_consider = mat_b[bpos].col;
				int temp_a = apos;
				int temp_b = bpos;
				float val = 0.0;
				while(temp_a < rows
					&& mat_a[temp_a].row == row_to_consider
					&& temp_b < file_2_lines
					&& mat_b[temp_b].col == col_to_consider)
				{
					if(mat_a[temp_a].col < mat_b[temp_b].row) temp_a++;
					else if(mat_a[temp_a].col > mat_b[temp_b].row) temp_b++;
					else val += mat_a[temp_a++].val * mat_b[temp_b++].val;
				}
				if(val != 0)
				{
					MatrixMarket temp; 
					bool newPos = true;
					for(int ij = 0; ij < insert; ij++)
					{
						if(mat_c_temp[ij].col == col_to_consider && mat_c_temp[ij].row == row_to_consider)
						{
							newPos = false;
							mat_c_temp[ij].val += val;
						}
					}
					if(newPos)
					{
						temp.col = col_to_consider;
						temp.row = row_to_consider;
						temp.val = val;
						mat_c_temp[insert++] = temp;
					}
				}
				while(bpos < file_2_lines && mat_b[bpos].col == col_to_consider) bpos++;	
			}
			while(apos < rows && mat_a[apos].row == row_to_consider) apos++;
		}
		mtype = FROM_WORKER;
		MPI_Send(&offset, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD);
		MPI_Send(&rows, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD);
		MPI_Send(&insert, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD);
		MPI_Send(&mat_c_temp[0], insert, mpi_matrix_market, MASTER, mtype, MPI_COMM_WORLD);
	}
	MPI_Type_free(&mpi_matrix_market); 
	MPI_Finalize();
	fclose(file);
	fclose(file_2);
	free(mat_c);
	free(mat_a);
	free(mat_b);
	free(mat_c_temp);
	return EXIT_SUCCESS;
}
