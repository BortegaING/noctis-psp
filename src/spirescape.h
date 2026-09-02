#pragma once
// PROJECT NOCTIS -- TELON DE FONDO = MEGAESTRUCTURA BRUTALISTA ENVOLVENTE.
// El patio esta DENTRO del castillo-BLAME!: pocas masas GRANDES (torres
// monoliticas + losas escalonadas coronadas por aguja gotica) + puentes
// horizontales (firma BLAME!) anillando el patio en TODAS las direcciones,
// r ~90..420. Fill-rate barato: pocos objetos, siluetas enormes. Fade fuerte
// por DISTANCIA (fadeToVoid con pseudo-distancia comprimida) y por ALTURA
// (spireSky, replica de heightHaze con la constante HAZE ya en scope: heightHaze
// se define despues de este include, no se puede llamar aqui) -> perspectiva
// aerea: lo lejano/alto se funde en la bruma calida, JAMAS vacio negro.
// Determinista: espiral aurea + hashing entero, sin rand, sin asignacion.
// Presupuesto ajustado: 3084 verts (<= 3500).

// hash entero de 32 bits (overflow unsigned = definido): mezcla fuerte.
static inline unsigned int spireHash(unsigned int x) {
    x = x * 2654435761u; x ^= x >> 15;
    x = x * 2246822519u; x ^= x >> 13;
    x = x * 3266489917u; x ^= x >> 16;
    return x;
}

// pseudo-distancia COMPRIMIDA: mapea el radio del anillo (90..420) dentro del
// rango util de fadeToVoid (26..98) para que el fade GRADUE a lo ancho del
// campo (cerca = silueta oscura legible, lejos = casi-niebla) en vez de saturar
// de golpe. r*0.22: r=90 -> ~sin fade (oscuro); r=420 -> ~niebla.
static inline float spireFog(float r) { return r * 0.22f; }

// niebla por ALTURA (identica a heightHaze de main.cpp, replicada porque aquel
// se declara DESPUES de este include). HAZE(90,84,74) ya esta en scope.
static inline unsigned int spireSky(unsigned int base, float y) {
    float t = y / 190.0f; if (t < 0.0f) t = 0.0f; if (t > 0.82f) t = 0.82f;
    int br = base & 0xFF, bg = (base >> 8) & 0xFF, bb = (base >> 16) & 0xFF;
    int cr = HAZE & 0xFF, cg = (HAZE >> 8) & 0xFF, cb = (HAZE >> 16) & 0xFF;
    int r  = br + (int)((cr - br) * t);
    int g  = bg + (int)((cg - bg) * t);
    int b2 = bb + (int)((cb - bb) * t);
    return RGBA(r, g, b2, 255);
}

// pizarra calida casi-negra -> fadeToVoid(distancia) + spireSky(altura) la
// disuelven en la bruma. r>=g>=b siempre (calido, nunca azul frio).
static inline unsigned int spireMat(unsigned int base, float r, float ymid) {
    return spireSky(fadeToVoid(base, spireFog(r)), ymid);
}

