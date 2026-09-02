#pragma once
// NOCTIS - geometria de ESCALA/ATMOSFERA. Vende el VACIO infinito y la
// MEGAESTRUCTURA sin fin (look BLAME!/Bloodborne):
//   buildVoidLayer      -> el ABISMO alrededor y DEBAJO de la plaza: ruinas
//                          suspendidas (cimas rotas, puentes, losas, arcos,
//                          PLATAFORMAS-ANILLO rotas) + motas de luz colgando
//                          en la profundidad. Sesgo HACIA ABAJO: al mirar el
//                          pozo la geometria se hunde y se disuelve en bruma
//                          -> "no se ve el fondo".
//   buildFarSilhouettes -> anillo COMPLETO de 360 grados de monolitos COLOSALES
//                          en el horizonte: torres, mega-losas, PUENTES que
//                          cruzan entre torres, y pistas de CATEDRAL con agujas
//                          gemelas. Ninguna direccion muestra vacio negro.
// Determinista: angulo aureo (2.39996) + hashing entero. Sin rand, sin heap.

// --- niebla por ALTURA para el anillo lejano (replica de heightHaze de
// main.cpp, que se declara DESPUES de este include; HAZE ya esta en scope).
// Las cimas colosales se funden en el cielo brumoso -> "salen de la vista". ---
static inline unsigned int farSky(unsigned int base, float y) {
    float t = y / 260.0f; if (t < 0.0f) t = 0.0f; if (t > 0.88f) t = 0.88f;
    int br = base & 0xFF, bg = (base >> 8) & 0xFF, bb = (base >> 16) & 0xFF;
    int cr = HAZE & 0xFF, cg = (HAZE >> 8) & 0xFF, cb = (HAZE >> 16) & 0xFF;
    int r  = br + (int)((cr - br) * t);
    int g  = bg + (int)((cg - bg) * t);
    int b2 = bb + (int)((cb - bb) * t);
    return RGBA(r, g, b2, 255);
}

// --- niebla por PROFUNDIDAD para el abismo. El pozo se llena de bruma: lo MUY
// hondo (y << 0) y lo MUY alto se disuelven en el color de niebla. Es la clave
// de "mirar abajo y no ver el fondo": la geometria no termina, se desvanece. ---
static inline unsigned int voidHaze(unsigned int base, float y) {
    float t = (y < 0.0f) ? (-y) / 520.0f    // hacia el pozo: -520 -> full niebla
                         :  y  / 340.0f;     // hacia arriba: mas suave
    if (t < 0.0f) t = 0.0f; if (t > 0.86f) t = 0.86f;
    int br = base & 0xFF, bg = (base >> 8) & 0xFF, bb = (base >> 16) & 0xFF;
    int cr = HAZE & 0xFF, cg = (HAZE >> 8) & 0xFF, cb = (HAZE >> 16) & 0xFF;
    int r  = br + (int)((cr - br) * t);
    int g  = bg + (int)((cg - bg) * t);
    int b2 = bb + (int)((cb - bb) * t);
    return RGBA(r, g, b2, 255);
}

// hash entero determinista -> dispersion "antigua e incomprensible" sin rand
static inline unsigned int ihash(unsigned int x) {
    x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16;
    return x;
}

