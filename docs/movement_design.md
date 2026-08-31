# PROJECT NOCTIS - Diseno de Movilidad y Gravedad

Documento del sistema de movimiento del vertical slice. Objetivo: MOVILIDAD
fluida (aceleracion/friccion, no rigido) y GRAVEDAD como mecanica de exploracion
vertical (saltar, planear, correr por paredes cambiando la direccion de la
gravedad). Todo en C++17 para PSPSDK, pensado para ~60 fps (las constantes son
por-frame, igual que el resto del motor).

Todos los bloques usan EXACTAMENTE los nombres que ya existen en `main.cpp`:
`playerX/playerY/playerZ`, `velY`, `en`, `EN_MAX`, `grounded`, `camYaw`,
`pad.Lx/pad.Ly`, `PSP_CTRL_*`, y los helpers `blocked(nx,nz,py)` y
`groundHeight(px,pz,py)`.

---

## 0. Como integrar (mapa rapido)

1. Declara los ESTADOS NUEVOS junto al estado del jugador (main.cpp ~linea 729,
   donde estan `velY`, `en`, `grounded`, `camYaw`):

```cpp
float velX = 0.0f, velZ = 0.0f;   // velocidad plana (nueva)
int   coyote  = 0;                // frames de gracia para saltar
int   prevJump = 0;               // flanco del boton X
int   jumpBuf  = 0;               // buffer de salto
```

2. Pon las CONSTANTES (seccion 1) arriba, cerca de `const float EN_MAX`.

3. Reemplaza el bloque de movimiento actual (main.cpp lineas ~752 a ~783: desde
   "camara (D-pad izq/der)" hasta el clamp de `en`) por el BLOQUE UNICO de la
   seccion 5. Deja intacta la recoleccion de recursos que va despues.

4. La gravedad reorientable (seccion 6) es ETAPA AVANZADA: es un esqueleto
   separado, no lo pegues hasta terminar 1-5.

---

## 1. Constantes (tuneadas)

```cpp
// ---- movimiento plano ----
const float DEADZONE   = 0.18f;   // zona muerta del stick (0..1)
const float RUN_SPEED  = 0.42f;   // velocidad plana maxima (u/frame ~= 25 u/s)
const float ACCEL_GND  = 0.22f;   // respuesta en suelo (0..1; mas alto = mas directo)
const float ACCEL_AIR  = 0.09f;   // control en el aire (menor = mas inercia)
const float STOP_FRIC  = 0.20f;   // freno extra al soltar el stick en suelo
const float CAM_SPEED  = 0.03f;   // giro de camara por frame (dpad) - igual que hoy

// ---- salto / gravedad ----
const float GRAVITY    = 0.020f;  // gravedad normal (u/frame^2)
const float JUMP_VEL   = 0.55f;   // impulso de salto (pico ~7.5 u, aire ~0.9 s)
const float SHORTHOP   = 0.50f;   // recorte de velY al soltar X subiendo (salto variable)
const int   COYOTE_MAX = 6;       // frames de gracia tras dejar el suelo (~0.1 s)
const int   JUMPBUF_MAX = 6;      // frames que "recuerda" el salto antes de aterrizar

// ---- planeo / control de gravedad (L) ----
const float FLOAT_LIFT   = 0.030f;  // empuje hacia arriba al planear
const float FLOAT_GRAV   = 0.006f;  // gravedad reducida mientras planeas (glide)
const float FLOAT_UPCAP  = 0.12f;   // subida maxima planeando
const float FLOAT_FALLCAP = -0.09f; // caida maxima planeando (descenso lento)
const float EN_FLOAT     = 6.0f;    // gasto de EN por frame planeando
const float EN_REGEN     = 5.0f;    // recarga de EN por frame en suelo
```

Notas de tuneo (por que estos numeros):
- `JUMP_VEL 0.55` con `GRAVITY 0.020` da un pico de `v^2/(2g) = 7.56` unidades
  (el jugador mide 3), tiempo de aire `2v/g = 55` frames (~0.9 s): un salto amplio
  y "flotante", coherente con un juego de movilidad.
- `ACCEL_GND 0.22` alcanza ~90% de la velocidad objetivo en ~10 frames (~0.16 s):
  responde rapido pero NO instantaneo (se siente peso, no rigidez).
