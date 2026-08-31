# PROJECT NOCTIS (PSP)

Action RPG de exploracion en tercera persona dentro de una megaestructura
vertical, gotica y oscura. Gravedad, escalada, combate, NPCs roboticos,
recursos ocultos y un vacio aparentemente infinito.

Plataforma objetivo: **PSP 1000 / 2000 / 3000** (corre tambien en PPSSPP).
Lenguaje: **C++17** con **PSPSDK** (toolchain `pspdev`), build con **CMake**.

### Objetivo 1 - Vertical slice: Distrito "Campanario"

- [x] Boot del motor + camara en 3a persona + medidor de FPS
- [x] Distrito Campanario (22 torres goticas en wireframe; niebla pendiente)
- [x] Movimiento con stick + salto/gravedad (caer)
- [ ] Un cambio de gravedad (pared -> suelo)
- [~] Aterrizar en azoteas (colision de techo; escalada real pendiente)
- [ ] Combate: 1 arma melee + 1 a distancia (arma se muestra en HUD, falta disparar)
- [x] HUD del mockup: barras HP / EN / GRV + panel de arma (minimapa pendiente)
- [x] Recursos con brillo al acercarse + recoleccion
- [~] The Cathedral silueta lejana (falta campana/audio y el encuentro)

## Estado (2026-08-31)

Vertical slice en marcha. Todo compila y corre en PPSSPP; falta verificacion
visual de los ultimos avances (recursos, HUD, gravedad). Render con **GU_LINES**
(mundo en wireframe) y **GU_SPRITES** (HUD y texto), primitivos ya validados en
pantalla. Fuente bitmap propia (no depende de flash0).

Pendiente principal: niebla (subdividiendo lineas), geometria solida, el cambio
de direccion gravitacional, combate real, NPCs roboticos y audio.

## Estructura

- `src/main.cpp` motor + gameplay (un archivo por ahora; se modulariza despues)
- `src/world_data.h` distrito Campanario (estructuras + puntos de recurso)
- `src/game_data.h` armas (10+10), materiales, familias de enemigos
- `src/font8x8_basic.h` fuente bitmap 8x8 (dominio publico)
- `assets/` recursos (modelos, texturas, audio) - placeholder
- `docs/DESIGN.md` la directiva maestra (pilares de diseno)
- `docs/ARCHITECTURE.md` modulos y plan de agentes
- `docs/BUILD.md` como compilar (WSL2 + pspdev) y probar (PPSSPP)
- `tools/build.sh` script de compilacion para WSL

## Controles (actuales)

- Stick: mover | D-pad izq/der: orbitar camara | X: saltar | START: salir
