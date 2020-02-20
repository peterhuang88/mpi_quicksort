#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include "quicksort.h"
#include <iostream>
#include <mpi.h>

#define DEBUG 1

int intialize_arrays(int*,int);
void quicksort(int world_size, int world_rank, int num_elements, int* arr, MPI_Comm comm);


char processor_name[MPI_MAX_PROCESSOR_NAME];

int main(int argc, char** argv) {
    int* arr;
    int num_elements = 21;

    MPI_Init(NULL, NULL);
    //printf("hello\n");

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    // printf("%d\n", world_size);
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);

    // printf("Hello world from processor %s, rank %d out of %d processors\n", processor_name, world_rank, world_size);


    if (world_rank == 0) {
        // intialize array of random numbers
        arr = (int*)calloc(num_elements, sizeof(int));
        for (int i = 0; i < num_elements; i++) {
            arr[i] = rand() % num_elements;
        }

        #ifdef DEBUG
        for (int i = 0; i < num_elements; i++) {
            printf("%d ", arr[i]);
        }
        printf("\n");
        #endif
    }
    // int subdomain_size = num_elements / world_size;
    // MPI_Scatter(arr, subdomain_size, MPI_INT, )
    MPI_Comm comm = MPI_COMM_WORLD;
    quicksort(world_size, world_rank, num_elements, arr, comm);


    MPI_Finalize();
}

void quicksort(int world_size, int world_rank, int num_elements, int* arr, MPI_Comm comm) {
    int subdomain_start = num_elements / world_size * world_rank;
    int subdomain_size = num_elements / world_size;
    if (world_rank = world_size - 1) {
        //subdomain_size = num_elements % subdomain_size;
        subdomain_size += num_elements % world_size;
    }

    #ifdef DEBUG
    printf("Hello world from processor %s, rank %d out of %d processors with start: %d, and size %d\n",\
         processor_name, world_rank, world_size, subdomain_start, subdomain_size);
    #endif

    //int* sub_arr = (int*) calloc(sizeof(int) * subdomain_size);
    int* sub_arr = (int*) calloc(subdomain_size, sizeof(int));
    
    MPI_Scatter(arr, subdomain_size, MPI_INT, sub_arr, subdomain_size, MPI_INT, 0, comm);

    #ifdef DEBUG
    printf("Printing for %s, rank %d\n", processor_name, world_rank);
    //if (world_rank == 0) {
        for (int i = 0; i < subdomain_size; i++) {
            printf("%d ", sub_arr[i]);
        }
    //}
    printf("\n");
    #endif
    


}



int intialize_arrays(int* arr, int num_elements) {
    arr = (int*)calloc(num_elements, sizeof(int));
    for (int i = 0; i < num_elements; i++) {
        arr[i] = rand() % num_elements;
    }
}