import subprocess
import time
import os

def run_benchmark(executable, filename, runs=10):
    print(f"\n" + "="*60)
    print(f"BENCHMARK PARALELO (OpenMP) - ARCHIVO: {filename}")
    print("="*60)
    
    times = []
    for i in range(runs):
        start = time.perf_counter()
        # Primera vez mostramos los centroides para verificar
        show_output = (i == 0)
        result = subprocess.run([f"./{executable}", filename], capture_output=not show_output)
        end = time.perf_counter()
        
        duration = end - start
        times.append(duration)
        if i > 0: print(f"Ejecución {i+1}: {duration:.4f}s")
    
    avg = sum(times) / runs
    print(f"\n>>> TIEMPO PROMEDIO (OpenMP): {avg:.6f} segundos\n")

if __name__ == "__main__":
    # IMPORTANTE: Se añade -fopenmp para habilitar el paralelismo
    print("Compilando kmeans_omp.c con -O3 y -fopenmp...")
    compile_proc = subprocess.run(["gcc", "-O3", "kmeans_omp.c", "-o", "kmeans_omp", "-lm", "-fopenmp"])
    
    if compile_proc.returncode == 0:
        if os.path.exists("movisA.csv"): run_benchmark("kmeans_omp", "movisA.csv")
        if os.path.exists("movisB.csv"): run_benchmark("kmeans_omp", "movisB.csv")
    else:
        print("Error en la compilación. Asegúrate de tener instalado libgomp.")