// Ruinas colgando en el vacio + plataformas-anillo rotas + motas de luz.
// Sesgadas HACIA ABAJO: mirar al abismo revela estructuras que se pierden en la
// bruma. Fade doble: fadeToVoid (radial) + voidHaze (por profundidad).
static int buildVoidLayer(LineVertex *buf) {
    int i = 0;
    const float GA = 2.39996f; // angulo aureo -> dispersion irregular

    // ---- RUINAS SUSPENDIDAS (cimas rotas, losas, puentes, arcos) ----
    for (int k = 0; k < 18; ++k) {
        const unsigned int hh = ihash((unsigned int)(k * 2654435761u) + 17u);
        const float ang = (float)k * GA;
        const float r   = 120.0f + (float)(hh % 220u);            // 120..339: MAS ALLA del area jugable
        const float cx  = cosf(ang) * r;
        const float cz  = sinf(ang) * r;

        // Y: sesgo al abismo -> ~2/3 por debajo de 0 (se hunden hacia -430)
        float y;
        if (k % 3 == 0) y =   30.0f + (float)((hh >> 5) % 160u);  // zona alta:  30..189
        else            y = -430.0f + (float)((hh >> 3) % 470u);  // pozo sin fondo: -430..39

        // silueta fria casi-negra; el fade radial + la bruma de profundidad la
        // llevan al color de niebla cuanto mas lejos/hondo esta (se disuelve)
        const unsigned int tri = (hh >> 7) % 3u;
        unsigned int base = (tri == 0) ? RGBA(32, 34, 46, 255)
                          : (tri == 1) ? RGBA(22, 24, 33, 255)
                                       : RGBA(44, 46, 60, 255);
        const unsigned int col = voidHaze(fadeToVoid(base, r * 0.18f), y);

        const int type = k % 4;
        if (type == 0) {
            // cima de torre rota: munon + punta quebrada
            const float w = 12.0f + (float)(hh % 15u);            // 12..26
            const float h = 16.0f + (float)((hh >> 4) % 24u);     // 16..39
            addSolidBox(buf, i, cx, y, cz, w, w * 0.9f, h, col);
            addPyramid (buf, i, cx, y + h, cz, w * 0.8f, w * 0.7f, h * 0.55f,
                        voidHaze(brighten(col, 1.10f), y + h));
        } else if (type == 1) {
            // losa inclinada a la deriva (fina y ancha)
            const float w = 22.0f + (float)((hh >> 2) % 26u);     // 22..47
            const float d = 14.0f + (float)((hh >> 6) % 18u);     // 14..31
            addSolidBox(buf, i, cx, y, cz, w, d, 2.5f + (float)(k % 4), col);
        } else if (type == 2) {
            // fragmento de PUENTE / pasarela (largo y delgado, orientacion alterna)
            const float len = 34.0f + (float)((hh >> 3) % 46u);   // 34..79
            if (hh & 1u) addSolidBox(buf, i, cx, y, cz, len, 4.0f, 2.5f, col);
            else         addSolidBox(buf, i, cx, y, cz, 4.0f, len, 2.5f, col);
        } else {
            // arco / contrafuerte gotico roto: pilar + arranque en punta
            const float w = 6.0f + (float)((hh >> 1) % 8u);       // 6..13
            const float h = 20.0f + (float)((hh >> 5) % 30u);     // 20..49
            addSolidBox(buf, i, cx, y, cz, w, w, h, col);
            addPyramid (buf, i, cx, y + h, cz, w * 1.3f, w * 0.6f, h * 0.45f,
                        voidHaze(brighten(col, 1.08f), y + h));
        }
    }

    // ---- PLATAFORMAS-ANILLO ROTAS suspendidas en el pozo (megaestructura) ----
    // Baldosas dispuestas en circulo; se salta 1 segmento -> anillo ROTO (ruina),
    // no un dona perfecto. Muy hondas -> casi disueltas en la bruma del abismo.
    for (int p = 0; p < 3; ++p) {
        const unsigned int ph = ihash((unsigned int)(p * 40503u) + 101u);
        const float pr  = 150.0f + (float)(ph % 170u);            // centro anillo: 150..319
        const float pa  = (float)p * GA + 0.6f;
        const float pcx = cosf(pa) * pr;
        const float pcz = sinf(pa) * pr;
        const float py  = -360.0f + (float)((ph >> 4) % 300u);    // hondo: -360..-61
        const float rr  = 16.0f + (float)((ph >> 2) % 14u);       // radio del anillo: 16..29
        const unsigned int rc = voidHaze(fadeToVoid(RGBA(30, 32, 42, 255), pr * 0.18f), py);
        const int SEG = 6;
        const int gap = (int)(ph % (unsigned int)SEG);            // segmento faltante -> anillo roto
        for (int s = 0; s < SEG; ++s) {
            if (s == gap) continue;
            const float sa = (float)s * (6.2831853f / (float)SEG);
            const float sx = pcx + cosf(sa) * rr;
            const float sz = pcz + sinf(sa) * rr;
            addSolidBox(buf, i, sx, py, sz, rr * 0.5f, rr * 0.5f, 2.5f, rc);
        }
    }

    // ---- MOTAS DE LUZ: farolillos distantes colgando en la profundidad ----
    for (int m = 0; m < 18; ++m) {
        const unsigned int mh = ihash((unsigned int)(m * 2246822519u) + 7u);
        const float ang = (float)m * GA + 1.1f;                   // desfasadas de las ruinas
        const float r   = 140.0f + (float)(mh % 220u);            // 140..359
        const float mx  = cosf(ang) * r;
        const float mz  = sinf(ang) * r;
        const float my  = 170.0f - (float)((mh >> 3) % 560u);     // +170 .. -389 (muchas al fondo)
        const float s   = 1.4f + (float)(mh % 3u) * 0.8f;         // 1.4..3.0
        unsigned int lc = (mh & 1u) ? RGBA(118, 140, 182, 255)    // azul palido frio
                                    : RGBA(158, 122,  86, 255);   // ambar apagado
        if (my < -220.0f) lc = voidHaze(lc, my * 0.7f);           // las hondas titilan hacia la niebla
        addSolidBox(buf, i, mx, my, mz, s, s, s, lc);
    }

    return i;   // 18 ruinas + 3 anillos rotos (5 baldosas c/u) + 18 motas = 1638 verts
}

