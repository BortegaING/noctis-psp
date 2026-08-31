# PROJECT NOCTIS (PSP)

Action RPG de exploracion en tercera persona dentro de una megaestructura
vertical, gotica y oscura. Gravedad, escalada, combate, NPCs roboticos,
recursos ocultos y un vacio aparentemente infinito.

Plataforma objetivo: **PSP 1000 / 2000 / 3000** (corre tambien en PPSSPP).
Lenguaje: **C++17** con **PSPSDK** (toolchain `pspdev`), build con **CMake**.

### Objetivo 1 - Vertical slice: Distrito "Campanario"

- [ ] Boot del motor + camara en 3a persona + medidor de FPS
- [ ] Distrito Campanario (geometria low-poly + niebla de distancia)
- [ ] Movimiento responsivo (correr, saltar, caer)
- [ ] Un cambio de gravedad (pared -> suelo)
- [ ] Escalada basica integrada con gravedad
- [ ] 1 arma melee + 1 arma a distancia
- [ ] HUD del mockup: HP / EN / GRV, minimapa, arma equipada
- [ ] Recursos con brillo al acercarse
- [ ] The Cathedral como silueta lejana + campana

## Estado

Andamiaje inicial. Falta instalar el toolchain (ver `docs/BUILD.md`).

## Estructura

- `src/` codigo C++ del juego
- `assets/` recursos (modelos, texturas, audio) - placeholder
- `docs/DESIGN.md` la directiva maestra (pilares de diseno)
- `docs/BUILD.md` como compilar (WSL2 + pspdev) y probar (PPSSPP)
- `tools/build.sh` script de compilacion para WSL
