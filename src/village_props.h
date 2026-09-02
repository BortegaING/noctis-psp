#pragma once
// PROJECT NOCTIS: la PLAZA CENTRAL vestida de gotico -- un MONUMENTO focal (obelisco
// sobre pedestal escalonado), BRASEROS de pie con llama EMISIVA calida (los unicos
// acentos saturados, la luz de contraste), GUARDIANES encapuchados en pedestales,
// ARCOS y COLUMNAS rotas + escombros para grandeza decaida, y un anillo bajo de
// bordillo alrededor del monumento. Piedra gris FRIA (r>=g>=b); calido solo la llama.
// Determinista (anillo par + espiral de angulo aureo + hash entero, SIN rand). <=2400 verts.

// --- hash entero determinista (mismo mezclador que winPick) para variacion sin rand ---
static unsigned int vpHash(unsigned int x) {
    x ^= 61u; x ^= (x >> 16); x *= 9u; x ^= (x >> 4); x *= 0x27d4eb2du; x ^= (x >> 15);
    return x;
}
// jitter deterministico en [-1,1] a partir de un entero
static float vpJit(unsigned int s) {
    return ((float)(vpHash(s) & 1023u) / 511.5f) - 1.0f;
}

// --- una posicion (x,z) esta BLOQUEADA (spawn central o footprint de una catedral) ---
// Footprints REALES de city.h (centros y anchos completos w x d), como rectangulos con
// margen -> ningun adorno pisa la base de una catedral. El jugador (1ra persona) queda
// acotado ~46 al centro; los muros lo frenan al acercarse.
static bool vpBlocked(float x, float z) {
    if (x * x + z * z < 4.5f * 4.5f) return true;               // radio spawn (centro exacto)
    // {cx, cz, semiancho+margen, semiprofundo+margen}  (margen ~2.5)
    static const float B[4][4] = {
        {   0.0f, -60.0f, 28.5f, 24.5f },   // 0 GRAND   (52x44) al norte
        {  62.0f,   6.0f, 24.5f, 26.5f },   // 1 TWIN    (44x48) al este
        { -60.0f,  10.0f, 26.5f, 38.5f },   // 2 BASILICA(48x72) al oeste (nave larga)
        {  10.0f,  60.0f, 22.5f, 22.5f },   // 3 BELL    (40x40) al sur
    };
    for (int b = 0; b < 4; ++b) {
        if (fabsf(x - B[b][0]) < B[b][2] && fabsf(z - B[b][1]) < B[b][3]) return true;
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

    // ----- paleta: piedra gris FRIA (r>=g>=b); calido EMISIVO solo la llama -----
    const unsigned int STONE = RGBA( 70,  72,  80, 255);            // piedra gris fria
    const unsigned int FLAME = RGBA(255, 185,  95, 255);           // llama EMISIVA calida (unico acento)

    // ===================== MONUMENTO CENTRAL: OBELISCO GOTICO =====================
    // Punto focal offset del spawn exacto (+z, deja libre la vista norte a la GRAND).
    // Pedestal escalonado -> plinto -> fuste afinado en 2 tramos -> piramidion.
    {
        const float mx = 0.0f, mz = 7.0f;
        addSolidBox(buf, i, mx, 0.0f,  mz, 5.6f, 5.6f, 0.5f, brighten(STONE, 1.05f));  // 30 escalon 0
        addSolidBox(buf, i, mx, 0.5f,  mz, 4.4f, 4.4f, 0.5f, brighten(STONE, 1.00f));  // 30 escalon 1
        addSolidBox(buf, i, mx, 1.0f,  mz, 3.4f, 3.4f, 0.5f, brighten(STONE, 0.95f));  // 30 escalon 2
        addSolidBox(buf, i, mx, 1.5f,  mz, 2.2f, 2.2f, 1.3f, brighten(STONE, 1.02f));  // 30 plinto
        addSolidBox(buf, i, mx, 2.8f,  mz, 1.2f, 1.2f, 4.2f, brighten(STONE, 1.00f));  // 30 fuste bajo
        addSolidBox(buf, i, mx, 7.0f,  mz, 0.9f, 0.9f, 3.4f, brighten(STONE, 1.06f));  // 30 fuste alto (afinado)
        addPyramid (buf, i, mx, 10.4f, mz, 0.9f, 0.9f, 1.5f, brighten(STONE, 1.15f));  // 12 piramidion
        // anillo BAJO de bordillo alrededor del monumento (define el centro de la plaza)
        for (int c = 0; c < 6; ++c) {
            float a  = (float)c * 1.04719755f;                                          // 60 grados
            float cx = mx + cosf(a) * 4.2f;
            float cz = mz + sinf(a) * 4.2f;
            addSolidBox(buf, i, cx, 0.0f, cz, 0.9f, 0.9f, 0.35f, brighten(STONE, 0.9f)); // 30 bordillo x6
        }
    }

    // ===================== BRASEROS / PILARES-ANTORCHA (7) =====================
    // anillo PAR alrededor de la plaza: pilar de piedra + cuenco + LLAMA emisiva calida.
    // r=16 -> siempre libre (los footprints de catedral llegan >=~37 del centro).
    for (int t = 0; t < 7; ++t) {
        float a = 0.30f + (float)t * 0.897597901f;                                      // 2*pi/7
        float x = cosf(a) * 16.0f, z = sinf(a) * 16.0f;
        addSolidBox(buf, i, x, 0.00f, z, 0.55f, 0.55f, 2.50f, brighten(STONE, 0.95f)); // 30 pilar
        addSolidBox(buf, i, x, 2.50f, z, 0.95f, 0.95f, 0.45f, brighten(STONE, 1.08f)); // 30 cuenco
        addPyramid (buf, i, x, 2.95f, z, 0.80f, 0.80f, 1.25f, FLAME);                   // 12 LLAMA calida
    }

    // ===================== GUARDIANES ENCAPUCHADOS (6) =====================
    // figura robada sugerida por cajas afinadas: pedestal + tunica + hombros + cabeza + capucha.
    // anillo exterior via espiral aurea (esquiva footprints solo). Cabeza inclinada por hash.
    for (int t = 0; t < 6; ++t) {
        float x, z; vpSpot(step, 24.0f, 40.0f, x, z);
        unsigned int h    = vpHash((unsigned int)(t * 149 + 11));
        float        tone = 0.90f + 0.22f * ((float)(h & 255) / 255.0f);               // variacion de tono
        float        hx   = vpJit((unsigned int)(t * 71 + 5)) * 0.18f;                 // leve inclinacion
        float        hz   = vpJit((unsigned int)(t * 97 + 9)) * 0.18f;
        addSolidBox(buf, i, x,      0.0f, z,      1.70f, 1.70f, 1.00f, brighten(STONE, 0.95f)); // 30 pedestal
        addSolidBox(buf, i, x,      1.0f, z,      1.15f, 0.95f, 2.50f, brighten(STONE, tone));  // 30 tunica
        addSolidBox(buf, i, x,      3.5f, z,      1.35f, 1.05f, 0.50f, brighten(STONE, tone*1.03f)); // 30 hombros
        addSolidBox(buf, i, x + hx, 4.0f, z + hz, 0.55f, 0.55f, 0.70f, brighten(STONE, 1.05f)); // 30 cabeza
        addPyramid (buf, i, x + hx, 4.7f, z + hz, 0.55f, 0.55f, 0.60f, brighten(STONE, 1.08f)); // 12 capucha
    }

    // ===================== COLUMNAS ROTAS (4) =====================
    // tambor de base + fuste truncado a distinta altura (rotas). Piedra fria oscura.
    for (int t = 0; t < 4; ++t) {
        float x, z; vpSpot(step, 20.0f, 42.0f, x, z);
        unsigned int h = vpHash((unsigned int)(t * 211 + 23));
        float ht = 1.4f + 2.2f * ((float)((h >> 5) & 7u) / 7.0f);                       // 1.4..3.6 (rotas)
        addSolidBox(buf, i, x, 0.0f, z, 0.95f, 0.95f, 0.30f, brighten(STONE, 0.88f));  // 30 tambor base
        addSolidBox(buf, i, x, 0.3f, z, 0.70f, 0.70f, ht,    brighten(STONE, 0.96f));  // 30 fuste truncado
    }

    // ===================== ARCOS GOTICOS EXENTOS (2) =====================
    // 2 pilares + dintel + apice apuntado (piramide). Orientacion X/Z por hash. Span ~2.8.
    for (int t = 0; t < 2; ++t) {
        float x, z; vpSpot(step, 26.0f, 42.0f, x, z);
        unsigned int h    = vpHash((unsigned int)(t * 307 + 31));
        bool         xax  = (h & 1u) != 0u;
        unsigned int col  = brighten(STONE, 0.98f);
        float dx = xax ? 1.40f : 0.0f, dz = xax ? 0.0f : 1.40f;
        addSolidBox(buf, i, x - dx, 0.0f, z - dz, 0.50f, 0.50f, 3.00f, col);            // 30 pilar A
        addSolidBox(buf, i, x + dx, 0.0f, z + dz, 0.50f, 0.50f, 3.00f, col);            // 30 pilar B
        if (xax) {
            addSolidBox(buf, i, x, 3.00f, z, 3.40f, 0.50f, 0.45f, col);                 // 30 dintel
            addPyramid (buf, i, x, 3.45f, z, 3.00f, 0.50f, 1.20f, brighten(col, 1.08f)); // 12 apice apuntado
        } else {
            addSolidBox(buf, i, x, 3.00f, z, 0.50f, 3.40f, 0.45f, col);                 // 30 dintel
            addPyramid (buf, i, x, 3.45f, z, 0.50f, 3.00f, 1.20f, brighten(col, 1.08f)); // 12 apice apuntado
        }
    }

    // ===================== ESCOMBROS (3) =====================
    // 2 bloques caidos por foco -> grandeza decaida sin saturar el suelo.
    for (int t = 0; t < 3; ++t) {
        float x, z; vpSpot(step, 12.0f, 40.0f, x, z);
        unsigned int h = vpHash((unsigned int)(t * 419 + 43));
        float ox = 0.55f + 0.10f * (float)(h & 3u);
        addSolidBox(buf, i, x,       0.0f, z,       1.10f, 0.90f, 0.55f, brighten(STONE, 0.85f)); // 30 bloque grande
        addSolidBox(buf, i, x + ox,  0.0f, z - 0.4f, 0.70f, 0.70f, 0.35f, brighten(STONE, 0.80f)); // 30 bloque chico
    }

    return i;   // verts escritos (<=2400)
}
