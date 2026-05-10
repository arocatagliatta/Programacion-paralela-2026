#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

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

    // Reservar memoria para ~600k puntos
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

    // Inicialización estimada (puedes ajustar estos valores según movisA o movisB)
    double centroids[K] = {0.2, 1.0, 2.5, 4.5};
    int counts[K] = {0};
    int iterations = 0;
    int total_changes;

    printf("Iniciando K-Means para: %s\n", argv[1]);

    do {
        total_changes = 0;
        for (int j = 0; j < K; j++) counts[j] = 0; // Reset conteo
        
        // Paso de Asignación
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
            counts[best_k]++;
        }

        // Paso de Actualización
        double sums[K] = {0};
        for (int i = 0; i < n; i++) {
            sums[points[i].cluster] += points[i].value;
        }

        printf("Iter %d: ", iterations);
        for (int j = 0; j < K; j++) {
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
