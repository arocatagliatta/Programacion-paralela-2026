import subprocess
import time
import os

def run_test(executable, filename, num_threads, runs=10):
    # Configuramos las variables de entorno de OpenMP para esta ejecución
    env = os.environ.copy()
    env["OMP_NUM_THREADS"] = str(num_threads)
    env["OMP_PROC_BIND"] = "close"  # Mantiene los hilos cerca para compartir caché
    env["OMP_PLACES"] = "cores"     # Mapea hilos a núcleos físicos

    print(f"\n" + "="*70)
    print(f" ARCHIVO: {filename} | HILOS: {num_threads} | AFINIDAD: cores")
    print("="*70)
    
    durations = []
    
    for i in range(runs):
        start = time.perf_counter()
        
        # En la primera ejecución mostramos la salida de C (centroides y cantidad)
        # para verificar que el cálculo es correcto.
        is_first = (i == 0)
        
        try:
            process = subprocess.run(
                [f"./{executable}", filename],
                env=env,
                capture_output=not is_first,
                text=True
            )
            
            end = time.perf_counter()
            duration = end - start
            durations.append(duration)
            
            if not is_first:
                print(f"  Ejecución {i+1:02d}: {duration:.4f}s")
            else:
                # Mostramos la salida del programa C en la primera vuelta
                print("--- Verificación de Datos (Primera Ejecución) ---")
                print(process.stdout if is_first else "")
                
        except Exception as e:
            print(f"Error ejecutando el benchmark: {e}")
            return

    avg_time = sum(durations) / runs
    print(f"\n>>> RESULTADO FINAL ({filename}):")
    print(f"    Tiempo Promedio: {avg_time:.6f} segundos")
    return avg_time

if __name__ == "__main__":
    # 1. Compilación
    c_source = "kmeans_omp.c"
    executable = "kmeans_omp"
    
    print(f"Compilando {c_source} con GCC -O3 y OpenMP...")
    compile_cmd = ["gcc", "-O3", c_source, "-o", executable, "-lm", "-fopenmp"]
    
    if subprocess.run(compile_cmd).returncode == 0:
        # 2. Definir archivos y cantidad de hilos a probar
        archivos = ["movisA.csv", "movisB.csv"]
        
        # Puedes cambiar este valor según los núcleos de tu procesador
        # Por ejemplo: [1, 2, 4, 8] para ver la escalabilidad
        cantidad_hilos = [16] 
        
        for csv in archivos:
            if os.path.exists(csv):
                for hilos in cantidad_hilos:
                    run_test(executable, csv, hilos)
            else:
                print(f"Archivo {csv} no encontrado. Saltando...")
    else:
        print("Error crítico: No se pudo compilar el código C.")
