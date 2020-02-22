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

int cmpfunc(const void*, const void*);
int floatcmpfunc(const void*, const void*);

void quicksort(int num_elements, int* arr, MPI_Comm comm);
int find_split_point(int* arr, int size, float pivot);


char processor_name[MPI_MAX_PROCESSOR_NAME];

int cmpfunc (const void * a, const void * b) {
   return ( *(int*)a - *(int*)b );
}

int floatcmpfunc (const void * a, const void * b) {
   return ( *(float*)a - *(float*)b );
}

int main(int argc, char** argv) {
    int* arr;
    int num_elements = 20;

    MPI_Init(NULL, NULL);
    //printf("hello\n");

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    // printf("%d\n", world_size);
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);

    

    if (world_rank == 0) {
        // intialize array of random numbers
        arr = (int*)calloc(num_elements, sizeof(int));
        for (int i = 0; i < num_elements; i++) {
            arr[i] = rand() % num_elements;
        }

        //#ifdef DEBUG
        for (int i = 0; i < num_elements; i++) {
            printf("%d ", arr[i]);
        }
        printf("\n");
        //#endif
    }
    // int subdomain_size = num_elements / world_size;
    // MPI_Scatter(arr, subdomain_size, MPI_INT, )
    MPI_Comm comm = MPI_COMM_WORLD;
    printf("Hello world from processor %s, rank %d out of %d processors\n", processor_name, world_rank, world_size);

    quicksort(num_elements, arr, comm);

    MPI_Finalize();
}

