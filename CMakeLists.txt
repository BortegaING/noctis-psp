cmake_minimum_required(VERSION 3.10)
project(noctis CXX C ASM)

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
    src/music_blob.S   # .incbin de assets/SERAPH.ADP (cancion de la pelea con el serafin)
    src/voice_blob.S   # .incbin de assets/VOICE.ADP (la voz de Baphomet: tools/voice/make_voice.py)
)
# el .incbin busca en assets/, y si la cancion cambia hay que reensamblar
set_source_files_properties(src/music_blob.S PROPERTIES
    COMPILE_FLAGS "-Wa,-I${CMAKE_SOURCE_DIR}/assets"
    OBJECT_DEPENDS "${CMAKE_SOURCE_DIR}/assets/SERAPH.ADP")
set_source_files_properties(src/voice_blob.S PROPERTIES
    COMPILE_FLAGS "-Wa,-I${CMAKE_SOURCE_DIR}/assets"
    OBJECT_DEPENDS "${CMAKE_SOURCE_DIR}/assets/VOICE.ADP")

target_link_libraries(${PROJECT_NAME}
    psputility
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
    MEMSIZE 1   # 64 MB de la PSP-2000/3000 (~52 libres). El helper pone 2 (modo limitado) si se omite
    ICON_PATH       ${CMAKE_SOURCE_DIR}/assets/ICON0.PNG   # icono del XMB (144x80)
    BACKGROUND_PATH ${CMAKE_SOURCE_DIR}/assets/PIC1.PNG    # fondo del XMB (480x272)
)
