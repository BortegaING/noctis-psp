#pragma once
// ============================================================================
// PROJECT NOCTIS - world_def.h
// LA UNICA FUENTE DE VERDAD DEL SECTOR (medidas, colores y formulas).
//
// POR QUE EXISTE ESTE ARCHIVO
// ---------------------------------------------------------------------------
// Las medidas del recinto estaban ESCRITAS SEIS VECES:
//   sector.h    las define (SEC_*)                       <- el original
//   objective.h las REPLICA como constantes propias      <- lo dice en su nota
//   gargoyles.h las REPLICA (GG_*) + el vector "arriba"  <- lo dice en su nota
//   tracery.h   RECALCULA la formula de los ventanales   <- "misma formula que sector.h"
//   hud.h       dibuja el minimapa con 9/45/56 a mano
//   vault.h     duplica ademas los colores de piedra
// Consecuencia: cambiar el tamano del mundo (p.ej. para tener NIVELES distintos)
// dejaba cuatro archivos MINTIENDO, y las gargolas o los materiales nacian
// dentro de una pared. Aqui esta todo UNA vez; los demas archivos leen de aca.
//
// REGLAS DE LA CASA (respetadas)
//   * header-only, sin heap, sin rand, sin I/O, C++17, <math.h> como unico include.
//   * NO depende de main.cpp: no usa RGBA(), brighten(), TILE, LineVertex ni
//     TexVertex. Se puede incluir ANTES QUE TODO LO DEMAS (incluso antes del
//     #define RGBA de main.cpp), que es justo lo que hace falta para que
//     objective.h / gargoyles.h dejen de duplicar por miedo al orden de inclusion.
//   * ESTE PASO NO CAMBIA EL MUNDO: cada numero reproduce EXACTAMENTE el valor
//     que hoy esta hardcodeado. Es centralizacion pura, la imagen no se mueve.
//
// ---------------------------------------------------------------------------
// EL MAPA, DE UNA VEZ
// ---------------------------------------------------------------------------
//   planta (vista desde arriba)                     corte
//   +--------------------------------+   muro exterior |x|,|z| = 58
//   |  ####      pasillo      ####   |   eje del muro          = 56  <- minimapa
//   |  ####        |          ####   |   cara INTERIOR         = 54  <- se camina
//   |  -----------+-----------       |   techo          y = 60
//   |  ####        |          ####   |   techo de masa  y = 30 (cornisa visible 31.6)
//   +--------------------------------+   capitel        y = 18.6
//   4 masas de 36x36 en |x|,|z| € [9,45]  suelo         y = 0
//   pasillo en cruz de 18 (|x|<9 o |z|<9) + perimetral de 9 (|x|>45 o |z|>45)
//
// LAS TRES MEDIDAS QUE SE CONFUNDEN TODO EL TIEMPO (y por eso tienen nombre propio):
//   WD_SEC_HALF (54) = cara INTERIOR del muro. Es el limite que camina el jugador.
//   WD_WALL_MID (56) = EJE del muro. Es lo que dibuja el minimapa de hud.h y lo
//                      que usa objective.h como red de seguridad (OBJ_OUT_XZ).
//   WD_WALL_OUT (58) = cara EXTERIOR. Hasta aqui llega el suelo teselado.
// ============================================================================

#include <math.h>

// ============================================================================
// 0) COLOR SIN DEPENDER DE main.cpp
// ============================================================================
// Replica EXACTA del macro RGBA y de la funcion brighten() de main.cpp
// (formato 0xAABBGGRR, y brighten trunca a entero y satura en 255 con alfa 255).
// Se reimplementan aqui, en constexpr, porque este header tiene que poder
// incluirse ANTES que main.cpp defina nada.
static constexpr unsigned int wdRGBA(int r, int g, int b, int a) {
    return (unsigned int)(((a) << 24) | ((b) << 16) | ((g) << 8) | (r));
}
static constexpr int wdSat255(int v) { return (v > 255) ? 255 : v; }
static constexpr unsigned int wdBrighten(unsigned int c, float f) {
    return wdRGBA(wdSat255((int)((float)( c        & 0xFF) * f)),
                  wdSat255((int)((float)((c >>  8) & 0xFF) * f)),
                  wdSat255((int)((float)((c >> 16) & 0xFF) * f)), 255);
}

