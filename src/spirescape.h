#pragma once
// PROJECT NOCTIS: BOSQUE DENSO DE AGUJAS GOTICAS -> el telon de fondo definitorio. Cientos de siluetas de torres/agujas finas apinadas que se pierden en la bruma calida-gris (Cathedral Ward de Bloodborne x escala BLAME!), vistas desde un balcon que las domina. Reparto por espiral aurea + hashing entero (determinista, sin rand). <= 4200 verts.

// hash entero de 32 bits (overflow unsigned = definido): mezcla fuerte para altura/ancho/color/jitter/tier.
static inline unsigned int spireHash(unsigned int x) {
    x = x * 2654435761u; x ^= x >> 15;
    x = x * 2246822519u; x ^= x >> 13;
    x = x * 3266489917u; x ^= x >> 16;
    return x;
}

// color gris-pardo calido desaturado (silueta en niebla): mas ALTO = mas claro (bruma aerea). Nunca azul frio: r>=g>=b siempre.
static inline unsigned int spireColor(float h, unsigned int hc) {
    float t = (h - 70.0f) / 350.0f;                 // 70..420 -> 0..1
    if (t < 0.0f) t = 0.0f; if (t > 1.0f) t = 1.0f; // clamp
    int j = (int)(hc % 9u) - 4;                     // jitter -4..+4 (igual en los 3 canales -> conserva calidez)
    int r = 50 + (int)(34.0f * t) + j;              // 50..84 (lejos/oscuro .. cerca/alto)
    int g = 48 + (int)(32.0f * t) + j;              // 48..80
    int b = 46 + (int)(28.0f * t) + j;              // 46..74
    if (r < 0) r = 0; if (r > 255) r = 255;
    if (g < 0) g = 0; if (g > 255) g = 255;
    if (b < 0) b = 0; if (b > 255) b = 255;
    return RGBA(r, g, b, 255);
}

