#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include "quicksort.h"
#include <iostream>
#include <mpi.h>

// #define DEBUG 1

int cmpfunc(const void*, const void*);
int floatcmpfunc(const void*, const void*);

void quicksort(int, int*, MPI_Comm, int);
int find_split_point(int* arr, int size, float pivot);
void merge_arrays(int* old_arr, int* recv_arr, int* new_arr, int old_num, int recv_num);


char processor_name[MPI_MAX_PROCESSOR_NAME];

int cmpfunc (const void * a, const void * b) {
   return ( *(int*)a - *(int*)b );
}

int floatcmpfunc (const void * a, const void * b) {
   return ( *(float*)a - *(float*)b );
}

int main(int argc, char** argv) {
    int* arr;
    //int num_elements = 20;
    int num_elements = 100000000;

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

        #ifdef DEBUG
        for (int i = 0; i < num_elements; i++) {
            printf("%d ", arr[i]);
        }
        printf("\n");
        #endif
    }
    // int subdomain_size = num_elements / world_size;
    // MPI_Scatter(arr, subdomain_size, MPI_INT, )
    // MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Comm comm;
    MPI_Comm_dup(MPI_COMM_WORLD, &comm);
    printf("Hello world from processor %s, rank %d out of %d processors\n", processor_name, world_rank, world_size);

    // divide big array into sub_arrays
    // calculate subdomain size and start for each processor
    int subdomain_start = num_elements / world_size * world_rank;
    int subdomain_size = num_elements / world_size;

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

    // resetting subdomain_size because i override it when doing the sendcounts thing
    if (world_rank != world_size - 1) {
        subdomain_size = num_elements / world_size;
    }

    // alloc subarray of main arr
    int* sub_arr = (int*) calloc(subdomain_size, sizeof(int));
    
    /*
    #ifdef DEBUG
    printf("send counts\n");
    for (int i = 0; i < world_size; i++) {
        printf("%d ", sendcounts[i]);
    }
    printf("\n");
    printf("displacements\n");
    for (int i = 0; i < world_size; i++) {
        printf("%d ", displs[i]);
    }
    printf("\n");
    #endif*/
    
    // if the rank == 0, then divide up the array
    MPI_Scatterv(arr, sendcounts, displs, MPI_INT, sub_arr, subdomain_size, MPI_INT, 0, comm);

    #ifdef DEBUG
    printf("Printing for %s, rank %d\n", processor_name, world_rank);
    for (int i = 0; i < subdomain_size; i++) {
        printf("%d ", sub_arr[i]);
    }
    printf("\n");
    #endif

    // quicksort the sub_arr rq for max effectiveness
    qsort(sub_arr, subdomain_size, sizeof(int), cmpfunc);

    // quicksort(num_elements, arr, comm);
    struct timeval start, end;
    if (world_rank == 0) {
        gettimeofday(&start, NULL);
        printf("starting quicksort\n");
    }

    quicksort(subdomain_size, sub_arr, comm, 1);
    MPI_Barrier(comm);

    if (world_rank == 0) {
        gettimeofday(&end, NULL); 

        double time_taken; 
  
        time_taken = (end.tv_sec - start.tv_sec) * 1e6; 
        time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6; 
  
        std::cout << "Time taken by program is : " << std::fixed << time_taken; 
        std::cout << " sec" << std::endl; 
    }


    MPI_Finalize();
}

