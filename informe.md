# Informe del Trabajo Práctico: K-Means Paralelo con OpenMP

---

## 1. Introducción

Este informe documenta el desarrollo de un Trabajo Práctico de Programación Paralela que consiste en implementar el algoritmo de agrupamiento K-Means sobre datos reales de varianza de movimiento, utilizando la técnica de paralelismo **OpenMP** en lenguaje C.

El objetivo es doble: por un lado, aplicar el algoritmo correctamente y cumplir la consigna; por el otro, entender en profundidad qué significa paralelizar un programa, por qué sirve hacerlo y cuáles son las consideraciones técnicas involucradas.

Este documento está pensado para que alguien que todavía no domina OpenMP ni K-Means pueda leerlo, aprender desde cero y entender cómo cada parte del código cumple con los requisitos del enunciado.

---

## 2. Contexto del Trabajo Práctico

### La consigna

> Dados los archivos de VarianzaA y VarianzaB, aplicar el algoritmo de k-means sobre cada uno de estos. Deberá trabajar sobre 4 grupos, los que se corresponden estimativamente con 4 categorías de movimiento. Los valores iniciales pueden asignarlos sobre estimaciones de la media de cada categoría en lugar de aleatoriamente, acorde a una visualización de los valores, por ej: 0.5, 4, 9, 15.
>
> Utilice la técnica de paralelismo OpenMP.
>
> La finalización de la ejecución debe ser cuando el cambio de componentes de una categoría a otra no supere el 0,1% de los datos o cuando se alcance un umbral de iteraciones. El valor del umbral lo deberá fijar experimentalmente.

### Los datos

Los archivos `movisA.csv` y `movisB.csv` representan las varianzas de movimiento de un sensor (VarianzaA y VarianzaB). Cada archivo tiene dos columnas separadas por coma; la segunda columna contiene el valor de varianza que se usa como dato de entrada al algoritmo. El volumen de datos es del orden de los 500.000 registros por archivo.

### Lo que se entrega

| Ruta | Descripción |
|---|---|
| `src/kmeans.c` | Algoritmo K-Means secuencial (línea base) |
| `src/kmeans_omp.c` | Algoritmo K-Means paralelo con OpenMP |
| `src/csv_reader.c` | Lectura de archivos CSV |
| `src/main_seq.c` | Punto de entrada del ejecutable secuencial |
| `src/main_omp.c` | Punto de entrada del ejecutable paralelo |
| `include/` | Headers: `point.h`, `config.h`, `csv_reader.h`, `kmeans.h`, `kmeans_omp.h` |
| `CMakeLists.txt` | Sistema de build principal (compila ambos ejecutables) |
| `scripts/benchmark.py` | Benchmark unificado: mide speedup con distintos hilos |
| `data/movisA.csv` | Dataset VarianzaA |
| `data/movisB.csv` | Dataset VarianzaB |

---

## 3. ¿Qué es OpenMP?

**OpenMP** (Open Multi-Processing) es una API estándar para programación paralela en sistemas de **memoria compartida**. Fue diseñada para lenguajes como C, C++ y Fortran, y está soportada por los compiladores más populares (GCC, Clang, MSVC).

En términos simples: OpenMP permite que un programa use **varios núcleos de la CPU al mismo tiempo**, dividiendo el trabajo entre múltiples hilos de ejecución. Esto puede reducir drásticamente el tiempo de cómputo en tareas que procesan grandes volúmenes de datos.

OpenMP funciona mediante **directivas de preprocesador** (líneas que empiezan con `#pragma omp ...`) que le indican al compilador qué partes del código deben ejecutarse en paralelo. El programa es el mismo; lo que cambia es cómo el compilador genera el código ejecutable.

La forma recomendada de compilar el proyecto es con **CMake**, que detecta y aplica la bandera correcta automáticamente:

```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles"   # en Windows con MinGW
cmake --build .
```

Como referencia, la bandera equivalente aplicada manualmente es `-fopenmp`:

```bash
gcc -O3 -fopenmp src/kmeans_omp.c -o kmeans_omp -lm
```

Sin esa bandera, las directivas `#pragma omp` son ignoradas y el código se comporta como si fuera secuencial.

---

## 4. ¿Para Qué Sirve OpenMP?

OpenMP sirve para **explotar el paralelismo de datos**: cuando tenés una misma operación que se debe aplicar a muchos elementos independientes entre sí, OpenMP puede dividir esos elementos entre varios núcleos y procesarlos al mismo tiempo.

### Ejemplo concreto

Supongamos que tenés 500.000 datos y para cada uno necesitás calcular a cuál de los 4 centroides es más cercano. En una ejecución **secuencial**, un solo núcleo hace los 500.000 cálculos uno detrás del otro. Con OpenMP y 8 núcleos, cada núcleo procesa aproximadamente 62.500 datos en paralelo — el tiempo total se divide aproximadamente por 8.

### Cuándo OpenMP es adecuado

OpenMP es ideal cuando:
- El trabajo se puede dividir en partes **independientes** (que no dependan entre sí).
- Los datos están en **memoria compartida** (un solo proceso, múltiples hilos).
- Las operaciones sobre cada dato son **costosas** (si son triviales, el overhead de crear hilos supera el beneficio).

Este TP cumple los tres criterios: calcular la distancia de cada punto a los centroides es independiente para cada punto, todos los hilos ven el mismo array en RAM, y con 500.000 datos el cómputo es significativo.

---

## 5. Programación Paralela en Memoria Compartida

Existen dos grandes modelos de programación paralela:

### Memoria compartida
Todos los hilos (unidades de ejecución) pertenecen al **mismo proceso** y comparten el mismo espacio de memoria RAM. Pueden leer y escribir las mismas variables directamente. OpenMP trabaja bajo este modelo.

