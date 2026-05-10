# K-Means Clustering: Movimiento Humano (Secuencial vs Paralelo)

Este proyecto implementa el algoritmo de agrupamiento K-Means en lenguaje C para analizar grandes volúmenes de datos de movimiento (extraídos de archivos CSV). Se comparan dos versiones: una **secuencial** (C puro) y una **paralela** (utilizando la librería OpenMP) para medir la mejora en tiempos de ejecución.

## 📋 Contenido del Proyecto

1.  **`kmeans.c`**: Implementación estándar del algoritmo.
2.  **`kmeans_omp.c`**: Implementación optimizada con paralelismo de datos.
3.  **`benchmark.py`**: Script de automatización para la versión secuencial.
4.  **`benchmark_final.py`**: Script avanzado para medir escalabilidad y afinidad en la versión paralela.

## 🛠 Algoritmo y Lógica

El programa está diseñado para clasificar datos de movimiento en **4 categorías** (K=4). 

### Características principales:
- **Inicialización:** Basada en estimaciones de medias (0.2, 1.0, 2.5, 4.5) para acelerar la convergencia según la naturaleza de los datos.
- **Criterio de Parada:**
    - Cambio de componentes menor al **0.1%** entre iteraciones.
    - Umbral máximo de **150 iteraciones**.
- **Procesamiento de Datos:** Manejo de archivos de ~500k registros, ignorando valores nulos o cabeceras.

## 🚀 Versiones de Benchmark

Hemos generado 4 tipos de mediciones de rendimiento divididas en dos flujos:

### 1. K-Means Secuencial (C estándar)
- **Prueba A (`movisA.csv`)**: Mide el tiempo promedio de 10 ejecuciones sobre el primer set de datos.
- **Prueba B (`movisB.csv`)**: Mide el tiempo promedio de 10 ejecuciones sobre el segundo set de datos.
*Objetivo: Establecer una línea base de rendimiento en un solo núcleo.*

### 2. K-Means Paralelizado (OpenMP)
- **Prueba de Escalabilidad**: Ejecución variando el número de hilos (definido por `OMP_NUM_THREADS`).
- **Prueba de Afinidad (Pinning)**: Uso de variables de entorno para fijar hilos a núcleos físicos y evitar saltos de caché.
*Objetivo: Demostrar la reducción de tiempo al distribuir la carga en múltiples núcleos.*

## 🧬 Herramientas de OpenMP Utilizadas

Para la versión paralela (`kmeans_omp.c`), se emplearon las siguientes directivas:

| Herramienta | Aplicación |
| :--- | :--- |
| `#pragma omp parallel for` | Distribuye el bucle de asignación de clusters entre los hilos. |
| `reduction(+:total_changes)` | Suma de forma segura el contador de cambios de cada hilo para verificar la convergencia. |
| `critical` | Sincroniza la actualización de los centroides globales desde sumas locales de hilos. |
| `OMP_PROC_BIND` / `PLACES` | Control manual de afinidad para que los hilos no migren entre núcleos de la CPU. |

## 💻 Instrucciones de Ejecución

### Requisitos
- Compilador `gcc`.
- Librería OpenMP instalada (`libgomp`).
- Python 3 para correr los benchmarks.

### Ejecución del Benchmark Final
El script `benchmark_final.py` automatiza la compilación y las 10 ejecuciones por archivo:

