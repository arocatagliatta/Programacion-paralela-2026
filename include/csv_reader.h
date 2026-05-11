/**
 * @file csv_reader.h
 * @brief Interfaz pública para la lectura de archivos CSV de varianza de movimiento.
 *
 * Este módulo encapsula toda la lógica de entrada de datos, desacoplando
 * el algoritmo K-Means del formato concreto de los archivos. Si el formato
 * del CSV cambia (separador, columnas, codificación), solo se modifica
 * `csv_reader.c` sin tocar el algoritmo.
 *
 * ### Formato esperado del CSV
 * ```
 * timestamp,varianza
 * 1234567890,0.312
 * 1234567891,3.847
 * ...
 * ```
 * - Primera fila: cabecera (se descarta automáticamente).
 * - Columna 1: timestamp (se ignora).
 * - Columna 2: valor de varianza de movimiento (se lee como `double`).
 * - Separador: coma (`,`).
 *
 * @author Programacion Paralela 2026
 * @see DataPoint
 * @see csv_read()
 */

#ifndef CSV_READER_H
#define CSV_READER_H

#include "point.h"

/**
 * @brief Lee un archivo CSV y llena un arreglo de DataPoint.
 *
 * Abre el archivo indicado por `filename`, descarta la primera línea
 * (cabecera), y extrae el valor de varianza de la segunda columna de
 * cada fila restante. Las filas vacías o con segunda columna inválida
 * se descartan silenciosamente.
 *
 * @param[in]  filename   Ruta al archivo CSV a leer.
 * @param[out] points     Buffer preasignado donde se almacenarán los puntos
 *                        leídos. Debe tener capacidad para al menos
 *                        `max_points` elementos.
 * @param[in]  max_points Capacidad máxima del buffer `points`. La lectura
 *                        se detiene al alcanzar este límite.
 *
 * @return Número de puntos efectivamente leídos (>= 0), o -1 si el archivo
 *         no pudo abrirse.
 *
 * @note El campo `cluster` de cada punto se inicializa en -1 (sin asignar).
 *
 * @warning El buffer `points` debe estar preasignado por el llamador.
 *          Esta función no reserva memoria.
 *
 * @par Ejemplo de uso
 * @code
 * DataPoint *pts = malloc(MAX_POINTS * sizeof(DataPoint));
 * int n = csv_read("data/movisA.csv", pts, MAX_POINTS);
 * if (n < 0) { fprintf(stderr, "Error al abrir el archivo\n"); }
 * @endcode
 */
int csv_read(const char *filename, DataPoint *points, int max_points);

#endif /* CSV_READER_H */