// ============================================================================
// 1) EL RECINTO
// ============================================================================
static constexpr float WD_SEC_HALF   = 54.0f;   // cara INTERIOR del muro: media anchura util
static constexpr float WD_SEC_CEIL   = 60.0f;   // TECHO del sector (y). = 2 x altura de una masa
static constexpr float WD_WALL_T     =  4.0f;   // espesor del muro exterior
static constexpr float WD_WALL_MID   = WD_SEC_HALF + WD_WALL_T * 0.5f;   // 56: EJE del muro
static constexpr float WD_WALL_OUT   = WD_SEC_HALF + WD_WALL_T;          // 58: cara exterior
static constexpr float WD_WALL_LEN   = 2.0f * WD_WALL_OUT;               // 116: largo de un muro
static constexpr float WD_FLOOR_HALF = WD_WALL_OUT;                      // 58: el suelo/techo
                                                                         //     teselado llega
                                                                         //     hasta el muro
static constexpr int   WD_FLOOR_CELLS = 20;     // celdas por lado del suelo/techo (~5.8u)

// ============================================================================
// 2) LAS 4 MASAS ("edificios")
// ============================================================================
static constexpr float WD_MASS_M0   =  9.0f;    // borde INTERIOR -> da el pasillo en cruz
static constexpr float WD_MASS_M1   = 45.0f;    // borde EXTERIOR -> da el pasillo perimetral
static constexpr float WD_MASS_H    = 30.0f;    // altura (su techo es PISABLE, colision en y=30)
static constexpr float WD_MASS_SIDE = WD_MASS_M1 - WD_MASS_M0;              // 36
static constexpr float WD_MASS_HALF = WD_MASS_SIDE * 0.5f;                  // 18
static constexpr float WD_MASS_CTR  = (WD_MASS_M0 + WD_MASS_M1) * 0.5f;     // 27: centro de masa
// remate visual: la colision termina en 30 pero la CORNISA dibujada sigue 1.6 mas.
// Cualquier adorno que apoye "sobre el techo de una masa" tiene que usar WD_MASS_TOP_VIS
// (es el `vis` de las anclas de objective.h y la y de los nidos de gargoyles.h).
static constexpr float WD_CORNICE_H   = 1.6f;
static constexpr float WD_MASS_TOP_VIS = WD_MASS_H + WD_CORNICE_H;          // 31.6

// ============================================================================
// 3) LOS PASILLOS
// ============================================================================
static constexpr float WD_CROSS_W = 2.0f * WD_MASS_M0;                 // 18: pasillo en cruz
static constexpr float WD_PERIM_W = WD_SEC_HALF - WD_MASS_M1;          //  9: pasillo perimetral

// ============================================================================
// 4) LA ARCADA: 20 COLUMNAS EXENTAS
// ============================================================================
// Van en los bordes de los dos brazos del pasillo en cruz (|x| = 7 y |z| = 7),
// 5 por lado (k = -2..2, cada 16). El CAPITEL es pisable a y = 18.6.
static constexpr float WD_COL_OFF  =  7.0f;   // separacion al eje del pasillo (cara de masa en 9)
static constexpr float WD_COL_STEP = 16.0f;   // paso entre columnas
static constexpr float WD_COL_W    =  3.3f;   // lado del fuste para COLISION (= lado de la basa)
static constexpr float WD_COL_H    = 18.6f;   // alto total con capitel = superficie pisable
static constexpr int   WD_COL_SEG  =  5;      // el fuste se dibuja en TRAMOS (recorte de camara)
static constexpr int   WD_COL_KMIN = -2;      // primer indice a lo largo del pasillo
static constexpr int   WD_COL_KMAX =  2;      // ultimo
static constexpr int   WD_COL_COUNT = 2 * 2 * (WD_COL_KMAX - WD_COL_KMIN + 1);   // 20
// despiece vertical dibujado (basa + fuste + capitel tiene que dar WD_COL_H)
static constexpr float WD_COL_R       =  1.5f;   // "radio" del que salen todos los anchos
static constexpr float WD_COL_BASE_H  =  1.2f;   // basa
static constexpr float WD_COL_SHAFT_H = 16.0f;   // fuste (en WD_COL_SEG tramos)
static constexpr float WD_COL_CAP_H   =  1.4f;   // capitel
// arcos ojivales entre columna y columna: arrancan SOBRE el capitel
static constexpr float WD_ARCH_BASE = WD_COL_H;                 // 18.6
static constexpr float WD_ARCH_RISE =  7.0f;
static constexpr float WD_ARCH_LOW  = WD_ARCH_BASE + WD_ARCH_RISE * 0.30f;  // 20.7: lo mas bajo
                                                                            // que cuelga encima
                                                                            // de un capitel

