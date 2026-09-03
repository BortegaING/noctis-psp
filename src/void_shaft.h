#pragma once
// NOCTIS - "THE SHAFT": el VACIO del pozo vertical (directiva maestra s.5).
//
// El jugador esta en una cornisa a y=0 (z ~ +118) dentro de un pozo colosal
// (radio ~160, paredes y=-700..+500). Este builder llena el vacio DEBAJO
// (y -660..-40) y ENCIMA (y +230..+480) con estructura que RETROCEDE en la
// bruma: cornisas/balcones que sobresalen de la pared, horcas/portales
// colgantes, pasarelas derrumbadas (en escalones = "inclinadas"), tuberias y
// cables verticales pegados a la pared, motas de luz frias (y unas pocas
// ambar) y masas oscuras enormes muy al fondo.
//
// La CLAVE del "mirar abajo y NO ver el fondo": todo se funde hacia HAZE
// (RGBA 58,70,88) con FUERZA segun |y| -> t = clamp(|y|/480, 0, 0.92).
// Las piezas mas hondas (y ~ -660) quedan a un 8% de su color: siluetas
// apenas visibles, del color de la niebla. Igual hacia arriba (+480).
//
// Determinista: angulo aureo (2.39996) + hash entero. Sin rand, sin heap.
// Presupuesto: <= VS_MAX_VERTS (2600). Emision fija = 2550 verts.
//   >>> El buffer destino debe tener al menos 2600 LineVertex. <<<
//
// Depende de lo que main.cpp declara ANTES del include: LineVertex, RGBA,
// HAZE, brighten, addSolidBox (30 v), addPyramid (12 v).

#include <math.h>

#define VS_MAX_VERTS 2600

// --- hash entero determinista (nombre propio: atmosphere.h ya define ihash) ---
static inline unsigned int vsHash(unsigned int x) {
    x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16;
    return x;
}
// 10 bits del hash -> [0,1]
static inline float vsFrac(unsigned int h, int shift) {
    return (float)((h >> shift) & 1023u) * (1.0f / 1023.0f);
}
// mezcla lineal hacia HAZE
static inline unsigned int vsMixHaze(unsigned int base, float t) {
    if (t < 0.0f) t = 0.0f; if (t > 1.0f) t = 1.0f;
    int br = base & 0xFF, bg = (base >> 8) & 0xFF, bb = (base >> 16) & 0xFF;
    int cr = HAZE & 0xFF, cg = (HAZE >> 8) & 0xFF, cb = (HAZE >> 16) & 0xFF;
    int r  = br + (int)((cr - br) * t);
    int g  = bg + (int)((cg - bg) * t);
    int b2 = bb + (int)((cb - bb) * t);
    return RGBA(r, g, b2, 255);
}
// niebla por |y|: FUERTE. -480/+480 -> 92% niebla (silueta apenas visible)
static inline unsigned int vsDepthFade(unsigned int col, float y) {
    float t = fabsf(y) / 480.0f;
    if (t > 0.92f) t = 0.92f;
    return vsMixHaze(col, t);
}
// motas de luz: se apagan con la profundidad, un poco mas lentas (siguen
// siendo "puntos" hasta bien hondo; las mas hondas quedan casi invisibles)
static inline unsigned int vsMoteFade(unsigned int col, float y) {
    float t = fabsf(y) / 560.0f;
    if (t > 0.90f) t = 0.90f;
    return vsMixHaze(col, t);
}

// Caja "radial": una losa de largo len que apunta al centro del pozo, con
// grosor tangencial thick, centrada a radio rCenter en el angulo ang.
// addSolidBox es axis-aligned, asi que se emite la caja envolvente de la losa
// rotada (w/d proyectados). A la escala del pozo se lee como cornisa/viga.
static inline void vsRadialBox(LineVertex *buf, int &i, float ang, float rCenter,
                               float len, float thick, float y, float h, unsigned int col) {
    const float c = cosf(ang), s = sinf(ang);
    const float ac = fabsf(c), as = fabsf(s);
    const float w = len * ac + thick * as;
    const float d = len * as + thick * ac;
    addSolidBox(buf, i, c * rCenter, y, s * rCenter, w, d, h, col);
}

// Y con sesgo al abismo: cada 4ta pieza va ARRIBA (+230..+480), el resto ABAJO
// (-660..-40). f en [0,1].
static inline float vsPickY(int k, float f) {
    if ((k & 3) == 3) return 230.0f + f * 250.0f;   // arriba: 230..480
    return -660.0f + f * 620.0f;                     // abajo:  -660..-40
}

