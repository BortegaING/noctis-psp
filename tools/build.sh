#!/usr/bin/env bash
# Compila NOCTIS dentro de WSL con el toolchain pspdev instalado.
# Uso:  bash tools/build.sh
set -euo pipefail

export PSPDEV="${PSPDEV:-/usr/local/pspdev}"
export PATH="$PSPDEV/bin:$PATH"

if ! command -v psp-cmake >/dev/null 2>&1; then
    echo "ERROR: no encuentro psp-cmake. Instala el toolchain pspdev (ver docs/BUILD.md)."
    exit 1
fi

psp-cmake -S . -B build
cmake --build build -j"$(nproc)"

echo ""
echo "OK -> build/EBOOT.PBP"
echo "Copialo a PPSSPP o a la PSP (PSP/GAME/NOCTIS/EBOOT.PBP) para probarlo."
