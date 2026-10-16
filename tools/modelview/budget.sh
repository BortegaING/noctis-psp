#!/bin/bash
# Cuanto dibuja el cementerio desde varios puntos del camino (mirando al norte). Correr en WSL:
#   wsl -d Ubuntu-24.04 -- bash tools/modelview/budget.sh
cd "$(dirname "$0")" && g++ -O1 -std=c++17 -w -o dump dump.cpp || exit 1
./dump pool /tmp/bcm.bin 2>&1 | head -1
for p in "0 35 llegada" "0 15 camino" "0 -3 cruce" "0 -27 atrio" "12 -12 cruce-este"; do
    set -- $p
    printf "%-11s " "$3"; ./dump budget /tmp/bcm.bin "$1" "$2" 0 -1 40 2>&1 | grep instancias
done
