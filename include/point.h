/**
 * @file point.h
 * @brief Definición de la estructura de datos principal del algoritmo K-Means.
 *
 * Este header define el tipo `DataPoint`, que representa un único dato de
 * varianza de movimiento leído desde los archivos CSV. Es la unidad
 * fundamental que recorre todo el pipeline: lectura, agrupamiento y reporte.
 *
 * @author Programacion Paralela 2026
 */

#ifndef POINT_H
#define POINT_H

/**
 * @brief Representa un punto de datos unidimensional con su asignación de cluster.
 *
 * Cada instancia corresponde a un registro del archivo CSV de varianza de
 * movimiento. El campo `cluster` se inicializa en -1 y se actualiza en cada
 * iteración del algoritmo K-Means hasta que el punto converge a un grupo estable.
 */
typedef struct {
    double value;   /**< Valor de varianza de movimiento leído del CSV. */
    int    cluster; /**< Índice del cluster asignado (0..K-1). -1 = sin asignar. */
} DataPoint;

#endif /* POINT_H */
