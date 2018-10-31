# CITS3402 Project-3 2018
# Names:             Jeremiah Pinto, Jainish Pithadiya, 
# Student number(s):	21545883, 		21962504
 
PROJECT = matrix_mul
OBJ = matrix_mul.o

#https://stackoverflow.com/questions/4058840/makefile-that-distincts-between-windows-and-unix-like-systems
ifdef SYSTEMROOT
	# For Windows machines (tested on UWA Lab Machines)
    G++ = g++ -std=c++14
else
    # For UNIX machines (tested on OSX)
    G++ = g++-8 -std=c++17
endif

MPI = mpicc -std=c++11
FLAGS = -Wall -pedantic -Werror -fopenmp

$(PROJECT) : $(OBJ)
	$(G++) $(FLAGS) -o $(PROJECT) $(OBJ)
%.o : %.cpp
	$(G++) $(FLAGS) -c $<

project: $(MPI) $(FLAGS) -o $(PROJECT) $(OBJ)
clean :
	rm -f $(PROJECT) $(OBJ)
	