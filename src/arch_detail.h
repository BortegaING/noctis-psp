#pragma once
#include <math.h>
#include "world_def.h"
// ============================================================================
// PROJECT NOCTIS - arch_detail.h
// ARQUITECTURA GOTICA SOBRE LAS CARAS DE LAS MASAS (y SOLIDA)
// ============================================================================
//
// EL PROBLEMA QUE RESUELVE
// ---------------------------------------------------------------------------
// El sector tenia 4 masas de 36x36x30 cuyas caras son PLANCHAS LISAS, y toda la
// "arquitectura" era la arcada de 20 columnas exentas del pasillo. Mirar una
// pared era mirar un rectangulo. Esto le cuelga a esas caras el vocabulario
// gotico que faltaba, y ademas lo hace SOLIDO: lo que vuela al pasillo entra en
// g_city y ni el jugador ni la camara lo atraviesan.
//
//   corte de una cara interior (|x| o |z| = 9)        planta de un contrafuerte
//   y=31.6 ---- cornisa ----  o o o o  <- BALAUSTRADA        pasillo
//   y=30.4      |==|                                           ^  dp=2.5
//         /\    |==| <- CONTRAFUERTE escalonado           +---------+
//   y=25  ||    |--|    (5 tramos, 2 taludes)             |  masa   |  cara=9
//   y=21  || <- HORNACINA con estatua                     +---------+
//   y=17  /\    |==|                                      (vuelo <= 2.5, el
//   y=12  ||    |==| <- ARCADA CIEGA (arquitos ojivales     pasillo en cruz
//   y= 3  ||    |==|    + columnilla + cornisa)             sigue libre)
//   y= 0  ====PORTAL====  ESCALINATA
//
// COMO SE APOYA EN LO QUE YA EXISTE
// ---------------------------------------------------------------------------
// Todas las medidas salen de world_def.h (WD_*). Las caras tratadas son las 8
// que dan al PASILLO EN CRUZ: cada masa q aporta la cara de normal X (en
// |x| = WD_MASS_M0 = 9) y la de normal Z (en |z| = 9).
//
// OJO - CONVIVENCIA CON buildSectorWalls():
// sector.h YA planta 3 contrafuertes LISOS por cara interior, en el eje de la
// cara y a +-10.8 de el (3.0 de ancho, 2.4 de vuelo, 24.6 de alto). NO se tocan
// (la orden es no editar sector.h), asi que este archivo se INTERCALA con ellos:
//   * los contrafuertes nuevos van a o = +-15.8 (esquinas), lejos de [9.3, 12.3];
//   * los paneles de arcada ciega van a o = +-6.5, entre el portal y el
//     contrafuerte viejo;
//   * el contrafuerte viejo del centro de la cara queda DENTRO del vano del
//     portal y se lee como TRUMEAU (el pilar central de un portal gotico).
// Si algun dia se borran esos 3 bloques lisos de sector.h, la composicion de
// aqui queda aun mas limpia: no hay que cambiar nada de este archivo.
//
// SISTEMA DE COORDENADAS LOCAL DE UNA CARA (AdFace)
// ---------------------------------------------------------------------------
//   o  = posicion A LO LARGO de la cara, medida desde su centro, en [-18, +18]
//   dp = VUELO hacia el pasillo, medido desde el plano de la cara (dp>0 sale,
//        dp<0 se mete en la masa)
//   tangente t = n x Y  <- ES EL MISMO CRITERIO DE addSolidBoxT: respetarlo es
//        lo unico que hace falta para que el winding salga bien con cull ON.
//        (verificado contra las caras "frente/atras/izq/der" de addSolidBoxT)
//
// REGLAS DE LA CASA QUE SE RESPETAN
//   * header-only, sin heap, sin rand, sin I/O.
//   * NADA de addLimb: este pase va con GU_CULL_FACE activo y winding de caja.
//   * El motor NO recorta en el plano de camara: ninguna pieza QUE VUELE del
//     muro pasa de 6.0 de largo. Los contrafuertes van en 5 tramos, las jambas
//     del portal en 2, la balaustrada en 6 y la escalinata en 6 peldanos.
//     Los panos PEGADOS al muro (timpano, arquitos ciegos) no se parten: estan
//     al ras de una cara de masa que ya es un quad de 30, no empeoran nada.
//   * Vuelo maximo 2.5 (AD_MAX_DP). El pasillo en cruz es |x|<9 / |z|<9, el
//     jugador tiene radio 1.1 y la camara 0.9: con 2.5 de vuelo queda libre
//     |x| < 5.4, mas ancho que el paso que ya dejan las columnas de la arcada
//     (estan en |x| = 7 con 3.3 de lado -> bloquean hasta |x| = 4.25).
//
// ============================================================================
// PRESUPUESTO DE VERTICES (contado a mano, elemento por elemento)
// ---------------------------------------------------------------------------
//   contrafuerte escalonado  102 x 16 = 1632   (2 por cara interior)
//   portal gotico            111 x  8 =  888   (1 por cara interior)
//   panel de arcada ciega     36 x 15 =  540   (2 por cara, menos el de la
//                                               escalinata)
//   hornacina con estatua     54 x  4 =  216   (solo en las caras de normal X)
//   balaustrada (6 tramos)    72 x  8 =  576   (borde superior de cada cara)
//   escalinata (6 peldanos)  108 x  1 =  108
//                                     -------
//                             TOTAL   = 3960 vertices  (tope pedido: 4000)
//
// PRIORIDAD QUAD > CAJA: casi todo son quads de UNA cara (6 verts) y triangulos
// sueltos (3 verts). La unica primitiva "cerrada" que se usa es addPyramidT
// (12) para la capucha de las estatuas. Un addSolidBoxT habria costado 30 por
// pieza: el contrafuerte entero (5 tramos) habria valido 150 en vez de 102.
//
// ============================================================================
// COLISION: QUE ENTRA Y QUE NO (y por que)
// ---------------------------------------------------------------------------
// archCollisionBoxes() devuelve 35 cajas AABB (g_city: x,z,w,d,h desde y=0):
//   ENTRAN (son gordas, vuelan al pasillo y hay que chocar con ellas)
//     16  contrafuertes  -> huella del tramo bajo (3.8 x 2.5), h = 30.4
//      8  portales       -> el cuerpo que sobresale (7.8 x 2.3), h = 15.0.
//                           Es un portal CIEGO (la masa es maciza): se cierra
//                           entero, asi no queda un bolsillo pegajoso de 2.2.
//      8  balaustradas   -> pretil corrido del borde superior. Como CityBldg
//                           arranca SIEMPRE en y=0, la caja va METIDA en la
//                           huella de la masa (|coord| 9.0..10.45): abajo no
//                           roba NADA de pasillo y arriba sobresale
//                           hasta y=32.8, o sea 2.8 sobre el techo pisable
//                           (y=30) -> pretil real, no se cae ni se atraviesa.
//                           Saltable (JUMP_VEL 0.55 da ~7.5 de altura).
//      3  escalinata     -> rampa aproximada por 3 escalones de 0.6; cada uno
//                           sube menos de 1.0, que es el step-up de
//                           groundHeight() -> se sube caminando.
//   NO ENTRAN (a proposito)
//     - arcada ciega, arquivoltas, cornisas, timpano, columnillas: vuelan 0.15
//       a 0.55. Meterlas solo haria el pasillo PEGAJOSO (blocked() infla cada
//       caja con el radio 1.1 del jugador: un relieve de 0.4 se sentiria como
//       un muro invisible de 1.5).
//     - hornacinas y estatuas: estan RE-HUNDIDAS respecto del plano de la cara
//       o vuelan 0.75 como mucho, y ademas nacen a y=15.5. Nada que chocar.
//     - taludes y remates de los contrafuertes: ya los cubre la caja del tramo
//       bajo, que es la mas ancha y la mas profunda.
//
// ============================================================================
// COMO SE CABLEA (las dos llamadas que tiene que hacer main.cpp)
// ---------------------------------------------------------------------------
//   en buildSolidWorld(), junto a buildSectorWalls (mismo pase, cull ON):
//       buildArchDetail(g_solidWorld, i);
//   en buildSectorCollision() o justo despues, con g_cityCount ya cargado:
//       g_cityCount += archCollisionBoxes(g_city + g_cityCount,
//                                         256 - g_cityCount);
// ============================================================================

