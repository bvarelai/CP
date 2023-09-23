#include <stdio.h>
#include <sys/time.h>
#include <mpi/mpi.h>
#include <math.h>

#define DEBUG 1

#define N 1024

int main(int argc, char *argv[] ) {

    int i, j, totalprocs, process, tam;
    struct timeval  tv1, tv2, tv3, tv4;

    MPI_Init(&argc, &argv); //Inicializamos el MPI
    MPI_Comm_size(MPI_COMM_WORLD, &totalprocs); // Obtenemos el número de procesos
    MPI_Comm_rank(MPI_COMM_WORLD, &process); //Obtenemos el número de proceso

    //Calculamos el tamaño del bloque (El número mayor de dividir N entre el número de procesos)
    tam = ceil(((double) N / totalprocs));

    //Creamos la matriz de esta forma añadiendo unas filas de basura para que los procesos hagan el mismo trabajo
    float matrix[tam*totalprocs][N];
    float vector[N];
    float result[tam*totalprocs];

    /* Initialize Matrix and Vector */
    if(process == 0){ //El proceso 0 es el encargado de inicializar el vector y la matriz
        for(i=0;i<N;i++) {
            vector[i] = i;
            for(j=0;j<N;j++) {
                matrix[i][j] = i+j;
            }
        }
    }

    /* Obtenemos el primer tiempo de comunicación*/
    gettimeofday(&tv3, NULL);

    /* Distribución de datos con operaciones colectivas */
    MPI_Scatter(*matrix, (N*tam), MPI_FLOAT, *matrix, (N*tam), MPI_FLOAT, 0, MPI_COMM_WORLD); //Distribuímos los datos de la matriz a todos los procesos. Pasamos N*tam datos porque son los que corresponden a cada proceso.
    MPI_Bcast(vector, N, MPI_FLOAT, 0, MPI_COMM_WORLD);//Le damos el valor del vector a todos los procesos.

    /* Obtenemos el primer tiempo de computación y el segundo de comunicación */
    gettimeofday(&tv1, NULL);

    for(i = 0; i < tam; i++) { //i Tiene que llegar hasta el número total de filas asignadas a cada proceso.
        result[i]=0;
        for(j = 0; j < N; j++) {
            result[i] += matrix[i][j]*vector[j];
        }
    }

    /* Obtenemos el segundo tiempo de computación */
    gettimeofday(&tv2, NULL);

    int microseconds = (tv2.tv_usec - tv1.tv_usec) + 1000000 * (tv2.tv_sec - tv1.tv_sec);

    /* Recolección del vector resultado utilizando una operación colectiva */
    MPI_Gather(result, tam, MPI_FLOAT, result, tam, MPI_FLOAT, 0, MPI_COMM_WORLD); //El tamaño de vuelta es el mismo que el tamaño de cada bloque.

    /* Obtenemos el sexto tiempo de comunicación*/
    gettimeofday(&tv4, NULL);

    int comunicacion = (tv1.tv_usec - tv3.tv_usec + tv4.tv_usec - tv2.tv_usec) + 1000000 * (tv1.tv_sec - tv3.tv_sec + tv4.tv_sec - tv2.tv_sec );

    printf ("Tiempo de computación del proceso %d (segundos) = %lf\n", process, (double) microseconds/1E6);
    printf ("Tiempo de comunicación del proceso %d (segundos) = %lf\n", process, (double) comunicacion/1E6);

    /* Display result */
    if (DEBUG && process == 0){
        for(i=0;i<N;i++) {
            printf(" %f \t ",result[i]);
        }
    }

    MPI_Finalize();

    return 0;
}