- `ACCEL_AIR 0.09` deja inercia en el aire: el salto se compromete un poco, se
  siente mejor que el control aereo total.

---

## 2. Movimiento relativo a camara (diseno)

El stick entrega dos ejes: adelante/atras (`Ly`) y strafe (`Lx`). En PSP el centro
es 128 y `Ly` crece hacia ABAJO, asi que "adelante" = `-(Ly-128)`.

Ese deseo local `(adelante, strafe)` se rota por `camYaw` para volverlo un vector
de MUNDO. De la matriz VIEW actual (main.cpp 820-831: `RotateXYZ{15,camYaw,0}`) se
deduce que la mirada horizontal de la camara en mundo es:

```
forward_mundo = ( sin(camYaw), 0, -cos(camYaw) )
right_mundo   = ( cos(camYaw), 0,  sin(camYaw) )
```

Con `camYaw = 0`, forward = (0,0,-1): coincide con el movimiento actual (stick
arriba mueve -Z), asi que el cambio es continuo, no invierte controles.

Formula de desplazamiento deseado (con `fwd = -ay`, `str = ax`):

```
wishX =  fwd*sin(camYaw) + str*cos(camYaw)
wishZ = -fwd*cos(camYaw) + str*sin(camYaw)
```

Fluidez: no movemos la posicion directo con el stick. El stick fija una VELOCIDAD
objetivo (`wish * RUN_SPEED`) y `velX/velZ` se aproximan a ella con un factor de
control (aceleracion) distinto en suelo y aire. Al soltar, friccion la lleva a 0.
La colision se aplica sobre `velX` y `velZ` POR EJES SEPARADOS con `blocked()`,
que es lo que permite deslizar por los muros (si un eje choca, se anula solo ese).

Zona muerta radial + reescalado: se mide la magnitud del stick; dentro de
`DEADZONE` no hay input, y fuera se reescala para no perder precision cerca del
centro. Ademas se normaliza para que la diagonal no sea mas rapida que recto.

---

## 3. Salto mejorado (diseno)

Tres mejoras sobre el salto actual (`if (grounded && X) velY = 0.55`):

- COYOTE TIME: `coyote` se recarga a `COYOTE_MAX` mientras estas en suelo y baja
  1 por frame en el aire. Puedes saltar si `coyote > 0`, o sea hasta ~0.1 s
  despues de salir del borde. Perdona el salto tarde en plataformeo.
- JUMP BUFFER: al pulsar X se arma `jumpBuf = JUMPBUF_MAX`. Si tocas suelo dentro
  de esa ventana, el salto sale solo. Perdona el salto pulsado un pelin antes de
  aterrizar.
- SALTO VARIABLE (short hop): si sueltas X mientras subes, `velY *= SHORTHOP`.
  Toque corto = salto bajo; mantener = salto alto. Da control de altura.

Se usa deteccion de FLANCO (`prevJump`) para que mantener X no re-salte solo.

---

## 4. Gravedad / planear con L (diseno)

Mantener L reduce la gravedad y da sustentacion: el jugador sube suave y, sobre
todo, PLANEA (limita la velocidad de caida a `FLOAT_FALLCAP`, un descenso lento).
Esto convierte L en herramienta de exploracion: cruzar huecos grandes, bajar
controlado desde una torre, alcanzar azoteas. Gasta `EN_FLOAT` por frame; `en`
(barra EN del HUD, `EN_MAX = 780`) se agota y se recarga `EN_REGEN` por frame solo
al estar `grounded`. Sin EN, cae la gravedad normal. Es identico en espiritu al L
actual pero con caida limitada (glide real) y numeros nombrados.

---

## 5. BLOQUE UNICO listo para pegar (movimiento + salto + planeo + energia)

Reemplaza el bloque de movimiento actual dentro de `if (!paused) { ... }`
(main.cpp ~752 a ~783). No toca la recoleccion de recursos posterior.

