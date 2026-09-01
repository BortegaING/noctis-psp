#pragma once
// PROJECT NOCTIS: "adorno de pueblo" en el SUELO alrededor de la plaza central -- lapidas, arboles muertos, muros bajos, barriles y una carreta rota (frio gotico MediEvil/Bloodborne) + faroles EMISIVOS calidos que dan los puntos de luz de contraste. Determinista (espiral de angulo aureo, sin rand). <=2200 verts.

// --- hash entero determinista (mismo mezclador que winPick) para variacion sin rand ---
static unsigned int vpHash(unsigned int x) {
    x ^= 61u; x ^= (x >> 16); x *= 9u; x ^= (x >> 4); x *= 0x27d4eb2du; x ^= (x >> 15);
    return x;
}
// jitter deterministico en [-1,1] a partir de un entero
static float vpJit(unsigned int s) {
    return ((float)(vpHash(s) & 1023u) / 511.5f) - 1.0f;
}

// --- una posicion (x,z) esta BLOQUEADA (spawn central o footprint de un edificio) ---
static bool vpBlocked(float x, float z) {
    if (x * x + z * z < 5.0f * 5.0f) return true;            // radio spawn (centro exacto)
    // edificios (ver city.h): {cx, cz, radio_a_evitar (footprint + margen)}
    static const float B[4][3] = {
        {  0.0f, -52.0f, 21.0f },   // catedral (30x30)
        {-38.0f, -20.0f, 15.0f },   // casona   (20x18)
        { 36.0f, -28.0f, 15.0f },   // mansion  (18x20)
        {-18.0f,  -6.0f, 12.0f },   // capilla/ruina (15x15)
    };
    for (int b = 0; b < 4; ++b) {
        float dx = x - B[b][0], dz = z - B[b][1];
        if (dx * dx + dz * dz < B[b][2] * B[b][2]) return true;
    }
    return false;
}

// --- siguiente punto libre de la espiral de angulo aureo dentro del anillo [rMin,rMax] ---
// Avanza 'step' (por referencia) hasta hallar SUELO ABIERTO; el n de props no cambia,
// solo se saltan las celdas ocupadas -> el conteo de verts es exacto.
static void vpSpot(int &step, float rMin, float rMax, float &ox, float &oz) {
    for (int tries = 0; tries < 512; ++tries) {
        ++step;
        float t   = (float)step;
        float ang = t * 2.39996323f;                                 // angulo aureo (rad)
        float u   = fmodf(t * 0.61803399f, 1.0f);                    // reparto radial
        float rad = rMin + (rMax - rMin) * sqrtf(u);                 // relleno por area
        float x   = cosf(ang) * rad;
        float z   = sinf(ang) * rad;
        if (!vpBlocked(x, z)) { ox = x; oz = z; return; }
    }
    ox = rMax; oz = 0.0f;                                            // fallback (no deberia ocurrir)
}

