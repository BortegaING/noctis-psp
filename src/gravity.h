// ============================================================================
// gravity.h se compila en DOS FASES (por eso NO usa #pragma once):
//
//   FASE 1 - MATEMATICA PURA (gravDirVec / gravBasis / gravCamEuler).
//            No depende de nada del juego. Se emite en la PRIMERA inclusion,
//            justo donde main.cpp ya la incluye hoy (linea ~238).
//
//   FASE 2 - MUNDO (gravPickTarget / gravGroundAlong / gravBlocked).
//            Necesita CityBldg / g_city / g_cityCount, que main.cpp declara
//            RECIEN en la linea ~446. Por eso se emite solo si el archivo que
//            incluye define antes la macro NOCTIS_GRAVITY_WORLD.
//
//   CABLEADO en main.cpp (2 lineas nuevas, no se toca nada de lo existente):
//       static CityBldg g_city[256];
//       static int g_cityCount = 0;          // <-- linea ~448 que YA existe
//       #define NOCTIS_GRAVITY_WORLD         // <-- AGREGAR
//       #include "gravity.h"                 // <-- AGREGAR (2da inclusion)
//
//   (El #include "gravity.h" de la linea 238 se deja tal cual: da la FASE 1.)
// ============================================================================
#ifndef NOCTIS_GRAVITY_MATH_H
#define NOCTIS_GRAVITY_MATH_H
#include <math.h>

// ============================================================================
// PROJECT NOCTIS - gravity.h
// Helpers de MATEMATICA PURA para la mecanica de "direccion de gravedad".
//
// SIN estado de juego, SIN efectos secundarios, SIN globals, SIN I/O.
// Todo es determinista y header-only; el main loop llama estas funciones.
// Unico include: <math.h>.
//
// Convencion del mundo:
//   - Y-up. La camara mira hacia -Z por defecto con un leve pitch hacia abajo.
//   - Sistema derecho: para la base del plano se cumple  right x fwd = up,
//     donde  up = -gravedad  (el "arriba" del jugador es opuesto a la caida).
//
// 6 direcciones de gravedad. g:
//   0 = abajo  (-Y, normal, como el juego actual)
//   1 = arriba (+Y)
//   2 = +X
//   3 = -X
//   4 = +Z
//   5 = -Z
//
// Reorientacion de camara (parte que se suma a basePitch):
//   La camara base (g=0) mira -Z con pitch=basePitch (12 grados) hacia abajo.
//   Para cada g aplicamos una rotacion axis-aligned de 90/180 grados que lleva
//   el "abajo" viejo (-Y) sobre la nueva gravedad, para que la superficie
//   caminable quede leyendose como "suelo":
//     g=0 -> identidad                (roll=0,        pitch=basePitch)
//     g=1 -> roll 180 sobre eje vista (roll=PI,       mundo invertido)
//     g=2 -> roll +90 sobre eje vista (roll=+PI/2,    pared +X = suelo)
//     g=3 -> roll -90 sobre eje vista (roll=-PI/2,    pared -X = suelo)
//     g=4 -> pitch -90 sobre eje X    (pitch=base-PI/2, pared +Z = suelo)
//     g=5 -> pitch +90 sobre eje X    (pitch=base+PI/2, pared -Z = suelo)
//   Orden Euler asumido por el engine: yaw(Y) -> pitch(X) -> roll(Z), con roll
//   sobre el eje de vista (para g=0 el eje de vista es ~world Z). Por eso las
//   gravedades horizontales-de-pantalla (+X/-X) usan roll y las de profundidad
//   (+Z/-Z) usan pitch; base y offset comparten eje, asi que se suman directo.
//
// La MISMA rotacion se aplica a la base (right0,fwd0) para que el plano de
// movimiento y la camara queden siempre coherentes.
//
// ------ NOTA DE SANIDAD (verificacion g=0) ------
//   gravDirVec(0)  -> ( 0,-1, 0)
//   gravBasis(0)   -> right=(1,0,0), fwd=(0,0,-1)   (up = right x fwd = (0,1,0))
//   gravCamEuler(0)-> pitch=basePitch, yaw=0, roll=0
//   => reproduce exactamente el juego actual.  OK.
// ============================================================================