```cpp
// ===== 1) CAMARA (igual que antes) =====
if (pad.Buttons & PSP_CTRL_LEFT)  camYaw -= CAM_SPEED;
if (pad.Buttons & PSP_CTRL_RIGHT) camYaw += CAM_SPEED;

// ===== 2) STICK -> DESEO RELATIVO A CAMARA =====
float ax = ((int)pad.Lx - 128) / 128.0f;   // -1..1  (derecha +)
float ay = ((int)pad.Ly - 128) / 128.0f;   // -1..1  (abajo +, convencion PSP)
float mag = sqrtf(ax*ax + ay*ay);
float wishX = 0.0f, wishZ = 0.0f;
if (mag > DEADZONE) {
    // reescala fuera de la zona muerta y normaliza (diagonal no mas rapida)
    float k = (mag - DEADZONE) / (1.0f - DEADZONE);
    if (k > 1.0f) k = 1.0f;
    ax = (ax / mag) * k;
    ay = (ay / mag) * k;
    float fwd = -ay;   // stick arriba = adelante
    float str =  ax;   // stick derecha = strafe
    wishX =  fwd * sinf(camYaw) + str * cosf(camYaw);
    wishZ = -fwd * cosf(camYaw) + str * sinf(camYaw);
    // Si en tu build adelante/atras o izq/der salen invertidos, cambia el signo
    // de fwd o de str aqui arriba (una linea). Depende del orden de RotateXYZ.
}

// ===== 3) ACELERACION / FRICCION (fluido, no rigido) =====
float targetVX = wishX * RUN_SPEED;
float targetVZ = wishZ * RUN_SPEED;
float ctrl = grounded ? ACCEL_GND : ACCEL_AIR;
velX += (targetVX - velX) * ctrl;
velZ += (targetVZ - velZ) * ctrl;
if (grounded && wishX == 0.0f && wishZ == 0.0f) {   // freno seco al parar
    velX -= velX * STOP_FRIC;
    velZ -= velZ * STOP_FRIC;
}

// ===== 4) COLISION POR EJES SEPARADOS (desliza por muros) =====
float nx = playerX + velX;
if (!blocked(nx, playerZ, playerY)) playerX = nx; else velX = 0.0f;
float nz = playerZ + velZ;
if (!blocked(playerX, nz, playerY)) playerZ = nz; else velZ = 0.0f;

// ===== 5) SALTO: coyote time + buffer + salto variable =====
if (grounded) coyote = COYOTE_MAX; else if (coyote > 0) coyote--;
int jumpNow = (pad.Buttons & PSP_CTRL_CROSS) ? 1 : 0;
if (jumpNow && !prevJump) jumpBuf = JUMPBUF_MAX; else if (jumpBuf > 0) jumpBuf--;
if (jumpBuf > 0 && coyote > 0) {          // intencion de salto + gracia de suelo
    velY = JUMP_VEL;
    grounded = 0;
    coyote = 0;
    jumpBuf = 0;
}
if (!jumpNow && velY > 0.0f) velY *= SHORTHOP;   // soltar X subiendo = salto corto
prevJump = jumpNow;

// ===== 6) GRAVEDAD / PLANEO (L gasta EN) =====
if ((pad.Buttons & PSP_CTRL_LTRIGGER) && en > 0.0f) {
    velY += FLOAT_LIFT;
    if (velY > FLOAT_UPCAP)    velY = FLOAT_UPCAP;
    velY -= FLOAT_GRAV;                            // gravedad casi anulada
    if (velY < FLOAT_FALLCAP)  velY = FLOAT_FALLCAP; // glide: caida lenta
    en -= EN_FLOAT;
    grounded = 0;
} else {
    velY -= GRAVITY;                               // gravedad normal
}

// ===== 7) INTEGRA VERTICAL + SUELO/AZOTEAS =====
playerY += velY;
float gh = groundHeight(playerX, playerZ, playerY);
if (playerY <= gh) { playerY = gh; velY = 0.0f; grounded = 1; }
else               { grounded = 0; }   // en el aire: necesario para el coyote time

// ===== 8) ENERGIA (EN) =====
if (grounded && en < EN_MAX) en += EN_REGEN;
if (en > EN_MAX) en = EN_MAX;
if (en < 0.0f)   en = 0.0f;
```

Con esto el HUD (barra EN, coords X/Z/Y) sigue funcionando sin cambios: solo se
leen `en`, `EN_MAX`, `playerX/Y/Z`, que se mantienen.

---

## 6. CAMBIO DE GRAVEDAD (correr por paredes) -- ETAPA AVANZADA

> Esqueleto/pseudocodigo. NO pegar hasta tener 1-5 estables. Requiere generalizar
> los dos helpers de colision, que hoy solo entienden el eje Y del mundo.

