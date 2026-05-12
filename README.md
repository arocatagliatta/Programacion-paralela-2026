# K-Means Paralelo con OpenMP

Implementación del algoritmo K-Means en C para clasificar datos de varianza de
movimiento en 4 categorías. El proyecto incluye una versión **secuencial** y una
**paralela con OpenMP**, con benchmark automático para comparar su rendimiento.

## Estructura del proyecto

```text
.
├── CMakeLists.txt          ← sistema de build principal
├── Doxyfile                ← configuración de documentación Doxygen
├── src/
│   ├── main_seq.c          ← punto de entrada: versión secuencial
│   ├── main_omp.c          ← punto de entrada: versión paralela
│   ├── csv_reader.c        ← lectura de archivos CSV
│   ├── kmeans.c            ← algoritmo K-Means secuencial
│   └── kmeans_omp.c        ← algoritmo K-Means paralelo (OpenMP)
├── include/
│   ├── point.h             ← estructura DataPoint
│   ├── config.h            ← constantes K, MAX_ITER, TOLERANCE, centroides
│   ├── csv_reader.h        ← interfaz del lector CSV
│   ├── kmeans.h            ← interfaz del algoritmo secuencial
│   └── kmeans_omp.h        ← interfaz del algoritmo paralelo
├── data/
│   ├── movisA.csv          ← dataset VarianzaA (~500k registros)
│   └── movisB.csv          ← dataset VarianzaB (~500k registros)
├── scripts/
│   └── benchmark.py        ← benchmark unificado (mide speedup)
├── informe.md              ← informe técnico del TP
└── plan_refactorizacion.md ← plan de arquitectura
```

## Requisitos

| Herramienta | Versión mínima |
|---|---|
| GCC o Clang con soporte OpenMP | 9.x |
| CMake | 3.16 |
| Python | 3.8 |
| Doxygen (opcional) | 1.9 |

En sistemas Debian/Ubuntu:
```bash
sudo apt install gcc cmake python3 doxygen
```

En macOS con Homebrew:
```bash
brew install cmake libomp
```

## Compilación

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

En macOS con AppleClang, `cmake` detecta automáticamente `libomp` si está
instalado con Homebrew. Si no lo está, la configuración falla con un mensaje
indicando que primero ejecutes `brew install libomp`.

En Windows con MinGW, reemplazar `cmake ..` por:

```bash
cmake .. -G "MinGW Makefiles"
```

Los ejecutables quedan en `build/`:
- `build/kmeans_seq` (Linux/macOS) / `build/kmeans_seq.exe` (Windows)
- `build/kmeans_omp` (Linux/macOS) / `build/kmeans_omp.exe` (Windows)

> La carpeta `build/` se genera automáticamente y no se incluye en el repositorio.

## Ejecución manual

```bash
# Versión secuencial
./build/kmeans_seq data/movisA.csv
./build/kmeans_seq data/movisB.csv

# Versión paralela (usa todos los núcleos disponibles)
./build/kmeans_omp data/movisA.csv

# Versión paralela con cantidad de hilos específica y afinidad
OMP_NUM_THREADS=4 OMP_PROC_BIND=close OMP_PLACES=cores \
    ./build/kmeans_omp data/movisB.csv
```

## Benchmark

Compilar primero con CMake, luego:

```bash
python scripts/benchmark.py
```

El script ejecuta ambas versiones 10 veces cada una sobre los dos datasets,
con distintas cantidades de hilos (1, 2, 4, 8), y reporta:
- Tiempo promedio secuencial
- Tiempo promedio paralelo por configuración de hilos
- Speedup = T_secuencial / T_paralelo
- Eficiencia = Speedup / N_hilos × 100%

Ajustar `THREAD_CONFIGS` en `scripts/benchmark.py` según los núcleos
disponibles en la máquina.

## Documentación

Generar documentación HTML con Doxygen:

```bash
# Opción A: directamente
doxygen Doxyfile

# Opción B: desde CMake
cmake --build build --target docs
```

La documentación queda en `docs/html/index.html`.

## Parámetros del algoritmo

Todos los parámetros se centralizan en [include/config.h](include/config.h):

| Parámetro | Valor | Descripción |
|---|---|---|
| `K` | 4 | Número de clusters |
| `MAX_ITER` | 150 | Límite de iteraciones (fijado experimentalmente) |
| `TOLERANCE` | 0.001 | Convergencia: < 0,1% de puntos cambia de cluster |
| `MAX_POINTS` | 600.000 | Capacidad del buffer de datos |
| `INITIAL_CENTROIDS` | {0.2, 1.0, 2.5, 4.5} | Centroides iniciales estimados por observación |

## Datasets

Los archivos CSV tienen dos columnas separadas por coma:

```
timestamp,varianza
1234567890,0.312
...
```

Solo se procesa la segunda columna (valor de varianza de movimiento).
Los 4 clusters corresponden a 4 categorías de movimiento del sensor:
reposo, leve, moderado e intenso.

## Versiones del algoritmo

### `kmeans_seq` — Secuencial
Implementación estándar en un único hilo. Sirve como **línea base** para
calcular el speedup de la versión paralela.

### `kmeans_omp` — Paralelo con OpenMP
Paraleliza las dos fases de mayor costo O(n):
- **Asignación**: `#pragma omp parallel for reduction(+:total_changes)`
- **Acumulación**: reducción manual con variables privadas por hilo + `#pragma omp critical`

La flag `-fopenmp` la gestiona CMake automáticamente mediante
`target_link_libraries(kmeans_omp PRIVATE OpenMP::OpenMP_C)`.
