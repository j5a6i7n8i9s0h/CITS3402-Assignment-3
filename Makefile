# CITS3402 Project-3 2018
# Names:             Jeremiah Pinto, Jainish Pithadiya, 
# Student number(s):	21545883, 		21962504
 
PROJECT = project
OBJ = project.o

# To be used on the cluster
MPI = mpic++
FLAGS = -std=c++11 -Wall -pedantic -Werror -fopenmp

$(PROJECT) : $(OBJ)
	$(MPI) $(FLAGS) -o $(PROJECT) $(OBJ)
%.o : %.cpp
	$(MPI) $(FLAGS) -c $<

clean :
	rm -f $(PROJECT) $(OBJ)
	