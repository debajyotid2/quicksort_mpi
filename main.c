/* Demonstration of quicksort using OpenMPI */

#include "src/mpi_quicksort.h"
#include <unistd.h>

int main(int argc, char **argv) {
    // Initialize MPI
    MPI_Init(&argc, &argv);

    if (argc!=2) {
        printf("Usage: %s <length of array to sort>\n", argv[0]);
        return EXIT_FAILURE;
    }
    size_t length = atoi(argv[1]);  // Length of array to sort
    long *times = NULL;             // Array to store time taken for the sort operation
    long parallel = 0;              // Average runtime in microseconds for the parallel version
    long serial = 0;                // Average runtime in microseconds for the serial version
    int rank = 0;                   // Rank of the process (used by MPI)
    int num_procs = 0;              // Number of processes (specified at runtime)
    
    // Ensure that a positive length of array is provided
    assert(length > 0);
    
    // Fill an array of specified size with random numbers 
    int *numbers = generate_randomarray(length);
    int *numbers_copy = (int *)calloc(length, sizeof(int));
        
    // Make a copy for the serial version of quicksort
    memcpy(numbers_copy, numbers, length*sizeof(int));
    
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    
    // Process with PID 0 aggregates the parallel process run times in the times array
    if (rank==0) {
        times = (long *)calloc(num_procs, sizeof(long));
    }
    
    // Do the serial quicksort
    serial = quicksort(numbers_copy, length);

    // Do the parallel quicksort
    parallel = mpi_quicksort_wrapper(numbers, length, num_procs, rank);

    // Destroy the number arrays
    free(numbers);
    free(numbers_copy);
    
    // Gather the run times of the parallel quicksort processes
    MPI_Gather(&parallel, 1, MPI_LONG,
            times, 1, MPI_LONG, 0, MPI_COMM_WORLD);

    // Process with PID 0 calculates the average runtime for the parallel quicksort
    if (rank==0) {
        for (size_t i=1; i<(size_t)num_procs; i++) {
            parallel += times[i];
        }
        parallel /= num_procs;

        // Length,processors,serial,parallel
        printf("Sorted %lu numbers using %d processors.\nSerial quicksort took %lu microseconds\nParallel quicksort took %lu microseconds.\n", length, num_procs, serial, parallel);
        free(times);
    }

    MPI_Finalize();
    
    return EXIT_SUCCESS;
}
