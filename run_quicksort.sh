# mpirun -npernode 1 --hostfile host_file2 ./quicksort
mpirun -np 16 --hostfile host_file ./quicksort $1