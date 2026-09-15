cmake_minimum_required(VERSION 3.10)
project(noctis CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# OPTIMIZACION. Sin esto CMake no pasa NINGUNA bandera -O, es decir compila en -O0:
# el juego entero corria sin optimizar en una CPU MIPS de 333 MHz y con aritmetica
# de coma flotante por todas partes. Medido: el EBOOT baja de 1.46 MB a 329 KB,
# porque -O2 ademas elimina el codigo muerto de los mundos viejos.
# -O2 y NO -O3 a proposito: la PSP tiene 16 KB de cache de instrucciones y el
# desenrollado agresivo de -O3 la desborda, que sale mas caro que lo que ahorra.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Tipo de build" FORCE)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_C_FLAGS_RELEASE   "-O2 -DNDEBUG")

# Compilar con el toolchain de pspdev:
#   psp-cmake -S . -B build
#   cmake --build build
# (psp-cmake ya apunta a $PSPDEV/psp/share/pspdev.cmake e incluye los
#  helpers de PSPSDK como create_pbp_file)

add_executable(${PROJECT_NAME}
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME}
    pspgum
    pspgu
    m
    pspvfpu
    pspdisplay
    pspge
    pspctrl
    psppower
    pspaudio
)

# Genera EBOOT.PBP ejecutable en PSP / PPSSPP
create_pbp_file(
    TARGET ${PROJECT_NAME}
    TITLE  "PROJECT NOCTIS"
)
