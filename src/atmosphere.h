#pragma once
// NOCTIS - geometria de ESCALA/ATMOSFERA: ruinas suspendidas en el abismo +
// horizonte de monolitos COLOSALES que se disuelven en la niebla (megaestructura
// BLAME!). El anillo lejano CIERRA los 360 grados: ninguna direccion muestra
// vacio. Determinista (espiral aurea / hashing entero), sin rand, sin asignacion.

// niebla por ALTURA para el anillo lejano (replica de heightHaze de main.cpp,
// que se declara DESPUES de este include; HAZE ya esta en scope). Las cimas
// colosales se funden en el cielo brumoso -> "salen de la vista".
static inline unsigned int farSky(unsigned int base, float y) {
    float t = y / 260.0f; if (t < 0.0f) t = 0.0f; if (t > 0.88f) t = 0.88f;
    int br = base & 0xFF, bg = (base >> 8) & 0xFF, bb = (base >> 16) & 0xFF;
    int cr = HAZE & 0xFF, cg = (HAZE >> 8) & 0xFF, cb = (HAZE >> 16) & 0xFF;
    int r  = br + (int)((cr - br) * t);
    int g  = bg + (int)((cg - bg) * t);
    int b2 = bb + (int)((cb - bb) * t);
    return RGBA(r, g, b2, 255);
}

// Ruinas colgando en el vacio + motas de luz distantes; sesgadas HACIA ABAJO
// para que mirar al abismo revele estructuras que se pierden en la oscuridad.
static int buildVoidLayer(LineVertex *buf) {
    int i = 0;
    const float GA = 2.39996f; // angulo aureo -> dispersion irregular, "antigua e incomprensible"

    // --- RUINAS SUSPENDIDAS (cimas rotas, losas inclinadas, puentes, arcos) ---
    for (int k = 0; k < 20; ++k) {
        const float ang = (float)k * GA;
        const float r   = 120.0f + (float)((k * 83) % 220);   // 120..339: MAS ALLA del area jugable
        const float cx  = cosf(ang) * r;
        const float cz  = sinf(ang) * r;

        // Y: sesgo al abismo -> ~2/3 de las piezas por debajo de 0 (se hunden hacia -420)
        float y;
        if (k % 3 == 0) y =   40.0f + (float)((k * 67) % 180); // zona alta:  40..219
        else            y = -420.0f + (float)((k * 97) % 440); // pozo sin fondo: -420..19

        // color frio casi-negro (silueta en niebla); lo profundo se apaga hacia la oscuridad
        unsigned int col = (k % 3 == 0) ? RGBA(30, 32, 42, 255)
                         : (k % 3 == 1) ? RGBA(20, 22, 30, 255)
                                        : RGBA(42, 44, 56, 255);
        const float df = (y < -120.0f) ? 0.70f : (y < 40.0f ? 0.86f : 1.0f);
        col = brighten(col, df);

        const int type = k % 4;
        if (type == 0) {
            // cima de torre rota: munon + punta quebrada
            const float w = 12.0f + (float)((k * 29) % 15);   // 12..26
            const float h = 16.0f + (float)((k * 37) % 24);   // 16..39
            addSolidBox(buf, i, cx, y, cz, w, w * 0.9f, h, col);
            addPyramid (buf, i, cx, y + h, cz, w * 0.8f, w * 0.7f, h * 0.55f, brighten(col, 1.12f));
        } else if (type == 1) {
            // losa inclinada (fina y ancha, a la deriva)
            const float w = 20.0f + (float)((k * 41) % 24);   // 20..43
            const float d = 14.0f + (float)((k * 53) % 16);   // 14..29
            addSolidBox(buf, i, cx, y, cz, w, d, 2.5f + (float)(k % 4), col);
        } else if (type == 2) {
            // fragmento de puente / pasarela (largo y delgado, orientacion alterna)
            const float len = 30.0f + (float)((k * 61) % 40); // 30..69
            if (k & 1) addSolidBox(buf, i, cx, y, cz, len, 4.0f, 2.5f, col);
            else       addSolidBox(buf, i, cx, y, cz, 4.0f, len, 2.5f, col);
        } else {
            // arco / contrafuerte gotico roto: pilar + arranque en punta
            const float w = 6.0f + (float)((k * 17) % 8);     // 6..13
            const float h = 20.0f + (float)((k * 43) % 30);   // 20..49
            addSolidBox(buf, i, cx, y, cz, w, w, h, col);
            addPyramid (buf, i, cx, y + h, cz, w * 1.3f, w * 0.6f, h * 0.45f, brighten(col, 1.10f));
        }
    }

    // --- MOTAS DE LUZ (lucecitas frias distantes suspendidas en el vacio) ---
    for (int m = 0; m < 24; ++m) {
        const float ang = (float)m * GA + 1.1f;               // desfasadas de las ruinas
        const float r   = 140.0f + (float)((m * 91) % 220);   // 140..359
        const float mx  = cosf(ang) * r;
        const float mz  = sinf(ang) * r;
        const float my  = 180.0f - (float)((m * 71) % 580);   // +180 .. -399 (muchas hacia el fondo)
        const float s   = 1.5f + (float)(m % 3) * 0.75f;      // 1.5..3.0
        const unsigned int lc = (m & 1) ? RGBA(120, 140, 180, 255)  // azul palido frio
                                        : RGBA(150, 120,  90, 255); // ambar apagado
        addSolidBox(buf, i, mx, my, mz, s, s, s, lc);
    }

    return i;   // 20 ruinas + 24 motas = 1440 verts
}