// ============================================================================
// 0) MEDIDAS DE ESTE ARCHIVO
// ============================================================================
static constexpr float AD_EPS     = 0.06f;   // despegue del plano de la masa (anti z-fight)
static constexpr float AD_MAX_DP  = 2.50f;   // vuelo maximo hacia el pasillo
static constexpr float AD_TOP     = WD_MASS_H + 0.40f;          // 30.4: bajo la cornisa
static constexpr float AD_CORN    = WD_MASS_TOP_VIS;            // 31.6: alto de la cornisa

// --- portal (centro de la cara) ---
static constexpr float AD_PT_HALF  =  3.90f;  // medio ancho del cuerpo que sobresale
static constexpr float AD_PT_VANO  =  2.40f;  // medio ancho del vano
static constexpr float AD_PT_DP    =  2.20f;  // vuelo de jambas y hastial
static constexpr float AD_PT_PLY   =  1.20f;  // alto del zocalo / umbral
static constexpr float AD_PT_SPR   = 10.50f;  // imposta: arranque del ojival
static constexpr float AD_PT_APEX  = 14.50f;  // clave
static constexpr float AD_PT_GBL   = 17.60f;  // cumbre del hastial

// --- arcada ciega (dos panos por cara, a los lados del portal) ---
static constexpr float AD_AR_O     =  6.50f;  // centro del pano (+-)
static constexpr float AD_AR_BAY   =  1.30f;  // separacion entre los ejes de los 2 arquitos
static constexpr float AD_AR_HALF  =  1.05f;  // medio ancho de un arquito
static constexpr float AD_AR_PAN   =  2.55f;  // medio ancho del pano (= el de su cornisa)
static constexpr float AD_AR_Y0    =  3.40f;  // basa de la arcada
static constexpr float AD_AR_SPR   =  9.40f;  // imposta
static constexpr float AD_AR_APEX  = 11.40f;  // clave
static constexpr float AD_AR_CORN  = 12.50f;  // cara alta de la cornisa que la remata

// --- contrafuertes (esquinas de la cara) ---
static constexpr float AD_BT_O     = 15.80f;  // posicion (+-)
static constexpr float AD_BT_HALF  =  1.90f;  // medio ancho
// despiece vertical: 5 tramos (ninguno pasa de 6.0) y 2 taludes
static constexpr float AD_BT_Y[7]  = { 0.0f, 6.0f, 12.0f, 13.0f, 18.6f, 24.2f, 25.2f };
static constexpr float AD_BT_DP[3] = { 2.50f, 2.00f, 1.40f };   // vuelo por escalon