// ============================================================================
// 5) LOS VENTANALES DE LOS MUROS
// ============================================================================
// Cada muro es: antefecho macizo (0..7), franja de VANOS (7..27) partida por
// pilares, y muro macizo (27..60). La COLISION es el muro entero: se ve hacia
// afuera pero no se sale.
static constexpr float WD_WIN_Y0  =  7.0f;   // remate del antefecho = base del vano
static constexpr float WD_WIN_Y1  = 27.0f;   // arranque del muro alto = dintel del vano
static constexpr float WD_PIER_W  =  5.0f;   // ancho del pilar entre vano y vano
static constexpr float WD_BAY     = 19.0f;   // paso (pilar + vano)
static constexpr float WD_BAY_HALF = (WD_BAY - WD_PIER_W) * 0.5f;   // 7: media luz del vano
static constexpr float WD_WIN_H    = WD_WIN_Y1 - WD_WIN_Y0;         // 20: alto del vano

// ============================================================================
// 6) LA PALETA COMPARTIDA
// ============================================================================
// Estos 4 colores estaban copiados en sector.h, vault.h y tracery.h (la piedra)
// y solo en sector.h (suelo/techo). La forma RAW + multiplicador se conserva
// porque es como se escribio el original; los valores ya multiplicados estan
// abajo por si conviene ahorrarse la llamada.
static constexpr unsigned int WD_STONE_RAW = wdRGBA(69, 65, 58, 255);
static constexpr float        WD_STONE_MUL = 2.1f;
static constexpr unsigned int WD_DARK_RAW  = wdRGBA(46, 41, 36, 255);
static constexpr float        WD_DARK_MUL  = 2.0f;

static constexpr unsigned int WD_COL_STONE = wdBrighten(WD_STONE_RAW, WD_STONE_MUL); // (144,136,121)
static constexpr unsigned int WD_COL_DARK  = wdBrighten(WD_DARK_RAW,  WD_DARK_MUL);  // ( 92, 82, 72)
static constexpr unsigned int WD_COL_FLOOR = wdRGBA(212, 194, 172, 255);  // losas del suelo
static constexpr unsigned int WD_COL_CEIL  = wdRGBA(104,  92,  76, 255);  // boveda (mas oscuro)

// ============================================================================
// 7) GRAVEDAD: el indice de cara y su vector "arriba"
// ============================================================================
// Convenio COMPARTIDO por gravity.h (gravDirVec), objective.h (objUpVec/face) y
// gargoyles.h (gargUpVec/GargNest::face):
//   0 = gravedad -Y (suelo, techos de masa, capiteles)   1 = +Y (TECHO del sector)
//   2 = +X (se pisa una cara -X)                         3 = -X (cara +X)
//   4 = +Z (cara -Z)                                     5 = -Z (cara +Z)
// "arriba" = -gravedad.  wdGravityDown() reproduce gravDirVec() de gravity.h.
static constexpr int WD_GRAV_N = 6;

static inline void wdGravityUp(int g, float *ux, float *uy, float *uz) {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    switch (g) {
        case 0:  y =  1.0f; break;   // gravedad -Y -> arriba +Y
        case 1:  y = -1.0f; break;   // gravedad +Y -> arriba -Y (TECHO)
        case 2:  x = -1.0f; break;   // gravedad +X -> arriba -X
        case 3:  x =  1.0f; break;
        case 4:  z = -1.0f; break;   // gravedad +Z -> arriba -Z
        case 5:  z =  1.0f; break;
        default: y =  1.0f; break;   // mismo fallback que objUpVec/gargUpVec
    }
    *ux = x; *uy = y; *uz = z;
}

static inline void wdGravityDown(int g, float *dx, float *dy, float *dz) {
    float ux, uy, uz; wdGravityUp(g, &ux, &uy, &uz);
    *dx = -ux; *dy = -uy; *dz = -uz;
}

// ============================================================================
// 8) LAS 4 MASAS: centro, pertenencia y pasillos
// ============================================================================
// Cuadrantes con el MISMO convenio que sector.h/hud.h/vault.h:
//   sx = (q & 1) ? -1 : +1 ,  sz = (q & 2) ? -1 : +1
//   q0 = (+,+) NE    q1 = (-,+) NO    q2 = (+,-) SE    q3 = (-,-) SO
static constexpr int WD_MASS_N = 4;

static constexpr float wdQuadSX(int q) { return (q & 1) ? -1.0f : 1.0f; }
static constexpr float wdQuadSZ(int q) { return (q & 2) ? -1.0f : 1.0f; }

