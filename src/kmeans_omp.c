/**
 * @file kmeans_omp.c
 * @brief Implementación paralela del algoritmo K-Means usando OpenMP.
 *
 * Misma lógica que `kmeans.c`, pero con directivas OpenMP en las dos fases
 * de mayor costo computacional. Ambas versiones exponen la misma firma de
 * función, lo que permite compararlas directamente en el benchmark.
 *
 * ### Estrategia de paralelización
 *
 * **Fase 1 — Asignación** (`parallel for` + `reduction`):
 * Las `n` iteraciones se dividen automáticamente entre los hilos disponibles.
 * Cada hilo procesa su subconjunto de puntos de forma completamente
 * independiente (no hay dependencia entre `points[i]` y `points[j]`).
 * El contador `total_changes` se acumula con `reduction(+:...)` para
 * evitar race conditions sin necesidad de sección crítica.
 *
 * **Fase 2 — Acumulación** (reducción manual con `critical`):
 * Dado que `reduction` en C estándar no soporta arreglos, cada hilo
 * acumula en sus propios arrays locales (`private_sums`, `private_counts`)
 * y consolida los resultados en los totales globales dentro de un bloque
 * `critical`, garantizando acceso exclusivo.
 *
 * **Fase 2 — Cálculo de centroides** (secuencial):
 * Con `counts[]` y `sums[]` ya consolidados, el cálculo del nuevo centroide
 * (`sums[j] / counts[j]`) es O(k) y no justifica paralelización adicional.
 *
 * @author Programacion Paralela 2026
 * @see kmeans_omp.h
 * @see kmeans.c  Versión secuencial de referencia.
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <omp.h>

#include "kmeans_omp.h"

void kmeans_omp_run(DataPoint *points, int n, double *centroids,
                    int k, int max_iter, double tolerance) {
    int    iterations    = 0;
    int    total_changes = 0;
    double sums[k];
    int    counts[k];

    do {
        total_changes = 0;
        memset(counts, 0, k * sizeof(int));

        /* ── Fase 1: Asignación — PARALELA ──────────────────────────────
         *
         * `parallel for` divide las n iteraciones entre los hilos.
         * `reduction(+:total_changes)` le da a cada hilo su propia copia
         * del contador; OpenMP las suma al terminar el bloque.
         *
         * Es seguro porque points[i].cluster solo lo escribe el hilo
         * que procesa el índice i — no hay solapamiento entre hilos.
         */
        #pragma omp parallel for reduction(+:total_changes)
        for (int i = 0; i < n; i++) {
            int    best     = 0;
            double min_dist = fabs(points[i].value - centroids[0]);

            for (int j = 1; j < k; j++) {
                double dist = fabs(points[i].value - centroids[j]);
                if (dist < min_dist) {
                    min_dist = dist;
                    best     = j;
                }
            }

            if (points[i].cluster != best) {
                points[i].cluster = best;
                total_changes++;
            }
        }

        /* ── Fase 2: Acumulación — PARALELA (reducción manual) ──────────
         *
         * No se puede usar `reduction` sobre arreglos en C estándar.
         * Solución: cada hilo usa arreglos locales (sin riesgo de carrera)
         * y los consolida en los globales dentro de `critical`.
         *
         * `nowait` elimina la barrera implícita del `omp for`, permitiendo
         * que el hilo que termina primero avance directamente al `critical`
         * en lugar de esperar a los demás en el punto de fin del for.
         */
        memset(sums, 0, k * sizeof(double));

        #pragma omp parallel
        {
            double private_sums[k];
            int    private_counts[k];
            memset(private_sums,   0, k * sizeof(double));
            memset(private_counts, 0, k * sizeof(int));

            #pragma omp for nowait
            for (int i = 0; i < n; i++) {
                private_sums[points[i].cluster]   += points[i].value;
                private_counts[points[i].cluster] += 1;
            }

            /* Un hilo a la vez suma sus parciales al total global. */
            #pragma omp critical
            {
                for (int j = 0; j < k; j++) {
                    sums[j]   += private_sums[j];
                    counts[j] += private_counts[j];
                }
            }
        } /* fin del bloque parallel — barrera implícita aquí */

        /* ── Fase 2: Actualización de centroides — SECUENCIAL ───────────
         *
         * O(k) operaciones; la paralelización no aportaría beneficio medible
         * con k=4. El centroide j es el promedio de los puntos del cluster j.
         */
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
