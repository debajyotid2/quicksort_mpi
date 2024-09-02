/* Quicksort implementation using OpenMPI */
#ifndef _MPI_QUICKSORT_H_
#define _MPI_QUICKSORT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <mpi.h>
#include <time.h>
#include <assert.h>
#include <sys/time.h>
#include <limits.h>

typedef struct {
    int *array;
    size_t length;
} array_t;

void swap(int *, int *);
int *generate_randomarray(size_t);
int *generate_worst_case(size_t);
void print_array(int *, size_t);
void print_unsigned_int_array(size_t *, size_t);
long quicksort(int *, size_t);
void sort_array(int *, size_t, size_t);
long mpi_quicksort_wrapper(int *, size_t, int, int);
array_t mpi_quicksort(int *, size_t, int, MPI_Comm);

#endif
