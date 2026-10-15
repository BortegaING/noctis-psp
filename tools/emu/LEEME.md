# Emulador (PPSSPP) — procedimiento de NOCTIS

PPSSPP portable vive en `ppsspp\` (exe `ppsspp\PPSSPPWindows64.exe`, config
`ppsspp\memstick\PSP\SYSTEM\ppsspp.ini`). Siempre carga `build\EBOOT.PBP`.

## Compilar
WSL Ubuntu-24.04 con pspdev:
`wsl -d Ubuntu-24.04 -u root -- bash -c 'cd /mnt/c/Users/Usuario/Desktop/Proyectos_Claude/noctis-psp && bash tools/build.sh'`
Desde Git Bash anteponer `MSYS_NO_PATHCONV=1` o mangla `/mnt/...`.
**Cerrar PPSSPP antes de compilar**: bloquea `build\EBOOT.PBP` ("Could not open the output file").
Los scripts con `-Build` ya lo cierran solos.

## Dos modos (segun el .ini)

| Modo | ini | Para que | Captura |
|---|---|---|---|
| Fuera de pantalla | `SoftwareRenderer = True`, `WindowX = -2600`, `InternalResolution = 1` | verificar sin interrumpir al usuario | `shot.ps1` (PrintWindow) |
| En pantalla (jugar) | `SoftwareRenderer = False` (GL), `WindowX = 60`, `InternalResolution = 2` | jugar/ver en el PC, render fiel al hardware | `grab.ps1` (CopyFromScreen) |

- `powershell -ExecutionPolicy Bypass -File tools\emu\shot.ps1 [-Build]` -> `build\noctis.png`.
  La **primera captura tras `-Build` sale negra** (transitorio): correr `shot.ps1` una
  segunda vez sin `-Build`. Rechaza solo frames blancos; negro puede ser real -> repetir.
- `show.ps1 [-Build]` lanza en pantalla y al frente y lo deja corriendo (no lo hagas si
  el usuario esta en reunion: tapa sus ventanas). `grab.ps1` captura la region de la
  ventana tal cual se ve (funciona con GL; PrintWindow con GL da negro).

## Ojos y manos sin tocar la pantalla: `psp_eye.py` (recomendado)
Con PPSSPP corriendo fuera de pantalla (`shot.ps1` lo lanza; el ini ya trae
`RemoteDebuggerOnStartup = True`), todo va por el WebSocket debugger, sin foco ni ventana:

`python tools\emu\psp_eye.py shot:antes "ay:-1.0:1500" "cross:100" shot:despues`

- `boton:ms`, `ax:X:ms` / `ay:Y:ms` (nub, -1..1; `ay` negativo = adelante), `wait:ms`,
  `shot[:nombre]` -> `build\<nombre>.png` (480x272 al doble). Tarda ~0.3 s por foto.
- Como captura: breakpoint en `sceDisplaySetFrameBuf` (lo llama `sceGuSwapBuffers` con el
  frame ya terminado), lee a0/a1/a2 (direccion, ancho de fila, formato) y lee esa VRAM con
  `memory.read`. Es el frame REAL que mostraria la PSP, no se congela como PrintWindow.
- `gpu.buffer.screenshot` NO sirve en PPSSPP 1.20.4 + software renderer ("Could not download
  output"); por eso el breakpoint.
- Necesita `SoftwareRenderer = True`: con GPU (GL/D3D) la VRAM no se actualiza.

## Que fiarse y que no
- El **software renderer es indulgente**: no reproduce el culling/recorte del plano
  cercano del hardware. Un piso o pared que "se ve" ahi puede DESAPARECER en la PSP.
  Para culling/clipping validar en GL (`grab.ps1`) o en la consola.
- main.cpp no habilita `GU_CLIP_PLANES`: un triangulo largo con un vertice detras de la
  camara se descarta ENTERO -> pisos/rieles/muros van teselados o partidos.
- El FPS del emulador (60) no dice nada del hardware: el numero real lo da la PSP.

## Pasar a la PSP
Con la consola en modo USB (aparece como unidad extraible, suele ser `D:`):
copiar `build\EBOOT.PBP` a `<unidad>\PSP\GAME\NOCTIS\EBOOT.PBP`.
`psp-release\NOCTIS\EBOOT.PBP` se mantiene como copia lista.
