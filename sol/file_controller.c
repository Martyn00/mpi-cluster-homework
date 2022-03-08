#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

char *create_input_file(int rank_number) {
    int n;
    char *buffer = malloc(sizeof(int) * 30);
    n = sprintf(buffer, "cluster%d.txt", rank_number);

    return buffer;
}

int *readInts(int number_of_ints, FILE *file) {
    int *ints = malloc(sizeof(int) * number_of_ints);
    for (int i = 0; i < number_of_ints; i++) {
        fscanf(file, "%d", (ints + i));
    }
    return ints;
}