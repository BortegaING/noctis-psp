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

---

# PROJECT NOCTIS — planteamiento del juego (acordado 2026-09-14)

El juego se llama **PROJECT NOCTIS**. Lo de abajo es su bucle de juego, no otro proyecto.

**Accion central:** escalar la megaestructura **cambiando la direccion de la gravedad**.
Una pared se vuelve tu suelo, corres por ella, volves a cambiar, caes a una repisa mas alta.
Objetivo: llegar arriba. Peligro: el vacio.

Elegido por Benjamin entre 4 opciones. Razon: reutiliza lo que YA funciona (gravedad +
monolitos), **no agrega geometria** (por lo tanto no rompe el FPS) y la verticalidad hace
mirar pocos objetos grandes en vez de panoramicas anchas, que es justo lo barato en PSP.

## Por que esto es un JUEGO y no un paseo
- **Energia (EN, ya esta en el HUD):** cada cambio de gravedad **cuesta EN**. La EN **solo se
  recarga estando parado sobre una superficie**. Asi no se puede cambiar a lo loco: hay que
  **planificar la ruta** y buscar donde apoyarse.
- **Caer:** si caes al vacio, volves al ultimo **ancla** (checkpoint). Sin animacion de muerte,
  sin penalizacion dura: la tension la da perder el avance, no un game over.
- **Ruta:** los monolitos tienen repisas y huecos. Subir es **encontrar el camino**, no apretar
  un boton. Ahi esta el juego.
- **Materiales:** en repisas dificiles. Recompensan la buena ruta y **suben la EN maxima**,
  que es lo mismo que decir "llegas mas alto". Progresion sin numeros de RPG.
- **Meta:** una **luz/faro en la cima** del monolito mas alto. Se ve desde el suelo: el jugador
  siempre sabe adonde va sin un solo icono en pantalla.

## Piezas a construir (en orden; la 2 es la critica)
1. **Cambio de gravedad DIRIGIDO.** Hoy Triangulo cicla 6 direcciones a ciegas. Para escalar
   hay que cambiar **hacia la superficie que estas mirando** (la pared mas cercana en la
   direccion de la camara). Es lo que convierte la mecanica en navegacion.
2. **Caminar por paredes (LO CRITICO).** `groundHeight()` y `blocked()` hoy asumen gravedad
   hacia abajo: el "suelo" es y=0 y los techos de las cajas. Hay que generalizarlos para que
   el suelo sea **la cara de la caja perpendicular a la gravedad**.
   **Clave tecnica:** como los monolitos son cajas **alineadas a los ejes** y la gravedad es
   una de 6 direcciones **tambien alineada a los ejes**, esto NO es colision generica: es el
   mismo test de caja con los ejes intercambiados. Es tratable.
3. **Coste y recarga de EN** en cada cambio (arrancar con ~20% del maximo por cambio; recarga
   solo con los pies apoyados).
4. **Anclas (checkpoints) + respawn** al caer bajo cierta altura. Reusar la red de seguridad
   que ya existe en `main.cpp`.
5. **Materiales en repisas** (el brillo por proximidad ya esta hecho) que suben la EN maxima.
6. **Faro en la cima** del monolito mas alto.

## Pendiente de ajuste (tuning, cuando 1 y 2 funcionen)
- Velocidad al correr por pared: a la velocidad actual, subir un monolito de 380 tarda ~1 min.
  Probablemente haya que acelerar al ir "cuesta arriba" o hacer los monolitos mas bajos.
- Que las repisas sean legibles desde lejos (salientes en las cajas, no decoracion fina).

## DISENO A SEGUIR (decidido 2026-09-14) — SECTOR CERRADO DE PASILLOS
Se abandona la plataforma gigante con monolitos lejanos (mucho fill). El mundo pasa a ser un
**recinto chico y cerrado**, porque **las paredes ocluyen y lo que no se ve no se pinta**:
- Recinto 112x112 con **suelo, TECHO a y=60** (el doble de la masa de 30) y **muros exteriores**.
  Limite arriba y abajo: **no hay vacio infinito**.
- Adentro **4 masas SEPARADAS** de 36x36 (nunca pegadas entre si) que dejan un **pasillo en cruz**
  de 18 de ancho y un **pasillo perimetral** de 9.
- **Gotico de verdad**: columnas separadas cada 16 con **arcos ojivales** entre ellas,
  contrafuertes en las caras que dan al pasillo, cornisa y remate escalonado por masa.