// centro en planta de la masa q (su altura va de 0 a WD_MASS_H)
static inline void wdMassCenter(int q, float *cx, float *cz) {
    *cx = WD_MASS_CTR * wdQuadSX(q);
    *cz = WD_MASS_CTR * wdQuadSZ(q);
}
static constexpr float wdMassHalf() { return WD_MASS_HALF; }   // 18 = medio lado
static constexpr float wdMassSide() { return WD_MASS_SIDE; }   // 36 = lado completo

// 1 = el punto (en planta) cae DENTRO de alguna de las 4 masas. Es el test que
// hacen a mano gargLOS() y gargAvoid(): |x| y |z| ambos dentro de [M0, M1].
static inline int wdIsInsideMass(float x, float z) {
    const float ax = fabsf(x), az = fabsf(z);
    return (ax >= WD_MASS_M0 && ax <= WD_MASS_M1 &&
            az >= WD_MASS_M0 && az <= WD_MASS_M1) ? 1 : 0;
}
// idem teniendo en cuenta la altura: por encima de WD_MASS_H ya no hay piedra.
static inline int wdIsInsideMassY(float x, float y, float z) {
    if (y > WD_MASS_H) return 0;
    return wdIsInsideMass(x, z);
}
// version con holgura: `clr` agranda la masa en planta y en altura (GG_CLR).
static inline int wdIsInsideMassClr(float x, float y, float z, float clr) {
    if (y > WD_MASS_H + clr) return 0;
    const float ax = fabsf(x), az = fabsf(z);
    return (ax >= WD_MASS_M0 - clr && ax <= WD_MASS_M1 + clr &&
            az >= WD_MASS_M0 - clr && az <= WD_MASS_M1 + clr) ? 1 : 0;
}

// dentro de los muros (cara interior)
static inline int wdIsInsideSector(float x, float z) {
    return (fabsf(x) <= WD_SEC_HALF && fabsf(z) <= WD_SEC_HALF) ? 1 : 0;
}
// el brazo en cruz (el del centro) y el anillo perimetral, por separado
static inline int wdIsInCrossCorridor(float x, float z) {
    if (!wdIsInsideSector(x, z)) return 0;
    return (fabsf(x) < WD_MASS_M0 || fabsf(z) < WD_MASS_M0) ? 1 : 0;
}
static inline int wdIsInPerimeter(float x, float z) {
    if (!wdIsInsideSector(x, z)) return 0;
    return (fabsf(x) > WD_MASS_M1 || fabsf(z) > WD_MASS_M1) ? 1 : 0;
}
// suelo libre: dentro del recinto y fuera de la piedra de las masas
static inline int wdIsInCorridor(float x, float z) {
    return (wdIsInsideSector(x, z) && !wdIsInsideMass(x, z)) ? 1 : 0;
}

// ============================================================================
// 9) LAS 20 COLUMNAS
// ============================================================================
// axis: 0 = arcada del pasillo en Z (las columnas se separan en X, |x| = 7)
//       1 = arcada del pasillo en X (|z| = 7)
// side: -1 / +1  (los dos lados del pasillo)
// k   : WD_COL_KMIN..WD_COL_KMAX  (posicion a lo largo del pasillo, cada 16)
// Reproduce literalmente el triple bucle de buildSectorCollision/buildSectorWalls.
static inline void wdColumnPos(int axis, int side, int k, float *x, float *z) {
    const float t = (float)k * WD_COL_STEP;
    const float o = WD_COL_OFF * (float)side;
    if (axis == 0) { *x = o; *z = t; }
    else           { *x = t; *z = o; }
}
static constexpr int wdColumnCount() { return WD_COL_COUNT; }   // 20
// altura pisable del capitel (misma que WD_COL_H: basa + fuste + capitel)
static constexpr float wdColumnTop() { return WD_COL_BASE_H + WD_COL_SHAFT_H + WD_COL_CAP_H; }