```
[ Proceso único ]
  ├── Hilo 0 → accede a points[0..62499]
  ├── Hilo 1 → accede a points[62500..124999]
  ├── Hilo 2 → accede a points[125000..187499]
  └── Hilo 3 → accede a points[187500..249999]
            ↓
    [ misma RAM, mismo array points[] ]
```

### Memoria distribuida
Varios procesos independientes, cada uno con su propia memoria. Para comunicarse, deben enviarse mensajes explícitamente (protocolo MPI). Es más complejo pero escala a clústeres con muchas máquinas.

En este TP se usa **memoria compartida** porque los datos caben en la RAM de una sola máquina y OpenMP es la herramienta indicada para aprovechar los múltiples núcleos del procesador.

---

## 6. Conceptos Básicos: Procesos, Hilos, Núcleos y Memoria Compartida

### Núcleo (core)
Es una unidad física de procesamiento dentro de la CPU. Una CPU moderna tiene múltiples núcleos (4, 8, 16...). Cada núcleo puede ejecutar instrucciones de forma independiente.

### Proceso
Es un programa en ejecución. Tiene su propio espacio de memoria, sus propias variables, su propio estado. Dos procesos distintos no comparten memoria directamente.

### Hilo (thread)
Es una unidad de ejecución **dentro de un proceso**. Un proceso puede tener múltiples hilos, y todos comparten la misma memoria. Es más liviano que un proceso porque no necesita duplicar el espacio de memoria.

### Relación
```
CPU (8 núcleos)
 └── Proceso kmeans_omp
       ├── Hilo 0  (corre en núcleo 0)
       ├── Hilo 1  (corre en núcleo 1)
       ├── Hilo 2  (corre en núcleo 2)
       └── Hilo 3  (corre en núcleo 3)
            todos comparten: points[], centroids[]
```

Con OpenMP, el programador indica qué secciones del código se ejecutan con múltiples hilos y cuáles de forma secuencial. La creación y gestión de los hilos la maneja OpenMP automáticamente.

---

## 7. Directivas Principales de OpenMP Usadas en el TP

### `#include <omp.h>`

```c
#include <omp.h>
```

Incluye la librería de OpenMP. Sin esta línea no se puede usar ninguna función ni directiva de OpenMP. Es el equivalente a incluir cualquier otra librería en C.

---

### `#pragma omp parallel for`

```c
#pragma omp parallel for reduction(+:total_changes)
for (int i = 0; i < n; i++) {
    // este código se ejecuta en paralelo
}
```

Esta es la directiva más usada. Le dice al compilador: *"dividí las iteraciones de este `for` entre todos los hilos disponibles"*. Si hay 8 hilos y `n = 500.000`, cada hilo procesa aproximadamente 62.500 iteraciones.

**Importante**: solo funciona correctamente si las iteraciones son **independientes** entre sí. En el paso de asignación, calcular el cluster del punto `i` no depende de los puntos `i-1` o `i+1`, por lo que la paralelización es segura.

---

### `reduction(+:total_changes)`

```c
#pragma omp parallel for reduction(+:total_changes)
for (int i = 0; i < n; i++) {
    if (points[i].cluster != best_k) {
        points[i].cluster = best_k;
        total_changes++;
    }
}
```

Imagina que 4 hilos están ejecutando el bucle al mismo tiempo. Todos quieren incrementar `total_changes++` cuando detectan un cambio. Si lo hacen sobre la misma variable sin coordinación, se produce una **condición de carrera** (ver sección 8).

`reduction(+:total_changes)` resuelve esto: cada hilo tiene su **propia copia local** de `total_changes`, la incrementa libremente, y al final OpenMP **suma todas las copias** en la variable original de forma segura. El programador no necesita hacer nada más.

---

### `#pragma omp parallel`

```c
#pragma omp parallel
{
    // este bloque se ejecuta en paralelo por todos los hilos
    double private_sums[K] = {0};
    int private_counts[K] = {0};
    // ...
}
```

A diferencia de `parallel for`, esta directiva crea un bloque de código que todos los hilos ejecutan completo. Se usa cuando querés que cada hilo tenga sus propias variables locales y luego coordine con los demás al final. Dentro de este bloque, las variables declaradas localmente son privadas a cada hilo.

---

### `#pragma omp for nowait`

```c
#pragma omp parallel
{
    double private_sums[K] = {0};

    #pragma omp for nowait
    for (int i = 0; i < n; i++) {
        private_sums[points[i].cluster] += points[i].value;
    }

    #pragma omp critical
    { /* consolidar */ }
}
```

Divide las iteraciones del `for` entre los hilos (igual que `parallel for`), pero el `nowait` elimina la **barrera implícita** al final del bucle. Normalmente, al terminar un `#pragma omp for`, todos los hilos esperan a que el último termine antes de continuar. Con `nowait`, el hilo que termina primero avanza directamente a la siguiente instrucción (`critical` en este caso).

---

### `#pragma omp critical`

```c
#pragma omp critical
{
    for (int j = 0; j < K; j++) {
        sums[j] += private_sums[j];
        local_counts[j] += private_counts[j];
    }
}
```

Garantiza que **solo un hilo a la vez** ejecuta el bloque de código encerrado. Es necesario cuando múltiples hilos necesitan modificar variables compartidas y no se puede usar `reduction` (por ejemplo, porque son arrays). El precio es que introduce una espera: los hilos se "hacen fila" para entrar a la sección crítica.

---

## 8. Problemas de Concurrencia: Race Conditions

Una **condición de carrera** (race condition) ocurre cuando dos o más hilos acceden a la misma variable compartida al mismo tiempo y al menos uno la modifica, produciendo resultados incorrectos e impredecibles.