- A ras de suelo: pasillos tapados y baratos. Arriba de una masa: se ve el sector entero.
  Ese contraste alimenta la mecanica de escalar con gravedad.

Ya escrito: **`src/sector.h`** (colision + suelo/techo + masas/muros/arcada). **NO integrado aun.**
Pasos pendientes para integrarlo:
1. Incluir `sector.h` en `main.cpp` y reemplazar el cuerpo de `buildSolidWorld`:
   suelo+techo en la ranura del piso (sin cull), estructura en la de muros (con cull).
2. `buildCity()` -> llamar a `buildSectorCollision()` (colision y techos pisables salen gratis).
3. Acotar al jugador a +-54 y bajar el **plano lejano de 520 a ~200** (nada esta mas lejos).
4. Reajustar la niebla: hoy `fadeToVoid` usa a=50,b=330, calibrado para el mundo grande.

## LA REFERENCIA VISUAL (la imagen que Benjamin manda una y otra vez)
Mockup en una PSP. **Es el objetivo de look, no negociable.** Contiene:
- **3ra persona**: el cazador visto de espaldas, de pie en una **terraza/balcon con baranda de
  hierro**, espada larga en la mano derecha, abrigo oscuro con detalles rojos.
- Al frente, un **bosque DENSO de agujas y campanarios goticos** que se pierde en **niebla
  gris-parda calida**. Cientos de siluetas casi negras, superpuestas, cada vez mas tenues.
- Detalles de primer plano: farol, **cables/cadenas colgando** entre estructuras, piedra gastada.
- Paleta: gris-pardo CALIDO en la niebla, siluetas casi negras, y **puntitos ambar** de ventanas
  encendidas como unico acento. Nada saturado.
- HUD: HP/EN/GRV arriba-izquierda, minimapa arriba-derecha con "DISTRITO: Campanario",
  aviso de objeto abajo-izquierda, arma abajo-derecha.

**Como se concilia con el sector de pasillos:** la referencia es el **LOOK**; los pasillos son
el **ESPACIO JUGABLE**. Se junta asi: el recinto tiene **aberturas/ventanales** y por ellas se ve
un **telon de agujas fogueadas** (impostores baratos, casi color niebla). El jugador camina por
pasillos (barato, ocluido) pero **cada abertura enmarca la vista de la referencia**. Al subir
arriba de una masa, se abre la panoramica completa: ese es el momento "postal".

## TANDA DE PULIDO (15 agentes, commits 1f934ab..8046f4a)
Integrado y compilado pieza por pieza: sector cerrado de pasillos con boveda de crucería y
VENTANALES con traceria gotica; muro de silleria y suelo de catedral; 12 braseros, estatuas,
sarcofagos y cadenas; 6 robots habitantes; telon de 106 agujas en 5 anillos; cielo de
sobrecast calido 4x mas barato; hunter organico (una sola caja en todo el modelo) con sable
curvo; ciclo de caminata real; gravedad DIRIGIDA a la cara que miras + colision generalizada.

### El hallazgo que explica anos de "se ve gris"
La piedra del sector era **azul dominante** (134,147,172). Multiplicada por la textura parda
daba un pixel final gris NEUTRO muerto: por mas textura gotica que se pusiera, el juego nunca
se iba a ver como la referencia calida. Arreglado igualando luminancia pero con r>g>b.

### Bugs que encontraron los agentes entre ellos (no repetirlos)
- `addLimb` tiene el winding OPUESTO al de `addSolidBox`: cilindros y elipsoides DESAPARECEN
  en un pase con back-face culling. Robots y hunter van en el pase SIN culling.
- Las cajas del motor (`addSolidBoxT`) NO traen cara inferior: para algo que se mira desde
  abajo (boveda) hay que emitir el intrados a mano.
- El bloque de animacion viejo inclinaba al personaje hacia ATRAS y aplicaba el pitch en el
  eje del MUNDO (corriendo de lado parecia volcar).
- La camara mira 11.75 grados hacia abajo: el horizonte cae en pantalla y=92, no en el centro.

### DECISION DEFERIDA (a proposito)
La auditoria de color pidio `fadeToVoid` a=45,b=380 para que el pasillo no quede lavado de
niebla. NO se aplico: el telon de agujas calibro sus 5 bandas de profundidad contra los
valores actuales (a=20,b=140) y cambiarlos sin re-derivar `spireFog` deja el fondo plano.
Si se toca uno, hay que recalcular el otro EN EL MISMO PASO, y validarlo mirando.
