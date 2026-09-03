#pragma once
// NOCTIS - "EL POZO": PUENTES / MEGAVIGAS que cruzan el pozo vertical.
// La imagen firma de BLAME!: estructura colosal atravesando un vacio enorme.
//
//   buildBridges(buf) -> 7 puentes/vigas de pared a pared (largo ~320) a
//                        DISTINTAS alturas y angulos, pasando por/cerca del
//                        centro del pozo (radio ~160, y de -700 a +500).
//                        El jugador esta en el mirador y=0, z=+118 mirando -Z.
//
// Cada puente: cubierta (caja larga) + barandas (cajas finas) + costillas de
// soporte bajo la cubierta + cadenas/cables colgando + alguna lampara fria.
// Como addSolidBox es AABB, los puentes ROTADOS se componen de tramos cortos
// AABB solapados que escalonan a lo largo de la direccion (look de viga
// industrial dentada). Los de 0/90 grados son una sola caja (baratos).
//
// Niebla: mezcla hacia HAZE por |y| (clamp(|y|/420, 0, 0.9)) y despues
// fadeToVoid por distancia horizontal al jugador. Lo muy arriba / muy abajo
// queda casi color niebla -> SILUETAS. Determinista (hash entero), sin rand,
// sin heap, C++17, <math.h>.
//
// Incluir en main.cpp DESPUES de addSolidBox/addPyramid/fadeToVoid/HAZE
// (junto a atmosphere.h). Presupuesto: <= BR_MAX_VERTS (2800) vertices.

#include <math.h>

#define BR_MAX_VERTS 2800
static const float BR_PLAYER_Z = 118.0f;   // mirador del jugador (x=0, y=0, z=+118)
static const float BR_PI       = 3.14159265f;

// hash entero determinista (nombre propio para no chocar con ihash de atmosphere.h)
static inline unsigned int brHash(unsigned int x) {
    x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16;
    return x;
}

// mezcla lineal hacia HAZE
static inline unsigned int brMixHaze(unsigned int base, float t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    int br = base & 0xFF, bg = (base >> 8) & 0xFF, bb = (base >> 16) & 0xFF;
    int cr = HAZE & 0xFF, cg = (HAZE >> 8) & 0xFF, cb = (HAZE >> 16) & 0xFF;
    int r  = br + (int)((cr - br) * t);
    int g  = bg + (int)((cg - bg) * t);
    int b2 = bb + (int)((cb - bb) * t);
    return RGBA(r, g, b2, 255);
}

// niebla del pozo: FUERTE por |y| (altura relativa al jugador) + horizontal.
// fadeToVoid funde del todo a 98 unidades (escala del distrito); el pozo es
// ~3x mas grande, asi que la distancia horizontal se escala x0.42.
static inline unsigned int brFade(unsigned int col, float x, float y, float z) {
    float t = fabsf(y) / 420.0f; if (t > 0.9f) t = 0.9f;
    const unsigned int c = brMixHaze(col, t);
    const float dx = x, dz = z - BR_PLAYER_Z;
    const float hd = sqrtf(dx * dx + dz * dz);
    return fadeToVoid(c, hd * 0.42f);
}

// caja con niebla por su centro + guardia de presupuesto
static inline void brBox(LineVertex *buf, int &i, float cx, float baseY, float cz,
                         float w, float d, float h, unsigned int col) {
    if (i + 30 > BR_MAX_VERTS) return;
    addSolidBox(buf, i, cx, baseY, cz, w, d, h, brFade(col, cx, baseY + h * 0.5f, cz));
}
static inline void brPyr(LineVertex *buf, int &i, float cx, float baseY, float cz,
                         float w, float d, float apexH, unsigned int col) {
    if (i + 12 > BR_MAX_VERTS) return;
    addPyramid(buf, i, cx, baseY, cz, w, d, apexH, brFade(col, cx, baseY + apexH * 0.5f, cz));
}

