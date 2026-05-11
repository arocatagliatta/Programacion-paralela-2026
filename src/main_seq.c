/**
 * @file main_seq.c
 * @brief Punto de entrada del ejecutable K-Means secuencial (`kmeans_seq`).
 *
 * Orquesta el pipeline completo en modo de un único hilo:
 * 1. Valida los argumentos de línea de comandos.
 * 2. Reserva memoria para los puntos.
 * 3. Lee el archivo CSV con `csv_read()`.
 * 4. Inicializa los centroides desde `INITIAL_CENTROIDS`.
 * 5. Invoca `kmeans_run()` (versión secuencial).
 * 6. Libera la memoria y retorna.
 *
 * Este binario se usa como **línea base** para el benchmark de speedup.
 * No contiene lógica de algoritmo propia — toda la lógica reside en
 * `csv_reader.c` y `kmeans.c`.
 *
 * @author Programacion Paralela 2026
 *
 * @par Uso
 * @code
 * ./kmeans_seq <ruta_al_csv>
 * @endcode
 *
 * @par Ejemplo
 * @code
 * ./kmeans_seq data/movisA.csv
 * ./kmeans_seq data/movisB.csv
 * @endcode
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "point.h"
#include "config.h"
#include "csv_reader.h"
#include "kmeans.h"

/**
 * @brief Punto de entrada principal del ejecutable secuencial.
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

    printf("K-Means SECUENCIAL | archivo: %s | puntos: %d | K: %d\n",
           argv[1], n, K);

    kmeans_run(points, n, centroids, K, MAX_ITER, TOLERANCE);

    free(points);
    return 0;
}
