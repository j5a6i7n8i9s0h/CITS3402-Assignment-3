# CITS3402 Project-1 2018
# Names:             Jeremiah Pinto, Jainish Pithadiya, 
# Student number(s):	21545883, 		21962504
 
PROJECT = matrix_mul
OBJ = matrix_mul.o

# will not compile on OSX
MPI = mpicc
FLAGS = -Wall -pedantic -Werror -fopenmp

$(PROJECT) : $(OBJ)
	$(MPI) $(FLAGS) -o $(PROJECT) $(OBJ)
%.o : %.c
	$(MPI) $(FLAGS) -c $<

clean :
	rm -f $(PROJECT) $(OBJ)
	