static const float NOCTIS_PI      = 3.14159265358979323846f;
static const float NOCTIS_HALF_PI = 1.57079632679489661923f;

// ----------------------------------------------------------------------------
// Vector unitario de la gravedad (hacia donde "cae" el jugador).
// ----------------------------------------------------------------------------
static void gravDirVec(int g, float *dx, float *dy, float *dz)
{
    float x = 0.0f, y = -1.0f, z = 0.0f; // default g=0: abajo (-Y)
    switch (g)
    {
        case 0: x = 0.0f; y = -1.0f; z =  0.0f; break; // abajo  -Y
        case 1: x = 0.0f; y =  1.0f; z =  0.0f; break; // arriba +Y
        case 2: x = 1.0f; y =  0.0f; z =  0.0f; break; //        +X
        case 3: x =-1.0f; y =  0.0f; z =  0.0f; break; //        -X
        case 4: x = 0.0f; y =  0.0f; z =  1.0f; break; //        +Z
        case 5: x = 0.0f; y =  0.0f; z = -1.0f; break; //        -Z
        default: break;                                 // fallback = g=0
    }
    *dx = x; *dy = y; *dz = z;
}

// ----------------------------------------------------------------------------
// Base ORTONORMAL del plano de movimiento (perpendicular a la gravedad).
//   right = eje horizontal-pantalla, fwd = "hacia adelante" (hacia dentro de
//   la pantalla) en ese plano.  Se cumple  right x fwd = up = -gravedad.
//
// Derivadas aplicando la rotacion de reorientacion R(g) a la base de g=0:
//   right0=(1,0,0), fwd0=(0,0,-1).
// ----------------------------------------------------------------------------
static void gravBasis(int g, float *rx, float *ry, float *rz,
                             float *fx, float *fy, float *fz)
{
    // Defaults = g=0 (juego actual).
    float Rx = 1.0f, Ry = 0.0f, Rz = 0.0f; // right = +X
    float Fx = 0.0f, Fy = 0.0f, Fz =-1.0f; // fwd   = -Z (hacia dentro)

    switch (g)
    {
        case 0: // abajo -Y : identidad
            Rx = 1.0f; Ry = 0.0f; Rz = 0.0f;
            Fx = 0.0f; Fy = 0.0f; Fz =-1.0f;
            break;
        case 1: // arriba +Y : roll 180 (mundo invertido) -> right voltea a -X
            Rx =-1.0f; Ry = 0.0f; Rz = 0.0f;
            Fx = 0.0f; Fy = 0.0f; Fz =-1.0f;
            break;
        case 2: // +X : roll +90 -> right = +Y
            Rx = 0.0f; Ry = 1.0f; Rz = 0.0f;
            Fx = 0.0f; Fy = 0.0f; Fz =-1.0f;
            break;
        case 3: // -X : roll -90 -> right = -Y
            Rx = 0.0f; Ry =-1.0f; Rz = 0.0f;
            Fx = 0.0f; Fy = 0.0f; Fz =-1.0f;
            break;
        case 4: // +Z : pitch -90 -> fwd baja a -Y
            Rx = 1.0f; Ry = 0.0f; Rz = 0.0f;
            Fx = 0.0f; Fy =-1.0f; Fz = 0.0f;
            break;
        case 5: // -Z : pitch +90 -> fwd sube a +Y
            Rx = 1.0f; Ry = 0.0f; Rz = 0.0f;
            Fx = 0.0f; Fy = 1.0f; Fz = 0.0f;
            break;
        default:
            break; // fallback = g=0
    }

    *rx = Rx; *ry = Ry; *rz = Rz;
    *fx = Fx; *fy = Fy; *fz = Fz;
}