void quicksort(int subdomain_size, int* sub_arr, MPI_Comm comm, int depth) {
    int world_rank;
    MPI_Comm_rank(comm, &world_rank);
    int world_size;
    MPI_Comm_size(comm, &world_size);

    if (world_size == 1) {
        /*printf("Rank %d is done!\n", world_rank);
        for (int i = 0; i < num_elements; i++) {
            printf("%d ", arr[i]);
        }
        printf("\n");*/
        //qsort(sub_arr, subdomain_size, sizeof(int), cmpfunc);
        return;
    }

    // qsort the array and find the median of each subarray
    // qsort(sub_arr, subdomain_size, sizeof(int), cmpfunc);
    float median = 0;
    if (subdomain_size % 2 == 0) {
        int num1 = sub_arr[subdomain_size / 2 - 1];
        int num2 = sub_arr[subdomain_size / 2];
        median = ((float)num1 + (float)num2) / 2; // technically this isn't right but what the heck
    } else {
        median = (float)sub_arr[subdomain_size / 2];
    }

    #ifdef DEBUG
    printf("After local sort for %s depth %d, rank %d, median: %f\n", processor_name, depth,world_rank, median);
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
    int split_ind = find_split_point(sub_arr, subdomain_size, mom);

    #ifdef DEBUG
    printf("Split ind for rank %d is %d\n", world_rank, split_ind);
    #endif

    //MPI_Barrier(comm);
    // printf("-------------------------------------\n");

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

    int new_len = 0;
    if (world_rank < world_size / 2) {
    // if (world_rank == 0) {
        new_len = split_ind + 1 + num_recv_elements;
        new_sub_arr = (int*) calloc(new_len, sizeof(int));
        //memcpy(new_sub_arr, sub_arr, split_ind + 1 * sizeof(int));
        //memcpy(new_sub_arr + split_ind + 1, recv_arr, num_recv_elements * sizeof(int));
        merge_arrays(sub_arr, recv_arr, new_sub_arr, split_ind + 1, num_recv_elements);
        /*
        #ifdef DEBUG
        printf("After memcopy from rank %d, num elements saved: %d, num new elements: %d \n", world_rank, split_ind + 1, num_recv_elements);
        for (int i = 0; i < new_len; i++) {
            printf("%d ", new_sub_arr[i]);
        }
        printf("\n");
        #endif*/
    } else {
        // if (world_rank == 2) {
        // printf("not yet!\n");
        new_len = (subdomain_size - (split_ind + 1)) + num_recv_elements;
        // new_len  = (subdomain_size - 1) - 
        new_sub_arr = (int*) calloc(new_len, sizeof(int));
        //memcpy(new_sub_arr, sub_arr + split_ind + 1, (subdomain_size - (split_ind + 1))*sizeof(int));
        // memcpy(new_sub_arr + (subdomain_size - (split_ind + 1)), recv_arr, num_recv_elements * sizeof(int));
        merge_arrays(sub_arr + split_ind + 1, recv_arr, new_sub_arr, subdomain_size - (split_ind + 1), num_recv_elements);

        #ifdef DEBUG
        printf("After memcopy from rank %d, num elements saved: %d, num new elements: %d \n", world_rank, (subdomain_size - (split_ind + 1)), num_recv_elements);
        for (int i = 0; i < new_len; i++) {
            printf("%d ", new_sub_arr[i]);
        }
        printf("\n");
        #endif
    }
    // MPI_Barrier(comm);
    MPI_Comm new_comm;
    int new_color = world_rank / (world_size / 2);
    MPI_Comm_split(comm, new_color, world_rank, &new_comm);
    //printf("Rank %d reached here on depth %d\n", world_rank,depth);
    //free(sub_arr);
    
    quicksort(new_len, new_sub_arr, new_comm, depth+1);
    
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

void merge_arrays(int* old_arr, int* recv_arr, int* new_arr, int old_num, int recv_num) {
    int i = 0;
    int j = 0; 
    int k = 0;

    while (i < old_num && j < recv_num) {
        if (old_arr[i] <= recv_arr[j]) {
            new_arr[k] = old_arr[i];
            i++;
        } else {
            new_arr[k] = recv_arr[j];
            j++;
        }
        k++;
    }

    while (i < old_num) {
        new_arr[k] = old_arr[i];
        i++; 
        k++;
    }

    while (j < recv_num) {
        new_arr[k] = recv_arr[i];
        j++; 
        k++;
    }

    return;
}