// --- hornacina con estatua (solo caras de normal X) ---
static constexpr float AD_HN_O     =  6.50f;
static constexpr float AD_HN_HALF  =  1.30f;
static constexpr float AD_HN_Y0    = 15.50f;  // basa de la mensula
static constexpr float AD_HN_PLI   = 16.10f;  // cara de la peana (ahi apoya la figura)
static constexpr float AD_HN_SPR   = 20.00f;  // imposta del nicho
static constexpr float AD_HN_APEX  = 21.60f;  // clave del nicho

// --- balaustrada del borde superior ---
static constexpr float AD_BL_FRONT =  0.15f;  // vuelo del pretil
static constexpr float AD_BL_BACK  = -1.45f;  // (hacia dentro de la masa)
static constexpr float AD_BL_TOP   = 32.80f;  // pasamanos
static constexpr int   AD_BL_SEG   =  6;      // 36 / 6 = 6.0 por tramo

// --- escalinata ---
static constexpr float AD_ST_O0    = -9.20f;  // arranque (o)
static constexpr float AD_ST_O1    = -3.95f;  // llegada, al pie del portal
static constexpr float AD_ST_DP    =  1.60f;  // fondo de la huella
static constexpr float AD_ST_RISE  =  0.30f;  // contrahuella
static constexpr int   AD_ST_N     =  6;      // peldanos

// reparto por cara: fi = q*2 + a  (a = 0 cara de normal X, a = 1 de normal Z)
static constexpr int   AD_FACE_N     = 8;
static constexpr int   AD_STAIR_FACE = 0;     // la escalinata va en una sola cara

// --- INVARIANTES: si alguien engorda un adorno, esto lo caza al compilar en vez
//     de que aparezca un muro invisible en mitad del pasillo ---
static_assert(AD_BT_DP[0]        <= AD_MAX_DP, "el contrafuerte invade el pasillo");
static_assert(AD_PT_DP + 0.10f   <= AD_MAX_DP, "el portal invade el pasillo");
static_assert(AD_ST_DP           <= AD_MAX_DP, "la escalinata invade el pasillo");
static_assert(AD_MAX_DP + 1.1f + 0.9f < WD_MASS_M0,
              "con este vuelo ya no cabe el jugador (r=1.1) + la camara (r=0.9)");
static_assert(AD_BT_O + AD_BT_HALF <= WD_MASS_HALF,
              "el contrafuerte se sale del ancho de la cara");
static_assert(AD_BT_O - AD_BT_HALF > 12.3f,
              "el contrafuerte nuevo pisa el contrafuerte liso de sector.h (+-10.8, 3.0 ancho)");
static_assert(AD_AR_O + AD_AR_PAN < 9.3f,
              "la arcada ciega choca con el contrafuerte liso de sector.h");
static_assert(AD_PT_HALF < AD_AR_O - AD_AR_PAN,
              "el portal pisa el pano de arcada ciega");
static_assert(AD_ST_O1 <= -AD_PT_HALF,
              "la escalinata se mete dentro del portal");
static_assert(AD_BT_Y[1] - AD_BT_Y[0] <= 6.0f && AD_BT_Y[2] - AD_BT_Y[1] <= 6.0f &&
              AD_BT_Y[4] - AD_BT_Y[3] <= 6.0f && AD_BT_Y[5] - AD_BT_Y[4] <= 6.0f &&
              AD_TOP    - AD_BT_Y[6] <= 6.0f,
              "un tramo de contrafuerte pasa de 6: cruzara el plano de camara y se borrara");
static_assert(WD_MASS_SIDE / (float)AD_BL_SEG <= 6.0f,
              "los tramos de balaustrada pasan de 6");
static_assert(AD_PT_SPR * 0.5f <= 6.0f, "los tramos de jamba del portal pasan de 6");

// ============================================================================
// 1) EL MARCO DE UNA CARA  (o, dp) -> (x, z)
// ============================================================================
struct AdFace {
    float fx, fz;     // centro de la cara, en planta
    float nx, nz;     // normal hacia el pasillo (unitaria, alineada a eje)
    float tx, tz;     // tangente = n x Y  (criterio de addSolidBoxT)
};

static inline AdFace adMakeFace(float fx, float fz, float nx, float nz) {
    AdFace f; f.fx = fx; f.fz = fz; f.nx = nx; f.nz = nz;
    f.tx = -nz; f.tz = nx;                   // n x (0,1,0)
    return f;
}

// las 8 caras que dan al pasillo en cruz
static inline AdFace adFace(int fi) {
    const int q = fi >> 1, a = fi & 1;
    const float sx = wdQuadSX(q), sz = wdQuadSZ(q);
    if (a == 0) return adMakeFace(WD_MASS_M0 * sx, WD_MASS_CTR * sz, -sx, 0.0f);
    return              adMakeFace(WD_MASS_CTR * sx, WD_MASS_M0 * sz, 0.0f, -sz);
}

static inline void adPt(const AdFace &f, float o, float dp, float *x, float *z) {
    *x = f.fx + f.tx * o + f.nx * dp;
    *z = f.fz + f.tz * o + f.nz * dp;
}