### 6.1 Concepto (vectorial, el modelo mental)

- La gravedad es un VECTOR `G` unitario que apunta "hacia abajo". Por defecto
  `G = (0,-1,0)`. El "arriba" del jugador es `U = -G`.
- `velY` deja de ser "eje Y" y pasa a ser velocidad a lo largo de `U`. El
  movimiento plano ocurre en el plano perpendicular a `U`.
- L+R busca una pared cercana, toma su NORMAL `N` (apunta de la pared hacia el
  jugador) y fija `G = -N`: la gravedad "cae" hacia la pared, o sea la pared se
  vuelve el nuevo SUELO. `U = N`.
- La camara recibe `U` como su vector up; asi la pared se ve como piso. El modelo
  del jugador tambien se reorienta para que los pies apunten al nuevo suelo.
- Para que no sea un tiron, se interpola `U` de la orientacion vieja a la nueva
  en ~0.3 s (aqui, un lerp simple del vector up ya se ve bien).

### 6.2 Enfoque realista para PSP: gravedad a las 6 CARAS del mundo

El problema real es la colision: `blocked()` y `groundHeight()` estan cableados al
eje Y (comparan `st.y + st.h` como "techo"). Gravedad totalmente libre exige
reescribir ambos con AABB proyectados sobre `U` arbitrario, que es caro y delicado
en PSP. La via practica: restringir `G` a las 6 direcciones de eje del mundo
(-Y normal, +Y techo, +X, -X, +Z, -Z). Asi el "suelo" siempre es un test
alineado a un eje y puedes REUSAR la logica AABB existente, solo cambiando el eje.

Estados nuevos (junto al estado del jugador):

```cpp
int   gravAxis = 0;   // 0=-Y(normal) 1=+X 2=-X 3=+Z 4=-Z 5=+Y
float upX = 0.0f, upY = 1.0f, upZ = 0.0f;         // up ACTUAL (interpolado)
float tgtUpX = 0.0f, tgtUpY = 1.0f, tgtUpZ = 0.0f; // up OBJETIVO
int   prevLR = 0;

// up = -G por cada eje de gravedad
static const float kUp[6][3] = {
    { 0,  1,  0},   // 0: G=-Y  suelo normal   (up=+Y)
    {-1,  0,  0},   // 1: G=+X  pared +X = piso (up=-X)
    { 1,  0,  0},   // 2: G=-X                  (up=+X)
    { 0,  0, -1},   // 3: G=+Z                  (up=-Z)
    { 0,  0,  1},   // 4: G=-Z                  (up=+Z)
    { 0, -1,  0},   // 5: G=+Y  techo          (up=-Y)
};
```

Deteccion de pared REUSANDO `blocked()` (lo sondea alrededor del jugador):

```cpp
// L+R (flanco): reorienta la gravedad hacia la pared mas cercana
int lr = ((pad.Buttons & PSP_CTRL_LTRIGGER) && (pad.Buttons & PSP_CTRL_RTRIGGER)) ? 1 : 0;
if (lr && !prevLR) {
    const float REACH = 2.2f;   // alcance de la sonda (u)
    int hitPX = blocked(playerX + REACH, playerZ, playerY);
    int hitNX = blocked(playerX - REACH, playerZ, playerY);
    int hitPZ = blocked(playerX, playerZ + REACH, playerY);
    int hitNZ = blocked(playerX, playerZ - REACH, playerY);
    if      (hitPX && !hitNX) gravAxis = 1;   // pared a +X -> G=+X
    else if (hitNX && !hitPX) gravAxis = 2;
    else if (hitPZ && !hitNZ) gravAxis = 3;
    else if (hitNZ && !hitPZ) gravAxis = 4;
    // sin pared clara: no cambia. (Otra pulsacion de L+R sin pared podria volver a 0.)
    tgtUpX = kUp[gravAxis][0]; tgtUpY = kUp[gravAxis][1]; tgtUpZ = kUp[gravAxis][2];
}
prevLR = lr;

// interpola el up hacia el objetivo (~0.3 s): evita el tiron de camara
const float UP_LERP = 0.08f;
upX += (tgtUpX - upX) * UP_LERP;
upY += (tgtUpY - upY) * UP_LERP;
upZ += (tgtUpZ - upZ) * UP_LERP;
```

