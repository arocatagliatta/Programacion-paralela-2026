/**
 * @file kmeans.c
 * @brief Implementación secuencial del algoritmo K-Means.
 *
 * Ejecuta el loop de clustering en un único hilo. Se usa como **línea base**
 * para cuantificar el speedup de la versión paralela (`kmeans_omp.c`).
 * La lógica del algoritmo es idéntica en ambas versiones; la diferencia
 * reside únicamente en las directivas OpenMP agregadas en `kmeans_omp.c`.
 *
 * @author Programacion Paralela 2026
 * @see kmeans.h
 * @see kmeans_omp.c
 */

#include <stdio.h>
#include <math.h>
#include <string.h>

#include "kmeans.h"

/**
 * @brief Calcula el índice del centroide más cercano a un valor dado.
 *
 * Recorre los `k` centroides y devuelve el índice de aquel cuya distancia
 * absoluta al valor sea mínima. Función auxiliar interna, no expuesta
 * en el header público.
 *
 * @param[in] value     Valor del punto a comparar.
 * @param[in] centroids Arreglo de `k` centroides actuales.
 * @param[in] k         Número de centroides.
 * @return Índice (0..k-1) del centroide más cercano.
 */
static int centroide_mas_cercano(double value, const double *centroids, int k) {
    int    best = 0;
    double min_dist = fabs(value - centroids[0]);

    for (int j = 1; j < k; j++) {
        double dist = fabs(value - centroids[j]);
        if (dist < min_dist) {
            min_dist = dist;
            best     = j;
        }
    }
    return best;
}

void kmeans_run(DataPoint *points, int n, double *centroids,
                int k, int max_iter, double tolerance) {
    int    iterations    = 0;
    int    total_changes = 0;
    double sums[k];
    int    counts[k];

    do {
        total_changes = 0;
        memset(counts, 0, k * sizeof(int));

        /* ── Fase 1: Asignación ───────────────────────────────────────────
         * Cada punto se asigna al centroide más cercano.
         * Si cambia de cluster, se incrementa total_changes.
         */
        for (int i = 0; i < n; i++) {
            int best = centroide_mas_cercano(points[i].value, centroids, k);
            if (points[i].cluster != best) {
                points[i].cluster = best;
                total_changes++;
            }
            counts[best]++;
        }

        /* ── Fase 2: Actualización ───────────────────────────────────────
         * El nuevo centroide de cada cluster es el promedio de sus puntos.
         */
        memset(sums, 0, k * sizeof(double));
        for (int i = 0; i < n; i++) {
            sums[points[i].cluster] += points[i].value;
        }

        printf("Iter %3d: ", iterations);
        for (int j = 0; j < k; j++) {
            if (counts[j] > 0) centroids[j] = sums[j] / counts[j];
            printf("[C%d: val=%.4f cant=%6d] ", j, centroids[j], counts[j]);
        }
        printf("| cambios: %d\n", total_changes);

        iterations++;

    } while (iterations < max_iter && (double)total_changes / n > tolerance);

    printf("Finalizado en %d iteraciones.\n", iterations);
}