// ============================================================================
// 2) PRIMITIVAS DE UNA CARA (todas con el winding de addSolidBoxT)
// ============================================================================

// --- quad VERTICAL que mira al pasillo (normal = n). 6 verts. o0 < o1 ---
static void adQuadV(TexVertex *b, int &i, const AdFace &f,
                    float o0, float o1, float y0, float y1, float dp, unsigned int col) {
    float ax, az, bx, bz;
    adPt(f, o0, dp, &ax, &az); adPt(f, o1, dp, &bx, &bz);
    const float uw = (o1 - o0) / TILE, uh = (y1 - y0) / TILE;
    addQuadT(b, i, ax, y0, az, bx, y0, bz, bx, y1, bz, ax, y1, az, 0, uh, uw, 0, col);
}

// --- quad VERTICAL en el plano de la cara, con los 4 puntos a mano (arquivoltas,
//     hastiales, tunicas). Orden: A abajo-"izq", B abajo-"der", C arriba-"der",
//     D arriba-"izq" (mismo ciclo que adQuadV). 6 verts ---
static void adQuadF(TexVertex *b, int &i, const AdFace &f,
                    float oA, float yA, float oB, float yB,
                    float oC, float yC, float oD, float yD, float dp, unsigned int col) {
    float ax, az, bx, bz, cx, cz, dx, dz;
    adPt(f, oA, dp, &ax, &az); adPt(f, oB, dp, &bx, &bz);
    adPt(f, oC, dp, &cx, &cz); adPt(f, oD, dp, &dx, &dz);
    const float uw = fabsf(oB - oA) / TILE, uh = fabsf(yD - yA) / TILE;
    addQuadT(b, i, ax, yA, az, bx, yB, bz, cx, yC, cz, dx, yD, dz, 0, uh, uw, 0, col);
}

// --- TRIANGULO en el plano de la cara: la cabeza OJIVAL, por 3 verts en vez de
//     6. Es el ahorro que permite tener 30 arquitos ciegos y 8 timpanos ---
static void adTriV(TexVertex *b, int &i, const AdFace &f,
                   float o0, float o1, float yBase, float oApex, float yApex,
                   float dp, unsigned int col) {
    float ax, az, bx, bz, cx, cz;
    adPt(f, o0, dp, &ax, &az); adPt(f, o1, dp, &bx, &bz); adPt(f, oApex, dp, &cx, &cz);
    const float uw = (o1 - o0) / TILE, uh = (yApex - yBase) / TILE;
    b[i++] = { 0, uh, col, ax, yBase, az };
    b[i++] = { uw, uh, col, bx, yBase, bz };
    b[i++] = { uw * 0.5f, 0, col, cx, yApex, cz };
}

// --- COSTADO de una pieza que vuela: normal = +t (side>0) o -t (side<0). 6 verts ---
static void adQuadS(TexVertex *b, int &i, const AdFace &f, float o,
                    float dp0, float dp1, float y0, float y1, int side, unsigned int col) {
    float ax, az, bx, bz;
    if (side > 0) { adPt(f, o, dp1, &ax, &az); adPt(f, o, dp0, &bx, &bz); }
    else          { adPt(f, o, dp0, &ax, &az); adPt(f, o, dp1, &bx, &bz); }
    const float uw = (dp1 - dp0) / TILE, uh = (y1 - y0) / TILE;
    addQuadT(b, i, ax, y0, az, bx, y0, bz, bx, y1, bz, ax, y1, az, 0, uh, uw, 0, col);
}

// --- quad HORIZONTAL que mira ARRIBA (peana, huella, talud, pasamanos).
//     La y puede ser distinta en dp0 y dp1 -> sirve de TALUD inclinado.
//     Requiere o0 < o1 y dp0 < dp1. 6 verts ---
static void adQuadH(TexVertex *b, int &i, const AdFace &f, float o0, float o1,
                    float dp0, float dp1, float yAtDp0, float yAtDp1, unsigned int col) {
    float ax, az, bx, bz, cx, cz, dx, dz;
    adPt(f, o0, dp1, &ax, &az); adPt(f, o1, dp1, &bx, &bz);
    adPt(f, o1, dp0, &cx, &cz); adPt(f, o0, dp0, &dx, &dz);
    const float uw = (o1 - o0) / TILE, ud = (dp1 - dp0) / TILE;
    addQuadT(b, i, ax, yAtDp1, az, bx, yAtDp1, bz, cx, yAtDp0, cz, dx, yAtDp0, dz, 0, 0, uw, ud, col);
}

// --- quad HORIZONTAL que mira ABAJO (sofito de cornisa: es lo que se ve desde
//     el pasillo, porque la cornisa esta por encima de los ojos). 6 verts ---
static void adQuadHD(TexVertex *b, int &i, const AdFace &f, float o0, float o1,
                     float dp0, float dp1, float y, unsigned int col) {
    float ax, az, bx, bz, cx, cz, dx, dz;
    adPt(f, o0, dp1, &ax, &az); adPt(f, o0, dp0, &bx, &bz);
    adPt(f, o1, dp0, &cx, &cz); adPt(f, o1, dp1, &dx, &dz);
    const float uw = (o1 - o0) / TILE, ud = (dp1 - dp0) / TILE;
    addQuadT(b, i, ax, y, az, bx, y, bz, cx, y, cz, dx, y, dz, 0, 0, ud, uw, col);
}

