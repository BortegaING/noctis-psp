#pragma once
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
