/**
 * @file kmeans.h
 * @brief Interfaz pública del algoritmo K-Means secuencial.
 *
 * Expone la función `kmeans_run()` que ejecuta el algoritmo completo de
 * agrupamiento sobre un arreglo de DataPoint usando un único hilo de CPU.
 * Esta versión sirve como **línea base** para medir el speedup obtenido
 * con la versión paralela (`kmeans_omp.h`).
 *
 * ### Algoritmo
 * El loop principal alterna dos fases hasta cumplir la condición de parada:
 * 1. **Asignación**: cada punto se asigna al centroide más cercano
 *    (distancia absoluta unidimensional).
 * 2. **Actualización**: cada centroide se desplaza al promedio aritmético
 *    de los puntos que le fueron asignados.
 *
 * ### Condición de parada
 * Se detiene cuando **cualquiera** de estas condiciones se cumple:
 * - `total_cambios / n <= tolerance` (convergencia: menos del 0,1% cambia).
 * - `iteracion >= max_iter` (límite experimental: 150 iteraciones).
 *
 * @author Programacion Paralela 2026
 * @see kmeans_omp.h  Versión paralela con idéntica firma.
 */

#ifndef KMEANS_H
#define KMEANS_H

#include "point.h"

/**
 * @brief Ejecuta el algoritmo K-Means secuencial sobre un conjunto de puntos.
 *
 * Modifica `points[i].cluster` in-place en cada iteración hasta alcanzar
 * la convergencia o el límite de iteraciones. Los centroides finales
 * quedan almacenados en `centroids[]` al retornar.
 *
 * @param[in,out] points     Arreglo de puntos a clasificar. El campo
 *                           `cluster` se actualiza en cada iteración.
 * @param[in]     n          Número total de puntos en el arreglo.
 * @param[in,out] centroids  Arreglo de `k` centroides. Se inicializan
 *                           antes de llamar a esta función y se actualizan
 *                           internamente en cada iteración.
 * @param[in]     k          Número de clusters (debe ser > 0).
 * @param[in]     max_iter   Número máximo de iteraciones permitidas.
 * @param[in]     tolerance  Proporción mínima de cambios para continuar
 *                           (ej: 0.001 = 0,1%).
 *
 * @pre `points` debe ser un arreglo válido de `n` elementos.
 * @pre `centroids` debe tener exactamente `k` elementos inicializados.
 * @pre `n > 0`, `k > 0`, `max_iter > 0`, `0 < tolerance < 1`.
 *
 * @par Ejemplo de uso
 * @code
 * double centroids[K];
 * memcpy(centroids, INITIAL_CENTROIDS, K * sizeof(double));
 * kmeans_run(points, n, centroids, K, MAX_ITER, TOLERANCE);
 * @endcode
 */
void kmeans_run(DataPoint *points, int n, double *centroids,
                int k, int max_iter, double tolerance);

#endif /* KMEANS_H */