// ----------------------------------------------------------------------------
// Euler (RAD) para la matriz VIEW: reorienta el mundo segun la gravedad de modo
// que la caminable quede como "suelo" y la gravedad apunte hacia abajo en la
// pantalla.  Orden asumido: yaw(Y) -> pitch(X) -> roll(Z), roll sobre eje vista.
//   g=0 -> pitch=basePitch, yaw=0, roll=0        (pasar basePitch=0.209f = 12 deg)
//   g=1 -> roll ~ PI      (mundo invertido)
//   g=2 -> roll ~ +PI/2   (pared derecha como suelo)
//   g=3 -> roll ~ -PI/2   (pared izquierda como suelo)
//   g=4 -> pitch ~ base-PI/2 (mira hacia el suelo +Z)
//   g=5 -> pitch ~ base+PI/2 (mira hacia el suelo -Z)
// ----------------------------------------------------------------------------
static void gravCamEuler(int g, float basePitch, float *pitch, float *yaw, float *roll)
{
    float p = basePitch, y = 0.0f, r = 0.0f; // default g=0

    switch (g)
    {
        case 0: p = basePitch;                 y = 0.0f; r = 0.0f;            break;
        case 1: p = basePitch;                 y = 0.0f; r = NOCTIS_PI;       break;
        case 2: p = basePitch;                 y = 0.0f; r = NOCTIS_HALF_PI;  break;
        case 3: p = basePitch;                 y = 0.0f; r =-NOCTIS_HALF_PI;  break;
        case 4: p = basePitch - NOCTIS_HALF_PI; y = 0.0f; r = 0.0f;           break;
        case 5: p = basePitch + NOCTIS_HALF_PI; y = 0.0f; r = 0.0f;           break;
        default: break; // fallback = g=0
    }

    *pitch = p; *yaw = y; *roll = r;
}

#endif // NOCTIS_GRAVITY_MATH_H  (fin FASE 1)

#if defined(NOCTIS_GRAVITY_WORLD) && !defined(NOCTIS_GRAVITY_WORLD_H)
#define NOCTIS_GRAVITY_WORLD_H

// ############################################################################
// #                                                                          #
// #   FASE 2 - GRAVEDAD CONTRA EL MUNDO  (necesita g_city / g_cityCount)      #
// #                                                                          #
// ############################################################################
//
// Se compila SOLO si quien incluye definio NOCTIS_GRAVITY_WORLD (ver cabecera).
// Sigue siendo header-only, determinista, sin heap, sin rand, sin globals
// nuevas y con <math.h> como unico include.
//
// ---------------------------------------------------------------------------
// LA IDEA TECNICA: INTERCAMBIO DE EJES
// ---------------------------------------------------------------------------
// Todo el mundo de NOCTIS son CAJAS ALINEADAS A LOS EJES (ver sector.h):
//     CityBldg { x, z, w, d, h }  ==  AABB
//         X en [x - w/2, x + w/2]
//         Y en [0      , h      ]        <-- OJO: TODA caja arranca en y=0
//         Z en [z - d/2, z + d/2]
// y las 6 direcciones de gravedad tambien estan alineadas a los ejes.
//
// Entonces caminar por una pared NO necesita colision generica: es EL MISMO
// test de caja de siempre, con los ejes renombrados. Para cada gravedad g:
//
//     eje VERTICAL  a  = gravAxis(g)     (0=X, 1=Y, 2=Z)
//     signo de caida s = gravSign(g)     (-1 hacia la coord. menor, +1 hacia la mayor)
//     "arriba" del jugador           u = -s   sobre ese mismo eje
//     ejes HORIZONTALES b1, b2       = los otros dos, (a+1)%3 y (a+2)%3
//
// Con eso:
//   - "altura del suelo"  ->  coordenada sobre el eje a  (gravGroundAlong)
//   - "huella en XZ"      ->  huella en los ejes b1/b2   (gravBlocked)
//   - "techo de la caja"  ->  la cara de la caja sobre el eje a que MIRA hacia
//                             el jugador: hi[a] si s<0, lo[a] si s>0.
//
// Para comparar alturas de forma uniforme se usa la COORDENADA ARRIBA:
//     U(c) = c * u
// asi "mas grande = mas arriba" vale para las 6 gravedades, y las formulas del
// juego original (que asumen g=0, u=+1) se copian tal cual.
//
// ---- VERIFICACION g=0 (tiene que reproducir el juego actual) ----
//   a=1 (Y), s=-1, u=+1, b1=2 (Z), b2=0 (X), U(c)=c
//   gravGroundAlong(0,..) == groundHeight(px,pz,py)   (mismo max b.h <= py+1, o 0)
//   gravBlocked(0,..,1.1) == blocked(px,pz,py)        (mismo py < b.h-0.8 + huella+r)
//   OK.
//
// ---------------------------------------------------------------------------
// COSTE DE ENERGIA (EN) DEL CAMBIO DE GRAVEDAD  -- constantes SUGERIDAS
// ---------------------------------------------------------------------------
// En main.cpp la barra ya vale: EN_MAX = 780, EN_REGEN = 5/frame, EN_FLOAT = 6/frame.
// La EN es LO QUE OBLIGA A PLANIFICAR LA RUTA: cada cambio de gravedad muerde un
// pedazo fijo de la barra y la barra SOLO se recarga con los pies apoyados. Si
// encadenas cambios en el aire te quedas sin EN a mitad de pared y te caes.
// ---------------------------------------------------------------------------
static const float GRAV_EN_SWITCH = 120.0f; // coste FIJO por cambio de gravedad.
                                            // 780/120 = 6 cambios con la barra llena.
