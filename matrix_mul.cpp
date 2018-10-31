#include <iostream>
#include <fstream>
#include <string>
#include <omp.h>
// #include <mpi.h>

using namespace std;

// sort -k1 -k2 -n on files pre run

int num_process, id;

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

int getLineCount(ifstream &file)
{
	string line;
	int count_line = 0;

	while (getline(file, line))	count_line++;

	return count_line; 
}

void createMatrixFromFile(Matrix *matrix, ifstream &file)
{
	string buff;
	int row;
	int col; 
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

	file.clear();
	file.seekg(0, ios::beg);

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

void matrix_multiplication(Matrix *matrix_a, Matrix *matrix_b, Matrix *matrix_c)
{
	MatrixMarket *mat_a = matrix_a->market;
	MatrixMarket *mat_b = matrix_b->market;
	MatrixMarket *mat_c = (MatrixMarket*)malloc(matrix_a->count*sizeof(MatrixMarket)); 

	if(mat_c == NULL)
	{
		cerr << "Failed to allocate memory\n";
		exit(EXIT_FAILURE);
	}

	int apos, row_to_consider, insert = 0;

	#pragma omp parallel for private(apos) shared(insert, row_to_consider)
	for(apos = 0; apos < matrix_a->count; )
	{
		for(int bpos = 0; bpos < matrix_b->count; )
		{
			row_to_consider =  mat_a[apos].row;
			int col_to_consider = mat_b[bpos].col;
			int temp_a = apos;
			int temp_b = bpos;
			double val = 0.0;
			while(temp_a < matrix_a->count
				&& mat_a[temp_a].row == row_to_consider
				&& temp_b < matrix_b->count
				&& mat_b[temp_b].col == col_to_consider)
			{
				if(mat_a[temp_a].col < mat_b[temp_b].row) temp_a++;
				else if(mat_a[temp_a].col > mat_b[temp_b].row) temp_b++;
				else val += (mat_a[temp_a++].val * mat_b[temp_b++].val);
			}
			if(val != 0)
			{
				MatrixMarket temp; 
				bool newPos = true;

				for(int i = 0; i < insert; i++)
				{
					if(mat_c[i].col == col_to_consider && mat_c[i].row == row_to_consider)
					{
						newPos = false;
						mat_c[i].val += val;
					}
				}

				if(newPos)
				{
					temp.col = col_to_consider;
					temp.row = row_to_consider;
					temp.val = val;
					mat_c[insert++] = temp;
				}
			}
			while(bpos < matrix_b->count && mat_b[bpos].col == col_to_consider) bpos++;	
		}
		while(apos < matrix_a->count && mat_a[apos].row == row_to_consider) apos++;
	}

	matrix_c->market = mat_c;
	matrix_c->count = insert;
}

void close_files(ifstream &file1, ifstream &file2)
{
	file1.close();
	file2.close();
}

int main(int argc, char* argv[])
{
	if(argc != 3)
	{
		cerr << "The program requires three arguements\n";
		return EXIT_FAILURE;
	}

	ifstream file(argv[1]);
	ifstream file_2(argv[2]);

	bool f_exists = file && file_2;

	if(!f_exists)
	{
		cerr << "Invalid file(s) provided \n";

		close_files(file, file_2);

		return EXIT_FAILURE;
	}
	
	Matrix matrix_a;
	matrix_a.num_rows = 0;
	matrix_a.num_cols = 0;

	createMatrixFromFile(&matrix_a, file);

	cout << matrix_a.count << " valid entries \n";
	cout << matrix_a.num_rows << " x " << matrix_a.num_cols << " matrix created \n";

	Matrix matrix_b;
	matrix_b.num_rows = 0;
	matrix_b.num_cols = 0;

	createMatrixFromFile(&matrix_b, file_2);

	cout << matrix_b.count << " valid entries \n";
	cout << matrix_b.num_rows << " x " << matrix_b.num_cols << " matrix created \n";

	close_files(file, file_2);

	if(matrix_a.num_cols != matrix_b.num_rows)
	{
		cerr << "Invalid dimensions to multiply\n";
		return EXIT_FAILURE;
	}

	Matrix matrix_c;
	matrix_c.num_rows = matrix_a.num_rows;
	matrix_c.num_cols = matrix_b.num_cols;
	matrix_c.count = 0;

	// MPI_Init(&argc, &argv);
	// MPI_Comm_size(MPI_COMM_WORLD, &num_process);
	// MPI_Comm_rank(MPI_COMM_WORLD, &id);
	
	matrix_multiplication(&matrix_a, &matrix_b, &matrix_c);

	for(int i = 0; i < matrix_c.count; i++)
	{
		cout << matrix_c.market[i].row << " " << matrix_c.market[i].col << " " << matrix_c.market[i].val << "\n";
	}

	cout << matrix_c.num_rows << " x " << matrix_c.num_cols << "matrix created: " << matrix_c.count << " enties : multiplication success\n";

	// MPI_Finalize();
	
	return EXIT_SUCCESS;
}