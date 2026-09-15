#pragma once
#include <math.h>
#include "sector.h"
// ==================== TRACERIA: el marco gotico de los VENTANALES ====================
// Los vanos que abrio buildSectorWalls en los 4 muros exteriores son RECTANGULOS PELADOS
// (14 de ancho x 20 de alto). Un ventanal gotico no es un rectangulo: esto le cuelga a cada
// vano su ARCO OJIVAL, su PARTELUZ, su TRACERIA de cabeza, su ALFEIZAR y sus JAMBAS, para que
// el hueco se lea como CATEDRAL y no como ventana de galpon.
//
//        alzado de UN vano (14 x 20; u = a lo largo del muro)
//                   __/\__           <- CLAVE (cuna, mas ancha arriba)
//                _--  ||  --_        <- ARCO OJIVAL (2 cuerdas por lado = ogiva)
//              _-  .  ||  .  -_      <- ENJUTAS (2 triangulitos)
//             |     ( () )    |      <- OCULO (rombo) en el timpano
//             |    /  ||  \   |
//             |   /   ||   \  |      <- LANCETAS (los 2 arquitos)
//             |  |    ||    | |
//             |  |    ||    | |      <- PARTELUZ (montante central)
//             |__|____||____|_|
//             |===============|      <- ALFEIZAR (moldura en talud, SALE al pasillo)
//              ^             ^
//              JAMBAS (un baqueton fino contra cada pilar)
//
// COMO CALZA CON sector.h: las posiciones se RECALCULAN con la MISMA formula de
// buildSectorWalls (wt=4, wc=SEC_HALF+wt/2, wl=2*(SEC_HALF+wt), opY0=7, opY1=27, pierW=5,
// bay=19, nPier=(int)(wl/bay)+1). Con los numeros actuales salen 7 pilares por muro y por
// tanto 6 VANOS por muro (centros u = -48.5, -29.5, -10.5, 8.5, 27.5, 46.5; luz 14) => 24.
//
// TRES TRAMPAS DEL MOTOR, y como se esquivan:
//   1) PRESUPUESTO. Una caja son 30 verts: con addSolidBoxT no entran ni 4 piezas por vano.
//      El jugador NUNCA sale del sector (la colision sigue siendo el muro entero), asi que de
//      cada pieza SOLO se ve la cara interior -> aqui todo son QUADS DE UNA CARA (6 verts) y
//      un par de TRIANGULOS (3 verts). Rinde 5x mas piezas por el mismo costo.
//   2) WINDING. addSolidBoxT emite cada cara con la normal visible = -((b-a) x (c-a)).
//      trcQuad/trcTri replican ESA regla: corrigen solos el sentido (area con signo en el
//      plano local del muro) y ademas por muro (w.flip), asi que la cara SIEMPRE mira al
//      interior. Se puede dibujar en el rango de buildSectorWalls (cull ON). NO es addLimb.
//   3) SIN RECORTE EN EL PLANO DE CAMARA: un triangulo con un vertice detras del jugador se
//      descarta entero. Por eso NINGUNA pieza pasa de ~6 unidades: el alfeizar va en 3 tramos
//      de 4.67, las jambas en 2 de 5.15, el parteluz en 2 de 5.95 y cada brazo del arco en 2
//      cuerdas de 4.6 y 5.0.
//
// DEJA PASAR LA VISTA: todo elemento tiene 0.55-0.9 de grosor y entre todos tapan ~26% del
// area del vano, casi todo pegado al borde y a la cabeza; el centro-bajo (por donde se ve el
// telon de agujas en niebla) queda partido nada mas que por el parteluz.
//
// COSTO: 19 quads + 2 triangulos = 120 verts por vano x 24 vanos = 2880 verts.
// DONDE SE LLAMA: en buildSolidWorld, junto a buildSectorWalls/buildVault y DENTRO del rango
// [g_wallStart, g_wallEnd) (textura industrial, MODULATE, cull ON).