// --- TRAMO que vuela: frente + los dos costados (18 verts). Es el ladrillo de
//     los contrafuertes y de las jambas. NO lleva tapa ni trasera: la tapa la
//     pone el talud del escalon siguiente y la trasera es la propia masa ---
static void adStack(TexVertex *b, int &i, const AdFace &f, float oC, float half,
                    float y0, float y1, float dp, unsigned int col) {
    adQuadV(b, i, f, oC - half, oC + half, y0, y1, dp, col);
    adQuadS(b, i, f, oC + half, 0.0f, dp, y0, y1, +1, brighten(col, 0.74f));
    adQuadS(b, i, f, oC - half, 0.0f, dp, y0, y1, -1, brighten(col, 0.74f));
}

// ============================================================================
// 3) LOS ELEMENTOS
// ============================================================================

// --- 3.1 ARCADA CIEGA: 2 arquitos ojivales rehundidos + columnilla + cornisa.
//     36 verts. El relieve real lo dan la columnilla (vuela 0.42) y la cornisa
//     con su sofito (vuela 0.50): el resto es pano oscuro al ras, que es lo que
//     convierte la plancha en un muro con ritmo sin gastar caja ---
static void adBlindArcade(TexVertex *b, int &i, const AdFace &f, float oC,
                          unsigned int stone, unsigned int dark) {
    const unsigned int hueco = brighten(dark, 0.62f);        // el fondo del arquito
    for (int s = -1; s <= 1; s += 2) {                       // 2 arquitos
        const float o = oC + (float)s * AD_AR_BAY;
        adQuadV(b, i, f, o - AD_AR_HALF, o + AD_AR_HALF, AD_AR_Y0, AD_AR_SPR, AD_EPS, hueco);  // 6
        adTriV (b, i, f, o - AD_AR_HALF, o + AD_AR_HALF, AD_AR_SPR, o, AD_AR_APEX, AD_EPS,
                brighten(hueco, 0.88f));                                                        // 3
    }
    // columnilla central (la que separa los dos arquitos)
    adQuadV(b, i, f, oC - 0.26f, oC + 0.26f, AD_AR_Y0, AD_AR_SPR + 0.5f, 0.42f, stone);         // 6
    // cornisa que remata la arcada: frente + SOFITO (se mira desde abajo)
    const float o0 = oC - AD_AR_PAN, o1 = oC + AD_AR_PAN;
    adQuadV (b, i, f, o0, o1, AD_AR_APEX + 0.20f, AD_AR_CORN, 0.50f, brighten(stone, 1.06f));   // 6
    adQuadHD(b, i, f, o0, o1, 0.0f, 0.50f, AD_AR_APEX + 0.20f, brighten(dark, 0.80f));          // 6
}

// --- 3.2 CONTRAFUERTE ESCALONADO: 5 tramos (ninguno pasa de 6.0 de alto) que
//     van perdiendo vuelo hacia arriba, con 2 TALUDES inclinados en los saltos.
//     102 verts. Baja desde la cornisa hasta el suelo ---
static void adButtress(TexVertex *b, int &i, const AdFace &f, float oC,
                       unsigned int stone, unsigned int dark) {
    const unsigned int c0 = dark, c1 = brighten(stone, 0.92f), c2 = stone;
    // tramos bajos (vuelo AD_BT_DP[0])
    adStack(b, i, f, oC, AD_BT_HALF, AD_BT_Y[0], AD_BT_Y[1], AD_BT_DP[0], c0);          // 18
    adStack(b, i, f, oC, AD_BT_HALF, AD_BT_Y[1], AD_BT_Y[2], AD_BT_DP[0], c1);          // 18
    // TALUD 1: del borde volado del tramo bajo hasta el muro, subiendo
    adQuadH(b, i, f, oC - AD_BT_HALF, oC + AD_BT_HALF, AD_BT_DP[1], AD_BT_DP[0],
            AD_BT_Y[3], AD_BT_Y[2], brighten(stone, 1.12f));                            //  6
    // tramos medios
    adStack(b, i, f, oC, AD_BT_HALF, AD_BT_Y[3], AD_BT_Y[4], AD_BT_DP[1], c2);          // 18
    adStack(b, i, f, oC, AD_BT_HALF, AD_BT_Y[4], AD_BT_Y[5], AD_BT_DP[1], c1);          // 18
    // TALUD 2
    adQuadH(b, i, f, oC - AD_BT_HALF, oC + AD_BT_HALF, AD_BT_DP[2], AD_BT_DP[1],
            AD_BT_Y[6], AD_BT_Y[5], brighten(stone, 1.12f));                            //  6
    // tramo alto, muere bajo la cornisa de la masa
    adStack(b, i, f, oC, AD_BT_HALF, AD_BT_Y[6], AD_TOP, AD_BT_DP[2], c2);              // 18
}