// lampara fria: cajita azul palida. Se funde MENOS que el acero (mitad del
// haze por altura, cuarto por distancia) para que quede como punto tenue
// en la bruma y no desaparezca.
static inline void brLamp(LineVertex *buf, int &i, float x, float y, float z) {
    if (i + 30 > BR_MAX_VERTS) return;
    const unsigned int cold = RGBA(150, 200, 255, 255);
    float t = fabsf(y) / 420.0f * 0.5f; if (t > 0.45f) t = 0.45f;
    const float dx = x, dz = z - BR_PLAYER_Z;
    const float hd = sqrtf(dx * dx + dz * dz);
    const unsigned int c = fadeToVoid(brMixHaze(cold, t), hd * 0.25f);
    addSolidBox(buf, i, x, y, z, 1.2f, 1.2f, 1.2f, c);
}

// cable/cadena colgando HACIA ABAJO desde (x, yTop, z)
static inline void brCable(LineVertex *buf, int &i, float x, float yTop, float z,
                           float len, unsigned int col) {
    brBox(buf, i, x, yTop - len, z, 0.7f, 0.7f, len, brighten(col, 0.85f));
}

// costilla de soporte bajo la cubierta, PERPENDICULAR a la direccion ang
static inline void brRib(LineVertex *buf, int &i, float x, float yDeck, float z,
                         float ang, float w, unsigned int col) {
    const float ac = fabsf(cosf(ang)), as = fabsf(sinf(ang));
    const float rw = w * 1.35f * as + 1.6f * ac;
    const float rd = w * 1.35f * ac + 1.6f * as;
    brBox(buf, i, x, yDeck - 5.0f, z, rw, rd, 5.0f, brighten(col, 0.9f));
}

// Cubierta a lo largo de la direccion ang (en XZ), horizontal en y.
// t0..t1 en [-0.5, 0.5] = fraccion del largo (permite mitades rotas).
// nseg cajas AABB solapadas; con ang = 0 o 90 grados y nseg=1 es UNA caja.
static void brDeck(LineVertex *buf, int &i, float cx, float y, float cz,
                   float ang, float len, float w, float thick,
                   int nseg, float t0, float t1, unsigned int col) {
    const float c = cosf(ang), s = sinf(ang);
    const float ac = fabsf(c), as = fabsf(s);
    const float span = len * (t1 - t0);
    const float seg = span / (float)nseg;
    const float sw = seg * ac + w * as + 0.6f;   // AABB del tramo rotado
    const float sd = seg * as + w * ac + 0.6f;
    for (int k = 0; k < nseg; ++k) {
        const float t = t0 + ((float)k + 0.5f) * seg / len;
        brBox(buf, i, cx + c * t * len, y, cz + s * t * len, sw, sd, thick, col);
    }
}

// Viga DIAGONAL empinada: de (x0,y0,z0) a (x1,y1,z1) en nseg cajas escalonadas.
static void brDiagBeam(LineVertex *buf, int &i, float x0, float y0, float z0,
                       float x1, float y1, float z1, float w, int nseg, unsigned int col) {
    const float dx = x1 - x0, dy = y1 - y0, dz = z1 - z0;
    const float hl = sqrtf(dx * dx + dz * dz);
    const float ac = fabsf(dx) / hl, as = fabsf(dz) / hl;
    const float segH = hl / (float)nseg;
    const float segY = fabsf(dy) / (float)nseg;
    const float sw = segH * ac + w * as + 0.6f;
    const float sd = segH * as + w * ac + 0.6f;
    for (int k = 0; k < nseg; ++k) {
        const float t = ((float)k + 0.5f) / (float)nseg;
        const float py = y0 + dy * t - segY * 0.5f;
        brBox(buf, i, x0 + dx * t, py, z0 + dz * t, sw, sd, segY + 1.5f, col);
    }
}

