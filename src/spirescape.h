#pragma once
// PROJECT NOCTIS -- TELON DE FONDO: BOSQUE DE AGUJAS GOTICAS EN NIEBLA.
// -----------------------------------------------------------------------------
// Que es: el anillo de siluetas que se ve POR LAS ABERTURAS del recinto y al
// subir sobre las masas. El recinto util llega a |x|,|z| <= 54 y sus muros
// suben hasta y = 60; TODO esto vive MAS ALLA, en r = 120..190, y solo existe
// para vender "la megaestructura sigue mucho mas alla de lo explorable".
//
// Referencia visual: bosque DENSO de agujas y campanarios que se pierde en la
// bruma; cientos de siluetas casi negras SUPERPUESTAS, cada vez mas tenues,
// con algun puntito ambar de ventana encendida.
//
// Como se consigue con 2790 vertices:
//   1) IMPOSTORES. Nada de detalle: caja + piramide (aguja), o 2 cajas
//      escalonadas + aguja (campanario). A 120+ unidades la niebla se come
//      cualquier ornamento, asi que el vertice se gasta en CANTIDAD, no en
//      filigrana: 50 siluetas superpuestas leen mucho mejor que 20 grandes.
//   2) TRES ANILLOS INTERCALADOS (k % 3): cerca / medio / lejos. Cada anillo
//      tiene su propio rango de radio Y de altura, asi que en cualquier
//      direccion hay tres profundidades apiladas -> parallax y "fondo sin fin".
//   3) NIEBLA EN DOS EJES.
//      - por DISTANCIA: fadeToVoid. OJO, fadeToVoid satura (t=1, HAZE puro) a
//        partir de dist=140, y este telon vive en 120..190: pasarle el radio
//        crudo dejaria TODO plano del mismo gris. spireFog() reescala el radio
//        real 118..192 sobre el tramo util 30..132 de fadeToVoid, asi que el
//        anillo cercano queda al ~11% de niebla (casi negro, silueta dura) y
//        el lejano al ~91% (fantasma a un paso de desaparecer). Es la misma
//        distancia real, solo estirada para que GRADUE en vez de saturar.
//      - por ALTURA: spireSky (replica de heightHaze, que se declara DESPUES
//        de este include y por eso no se puede llamar aqui). Las bases quedan
//        oscuras contra la banda luminosa del horizonte y las PUNTAS se
//        disuelven en el cielo. Perspectiva aerea: nunca un borde duro.
//   4) COBERTURA 360 por ANGULO AUREO (2.39996 rad). La secuencia k*GA es la
//      de menor discrepancia que existe en 1D: con 50 puntos el hueco angular
//      maximo es ~10.6 grados (+-1.7 de jitter), contra un campo de vision
//      horizontal de ~98 grados. Mires donde mires hay siluetas de los tres
//      anillos; no hay un solo sector vacio.
//   5) AMBAR. 8 ventanas encendidas (cajitas), solo en el anillo cercano y con
//      la niebla al 40% para que NO se agrisen: es el unico acento calido de
//      todo el telon, y quedan repartidas con un hueco maximo de 60 grados,
//      o sea que siempre se ve al menos una.
// Los fustes arrancan en y = -30 (bajo el horizonte) para que nunca se vea la
// "base flotando" al mirar desde arriba de una masa.
// Determinista: espiral aurea + hash entero. Sin rand, sin heap, C++17.

#include <math.h>

// hash entero de 32 bits (el overflow unsigned es comportamiento definido)
static inline unsigned int spireHash(unsigned int x) {
    x = x * 2654435761u; x ^= x >> 15;
    x = x * 2246822519u; x ^= x >> 13;
    x = x * 3266489917u; x ^= x >> 16;
    return x;
}

// hash -> [0,1)
static inline float spireF01(unsigned int h) {
    return (float)(h & 0xFFFFu) * (1.0f / 65536.0f);
}

