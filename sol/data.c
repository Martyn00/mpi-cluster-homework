#include "data.h"

#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// crearea unui map nou
process_map* create_new_map() {
    process_map* map = malloc(sizeof(process_map));
    map->sizes = calloc(sizeof(int), 3);
    map->workers = malloc(sizeof(int*) * 3);
    return map;
}

// printarea map-ului
void print_process_map(process_map* map, int rank) {
    printf("%d -> ", rank);
    for (int coord_rank = 0; coord_rank < 3; coord_rank++) {
        printf("%d:", coord_rank);
        for (int i = 0; i < map->sizes[coord_rank]; i++) {
            printf("%d", map->workers[coord_rank][i]);
            if (i != map->sizes[coord_rank] - 1) {
                printf(",");
            }
        }
        if (coord_rank != 2) {
            printf(" ");
        } else {
            printf("\n");
        }
    }
}

// trimitere datelor catre ceilalti coordonatori
void send_coord_data(int rank, int size, int* workers) {
    for (int i = 0; i < 3; i++) {
        if (i != rank) {
            printf("M(%d,%d)\n", rank, i);
            MPI_Send(&size, 1, MPI_INT, i, rank, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, i);
            MPI_Send(workers, size, MPI_INT, i, rank + 10, MPI_COMM_WORLD);
        }
    }
}
// formarea map-ului din colectarea datelor de la ceilalti coordonatori
process_map* recv_coord_data(int rank, int size, int* workers) {
    MPI_Status status;
    process_map* map = create_new_map();
    map->sizes[rank] = size;
    map->workers[rank] = workers;
    // preluarea size
    for (int i = 0; i < 3; i++) {
        int new_size;
        if (i != rank) {
            MPI_Recv(&new_size, 1, MPI_INT, i, i, MPI_COMM_WORLD, &status);
            map->sizes[i] = new_size;
        }
    }
    // preluarea rangului workerilor
    for (int i = 0; i < 3; i++) {
        if (i != rank) {
            int* workers = malloc(sizeof(int) * map->sizes[i]);
            MPI_Recv(workers, map->sizes[i], MPI_INT, i, i + 10, MPI_COMM_WORLD, &status);
            map->workers[i] = workers;
        }
    }
    return map;
}
// trimiterea mai departe a map-ului catre workeri
void send_map_to_workers(int rank, process_map* map) {
    for (int i = 0; i < map->sizes[rank]; i++) {
        // trimitere size
        printf("M(%d,%d)\n", rank, map->workers[rank][i]);
        MPI_Send(map->sizes, 3, MPI_INT, map->workers[rank][i], 66, MPI_COMM_WORLD);
        for (int j = 0; j < 3; j++) {
            // trimitere workeri
            printf("M(%d,%d)\n", rank, map->workers[rank][i]);
            MPI_Send(map->workers[j], map->sizes[j], MPI_INT, map->workers[rank][i], 67 + j, MPI_COMM_WORLD);
        }
    }
}

// primirea map-ului de la coordonatori de catre workeri
process_map* recv_map_from_coord() {
    MPI_Status status;
    process_map* map = create_new_map();
    MPI_Recv(map->sizes, 3, MPI_INT, MPI_ANY_SOURCE, 66, MPI_COMM_WORLD, &status);
    for (int i = 0; i < 3; i++) {
        map->workers[i] = calloc(sizeof(int), map->sizes[i]);
        MPI_Recv(map->workers[i], map->sizes[i], MPI_INT, MPI_ANY_SOURCE, 67 + i, MPI_COMM_WORLD, &status);
    }

    return map;
}

