/**
 * @file csv_reader.c
 * @brief Implementación del lector de archivos CSV de varianza de movimiento.
 *
 * Este módulo es la única unidad del proyecto que conoce el formato concreto
 * del archivo de entrada. Si el CSV cambia de separador, número de columnas
 * o codificación, solo se modifica este archivo.
 *
 * @author Programacion Paralela 2026
 * @see csv_reader.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "csv_reader.h"

/** @brief Longitud máxima de una línea del CSV (en bytes). */
#define LINE_BUF 1024

/**
 * @brief Determina si un token leído del CSV es un valor numérico válido.
 *
 * Descarta tokens nulos, vacíos o que sean solo saltos de línea. Se usa
 * internamente para filtrar filas incompletas o corruptas sin abortar la
 * lectura completa del archivo.
 *
 * @param[in] token Puntero al token extraído por `strtok()`.
 * @return 1 si el token es utilizable, 0 en caso contrario.
 */
static int token_valido(const char *token) {
    return token != NULL
        && token[0] != '\n'
        && token[0] != '\r'
        && strcmp(token, "") != 0;
}

int csv_read(const char *filename, DataPoint *points, int max_points) {
    FILE *file = fopen(filename, "r");
    if (!file) return -1;

    char line[LINE_BUF];
    int n = 0;

    /* Descartar la cabecera (primera línea del CSV). */
    fgets(line, LINE_BUF, file);

    while (n < max_points && fgets(line, LINE_BUF, file)) {
        /* Columna 1: timestamp — se extrae y se descarta. */
        char *token = strtok(line, ",");

        /* Columna 2: valor de varianza de movimiento. */
        token = strtok(NULL, ",");

        if (token_valido(token)) {
            points[n].value   = atof(token);
            points[n].cluster = -1; /* Sin cluster asignado todavía. */
            n++;
        }
    }

    fclose(file);
    return n;
}
