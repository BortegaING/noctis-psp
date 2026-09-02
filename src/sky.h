// sky.h - CIELO GOTICO dramatico para PROJECT NOCTIS (reemplaza drawBackdrop).
// -----------------------------------------------------------------------------
// Se dibuja 2D, SIN profundidad, DETRAS de todo el mundo, una vez por frame.
// El que llama ya dejo GU_DEPTH_TEST desactivado (igual que para drawBackdrop):
//     sceGuDisable(GU_DEPTH_TEST);  drawSky();  sceGuEnable(GU_DEPTH_TEST);
// Estado al entrar: depth OFF, textura OFF, blend OFF. Al salir queda IGUAL
// (blend/textura OFF) -> no rompe nada del pipeline del que llama.
//
// Objetivo (directiva 3,49): vender ESCALA y SOLEDAD. Una LUNA palida enorme,
// baja sobre una megaestructura brumosa infinita. Oscuro y atmosferico
// (Bloodborne/BLAME!), NO brillante. Tonos gris-calido consistentes con
// CLEAR_COLOR / HAZE (definidos en main.cpp antes de este include).
//
// Reusa EXACTAMENTE el patron 2D de main.cpp (gradQuad/GradVertex/drawRect):
//   sceGuGetMemory(...) + vertice { color(8888), x,y,z(16bit) } +
//   GU_TRIANGLES con (GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D),
//   coords en espacio de PANTALLA, mismo winding TL,TR,BR / TL,BR,BL.
// Helpers y struct con nombres PROPIOS (Sky*) para no chocar si este header se
// incluye antes de que main.cpp defina GradVertex/gradQuad.
//
// Barato y determinista: ~64 triangulos, sin rand, sin heap, C++17, <math.h>.
#ifndef NOCTIS_SKY_H
#define NOCTIS_SKY_H

#include <pspgu.h>
#include <math.h>

// vertice 2D con color por-vertice (mismo layout que GradVertex de main.cpp)
struct SkyVtx { unsigned int color; short x, y, z; };
#define SKY_FLAGS (GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D)

// cambia SOLO el canal alpha de un color 0xAABBGGRR (para franjas translucidas)
static inline unsigned int skyWithA(unsigned int c, int a) {
    return (c & 0x00FFFFFFu) | ((unsigned int)a << 24);
}

// franja horizontal de ANCHO COMPLETO con gradiente vertical (== gradQuad).
static void skyBand(int y0, int y1, unsigned int cTop, unsigned int cBot) {
    SkyVtx *v = (SkyVtx *)sceGuGetMemory(sizeof(SkyVtx) * 6);
    v[0] = { cTop, 0,                  (short)y0, 0 };
    v[1] = { cTop, (short)SCR_WIDTH,   (short)y0, 0 };
    v[2] = { cBot, (short)SCR_WIDTH,   (short)y1, 0 };
    v[3] = { cTop, 0,                  (short)y0, 0 };
    v[4] = { cBot, (short)SCR_WIDTH,   (short)y1, 0 };
    v[5] = { cBot, 0,                  (short)y1, 0 };
    sceGuDrawArray(GU_TRIANGLES, SKY_FLAGS, 6, 0, v);
}

// quad libre con color por-esquina (TL,TR,BR,BL). Mismo winding que gradQuad.
static void skyQuad(int x0, int y0, int x1, int y1,
                    unsigned int c00, unsigned int c10,
                    unsigned int c11, unsigned int c01) {
    SkyVtx *v = (SkyVtx *)sceGuGetMemory(sizeof(SkyVtx) * 6);
    v[0] = { c00, (short)x0, (short)y0, 0 };
    v[1] = { c10, (short)x1, (short)y0, 0 };
    v[2] = { c11, (short)x1, (short)y1, 0 };
    v[3] = { c00, (short)x0, (short)y0, 0 };
    v[4] = { c11, (short)x1, (short)y1, 0 };
    v[5] = { c01, (short)x0, (short)y1, 0 };
    sceGuDrawArray(GU_TRIANGLES, SKY_FLAGS, 6, 0, v);
}

// disco aproximado por abanico de triangulos (centro->borde). Centro y borde
// con color distinto -> degrade radial suave (luna palida / halo). CW en
// pantalla (y hacia abajo): coincide con el winding de gradQuad.
static void skyFan(float cx, float cy, float r, int segs,
                   unsigned int cCenter, unsigned int cRim) {
    const int nv = segs * 3;
    SkyVtx *v = (SkyVtx *)sceGuGetMemory(sizeof(SkyVtx) * nv);
    const float step = 6.28318531f / (float)segs;
    int n = 0;
    for (int s = 0; s < segs; ++s) {
        const float a0 = step * (float)s;
        const float a1 = step * (float)(s + 1);
        v[n++] = { cCenter, (short)cx, (short)cy, 0 };
        v[n++] = { cRim, (short)(cx + cosf(a0) * r), (short)(cy + sinf(a0) * r), 0 };
        v[n++] = { cRim, (short)(cx + cosf(a1) * r), (short)(cy + sinf(a1) * r), 0 };
    }
    sceGuDrawArray(GU_TRIANGLES, SKY_FLAGS, nv, 0, v);
}

