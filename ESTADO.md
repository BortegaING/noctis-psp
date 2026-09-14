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

## Convenciones del motor (firmas EXACTAS, verificadas)
Sin `rand`, sin heap, C++17, solo `<math.h>`. Todo determinista. Los `.h` se incluyen desde
`main.cpp` y usan sus helpers: **no** son standalone.

Vertices:
- `struct LineVertex { unsigned int color; float x,y,z; }` — plano, sin textura (`LINE_FLAGS`).
- `struct TexVertex { float u,v; unsigned int color; float x,y,z; }` — texturizado (`TEX_FLAGS`).

Primitivas (costo en verts):
- `addSolidBox(LineVertex*, int& i, cx, baseY, cz, w, d, h, col)` — caja, **30v**. `baseY` = base, no centro.
- `addPyramid(LineVertex*, int& i, cx, baseY, cz, w, d, apexH, col)` — **12v**.
- Versiones texturizadas: `addSolidBoxT`, `addPyramidT`, y
  `addQuadT(buf,i, ax,ay,az, bx,by,bz, cx,cy,cz, dx,dy,dz, u0,v0,u1,v1, col)` — **6v**.
- Organicas (`src/char_prims.h`), las buenas para personajes y armas:
  - `addLimb(buf,i, x0,y0,z0, x1,y1,z1, r0,r1, sides, colB, colT)` — cilindro **que se afina**
    entre dos puntos cualesquiera (cualquier angulo). Costo `sides*12`.
  - `addBall(buf,i, cx,cy,cz, rx,ry,rz, stacks,slices, col)` — elipsoide. Costo `stacks*slices*6`.
  - `addLoft(buf,i, cx,cz, const float* ys, const float* rs, int n, sides, colBot, colTop)` —
    anillos apilados (torso, faldon).

Color: `RGBA(r,g,b,a)` 0..255; `brighten(col,f)`, `warmTint(col)`,
`fadeToVoid(col,dist)` (niebla por distancia), `heightHaze(col,y)` (niebla por altura).

Personaje: `src/cand/hunter.h` -> `static int build_hunter(LineVertex* buf)` devuelve el n de
verts; se vuelca en `g_hero[3200]` (`main.cpp:602`). Hay margen de sobra en ese buffer.

## Tarea pendiente #1 para sesiones sin PC: EL SABLE
Ahora mismo (`hunter.h`, bloque "CLEAVER") la hoja son **4 `addSolidBox` apilados** que se van
angostando: por eso se lee como "cuadrados grises". La guarda es otra caja.
Como arreglarlo **sin ver el resultado**:
- Rehacer la hoja con `addLimb` (2-3 tramos) para que se afine de verdad y termine en **punta**,
  con `sides` 4-6 (perfil de hoja, no tubo). Filo mas claro que el lomo usando `colB`/`colT`.
- Guarda: dos `addLimb` cortos cruzados o una caja fina + dos remates, no un ladrillo.
- Empunadura: `addLimb` con `r0>r1` + `addBall` de pomo.
- Mantenerse dentro de ~400 verts y en el mismo sitio/orientacion (mano derecha, apunta abajo).
- No tocar `main.cpp`: el archivo se compila solo al reconstruir.