// --- geometria del vano: MISMA formula que buildSectorWalls (sector.h) ---
static const float TRC_WT    = 4.0f;                          // espesor del muro
static const float TRC_WC    = SEC_HALF + TRC_WT * 0.5f;      // 56: eje del muro
static const float TRC_WL    = 2.0f * (SEC_HALF + TRC_WT);    // 116: largo del muro
static const float TRC_OPY0  = 7.0f;                          // opY0: remate del antefecho
static const float TRC_OPY1  = 27.0f;                         // opY1: arranque del muro alto
static const float TRC_PIERW = 5.0f;                          // ancho del pilar
static const float TRC_BAY   = 19.0f;                         // paso entre ventanales
static const float TRC_HW    = (TRC_BAY - TRC_PIERW) * 0.5f;  // 7: media luz del vano

// --- alturas del despiece (locales al vano) ---
static const float TRC_SILL  =  9.0f;   // remate del alfeizar
static const float TRC_LSPR  = 15.4f;   // arranque de las lancetas
static const float TRC_SPR   = 19.3f;   // arranque del arco ojival = remate de las jambas
static const float TRC_LAPX  = 19.9f;   // vertice de las lancetas
static const float TRC_MULT  = 20.9f;   // remate del parteluz (topa el oculo)
static const float TRC_APEX  = 25.6f;   // vertice del arco ojival (el dintel esta en 27)
// --- profundidad (n = hacia el INTERIOR desde el eje del muro; cara interior en n = 2) ---
static const float TRC_N_TRA = 0.9f;    // plano de la traceria (arco, lancetas, jambas)
static const float TRC_N_MUL = 1.45f;   // parteluz: algo mas adelante -> recorta mejor
static const float TRC_N_ORN = 1.15f;   // oculo y clave
static const float TRC_N_SL0 = 1.85f;   // alfeizar: canto bajo (al ras del muro)
static const float TRC_N_SL1 = 2.65f;   // alfeizar: canto alto (SALE 0.65 al pasillo)

// muro en el que se trabaja: convierte (u, y, n) locales a mundo
struct TrcWall { int alongX; float sgn; int flip; };

static inline void trcXZ(const TrcWall &w, float u, float nOff, float &ox, float &oz) {
    const float nw = w.sgn * (TRC_WC - nOff);          // n = 2 -> cara interior del muro
    if (w.alongX) { ox = u;  oz = nw; }                // muros +Z/-Z: la tangente es X
    else          { ox = nw; oz = u;  }                // muros +X/-X: la tangente es Z
}

// QUAD de UNA CARA mirando al interior del sector (6 verts). Los 4 puntos van en locales
// (u,y,n) y el sentido se corrige solo, asi que da igual como se listen.
static void trcQuad(TexVertex *buf, int &i, const TrcWall &w,
                    float u0, float y0, float n0, float u1, float y1, float n1,
                    float u2, float y2, float n2, float u3, float y3, float n3,
                    unsigned int col) {
    const float U[4] = { u0, u1, u2, u3 }, Y[4] = { y0, y1, y2, y3 }, N[4] = { n0, n1, n2, n3 };
    const float a2 = (u0*y1 - u1*y0) + (u1*y2 - u2*y1) + (u2*y3 - u3*y2) + (u3*y0 - u0*y3);
    int rev = (a2 < 0.0f) ? 1 : 0;                     // poligono al reves en el plano local
    if (w.flip) rev = 1 - rev;                         // ...y el muro puede invertirlo otra vez
    const int id[6] = { rev?1:0, rev?0:1, rev?3:2, rev?1:0, rev?3:2, rev?2:3 };
    for (int k = 0; k < 6; ++k) {
        const int p = id[k];
        float x, z; trcXZ(w, U[p], N[p], x, z);
        buf[i++] = { U[p] * (1.0f/TILE), -Y[p] * (1.0f/TILE), col, x, Y[p], z };
    }
}