// Anillo de monolitos COLOSALES cerrando los 360 grados del horizonte, con
// PUENTES que los enlazan y pistas de CATEDRAL de agujas gemelas. Impostores
// muy fundidos en niebla (casi el color de HAZE) -> escala extrema, coste minimo.
// CLAVE anti-vacio: la base de CADA impostor es una planta CUADRADA de lado
// s = r*0.55, cuya anchura angular (~0.54 rad) SUPERA el paso (2*PI/18 = 0.349
// rad) en cualquier orientacion -> las bases adyacentes SIEMPRE se solapan:
// NINGUNA direccion de brujula deja ver vacio. Las cimas y variantes van ENCIMA
// de esa base y no afectan el cierre del anillo.
static int buildFarSilhouettes(LineVertex *buf) {
    int i = 0;
    const int   N    = 18;
    const float STEP = 6.2831853f / (float)N;   // 20 grados exactos entre impostores

    // pizarra calida base; el fade la lleva casi al color de la niebla (r>=g>=b)
    const unsigned int BASE = warmTint(RGBA(70, 64, 55, 255));

    for (int k = 0; k < N; ++k) {
        const unsigned int hh = ihash((unsigned int)(k * 2654435761u) + 91u);
        const float ang = (float)k * STEP;                        // paso EXACTO -> anillo sin brechas
        const float r   = 330.0f + (float)(hh % 140u);            // 330..469
        const float cx  = cosf(ang) * r;
        const float cz  = sinf(ang) * r;

        const float s = r * 0.55f;                                // planta cuadrada: cobertura angular > STEP
        const float H = 320.0f + (float)((hh >> 3) % 460u);       // 320..779: colosales, salen de la vista

        // color casi-niebla, graduado por distancia (mas lejos = mas fundido)
        const unsigned int col = fadeToVoid(BASE, r * 0.24f);

        const float h1 = H * 0.55f, h2 = H * 0.30f;
        // TIER 1 (base cuadrada): la capa que CIERRA los 360 grados. Inamovible.
        addSolidBox(buf, i, cx, 0.0f, cz, s, s, h1, farSky(col, h1 * 0.5f));

        if (k % 9 == 0) {
            // PISTA DE CATEDRAL colosal: cuerpo + AGUJAS GEMELAS (silueta inconfundible)
            addSolidBox(buf, i, cx, h1, cz, s * 0.66f, s * 0.66f, h2,
                        farSky(brighten(col, 1.05f), h1 + h2 * 0.5f));
            const float tw = s * 0.24f, off = s * 0.22f;
            addPyramid(buf, i, cx - off, h1 + h2, cz, tw, tw, H * 0.30f, farSky(brighten(col, 1.12f), H));
            addPyramid(buf, i, cx + off, h1 + h2, cz, tw, tw, H * 0.30f, farSky(brighten(col, 1.12f), H));
        } else if (k % 3 == 1) {
            // MEGA-LOSA: ancha en tangencial, fina en radial (muro colosal de fondo)
            float ws, ds;
            if (fabsf(cosf(ang)) >= fabsf(sinf(ang))) { ws = s * 0.42f; ds = s * 1.15f; }
            else                                      { ws = s * 1.15f; ds = s * 0.42f; }
            addSolidBox(buf, i, cx, h1, cz, ws, ds, h2, farSky(brighten(col, 1.05f), h1 + h2 * 0.5f));
            addPyramid(buf, i, cx, h1 + h2, cz, ws * 0.7f, ds * 0.7f, H * 0.14f, farSky(brighten(col, 1.10f), H));
        } else {
            // torre escalonada clasica rematada en aguja
            addSolidBox(buf, i, cx, h1, cz, s * 0.64f, s * 0.64f, h2,
                        farSky(brighten(col, 1.06f), h1 + h2 * 0.5f));
            addPyramid(buf, i, cx, h1 + h2, cz, s * 0.64f, s * 0.64f, H * 0.20f, farSky(brighten(col, 1.10f), H));
        }
    }

    // ---- PUENTES COLOSALES que cruzan entre torres vecinas ----
    // "la estructura continua mas alla": tableros horizontales tendidos en el
    // vano medio de cada 2do hueco, orientados segun la tangente dominante.
    for (int k = 0; k < N; ++k) {
        if (k & 1) continue;                                      // 9 puentes (huecos pares): peso controlado
        const unsigned int ha = ihash((unsigned int)(k * 2654435761u) + 91u);
        const unsigned int hb = ihash((unsigned int)(((k + 1) % N) * 2654435761u) + 91u);
        const float ra = 330.0f + (float)(ha % 140u);
        const float rb = 330.0f + (float)(hb % 140u);
        const float Ha = 320.0f + (float)((ha >> 3) % 460u);
        const float Hb = 320.0f + (float)((hb >> 3) % 460u);
        const float ma = ((float)k + 0.5f) * STEP;                // punto medio del vano
        const float mr = (ra + rb) * 0.5f;
        const float mx = cosf(ma) * mr;
        const float mz = sinf(ma) * mr;
        const float span = mr * STEP * 1.25f;                     // ~cuerda, un pelo mas para solapar torres
        const float by  = (Ha + Hb) * 0.5f * (0.46f + 0.12f * (float)((ha >> 7) & 1u)); // altura del tablero
        const unsigned int bc = farSky(fadeToVoid(BASE, mr * 0.24f), by + 12.0f);
        if (fabsf(cosf(ma)) >= fabsf(sinf(ma)))
            addSolidBox(buf, i, mx, by, mz, span * 0.16f, span, 14.0f, bc);  // largo en Z
        else
            addSolidBox(buf, i, mx, by, mz, span, span * 0.16f, 14.0f, bc);  // largo en X
    }

    return i;   // 18 impostores (2 catedrales gemelas) + 9 puentes = 1590 verts; anillo 360 continuo
}

// ============================ NOTA DE VERIFICACION ============================
// buildVoidLayer  -> 1638 verts (limite g_void 2000; presupuesto <=~1700 OK)
//   * 18 ruinas   radio 120..339,  Y  +189 .. -430 (2/3 bajo 0, sesgo al pozo)
//   * 3 anillos rotos (5 baldosas) radio 150..319,  Y  -360 .. -61
//   * 18 motas de luz              radio 140..359,  Y  +170 .. -389
//   Fade doble fadeToVoid(radial)+voidHaze(profundidad): lo hondo se disuelve en
//   niebla -> mirar abajo NO revela fondo (la estructura no termina, se esfuma).
// buildFarSilhouettes -> 1590 verts (limite g_farSil 2000; presupuesto <=~1700 OK)
//   * 18 impostores colosales radio 330..469, altura H 320..779 (cimas -> cielo)
//   * incluye 2 catedrales de agujas gemelas y 9 puentes entre torres
//   * base cuadrada s=r*0.55: cobertura angular ~0.54 rad > STEP 0.349 rad ->
//     bases adyacentes SIEMPRE solapan: COBERTURA 360 CONTINUA, sin vacio negro.
// Determinista (angulo aureo + ihash), sin rand/heap, C++17. Firmas intactas.
