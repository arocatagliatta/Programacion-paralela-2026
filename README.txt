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