// TRIANGULO de una cara (3 verts): para las enjutas, la pieza mas barata que hay aqui
static void trcTri(TexVertex *buf, int &i, const TrcWall &w,
                   float u0, float y0, float u1, float y1, float u2, float y2,
                   float nOff, unsigned int col) {
    const float U[3] = { u0, u1, u2 }, Y[3] = { y0, y1, y2 };
    const float a2 = (u0*y1 - u1*y0) + (u1*y2 - u2*y1) + (u2*y0 - u0*y2);
    int rev = (a2 < 0.0f) ? 1 : 0;
    if (w.flip) rev = 1 - rev;
    const int id[3] = { 0, rev?2:1, rev?1:2 };
    for (int k = 0; k < 3; ++k) {
        const int p = id[k];
        float x, z; trcXZ(w, U[p], nOff, x, z);
        buf[i++] = { U[p] * (1.0f/TILE), -Y[p] * (1.0f/TILE), col, x, Y[p], z };
    }
}

// BAQUETON: barra recta de grosor th entre dos puntos del alzado (sirve para jambas,
// parteluz, brazos del arco y lancetas). 6 verts, en cualquier inclinacion.
static void trcBar(TexVertex *buf, int &i, const TrcWall &w,
                   float ua, float ya, float ub, float yb,
                   float th, float nOff, unsigned int col) {
    const float dx = ub - ua, dy = yb - ya;
    const float L  = sqrtf(dx*dx + dy*dy);
    if (L < 0.001f) return;
    const float px = -dy / L * th * 0.5f, py = dx / L * th * 0.5f;   // perpendicular al eje
    trcQuad(buf, i, w, ua-px, ya-py, nOff, ub-px, yb-py, nOff,
                       ub+px, yb+py, nOff, ua+px, ya+py, nOff, col);
}

