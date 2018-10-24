# CITS3402 Project-1 2018
# Names:             Jeremiah Pinto, Jainish Pithadiya, 
# Student number(s):	21545883, 		21962504
 
PROJECT = matrix_mul
OBJ = matrix_mul.o

#https://stackoverflow.com/questions/4058840/makefile-that-distincts-between-windows-and-unix-like-systems
ifdef SYSTEMROOT
	# For Windows machines (tested on UWA Lab Machines)
    GCC = gcc -std=c11
else
    # For UNIX machines (tested on OSX)
    GCC = gcc-8 -std=c11
endif

FLAGS = -Wall -pedantic -Werror -fopenmp

$(PROJECT) : $(OBJ)
	$(GCC) $(FLAGS) -o $(PROJECT) $(OBJ)
%.o : %.cpp
	$(CC) $(FLAGS) -c $<


clean :
	rm -f $(PROJECT) $(OBJ)
	