static const float GRAV_EN_MIN    = 120.0f; // EN minima para poder cambiar (== coste:
                                            // no se permite quedar en negativo).
static const float GRAV_EN_REGEN  = 5.0f;   // recarga por frame SOLO si grounded==1.
                                            // 120/5 = 24 frames (~0.4 s) de pies apoyados
                                            // para recuperar UN cambio.
static const int   GRAV_SWITCH_CD = 18;     // frames de bloqueo entre cambios (~0.3 s a
                                            // 60 fps): evita el tartamudeo de gravedad.

// --- geometria del SECTOR: DEBEN COINCIDIR con sector.h -----------------------
// (No se puede incluir sector.h desde aca: sector.h se incluye DESPUES y depende
//  de RGBA/addQuadT. Si se tocan SEC_CEIL / SEC_HALF, actualizar estos valores.)
static const float GRAV_FLOOR_Y   =   0.0f; // plano del suelo            (== y=0)
static const float GRAV_CEIL_Y    =  60.0f; // plano del techo            (== SEC_CEIL)
static const float GRAV_SECT_HALF =  54.0f; // cara INTERIOR de los muros (== SEC_HALF)
static const float GRAV_WORLD_LIM = 200.0f; // red de seguridad en XZ (igual que blocked())

// --- tolerancias del jugador --------------------------------------------------
static const float GRAV_BODY_H     =  3.0f; // alto del cuerpo (== g_playerBox: 1.4x1.4x3.0).
                                            // Se mide DESDE LOS PIES hacia su "arriba" (u).
static const float GRAV_STEP_TOL   =  0.8f; // margen para pararse encima sin quedar trabado
                                            // (es el mismo 0.8f de blocked() en main.cpp).
static const float GRAV_LAND_TOL   =  1.0f; // margen para aceptar una cara como apoyo
                                            // (es el mismo +1.0f de groundHeight()).
static const float GRAV_PICK_RANGE = 30.0f; // alcance del rayo de gravPickTarget (unidades).
static const float GRAV_PICK_EPS   =  0.05f;// no se considera nada mas cerca que esto
                                            // (evita elegir la superficie que ya pisas).
static const float GRAV_EPS        =  1e-6f;

// ----------------------------------------------------------------------------
// Eje sobre el que actua la gravedad g.  0 = X, 1 = Y, 2 = Z.
//   g=0,1 -> Y      g=2,3 -> X      g=4,5 -> Z
// ----------------------------------------------------------------------------
static int gravAxis(int g)
{
    if (g < 0 || g > 5) g = 0;              // fallback = abajo
    return (g < 2) ? 1 : ((g < 4) ? 0 : 2);
}

