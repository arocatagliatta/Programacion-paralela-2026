/**
 * @file config.h
 * @brief Parámetros de configuración centralizados del algoritmo K-Means.
 *
 * Todas las constantes que controlan el comportamiento del algoritmo se
 * definen aquí. Modificar este archivo afecta simultáneamente a la versión
 * secuencial y a la paralela, garantizando consistencia y evitando valores
 * mágicos dispersos en el código.
 *
 * ### Criterio de parada dual
 * El algoritmo se detiene cuando se cumple **cualquiera** de estas condiciones:
 * - La proporción de puntos que cambia de cluster cae por debajo de
 *   `TOLERANCE` (convergencia natural).
 * - Se alcanza `MAX_ITER` iteraciones (salvaguarda ante divergencia).
 *
 * `MAX_ITER = 150` fue determinado experimentalmente: en todas las corridas
 * observadas con `movisA.csv` y `movisB.csv` la convergencia ocurrió antes
 * de las 100 iteraciones.
 *
 * @author Programacion Paralela 2026
 */

#ifndef CONFIG_H
#define CONFIG_H

/** @brief Número de clusters (categorías de movimiento). */
#define K           4

/**
 * @brief Máximo de iteraciones antes de forzar la detención.
 *
 * Valor fijado experimentalmente. Actúa como salvaguarda si la tolerancia
 * no se alcanza en un tiempo razonable.
 */
#define MAX_ITER    150

/**
 * @brief Proporción mínima de cambios para continuar iterando.
 *
 * Representa el 0,1% expresado como fracción decimal (0,1 / 100 = 0,001).
 * Cuando `total_changes / n <= TOLERANCE`, el algoritmo considera que
 * los clusters convergieron.
 */
#define TOLERANCE   0.001

/**
 * @brief Capacidad máxima del buffer de puntos en memoria.
 *
 * Los archivos CSV contienen aproximadamente 500.000 registros.
 * Se reserva un 20% adicional como margen de seguridad.
 */
#define MAX_POINTS  600000

/**
 * @brief Centroides iniciales estimados para los 4 grupos de movimiento.
 *
 * Elegidos por observación visual de los datos, ubicando un centroide
 * en la zona de mayor densidad de cada categoría de movimiento:
 * - C0 ≈ 0.2 : reposo / sin movimiento
 * - C1 ≈ 1.0 : movimiento leve
 * - C2 ≈ 2.5 : movimiento moderado
 * - C3 ≈ 4.5 : movimiento intenso
 *
 * @note Declarado `static` para evitar el error de símbolo duplicado
 *       cuando este header se incluye en múltiples unidades de traducción.
 */
static const double INITIAL_CENTROIDS[K] = {0.2, 1.0, 2.5, 4.5};

#endif /* CONFIG_H */
