#include <bits/stdc++.h>
#include <mpi.h>
#include <omp.h>

#define MASTER 0
#define FROM_MASTER 1 /* setting a message type */
#define FROM_WORKER 2 /* setting a message type */
// sort -k1 -k2 -n on files pre run

using namespace std;

typedef struct {
	int row;
	int col;
	double val; 
} MatrixMarket;

typedef struct {
	int num_rows; 
	int num_cols;
	int count; 
	MatrixMarket *market;
} Matrix;

// Function to close both matrix files
void close_files(ifstream &file1, ifstream &file2)
{
	file1.close();
	file2.close();
}

// Function to get the line count of matrix files
int getLineCount(ifstream &file)
{
	string line;
	int count_line = 0;

	while (getline(file, line))	count_line++;

	file.clear();
	file.seekg(0, ios::beg);

	return count_line; 
}

// Function to create a matrix from the file sent as a parameter
void createMatrixFromFile(Matrix *matrix, ifstream &file)
{
	string buff;
	int row, col; 
	double val;
	int i = 0;
	int count = getLineCount(file);
	MatrixMarket temp;
	matrix->market = (MatrixMarket*)malloc(count*sizeof(MatrixMarket)); 

	if(matrix->market == NULL)
	{
		cerr << "Failure to allocate memory\n";
		exit(EXIT_FAILURE);
	}

	matrix->count = count;

	if(file.is_open())
		while(file >> row >> col >> val)
			if (row != 0 && col != 0)
			{
				temp.row = row;
				temp.col = col;
				temp.val = val;

				matrix->num_cols = (matrix->num_cols < col) ? col : matrix->num_cols;
				matrix->num_rows = (matrix->num_rows < row) ? row : matrix->num_rows;
				matrix->market[i++] = temp;
			}
}