void quicksort(int num_elements, int* arr, MPI_Comm comm) {
    int world_rank;
    MPI_Comm_rank(comm, &world_rank);
    int world_size;
    MPI_Comm_size(comm, &world_size);

    if (world_size == 1) {
        printf("Rank %d is done!\n", world_rank);
        return;
    }

    //printf("Printing for %s, rank %d, size %d\n", processor_name, my_world_rank, my_world_size);
    // calculate subdomain size and start for each processor
    int subdomain_start = num_elements / world_size * world_rank;
    int subdomain_size = num_elements / world_size;
    
    // alloc subarray of main arr
    int* sub_arr = (int*) calloc(subdomain_size, sizeof(int));
    
    // create send counts and displacements for scatterv
    int* sendcounts = (int*) calloc(world_size, sizeof(int));
    int* displs = (int*) calloc(world_size, sizeof(int)); 
    int count_incrementer = 0;
    for (int i = 0; i < world_size; i++) {
        displs[i] = count_incrementer;
        if (i == world_size - 1) {
            subdomain_size += num_elements % world_size;
        }
        sendcounts[i] = subdomain_size;

        // printf("Rank %d, displ: %d, counts: %d\n", i, displs[i], sendcounts[i]);

        count_incrementer += subdomain_size;
    }
    
    // if the rank == 0, then divide up the array
    MPI_Scatterv(arr, sendcounts, displs, MPI_INT, sub_arr, subdomain_size, MPI_INT, 0, comm);

    // resetting subdomain_size because i override it when doing the sendcounts thing
    if (world_rank != world_size - 1) {
        subdomain_size = num_elements / world_size;
    }
    #ifdef DEBUG
    printf("Printing for %s, rank %d\n", processor_name, world_rank);
    for (int i = 0; i < subdomain_size; i++) {
        printf("%d ", sub_arr[i]);
    }
    printf("\n");
    #endif

    // qsort the array and find the median of each subarray
    qsort(sub_arr, subdomain_size, sizeof(int), cmpfunc);
    float median = 0;
    if (subdomain_size % 2 == 0) {
        int num1 = sub_arr[subdomain_size / 2 - 1];
        int num2 = sub_arr[subdomain_size / 2];
        median = ((float)num1 + (float)num2) / 2; // technically this isn't right but what the heck
    } else {
        median = (float)sub_arr[subdomain_size / 2];
    }

    #ifdef DEBUG
    printf("After local sort for %s, rank %d, median: %f\n", processor_name, world_rank, median);
    for (int i = 0; i < subdomain_size; i++) {
        printf("%d ", sub_arr[i]);
    }
    printf("\n");
    #endif

    // allgather and calculate the median of medians
    float* medians = (float*) calloc(world_size, sizeof(float));
    MPI_Allgather(&median, 1, MPI_FLOAT, medians, 1, MPI_FLOAT, comm);
    qsort(medians, world_size, sizeof(float), floatcmpfunc);
    float mom = 0;
    if (world_size % 2 == 0) {
        float num1 = medians[world_size / 2 - 1];
        float num2 = medians[world_size / 2];
        mom = (num1 + num2) / 2; // technically this isn't right but what the heck
    } else {
        mom = medians[world_size / 2];
    }

    #ifdef DEBUG
    printf("After allgather for rank %d, mom: %.1f\n", world_rank, mom);
    for (int i = 0; i < world_size; i++) {
        printf("%.1f ", medians[i]);
    }
    printf("\n");
    #endif

    // find index of split
    int split_ind = find_split_point(sub_arr, num_elements, mom);

    #ifdef DEBUG
    printf("Split ind for rank %d is %d\n", world_rank, split_ind);
    #endif

    MPI_Barrier(comm);
    printf("-------------------------------------\n");

    // exchange halves
    MPI_Status status;
    int num_recv_elements;
    int* recv_arr;
    if (world_rank < world_size / 2) {
    // if (world_rank == 0) {
        // send elements
        MPI_Send(sub_arr + split_ind + 1, subdomain_size - 1 - split_ind, MPI_INT, world_rank + world_size / 2, 0, comm);

        // probe, alloc array, then receive
        MPI_Probe(world_rank + world_size / 2, 1, comm, &status);
        MPI_Get_count(&status, MPI_INT, &num_recv_elements);
        recv_arr = (int*) calloc(num_recv_elements, sizeof(int));
        MPI_Recv(recv_arr, num_recv_elements, MPI_INT, world_rank + world_size / 2, 1, comm, MPI_STATUS_IGNORE);

        #ifdef DEBUG
        int rec_rank = world_rank + world_size / 2;
        printf("Receiving from %d the following array:\n", rec_rank);
        for (int i = 0; i < num_recv_elements; i++) {
            printf("%d ", recv_arr[i]);
        }
        printf("\n");
        #endif

    } else {//if (world_rank == 2){
        // receive first, then send, idk why but it works like that
        MPI_Probe(world_rank - world_size / 2, 0, comm, &status);
        MPI_Get_count(&status, MPI_INT, &num_recv_elements);
        recv_arr = (int*) calloc(num_recv_elements, sizeof(int));
        MPI_Recv(recv_arr, num_recv_elements, MPI_INT, world_rank - world_size / 2, 0, comm, MPI_STATUS_IGNORE);
        MPI_Send(sub_arr, split_ind + 1, MPI_INT, world_rank - world_size / 2, 1, comm);

        #ifdef DEBUG
        int rec_rank = world_rank - world_size / 2;
        printf("Receiving from %d the following array:\n", rec_rank);
        for (int i = 0; i < num_recv_elements; i++) {
            printf("%d ", recv_arr[i]);
        }
        printf("\n");
        #endif
    }

    // combine the 2 arrays
    int* new_sub_arr;

    if (world_rank < world_size / 2) {
    // if (world_rank == 0) {
        int new_len = split_ind + 1 + num_recv_elements;
        new_sub_arr = (int*) calloc(new_len, sizeof(int));
        memcpy(new_sub_arr, sub_arr, split_ind + 1 * sizeof(int));
        memcpy(new_sub_arr + split_ind + 1, recv_arr, num_recv_elements * sizeof(int));

        #ifdef DEBUG
        printf("After memcopy from rank %d, num elements saved: %d, num new elements: %d \n", world_rank, split_ind + 1, num_recv_elements);
        for (int i = 0; i < new_len; i++) {
            printf("%d ", new_sub_arr[i]);
        }
        printf("\n");
        #endif
    } else {
        printf("not yet!\n");
    }


}

int find_split_point(int* arr, int size, float pivot) {
    //int world_rank;
    //MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    int i = 0;
    for (i = 0; i < size; i++) {
        if (arr[i] > pivot) {
            return i-1; // we return i-1 because it plays nicer if we 
                        // get to the end of the array and everything is <= pivot
        }
    }

    return i-1;
}