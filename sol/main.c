#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data.h"
#include "file_controller.h"

#define COORDINATOR0 0
#define COORDINATOR1 1
#define COORDINATOR2 2

int main(int argc, char *argv[]) {
    int rank;
    int nProcesses;
    int *owned_workers;
    int number_of_workers = -1;
    process_map *map;
    int *v, *to_double, *dims, dim;
    int dimension_to_double;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nProcesses);
    if (rank == COORDINATOR0 || rank == COORDINATOR1 || rank == COORDINATOR2) {
        // citirea datelor din fisiere
        char *file_name = create_input_file(rank);
        FILE *file = fopen(file_name, "r");
        number_of_workers;
        fscanf(file, "%d", &number_of_workers);
        owned_workers = readInts(number_of_workers, file);
        free(file_name);
        fclose(file);
        send_coord_data(rank, number_of_workers, owned_workers);
        // crearea map-ului prin comunicarea intre cooordonatori
        map = recv_coord_data(rank, number_of_workers, owned_workers);
        // trimiterea mapului intre workero
        send_map_to_workers(rank, map);
    } else {
        map = recv_map_from_coord();
    }
    // printarea topologiei din fiecare proces
    MPI_Barrier(MPI_COMM_WORLD);
    print_process_map(map, rank);
    MPI_Barrier(MPI_COMM_WORLD);

    // crearea vectorului ce trebuie dublat
    if (rank == COORDINATOR0) {
        dim = atoi(argv[1]);
        v = calloc(sizeof(int), dim);
        for (int i = 0; i < dim; i++) {
            v[i] = i;
        }

        dims = send_data_coord(v, map, dim);
    }
    if (rank == COORDINATOR0 || rank == COORDINATOR1 || rank == COORDINATOR2) {
        // primirea datelor de la coordonatorul 0
        if (rank != COORDINATOR0) {
            to_double = recv_data_coord(rank, &dimension_to_double);
            // datele asignate coordonatoorului0 nu se vor trimite inapoi catre el insusi
        } else {
            dimension_to_double = dims[0];
            to_double = create_data_for_coord0(v, dimension_to_double);
        }
        // trimiterea date catre workeri si primirea datelor dublate inapoi
        comunicate_data_to_workers(rank, to_double, dimension_to_double, map);
        // trimiterea datelor catre coordonatorul0 ce le va reasambla
        send_back_data_to_coord0(rank, to_double, dimension_to_double);

        // dublarea valorile doar in cazul workerilor
    } else {
        double_the_data(rank);
    }

    // preluarea datelor de la coordonatori si punerea inapoi in vectorul initial doar ca are valorile dublate
    if (rank == COORDINATOR0) {
        memcpy(v, to_double, dimension_to_double * sizeof(int));
        recv_data_from_coord(rank, v, map, dims);
        print_vector(v, dim);
    }

    // eliberare memorie
    if (rank == COORDINATOR0 || rank == COORDINATOR1 || rank == COORDINATOR2) {
        free(to_double);
    }
    if (number_of_workers != -1) {
        free(owned_workers);
    }
    if (rank == COORDINATOR0) {
        free(v);
        free(dims);
    }
    free_map(map);
    MPI_Finalize();
    return 0;
}
