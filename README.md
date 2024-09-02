# Parallel Quicksort Algorithm implemented in OpenMPI

### Dependencies

- [`gcc`](https://gcc.gnu.org/install/) >= 13.3.1
- [OpenMPI](https://www.open-mpi.org/software/ompi/v5.0/) >= 5.0
- [`make`](https://www.gnu.org/software/make/) >= 4.4.1

### How to build
```
make
```
### How to run
```
mpirun -n <NUMBER OF PROCESSORS> ./run <SIZE OF ARRAY TO SORT>
```
### License
[BSD-3](https://opensource.org/license/BSD-3-clause)
