#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include "quicksort.h"
#include <iostream>
#include <mpi.h>

int main(int argc, char** argv) {
    MPI_Init(NULL, NULL);
    //printf("hello\n");

    int world_size = MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    // printf("%d\n", world_size);
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);

    printf("Hello world from processor %s, rank %d out of %d processors\n", processor_name, world_rank, world_size);

    MPI_Finalize();
}