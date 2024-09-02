/* Quicksort implementation using OpenMPI */

#include "mpi_quicksort.h"

// Swap integers between two locations 
void swap(int *a, int *b) {
    if (a==b) {
        return;
    }
    int temp = *a;
    *a = *b;
    *b = temp;
}

// Compute a prefix sum of a given 1D array
// e.g. for array A = [1 2 3 4 5],
// prefix_sum(A) = [1 3 6 10 15], i.e.
// the i-th element is the sum of the first i-th elements
size_t *prefix_sum(size_t *array, size_t length) {
    if (length==0) {
        return NULL;
    }

    size_t *result = (size_t *)calloc(length, sizeof(size_t));

    result[0] = array[0];
    for (size_t i=0; i<length-1; i++) {
        result[i+1] = result[i]+array[i+1];
    }
    return result;
}

// Generate an array of specified length and fill it with
// numbers from 0 to 99 with repetition randomly
int *generate_randomarray(size_t length) {
    assert(length>0);

    int *array = (int *)calloc(length, sizeof(int));
    
    srand(time(NULL)*length);

    for (size_t i=0; i<length; i++) {
        array[i] = rand()%100;
    }

    return array;
}

// Generate an array of specified length and fill it with
// numbers from 0 to 99 with repetition in descending order
// (worst case for an ascending sorting algorithm)
int *generate_worst_case(size_t length) {
    assert(length>0);

    int *array = (int *)calloc(length, sizeof(int));
    
    for (size_t i=0; i<length; i++) {
        array[i] = length-i;
    }

    return array;
}

// Utility function to print an array of integers
void print_array(int *array, size_t length) {
    for (size_t i=0; i<length; i++) {
        printf("%d ", array[i]);
    }
    printf("\n");
}

// Utility function to print an array of unsigned integers
void print_unsigned_int_array(size_t *array, size_t length) {
    for (size_t i=0; i<length; i++) {
        printf("%lu ", array[i]);
    }
    printf("\n");
}

// Serial quicksort wrapper function. Sorts an array and
// returns the runtime in microseconds.
long quicksort(int *array, size_t length) {
    assert(length>0);

    struct timeval start_t, end_t;

    gettimeofday(&start_t, NULL);

    // Sort using serial version of quicksort
    sort_array(array, 0, length-1);

    gettimeofday(&end_t, NULL);
    
    // Calculate runtime
    long duration = (end_t.tv_sec-start_t.tv_sec)*1000000+(end_t.tv_usec-start_t.tv_usec);

    return duration;
}

// Serial version of quicksort called recursively by both the 
// serial and parallel algorithms
void sort_array(int *array, size_t start, size_t end) {
    // Already sorted
    if (start>=end) {
        return;
    }
    // Only two elements
    if (end-start==1) {
        if (array[end]<array[start]) {
            swap(&array[end], &array[start]);
        }
        return;
    }
    
    srand(time(NULL)+start);
    
    // Swap the last element with a random element in the middle
    swap(&array[end], &array[start+rand()%(end-start)]);

    size_t low = start, mid = start; 
    size_t high = (end==0) ? end: end-1;
    
    // Partition
    while (mid<=high) { 
        if (array[mid]<array[end]) {
            swap(&array[mid], &array[low]);
            low++;
            mid++;
        }
        else if (array[mid]==array[end]) {
            mid++;
        }
        else {
            swap(&array[mid], &array[high]);
            if (high==0) {
                break;
            }
            high--;
        }
    }
    if (mid<=end) {
        swap(&array[end], &array[mid]);
    }

    size_t left_end = (low==0)? start : low-1;
    
    // Sort left half
    sort_array(array, start, left_end);
    // Sort right half
    sort_array(array, mid+1, end);
}