// Anillo de monolitos COLOSALES cerrando los 360 grados del horizonte. Impostores
// muy fundidos en niebla (casi el color de HAZE) -> escala extrema, coste minimo.
// CLAVE anti-vacio: reparto en pasos ANGULARES REGULARES (sin jitter angular) y
// planta CUADRADA de lado s = r*0.55, cuya anchura angular (~0.55 rad) SUPERA el
// paso (2*PI/18 = 0.349 rad) en cualquier orientacion -> impostores adyacentes
// SIEMPRE se solapan: NINGUNA direccion de brujula deja ver vacio.
static int buildFarSilhouettes(LineVertex *buf) {
    int i = 0;
    const int   N    = 18;
    const float STEP = 6.2831853f / (float)N;   // 20 grados exactos entre impostores

    // pizarra calida base; el fade la lleva casi al color de la niebla (r>=g>=b)
    const unsigned int BASE = warmTint(RGBA(70, 64, 55, 255));

    for (int k = 0; k < N; ++k) {
        const float ang = (float)k * STEP;                   // paso EXACTO -> anillo sin brechas
        const float r   = 320.0f + (float)((k * 97) % 110);  // 320..429
        const float cx  = cosf(ang) * r;
        const float cz  = sinf(ang) * r;

        const float s = r * 0.55f;                           // planta cuadrada: cobertura angular > STEP
        const float H = 300.0f + (float)((k * 151) % 420);   // 300..719: colosales, salen de la vista

        // color casi-niebla, graduado por distancia (mas lejos = mas fundido)
        const unsigned int col = fadeToVoid(BASE, r * 0.24f);

        const float h1 = H * 0.55f, h2 = H * 0.30f, h3 = H - h1 - h2;
        addSolidBox(buf, i, cx, 0.0f,      cz, s,         s,         h1, farSky(col,               h1 * 0.5f));
        addSolidBox(buf, i, cx, h1,        cz, s * 0.64f, s * 0.64f, h2, farSky(brighten(col, 1.06f), h1 + h2 * 0.5f));
        addPyramid (buf, i, cx, h1 + h2,   cz, s * 0.64f, s * 0.64f, h3 + H * 0.10f, farSky(brighten(col, 1.10f), H));
    }

    return i;   // 18 impostores * (2 cajas + aguja = 72) = 1296 verts; anillo 360 continuo
}
