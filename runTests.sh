# modify the value of nodes (max = 29) and ppn/cores (max = 12)
#PBS -m abe
#PBS -M 21545883@student.uwa.edu.au,21962504@student.uwa.edu.au
source /etc/bash.bashrc
make
syncCluster
#PBS -l nodes=2:ppn=2
mpirun pro 1138_bus.mtx 1138_bus_1.mtx
syncCluster
#PBS -l nodes=2:ppn=3
mpirun pro 1138_bus.mtx 1138_bus_1.mtx
syncCluster
#PBS -l nodes=2:ppn=4
mpirun pro 1138_bus.mtx 1138_bus_1.mtx
syncCluster
#PBS -l nodes=2:ppn=5
mpirun pro 1138_bus.mtx 1138_bus_1.mtx
syncCluster
#PBS -l nodes=3:ppn=2
mpirun pro 1138_bus.mtx 1138_bus_1.mtx
syncCluster
#PBS -l nodes=3:ppn=3
mpirun pro 1138_bus.mtx 1138_bus_1.mtx
syncCluster
#PBS -l nodes=3:ppn=4
mpirun pro 1138_bus.mtx 1138_bus_1.mtx
syncCluster
#PBS -l nodes=3:ppn=5
mpirun pro 1138_bus.mtx 1138_bus_1.mtx
syncCluster
#PBS -l nodes=4:ppn=2
mpirun pro 1138_bus.mtx 1138_bus_1.mtx
syncCluster
#PBS -l nodes=4:ppn=3
mpirun pro 1138_bus.mtx 1138_bus_1.mtx
syncCluster
#PBS -l nodes=4:ppn=4
mpirun pro 1138_bus.mtx 1138_bus_1.mtx
syncCluster
#PBS -l nodes=4:ppn=5
mpirun pro 1138_bus.mtx 1138_bus_1.mtx
syncCluster
#PBS -l nodes=5:ppn=2
mpirun pro 1138_bus.mtx 1138_bus_1.mtx
syncCluster
#PBS -l nodes=5:ppn=3
mpirun pro 1138_bus.mtx 1138_bus_1.mtx
syncCluster
#PBS -l nodes=5:ppn=3
mpirun pro 1138_bus.mtx 1138_bus_1.mtx
syncCluster
#PBS -l nodes=5:ppn=4
mpirun pro 1138_bus.mtx 1138_bus_1.mtx
syncCluster
#PBS -l nodes=5:ppn=5
#PBS -m abe
#PBS -M 21545883@student.uwa.edu.au,21962504@student.uwa.edu.au
mpirun pro 1138_bus.mtx 1138_bus_1.mtx
syncCluster