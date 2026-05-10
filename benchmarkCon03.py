import subprocess
import time
import os

def run_benchmark(executable, filename, runs=10):
    print(f"\n" + "="*60)
    print(f"PROCESANDO ARCHIVO: {filename}")
    print("="*60)
    
    times = []
    
    for i in range(runs):
        start = time.time()
        # En la primera ejecución mostramos el detalle de los centroides
        if i == 0:
            result = subprocess.run([f"./{executable}", filename], capture_output=False)
        else:
            # En las siguientes solo medimos tiempo (silencioso)
            result = subprocess.run([f"./{executable}", filename], capture_output=True)
            
        end = time.time()
        duration = end - start
        times.append(duration)
        if i > 0: print(f"Ejecución {i+1}: {duration:.4f}s")
    
    avg = sum(times) / runs
    print(f"\n>>> TIEMPO PROMEDIO FINAL ({filename}): {avg:.6f} segundos\n")

if __name__ == "__main__":
    print("Compilando kmeans.c con optimización -O3...")
    compile_proc = subprocess.run(["gcc", "-O3", "kmeans.c", "-o", "kmeans", "-lm"])
    
    if compile_proc.returncode == 0:
        if os.path.exists("movisA.csv"):
            run_benchmark("kmeans", "movisA.csv")
        if os.path.exists("movisB.csv"):
            run_benchmark("kmeans", "movisB.csv")
    else:
        print("Error en la compilación. Verifica que kmeans.c esté en la carpeta.")
