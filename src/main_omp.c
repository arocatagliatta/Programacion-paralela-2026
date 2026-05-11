/**
 * @file main_omp.c
 * @brief Punto de entrada del ejecutable K-Means paralelo con OpenMP (`kmeans_omp`).
 *
 * Orquesta el mismo pipeline que `main_seq.c`, pero invoca `kmeans_omp_run()`
 * en lugar de `kmeans_run()`. Toda la lógica de paralelización reside en
 * `kmeans_omp.c`; este archivo solo coordina la entrada de datos y la
 * llamada al algoritmo.
 *
 * El número de hilos se controla externamente mediante la variable de entorno
 * `OMP_NUM_THREADS`. Si no se define, OpenMP usa tantos hilos como núcleos
 * lógicos tenga el sistema.
 *
 * @author Programacion Paralela 2026
 *
 * @par Uso
 * @code
 * ./kmeans_omp <ruta_al_csv>
 * OMP_NUM_THREADS=8 ./kmeans_omp <ruta_al_csv>
 * @endcode
 *
 * @par Ejemplo con afinidad de hilos
 * @code
 * OMP_NUM_THREADS=4 OMP_PROC_BIND=close OMP_PLACES=cores \
 *     ./kmeans_omp data/movisA.csv
 * @endcode
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#include "point.h"
#include "config.h"
#include "csv_reader.h"
#include "kmeans_omp.h"

/**
 * @brief Punto de entrada principal del ejecutable paralelo.
 *
 * @param[in] argc Número de argumentos (debe ser >= 2).
 * @param[in] argv argv[1] debe ser la ruta al archivo CSV de entrada.
 * @return 0 en caso de éxito, 1 si los argumentos son inválidos o el
 *         archivo no puede abrirse, 2 si la reserva de memoria falla.
 */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <archivo.csv>\n", argv[0]);
        return 1;
    }

    DataPoint *points = malloc(MAX_POINTS * sizeof(DataPoint));
    if (!points) {
        fprintf(stderr, "Error: no se pudo reservar memoria para %d puntos.\n",
                MAX_POINTS);
        return 2;
    }

    int n = csv_read(argv[1], points, MAX_POINTS);
    if (n < 0) {
        fprintf(stderr, "Error: no se pudo abrir '%s'.\n", argv[1]);
        free(points);
        return 1;
    }

    double centroids[K];
    memcpy(centroids, INITIAL_CENTROIDS, K * sizeof(double));

    printf("K-Means PARALELO (OpenMP) | archivo: %s | puntos: %d | K: %d | hilos: %d\n",
           argv[1], n, K, omp_get_max_threads());

    kmeans_omp_run(points, n, centroids, K, MAX_ITER, TOLERANCE);

    free(points);
    return 0;
}