// ----------------------------------------------------------------------------
// Signo de la gravedad sobre su eje: -1 = cae hacia la coordenada MENOR,
// +1 = cae hacia la MAYOR.  (Coincide con la componente no nula de gravDirVec.)
//   g=0 -Y:-1   g=1 +Y:+1   g=2 +X:+1   g=3 -X:-1   g=4 +Z:+1   g=5 -Z:-1
// ----------------------------------------------------------------------------
static float gravSign(int g)
{
    switch (g) {
        case 0: return -1.0f;   // abajo  -Y
        case 1: return  1.0f;   // arriba +Y
        case 2: return  1.0f;   //        +X
        case 3: return -1.0f;   //        -X
        case 4: return  1.0f;   //        +Z
        case 5: return -1.0f;   //        -Z
        default: return -1.0f;  // fallback = g=0
    }
}

// ----------------------------------------------------------------------------
// Extremos [lo, hi] de la caja i de g_city sobre el eje pedido (0=X, 1=Y, 2=Z).
// Aca es donde vive el "intercambio de ejes": el resto del archivo ya no sabe
// que la caja se guarda como (x, z, w, d, h) ni que siempre arranca en y=0.
// ----------------------------------------------------------------------------
static void gravBoxSpan(int i, int axis, float *lo, float *hi)
{
    const CityBldg &b = g_city[i];
    if (axis == 0)      { *lo = b.x - b.w * 0.5f; *hi = b.x + b.w * 0.5f; }  // X
    else if (axis == 1) { *lo = 0.0f;             *hi = b.h;              }  // Y (siempre desde y=0)
    else                { *lo = b.z - b.d * 0.5f; *hi = b.z + b.d * 0.5f; }  // Z
}

// ----------------------------------------------------------------------------
// Cara golpeada -> direccion de gravedad que la convierte en SUELO.
// Si la cara tiene normal exterior n, hay que caer CONTRA ella: gravedad = -n.
//   n=+Y (techo de una caja, o el suelo y=0)  -> gravedad -Y  = 0
//   n=-Y (cara inferior, o el techo y=60)     -> gravedad +Y  = 1
//   n=+X -> gravedad -X = 3      n=-X -> gravedad +X = 2
//   n=+Z -> gravedad -Z = 5      n=-Z -> gravedad +Z = 4
// ----------------------------------------------------------------------------
static int gravFromFaceNormal(int axis, float nsign)
{
    if (axis == 1) return (nsign > 0.0f) ? 0 : 1;   // Y
    if (axis == 0) return (nsign > 0.0f) ? 3 : 2;   // X
    return                (nsign > 0.0f) ? 5 : 4;   // Z
}