// firma exacta pedida: recibe el buffer y devuelve el n de verts escritos
static int buildVillageProps(LineVertex *buf) {
    int i = 0;
    int step = 7;                                                    // semilla de la espiral

    // ----- paleta -----
    const unsigned int STONE = RGBA( 70,  74,  86, 255);            // piedra gris FRIA (lapidas/muros)
    const unsigned int DEAD  = RGBA( 26,  22,  20, 255);            // arbol muerto casi negro
    const unsigned int WOOD  = RGBA( 46,  38,  30, 255);            // madera oscura (barril/carreta)
    const unsigned int POST  = RGBA( 40,  36,  40, 255);            // poste de farol (hierro frio)
    const unsigned int WARM  = RGBA(255, 180,  90, 255);            // farol EMISIVO calido (contraste)

    // ========================= LAPIDAS (~14) =========================
    // losa fina baja + a veces una cruz (2 cajitas). Piedra gris fria con variacion.
    for (int t = 0; t < 14; ++t) {
        float x, z; vpSpot(step, 8.0f, 42.0f, x, z);
        unsigned int h  = vpHash((unsigned int)(t * 131 + 7));
        unsigned int col = brighten(STONE, 0.80f + 0.30f * ((float)(h & 255) / 255.0f)); // variacion de tono
        // la cara delgada mira a un lado u otro segun el hash ("inclinada-ish")
        bool faceX = (h & 4u) != 0u;
        float w = faceX ? 0.15f : 0.50f;
        float d = faceX ? 0.50f : 0.15f;
        float ht = 0.80f + 0.30f * ((float)((h >> 8) & 7u) / 7.0f);
        addSolidBox(buf, i, x, 0.0f, z, w, d, ht, col);                                   // 30 losa
        // cruz "a veces": ~1 de cada 5 lapidas -> t = 0,5,10
        if (t % 5 == 0) {
            float cy = ht + 0.05f;
            addSolidBox(buf, i, x, cy,        z, 0.14f, 0.14f, 0.55f, col);               // 30 palo vertical
            addSolidBox(buf, i, x, cy + 0.30f, z, 0.42f, 0.14f, 0.13f, col);              // 30 travesano
        }
    }

    // ===================== ARBOLES MUERTOS (~6) =====================
    // tronco delgado alto + 3..5 ramas (cajitas finas orientadas por posicion). Casi negro.
    for (int t = 0; t < 6; ++t) {
        float x, z; vpSpot(step, 10.0f, 44.0f, x, z);
        unsigned int h   = vpHash((unsigned int)(t * 977 + 41));
        unsigned int col = brighten(DEAD, 0.85f + 0.35f * ((float)(h & 255) / 255.0f));
        float trunkH = 3.0f + 3.0f * ((float)((h >> 4) & 15u) / 15.0f);                   // 3..6
        addSolidBox(buf, i, x, 0.0f, z, 0.40f, 0.40f, trunkH, col);                       // 30 tronco
        int nb = 3 + (t % 3);                                                             // 3,4,5,3,4,5 -> 24 ramas
        float base = vpJit((unsigned int)(t * 53 + 3)) * 3.14159f;
        for (int b = 0; b < nb; ++b) {
            float ba   = base + (float)b * 1.90f;                                         // reparto radial
            float dirx = cosf(ba), dirz = sinf(ba);
            float blen = 0.85f + 0.30f * (float)(b % 3);
            float by   = trunkH * (0.45f + 0.11f * (float)b);                             // sube por el tronco
            if (by > trunkH - 0.4f) by = trunkH - 0.4f;
            float bcx = x + dirx * (blen * 0.5f + 0.20f);
            float bcz = z + dirz * (blen * 0.5f + 0.20f);
            bool xdom = (dirx * dirx >= dirz * dirz);                                      // elonga en el eje dominante
            float bw = xdom ? blen : 0.16f;
            float bd = xdom ? 0.16f : blen;
            addSolidBox(buf, i, bcx, by, bcz, bw, bd, 0.18f, col);                        // 30 rama
        }
    }

    // ================= CERCA / MURO BAJO de piedra (~2 tramos) =================
    // filas de cajas bajas (0.6 alto) formando un tramo de ~10 unidades, gris frio.
    for (int t = 0; t < 2; ++t) {
        float x, z; vpSpot(step, 12.0f, 40.0f, x, z);
        unsigned int h = vpHash((unsigned int)(t * 617 + 19));
        float ang   = ((float)(h & 255) / 255.0f) * 6.2832f;                              // orientacion del tramo
        float dirx  = cosf(ang), dirz = sinf(ang);
        bool  xdom  = (dirx * dirx >= dirz * dirz);
        const float seg = 3.35f;                                                          // 3 cajas ~= 10 u
        for (int s = -1; s <= 1; ++s) {
            float mx = x + dirx * (seg * (float)s);
            float mz = z + dirz * (seg * (float)s);
            float bw = xdom ? (seg + 0.1f) : 0.6f;
            float bd = xdom ? 0.6f : (seg + 0.1f);
            addSolidBox(buf, i, mx, 0.0f, mz, bw, bd, 0.60f, brighten(STONE, 0.9f));      // 30 tramo de muro
        }
    }

    // ========================= FAROLES (~6) =========================
    // poste + farol EMISIVO calido arriba. Cerca de caminos/plaza (anillo interior).
    for (int t = 0; t < 6; ++t) {
        float x, z; vpSpot(step, 7.0f, 20.0f, x, z);                                      // cerca de la plaza
        addSolidBox(buf, i, x, 0.0f, z, 0.25f, 0.25f, 3.20f, POST);                       // 30 poste
        addSolidBox(buf, i, x, 3.00f, z, 0.50f, 0.50f, 0.60f, WARM);                      // 30 farol calido (luz)
    }

    // ============ Opcional: BARRILES (2) + CARRETA rota (1) para vida de pueblo ============
    for (int t = 0; t < 2; ++t) {
        float x, z; vpSpot(step, 11.0f, 30.0f, x, z);
        unsigned int h = vpHash((unsigned int)(t * 401 + 89));
        addSolidBox(buf, i, x, 0.0f, z, 0.70f, 0.70f, 1.00f,
                    brighten(WOOD, 0.9f + 0.3f * ((float)(h & 63) / 63.0f)));             // 30 barril/cajon
    }
    {
        float x, z; vpSpot(step, 12.0f, 30.0f, x, z);
        // carreta rota: plataforma inclinada (apoyada) + 2 ruedas (cajitas finas)
        addSolidBox(buf, i, x,          0.35f, z, 1.80f, 0.90f, 0.30f, brighten(WOOD, 1.05f)); // 30 tabla/cama
        addSolidBox(buf, i, x - 0.80f,  0.00f, z, 0.16f, 0.90f, 0.90f, brighten(WOOD, 0.8f));  // 30 rueda
        addSolidBox(buf, i, x + 0.80f,  0.00f, z, 0.16f, 0.90f, 0.55f, brighten(WOOD, 0.8f));  // 30 rueda caida
    }

    return i;
}
