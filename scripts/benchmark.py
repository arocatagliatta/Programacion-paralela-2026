"""
benchmark.py — Benchmark de rendimiento: K-Means Secuencial vs. Paralelo (OpenMP)
===================================================================================

Mide y compara los tiempos de ejecución de kmeans_seq y kmeans_omp sobre los
datasets de varianza de movimiento. Calcula el speedup para distintas
cantidades de hilos.

IMPORTANTE: Los ejecutables deben compilarse con CMake ANTES de correr este
script. El script no compila — eso es responsabilidad de CMake.

Compilar:
    mkdir build && cd build && cmake .. && cmake --build .

Ejecutar:
    python scripts/benchmark.py

Ajustar THREAD_CONFIGS según los núcleos disponibles en la máquina.
"""

import subprocess
import time
import os
import sys

# ── Rutas ─────────────────────────────────────────────────────────────────────
BASE_DIR  = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD_DIR = os.path.join(BASE_DIR, "build")
DATA_DIR  = os.path.join(BASE_DIR, "data")

# Nombres de ejecutables (agrega .exe en Windows automáticamente si hace falta)
def exe_path(name):
    path = os.path.join(BUILD_DIR, name)
    if not os.path.exists(path) and os.path.exists(path + ".exe"):
        return path + ".exe"
    return path

SEQ_EXE = exe_path("kmeans_seq")
OMP_EXE = exe_path("kmeans_omp")

DATASETS = [
    os.path.join(DATA_DIR, "movisA.csv"),
    os.path.join(DATA_DIR, "movisB.csv"),
]

# Configuraciones de hilos a probar — ajustar según núcleos del sistema
THREAD_CONFIGS = [1, 2, 4, 8]

RUNS = 10  # Repeticiones por medición (la primera muestra salida, las demás son silenciosas)


# ── Funciones ─────────────────────────────────────────────────────────────────

def verificar_ejecutables():
    """Verifica que los ejecutables compilados existen. Aborta con mensaje claro si no."""
    faltantes = [e for e in (SEQ_EXE, OMP_EXE) if not os.path.exists(e)]
    if faltantes:
        print("\nERROR: Ejecutables no encontrados:")
        for f in faltantes:
            print(f"  {f}")
        print("\nCompilar primero con CMake:")
        print("  mkdir build")
        print("  cd build")
        print("  cmake ..")
        print("  cmake --build .")
        sys.exit(1)


def run_timed(exe, dataset, num_threads=None, runs=RUNS):
    """
    Ejecuta un binario sobre un dataset varias veces y devuelve el tiempo promedio.

    La primera ejecución muestra la salida del programa (para verificar
    centroides y convergencia). Las siguientes son silenciosas.

    Args:
        exe (str): Ruta al ejecutable.
        dataset (str): Ruta al archivo CSV.
        num_threads (int | None): Valor de OMP_NUM_THREADS. None = usar default del sistema.
        runs (int): Número de repeticiones.

    Returns:
        float: Tiempo promedio en segundos, o None si el ejecutable falló.
    """
    env = os.environ.copy()
    if num_threads is not None:
        env["OMP_NUM_THREADS"] = str(num_threads)
        env["OMP_PROC_BIND"]   = "close"
        env["OMP_PLACES"]      = "cores"

    nombre_exe = os.path.basename(exe)
    nombre_csv = os.path.basename(dataset)
    hilos_label = f"{num_threads} hilos" if num_threads is not None else "hilos del sistema"

    print(f"\n{'='*65}")
    print(f"  {nombre_exe} | {nombre_csv} | {hilos_label}")
    print(f"{'='*65}")

    tiempos = []
    for i in range(runs):
        es_primera = (i == 0)
        start = time.perf_counter()
        resultado = subprocess.run(
            [exe, dataset],
            env=env,
            capture_output=not es_primera,
            text=True
        )
        elapsed = time.perf_counter() - start

        if resultado.returncode != 0:
            print(f"  ERROR en ejecución {i+1} (código {resultado.returncode})")
            return None

        tiempos.append(elapsed)

        if es_primera and resultado.stdout:
            print(resultado.stdout, end="")
        elif not es_primera:
            print(f"  Run {i+1:02d}: {elapsed:.4f}s")

    promedio = sum(tiempos) / runs
    print(f"\n  Promedio ({runs} runs): {promedio:.6f}s")
    return promedio


def imprimir_resumen(resultados):
    """Imprime tabla de speedup al final del benchmark."""
    print(f"\n{'='*65}")
    print("  RESUMEN DE SPEEDUP")
    print(f"{'='*65}")

    for dataset, datos in resultados.items():
        nombre = os.path.basename(dataset)
        t_seq  = datos.get("seq")
        if t_seq is None:
            continue

        print(f"\n  Dataset: {nombre}")
        print(f"    Secuencial:    {t_seq:.4f}s")

        for hilos, t_omp in sorted(datos.get("omp", {}).items()):
            if t_omp is None:
                print(f"    {hilos:2d} hilos (OMP): ERROR")
            else:
                speedup = t_seq / t_omp
                eficiencia = speedup / hilos * 100
                print(f"    {hilos:2d} hilos (OMP): {t_omp:.4f}s  "
                      f"→ speedup: {speedup:.2f}x  eficiencia: {eficiencia:.1f}%")


# ── Main ──────────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    verificar_ejecutables()

    resultados = {}

    for dataset in DATASETS:
        if not os.path.exists(dataset):
            print(f"\nAVISO: dataset no encontrado, se omite: {dataset}")
            continue

        resultados[dataset] = {"seq": None, "omp": {}}

        # Medir versión secuencial
        t_seq = run_timed(SEQ_EXE, dataset)
        resultados[dataset]["seq"] = t_seq

        # Medir versión paralela con distintas cantidades de hilos
        for hilos in THREAD_CONFIGS:
            t_omp = run_timed(OMP_EXE, dataset, num_threads=hilos)
            resultados[dataset]["omp"][hilos] = t_omp

            if t_seq and t_omp:
                print(f"  → Speedup con {hilos} hilos: {t_seq/t_omp:.2f}x")

    imprimir_resumen(resultados)