// Distancia reescalada para fadeToVoid (a=20, b=140). El telon ocupa r=120..190,
// un tramo que fadeToVoid crudo aplastaria contra el tope: r>=140 -> HAZE puro,
// todo del mismo color, sin profundidad. Estiramos 118..192 sobre 30..132:
//   r=120 -> t~0.11 (casi negro)   r=155 -> t~0.46
//   r=170 -> t~0.74                r=190 -> t~0.91 (casi niebla pura)
static inline float spireFog(float r) {
    const float f = 30.0f + (r - 118.0f) * 1.38f;
    return (f < 0.0f) ? 0.0f : f;
}

// Niebla por ALTURA. Copia fiel de heightHaze() de main.cpp (que se define
// DESPUES de este include), recalibrada al rango de alturas del telon (hasta
// 260) y con zona muerta bajo y=20 para que las BASES sigan oscuras: si las
// bases tambien se aclararan, la silueta perderia el contraste contra la banda
// luminosa del horizonte que pinta drawSky(). HAZE ya esta en scope.
static inline unsigned int spireSky(unsigned int base, float y) {
    float t = (y - 20.0f) * (1.0f / 280.0f);
    if (t < 0.0f)   t = 0.0f;
    if (t > 0.80f)  t = 0.80f;
    const int br = base & 0xFF, bg = (base >> 8) & 0xFF, bb = (base >> 16) & 0xFF;
    const int cr = HAZE & 0xFF, cg = (HAZE >> 8) & 0xFF, cb = (HAZE >> 16) & 0xFF;
    const int r  = br + (int)((cr - br) * t);
    const int g  = bg + (int)((cg - bg) * t);
    const int b2 = bb + (int)((cb - bb) * t);
    return RGBA(r, g, b2, 255);
}

// color final de un trozo de silueta: niebla por distancia + niebla por altura
static inline unsigned int spireMat(unsigned int base, float r, float ymid) {
    return spireSky(fadeToVoid(base, spireFog(r)), ymid);
}