### Ejemplo clásico

```
Hilo 0 lee total_changes = 5
Hilo 1 lee total_changes = 5    ← lee el mismo valor antes de que Hilo 0 escriba
Hilo 0 escribe total_changes = 6
Hilo 1 escribe total_changes = 6  ← debería ser 7, pero perdió un incremento
```

Si cuatro hilos hacen 1000 incrementos cada uno, el resultado debería ser 4000. Pero con una race condition podría ser 3200, 3800, o cualquier número menor — y varía en cada ejecución.

### Cómo se evitan en el TP

| Problema | Solución usada |
|---|---|
| `total_changes++` en paralelo | `reduction(+:total_changes)` |
| Actualizar `sums[]` y `counts[]` globales | `#pragma omp critical` con acumuladores locales por hilo |

---

## 9. ¿Qué es K-Means?

**K-Means** es un algoritmo de **agrupamiento no supervisado**. Su objetivo es dividir un conjunto de datos en `K` grupos (llamados clusters) de forma que los datos dentro de cada grupo sean lo más parecidos entre sí y lo más diferentes de los datos de otros grupos.

"No supervisado" significa que el algoritmo no tiene etiquetas previas para los datos — no sabe de antemano a qué grupo pertenece cada uno. Lo descubre por sí solo, basándose únicamente en la similitud numérica.

### ¿Qué representa un centroide?

Un **centroide** es el "centro" representativo de un cluster. Es simplemente el valor promedio de todos los datos que pertenecen a ese cluster. Si el cluster 2 tiene los valores {3.1, 3.5, 2.9, 4.0}, su centroide es (3.1+3.5+2.9+4.0)/4 = 3.375.

El centroide es un punto imaginario que resume a todo el grupo.

---

## 10. Funcionamiento General del Algoritmo K-Means

El algoritmo funciona en un ciclo iterativo de dos pasos:

### Pseudocódigo general

```
1. Inicializar K centroides (con valores estimados o aleatorios)

2. REPETIR:
   a. ASIGNACIÓN:
      Para cada punto de datos:
          Calcular la distancia al centroide más cercano
          Asignar el punto a ese cluster

   b. ACTUALIZACIÓN:
      Para cada cluster k:
          Nuevo centroide[k] = promedio de todos los puntos asignados a k

   c. Contar cuántos puntos cambiaron de cluster

3. HASTA QUE:
   - Los cambios sean < 0.1% del total de datos  (convergencia)
   - O se alcance el máximo de iteraciones       (límite experimental)

4. Reportar los clusters y centroides finales
```

### Paso de asignación

Cada punto se compara con los K centroides. Se calcula la "distancia" (en datos 1D, simplemente el valor absoluto de la diferencia) y el punto se asigna al cluster cuyo centroide esté más cerca:

```
Para el punto con value = 3.2 y centroides = {0.2, 1.0, 2.5, 4.5}:
  distancia a C0 = |3.2 - 0.2| = 3.0
  distancia a C1 = |3.2 - 1.0| = 2.2
  distancia a C2 = |3.2 - 2.5| = 0.7  ← mínima
  distancia a C3 = |3.2 - 4.5| = 1.3
→ El punto se asigna al cluster 2
```

### Paso de actualización

Una vez que todos los puntos están asignados, cada centroide se mueve al promedio de sus puntos:

```
Cluster 2 tiene los puntos {2.8, 3.1, 3.2, 2.9, 3.0}
Nuevo centroide[2] = (2.8 + 3.1 + 3.2 + 2.9 + 3.0) / 5 = 3.0
```

### Convergencia

El algoritmo "converge" cuando los centroides ya no se mueven significativamente — es decir, cuando casi ningún punto cambia de cluster entre una iteración y la siguiente. En este TP, el criterio es que los cambios sean menores al 0,1% del total de puntos.

---

## 11. K-Means Aplicado a VarianzaA y VarianzaB

Los datos de los archivos `movisA.csv` y `movisB.csv` representan la **varianza del movimiento** de un sensor en diferentes instantes de tiempo. La varianza es una medida de cuánto varía una señal — valores bajos indican poco movimiento (quietud) y valores altos indican mucho movimiento.

K-Means agrupa estos valores numéricos en 4 clusters que corresponden a **4 categorías de movimiento**:

| Cluster | Rango de varianza (aproximado) | Interpretación |
|---|---|---|
| 0 | ~0.2 | Sin movimiento / reposo |
| 1 | ~1.0 | Movimiento leve |
| 2 | ~2.5 | Movimiento moderado |
| 3 | ~4.5 | Movimiento intenso |

El algoritmo no sabe de antemano qué significa cada cluster — simplemente agrupa los valores por similitud numérica. La interpretación de "sin movimiento" o "intenso" la hacemos nosotros al analizar los resultados.

---

## 12. Relación Entre los 4 Grupos y las Categorías de Movimiento

El enunciado indica que los 4 grupos "se corresponden estimativamente con 4 categorías de movimiento". ¿Por qué 4?

Porque al visualizar los datos de varianza se observan naturalmente cuatro zonas de densidad. Los datos no se distribuyen de forma continua y uniforme, sino que se agrupan alrededor de ciertos valores típicos que corresponden a los distintos estados de movimiento del sensor.

Si se usaran menos clusters (por ejemplo 2), los grupos serían demasiado amplios y mezclarían movimientos distintos. Si se usaran más (por ejemplo 8), se fragmentarían categorías que en realidad son iguales.

`K = 4` es un compromiso basado en el conocimiento del dominio (el sensor detecta 4 estados de movimiento distintos) y en la observación de los datos.

---

## 13. Selección de Centroides Iniciales

El enunciado permite elegir los centroides iniciales "sobre estimaciones de la media de cada categoría en lugar de aleatoriamente", dando como ejemplo: `0.5, 4, 9, 15`.