// Puente COMPLETO axis-aligned/rotado: cubierta + barandas (solo si nseg==1,
// baratas) + costillas + cables + lamparas. Devuelve por referencia i.
static void brBridge(LineVertex *buf, int &i, float cx, float y, float cz, float ang,
                     float len, float w, float thick, int nseg,
                     int ribs, int cables, int lamps, int lampsBelow, unsigned int col) {
    const float c = cosf(ang), s = sinf(ang);
    brDeck(buf, i, cx, y, cz, ang, len, w, thick, nseg, -0.5f, 0.5f, col);

    // barandas: cajas finas largas en los bordes (solo puentes de 0/90 grados)
    if (nseg == 1) {
        const float off = w * 0.5f - 0.5f;
        for (int side = -1; side <= 1; side += 2) {
            const float rx = cx - s * off * (float)side, rz = cz + c * off * (float)side;
            const float rw = len * fabsf(c) + 0.8f * fabsf(s);
            const float rd = len * fabsf(s) + 0.8f * fabsf(c);
            brBox(buf, i, rx, y + thick, rz, rw, rd, 2.6f, brighten(col, 0.92f));
        }
    }
    // costillas de soporte bajo la cubierta
    for (int k = 0; k < ribs; ++k) {
        const float t = -0.36f + 0.72f * (float)k / (float)((ribs > 1) ? ribs - 1 : 1);
        brRib(buf, i, cx + c * t * len, y, cz + s * t * len, ang, w, col);
    }
    // cadenas colgando (largo variado por hash)
    for (int k = 0; k < cables; ++k) {
        const unsigned int h = brHash((unsigned int)k * 2654435761u + (unsigned int)(y + 1000.0f));
        const float t = -0.42f + 0.84f * (float)k / (float)((cables > 1) ? cables - 1 : 1);
        const float side = ((h & 1u) ? 1.0f : -1.0f) * (w * 0.5f - 1.0f);
        const float clen = 18.0f + (float)(h % 23u);
        brCable(buf, i, cx + c * t * len - s * side, y - 5.0f, cz + s * t * len + c * side, clen, col);
    }
    // lamparas frias sobre (o bajo) la cubierta
    for (int k = 0; k < lamps; ++k) {
        const float t = -0.3f + 0.6f * (float)k / (float)((lamps > 1) ? lamps - 1 : 1);
        const float ly = lampsBelow ? (y - 1.6f) : (y + thick + 0.6f);
        brLamp(buf, i, cx + c * t * len, ly, cz + s * t * len);
    }
}

// Puente ROTO: dos mitades con hueco al medio, extremos dentados (munones
// de viga + puas) y cadenas cortadas colgando del borde de la rotura.
static void brBrokenBridge(LineVertex *buf, int &i, float cx, float y, float cz, float ang,
                           float len, float w, float thick, int nsegHalf, unsigned int col) {
    const float c = cosf(ang), s = sinf(ang);
    const float gap = 0.09f;
    brDeck(buf, i, cx, y, cz, ang, len, w, thick, nsegHalf, -0.5f, -gap, col);
    brDeck(buf, i, cx, y, cz, ang, len, w, thick, nsegHalf,  gap,  0.5f, col);
    brRib(buf, i, cx + c * (-0.32f) * len, y, cz + s * (-0.32f) * len, ang, w, col);
    brRib(buf, i, cx + c * ( 0.32f) * len, y, cz + s * ( 0.32f) * len, ang, w, col);

    for (int side = -1; side <= 1; side += 2) {
        const float te = gap * (float)side;               // borde de la rotura
        const float ex = cx + c * te * len, ez = cz + s * te * len;
        const float dir = -(float)side;                   // hacia el hueco
        // munones: dos vigas retorcidas que sobresalen al hueco, largos distintos
        for (int m = 0; m < 2; ++m) {
            const unsigned int h = brHash((unsigned int)(side + 3) * 977u + (unsigned int)m * 131u);
            const float stub = 5.0f + (float)(h % 6u);
            const float lat = ((m == 0) ? -1.0f : 1.0f) * (w * 0.28f);
            const float sx = ex + c * dir * stub * 0.5f - s * lat;
            const float sz = ez + s * dir * stub * 0.5f + c * lat;
            const float sw = stub * fabsf(c) + 2.0f * fabsf(s) + 0.4f;
            const float sd = stub * fabsf(s) + 2.0f * fabsf(c) + 0.4f;
            const float droop = (float)(h % 3u);          // se comba hacia abajo
            brBox(buf, i, sx, y - droop, sz, sw, sd, thick * 0.6f, brighten(col, 0.88f));
            brPyr(buf, i, sx + c * dir * stub * 0.5f, y - droop, sz + s * dir * stub * 0.5f,
                  2.2f, 2.2f, 4.0f, brighten(col, 1.05f));
        }
        // cadena cortada que cuelga del borde roto
        brCable(buf, i, ex - s * (w * 0.3f) * (float)side, y - 1.0f, ez + c * (w * 0.3f) * (float)side,
                26.0f + 9.0f * (float)(side + 1), col);
    }
}