// Parallel version of quicksort - a wrapper function
// that calls mpi_quicksort() to do the sorting
long mpi_quicksort_wrapper(int *array, size_t length, 
                            int num_procs, int rank) {
    // Check length>num_proc
    assert(length>(size_t)num_procs);

    struct timeval start_t, end_t;

    gettimeofday(&start_t, NULL);
 
    // Divide array equally among all processes
    size_t part_size = length/num_procs;
    int *part_array = NULL;
    array_t ret_array = {.array = NULL, .length = 0};
    
    // Each process splits its own array portion into
    // a lower half Li and an upper half Ui. Elements in 
    // Li are less than the pivot and elements in Ui are
    // greater than the pivot.
    part_array = (int *)calloc(part_size, sizeof(int));
    
    int start_idx = (part_size+length%num_procs);

    // Exchange Li and Ui between the "partner" processes
    if (rank==0) {
        // Send
        part_array = (int *)realloc(part_array, start_idx*sizeof(int));
        memcpy(part_array, array, start_idx*sizeof(int));
        
        for (size_t i=1; i<(size_t)num_procs; i++) {
            MPI_Send(&array[start_idx], part_size, 
                            MPI_INT, i, 2, MPI_COMM_WORLD);
            start_idx += part_size;
        }
        part_size += length%num_procs;
    }
    else {
        // Receive
        MPI_Recv(part_array, part_size, MPI_INT,
                    0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    
    // Sort respective parts of the array
    ret_array = mpi_quicksort(part_array, part_size, 
                        num_procs, MPI_COMM_WORLD);

    part_array = ret_array.array;
    part_size = ret_array.length;

    MPI_Barrier(MPI_COMM_WORLD);
    
    // Gather arrays
    int *recvcounts = (int *)calloc(num_procs, sizeof(int));
    int *displs = (int *)calloc(num_procs, sizeof(int));

    MPI_Gather(&part_size, 1, MPI_INT, 
            recvcounts, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    if (rank==0) {
        displs[0] = 0;
        for (size_t i=1; i<(size_t)num_procs; i++) {
            displs[i] = recvcounts[i-1]+displs[i-1];
        }
    }

    MPI_Gatherv(part_array, part_size, MPI_INT,
        array, recvcounts, displs, MPI_INT,
        0, MPI_COMM_WORLD);

    if (part_array!=NULL) {
        free(part_array);
    }
    free(recvcounts);
    free(displs);

    gettimeofday(&end_t, NULL);
    
    // Calculate time taken by process
    long duration = (end_t.tv_sec-start_t.tv_sec)*1000000+(end_t.tv_usec-start_t.tv_usec);

    return duration;
}

// Parallel quicksort using MPI. The array is split between processors
// until each processor has its own array. Then each processor
// uses serial quicksort to sort its own array.
array_t mpi_quicksort(int *array, 
                      size_t length, 
                      int num_procs,
                      MPI_Comm curr_comm) {
    int rank, start_rank, end_rank;
    int pivot, partner_id;
    int low_half_size = 0, high_half_size = 0;
    size_t *l_pivot = NULL;
    size_t *idxs = NULL;
    array_t ret_array = {.array = NULL, .length = 0};
    
    MPI_Comm_rank(curr_comm, &rank);

    start_rank = 0;
    end_rank = num_procs-1;

    // If start_rank == end_rank, sort locally
    if (num_procs==1) {
        if (length>0) {
            sort_array(array, 0, length-1);
        }
        ret_array.array = array;
        ret_array.length = length;
        return ret_array;
    }

    // Within each process, choose partner process
    partner_id = (end_rank-rank+start_rank);

    // Choose and broadcast pivot to all processes
    srand(time(NULL));
    
    pivot = length>0 ? array[rand()%length] : INT_MIN;
    MPI_Bcast(&pivot, 1, MPI_INT, 
                start_rank, curr_comm);

    // Split local list into elements less than the pivot, Li and 
    // elements greater than the pivot, Ui
    if (length>0) {
        l_pivot = (size_t *)calloc(length, sizeof(size_t));
    }

    for (size_t i=0; i<length; i++) {
        l_pivot[i] = array[i]<pivot ? 1: 0;
    }
    idxs = prefix_sum(l_pivot, length);

    for (size_t i=0; i<length; i++) {
        if (l_pivot[i]) {
            swap(&array[i], &array[idxs[i]-1]);
        }
    }

    // If low half proc (< num_proc/2), send Ui to partner, receive Li from partner
    if (rank<partner_id) {
        high_half_size = length>0 ? (int)length-idxs[length-1] : 0;
        
        MPI_Send(&high_half_size, 1, MPI_INT,
                        partner_id, 1, curr_comm);
        if (high_half_size>0) {
            MPI_Send(&array[idxs[length-1]], high_half_size, 
                    MPI_INT, partner_id, 2, curr_comm);
            array = (int *)realloc(array, 
                            (length-high_half_size)*sizeof(int));
            length -= high_half_size;
        }
        MPI_Recv(&low_half_size, 1, MPI_INT,
                        partner_id, 1, curr_comm, MPI_STATUS_IGNORE);
        
        if (low_half_size>0) {
            array = (int *)realloc(array, 
                            (length+low_half_size)*sizeof(int));
            MPI_Recv(&array[length], low_half_size, 
                MPI_INT, partner_id, 2, curr_comm, MPI_STATUS_IGNORE);
            length += low_half_size;
        }
    }
    // If high half proc (> num_proc/2), send Li to partner
    else {
        low_half_size = length>0 ? (int)idxs[length-1]: 0;
        
        MPI_Recv(&high_half_size, 1, MPI_INT,
                        partner_id, 1, curr_comm, MPI_STATUS_IGNORE);
        if (high_half_size>0) {
            array = (int *)realloc(array, 
                   (length+high_half_size)*sizeof(int));
            MPI_Recv(&array[length], 
                high_half_size, MPI_INT, partner_id, 2, 
                curr_comm, MPI_STATUS_IGNORE);
            length += high_half_size;
        }

        MPI_Send(&low_half_size, 1, MPI_INT,
                        partner_id, 1, curr_comm);
        if (low_half_size>0) {
            MPI_Send(array, low_half_size, MPI_INT,
                            partner_id, 2, curr_comm);
            array = (int *)memmove(array, &array[low_half_size],
                        (length-low_half_size)*sizeof(int));
            length -= low_half_size;
        }
    }
    MPI_Barrier(curr_comm);

    MPI_Comm new_comm;

    if (num_procs>2) {
        int group = rank / (num_procs/2);

        MPI_Comm_split(curr_comm, group, rank, &new_comm);
        MPI_Comm_size(new_comm, &num_procs);
    }
    else {
        new_comm = curr_comm;
        num_procs = 1;
    }

    // Recursively sort within your own process
    ret_array = mpi_quicksort(array, length, num_procs, new_comm);
    MPI_Barrier(new_comm);

    if (num_procs>2 && new_comm != MPI_COMM_WORLD) {
        MPI_Comm_free(&new_comm);
    }

    if (l_pivot!=NULL) {
        free(l_pivot);
    }
    if (idxs!=NULL) {
        free(idxs);
    }

    return ret_array;
}