```bash
python3 benchmark_final.py

Programación paralela
Gem personalizado
Para esta versión, utilizaremos OpenMP para distribuir la carga de trabajo entre los núcleos de tu procesador. El procesamiento de grandes volúmenes de datos (como tus archivos de ~500k registros) es un caso de uso ideal para el paralelismo de datos.

1. Código K-Means con OpenMP (kmeans_omp.c)
He añadido directivas de OpenMP en las secciones más pesadas: la asignación de clusters (que recorre todos los puntos) y el conteo/suma para nuevos centroides.

C
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <omp.h>

#define K 4
#define MAX_ITER 150
#define TOLERANCE 0.001 

typedef struct {
    double value;
    int cluster;
} DataPoint;

int main(int argc, char *argv[]) {
    if (argc < 2) return 1;

    FILE *file = fopen(argv[1], "r");
    if (!file) return 1;

    DataPoint *points = malloc(600000 * sizeof(DataPoint));
    char line[1024];
    int n = 0;

    fgets(line, 1024, file); 
    while (fgets(line, 1024, file)) {
        char *token = strtok(line, ","); 
        token = strtok(NULL, ",");       
        if (token && token[0] != '\n' && token[0] != '\r' && strcmp(token, "") != 0) {
            points[n].value = atof(token);
            points[n].cluster = -1;
            n++;
        }
    }
    fclose(file);

    double centroids[K] = {0.2, 1.0, 2.5, 4.5};
    int counts[K];
    int iterations = 0;
    int total_changes;

    printf("Iniciando K-Means PARALELO (OpenMP) para: %s\n", argv[1]);

    do {
        total_changes = 0;
        for (int j = 0; j < K; j++) counts[j] = 0;
        
        // --- PARALELIZACIÓN: ASIGNACIÓN ---
        #pragma omp parallel for reduction(+:total_changes)
        for (int i = 0; i < n; i++) {
            int best_k = 0;
            double min_dist = fabs(points[i].value - centroids[0]);
            for (int j = 1; j < K; j++) {
                double dist = fabs(points[i].value - centroids[j]);
                if (dist < min_dist) {
                    min_dist = dist;
                    best_k = j;
                }
            }
            if (points[i].cluster != best_k) {
                points[i].cluster = best_k;
                total_changes++;
            }
        }

        // --- PARALELIZACIÓN: ACTUALIZACIÓN (Reducción manual por clusters) ---
        double sums[K] = {0};
        int local_counts[K] = {0};

        #pragma omp parallel
        {
            double private_sums[K] = {0};
            int private_counts[K] = {0};

            #pragma omp for nowait
            for (int i = 0; i < n; i++) {
                private_sums[points[i].cluster] += points[i].value;
                private_counts[points[i].cluster]++;
            }

            #pragma omp critical
            {
                for (int j = 0; j < K; j++) {
                    sums[j] += private_sums[j];
                    local_counts[j] += private_counts[j];
                }
            }
        }

        printf("Iter %d: ", iterations);
        for (int j = 0; j < K; j++) {
            counts[j] = local_counts[j];
            if (counts[j] > 0) centroids[j] = sums[j] / counts[j];
            printf("[C%d: Val=%.2f, Cant=%d] ", j, centroids[j], counts[j]);
        }
        printf("| Cambios: %d\n", total_changes);

        iterations++;
    } while (iterations < MAX_ITER && (double)total_changes / n > TOLERANCE);

    printf("Finalizado en %d iteraciones.\n", iterations);
    free(points);
    return 0;
}
2. Herramientas de OpenMP utilizadas y no utilizadas
Utilizadas:

#pragma omp parallel for: Divide el bucle de asignación de puntos automáticamente entre los hilos disponibles.

reduction(+:total_changes): Crucial para evitar condiciones de carrera (race conditions) al contar cuántos puntos cambiaron de grupo. Cada hilo tiene su copia y se suman al final.

#pragma omp critical: Utilizada para consolidar las sumas de los centroides de forma segura, permitiendo que solo un hilo a la vez actualice los resultados globales.

private_sums / private_counts: Variables locales a cada hilo para maximizar la velocidad y evitar el uso excesivo de critical dentro de los bucles grandes.

No utilizadas:

sections: No se usó porque nuestro paralelismo es de datos (misma tarea sobre muchos datos), no de tareas distintas.

barrier: El final de los bucles #pragma omp for ya incluye una barrera implícita, por lo que no fue necesario añadirla manualmente.

atomic: Se prefirió critical o reducción manual para manejar arreglos (K), ya que atomic suele aplicarse solo a actualizaciones escalares simples.
