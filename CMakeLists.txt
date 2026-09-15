cmake_minimum_required(VERSION 3.10)
project(noctis CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
