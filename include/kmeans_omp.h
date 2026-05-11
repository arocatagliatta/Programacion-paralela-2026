/**
 * @file kmeans_omp.h
 * @brief Interfaz pública del algoritmo K-Means paralelo con OpenMP.
 *
 * Expone `kmeans_omp_run()`, que tiene **exactamente la misma firma** que
 * `kmeans_run()` en `kmeans.h`. Esto permite intercambiar ambas versiones
 * en el `main` sin cambiar el contrato de llamada.
 *
 * ### Paralelismo aplicado
 *
 * Se paralelizaron las dos fases de mayor costo computacional O(n):
 *
 * **Fase de asignación** (`#pragma omp parallel for reduction`):
 * Cada hilo recibe un subconjunto de puntos y calcula su cluster más
 * cercano de forma independiente. El contador `total_changes` se acumula
 * sin race condition mediante `reduction(+:total_changes)`.
 *
 * **Fase de actualización** (reducción manual con `critical`):
 * Cada hilo acumula sumas y conteos en arreglos locales (`private_sums`,
 * `private_counts`). Al finalizar, consolida sus parciales en los totales
 * globales dentro de una sección `#pragma omp critical`. Esto evita la
 * limitación de `reduction` sobre arreglos en C estándar.
 *
 * ### Directivas OpenMP utilizadas
 * - `#pragma omp parallel for reduction(+:var)` — asignación paralela.
 * - `#pragma omp parallel` — bloque paralelo con variables privadas.
 * - `#pragma omp for nowait` — distribución de iteraciones sin barrera.
 * - `#pragma omp critical` — sección crítica para consolidar resultados.
 *
 * @author Programacion Paralela 2026
 * @see kmeans.h  Versión secuencial con idéntica firma.
 */

#ifndef KMEANS_OMP_H
#define KMEANS_OMP_H

#include "point.h"

/**
 * @brief Ejecuta el algoritmo K-Means paralelo (OpenMP) sobre un conjunto de puntos.
 *
 * Funcionalidad idéntica a `kmeans_run()`, pero distribuye el trabajo de
 * asignación y acumulación entre todos los hilos disponibles según la
 * variable de entorno `OMP_NUM_THREADS`.
 *
 * @param[in,out] points     Arreglo de puntos a clasificar. El campo
 *                           `cluster` se actualiza en cada iteración.
 * @param[in]     n          Número total de puntos en el arreglo.
 * @param[in,out] centroids  Arreglo de `k` centroides inicializados antes
 *                           de llamar a esta función.
 * @param[in]     k          Número de clusters (debe ser > 0).
 * @param[in]     max_iter   Número máximo de iteraciones permitidas.
 * @param[in]     tolerance  Proporción mínima de cambios para continuar
 *                           (ej: 0.001 = 0,1%).
 *
 * @pre Requiere que el ejecutable haya sido compilado con `-fopenmp`
 *      (gestionado automáticamente por CMake mediante `OpenMP::OpenMP_C`).
 * @pre Mismas precondiciones que `kmeans_run()`.
 *
 * @par Control de hilos
 * El número de hilos se controla externamente:
 * @code
 * OMP_NUM_THREADS=8 ./kmeans_omp data/movisA.csv
 * @endcode
 */
void kmeans_omp_run(DataPoint *points, int n, double *centroids,
                    int k, int max_iter, double tolerance);

#endif /* KMEANS_OMP_H */
