# modify the value of nodes (max = 29) and ppn/cores (max = 12)
#PBS -l nodes=4:ppn=2
#PBS -m abe
#PBS -M 21545883@student.uwa.edu.au,21962504@student.uwa.edu.au
source /etc/bash.bashrc
mpirun pro 1138_bus.mtx 1138_bus_1.mtx