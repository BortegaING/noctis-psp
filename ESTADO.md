# PROJECT NOCTIS — Estado del proyecto

> Si sos una sesion nueva (telefono/nube): **lee este archivo primero**. Aca esta todo el
> contexto que no se deduce del codigo. No hace falta que el usuario pegue nada.

Repo: `BortegaING/noctis-psp` (privado, rama `main`).
Clon en el PC: `C:\Users\Usuario\Desktop\Proyectos_Claude\noctis-psp`.
Usuario: Benjamin (BortegaING). Responder **en espanol**, sin emojis. No gastar tokens de mas.

## Que es
Juego para **PSP** (consola real: PSP-2000) en C++17 con PSPSDK. Inspiracion: BLAME!
(brutalismo, megaestructura, escala) + gravedad. Se juega en **3ra persona**.

## Estado actual (commit 5e62c77)
- Escena: **piso teselado grande + 8 monolitos enormes muy separados** (`src/city.h`, ~720 verts),
  niebla fria, luna 2D. Personaje (hunter) visible con camara detras.
- **Gravedad**: Triangulo cicla 6 direcciones; la camara toma como "arriba" la contra-gravedad
  y el personaje se orienta con los pies hacia donde cae.
- Reloj de la consola al **maximo**: `scePowerSetClockFrequency(333,333,166)` (por defecto 222/111).
- Controles: nub X gira / nub Y camina, D-pad izq-der strafe y arr-aba mirar, X salta,
  Cuadrado planea, L apunta, R dispara, Circulo melee, Triangulo gravedad, Start pausa.

## Descartado (sigue en el repo, SIN USO)
El diseno gotico tipo Bloodborne y "EL POZO" (pozo vertical con balcon) se sacaron porque
el **fill rate** mataba el FPS (llego a 1 FPS en la consola):
`cathedral_grand/twin/basilica/bell.h`, `shaft_walls.h`, `ledge.h`, `bridges.h`, `void_shaft.h`,
`gothic_bldg.h`, `facade_tex.h`, `robots.h`, `village_props.h`, `atmosphere.h`, `spirescape.h`.
No borrarlos: sirven de referencia.

## Pendiente, en orden
1. **FPS real** en la PSP del ultimo build (solo lo mide Benjamin en la consola).
2. Cerrar el bucle autonomo de prueba: leer el frame con `gpu.buffer.screenshot` del
   debugger de PPSSPP (PrintWindow devuelve un frame **congelado**).
3. **El sable del hunter sigue siendo un bloque gris** (`src/cand/hunter.h`) — quejado varias veces.
4. Fill rate: texturas 8888 -> 5650 (16 bits) y menos capas de pintado.

## Lecciones caras (no repetirlas)
- El **software renderer de PPSSPP miente**: no reproduce el culling ni el recorte del plano
  cercano del hardware. Un piso que "se ve" ahi puede NO existir en la PSP. Validar en GL o en consola.
- `main.cpp` **no habilita GU_CLIP_PLANES**: un triangulo largo con un vertice detras de la camara
  se descarta ENTERO. Por eso pisos/rieles/muros van **teselados** (celdas <= 2.5u).
- El **piso** no se cullea (sus quads son de una cara y el winding es opuesto al de las cajas);
  las cajas/monolitos SI con `NOCTIS_FRONTFACE GU_CCW`.
- La **RAM nunca fue el cuello**: el juego usa ~4 MB de 24 disponibles. Los 64 MB de la PSP-2000
  no dan FPS. El cuello es **fill rate** (pixeles pintados por frame).
- El emulador **siempre marca 60 FPS**: no sirve para medir velocidad, solo para ver fallos visuales.
- Cerrar PPSSPP antes de compilar (bloquea `build/EBOOT.PBP`).

## Como se trabaja
- Compilar (**solo en el PC**, WSL): `bash tools/build.sh` dentro de
  `wsl -d Ubuntu-24.04 -u root` (desde Git Bash anteponer `MSYS_NO_PATHCONV=1`).
- Emulador y capturas: ver `tools/emu/LEEME.md` (`shot.ps1` fuera de pantalla, `show.ps1` en
  pantalla, `psp_input.py` manda botones por el debugger). **Nunca** poner el emulador al frente
  sin avisar: le tapa la pantalla al usuario.
- Pasar a la PSP: copiar `build/EBOOT.PBP` a `<unidad USB>\PSP\GAME\NOCTIS\EBOOT.PBP`.

## Desde el telefono (PC apagado)
Solo hay repo: se puede **escribir codigo, decidir diseno y dejar commits**, pero **no compilar,
no ver el juego, no copiar a la consola**. El codigo escrito asi va **sin verificar**: se compila
y se prueba la proxima vez en el PC. Dejar los cambios en commits chicos y claros.