### ¿Por qué no elegirlos al azar?

Si los centroides iniciales se eligen al azar, el algoritmo puede:
- Converger a una solución subóptima (mínimo local).
- Tardar muchas más iteraciones en converger.
- En casos extremos, dejar clusters vacíos.

### Centroides usados en el código

```c
double centroids[K] = {0.2, 1.0, 2.5, 4.5};
```

Estos valores se eligieron **mirando los datos** de los archivos CSV y estimando en qué zonas de varianza se concentran los datos de cada categoría. Al inicializar los centroides cerca de los promedios reales, el algoritmo converge en pocas iteraciones.

Esta es una forma válida y recomendada de inicialización cuando se tiene conocimiento previo del dominio.

---

## 14. Explicación de la Implementación Secuencial

El archivo [src/kmeans.c](src/kmeans.c) es la versión **sin paralelismo**. Sirve como línea base para medir el tiempo de referencia y compararlo con la versión OpenMP.

### Lectura del CSV

```c
FILE *file = fopen(argv[1], "r");    // abre el archivo pasado como argumento
fgets(line, 1024, file);             // descarta la primera línea (cabecera)
while (fgets(line, 1024, file)) {    // lee línea por línea
    char *token = strtok(line, ","); // extrae el primer campo (lo ignora)
    token = strtok(NULL, ",");       // extrae el segundo campo (valor de varianza)
    points[n].value = atof(token);   // convierte a double
    points[n].cluster = -1;          // sin cluster asignado aún
    n++;
}
```

El CSV tiene dos columnas: la primera es ignorada, la segunda es el valor de varianza que se procesa.

### Paso de asignación (secuencial)

```c
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
```

Para cada punto, se recorren los 4 centroides y se elige el más cercano (menor distancia absoluta). Si el cluster cambió, se incrementa `total_changes`.

### Paso de actualización (secuencial)

```c
double sums[K] = {0};
for (int i = 0; i < n; i++) {
    sums[points[i].cluster] += points[i].value;
}
for (int j = 0; j < K; j++) {
    if (counts[j] > 0) centroids[j] = sums[j] / counts[j];
}
```

Se suma el valor de todos los puntos de cada cluster y se divide por la cantidad para obtener el nuevo centroide.

### Condición de parada

```c
} while (iterations < MAX_ITER && (double)total_changes / n > TOLERANCE);
```

El loop continúa mientras no se supere el límite de iteraciones **Y** el porcentaje de cambios siga siendo mayor al 0,1%.

---

## 15. Explicación de la Implementación Paralela con OpenMP

El archivo [src/kmeans_omp.c](src/kmeans_omp.c) agrega paralelismo a los dos pasos más costosos del algoritmo.

### Paso de asignación paralela

```c
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
```

La directiva `parallel for` divide automáticamente las `n` iteraciones entre los hilos. Cada hilo calcula el cluster de su subconjunto de puntos. Como `total_changes` es compartida, se usa `reduction` para evitar la race condition.

**Por qué es seguro**: el punto `i` solo escribe en `points[i].cluster`. Los distintos hilos acceden a índices distintos, así que no hay conflicto en el array `points[]`.

### Pseudocódigo paralelo del paso de asignación

```
PARALELO (cada hilo procesa su rango de puntos):
  Para i desde mi_inicio hasta mi_fin:
    best_k = cluster más cercano para points[i]
    Si points[i].cluster != best_k:
      points[i].cluster = best_k
      mi_total_changes++    ← variable local del hilo

AL FINAL: total_changes = suma de todos los mi_total_changes (reduction)
```

### Paso de actualización paralela

```c
#pragma omp parallel
{
    double private_sums[K] = {0};    // local al hilo, sin riesgo de carrera
    int private_counts[K] = {0};

    #pragma omp for nowait
    for (int i = 0; i < n; i++) {
        private_sums[points[i].cluster] += points[i].value;
        private_counts[points[i].cluster]++;
    }

    #pragma omp critical              // un hilo a la vez consolida
    {
        for (int j = 0; j < K; j++) {
            sums[j] += private_sums[j];
            local_counts[j] += private_counts[j];
        }
    }
}
```

No se puede usar `reduction` sobre arrays en C estándar con OpenMP, así que se implementa la reducción manualmente:

1. Cada hilo acumula en sus propios arrays `private_sums` y `private_counts`.
2. Al final, cada hilo entra (de a uno) a la sección crítica y suma sus parciales al total global.

### Pseudocódigo paralelo del paso de actualización

```
PARALELO (cada hilo tiene su private_sums[] y private_counts[]):
  Para i desde mi_inicio hasta mi_fin:
    private_sums[cluster_de_points[i]] += points[i].value
    private_counts[cluster_de_points[i]]++

  CRITICO (un hilo a la vez):
    Para j de 0 a K-1:
      sums[j] += private_sums[j]
      counts[j] += private_counts[j]

SECUENCIAL:
  Para j de 0 a K-1:
    centroids[j] = sums[j] / counts[j]
```

---

## 16. Condición de Finalización

### Tolerancia del 0,1%

```c
#define TOLERANCE 0.001
```

```c
(double)total_changes / n > TOLERANCE
```

`total_changes` es la cantidad absoluta de puntos que cambiaron de cluster en la última iteración. Al dividirlo por `n` (total de puntos) se obtiene la **proporción** de cambios. Si esa proporción supera `0.001`, el algoritmo todavía no convergió.

`0.001` representa exactamente el `0,1%`:
```
0.001 = 1/1000 = 0.1/100 = 0.1%
```

Es decir: si menos del 0,1% de los datos cambia de cluster, se considera que el algoritmo ya convergió y se detiene aunque no haya llegado al máximo de iteraciones.