static int buildSpirescape(LineVertex *buf) {
    int i = 0;
    const float GA = 2.39996f;                      // angulo aureo -> dispersion irregular sin arcos visibles

    // ================= AGUJAS SUELTAS (130) =================
    // cada k: espiral aurea + radio hash (solapan en pantalla = bosque). Sesgo de densidad a -Z (frente de camara)
    // plegando ~70% de las que caen detras hacia el frente, dejando ~30% detras para ENVOLVER al jugador.
    for (int k = 0; k < 32; ++k) {   // aun menos agujas (fill-rate PSP); igual se lee el bosque
        const unsigned int hT = spireHash((unsigned int)(k * 4 + 1));   // tier (fija el conteo de verts)
        const unsigned int hR = spireHash((unsigned int)(k * 9 + 2));   // radio
        const unsigned int hW = spireHash((unsigned int)(k * 9 + 3));   // ancho
        const unsigned int hJ = spireHash((unsigned int)(k * 9 + 4));   // jitter angular
        const unsigned int hF = spireHash((unsigned int)(k * 9 + 5));   // pliegue al frente
        const unsigned int hH = spireHash((unsigned int)(k * 9 + 7));   // altura
        const unsigned int hC = spireHash((unsigned int)(k * 9 + 8));   // color

        float a = (float)k * GA + ((float)(hJ % 100) / 100.0f - 0.5f) * 0.22f;
        float r = 55.0f + (float)(hR % 326u);                          // 55..380
        float cx = cosf(a) * r;
        float cz = sinf(a) * r;
        if (cz > 0.0f && (hF % 10u) < 7u) cz = -cz;                     // sesgo -Z (frente denso, retaguardia rala)

        // altura: mayoria 70..219, ~22% torres-catedral 240..419 (agujas altisimas dispersas en la multitud)
        float h = ((hH % 100u) < 22u) ? (240.0f + (float)(hH % 180u))
                                      : ( 70.0f + (float)(hH % 150u));
        float w = 2.0f + (float)(hW % 4u);                             // 2..5: finas, tipo aguja
        const unsigned int col = spireColor(h, hC);

        const unsigned int tier = hT % 100u;
        if (tier < 86u) {
            // NEEDLE: la aguja ES la torre -> una sola piramide finisima y altisima (12)
            addPyramid(buf, i, cx, 0.0f, cz, w * 0.9f, w * 0.9f, h, brighten(col, 1.06f));
        } else if (tier < 97u) {
            // FUSTE SIMPLE: caja delgada + aguja piramidal ALTA (30 + 12)
            const float bh = h * 0.60f;
            addSolidBox(buf, i, cx, 0.0f, cz, w, w, bh, col);
            addPyramid (buf, i, cx, bh,  cz, w, w, h * 0.46f, brighten(col, 1.12f));
        } else {
            // FUSTE AHUSADO: 2 cajas que se estrechan + aguja (30 + 30 + 12) -> torre-aguja imponente
            const float b1 = h * 0.40f, b2 = h * 0.34f;
            addSolidBox(buf, i, cx, 0.0f,     cz, w,        w,        b1, col);
            addSolidBox(buf, i, cx, b1,       cz, w * 0.68f, w * 0.68f, b2, brighten(col, 1.06f));
            addPyramid (buf, i, cx, b1 + b2,  cz, w * 0.68f, w * 0.68f, h * 0.36f, brighten(col, 1.12f));
        }
    }

    // ================= CATEDRALES / HITOS (14) =================
    // cuerpo escalonado (2 cajas) coronado por un RACIMO de 4 agujas finas: los grandes referentes del gentio.
    for (int c = 0; c < 3; ++c) {   // menos catedrales de fondo (FPS)
        const unsigned int hA = spireHash((unsigned int)(c * 11 + 101));  // angulo/radio
        const unsigned int hB = spireHash((unsigned int)(c * 11 + 102));  // dimensiones
        const unsigned int hF = spireHash((unsigned int)(c * 11 + 103));  // pliegue frente
        const unsigned int hN = spireHash((unsigned int)(c * 11 + 104));  // alturas del racimo
        const unsigned int hC = spireHash((unsigned int)(c * 11 + 105));  // color

        float a = (float)c * (GA * 1.7f) + 0.6f;
        float r = 70.0f + (float)(hA % 280u);                            // 70..349 (dentro del campo)
        float cx = cosf(a) * r;
        float cz = sinf(a) * r;
        if (cz > 0.0f && (hF % 10u) < 7u) cz = -cz;                       // tambien sesgadas al frente

        float H = 160.0f + (float)(hB % 180u);                           // 160..339: masas altas = hitos
        float W = 14.0f + (float)((hB >> 5) % 12u);                      // 14..25: mas anchas
        float D = W * (0.82f + (float)((hB >> 9) % 6u) * 0.05f);         // 0.82..1.07 * W
        const unsigned int col = spireColor(H, hC);

        // cuerpo: 2 cajas escalonadas (60)
        const float bh1 = H * 0.55f, bh2 = H * 0.28f;
        addSolidBox(buf, i, cx, 0.0f, cz, W,          D,          bh1, col);
        addSolidBox(buf, i, cx, bh1,  cz, W * 0.78f,  D * 0.78f,  bh2, brighten(col, 1.05f));
        const float top = bh1 + bh2;                                     // ~0.83 H

        // aguja CENTRAL del racimo: fuste fino + piramide alta (30 + 12)
        const float cw = W * 0.22f;
        const float ch = H * 0.42f;
        addSolidBox(buf, i, cx + W * 0.06f, top,             cz - D * 0.04f, cw, cw, ch * 0.66f, brighten(col, 1.02f));
        addPyramid (buf, i, cx + W * 0.06f, top + ch * 0.66f, cz - D * 0.04f, cw, cw, ch * 0.60f, brighten(col, 1.14f));

        // 3 agujas FLANQUEANTES (piramide sola, alturas variadas por hash) (12 * 3)
        const float ox[3] = { -W * 0.34f,  W * 0.34f, -W * 0.10f };
        const float oz[3] = { -D * 0.28f,  D * 0.30f,  D * 0.36f };
        const float nw = W * 0.13f;
        for (int s = 0; s < 3; ++s) {
            float nh = H * (0.30f + (float)((hN >> (s * 4)) % 5u) * 0.04f); // 0.30..0.46 H
            addPyramid(buf, i, cx + ox[s], top, cz + oz[s], nw, nw, nh, brighten(col, 1.10f));
        }
    }

    return i;   // 130 agujas + 14 catedrales(racimo x4) = 4122 verts
}
