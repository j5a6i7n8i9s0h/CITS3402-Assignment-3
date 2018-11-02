#include <stdio.h> 
#include <stdlib.h> 
#include <stdbool.h>
#include <omp.h>
#include <mpi.h>

#define MASTER 0
#define FROM_MASTER 1 /* setting a message type */
#define FROM_WORKER 2 /* setting a message type */
// sort -k1 -k2 -n on files pre run

typedef struct {
	int row; 
	int col;
	float val; 
} MatrixMarket;

int getLineCount(FILE *file)
{
	int lines = 0;
	while(!feof(file))
		lines = (fgetc(file) == '\n') ? lines+1 : lines; 
	return lines; 
}

void createMatrixFromFile(MatrixMarket *matrix, FILE *file, int count)
{
	char buff[BUFSIZ];
	int row;
	int col; 
	float val;
	int i = 0;
	MatrixMarket temp;
	while(fgets(buff, BUFSIZ, file) != NULL)
	{
		sscanf(buff,"%d %d %f", &row, &col, &val);
		temp.row = row;
		temp.col = col;
		temp.val = val;
		matrix[i++] = temp;
	}
}

int main(int argc, char* argv[])
{
	int numtasks, taskid, numworkers, source, dest, mtype, rows, averow, extra, insert, offset,i, rc;
	MPI_Status status;
	if(argc != 3)
	{
		fprintf(stderr, "The program requires atleast three arguements\n");
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

	int file_1_lines = getLineCount(file);
	fseek(file, 0, SEEK_SET);
	int file_2_lines = getLineCount(file_2);
	fseek(file_2, 0, SEEK_SET);
	int max_lines = 2 * ((file_1_lines>file_2_lines)? file_1_lines: file_2_lines); // at worst resulting matrix would have 2x the amount of valid entries (upper bound)

	MatrixMarket *MATRIX_C = (MatrixMarket*)malloc(max_lines*sizeof(MatrixMarket)); // The 
	MatrixMarket *MATRIX_C__BUFFER = (MatrixMarket*)malloc(max_lines*sizeof(MatrixMarket)); // To extrapolate worker data and update info at master 
	MatrixMarket *MATRIX_A;
	MatrixMarket *MATRIX_B;
	if(MATRIX_C == NULL || MATRIX_C__BUFFER == NULL)
	{
		fprintf(stderr,"Failed to allocate memory for resultant matrix\n");
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
		printf("Master doing stuff\n");
		int MATRIX_C_COUNT = 0;

		createMatrixFromFile(MATRIX_A, file, file_1_lines);
		createMatrixFromFile(MATRIX_A, file_2, file_2_lines);
		// create matrix a and b
		averow = file_1_lines/numworkers;
		extra = file_1_lines % numworkers;
		offset = 0;
		mtype = FROM_MASTER;
		for(dest = 1; dest<=numworkers; dest++)
		{
			rows = (dest <= extra)? averow +1: averow;
			MPI_Send(&offset, 1, MPI_INT, dest, mtype, MPI_COMM_WORLD); 
			MPI_Send(&rows, 1, MPI_INT, dest, mtype, MPI_COMM_WORLD);
			MPI_Send(&MATRIX_A[offset], rows, mpi_matrix_market, dest, mtype, MPI_COMM_WORLD);
			MPI_Send(&MATRIX_B[0], file_2_lines, mpi_matrix_market, dest, mtype, MPI_COMM_WORLD);
			offset += rows;
		}
		mtype = FROM_WORKER;
		for(i=1; i <= numworkers; ++i)
		{
			source = i; 
			MPI_Recv(&offset, 1, MPI_INT, source, mtype, MPI_COMM_WORLD, &status);
			MPI_Recv(&rows, 1, MPI_INT, source, mtype, MPI_COMM_WORLD, &status);
			MPI_Recv(&insert, 1, MPI_INT, source, mtype, MPI_COMM_WORLD,&status);
			MPI_Recv(MATRIX_C__BUFFER, insert, mpi_matrix_market, source, mtype, MPI_COMM_WORLD, &status);
			if(insert>0){
				for(int in=0;in<insert;in++)
				{
					for(int items=0; items<max_lines;items++)
					{
						if(MATRIX_C_COUNT==items)
						{
							MATRIX_C[MATRIX_C_COUNT++] = MATRIX_C__BUFFER[in];
							break;
						}
						if(MATRIX_C__BUFFER[in].row == MATRIX_C[items].row && MATRIX_C__BUFFER[in].col == MATRIX_C[items].col)
						{
							MATRIX_C[items].val+=MATRIX_C__BUFFER[in].val;
							break;
						}
					}
				}
			}
			printf("Received results from task %d\n",source);
		}
		for(int lp=0;lp<MATRIX_C_COUNT;lp++)
		{
			printf("%f %d %d market \n",MATRIX_C[lp].val, MATRIX_C[lp].row, MATRIX_C[lp].col);
		}
		// free(MATRIX_A);
		// free(MATRIX_B);
		// free(MATRIX_C);
		// free(MATRIX_C__BUFFER);
		fclose(file);
		fclose(file_2);
		printf("TOTAL %d    total lines %d \n",MATRIX_C_COUNT, max_lines);
	}

	if(taskid > MASTER)
	{
		mtype = FROM_MASTER;
		MPI_Recv(&offset, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD, &status);
		MPI_Recv(&rows, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD, &status);
		MPI_Recv(MATRIX_A, rows, mpi_matrix_market, MASTER, mtype, MPI_COMM_WORLD, &status);
		MPI_Recv(MATRIX_B, file_2_lines, mpi_matrix_market, MASTER, mtype, MPI_COMM_WORLD, &status);
		int apos, row_to_consider;
		insert = 0;
		for(apos = 0; apos < rows; )
		{
			for(int bpos = 0; bpos < file_2_lines; )
			{
				row_to_consider =  MATRIX_A[apos].row;
				int col_to_consider = MATRIX_B[bpos].col;
				int temp_a = apos;
				int temp_b = bpos;
				float val = 0.0;
				while(temp_a < rows
					&& MATRIX_A[temp_a].row == row_to_consider
					&& temp_b < file_2_lines
					&& MATRIX_B[temp_b].col == col_to_consider)
				{
					if(MATRIX_A[temp_a].col < MATRIX_B[temp_b].row) temp_a++;
					else if(MATRIX_A[temp_a].col > MATRIX_B[temp_b].row) temp_b++;
					else val += MATRIX_A[temp_a++].val * MATRIX_B[temp_b++].val;
				}
				if(val != 0)
				{
					MatrixMarket temp; 
					bool newPos = true;
					for(int ij = 0; ij < insert; ij++)
					{
						if(MATRIX_C__BUFFER[ij].col == col_to_consider && MATRIX_C__BUFFER[ij].row == row_to_consider)
						{
							newPos = false;
							MATRIX_C__BUFFER[ij].val += val;
						}
					}
					if(newPos)
					{
						temp.col = col_to_consider;
						temp.row = row_to_consider;
						temp.val = val;
						MATRIX_C__BUFFER[insert++] = temp;
					}
				}
				while(bpos < file_2_lines && MATRIX_B[bpos].col == col_to_consider) bpos++;	
			}
			while(apos < rows && MATRIX_A[apos].row == row_to_consider) apos++;
		}
		mtype = FROM_WORKER;
		MPI_Send(&offset, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD);
		MPI_Send(&rows, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD);
		MPI_Send(&insert, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD);
		MPI_Send(&MATRIX_C__BUFFER[0], insert, mpi_matrix_market, MASTER, mtype, MPI_COMM_WORLD);
	}
	MPI_Type_free(&mpi_matrix_market); 
	MPI_Finalize();
	return EXIT_SUCCESS;
}