### Máximo de iteraciones (umbral experimental)

```c
#define MAX_ITER 150
```

El enunciado pide fijar este valor **experimentalmente**. El proceso fue:
1. Ejecutar el algoritmo con un umbral alto (ej: 1000 iteraciones).
2. Observar en cuántas iteraciones típicamente converge con los datos reales.
3. Fijar `MAX_ITER = 150` como valor seguro que cubre todos los casos observados sin ser excesivamente alto.

Este valor actúa como **red de seguridad**: si por alguna razón la tolerancia no se cumple (datos atípicos, inicialización subóptima), el programa igual termina en un tiempo razonable.

### La condición completa

```c
} while (iterations < MAX_ITER && (double)total_changes / n > TOLERANCE);
```

El loop continúa **solo si se cumplen ambas condiciones simultáneamente**:
- No se llegó al máximo de iteraciones.
- El cambio proporcional todavía supera el 0,1%.

Se detiene en cuanto cualquiera de las dos falla (convergencia o límite).

---

## 17. Benchmark y Comparación de Rendimiento

### ¿Qué mide el benchmark?

El script `scripts/benchmark.py` ejecuta ambas versiones (`kmeans_seq` y `kmeans_omp`) varias veces sobre los dos datasets y registra el tiempo de ejecución de cada corrida. Se prueba con distintas cantidades de hilos (1, 2, 4, 8) usando `OMP_NUM_THREADS`. Luego calcula:

- **Tiempo promedio secuencial**: cuánto tarda en promedio `kmeans_seq` (un solo núcleo).
- **Tiempo promedio paralelo**: cuánto tarda en promedio `kmeans_omp` por configuración de hilos.
- **Speedup**: cuántas veces más rápida es la versión paralela.
- **Eficiencia**: Speedup / N_hilos × 100%.

### ¿Qué es el Speedup?

El **speedup** es la relación entre el tiempo secuencial y el paralelo:

```
Speedup = Tiempo_secuencial / Tiempo_paralelo
```

Un speedup de 4.0 significa que la versión paralela es 4 veces más rápida que la secuencial.

### ¿Por qué el speedup mejora el tiempo?

Porque el trabajo se distribuye entre múltiples núcleos. Si el tiempo de asignación de clusters con 1 núcleo es `T`, con `P` núcleos idealmente sería `T/P`.

### ¿Por qué el speedup no siempre es perfecto?

El speedup **nunca** es igual al número de núcleos por varias razones:

1. **Partes secuenciales**: Leer el CSV, inicializar los centroides y actualizar los centroides finales se hace de forma secuencial. La **Ley de Amdahl** dice que si el 20% del programa es secuencial, el speedup máximo posible es 5, sin importar cuántos núcleos se usen.

2. **Overhead de OpenMP**: Crear y destruir hilos, sincronizarlos (`critical`, barreras) tiene un costo de tiempo propio.

3. **Conflictos de caché**: Cuando múltiples núcleos acceden a la misma zona de memoria, compiten por el caché del procesador (cache coherency).

4. **Balance de carga**: Si los hilos no reciben exactamente la misma cantidad de trabajo, los más cargados hacen esperar a los demás.

5. **Hyperthreading**: Si el número de hilos supera los núcleos físicos, los hilos comparten núcleos y el speedup adicional es menor.

### Tabla de comparación tipo

| Métrica | Valor típico (estimado) |
|---|---|
| Tiempo secuencial (movisA) | ~X segundos |
| Tiempo paralelo (4 hilos) | ~X/3 a X/4 segundos |
| Speedup | 3x a 4x |
| Speedup teórico (Amdahl) | Depende de % paralelo |

---

## 18. Tabla de Cumplimiento de la Consigna

| Requisito del TP | Cómo se cumple en el código | Explicación |
|---|---|---|
| Aplicar K-Means sobre VarianzaA y VarianzaB | `./kmeans_omp data/movisA.csv` y `./kmeans_omp data/movisB.csv` | El archivo se pasa como argumento; el mismo ejecutable sirve para ambos |
| 4 grupos | `#define K 4` ([include/config.h](include/config.h)) | Constante centralizada que controla todo el algoritmo |
| Valores iniciales estimados (no aleatorios) | `static const double INITIAL_CENTROIDS[K] = {0.2, 1.0, 2.5, 4.5};` ([include/config.h](include/config.h)) | Valores elegidos mirando los datos, no al azar |
| Usar paralelismo OpenMP | `#include <omp.h>` + directivas `#pragma omp` en [src/kmeans_omp.c](src/kmeans_omp.c) | Todo el procesamiento pesado está paralelizado |
| Parar cuando cambios < 0,1% | `(double)total_changes / n > TOLERANCE` con `TOLERANCE = 0.001` ([include/config.h](include/config.h)) | 0.001 = 0.1%, se evalúa al final de cada iteración |
| O al alcanzar umbral de iteraciones | `iterations < MAX_ITER` con `MAX_ITER = 150` ([include/config.h](include/config.h)) | Valor fijado experimentalmente con el benchmark |
| Umbral fijado experimentalmente | `MAX_ITER = 150` determinado observando convergencia real | Se probaron distintos valores; 150 cubre todos los casos |

---

## 19. Arquitectura Modular Implementada

El proyecto está organizado en módulos con responsabilidades separadas, compilados y enlazados mediante **CMakeLists.txt**.

### Estructura de carpetas

```text
.
├── CMakeLists.txt          ← sistema de build principal
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
│   ├── movisA.csv
│   └── movisB.csv
└── scripts/
    └── benchmark.py        ← benchmark unificado (mide speedup y eficiencia)
```