// --- 3.3 PORTAL GOTICO: umbral, dos jambas (partidas en 2 por el recorte de
//     camara), puerta y timpano al ras, arquivolta y hastial en dos mitades.
//     111 verts. Las ARQUIVOLTAS quedan ESCALONADAS en profundidad:
//       timpano dp=0.15  ->  arquivolta dp=1.40  ->  jamba/hastial dp=2.20 ---
static void adPortal(TexVertex *b, int &i, const AdFace &f,
                     unsigned int stone, unsigned int dark) {
    const unsigned int hueco = brighten(dark, 0.55f);
    // UMBRAL: solo bajo el vano (4.8 de ancho). Antes iba de jamba a jamba, 7.8,
    // y eso rompia la regla de los 6 de largo por pieza que vuela.
    adQuadV(b, i, f, -AD_PT_VANO, AD_PT_VANO, 0.0f, AD_PT_PLY, 1.20f, dark);              // 6
    adQuadH(b, i, f, -AD_PT_VANO, AD_PT_VANO, 0.0f, 1.20f,
            AD_PT_PLY, AD_PT_PLY, brighten(stone, 1.10f));                                // 6
    // JAMBAS: bajan hasta el suelo, 2 lados x 2 tramos de 5.25 (nunca un pilar
    // de 10.5 de una pieza: cruzaria el plano de camara y desapareceria)
    const float jc = (AD_PT_VANO + AD_PT_HALF) * 0.5f, jh = (AD_PT_HALF - AD_PT_VANO) * 0.5f;
    const float jm = AD_PT_SPR * 0.5f;
    for (int s = -1; s <= 1; s += 2) {
        adStack(b, i, f, (float)s * jc, jh, 0.0f, jm, AD_PT_DP, stone);                   // 18
        adStack(b, i, f, (float)s * jc, jh, jm, AD_PT_SPR, AD_PT_DP, brighten(stone, 0.94f)); // 18
    }
    // puerta ciega + timpano, al ras (la masa es maciza: no se entra)
    adQuadV(b, i, f, -AD_PT_VANO, AD_PT_VANO, AD_PT_PLY, AD_PT_SPR, 0.15f, hueco);        // 6
    adTriV (b, i, f, -AD_PT_VANO, AD_PT_VANO, AD_PT_SPR, 0.0f, AD_PT_APEX, 0.15f,
            brighten(hueco, 1.25f));                                                      // 3
    // ARQUIVOLTA: dos rampantes con canto, a media profundidad.
    // OJO: los 4 puntos van SIEMPRE de o menor a o mayor por abajo y al reves
    // por arriba (el ciclo de adQuadV). Escribir el lado -o como un simple
    // cambio de signo del lado +o da un quad CRUZADO y con el winding al reves:
    // con cull ON desaparece. Por eso los dos lados van escritos aparte.
    const float ai = AD_PT_VANO - 0.55f;                  // canto interior
    const float at = 0.33f, ay = AD_PT_APEX + 0.70f;      // canto exterior en la clave
    adQuadF(b, i, f,  ai, AD_PT_SPR,  AD_PT_VANO, AD_PT_SPR,
            at, ay, 0.0f, AD_PT_APEX, 1.40f, brighten(stone, 1.14f));                     // 6
    adQuadF(b, i, f, -AD_PT_VANO, AD_PT_SPR, -ai, AD_PT_SPR,
            0.0f, AD_PT_APEX, -at, ay, 1.40f, brighten(stone, 1.14f));                    // 6
    // HASTIAL sobre el arco (mismo plano que las jambas). Va en DOS mitades de
    // 3.9: entero seria una pieza volada de 7.8 de base.
    adTriV(b, i, f, -AD_PT_HALF, 0.0f, AD_PT_APEX - 0.30f, 0.0f, AD_PT_GBL,
           AD_PT_DP, brighten(stone, 1.18f));                                             // 3
    adTriV(b, i, f, 0.0f, AD_PT_HALF, AD_PT_APEX - 0.30f, 0.0f, AD_PT_GBL,
           AD_PT_DP, brighten(stone, 1.10f));                                             // 3
}

// --- 3.4 HORNACINA CON FIGURA: nicho rehundido, mensula, estatua encapuchada y
//     guardapolvo. 54 verts. Va SOLO en las caras de normal X -> 4 en todo el
//     sector, que es el "ritmo" pedido (no en todas) ---
static void adNiche(TexVertex *b, int &i, const AdFace &f, float oC,
                    unsigned int stone, unsigned int dark) {
    const unsigned int hueco = brighten(dark, 0.50f);
    // el nicho: pano oscuro + cabeza ojival, al ras
    adQuadV(b, i, f, oC - AD_HN_HALF, oC + AD_HN_HALF, AD_HN_PLI, AD_HN_SPR, AD_EPS, hueco);  // 6
    adTriV (b, i, f, oC - AD_HN_HALF, oC + AD_HN_HALF, AD_HN_SPR, oC, AD_HN_APEX, AD_EPS,
            brighten(hueco, 0.85f));                                                          // 3
    // mensula/peana donde apoya la figura
    adQuadV(b, i, f, oC - 1.50f, oC + 1.50f, AD_HN_Y0, AD_HN_PLI, 0.75f, dark);               // 6
    adQuadH(b, i, f, oC - 1.50f, oC + 1.50f, 0.0f, 0.75f, AD_HN_PLI, AD_HN_PLI,
            brighten(stone, 1.10f));                                                          // 6
    // ESTATUA: silueta ENCAPUCHADA. Tunica troncoconica (frente trapecial + dos
    // costados) y capucha en piramide; el gesto se lee de lejos, que es lo unico
    // que se puede pedir en PSP.
    const unsigned int robe = brighten(dark, 1.16f);
    const float hb = 0.65f, ht = 0.40f, yb = AD_HN_PLI, ys = yb + 2.90f;
    adQuadF(b, i, f, oC - hb, yb, oC + hb, yb, oC + ht, ys, oC - ht, ys, 0.58f, robe);        // 6
    const float hs = (hb + ht) * 0.5f;   // los costados no pueden afinarse (dp fijo):
                                         // van al ancho medio para repartir el error
    adQuadS(b, i, f, oC + hs, 0.12f, 0.58f, yb, ys, +1, brighten(robe, 0.70f));              // 6
    adQuadS(b, i, f, oC - hs, 0.12f, 0.58f, yb, ys, -1, brighten(robe, 0.70f));              // 6
    float sx2, sz2; adPt(f, oC, 0.35f, &sx2, &sz2);
    addPyramidT(b, i, sx2, ys, sz2, 0.86f, 0.86f, 0.95f, brighten(robe, 0.86f));              // 12
    // guardapolvo (el gablete que corona el nicho)
    adTriV(b, i, f, oC - 1.70f, oC + 1.70f, AD_HN_APEX - 0.30f, oC, AD_HN_APEX + 1.40f,
           0.80f, brighten(stone, 1.16f));                                                    // 3
}

