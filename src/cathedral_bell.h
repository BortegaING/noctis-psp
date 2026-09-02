#pragma once
#include <math.h>
// PROJECT NOCTIS: CAMPANILE (torre-campanario / reloj) -> el LANDMARK ICONICO del sur.
// NO es una caja de backrooms: la SILUETA la vende -> base pesada, fuste que AFINA en
// escalones, motivo de RELOJ mirando a la plaza, campanario calado con vanos ojivales
// donde las campanas cuelgan en tinieblas, 4 torretas de esquina rematadas en aguja y
// una GRAN AGUJA central coronando el conjunto (el punto mas alto). Original, con alma.
//
// Convenciones (main.cpp): addSolidBoxT(buf,i, cx,baseY,cz, w,d,h, col) [30v]
//                          addPyramidT(buf,i, cx,baseY,cz, w,d,apexH, col) [12v]
//   w=extension X, d=extension Z, h/apexH=altura Y. baseY=0 suelo, +Y arriba.
// Muros: brighten(baseColor,2.7f) + heightHaze(y) + fadeToVoid(dist).
// Vanos/recesos: piedra OSCURA SIN heightHaze -> se quedan negros y "perforan" contra
//   la bruma que aclara el resto de la piedra en altura (asi leen como huecos hondos).
// Cara a la plaza / NORTE del jugador = -Z (ahi va el reloj y el vano "heroe").
static void buildCathedralBell(TexVertex* buf, int& i, float cx, float cz, float w, float d, float h,
                               unsigned int baseColor, float dist, int* detailStartOut) {
    // ---- PALETA ----
    const unsigned int stone = fadeToVoid(brighten(baseColor, 2.7f), dist);              // muro legible
    const unsigned int slate = fadeToVoid(RGBA(30, 28, 26, 255), dist);                  // pizarra/agujas oscuras
    const unsigned int dark  = fadeToVoid(brighten(RGBA(20, 18, 20, 255), 1.0f), dist);  // vano recesado (sin haze)
    const unsigned int light = fadeToVoid(warmTint(brighten(baseColor, 3.6f)), dist);    // esfera del reloj

    // ---- COTAS VERTICALES (todo derivado de w,d,h -> escala con el footprint ~40x40x200) ----
    const float plinthH   = h * 0.025f;                 // apron/escalon inferior
    const float baseH     = h * 0.11f;                  // BASE pesada (ancha y baja)
    const float baseTop   = baseH;
    const float chamferH  = h * 0.03f;                  // chaflan / retiro sobre la base
    const float shaftBase = baseTop + chamferH;
    const float belfryBase = h * 0.70f;                 // arranque del campanario
    const float belfryH   = h * 0.12f;
    const float belfryTop = belfryBase + belfryH;
    const float shaftSpan = belfryBase - shaftBase;     // altura repartida en 3 escalones

    // ================= NUCLEO (base + fuste + campanario) -> ANTES del marcador LOD =================

    // ---- BASE pesada con apron y chaflan ----
    addSolidBoxT(buf, i, cx, 0.0f,    cz, w * 1.10f, d * 1.10f, plinthH,  heightHaze(brighten(stone, 0.86f), plinthH));      // 30 apron
    addSolidBoxT(buf, i, cx, 0.0f,    cz, w,         d,         baseH,    heightHaze(stone, baseH * 0.5f));                  // 30 base
    addSolidBoxT(buf, i, cx, baseTop, cz, w * 0.88f, d * 0.88f, chamferH, heightHaze(brighten(stone, 1.06f), baseTop));     // 30 chaflan/retiro

    // ---- FUSTE que AFINA en 3 escalones (taper vertical fuerte) ----
    const float wsh1 = w * 0.80f, wsh2 = w * 0.68f, wsh3 = w * 0.58f;
    const float t1 = shaftSpan * 0.40f, t2 = shaftSpan * 0.34f, t3 = shaftSpan * 0.26f;
    const float y1 = shaftBase, y2 = y1 + t1, y3 = y2 + t2;
    addSolidBoxT(buf, i, cx, y1, cz, wsh1, wsh1, t1, heightHaze(stone, y1 + t1 * 0.5f));                                     // 30 tramo 1
    addSolidBoxT(buf, i, cx, y2, cz, wsh2, wsh2, t2, heightHaze(stone, y2 + t2 * 0.5f));                                     // 30 tramo 2
    addSolidBoxT(buf, i, cx, y3, cz, wsh3, wsh3, t3, heightHaze(stone, y3 + t3 * 0.5f));                                     // 30 tramo 3

    // ---- Cornisa/corbel bajo el campanario (vuela hacia afuera) ----
    addSolidBoxT(buf, i, cx, belfryBase - h * 0.02f, cz, w * 0.70f, d * 0.70f, h * 0.02f,
                 heightHaze(brighten(stone, 1.10f), belfryBase));                                                           // 30 cornisa

    // ---- CAMPANARIO: caja mas ancha que el fuste ----
    const float belfW = w * 0.72f, belfD = d * 0.72f;
    addSolidBoxT(buf, i, cx, belfryBase, cz, belfW, belfD, belfryH, heightHaze(stone, belfryBase + belfryH * 0.5f));        // 30 caja campanario

    // ---- VANOS OJIVALES del campanario: 2 por cara x 4 caras (caja oscura recesada + capucha en punta) ----
    // las campanas cuelgan en tinieblas -> los vanos NO llevan heightHaze (quedan negros).
    {
        const float openW    = w * 0.13f;                       // ancho del vano
        const float openH     = belfryH * 0.60f;               // alto del vano
        const float openBaseY = belfryBase + belfryH * 0.14f;
        const float thick     = 1.4f;                           // hondura (atraviesa la cara)
        const float archH     = openW * 0.55f;                  // capucha apuntada
        const float bias      = 0.10f;                          // leve saliente para que no z-pelee con el muro
        for (int f = 0; f < 4; ++f) {                            // 0:-Z 1:+Z 2:+X 3:-X
            for (int k = -1; k <= 1; k += 2) {
                if (f < 2) {                                     // caras -Z / +Z (2 vanos repartidos en X)
                    const float oz = (f == 0) ? (cz - belfD * 0.5f - bias) : (cz + belfD * 0.5f + bias);
                    const float ox = cx + (float)k * (w * 0.16f);
                    addSolidBoxT(buf, i, ox, openBaseY, oz, openW, thick, openH, dark);                                      // 30 vano
                    addPyramidT (buf, i, ox, openBaseY + openH, oz, openW, thick, archH, dark);                             // 12 arco
                } else {                                         // caras +X / -X (2 vanos repartidos en Z)
                    const float ox = (f == 2) ? (cx + belfW * 0.5f + bias) : (cx - belfW * 0.5f - bias);
                    const float oz = cz + (float)k * (d * 0.16f);
                    addSolidBoxT(buf, i, ox, openBaseY, oz, thick, openW, openH, dark);                                      // 30 vano
                    addPyramidT (buf, i, ox, openBaseY + openH, oz, thick, openW, archH, dark);                             // 12 arco
                }
            }
        }
    }                                                                                                                       // 8 x (30+12) = 336

    // ---- Cornisa de coronacion del campanario (vuela; asienta la linea de torretas) ----
    addSolidBoxT(buf, i, cx, belfryTop, cz, w * 0.80f, d * 0.80f, h * 0.02f,
                 heightHaze(brighten(stone, 1.10f), belfryTop));                                                            // 30 cornisa alta

    // ---- MARCADOR LOD: nucleo listo (base+fuste+campanario). Lo fino va DESPUES. ----
    if (detailStartOut) *detailStartOut = i;   // core = 606 verts

    // ================= DETALLE (torretas + aguja + reloj + contrafuertes) -> tras el marcador =================
    const float capTop = belfryTop + h * 0.02f;

    // ---- 4 TORRETAS de esquina del campanario, cada una con su AGUJA ----
    {
        const float toff = w * 0.36f;               // esquinas del campanario
        const float tw   = w * 0.11f;               // finas
        const float th   = h * 0.18f;               // suben sobre el campanario
        const float tspH = h * 0.05f;               // agujita
        for (int sx = -1; sx <= 1; sx += 2) {
            for (int sz = -1; sz <= 1; sz += 2) {
                const float tx = cx + (float)sx * toff;
                const float tz = cz + (float)sz * toff;
                addSolidBoxT(buf, i, tx, belfryBase, tz, tw, tw, th, heightHaze(stone, belfryBase + th * 0.5f));            // 30 fuste torreta
                addPyramidT (buf, i, tx, belfryBase + th, tz, tw, tw, tspH,
                             heightHaze(brighten(slate, 1.12f), belfryBase + th));                                          // 12 aguja torreta
            }
        }
    }                                                                                                                       // 4 x 42 = 168

    // ---- LINTERNA + GRAN AGUJA central (el punto MAS ALTO) ----
    const float lantH = h * 0.05f;
    addSolidBoxT(buf, i, cx, capTop, cz, w * 0.42f, d * 0.42f, lantH,
                 heightHaze(brighten(stone, 1.08f), capTop));                                                               // 30 linterna
    addPyramidT (buf, i, cx, capTop + lantH, cz, w * 0.34f, d * 0.34f, h - (capTop + lantH),
                 heightHaze(brighten(slate, 1.14f), capTop + lantH));                                                       // 12 AGUJA central -> apice ~= h

    // ---- MOTIVO DE RELOJ en el fuste, cara a la plaza (-Z): recuadro + receso oscuro + esfera clara + agujas ----
    {
        const float faceZ  = cz - wsh2 * 0.5f;      // cara -Z del tramo 2
        const float cy     = y2 + t2 * 0.5f;        // centro del reloj a media altura del tramo 2
        const float fw     = w * 0.34f;             // recuadro
        addSolidBoxT(buf, i, cx,        cy - fw * 0.5f,       faceZ - 0.10f, fw,        0.6f, fw,        heightHaze(brighten(stone, 1.20f), cy)); // 30 recuadro
        addSolidBoxT(buf, i, cx,        cy - w * 0.14f,       faceZ - 0.20f, w * 0.28f, 0.7f, w * 0.28f, dark);                                    // 30 receso oscuro
        addSolidBoxT(buf, i, cx,        cy - w * 0.09f,       faceZ - 0.35f, w * 0.18f, 0.8f, w * 0.18f, heightHaze(light, cy));                   // 30 esfera clara
        addSolidBoxT(buf, i, cx,        cy,                   faceZ - 0.55f, 0.7f,      0.9f, w * 0.14f, dark);                                    // 30 aguja larga (arriba)
        addSolidBoxT(buf, i, cx + w * 0.055f, cy - 0.35f,     faceZ - 0.55f, w * 0.11f, 0.9f, 0.7f,      dark);                                    // 30 aguja corta (der)
    }                                                                                                                       // 5 x 30 = 150

    // ---- CONTRAFUERTES de esquina en la base (macizan la silueta al pie) ----
    {
        const float boff = w * 0.46f;
        const float bw   = w * 0.12f;
        const float bh   = h * 0.16f;
        const float bsp  = h * 0.05f;
        for (int sx = -1; sx <= 1; sx += 2) {
            for (int sz = -1; sz <= 1; sz += 2) {
                const float bx = cx + (float)sx * boff;
                const float bz = cz + (float)sz * boff;
                addSolidBoxT(buf, i, bx, 0.0f, bz, bw, bw, bh, heightHaze(brighten(stone, 0.92f), bh * 0.5f));              // 30 pilar
                addPyramidT (buf, i, bx, bh,   bz, bw, bw, bsp, heightHaze(brighten(slate, 1.08f), bh));                    // 12 remate
            }
        }
    }                                                                                                                       // 4 x 42 = 168
}
// ---------------------------------------------------------------------------------------------------
// ELEMENTOS: apron+base+chaflan (base pesada con retiro) | fuste de 3 escalones (taper) | cornisa |
//   campanario + 8 vanos ojivales calados (2/cara, caja oscura + capucha) + cornisa alta | 4 torretas
//   de esquina con aguja | linterna + GRAN aguja central (apice = h, el punto mas alto) | reloj -Z
//   (recuadro + receso + esfera + 2 agujas) | 4 contrafuertes de base.
// VERTS: nucleo (antes del marcador) = 9*30 + 8*(30+12) = 270 + 336 = 606
//        detalle (tras el marcador)  = 4*42 + 30 + 12 + 5*30 + 4*42 = 168+42+150+168 = 528
//        TOTAL = 1134 verts  (<= ~1600 OK)
// detailStartOut = i_entrada + 606  (marcado justo tras base+fuste+campanario; antes de
//        torretas + aguja coronaria + reloj + contrafuertes, que la LOD lejana puede cullear).