static int buildSpirescape(LineVertex *buf) {
    int i = 0;
    const float GA = 2.39996f;                         // angulo aureo: cubre 360 sin arcos visibles

    // pizarra calida base (todas las masas parten de aqui; el fade la disuelve)
    const unsigned int SLATE = warmTint(RGBA(56, 50, 42, 255)); // -> ~(67,51,34): pardo oscuro calido

    // ================= MASAS BRUTALISTAS (30) =================
    // 2 torres + 2 losas + 1 torronazo por cada 5 (k%5): presupuesto EXACTO.
    // Reparto por espiral aurea en TODO el circulo (la camara gira libre ahora:
    // sin sesgo -Z), radio 90..419 -> se solapan a distintas profundidades.
    for (int k = 0; k < 30; ++k) {
        const unsigned int hR = spireHash((unsigned int)(k * 7 + 1));   // radio
        const unsigned int hH = spireHash((unsigned int)(k * 7 + 2));   // altura
        const unsigned int hW = spireHash((unsigned int)(k * 7 + 3));   // ancho/fondo
        const unsigned int hJ = spireHash((unsigned int)(k * 7 + 4));   // jitter angular
        const unsigned int hC = spireHash((unsigned int)(k * 7 + 5));   // jitter color

        const float a  = (float)k * GA + ((float)(hJ % 100) / 100.0f - 0.5f) * 0.26f;
        const float r  = 90.0f + (float)(hR % 330u);   // 90..419: envuelve todas las direcciones
        const float cx = cosf(a) * r;
        const float cz = sinf(a) * r;

        // color por masa: leve jitter calido sobre la pizarra (r>=g>=b conservado)
        const unsigned int base = brighten(SLATE, 1.0f + ((float)(hC % 11u) - 5.0f) * 0.02f);

        const int type = k % 5;
        if (type == 0 || type == 1) {
            // TORRE MONOLITICA: 3 cajas ahusadas + aguja gotica alta (90 + 12 = 102)
            const float H  = 130.0f + (float)(hH % 190u);              // 130..319
            const float W  = 22.0f + (float)(hW % 20u);               // 22..41
            const float D  = W * (0.80f + (float)((hW >> 5) % 5u) * 0.06f);
            const float h1 = H * 0.50f, h2 = H * 0.30f, h3 = H - h1 - h2;
            addSolidBox(buf, i, cx, 0.0f,     cz, W,        D,        h1, spireMat(base,               r, h1 * 0.5f));
            addSolidBox(buf, i, cx, h1,       cz, W * 0.72f, D * 0.72f, h2, spireMat(brighten(base, 1.05f), r, h1 + h2 * 0.5f));
            addSolidBox(buf, i, cx, h1 + h2,  cz, W * 0.50f, D * 0.50f, h3, spireMat(brighten(base, 1.10f), r, h1 + h2 + h3 * 0.5f));
            addPyramid (buf, i, cx, H,        cz, W * 0.50f, D * 0.50f, H * 0.55f, spireMat(base, r, H));
        } else if (type == 2 || type == 3) {
            // LOSA ESCALONADA BRUTALISTA: 2 cajas ANCHAS + aguja corta (60 + 12 = 72)
            const float H  = 100.0f + (float)(hH % 150u);              // 100..249
            const float W  = 34.0f + (float)(hW % 30u);               // 34..63 (masa ancha)
            const float D  = 20.0f + (float)((hW >> 6) % 16u);        // 20..35
            const float h1 = H * 0.58f, h2 = H - h1;
            addSolidBox(buf, i, cx, 0.0f, cz, W,        D,        h1, spireMat(base,               r, h1 * 0.5f));
            addSolidBox(buf, i, cx, h1,   cz, W * 0.74f, D * 0.90f, h2, spireMat(brighten(base, 1.06f), r, h1 + h2 * 0.5f));
            addPyramid (buf, i, cx, H,    cz, W * 0.30f, D * 0.60f, H * 0.35f, spireMat(base, r, H));
        } else {
            // TORRONAZO CATEDRAL: 3 cajas + aguja alta + 2 pinaculos flanqueantes (90 + 12 + 24 = 126)
            const float H  = 220.0f + (float)(hH % 190u);              // 220..409: hitos que salen de la vista
            const float W  = 30.0f + (float)(hW % 20u);               // 30..49
            const float D  = W * 0.9f;
            const float h1 = H * 0.46f, h2 = H * 0.30f, h3 = H - h1 - h2;
            addSolidBox(buf, i, cx, 0.0f,     cz, W,        D,        h1, spireMat(base,               r, h1 * 0.5f));
            addSolidBox(buf, i, cx, h1,       cz, W * 0.74f, D * 0.74f, h2, spireMat(brighten(base, 1.05f), r, h1 + h2 * 0.5f));
            addSolidBox(buf, i, cx, h1 + h2,  cz, W * 0.52f, D * 0.52f, h3, spireMat(brighten(base, 1.10f), r, h1 + h2 + h3 * 0.5f));
            addPyramid (buf, i, cx, H,        cz, W * 0.52f, D * 0.52f, H * 0.60f, spireMat(base, r, H));
            const unsigned int cp = spireMat(base, r, H * 0.9f);
            addPyramid (buf, i, cx - W * 0.34f, h1 + h2, cz, W * 0.20f, W * 0.20f, H * 0.34f, cp);
            addPyramid (buf, i, cx + W * 0.34f, h1 + h2, cz, W * 0.20f, W * 0.20f, H * 0.34f, cp);
        }
    }

    // ================= PUENTES BLAME! (8) =================
    // losas horizontales colgadas en alto que "cruzan" entre torres lejanas:
    // firma de megaestructura. Caja larga y fina (axis-aligned, alterna X/Z).
    for (int b = 0; b < 8; ++b) {
        const unsigned int hb = spireHash((unsigned int)(b * 13 + 201));
        const float a   = (float)b * GA + 0.9f;
        const float r   = 140.0f + (float)(hb % 190u);          // 140..329
        const float cx  = cosf(a) * r;
        const float cz  = sinf(a) * r;
        const float yb  = 55.0f + (float)((hb >> 5) % 130u);    // 55..184 de altura
        const float len = 80.0f + (float)((hb >> 9) % 90u);     // 80..169 de largo
        const unsigned int c = spireMat(SLATE, r, yb);
        if (b & 1) addSolidBox(buf, i, cx, yb, cz, len,  7.0f, 5.0f, c);   // largo en X
        else       addSolidBox(buf, i, cx, yb, cz, 7.0f, len,  5.0f, c);   // largo en Z
    }

    return i;   // 30 masas (12 torres + 12 losas + 6 torronazos) + 8 puentes = 3084 verts
}
