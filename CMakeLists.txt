cmake_minimum_required(VERSION 3.16)

project(KMeansOpenMP
    VERSION     1.0
    DESCRIPTION "K-Means clustering secuencial y paralelo (OpenMP) sobre datos de varianza de movimiento"
    LANGUAGES   C
)

# ── Estándar C ────────────────────────────────────────────────────────────────
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ── OpenMP ────────────────────────────────────────────────────────────────────
# find_package detecta la flag correcta según el compilador activo:
#   GCC/Clang → -fopenmp   |   MSVC → /openmp
# REQUIRED hace fallar el configure si OpenMP no está disponible.
find_package(OpenMP REQUIRED)

# ── Headers compartidos ───────────────────────────────────────────────────────
include_directories(include)

# ── Ejecutable 1: versión secuencial (línea base para benchmark) ──────────────
add_executable(kmeans_seq
    src/main_seq.c
    src/csv_reader.c
    src/kmeans.c
)
target_compile_options(kmeans_seq PRIVATE -O3 -Wall -Wextra)
target_link_libraries(kmeans_seq PRIVATE m)

# ── Ejecutable 2: versión paralela con OpenMP ─────────────────────────────────
add_executable(kmeans_omp
    src/main_omp.c
    src/csv_reader.c
    src/kmeans_omp.c
)
target_compile_options(kmeans_omp PRIVATE -O3 -Wall -Wextra)
# OpenMP::OpenMP_C aplica -fopenmp tanto en compilación como en linkeo.
# kmeans_seq NO hereda esta flag.
target_link_libraries(kmeans_omp PRIVATE OpenMP::OpenMP_C m)

# ── Target opcional: documentación con Doxygen ────────────────────────────────
find_package(Doxygen OPTIONAL_COMPONENTS dot)

if(DOXYGEN_FOUND)
    set(DOXYGEN_IN  ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/docs)

    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_IN}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generando documentación HTML con Doxygen..."
        VERBATIM
    )
    message(STATUS "Doxygen encontrado: 'cmake --build build --target docs' para generar docs.")
else()
    message(STATUS "Doxygen no encontrado: el target 'docs' no estará disponible.")
endif()

# ── Mensaje de ayuda post-configure ──────────────────────────────────────────
message(STATUS "")
message(STATUS "  Compilar:  cmake --build <build_dir>")
message(STATUS "  Ejecutar:  ./<build_dir>/kmeans_seq data/movisA.csv")
message(STATUS "             ./<build_dir>/kmeans_omp data/movisA.csv")
message(STATUS "  Benchmark: python scripts/benchmark.py")
message(STATUS "  Docs:      cmake --build <build_dir> --target docs")
message(STATUS "")
