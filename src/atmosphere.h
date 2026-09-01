#pragma once
// NOCTIS - geometria de ESCALA/ATMOSFERA: ruinas suspendidas en el abismo + horizonte de monolitos que se disuelven en la niebla (megaestructura BLAME!, directiva 5 y 7).

// Ruinas colgando en el vacio + motas de luz distantes; sesgadas HACIA ABAJO para que mirar al abismo revele estructuras que se pierden en la oscuridad.
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

    return i;
}

// Anillo de monolitos COLOSALES en el horizonte: cajas conicas apiladas + aguja, apenas mas claros que la niebla para leerse como escala extrema disolviendose en bruma.
static int buildFarSilhouettes(LineVertex *buf) {
    int i = 0;
    const int N = 14;

    for (int k = 0; k < N; ++k) {
        // reparto irregular en todo el circulo: base regular + jitter determinista
        const float ang = (float)k * (6.2831853f / (float)N) + (float)(k % 3) * 0.42f;
        const float r   = 200.0f + (float)((k * 97) % 220);   // 200..419
        const float cx  = cosf(ang) * r;
        const float cz  = sinf(ang) * r;

        const float h     = 300.0f + (float)((k * 151) % 600);// 300..899: algunas verdaderamente gigantes
        const float baseY = -(float)((k * 53) % 40);          // 0 .. -39: algo hundidas bajo el horizonte
        const float w     = 46.0f + (float)((k * 71) % 54);   // 46..99

        // casi el color de la niebla; las mas altas, un pelo mas claras (bruma aerea = lejania)
        const float t  = (h - 300.0f) / 600.0f;               // 0..1
        const int   rr = 24 + (int)(10.0f * t);               // 24..34
        const int   gg = 26 + (int)(10.0f * t);               // 26..36
        const int   bb = 36 + (int)(12.0f * t);               // 36..48
        const unsigned int col = RGBA(rr, gg, bb, 255);

        // pila conica (3 cajas ahusadas) + aguja: monolito lejano, no edificio detallado
        const float h1 = h * 0.50f, h2 = h * 0.30f, h3 = h - h1 - h2;
        addSolidBox(buf, i, cx, baseY,            cz, w,         w,         h1, col);
        addSolidBox(buf, i, cx, baseY + h1,       cz, w * 0.70f, w * 0.70f, h2, brighten(col, 1.06f));
        addSolidBox(buf, i, cx, baseY + h1 + h2,  cz, w * 0.46f, w * 0.46f, h3, brighten(col, 1.12f));
        addPyramid (buf, i, cx, baseY + h,        cz, w * 0.46f, w * 0.46f, h * 0.26f, brighten(col, 1.18f));
    }

    return i;
}