// ============================================================================
// 1)  gravPickTarget  -  "cambia la gravedad HACIA LO QUE ESTAS MIRANDO"
// ----------------------------------------------------------------------------
// Lanza un rayo corto (GRAV_PICK_RANGE = 30 u) desde (px,py,pz) en la direccion
// (fwdX,fwdY,fwdZ) -- NO hace falta que venga normalizada -- contra:
//     - las g_cityCount cajas de g_city   (slab test, guardando la cara de ENTRADA)
//     - el plano del suelo   y = GRAV_FLOOR_Y = 0   (normal +Y)
//     - el plano del techo   y = GRAV_CEIL_Y  = 60  (normal -Y)
// y devuelve el INDICE DE GRAVEDAD (0..5) que convertiria esa cara en suelo.
//
// Devuelve -1 si no golpea nada util: rayo degenerado, nada dentro del alcance,
// o el origen esta DENTRO de una caja (no hay cara de entrada que elegir).
//
// NOTA: devuelve una DIRECCION, no un punto. Puede devolver la gravedad que ya
// tenias (si miras el piso que pisas); main.cpp decide si la ignora.
// ============================================================================
static int gravPickTarget(float px, float py, float pz, float fwdX, float fwdY, float fwdZ)
{
    const float len2 = fwdX * fwdX + fwdY * fwdY + fwdZ * fwdZ;
    if (len2 < GRAV_EPS) return -1;                       // mirada degenerada
    const float inv = 1.0f / sqrtf(len2);
    const float o[3] = { px, py, pz };
    const float d[3] = { fwdX * inv, fwdY * inv, fwdZ * inv };

    float bestT = GRAV_PICK_RANGE;                        // solo interesa lo mas cercano
    int   bestG = -1;

    // ---- cajas: slab test clasico, pero anotando POR QUE CARA entra el rayo ----
    for (int i = 0; i < g_cityCount; ++i) {
        float lo[3], hi[3];
        gravBoxSpan(i, 0, &lo[0], &hi[0]);
        gravBoxSpan(i, 1, &lo[1], &hi[1]);
        gravBoxSpan(i, 2, &lo[2], &hi[2]);

        float tNear = GRAV_PICK_EPS;   // ventana util del rayo: [eps, mejor hasta ahora]
        float tFar  = bestT;
        int   hitAxis = -1;            // eje de la cara de ENTRADA
        float hitSign = 0.0f;          // normal exterior de esa cara: -1 (cara lo) o +1 (cara hi)
        int   miss = 0;

        for (int a = 0; a < 3 && !miss; ++a) {
            if (d[a] > -GRAV_EPS && d[a] < GRAV_EPS) {    // rayo paralelo a este par de caras
                if (o[a] < lo[a] || o[a] > hi[a]) miss = 1;
                continue;                                 // si esta dentro del slab, no acota
            }
            const float invd = 1.0f / d[a];
            float t1 = (lo[a] - o[a]) * invd;             // cruce con la cara lo (normal -1)
            float t2 = (hi[a] - o[a]) * invd;             // cruce con la cara hi (normal +1)
            float sgn = -1.0f;                            // por defecto entra por la cara lo
            if (t1 > t2) { const float tmp = t1; t1 = t2; t2 = tmp; sgn = 1.0f; } // d<0: entra por hi
            if (t1 > tNear) { tNear = t1; hitAxis = a; hitSign = sgn; }
            if (t2 < tFar)  tFar  = t2;
            if (tNear > tFar) miss = 1;                   // los slabs no se solapan: sin impacto
        }
        // hitAxis < 0 => el rayo ya entraba dentro de los 3 slabs en t<=eps, o sea el
        // origen esta dentro de la caja: no hay cara de entrada, se ignora esa caja.
        if (miss || hitAxis < 0) continue;

        bestT = tNear;                                    // tNear <= tFar <= bestT garantizado
        bestG = gravFromFaceNormal(hitAxis, hitSign);
    }

    // ---- suelo del sector (y = 0, normal +Y): solo si el rayo BAJA ----
    if (d[1] < -GRAV_EPS) {
        const float t = (GRAV_FLOOR_Y - o[1]) / d[1];
        if (t > GRAV_PICK_EPS && t < bestT) {
            const float hx = o[0] + d[0] * t, hz = o[2] + d[2] * t;
            if (hx > -GRAV_SECT_HALF && hx < GRAV_SECT_HALF &&
                hz > -GRAV_SECT_HALF && hz < GRAV_SECT_HALF) { bestT = t; bestG = 0; }
        }
    }
    // ---- techo del sector (y = 60, normal -Y): solo si el rayo SUBE ----
    if (d[1] > GRAV_EPS) {
        const float t = (GRAV_CEIL_Y - o[1]) / d[1];
        if (t > GRAV_PICK_EPS && t < bestT) {
            const float hx = o[0] + d[0] * t, hz = o[2] + d[2] * t;
            if (hx > -GRAV_SECT_HALF && hx < GRAV_SECT_HALF &&
                hz > -GRAV_SECT_HALF && hz < GRAV_SECT_HALF) { bestT = t; bestG = 1; }
        }
    }

    return bestG;
}