La carpeta `build/` se genera automáticamente al compilar con CMake y **no forma parte del código fuente**. No se incluye en el repositorio.

### Responsabilidad de cada módulo

**`include/point.h`** — Estructura de datos central
```c
typedef struct {
    double value;   /* valor de varianza leído del CSV */
    int    cluster; /* índice del cluster asignado (0..K-1) */
} DataPoint;
```
Define el tipo de dato una sola vez. Todos los demás módulos la incluyen.

**`include/config.h`** — Configuración centralizada
```c
#define K           4
#define MAX_ITER    150
#define TOLERANCE   0.001
#define MAX_POINTS  600000
static const double INITIAL_CENTROIDS[K] = {0.2, 1.0, 2.5, 4.5};
```
Un único lugar para cambiar los parámetros del algoritmo. La declaración `static const` evita errores de símbolo duplicado cuando el header se incluye en múltiples unidades de compilación.

**`src/csv_reader.c` / `include/csv_reader.h`** — Entrada de datos
```c
int csv_read(const char *filename, DataPoint *points, int max_points);
```
Responsabilidad única: leer un CSV y devolver los puntos cargados. No sabe nada del algoritmo.

**`src/kmeans.c` / `include/kmeans.h`** — Algoritmo secuencial
```c
void kmeans_run(DataPoint *points, int n, double *centroids,
                int k, int max_iter, double tolerance);
```
Solo el algoritmo K-Means sin paralelismo. Sirve como línea base para el benchmark.

**`src/kmeans_omp.c` / `include/kmeans_omp.h`** — Algoritmo paralelo
```c
void kmeans_omp_run(DataPoint *points, int n, double *centroids,
                    int k, int max_iter, double tolerance);
```
Misma interfaz que la versión secuencial. La diferencia es interna: usa `#pragma omp` para paralelizar los dos pasos de mayor costo O(n).

**`src/main_seq.c` y `src/main_omp.c`** — Orquestación
```c
int main(int argc, char *argv[]) {
    DataPoint *points = malloc(MAX_POINTS * sizeof(DataPoint));
    int n = csv_read(argv[1], points, MAX_POINTS);
    double centroids[K];
    memcpy(centroids, INITIAL_CENTROIDS, K * sizeof(double));
    kmeans_omp_run(points, n, centroids, K, MAX_ITER, TOLERANCE);
    free(points);
    return 0;
}
```
Solo coordina: lee los datos, llama al algoritmo, libera memoria. No tiene lógica propia.

---

## 20. Integración con CMake y OpenMP

CMake es el sistema de build del proyecto. El archivo `CMakeLists.txt` define dos ejecutables y gestiona todas las flags de compilación:

```cmake
find_package(OpenMP REQUIRED)

add_executable(kmeans_seq src/main_seq.c src/csv_reader.c src/kmeans.c)
target_compile_options(kmeans_seq PRIVATE -O3 -Wall -Wextra)
target_link_libraries(kmeans_seq PRIVATE m)

add_executable(kmeans_omp src/main_omp.c src/csv_reader.c src/kmeans_omp.c)
target_compile_options(kmeans_omp PRIVATE -O3 -Wall -Wextra)
target_link_libraries(kmeans_omp PRIVATE OpenMP::OpenMP_C m)
```

Puntos clave:

- `find_package(OpenMP REQUIRED)` detecta automáticamente la flag correcta según el compilador (`-fopenmp` en GCC, `/openmp` en MSVC).
- `OpenMP::OpenMP_C` aplica la flag **solo** a `kmeans_omp`, no a `kmeans_seq`. Así se garantiza que la versión secuencial no incluye código OpenMP.
- `-O3` activa optimizaciones agresivas, lo que reduce el tiempo base y hace el benchmark más representativo.
- `-lm` enlaza la librería matemática (necesaria para `fabs()`).
- La carpeta `build/` generada por CMake contiene los archivos intermedios y los ejecutables finales. No se versiona.

---

## 21. Principios de Ingeniería de Software Aplicados

### Bajo acoplamiento
Los módulos no dependen de los detalles internos de los demás. `main.c` solo conoce la interfaz de `csv_reader.h` y `kmeans.h`, no cómo están implementados internamente. Si se cambia la implementación de la lectura del CSV, `main.c` no necesita modificarse.

### Alta cohesión
Cada módulo tiene una responsabilidad clara y acotada. `csv_reader.c` solo lee datos; `kmeans.c` solo ejecuta el algoritmo.

### Separación de responsabilidades (SoC)
La entrada de datos (CSV), el algoritmo (K-Means) y la presentación de resultados (`printf`) son tres responsabilidades distintas que no deben mezclarse en el mismo código.

### Evitar valores mágicos
En lugar de escribir `0.001` o `150` directamente en el código, se definen como constantes nombradas en `config.h`. Así el código se auto-documenta y los cambios son locales.

### Buenas prácticas en C para este TP

| Práctica | Aplicación |
|---|---|
| Validar argumentos | `if (argc < 2) return 1;` |
| Verificar apertura de archivo | `if (!file) return 1;` |
| Liberar memoria | `free(points);` al final |
| Funciones pequeñas | Cada función hace una sola cosa |
| Constantes nombradas | `#define K 4` en lugar de `4` suelto |
| Separar algoritmo de I/O | No mezclar `printf` con el loop principal |

### Qué sí conviene abstraer y qué no

**Conviene abstraer:**
- La lectura del CSV (puede cambiar el formato, la codificación, el separador).
- La función de distancia (para datos multidimensionales habría que cambiarla).
- Los parámetros de configuración.

**No conviene sobreabstraer:**
- El loop principal de K-Means: es simple y claro como está.
- La condición de parada: una sola línea, no justifica función propia.
- La actualización de centroides: pocas líneas, extraerla no agrega claridad.