// --- 3.5 BALAUSTRADA del borde superior: 6 tramos de 6.0 (el jugador aterriza
//     aqui al escalar, o sea que la camara pasa MUY cerca -> tramos cortos).
//     72 verts. Cada tramo: pano calado al frente + pasamanos en chaflan hacia
//     el techo de la masa. El chaflan cierra la pieza por dentro con UN solo
//     quad: desde el techo no se ve el hueco y no hay que pagar la trasera ---
static void adBalustrade(TexVertex *b, int &i, const AdFace &f,
                         unsigned int stone, unsigned int dark) {
    const float seg = (WD_MASS_SIDE) / (float)AD_BL_SEG;     // 6.0
    for (int k = 0; k < AD_BL_SEG; ++k) {
        const float o0 = -WD_MASS_HALF + seg * (float)k, o1 = o0 + seg;
        // pilaretes + pasamanos leidos como un pano calado (la textura industrial
        // le pone la junta); el color alterna para que se vean los tramos
        adQuadV(b, i, f, o0, o1, AD_CORN, AD_BL_TOP, AD_BL_FRONT,
                (k & 1) ? dark : brighten(dark, 1.12f));                                  // 6
        adQuadH(b, i, f, o0, o1, AD_BL_BACK, AD_BL_FRONT, AD_CORN, AD_BL_TOP,
                brighten(stone, 1.14f));                                                  // 6
    }
}

// --- 3.6 ESCALINATA: 6 peldanos de piedra pegados a la cara, subiendo hacia el
//     pie del portal. 108 verts. Cada peldano: costado (la silueta escalonada
//     que se ve desde el pasillo), huella y contrahuella ---
static void adStair(TexVertex *b, int &i, const AdFace &f,
                    unsigned int stone, unsigned int dark) {
    const float st = (AD_ST_O1 - AD_ST_O0) / (float)AD_ST_N;
    for (int k = 0; k < AD_ST_N; ++k) {
        const float o0 = AD_ST_O0 + st * (float)k, o1 = o0 + st;
        const float yb = AD_ST_RISE * (float)k, yt = yb + AD_ST_RISE;
        adQuadV(b, i, f, o0, o1, 0.0f, yt, AD_ST_DP, (k & 1) ? stone : brighten(stone, 0.90f)); // 6
        adQuadH(b, i, f, o0, o1, 0.0f, AD_ST_DP, yt, yt, brighten(stone, 1.12f));               // 6
        adQuadS(b, i, f, o0, 0.0f, AD_ST_DP, yb, yt, -1, dark);                                 // 6
    }
}

// ============================================================================
// 4) EL PASE COMPLETO
// ============================================================================
// Se dibuja en el MISMO rango que buildSectorWalls (textura industrial, cull ON).
// Coste medido: 3960 vertices.
static void buildArchDetail(TexVertex *buf, int &i) {
    const unsigned int stone = WD_COL_STONE;
    const unsigned int dark  = WD_COL_DARK;

    for (int fi = 0; fi < AD_FACE_N; ++fi) {
        const AdFace f = adFace(fi);

        // --- portal en el centro de la cara (8 x 111 = 888) ---
        adPortal(buf, i, f, stone, dark);

        // --- contrafuertes en las dos esquinas (16 x 102 = 1632) ---
        adButtress(buf, i, f, -AD_BT_O, stone, dark);
        adButtress(buf, i, f,  AD_BT_O, stone, dark);

        // --- arcada ciega a los dos lados del portal (15 x 36 = 540).
        //     En la cara de la escalinata se omite el pano de ese lado: lo
        //     taparia el perron y seria relleno pagado que no se ve.
        adBlindArcade(buf, i, f,  AD_AR_O, stone, dark);
        if (fi != AD_STAIR_FACE) adBlindArcade(buf, i, f, -AD_AR_O, stone, dark);

        // --- hornacina con estatua solo en las caras de normal X (4 x 54 = 216) ---
        if ((fi & 1) == 0) adNiche(buf, i, f, AD_HN_O, stone, dark);

        // --- balaustrada del borde superior (8 x 72 = 576) ---
        adBalustrade(buf, i, f, stone, dark);

        // --- escalinata (1 x 108 = 108) ---
        if (fi == AD_STAIR_FACE) adStair(buf, i, f, stone, dark);
    }
}