// --- el despiece de UN ventanal, centrado en u = uc (luz 2*TRC_HW, de y=7 a y=27) ---
static void trcBay(TexVertex *buf, int &i, const TrcWall &w, float uc) {
    const unsigned int stone = brighten(RGBA(64, 70, 82, 255), 2.1f);
    const unsigned int dark  = brighten(RGBA(46, 50, 60, 255), 2.0f);
    const unsigned int face  = brighten(dark,  0.82f);   // = "frente" de una caja: calza con los pilares
    const unsigned int faceL = brighten(stone, 0.82f);   // parteluz/clave/oculo: un punto mas claro
    const unsigned int faceD = brighten(dark,  0.60f);   // = "costado": el talud del alfeizar

    // 1) ALFEIZAR: moldura en talud (el canto alto sale al pasillo), en 3 tramos de 4.67
    const float sw = (2.0f * TRC_HW) / 3.0f;
    for (int s = 0; s < 3; ++s) {
        const float a = uc - TRC_HW + sw * (float)s, b = a + sw;
        trcQuad(buf, i, w, a, TRC_OPY0, TRC_N_SL0, b, TRC_OPY0, TRC_N_SL0,
                           b, TRC_SILL, TRC_N_SL1, a, TRC_SILL, TRC_N_SL1, faceD);
    }
    // 2) JAMBAS: un baqueton fino contra cada pilar, en 2 tramos de 5.15
    const float jm = (TRC_SILL + TRC_SPR) * 0.5f;
    for (int s = -1; s <= 1; s += 2) {
        const float ju = uc + 6.35f * (float)s;
        trcBar(buf, i, w, ju, TRC_SILL, ju, jm,      0.70f, TRC_N_TRA, face);
        trcBar(buf, i, w, ju, jm,       ju, TRC_SPR, 0.70f, TRC_N_TRA, face);
    }
    // 3) PARTELUZ: parte el vano en dos lancetas; en 2 tramos de 5.95
    const float mm = (TRC_SILL + TRC_MULT) * 0.5f;
    trcBar(buf, i, w, uc, TRC_SILL, uc, mm,       0.90f, TRC_N_MUL, faceL);
    trcBar(buf, i, w, uc, mm,       uc, TRC_MULT, 0.90f, TRC_N_MUL, faceL);
    // 4) ARCO OJIVAL: 2 cuerdas por lado (empinada al arrancar, tendida al cerrar = ogiva)
    for (int s = -1; s <= 1; s += 2) {
        const float f = (float)s;
        trcBar(buf, i, w, uc + 6.35f*f, TRC_SPR, uc + 4.60f*f, 23.60f,   0.75f, TRC_N_TRA, face);
        trcBar(buf, i, w, uc + 4.60f*f, 23.60f,  uc,           TRC_APEX, 0.75f, TRC_N_TRA, face);
    }
    // 5) CLAVE: cuna que monta sobre el vertice (mas ancha arriba, como toda clave)
    trcQuad(buf, i, w, uc-0.60f, 24.40f, TRC_N_ORN, uc+0.60f, 24.40f, TRC_N_ORN,
                       uc+0.95f, 26.50f, TRC_N_ORN, uc-0.95f, 26.50f, TRC_N_ORN, faceL);
    // 6) LANCETAS: los 2 arquitos de la cabeza (del baqueton al parteluz)
    for (int s = -1; s <= 1; s += 2) {
        const float f = (float)s;
        trcBar(buf, i, w, uc + 5.90f*f, TRC_LSPR, uc + 3.20f*f, TRC_LAPX, 0.55f, TRC_N_TRA, face);
        trcBar(buf, i, w, uc + 0.55f*f, TRC_LSPR, uc + 3.20f*f, TRC_LAPX, 0.55f, TRC_N_TRA, face);
    }
    // 7) OCULO: rombo en el timpano, apoyado en el remate del parteluz
    trcQuad(buf, i, w, uc, TRC_MULT, TRC_N_ORN, uc+1.50f, 22.40f, TRC_N_ORN,
                       uc, 23.90f,   TRC_N_ORN, uc-1.50f, 22.40f, TRC_N_ORN, faceL);
    // 8) ENJUTAS: un triangulito de piedra entre cada lanceta y el arco (3 verts cada uno)
    for (int s = -1; s <= 1; s += 2) {
        const float f = (float)s;
        trcTri(buf, i, w, uc + 5.55f*f, 19.35f, uc + 3.50f*f, 20.00f,
                          uc + 4.60f*f, 21.50f, TRC_N_TRA, face);
    }
}

// --- TRACERIA de los 24 ventanales (4 muros x 6 vanos) ---
static void buildTracery(TexVertex *buf, int &i) {
    const int nPier = (int)(TRC_WL / TRC_BAY) + 1;        // 7 pilares por muro (igual que sector.h)
    for (int wI = 0; wI < 4; ++wI) {
        TrcWall w;
        w.alongX = (wI < 2) ? 1 : 0;                      // 0,1 = muros +Z/-Z ; 2,3 = muros +X/-X
        w.sgn    = (wI & 1) ? -1.0f : 1.0f;
        // sentido del quad para que la cara mire SIEMPRE al interior del sector
        w.flip   = w.alongX ? (w.sgn < 0.0f) : (w.sgn > 0.0f);
        for (int k = 0; k + 1 < nPier; ++k) {             // un vano entre el pilar k y el k+1
            const float t0 = -TRC_WL * 0.5f + TRC_BAY * (float)k;
            const float t1 = t0 + TRC_BAY;
            if (t0 < -TRC_WL * 0.5f || t0 > TRC_WL * 0.5f) continue;   // mismo guard que sector.h
            if (t1 < -TRC_WL * 0.5f || t1 > TRC_WL * 0.5f) continue;
            trcBay(buf, i, w, (t0 + t1) * 0.5f);          // centro del vano
        }
    }
}