La regla práctica: si una función tiene más de una responsabilidad o más de 40-50 líneas, considerar separarla. Si se llama desde un solo lugar y no tiene lógica propia, probablemente no valga.

---

## 22. Pseudocódigo del Algoritmo K-Means (Secuencial)

```
FUNCIÓN kmeans(points[], n, K, centroids[], MAX_ITER, TOLERANCE):
    iterations = 0
    
    REPETIR:
        total_changes = 0
        counts[] = {0, 0, 0, 0}
        
        // PASO 1: ASIGNACIÓN
        PARA i = 0 hasta n-1:
            min_dist = INFINITO
            best_k = -1
            
            PARA j = 0 hasta K-1:
                dist = |points[i].value - centroids[j]|
                SI dist < min_dist:
                    min_dist = dist
                    best_k = j
            
            SI points[i].cluster != best_k:
                points[i].cluster = best_k
                total_changes++
            
            counts[best_k]++
        
        // PASO 2: ACTUALIZACIÓN
        sums[] = {0, 0, 0, 0}
        PARA i = 0 hasta n-1:
            sums[points[i].cluster] += points[i].value
        
        PARA j = 0 hasta K-1:
            SI counts[j] > 0:
                centroids[j] = sums[j] / counts[j]
        
        iterations++
    
    HASTA QUE: iterations >= MAX_ITER OR total_changes/n <= TOLERANCE
    
    RETORNAR centroids[], clusters asignados en points[]
```

---

## 23. Pseudocódigo de la Versión Paralela con OpenMP

```
FUNCIÓN kmeans_omp(points[], n, K, centroids[], MAX_ITER, TOLERANCE):
    iterations = 0
    
    REPETIR:
        total_changes = 0
        
        // PASO 1: ASIGNACIÓN — PARALELO
        PARALELO PARA i = 0 hasta n-1 (dividido entre hilos):
            [cada hilo trabaja en su subconjunto independientemente]
            best_k = índice del centroide más cercano a points[i].value
            
            SI points[i].cluster != best_k:
                points[i].cluster = best_k
                mi_total_changes++    [variable local del hilo]
        
        [AL TERMINAR: total_changes = SUMA de todos los mi_total_changes]
        
        // PASO 2: ACTUALIZACIÓN — PARALELO con reducción manual
        PARALELO (cada hilo crea private_sums[] y private_counts[] locales):
        
            PARA i = 0 hasta n-1 (subconjunto del hilo):
                private_sums[points[i].cluster] += points[i].value
                private_counts[points[i].cluster]++
            
            CRÍTICO (un hilo a la vez):
                PARA j = 0 hasta K-1:
                    sums[j] += private_sums[j]
                    counts[j] += private_counts[j]
        
        // SECUENCIAL: calcular nuevos centroides
        PARA j = 0 hasta K-1:
            SI counts[j] > 0:
                centroids[j] = sums[j] / counts[j]
        
        iterations++
    
    HASTA QUE: iterations >= MAX_ITER OR total_changes/n <= TOLERANCE
```

---

## 24. Preguntas Técnicas Frecuentes

A continuación se resumen preguntas técnicas habituales sobre la implementación, junto con respuestas breves de referencia.

---

**¿Por qué usaron OpenMP y no MPI o pthreads?**

> OpenMP es la herramienta más adecuada para paralelismo en memoria compartida en C. Para este problema todos los datos caben en la RAM de una sola máquina, así que no necesitamos distribuir datos entre nodos como haría MPI. pthreads es de más bajo nivel y requeriría gestionar manualmente la creación, sincronización y destrucción de hilos — OpenMP abstrae todo eso con directivas simples, lo que reduce el riesgo de errores y el código necesario.

---

**¿Qué parte del algoritmo paralelizaron y por qué esa?**

> Paralelizamos las dos partes computacionalmente más pesadas: el paso de asignación (calcular el cluster de cada punto) y el paso de acumulación para actualizar los centroides. Ambas partes tienen complejidad O(n), donde n es la cantidad de puntos (~500.000). Son ideales para paralelizar porque cada punto se procesa independientemente de los demás — no hay dependencias entre iteraciones del bucle.

---

**¿Por qué hace falta `reduction` en `total_changes`?**

> Porque `total_changes` es una variable compartida que múltiples hilos intentan incrementar simultáneamente. Sin `reduction`, dos hilos podrían leer el valor en el mismo instante (por ejemplo, ambos leen `5`), incrementarlo localmente (ambos obtienen `6`) y escribirlo (`6` en lugar de `7`). Perdemos un incremento — es una condición de carrera. Con `reduction(+:total_changes)`, cada hilo tiene su propia copia local, la incrementa libremente sin conflictos, y OpenMP suma todas las copias al final de forma segura y atómica.

---

**¿Por qué se usa `critical` en lugar de `reduction` para los centroides?**

> `reduction` en OpenMP C solo funciona directamente con variables escalares (un solo número), no con arrays. Los centroides son arrays de tamaño K. Para hacer la reducción de forma segura sobre arrays, implementamos la reducción manualmente: cada hilo acumula en sus propios arrays `private_sums` y `private_counts` (sin riesgo de carrera porque son locales a cada hilo), y luego cada hilo usa `critical` para sumar sus parciales al total global de a uno por vez.

---

**¿Qué representa el 0,1%? ¿Por qué `TOLERANCE = 0.001`?**

> El 0,1% en fracción decimal es 0,1/100 = 0,001. En el código, `total_changes / n` da la proporción de puntos que cambiaron de cluster. Cuando esa proporción es menor o igual a 0,001 (es decir, 0,1%), consideramos que el algoritmo convergió — casi ningún punto cambia de grupo, los centroides ya no se van a mover significativamente.

---

