typedef struct process_map {
    int *sizes;
    int **workers;
} process_map;

void send_coord_data(int rank, int size, int *workers);
process_map *recv_coord_data(int rank, int size, int *workers);
void print_process_map(process_map *map, int rank);
void send_map_to_workers(int rank, process_map *map);
process_map *recv_map_from_coord();
int *send_data_coord(int *v, process_map *map, int dim);
int *recv_data_coord(int rank, int *dim);
void comunicate_data_to_workers(int rank, int *to_double, int dimension_to_double, process_map *map);
void double_the_data(int rank);
void send_back_data_to_coord0(int rank, int *to_double, int dimension_to_double);
void recv_data_from_coord(int rank, int *v, process_map *map, int *dims);
void print_vector(int *v, int dim);
int *create_data_for_coord0(int *v, int dim);
void free_map(process_map *map);