> OJO de diseno de control: L+R queda como gesto de "reorientar gravedad", pero L
> solo (seccion 5) es "planear". Elige: o L+R para reorientar y L para planear (no
> chocan porque uno pide las dos gatillos), o mueve el planeo a otro boton. Tal
> como esta el bloque de arriba, L+R disparado por flanco no interfiere con el
> planeo continuo de L.

### 6.3 Fisica generalizada (lo que hay que escribir)

Cuando `gravAxis != 0`, la integracion vertical de la seccion 5 (pasos 6-7) deja
de servir tal cual: hay que empujar `velY` a lo largo de `U` y testear el suelo a
lo largo de `-U`. Como `blocked()`/`groundHeight()` son solo-Y, necesitas dos
helpers NUEVOS derivados de ellos (misma logica AABB de las `kStructures`, pero
con el eje elegido). Firma sugerida:

```cpp
// altura de suelo a lo largo del eje de gravedad (generaliza groundHeight)
float groundAlong(int axis, float px, float py, float pz);
// choque a lo largo de los dos ejes del PLANO (generaliza blocked por eje)
bool  blockedAlong(int axis, float px, float py, float pz);
```

Pseudocodigo de la integracion generalizada (reemplaza pasos 6-7 cuando hay
gravedad reorientada; para `axis==0` se comporta igual que hoy):

```
// componente de velocidad a lo largo de U ("arriba" actual)
velUp -= GRAVITY            // o la variante de planeo si L
pos    = pos + U * velUp    // integra en el eje de gravedad
suelo  = groundAlong(gravAxis, playerX, playerY, playerZ)
if (proyeccion de pos sobre U <= suelo) {
    fijar pos al suelo; velUp = 0; grounded = 1
} else grounded = 0
// el movimiento plano (velX/velZ del stick) se hace en los DOS ejes
// perpendiculares a U, con blockedAlong() por eje (mismo patron de deslizar)
```

### 6.4 Camara con up dinamico (esqueleto)

Reemplaza el bloque VIEW orbital actual (main.cpp 820-831) por un lookAt que
recibe el up interpolado. `sceGumLookAt` es de pspgum (existe en el toolchain):

```cpp
// mirada horizontal segun camYaw, reproyectada al plano perpendicular al up
float Lx = sinf(camYaw), Ly = 0.0f, Lz = -cosf(camYaw);
float d  = Lx*upX + Ly*upY + Lz*upZ;
Lx -= d*upX; Ly -= d*upY; Lz -= d*upZ;                 // quita la componente en up
float ln = sqrtf(Lx*Lx + Ly*Ly + Lz*Lz);
if (ln > 0.0001f) { Lx/=ln; Ly/=ln; Lz/=ln; }

ScePspFVector3 center = { playerX + upX*1.5f, playerY + upY*1.5f, playerZ + upZ*1.5f };
ScePspFVector3 eye    = { center.x - Lx*13.0f + upX*6.0f,   // detras (-mirada) y arriba (+up)
                          center.y - Ly*13.0f + upY*6.0f,
                          center.z - Lz*13.0f + upZ*6.0f };
ScePspFVector3 up     = { upX, upY, upZ };
sceGumMatrixMode(GU_VIEW);
sceGumLoadIdentity();
sceGumLookAt(&eye, &center, &up);
```

Para `gravAxis==0` (up = (0,1,0)) esto reproduce la camara 3a persona de siempre
(detras y arriba, ~13 u de distancia, ~6 u de alto). Al reorientar la gravedad, el
mismo codigo hace que la pared se vea como piso, con transicion suave por el lerp
de `up`. El modelo del jugador (bloque MODEL, main.cpp 855-862) debe rotar tambien
para alinear los pies con `U`; lo mas simple es construir su matriz a partir de la
base (forward reproyectado, up, y su producto cruz) en vez del `RotateXYZ` de solo
yaw actual.

### 6.5 Roadmap sugerido

1. Seccion 5 completa y sentida (movimiento, salto, planeo). <- primero.
2. `groundAlong` para +Y (techo) y probar gravedad invertida (el caso mas facil,
   sigue siendo eje Y).
3. `blockedAlong`/`groundAlong` para +X/-X/+Z/-Z (paredes) + camara lookAt.
4. Reorientar el modelo del jugador y pulir la interpolacion (curva ease, no lerp
   lineal) para que "correr por la pared" se sienta AAA.