// ============================================================================
// 2)  gravGroundAlong  -  groundHeight() generalizado a las 6 gravedades
// ----------------------------------------------------------------------------
// Devuelve la COORDENADA (en unidades de mundo, sobre el eje gravAxis(g)) de la
// superficie de apoyo que esta "debajo" del jugador en el sentido de la caida.
// Es donde quedan los PIES al aterrizar:
//     g=0 (-Y) -> devuelve un valor de Y  (techo de una caja, o 0)
//     g=1 (+Y) -> devuelve un valor de Y  (cara inferior de una caja, o 60)
//     g=2 (+X) -> devuelve un valor de X  (cara -X de una caja, o +54)
//     g=3 (-X) -> devuelve un valor de X  (cara +X de una caja, o -54)
//     g=4 (+Z) -> devuelve un valor de Z  (cara -Z de una caja, o +54)
//     g=5 (-Z) -> devuelve un valor de Z  (cara +Z de una caja, o -54)
//
// Recorre g_city: una caja aporta apoyo solo si el jugador esta DENTRO de su
// huella en LOS OTROS DOS EJES (ese es el intercambio de ejes: para gravedad
// +-X la "huella" es en Y y Z, no en X y Z). De las candidatas se queda con la
// mas CERCANA en el sentido de la caida. Si ninguna sirve devuelve el limite del
// sector en esa direccion (suelo, techo, o la cara interior del muro).
//
// Se aceptan caras hasta GRAV_LAND_TOL (1 u) POR ENCIMA de los pies, igual que
// el "b.h <= py + 1.0f" del groundHeight original (margen de aterrizaje).
// ============================================================================
static float gravGroundAlong(int g, float px, float py, float pz)
{
    if (g < 0 || g > 5) g = 0;
    const int   a  = gravAxis(g);
    const float sg = gravSign(g);
    const int   b1 = (a + 1) % 3, b2 = (a + 2) % 3;    // los OTROS dos ejes
    const float p[3] = { px, py, pz };
    const float pa = p[a];                             // coordenada "vertical" del jugador

    // Limite del sector en la direccion de la caida = apoyo de ultimo recurso.
    float best;
    if (a == 1) best = (sg < 0.0f) ?  GRAV_FLOOR_Y   : GRAV_CEIL_Y;    // suelo / techo
    else        best = (sg < 0.0f) ? -GRAV_SECT_HALF : GRAV_SECT_HALF; // cara interior del muro
    // dist > 0 => la cara esta POR DEBAJO del jugador (en el sentido de la caida).
    float bestD = (best - pa) * sg;
    if (bestD < -GRAV_LAND_TOL) bestD = 1.0e30f;   // el limite quedo ARRIBA: no es apoyo valido,
                                                   // pero se conserva como valor de retorno.

    for (int i = 0; i < g_cityCount; ++i) {
        float lo[3], hi[3];
        gravBoxSpan(i, 0, &lo[0], &hi[0]);
        gravBoxSpan(i, 1, &lo[1], &hi[1]);
        gravBoxSpan(i, 2, &lo[2], &hi[2]);

        // huella: el jugador tiene que estar dentro de la caja en los otros dos ejes
        if (p[b1] < lo[b1] || p[b1] > hi[b1]) continue;
        if (p[b2] < lo[b2] || p[b2] > hi[b2]) continue;

        // cara de apoyo = la que MIRA hacia el jugador (contra la gravedad):
        //   sg<0 (cae hacia menos) -> te frena la cara hi  (ej. g=0: el techo y=h)
        //   sg>0 (cae hacia mas)   -> te frena la cara lo  (ej. g=2: la cara -X de la caja)
        const float face = (sg < 0.0f) ? hi[a] : lo[a];
        const float dist = (face - pa) * sg;
        if (dist < -GRAV_LAND_TOL) continue;       // esa cara esta por ENCIMA: no es apoyo
        if (dist < bestD) { bestD = dist; best = face; }
    }
    return best;
}