static int buildSpirescape(LineVertex *buf) {
    int i = 0;

    const float GA    = 2.39996f;   // angulo aureo: reparto uniforme en 360 sin arcos visibles
    const float YBASE = -30.0f;     // los fustes nacen bajo el horizonte (sin bases flotantes)

    // pizarra CALIDA casi negra. Es deliberado que el material sea calido y la
    // niebla (HAZE) fria: lo cercano tira a pardo-negro, lo lejano a gris-azul.
    const unsigned int SLATE = warmTint(RGBA(26, 20, 16, 255));   // ~(31,20,13)
    // ambar de ventana. Se pre-aclara porque addSolidBox oscurece las caras
    // laterales (x0.82 / x0.60) y son justamente las que se ven de lejos.
    const unsigned int EMBER = brighten(RGBA(190, 112, 44, 255), 1.20f);

    for (int k = 0; k < 50; ++k) {
        const unsigned int hA = spireHash((unsigned int)(k * 9 + 11));   // jitter angular
        const unsigned int hR = spireHash((unsigned int)(k * 9 + 12));   // radio
        const unsigned int hH = spireHash((unsigned int)(k * 9 + 13));   // altura
        const unsigned int hW = spireHash((unsigned int)(k * 9 + 14));   // ancho / fondo / remate
        const unsigned int hC = spireHash((unsigned int)(k * 9 + 15));   // jitter de color

        // ---- anillo (k%3): los tres se intercalan, asi que cada direccion
        //      recibe cerca+medio+lejos apilados uno detras de otro ----
        const int ring = k % 3;
        float r, H;                                  // H = altura de la PUNTA sobre y=0
        if (ring == 0)      { r = 120.0f + (float)(hR % 22u);  H =  42.0f + (float)(hH % 118u); } // 120..141 / 42..159
        else if (ring == 1) { r = 141.0f + (float)(hR % 27u);  H =  65.0f + (float)(hH % 130u); } // 141..167 / 65..194
        else                { r = 165.0f + (float)(hR % 26u);  H =  95.0f + (float)(hH % 165u); } // 165..190 / 95..259

        const float a  = (float)k * GA + (spireF01(hA) - 0.5f) * 0.060f;  // +-1.7 grados
        const float ca = cosf(a), sa = sinf(a);
        const float cx = ca * r, cz = sa * r;

        // leve variacion de tono por silueta (r>=g>=b conservado: nunca frio)
        const unsigned int base = brighten(SLATE, 0.94f + spireF01(hC) * 0.16f);

        float W, D;
        if ((k % 10) < 3) {
            // ---- CAMPANARIO: 2 cajas escalonadas + aguja (30+30+12 = 72 v) ----
            W = 12.0f + H * 0.100f + (float)(hW % 7u);
            D = W * (0.82f + (float)((hW >> 5) % 5u) * 0.07f);
            const float y1 = H * 0.42f;                 // cuerpo bajo (el mas ancho)
            const float y2 = H * 0.72f;                 // cuerpo de campanas
            addSolidBox(buf, i, cx, YBASE, cz, W,         D,         y1 - YBASE,
                        spireMat(base, r, (YBASE + y1) * 0.5f));
            addSolidBox(buf, i, cx, y1,    cz, W * 0.70f, D * 0.70f, y2 - y1,
                        spireMat(brighten(base, 1.06f), r, (y1 + y2) * 0.5f));
            addPyramid (buf, i, cx, y2,    cz, W * 0.70f, D * 0.70f, H - y2,
                        spireMat(base, r, (y2 + H) * 0.5f));
        } else {
            // ---- AGUJA: caja + piramide (30+12 = 42 v). El grueso del bosque ----
            W = 9.0f + H * 0.075f + (float)(hW % 5u);   // esbeltez ~8:1, gotica
            D = W * (0.80f + (float)((hW >> 5) % 6u) * 0.06f);
            const float ys = H * (0.60f + (float)((hW >> 9) % 5u) * 0.025f); // arranque de la aguja
            addSolidBox(buf, i, cx, YBASE, cz, W, D, ys - YBASE,
                        spireMat(base, r, (YBASE + ys) * 0.5f));
            addPyramid (buf, i, cx, ys,    cz, W, D, H - ys,
                        spireMat(brighten(base, 1.05f), r, (ys + H) * 0.5f));
        }

        // ---- VENTANA ENCENDIDA (8 siluetas: k = 3,9,...,45; todas anillo 0) ----
        // k%6==3 implica k%3==0, o sea SIEMPRE el anillo cercano: es el unico
        // donde la niebla todavia deja pasar el color. Huecos angulares entre
        // ambares: 15..60 grados, siempre por debajo del campo de vision (~98).
        if ((k % 6) == 3) {
            const unsigned int hE = spireHash((unsigned int)(k * 9 + 16));
            const float sz = 2.6f + spireF01(hE) * 1.6f;                       // 2.6..4.2 (3..6 px a esa distancia)
            const float wy = H * (0.30f + spireF01(hE >> 8) * 0.34f);          // en el fuste, nunca en la aguja
            // pegada a la cara que MAS mira al centro del recinto (eje dominante):
            // asi el punto queda sobre la silueta y no flotando al lado.
            float wx = cx, wz = cz;
            if (fabsf(ca) >= fabsf(sa)) wx = cx - ((ca >= 0.0f) ? 1.0f : -1.0f) * (W * 0.5f + sz * 0.5f + 0.8f);
            else                        wz = cz - ((sa >= 0.0f) ? 1.0f : -1.0f) * (D * 0.5f + sz * 0.5f + 0.8f);
            // niebla al 40%: la luz "atraviesa" la bruma. Sin niebla de altura:
            // una ventana encendida no se disuelve, se apaga por distancia.
            addSolidBox(buf, i, wx, wy, wz, sz, sz, sz * 1.35f,
                        fadeToVoid(EMBER, spireFog(r) * 0.40f));
        }
    }

    // 35 agujas (42) + 15 campanarios (72) + 8 ventanas (30) = 1470+1080+240
    return i;   // 2790 vertices
}