// ============================================================================
static int buildVoidShaft(LineVertex *buf) {
    int i = 0;
    const float GA     = 2.39996f;  // angulo aureo -> dispersion irregular en 360
    const float R_WALL = 160.0f;    // radio de la pared del pozo

    // paleta fria (piedra/hierro del pozo)
    const unsigned int STONE_A = RGBA(30, 34, 46, 255);
    const unsigned int STONE_B = RGBA(22, 26, 36, 255);
    const unsigned int STONE_C = RGBA(40, 44, 58, 255);
    const unsigned int IRON    = RGBA(26, 28, 38, 255);
    const unsigned int PIPE    = RGBA(18, 22, 30, 255);
    const unsigned int MASS    = RGBA(14, 16, 24, 255);
    const unsigned int MOTE_COLD = RGBA(120, 170, 220, 255);
    const unsigned int MOTE_WARM = RGBA(255, 170,  80, 255);

    // ---- 1) CORNISAS / BALCONES pegados a la pared (20 x 30 v = 600) ----
    // Losas que sobresalen hacia adentro desde la pared, a muchas alturas.
    for (int k = 0; k < 20; ++k) {
        if (i + 30 > VS_MAX_VERTS) break;
        const unsigned int h = vsHash((unsigned int)k * 2654435761u + 101u);
        const float ang = (float)k * GA + 0.37f;
        const float y   = vsPickY(k, vsFrac(h, 0));
        const float len = 14.0f + vsFrac(h, 10) * 22.0f;     // 14..36 hacia adentro
        const float thk = 10.0f + vsFrac(h, 20) * 18.0f;     // 10..28 de ancho
        const float hh  = 2.0f  + vsFrac(h, 4)  * 3.5f;      // 2..5.5 de espesor
        const float rc  = R_WALL + 2.0f - len * 0.5f;        // borde exterior EMBEBIDO en la pared
        const unsigned int tri = (h >> 7) % 3u;
        const unsigned int base = (tri == 0) ? STONE_A : (tri == 1) ? STONE_B : STONE_C;
        vsRadialBox(buf, i, ang, rc, len, thk, y, hh, vsDepthFade(base, y));
    }

    // ---- 2) PLATAFORMAS ROTAS flotando a media altura del pozo (6 x 30 = 180) ----
    // Fragmentos sueltos, mas al centro (radio 40..110), casi todos en el abismo.
    for (int k = 0; k < 6; ++k) {
        if (i + 30 > VS_MAX_VERTS) break;
        const unsigned int h = vsHash((unsigned int)k * 2246822519u + 211u);
        const float ang = (float)k * GA + 1.9f;
        const float r   = 40.0f + vsFrac(h, 0) * 70.0f;
        const float y   = (k == 5) ? (260.0f + vsFrac(h, 10) * 200.0f)     // una arriba
                                   : (-620.0f + vsFrac(h, 10) * 540.0f);    // resto: -620..-80
        const float w   = 10.0f + vsFrac(h, 20) * 16.0f;
        const float d   = 8.0f  + vsFrac(h, 5)  * 14.0f;
        const float hh  = 1.6f  + vsFrac(h, 15) * 2.4f;
        const unsigned int base = ((h >> 9) & 1u) ? STONE_B : STONE_A;
        addSolidBox(buf, i, cosf(ang) * r, y, sinf(ang) * r, w, d, hh, vsDepthFade(base, y));
    }

    // ---- 3) HORCAS / PORTICOS colgantes (8 x 2 cajas = 480) ----
    // Viga fina que sale de la pared + puntal colgando de su extremo interior
    // (marco de horca). 6 abajo, 2 arriba.
    for (int k = 0; k < 8; ++k) {
        if (i + 60 > VS_MAX_VERTS) break;
        const unsigned int h = vsHash((unsigned int)k * 3266489917u + 307u);
        const float ang  = (float)k * GA + 3.1f;
        const float y    = vsPickY(k, vsFrac(h, 0));
        const float len  = 22.0f + vsFrac(h, 10) * 20.0f;    // 22..42 de viga
        const float rc   = R_WALL + 1.0f - len * 0.5f;
        const unsigned int col = vsDepthFade(IRON, y);
        // viga horizontal (delgada)
        vsRadialBox(buf, i, ang, rc, len, 1.6f, y, 1.4f, col);
        // puntal colgando del extremo interior, hacia abajo
        const float rIn  = R_WALL + 1.0f - len + 1.0f;
        const float drop = 10.0f + vsFrac(h, 20) * 22.0f;    // 10..32 colgando
        const float c = cosf(ang), s = sinf(ang);
        addSolidBox(buf, i, c * rIn, y - drop, s * rIn, 1.2f, 1.2f, drop, vsDepthFade(IRON, y - drop * 0.5f));
    }

    // ---- 4) PASARELAS DERRUMBADAS, inclinadas por escalones (3 x 3 cajas = 270) ----
    // Tres segmentos que bajan y avanzan hacia el centro: se lee como una
    // pasarela que cedio y cuelga inclinada al vacio. Todas en el abismo.
    for (int k = 0; k < 3; ++k) {
        if (i + 90 > VS_MAX_VERTS) break;
        const unsigned int h = vsHash((unsigned int)k * 2654435761u + 419u);
        const float ang  = (float)k * GA + 4.6f;
        const float y0   = -440.0f + vsFrac(h, 0) * 360.0f;  // -440..-80 (inicio en la pared)
        const float seg  = 9.0f + vsFrac(h, 10) * 5.0f;      // 9..14 por tramo
        const float wid  = 4.0f + vsFrac(h, 20) * 3.0f;      // 4..7 de ancho
        const float dip  = 3.0f + vsFrac(h, 5)  * 5.0f;      // 3..8 de caida por tramo
        for (int t = 0; t < 3; ++t) {
            const float rC = R_WALL + 1.0f - seg * 0.5f - (float)t * seg * 0.92f;
            const float yT = y0 - (float)t * dip;
            vsRadialBox(buf, i, ang, rC, seg, wid, yT, 0.9f,
                        vsDepthFade(brighten(IRON, 1.1f), yT));
        }
    }

    // ---- 5) TUBERIAS y CABLES verticales por la pared (14 x 30 = 420) ----
    // Cajas altas y finas pegadas a la pared (radio ~156). 10 abajo, 4 arriba.
    for (int k = 0; k < 14; ++k) {
        if (i + 30 > VS_MAX_VERTS) break;
        const unsigned int h = vsHash((unsigned int)k * 2246822519u + 523u);
        const float ang  = (float)k * GA + 0.9f;
        const bool  up   = (k % 7) == 3 || (k % 7) == 6;      // 4 de 14 arriba
        const float tall = 60.0f + vsFrac(h, 10) * 160.0f;    // 60..220 de alto
        float yBase;
        if (up) yBase = 240.0f + vsFrac(h, 0) * (480.0f - 240.0f - tall * 0.5f);
        else    yBase = -660.0f + vsFrac(h, 0) * (620.0f - tall * 0.4f);  // -660..~-40 (tope)
        if (!up && yBase + tall > -40.0f) yBase = -40.0f - tall;          // nunca invade la cornisa
        if ( up && yBase + tall > 480.0f) yBase = 480.0f - tall;          // nunca asoma sobre el pozo
        const float thk  = 1.4f + vsFrac(h, 20) * 1.8f;       // 1.4..3.2 (cable..tubo)
        const float c = cosf(ang), s = sinf(ang);
        const float r = 156.0f;
        // el color se toma en la mitad del tramo
        const unsigned int base = ((h >> 8) & 1u) ? PIPE : brighten(PIPE, 1.25f);
        addSolidBox(buf, i, c * r, yBase, s * r, thk, thk, tall, vsDepthFade(base, yBase + tall * 0.5f));
    }

    // ---- 6) MOTAS DE LUZ (16 x 30 = 480) ----
    // Cajitas brillantes: casi todas azul frio, cada 4ta ambar. Se apagan con la
    // profundidad; las mas hondas quedan como un fantasma de luz. 12 abajo, 4 arriba.
    for (int k = 0; k < 16; ++k) {
        if (i + 30 > VS_MAX_VERTS) break;
        const unsigned int h = vsHash((unsigned int)k * 3266489917u + 631u);
        const float ang  = (float)k * GA + 2.4f;
        const float r    = 96.0f + vsFrac(h, 10) * 56.0f;     // 96..152: cerca de la pared, algunas a medio pozo
        const float y    = vsPickY(k, vsFrac(h, 0));
        const float sz   = 1.2f + vsFrac(h, 20) * 0.9f;       // 1.2..2.1
        const unsigned int base = ((k & 3) == 1) ? MOTE_WARM : MOTE_COLD;
        addSolidBox(buf, i, cosf(ang) * r, y, sinf(ang) * r, sz, sz, sz, vsMoteFade(base, y));
    }

    // ---- 7) MASAS OSCURAS enormes muy al fondo (4 x 30 = 120) ----
    // Bloques colosales entre -660 y -520: a esa profundidad quedan al ~92% de
    // niebla -> siluetas apenas mas oscuras que la bruma. Sugieren que la
    // estructura sigue: no hay fondo.
    for (int k = 0; k < 4; ++k) {
        if (i + 30 > VS_MAX_VERTS) break;
        const unsigned int h = vsHash((unsigned int)k * 2654435761u + 743u);
        const float ang  = (float)k * GA + 5.2f;
        const float r    = 60.0f + vsFrac(h, 10) * 70.0f;     // 60..130
        const float y    = (k == 0) ? -660.0f                                // una SIEMPRE en el fondo del rango
                                    : -660.0f + vsFrac(h, 0) * 100.0f;      // resto: -660..-560 (base)
        const float w    = 40.0f + vsFrac(h, 20) * 50.0f;     // 40..90
        const float d    = 36.0f + vsFrac(h, 5)  * 46.0f;
        const float hh   = 30.0f + vsFrac(h, 15) * 60.0f;     // 30..90 de alto
        addSolidBox(buf, i, cosf(ang) * r, y, sinf(ang) * r, w, d, hh, vsDepthFade(MASS, y + hh * 0.5f));
    }

    return i;   // 2550 con la emision fija de arriba
}