// coordonatorul 0 trimite datele create din param catre 1 si 2
int* send_data_coord(int* v, process_map* map, int dim) {
    int total_number_of_workers = 0;

    for (int i = 0; i < 3; i++) {
        total_number_of_workers += map->sizes[i];
    }
    // calcularea dimensiunilor pentru fiecare coordonator in functie de numarul de workeri
    int interval_len = dim / total_number_of_workers;
    int* dims = malloc(sizeof(int) * 3);
    dims[0] = interval_len * map->sizes[0];
    dims[1] = interval_len * map->sizes[1];
    dims[2] = dim - dims[0] - dims[1];

    // trimiterea datelor in functie de dimensiuni
    int start = dims[0];
    for (int i = 1; i < 3; i++) {
        MPI_Send(dims + i, 1, MPI_INT, i, 43 + i, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", 0, i);
        MPI_Send((v + start), dims[i], MPI_INT, i, 33 + i, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", 0, i);
        start += dims[i];
    }
    return dims;
}

// primirea datelor de catre coordonatorii 1 si 2
int* recv_data_coord(int rank, int* dim) {
    MPI_Status status;
    MPI_Recv(dim, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    int* v = malloc(sizeof(int) * (*dim));
    MPI_Recv(v, *dim, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    return v;
}
// impartirea datelor primite, trimiterea datelor catre workeri, primirea datelor de la workeri
void comunicate_data_to_workers(int rank, int* to_double, int dimension_to_double, process_map* map) {
    // impartirea datelor
    int nr_workers = map->sizes[rank];
    int* dims = malloc(sizeof(int) * nr_workers);
    int one_dim = dimension_to_double / nr_workers;
    for (int i = 0; i < nr_workers - 1; i++) {
        dims[i] = one_dim;
    }
    dims[nr_workers - 1] = dimension_to_double - (nr_workers - 1) * one_dim;
    // trimiterea datelor catre workeri
    int start = 0;
    for (int i = 0; i < nr_workers; i++) {
        int worker_rank = map->workers[rank][i];
        MPI_Send(dims + i, 1, MPI_INT, worker_rank, 43, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, worker_rank);
        MPI_Send((to_double + start), dims[i], MPI_INT, worker_rank, 33, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, worker_rank);
        start += dims[i];
    }
    // primirea datelor schimbate de la workeri si reasamblarea acestora in vector
    MPI_Status status;
    start = 0;
    for (int i = 0; i < nr_workers; i++) {
        int worker_rank = map->workers[rank][i];
        MPI_Recv((to_double + start), dims[i], MPI_INT, worker_rank, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        start += dims[i];
    }
    free(dims);
}
// functie ce dubleaza valorile unui vector
void double_vector(int* v, int dim) {
    for (int i = 0; i < dim; i++) {
        v[i] = 2 * v[i];
    }
}
// primirea vectorului de catre workeri dublarea acestuia si trimiterea inapoi catre coordonator
void double_the_data(int rank) {
    MPI_Status status;
    int dim;
    MPI_Recv(&dim, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    int* v = malloc(sizeof(int) * dim);
    MPI_Recv(v, dim, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    double_vector(v, dim);
    MPI_Send(v, dim, MPI_INT, status.MPI_SOURCE, 312, MPI_COMM_WORLD);
    printf("M(%d,%d)\n", rank, status.MPI_SOURCE);
    free(v);
}
// coordonatorii trimit datele primite inapoi de la workeri catre coordonatorul 0
void send_back_data_to_coord0(int rank, int* to_double, int dimension_to_double) {
    if (rank != 0) {
        MPI_Send(to_double, dimension_to_double, MPI_INT, 0, 313, MPI_COMM_WORLD);
        printf("M(%d,0)\n", rank);
    }
}
// coordonatorul 0 primeste datele de la coordonatorii 1 so 2
void recv_data_from_coord(int rank, int* v, process_map* map, int* dims) {
    MPI_Status status;
    int start = dims[0];
    for (int i = 1; i < 3; i++) {
        MPI_Recv((v + start), dims[i], MPI_INT, i, 313, MPI_COMM_WORLD, &status);
        start += dims[i];
    }
}
// printarea vectorului conform outputului cerut
void print_vector(int* v, int dim) {
    printf("Rezultat: ");
    for (int i = 0; i < dim; i++) {
        printf("%d ", v[i]);
    }
    printf("\n");
}

// crearea vectorului de dublare pentru coordonatorul0
int* create_data_for_coord0(int* v, int dim) {
    int* to_double = malloc(sizeof(int) * dim);
    memcpy(to_double, v, sizeof(int) * dim);
    return to_double;
}
// eliberarea de memorie a process_map
void free_map(process_map* map) {
    free(map->sizes);
    free(map->workers);
    free(map);
}