// ============================================================================
// 3)  gravBlocked  -  blocked() generalizado a las 6 gravedades
// ----------------------------------------------------------------------------
// true = el jugador (cilindro de radio `radius`, alto GRAV_BODY_H medido desde
// los PIES hacia su "arriba") choca con algo y no puede ocupar (px,py,pz).
//
// OJO CON EL ORDEN DE LOS ARGUMENTOS: el blocked() de main.cpp es
// blocked(px, pz, py) (Z antes que Y). Aca es (g, px, PY, PZ, radius), en orden
// natural X,Y,Z. Al cablear NO copiar el orden viejo.
//
// Test, con los ejes intercambiados segun g:
//   - huella en los DOS ejes horizontales (b1,b2), inflada por `radius`;
//   - solape sobre el eje vertical (a) usando la coordenada arriba U(c)=c*u:
//         pies  = U(pa)            cabeza = U(pa) + GRAV_BODY_H
//         caja  = [U(cara lejana), U(cara de apoyo) - GRAV_STEP_TOL]
//     El -GRAV_STEP_TOL es el que permite PARARSE ENCIMA sin quedar trabado
//     (mismo 0.8f del original). El test de la cabeza (uHead > uBot) es NUEVO y
//     es imprescindible fuera de g=0: como todas las cajas arrancan en y=0, sin
//     el, caminar por el TECHO (g=1) quedaria bloqueado por las masas de abajo.
//
// Ademas frena las dos superficies del sector que NO son cajas de g_city:
//   - el suelo y=0 y el techo y=60 son solidos. Cuando la gravedad es horizontal
//     (+-X / +-Z) el eje Y pasa a ser HORIZONTAL y esos planos hay que frenarlos
//     a mano, o el jugador se sale del recinto caminando por la pared.
//   - red de seguridad en XZ (radio GRAV_WORLD_LIM), igual que blocked().
// ============================================================================
static bool gravBlocked(int g, float px, float py, float pz, float radius)
{
    if (g < 0 || g > 5) g = 0;
    const int   a  = gravAxis(g);
    const float sg = gravSign(g);
    const float u  = -sg;                              // "arriba" del jugador sobre el eje a
    const int   b1 = (a + 1) % 3, b2 = (a + 2) % 3;
    const float p[3] = { px, py, pz };

    // red de seguridad: fuera del recinto = bloqueado
    if (px * px + pz * pz > GRAV_WORLD_LIM * GRAV_WORLD_LIM) return true;

    // suelo y techo del sector, cuando Y es un eje HORIZONTAL (gravedad +-X / +-Z)
    if (a != 1) {
        if (py - radius < GRAV_FLOOR_Y) return true;
        if (py + radius > GRAV_CEIL_Y)  return true;
    }

    const float uFeet = p[a] * u;                      // coordenada arriba de los pies
    const float uHead = uFeet + GRAV_BODY_H;           // ... y de la cabeza

    for (int i = 0; i < g_cityCount; ++i) {
        float lo[3], hi[3];
        gravBoxSpan(i, 0, &lo[0], &hi[0]);
        gravBoxSpan(i, 1, &lo[1], &hi[1]);
        gravBoxSpan(i, 2, &lo[2], &hi[2]);

        // huella en los dos ejes horizontales, inflada por el radio del jugador
        if (p[b1] <= lo[b1] - radius || p[b1] >= hi[b1] + radius) continue;
        if (p[b2] <= lo[b2] - radius || p[b2] >= hi[b2] + radius) continue;

        // span de la caja sobre el eje vertical, en coordenada arriba.
        // Al multiplicar por u el orden se puede dar vuelta -> se reordena.
        const float uA = lo[a] * u, uB = hi[a] * u;
        const float uBot = (uA < uB) ? uA : uB;        // cara LEJANA (el "piso" de la caja)
        const float uTop = (uA < uB) ? uB : uA;        // cara de APOYO (el "techo" de la caja)

        if (uFeet < uTop - GRAV_STEP_TOL && uHead > uBot) return true;
    }
    return false;
}

#endif // NOCTIS_GRAVITY_WORLD  (fin FASE 2)