**¿Por qué eligieron `MAX_ITER = 150`?**

> Lo fijamos experimentalmente. Corrimos el algoritmo con los datos reales usando un límite alto (1000 iteraciones) y observamos cuántas iteraciones tomaba hasta converger. En todos los casos observamos convergencia antes de las 80-100 iteraciones. Elegimos 150 como valor de seguridad que garantiza que el programa siempre termina, incluso en casos con inicialización menos favorable.

---

**¿Por qué se usan 4 clusters y no 3 o 5?**

> El enunciado indica que los datos corresponden a 4 categorías de movimiento. Al visualizar los datos, se observan cuatro zonas de concentración de valores (alrededor de ~0.2, ~1.0, ~2.5 y ~4.5). Con 3 clusters dos categorías reales quedarían mezcladas; con 5 se fragmentaría una categoría real en dos partes artificiales. 4 clusters es el número que mejor captura la estructura natural de los datos.

---

**¿Qué es un centroide?**

> Un centroide es el "centro de masa" o valor representativo de un cluster. Matemáticamente es el promedio aritmético de todos los puntos asignados a ese cluster. Por ejemplo, si el cluster 2 tiene los valores {2.8, 3.0, 3.2}, su centroide es (2.8+3.0+3.2)/3 = 3.0. El centroide representa a todo el grupo con un solo número.

---

**¿Qué pasa si los centroides iniciales son malos?**

> Si los centroides iniciales están todos concentrados en una misma zona de los datos, el algoritmo puede converger a una solución subóptima donde algunos clusters quedan vacíos o dos clusters capturan lo que debería ser un solo grupo. También puede tomar muchas más iteraciones converger. Por eso elegimos los centroides iniciales basándonos en la observación de los datos, iniciando uno en cada zona de concentración natural.

---

**¿Por qué hay una versión secuencial si la consigna pide paralela?**

> La versión secuencial sirve como **línea base** para el benchmark. Sin un punto de referencia, no podemos medir si la versión paralela es más rápida ni cuánto. El speedup se calcula dividiendo el tiempo secuencial por el tiempo paralelo. Sin `kmeans.c` no tendríamos esa referencia y no podríamos demostrar experimentalmente la mejora que aporta OpenMP.

---

**¿Qué significa speedup? ¿Siempre es igual al número de hilos?**

> Speedup es la relación entre el tiempo secuencial y el paralelo: `S = T_sec / T_par`. Un speedup de 4 significa que la versión paralela es 4 veces más rápida. No siempre es igual al número de hilos porque: (1) parte del programa es secuencial y no se puede paralelizar (Ley de Amdahl), (2) crear y sincronizar hilos tiene un costo propio, (3) pueden surgir conflictos de caché entre núcleos, y (4) si hay más hilos que núcleos físicos, los hilos compiten por los mismos recursos.

---

**¿Por qué usaron distancia absoluta y no distancia euclidiana?**

> Porque los datos son unidimensionales — cada punto es un solo número (el valor de varianza). En una dimensión, la distancia euclidiana y el valor absoluto de la diferencia son equivalentes: `|a - b| = sqrt((a-b)²)`. La distancia absoluta es computacionalmente más simple (evita calcular raíces cuadradas) y produce exactamente el mismo resultado de agrupamiento.

---

## 25. Conclusión

Este trabajo práctico integra tres conceptos fundamentales de la programación paralela moderna:

1. **El algoritmo K-Means** como ejemplo clásico de problema de clustering, con convergencia iterativa y criterio de parada cuantificable.

2. **OpenMP** como herramienta para explotar el paralelismo de datos en memoria compartida, aprovechando los múltiples núcleos de una CPU estándar sin necesidad de infraestructura distribuida.

3. **El diseño del código en C**, con atención a las condiciones de carrera, las técnicas de reducción y la sincronización con secciones críticas.

El código cumple todos los requisitos de la consigna: aplica K-Means sobre ambos datasets, usa 4 clusters con centroides iniciales estimados, emplea OpenMP en las partes computacionalmente costosas, y se detiene por convergencia (0,1%) o por el umbral de 150 iteraciones determinado experimentalmente.

Desde el punto de vista de ingeniería de software, el proyecto adopta una arquitectura modular: el código fuente está separado en `src/`, los headers en `include/`, los datasets en `data/`, y el benchmark en `scripts/`. La compilación oficial se realiza con CMake, que gestiona flags, dependencias y la integración con OpenMP de forma portable.

Los resultados experimentales confirman que la paralelización con OpenMP mejora de forma clara el tiempo de ejecución en ambos datasets. En `movisA.csv`, la versión secuencial promedió `0.1970 s`, mientras que la versión paralela alcanzó speedups de `1.60x` con 1 hilo, `2.81x` con 2 hilos, `3.57x` con 4 hilos y `3.58x` con 8 hilos. En `movisB.csv`, la versión secuencial promedió `0.1487 s`, y la versión paralela obtuvo `1.13x`, `1.64x`, `2.16x` y `2.18x` de speedup para 1, 2, 4 y 8 hilos respectivamente.

Estos valores muestran dos comportamientos importantes. Por un lado, la implementación paralela logra una aceleración efectiva incluso con pocos hilos, lo que valida la elección de OpenMP para este problema. Por otro lado, la mejora deja de crecer de manera significativa al pasar de 4 a 8 hilos, especialmente en `movisA.csv`, lo que refleja los límites esperables de la paralelización real: costo de sincronización, fracción secuencial del algoritmo, competencia por caché y saturación de los recursos de hardware.

En síntesis, el benchmark no solo verifica que la solución paralela funciona correctamente, sino que también demuestra una mejora medible y consistente respecto de la versión secuencial, conectando la teoría del paralelismo con evidencia experimental concreta.