// ============================================================================
// 10) LOS VENTANALES: pilares y vanos
// ============================================================================
// LA formula que hoy vive por duplicado en sector.h (buildSectorWalls) y
// tracery.h (buildTracery).  Con los numeros actuales: 7 pilares y 6 vanos por
// muro, centros de vano en u = -48.5, -29.5, -10.5, 8.5, 27.5, 46.5 -> 24 vanos.
//
// OJO, RAREZA HEREDADA (se conserva TAL CUAL, no es un cambio de este paso):
// el primer pilar cae en u = -58 (el borde del muro) y el ultimo en u = +56, o
// sea que la hilera de pilares queda CORRIDA 1 unidad hacia -u respecto del eje
// del muro, y sobra 1.5u de muro liso en el extremo +u. Es asi desde el origen
// y ambos archivos lo reproducen igual.
static constexpr int   wdPierCount() { return (int)(WD_WALL_LEN / WD_BAY) + 1; }  // 7
static constexpr float wdPierU(int k) { return -WD_WALL_LEN * 0.5f + WD_BAY * (float)k; }
// el mismo guard de sector.h: un pilar fuera del muro no se dibuja
static constexpr int   wdPierValid(int k) {
    return (wdPierU(k) < -WD_WALL_LEN * 0.5f || wdPierU(k) > WD_WALL_LEN * 0.5f) ? 0 : 1;
}
static constexpr int   wdBayCount() { return wdPierCount() - 1; }                 // 6
// un vano va entre el pilar b y el b+1; guard de tracery.h (ambos extremos dentro)
static constexpr int   wdBayValid(int b) { return (wdPierValid(b) && wdPierValid(b + 1)) ? 1 : 0; }
static constexpr float wdBayCenterU(int b) { return wdPierU(b) + WD_BAY * 0.5f; }
static constexpr float wdBayHalf() { return WD_BAY_HALF; }                        // 7

// --- los 4 muros ------------------------------------------------------------
// wall: 0,1 = muros +Z/-Z (su tangente es X) ; 2,3 = muros +X/-X (tangente Z)
//       signo = (wall & 1) ? -1 : +1     (mismo convenio que sector.h y tracery.h)
static constexpr int   WD_WALL_N = 4;
static constexpr int   wdWallAlongX(int wall) { return (wall < 2) ? 1 : 0; }
static constexpr float wdWallSign(int wall)   { return (wall & 1) ? -1.0f : 1.0f; }

// (u, nOff) locales del muro -> (x, z) del mundo.
//   u    = coordenada a lo largo del muro
//   nOff = profundidad medida desde el EJE del muro hacia el INTERIOR del sector
//          (nOff = 0 -> eje, en 56 ; nOff = WD_WALL_T*0.5 = 2 -> cara interior, en 54)
// Es exactamente trcXZ() de tracery.h.
static inline void wdWallPoint(int wall, float u, float nOff, float *x, float *z) {
    const float nw = wdWallSign(wall) * (WD_WALL_MID - nOff);
    if (wdWallAlongX(wall)) { *x = u;  *z = nw; }
    else                    { *x = nw; *z = u;  }
}

// Centro del vano `bay` del muro `wall`, sobre la CARA INTERIOR del muro
// (|coord| = WD_SEC_HALF = 54). Devuelve ademas la coordenada local u, que es
// lo que necesita el despiece de tracery.h.
static inline void wdBayCenter(int wall, int bay, float *x, float *z, float *u) {
    const float uu = wdBayCenterU(bay);
    if (u) *u = uu;
    wdWallPoint(wall, uu, WD_WALL_T * 0.5f, x, z);
}
// altura del centro del vano (util para apuntar algo "por la ventana")
static constexpr float wdBayCenterY() { return (WD_WIN_Y0 + WD_WIN_Y1) * 0.5f; }   // 17

// ============================================================================
// 11) INVARIANTES (si alguien toca un numero de arriba, esto lo caza al compilar)
// ============================================================================
// Son justo las relaciones que los comentarios de sector.h prometen y que hasta
// hoy nadie verificaba.
static_assert(WD_MASS_SIDE == 36.0f,          "las masas dejaron de ser de 36x36");
static_assert(WD_CROSS_W   == 18.0f,          "el pasillo en cruz ya no mide 18");
static_assert(WD_PERIM_W   ==  9.0f,          "el pasillo perimetral ya no mide 9");
static_assert(WD_SEC_CEIL  == 2.0f * WD_MASS_H, "el techo ya no es el doble de una masa");
static_assert(WD_COL_BASE_H + WD_COL_SHAFT_H + WD_COL_CAP_H == WD_COL_H,
              "el despiece de la columna no suma su altura total (capitel mal apoyado)");
static_assert(WD_COL_OFF + WD_COL_W * 0.5f < WD_MASS_M0,
              "la columna se metio dentro de la masa");
static_assert(WD_BAY_HALF  ==  7.0f,          "cambio la luz del vano (revisar tracery.h)");
static_assert(wdPierCount() == 7,             "cambio el numero de pilares por muro");
static_assert(wdBayCount()  == 6,             "cambio el numero de vanos por muro");
static_assert(WD_WIN_Y1 < WD_SEC_CEIL,        "el dintel del vano se comio el techo");
static_assert(WD_MASS_M1 < WD_SEC_HALF,       "las masas atraviesan el muro");
static_assert(WD_COL_COUNT == 20,             "ya no son 20 columnas");