int main(int argc, char *argv[])
{
    if(argc != 3)
    {
        cerr << "The program requires three arguements\n";
        return EXIT_FAILURE;
    }

	int numtasks, taskid, numworkers, source, dest, mtype, avail_threads;
	int  rows, averow, extra, insert, offset, i, rc = 0;
	MPI_Status status;

    ifstream file_1(argv[1]);
	ifstream file_2(argv[2]);

	bool f_exists = file_1 && file_2;

	if(!f_exists)
	{
		cerr << "Invalid file(s) provided \n";
		close_files(file_1, file_2);
		return EXIT_FAILURE;
    }

	auto start = chrono::steady_clock::now();

	int file_1_lines = getLineCount(file_1);
	int file_2_lines = getLineCount(file_2);

	int max_lines = 4 * ((file_1_lines > file_2_lines) ? file_1_lines : file_2_lines);
	
	MatrixMarket *mat_a = (MatrixMarket*) malloc(file_1_lines * sizeof(MatrixMarket));
	MatrixMarket *mat_b = (MatrixMarket*) malloc(file_2_lines * sizeof(MatrixMarket));
	MatrixMarket *mat_c = (MatrixMarket*) malloc(max_lines * sizeof(MatrixMarket));
	MatrixMarket *mat_c_temp = (MatrixMarket*) malloc(max_lines * sizeof(MatrixMarket));

	if(mat_c == NULL || mat_c_temp == NULL || mat_a == NULL || mat_b == NULL)
	{
		cerr << "Failed to allocate memory\n";
		return EXIT_FAILURE;
	}

	MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &avail_threads);

	const int nitems = 3;
	int blocklengths[3] = { 1, 1, 1 };

    MPI_Datatype types[3] = { MPI_INT, MPI_INT, MPI_DOUBLE };
    MPI_Datatype mpi_matrix_market;
    MPI_Aint offsets[3];

    offsets[0] = offsetof(MatrixMarket, row);
    offsets[1] = offsetof(MatrixMarket, col);
    offsets[2] = offsetof(MatrixMarket, val);

	MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_matrix_market);
    MPI_Type_commit(&mpi_matrix_market);

	MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
	MPI_Comm_rank(MPI_COMM_WORLD, &taskid);

	numworkers = numtasks - 1; 
	if (numtasks < 2 )
	{
		cerr << "Need at least two MPI tasks. Quitting...\n";

		MPI_Abort(MPI_COMM_WORLD, rc);
		return EXIT_FAILURE;
	}

	// Master Node processes
	if(taskid == MASTER)
	{
		cerr << "Master doing stuff\n";
		Matrix matrix_a = { };
		Matrix matrix_b = { };
		Matrix matrix_c = { };

		createMatrixFromFile(&matrix_a, file_1);
		createMatrixFromFile(&matrix_b, file_2);

		mat_a = matrix_a.market;
		mat_b = matrix_b.market;
		averow = matrix_a.count / numworkers;
		extra = matrix_a.count % numworkers;
		offset = 0;
		mtype = FROM_MASTER;

		for(dest = 1; dest <= numworkers; dest++)
		{
			rows = (dest <= extra) ? averow+1 : averow;
			MPI_Send(&offset, 1, MPI_INT, dest, mtype, MPI_COMM_WORLD); 
			MPI_Send(&rows, 1, MPI_INT, dest, mtype, MPI_COMM_WORLD);
			MPI_Send(&mat_a[offset], rows, mpi_matrix_market, dest, mtype, MPI_COMM_WORLD);
			MPI_Send(&mat_b[0], file_2_lines, mpi_matrix_market, dest, mtype, MPI_COMM_WORLD);
			offset += rows;
		}

		mtype = FROM_WORKER;

		for(i = 1; i <= numworkers; i++)
		{
			source = i;

			// Recieve information from the worker nodes
			MPI_Recv(&offset, 1, MPI_INT, source, mtype, MPI_COMM_WORLD, &status);
			MPI_Recv(&rows, 1, MPI_INT, source, mtype, MPI_COMM_WORLD, &status);
			MPI_Recv(&insert, 1, MPI_INT, source, mtype, MPI_COMM_WORLD,&status);
			MPI_Recv(mat_c_temp, insert, mpi_matrix_market, source, mtype, MPI_COMM_WORLD, &status);

			if(insert > 0)
			{
				int in;
				for(in = 0; in < insert; in++)
				{
					for(int items = 0; items < max_lines; items++)
					{
						if(matrix_c.count == items)
						{
							mat_c[matrix_c.count++] = mat_c_temp[in];
							break;
						}

						if(mat_c_temp[in].row == mat_c[items].row && mat_c_temp[in].col == mat_c[items].col)
						{
							mat_c[items].val += mat_c_temp[in].val;
							break;
						}
					}
				}
			}
		}
		matrix_c.market = mat_c;

		for(int lp = 0; lp < matrix_c.count; lp++)
		{
			cout << matrix_c.market[lp].row << " " << matrix_c.market[lp].col;
			cout << " " << setprecision(10) << matrix_c.market[lp].val << "\n";
		}

		auto end = chrono::steady_clock::now();

		cout << "Time is: " << chrono::duration <double, milli> (end - start).count() << "ms\n";

	}

	// Worker (Slave) Node processes
	if(taskid > MASTER)
	{
		mtype = FROM_MASTER;

		// Recieve the required information from the master node
		MPI_Recv(&offset, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD, &status);
		MPI_Recv(&rows, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD, &status);
		MPI_Recv(mat_a, rows, mpi_matrix_market, MASTER, mtype, MPI_COMM_WORLD, &status);
		MPI_Recv(mat_b, file_2_lines, mpi_matrix_market, MASTER, mtype, MPI_COMM_WORLD, &status);

		int apos, row_to_consider;
		insert = 0;

		// Compute the matrix multiplication here
		#pragma omp parallel for default(shared) private(apos)
		for(apos = 1; apos <= rows; apos++)
		{
			apos--; // added this in as it is required that the omp for has an increment statement

			row_to_consider =  mat_a[apos].row;
			for(int bpos = 0; bpos < file_2_lines; )
			{
				int col_to_consider = mat_b[bpos].col;
				int temp_a = apos;
				int temp_b = bpos;
				double val = 0.0;

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

		// Send information back to the master node after computation
		MPI_Send(&offset, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD);
		MPI_Send(&rows, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD);
		MPI_Send(&insert, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD);
		MPI_Send(&mat_c_temp[0], insert, mpi_matrix_market, MASTER, mtype, MPI_COMM_WORLD);
	}
	MPI_Type_free(&mpi_matrix_market); 
	MPI_Finalize();

	close_files(file_1, file_2);

	free(mat_c);
	free(mat_a);
	free(mat_b);
	free(mat_c_temp);

    return EXIT_SUCCESS;
}