// ===================== construccion =====================
// Devuelve la cantidad de vertices escritos en buf (<= BR_MAX_VERTS).
static int buildBridges(LineVertex *buf) {
    int i = 0;
    const unsigned int steel = RGBA(52, 56, 66, 255);      // acero frio oscuro
    const unsigned int steelB = RGBA(48, 54, 68, 255);     // variante mas azulada
    const unsigned int steelD = RGBA(56, 58, 64, 255);     // variante mas gris

    // 1) ABISMO  y=-300, 0 grados (a lo largo de X): silueta profunda bajo los pies.
    brBridge(buf, i, 0.0f, -300.0f, -20.0f, 0.0f, 320.0f, 14.0f, 6.0f, 1,
             4, 3, 2, 0, steelD);

    // 2) BAJO EL MIRADOR  y=-150, 90 grados (a lo largo de Z, x=+30): pasa
    //    justo por debajo del mirador del jugador -> al mirar abajo se ve.
    brBridge(buf, i, 30.0f, -150.0f, 0.0f, BR_PI * 0.5f, 320.0f, 12.0f, 5.0f, 1,
             3, 3, 3, 0, steel);

    // 3) SOBRE LA CABEZA  y=+75, 15 grados, centrado en z=+80: cruza el pozo
    //    justo por encima de la linea de ojos del jugador, cadenas colgando.
    brBridge(buf, i, 0.0f, 75.0f, 80.0f, BR_PI * (15.0f / 180.0f), 320.0f, 12.0f, 5.0f, 7,
             3, 4, 3, 1, steel);

    // 4) CRUZ ALTA  y=+200, 90 grados (a lo largo de Z, x=-45).
    brBridge(buf, i, -45.0f, 200.0f, 0.0f, BR_PI * 0.5f, 320.0f, 10.0f, 5.0f, 1,
             3, 3, 2, 0, steelB);

    // 5) CIMA  y=+350, 0 grados: casi puro color niebla, solo silueta.
    brBridge(buf, i, 0.0f, 350.0f, -60.0f, 0.0f, 330.0f, 14.0f, 6.0f, 1,
             2, 0, 0, 0, steelD);

    // 6) DIAGONAL EMPINADA: de (-150,-380,+40) a (+150,+140,-40): sube 520 en
    //    310 horizontales (~59 grados), cruzando por delante del jugador.
    brDiagBeam(buf, i, -150.0f, -380.0f, 40.0f, 150.0f, 140.0f, -40.0f, 10.0f, 13, steelB);

    // 7) ROTA  y=-60, 45 grados: hueco al medio, extremos dentados, cadenas
    //    cortadas. Queda un poco por debajo del mirador, en diagonal.
    brBrokenBridge(buf, i, 10.0f, -60.0f, 20.0f, BR_PI * 0.25f, 320.0f, 12.0f, 5.0f, 5, steel);

    return i;
}