// ============================================================================
// 5) COLISION
// ============================================================================
// Devuelve cuantas cajas escribio. Ver la nota de cabecera para el criterio de
// que entra y que no. Total: 16 + 8 + 8 + 3 = 35 cajas.
static constexpr int ARCH_COLLISION_BOXES = 35;

// caja AABB a partir de un rectangulo (o0..o1, dp0..dp1) de una cara
static inline CityBldg adBox(const AdFace &f, float o0, float o1,
                             float dp0, float dp1, float h, unsigned int col) {
    float ax, az, bx, bz;
    adPt(f, o0, dp0, &ax, &az);
    adPt(f, o1, dp1, &bx, &bz);
    const float x0 = (ax < bx) ? ax : bx, x1 = (ax < bx) ? bx : ax;
    const float z0 = (az < bz) ? az : bz, z1 = (az < bz) ? bz : az;
    CityBldg c;
    c.x = (x0 + x1) * 0.5f; c.z = (z0 + z1) * 0.5f;
    c.w = x1 - x0;          c.d = z1 - z0;
    c.h = h;                c.color = col;
    return c;
}

static int archCollisionBoxes(CityBldg *out, int maxOut) {
    int n = 0;
    const unsigned int col = WD_COL_DARK;
    for (int fi = 0; fi < AD_FACE_N; ++fi) {
        const AdFace f = adFace(fi);

        // 1) CONTRAFUERTES: huella del tramo bajo (el mas ancho y el que mas
        //    vuela), hasta arriba del todo. 2 por cara = 16.
        for (int s = -1; s <= 1; s += 2) {
            if (n >= maxOut) return n;
            const float o = (float)s * AD_BT_O;
            out[n++] = adBox(f, o - AD_BT_HALF, o + AD_BT_HALF, 0.0f, AD_BT_DP[0], AD_TOP, col);
        }

        // 2) PORTAL: el cuerpo entero que sobresale. Se cierra macizo (es un
        //    portal ciego) para no dejar un bolsillo de 2.2 donde el jugador se
        //    quede enganchado. 1 por cara = 8.
        if (n >= maxOut) return n;
        out[n++] = adBox(f, -AD_PT_HALF, AD_PT_HALF, 0.0f, AD_PT_DP + 0.10f, 15.0f, col);

        // 3) BALAUSTRADA: pretil corrido. Va METIDO en la huella de la masa
        //    (dp de AD_BL_BACK a 0) porque CityBldg arranca SIEMPRE en y=0:
        //    asi abajo la caja no asoma ni un milimetro al pasillo (coincide
        //    con la cara de la masa, que ya bloquea ahi) y arriba sobresale
        //    hasta 32.8, o sea 2.8 sobre el techo pisable. OJO: llegar hasta
        //    AD_BL_FRONT (0.15) parece inofensivo y NO lo es: estrecharia el
        //    pasillo entero 0.15 por lado en TODA su altura y dejaria la
        //    escalinata sin sitio donde pararse. El pretil dibujado vuela esos
        //    0.15 de mas: se para 0.15 antes de tocarlo y no se nota.
        //    1 por cara = 8.
        if (n >= maxOut) return n;
        out[n++] = adBox(f, -WD_MASS_HALF, WD_MASS_HALF,
                         AD_BL_BACK, 0.0f, AD_BL_TOP, col);

        // 4) ESCALINATA: 3 escalones de colision (0.6 cada uno < step-up 1.0).
        if (fi == AD_STAIR_FACE) {
            const float w = (AD_ST_O1 - AD_ST_O0) / 3.0f;
            for (int k = 0; k < 3; ++k) {
                if (n >= maxOut) return n;
                const float o0 = AD_ST_O0 + w * (float)k;
                out[n++] = adBox(f, o0, o0 + w, 0.0f, AD_ST_DP, 0.6f * (float)(k + 1), col);
            }
        }
    }
    return n;
}

// ============================================================================
// NOTA FINAL (resumen de una ojeada)
// ---------------------------------------------------------------------------
// QUE Y DONDE: sobre las 8 caras de masa que dan al pasillo en cruz -> PORTAL
//   gotico al centro (umbral, jambas, puerta, timpano, arquivolta y hastial),
//   2 CONTRAFUERTES escalonados en las esquinas (5 tramos, 2 taludes, del suelo
//   a la cornisa), 2 panos de ARCADA CIEGA ojival con columnilla y cornisa,
//   BALAUSTRADA corrida en el borde superior; ademas 4 HORNACINAS con estatua
//   encapuchada (solo caras de normal X) y 1 ESCALINATA de 6 peldanos.
// VERTICES: 3960 exactos (1632 contrafuertes + 888 portales + 540 arcada +
//   216 hornacinas + 576 balaustradas + 108 escalinata). Tope pedido: 4000.
// COLISION: 35 cajas (16 contrafuertes + 8 portales + 8 pretiles + 3 escalones).
//   FUERA a proposito: arcada ciega, arquivoltas, cornisas, timpanos,
//   columnillas, hornacinas y estatuas -> vuelan 0.15-0.80 y solo harian el
//   pasillo pegajoso (blocked() infla cada caja con el radio 1.1 del jugador).
// CABLEADO: buildArchDetail(g_solidWorld, i); junto a buildSectorWalls, y
//   g_cityCount += archCollisionBoxes(g_city + g_cityCount, 256 - g_cityCount);
// ============================================================================