// banda de nube/bruma: tira horizontal oscura, densa al centro y disuelta en
// los extremos (alpha 0). Requiere blend activo. 2 quads = 4 tris.
static void skyCloud(int cy, int halfH, int cx, int halfW, unsigned int base, int aMax) {
    const unsigned int c0 = skyWithA(base, 0);
    const unsigned int cM = skyWithA(base, aMax);
    const int y0 = cy - halfH, y1 = cy + halfH;
    skyQuad(cx - halfW, y0, cx,          y1, c0, cM, cM, c0);  // entra (fade in)
    skyQuad(cx,         y0, cx + halfW,  y1, cM, c0, c0, cM);  // sale  (fade out)
}

// linea del horizonte escalonada: siluetas colosales OSCURAS de una
// megaestructura lejana. Alturas pseudo-aleatorias DETERMINISTAS (hash del
// indice, sin rand). El disco de la luna sube limpio sobre estructuras bajas.
static void skySilhouette() {
    const int   N     = 12;
    const int   yH    = 168;                 // linea base del horizonte
    const int   stepW = SCR_WIDTH / N;       // 40 px por "torre"
    const int   moonX = 306, moonGuard = 62; // ventana donde la luna sube limpia
    const unsigned int cTopSil = RGBA(26, 24, 21, 255);  // silueta: casi negro calido
    const unsigned int cBotSil = RGBA(15, 14, 12, 255);  // mas oscura hacia el pie (niebla baja)
    for (int s = 0; s < N; ++s) {
        const int x0  = s * stepW;
        const int x1  = (s == N - 1) ? SCR_WIDTH : (x0 + stepW);
        const int cxs = (x0 + x1) >> 1;
        unsigned int h = (unsigned int)(s * 2654435761u);   // hash entero barato
        h ^= h >> 13; h *= 0x9e3779b1u; h ^= h >> 15;
        int height = 8 + (int)(h % 44u);                    // 8..51 px sobre el horizonte
        if (cxs > moonX - moonGuard && cxs < moonX + moonGuard && height > 14)
            height = 6 + (int)(h % 8u);                     // frente a la luna: bajo (6..13)
        skyQuad(x0, yH - height, x1, SCR_HEIGHT, cTopSil, cTopSil, cBotSil, cBotSil);
    }
}

// ============================ CIELO COMPLETO ============================
static void drawSky() {
    // ---- 1) GRADIENTE VERTICAL (fondo). Carbon calido desaturado arriba ->
    //         se aclara hacia una banda de HAZE luminosa en el horizonte. ----
    const unsigned int cTop  = RGBA(34, 32, 28, 255);   // carbon calido (cielo alto, oscuro)
    const unsigned int cHigh = RGBA(56, 52, 46, 255);
    const unsigned int cGlow = RGBA(126, 118, 104, 255);// CLEAR_COLOR levantado: el horizonte "brilla"
    const unsigned int cLow  = RGBA(30, 28, 25, 255);   // niebla baja bajo el horizonte
    skyBand(0,   72,         cTop,  cHigh);
    skyBand(72,  134,        cHigh, HAZE);               // se funde en la bruma calida (HAZE)
    skyBand(134, 172,        HAZE,  cGlow);              // el horizonte brilla a traves de la niebla
    skyBand(172, SCR_HEIGHT, cGlow, cLow);               // piso brumoso (como el floorc de drawBackdrop)

    // ---- translucidos: halo, luna atenuada por nubes ----
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

    const float moonCx = 306.0f, moonCy = 126.0f;       // baja y descentrada (a la derecha)
    const float moonR  = 46.0f;

    // ---- 2a) HALO tenue: disco palido grande y translucido detras de la luna.
    skyFan(moonCx, moonCy, moonR * 2.05f, 12,
           skyWithA(RGBA(150, 142, 126, 255), 54),      // centro: brillo calido tenue
           skyWithA(RGBA(150, 142, 126, 255), 0));       // borde: se disuelve en el cielo

    // ---- 2b) LUNA: disco palido gris-blanco enorme, tinte calido leve. Centro
    //          mas luminoso que el borde -> se lee suave y voluminosa.
    skyFan(moonCx, moonCy, moonR, 12,
           RGBA(178, 172, 158, 255),                     // nucleo: palido luminoso (lo mas claro de la escena)
           RGBA(120, 114, 101, 255));                    // borde: cae hacia el tono del horizonte

    // ---- 3) NUBES/BRUMA: 2 tiras oscuras translucidas que DERIVAN (sin/tiempo,
    //         determinista) cruzando por delante de la luna -> profundidad.
    static float skyT = 0.0f; skyT += 0.01f;
    const int drift1 = (int)(sinf(skyT)          * 90.0f);
    const int drift2 = (int)(sinf(skyT * 0.7f + 1.7f) * 70.0f);
    skyCloud(96,  10, 250 + drift1, 185, RGBA(46, 43, 38, 255), 72); // cruza el borde superior de la luna
    skyCloud(150,  8, 210 + drift2, 150, RGBA(38, 35, 31, 255), 62); // banda baja cerca del horizonte

    sceGuDisable(GU_BLEND);

    // ---- 4) SILUETA DEL HORIZONTE: megaestructura oscura escalonada al frente.
    skySilhouette();
}

#endif // NOCTIS_SKY_H
