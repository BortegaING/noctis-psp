#pragma once
// =============================================================================
// cementerio_models.h -- los MODELOS y la DISPOSICION del cementerio (piso 2), sin nada del motor:
// se arman igual en la PSP (src/cementerio.h) y en la PC (tools/modelview) para disenarlos a la vista.
//
// Benjamin (2026-10-16): "dejemos que el cementerio sea un lugar como de ambiente, mejora mucho
// mucho su diseno, tipo silent hill". Nada de cajas: lapidas de tope redondo y ojival con zocalo
// biselado y placa grabada, cruces y cruces celtas, obeliscos, tumbas con reja, angeles que LLORAN
// (alas de plumas como las del Serafin), mausoleos goticos, cipreses, arboles secos, faroles, la
// capilla al fondo y la figura de la niebla. Colores apagados (piedra, musgo, hierro): la niebla
// hace el resto.
//
// Convenciones: cada modelo se arma en el ORIGEN con el frente mirando a -Z (como todo el juego) y
// se coloca con yaw (rotY). Las tumbas miran al SUR (+Z, hacia la entrada): yaw = PI.
// =============================================================================

// ---------------------------------------------------------------- paleta (apagada, Silent Hill)
static const unsigned int BCM_STONE  = RGBA(152, 150, 142, 255);
static const unsigned int BCM_STONE2 = RGBA(124, 122, 116, 255);
static const unsigned int BCM_STONED = RGBA( 92,  90,  86, 255);
static const unsigned int BCM_MOSS   = RGBA( 78,  90,  66, 255);
static const unsigned int BCM_MARBLE = RGBA(190, 188, 182, 255);
static const unsigned int BCM_IRON   = RGBA( 40,  40,  44, 255);
static const unsigned int BCM_IRONL  = RGBA( 76,  76,  82, 255);
static const unsigned int BCM_DIRT   = RGBA( 74,  64,  52, 255);
static const unsigned int BCM_GRASS  = RGBA( 86,  94,  72, 255);
static const unsigned int BCM_GRASSD = RGBA( 66,  74,  58, 255);
static const unsigned int BCM_GRAVEL = RGBA(122, 116, 106, 255);
static const unsigned int BCM_BARK   = RGBA( 56,  50,  44, 255);
static const unsigned int BCM_CYP    = RGBA( 30,  44,  34, 255);
static const unsigned int BCM_CYP2   = RGBA( 50,  68,  48, 255);
static const unsigned int BCM_GLASS  = RGBA(255, 206, 128, 255);   // vidrio del farol (se enciende)
static const unsigned int BCM_DARK   = RGBA( 22,  20,  24, 255);   // la figura

// DE LEJOS (LOD): cada modelo grande se arma dos veces; con g_bcmLod = 1 salen menos lados, sin
// monticulo, placa ni barrotes (a mas de ~22 unidades, entre la niebla, no se distinguen).
static int g_bcmLod = 0;

static float bcmRnd(int a, int b) {
    unsigned int h = (unsigned int)a * 73856093u ^ (unsigned int)b * 19349663u ^ 0x2545F491u;
    h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
    return (float)(h & 0xFFFF) / 65535.0f;
}

// ---------------------------------------------------------------- piezas comunes
// caja con bisel de cerca, simple de lejos
static void bcmBox(LineVertex *b, int &i, float cx, float y0, float cz, float w, float d, float h, float bv,
                   unsigned int cTop, unsigned int cBot) {
    if (g_bcmLod) gpBox(b, i, cx, y0, cz, w, d, h, cTop, cBot);
    else gpBevelBox(b, i, cx, y0, cz, w, d, h, bv, cTop, cBot);
}
// placa grabada en la cara de adelante (-Z) de una losa: rectangulo oscuro + 3 renglones claros
static void bcmPlaque(LineVertex *b, int &i, float cx, float y0, float z, float w, float h) {
    const unsigned int d = RGBA(70, 68, 64, 255), l = RGBA(108, 104, 98, 255);
    gpQuad(b, i, cx - w * 0.5f, y0, z, d, cx + w * 0.5f, y0, z, d, cx + w * 0.5f, y0 + h, z, d, cx - w * 0.5f, y0 + h, z, d);
    for (int k = 0; k < 3; ++k) {
        const float ly = y0 + h * (0.72f - 0.24f * k), lw = w * (k == 0 ? 0.62f : 0.78f - 0.1f * k);
        gpQuad(b, i, cx - lw * 0.5f, ly, z - 0.004f, l, cx + lw * 0.5f, ly, z - 0.004f, l,
                     cx + lw * 0.5f, ly + h * 0.07f, z - 0.004f, l, cx - lw * 0.5f, ly + h * 0.07f, z - 0.004f, l);
    }
}
// montículo de tierra con pasto delante de la lapida (hacia -Z)
static void bcmMound(LineVertex *b, int &i, float z, float w, float l) {
    gpDome(b, i, 0.0f, 0.0f, z, w * 0.5f, 0.24f, l * 0.5f, 2, 8, BCM_GRASS, cp_lerp(BCM_DIRT, BCM_GRASSD, 0.4f));
}

// musgo solo en el pie de la piedra (un degradado entero a musgo la dejaba verde)
static const unsigned int BCM_FOOT = RGBA(112, 116, 98, 255);

// ---------------------------------------------------------------- MODELOS
// lapida de tope REDONDO, zocalo biselado, placa, cruz en relieve, monticulo. ~370 v
static int bcmLapidaR(LineVertex *b, int cap, int variant) {
    int i = 0; if (cap < 420) return 0;
    const float w = 0.95f + 0.15f * variant, h = 1.55f + 0.25f * variant;
    bcmBox(b, i, 0.0f, 0.0f, 0.0f, w + 0.32f, 0.50f, 0.24f, 0.05f, BCM_STONE2, BCM_MOSS);
    gpArchSlab(b, i, 0.0f, 0.24f, 0.0f, w, h, 0.20f, g_bcmLod ? 5 : 10, 0, BCM_STONE, BCM_FOOT);
    if (g_bcmLod) return i;
    bcmPlaque(b, i, 0.0f, 0.24f + h * 0.18f, -0.104f, w * 0.62f, h * 0.30f);
    {   // cruz en relieve arriba
        const unsigned int c = cp_scale(BCM_STONE, 1.08f); const float z = -0.106f, yc = 0.24f + h * 0.66f;
        gpQuad(b, i, -0.035f, yc - 0.17f, z, c, 0.035f, yc - 0.17f, z, c, 0.035f, yc + 0.17f, z, c, -0.035f, yc + 0.17f, z, c);
        gpQuad(b, i, -0.11f, yc + 0.04f, z, c, 0.11f, yc + 0.04f, z, c, 0.11f, yc + 0.10f, z, c, -0.11f, yc + 0.10f, z, c);
    }
    bcmMound(b, i, -1.25f, 1.10f, 1.9f);
    return i;
}
// lapida OJIVAL (gotica), mas alta y angosta. ~350 v
static int bcmLapidaO(LineVertex *b, int cap) {
    int i = 0; if (cap < 400) return 0;
    bcmBox(b, i, 0.0f, 0.0f, 0.0f, 1.15f, 0.52f, 0.28f, 0.05f, BCM_STONE2, BCM_MOSS);
    gpArchSlab(b, i, 0.0f, 0.28f, 0.0f, 0.86f, 1.95f, 0.22f, g_bcmLod ? 4 : 10, 1, cp_scale(BCM_STONE, 0.94f), BCM_FOOT);
    if (g_bcmLod) return i;
    bcmPlaque(b, i, 0.0f, 0.28f + 0.32f, -0.114f, 0.52f, 0.62f);
    bcmMound(b, i, -1.25f, 1.05f, 1.9f);
    return i;
}
// CRUZ sobre zocalo de dos escalones. ~460 v
static int bcmCruz(LineVertex *b, int cap) {
    int i = 0; if (cap < 470) return 0;
    bcmBox(b, i, 0.0f, 0.0f, 0.0f, 1.10f, 0.80f, 0.22f, 0.05f, BCM_STONE2, BCM_MOSS);
    if (!g_bcmLod) bcmBox(b, i, 0.0f, 0.22f, 0.0f, 0.72f, 0.52f, 0.28f, 0.05f, BCM_STONE2, BCM_STONE2);
    bcmBox(b, i, 0.0f, g_bcmLod ? 0.22f : 0.50f, 0.0f, 0.22f, 0.20f, g_bcmLod ? 2.38f : 2.10f, 0.04f, BCM_STONE, BCM_STONE2);   // fuste
    bcmBox(b, i, 0.0f, 1.96f, 0.0f, 1.00f, 0.20f, 0.22f, 0.04f, BCM_STONE, BCM_STONE);    // brazo
    if (!g_bcmLod) bcmMound(b, i, -1.25f, 1.05f, 1.9f);
    return i;
}
// CRUZ CELTA: cruz alta con la rueda (aro). ~690 v
static int bcmCelta(LineVertex *b, int cap) {
    int i = 0; if (cap < 700) return 0;
    bcmBox(b, i, 0.0f, 0.0f, 0.0f, 1.00f, 0.70f, 0.40f, 0.06f, BCM_STONE2, BCM_MOSS);
    bcmBox(b, i, 0.0f, 0.40f, 0.0f, 0.26f, 0.22f, 2.50f, 0.04f, BCM_STONE, BCM_STONE2);
    bcmBox(b, i, 0.0f, 2.20f, 0.0f, 1.12f, 0.22f, 0.26f, 0.04f, BCM_STONE, BCM_STONE);
    gpRingXY(b, i, 0.0f, 2.33f, 0.0f, 0.36f, 0.08f, 0.16f, g_bcmLod ? 8 : 12, BCM_STONE);
    if (!g_bcmLod) bcmMound(b, i, -1.25f, 1.05f, 1.9f);
    return i;
}
// OBELISCO de cuatro caras con piramidion. ~240 v
static int bcmObelisco(LineVertex *b, int cap) {
    int i = 0; if (cap < 260) return 0;
    bcmBox(b, i, 0.0f, 0.0f, 0.0f, 1.20f, 1.20f, 0.30f, 0.06f, BCM_STONE2, BCM_MOSS);
    bcmBox(b, i, 0.0f, 0.30f, 0.0f, 0.86f, 0.86f, 0.55f, 0.05f, BCM_STONE, BCM_STONE2);
    gpPrism(b, i, 0.0f, 0.85f, 0.0f, 0.36f, 0.25f, 2.90f, 4, 0.7854f, BCM_STONE2, BCM_STONE, 0);
    gpPrism(b, i, 0.0f, 3.75f, 0.0f, 0.25f, 0.0f, 0.42f, 4, 0.7854f, BCM_STONE, BCM_STONE, 0);
    bcmPlaque(b, i, 0.0f, 0.42f, -0.434f, 0.5f, 0.32f);
    return i;
}
// TUMBA CON REJA de hierro alrededor de la losa. ~600 v
static int bcmTumbaReja(LineVertex *b, int cap) {
    int i = 0; if (cap < 640) return 0;
    bcmBox(b, i, 0.0f, 0.0f, -0.95f, 1.30f, 2.20f, 0.30f, 0.06f, BCM_STONE2, BCM_MOSS);   // losa
    gpArchSlab(b, i, 0.0f, 0.0f, 0.12f, 0.80f, 1.15f, 0.16f, g_bcmLod ? 4 : 8, 0, BCM_STONE, BCM_FOOT);     // cabecera
    const float x0 = -0.78f, x1 = 0.78f, z0 = -2.20f, z1 = 0.32f, h = 0.78f;
    const float px[4] = { x0, x1, x1, x0 }, pz[4] = { z0, z0, z1, z1 };
    for (int k = 0; k < 4; ++k) {
        gpPrism(b, i, px[k], 0.0f, pz[k], 0.045f, 0.040f, h, 4, 0.7854f, BCM_IRON, BCM_IRONL, 0);
        const int j = (k + 1) & 3;
        gpTube(b, i, px[k], h, pz[k], px[j], h, pz[j], 0.022f, 0.022f, 4, BCM_IRONL, BCM_IRONL);
    }
    for (int s = 0; s < 2 && !g_bcmLod; ++s)                                                 // barrotes de los lados largos
        for (int k = 1; k <= 5; ++k) {
            const float z = z0 + (z1 - z0) * k / 6.0f, x = s ? x1 : x0;
            gpTube(b, i, x, 0.0f, z, x, h + 0.10f, z, 0.016f, 0.010f, 3, BCM_IRON, BCM_IRONL);
        }
    return i;
}
// ANGEL QUE LLORA: pedestal con molduras, tunica acampanada, cabeza gacha con las manos tapandole
// la cara y dos alas de plumas plegadas que suben por la espalda. ~2400 v
static int bcmAngel(LineVertex *b, int cap) {
    int i = 0; if (cap < 2600) return 0;
    // pedestal: base, dado con placa, cornisa
    bcmBox(b, i, 0.0f, 0.0f, 0.0f, 1.70f, 1.70f, 0.30f, 0.07f, BCM_STONE2, BCM_MOSS);
    bcmBox(b, i, 0.0f, 0.30f, 0.0f, 1.24f, 1.24f, 0.74f, 0.06f, BCM_STONE, BCM_STONE2);
    bcmBox(b, i, 0.0f, 1.04f, 0.0f, 1.46f, 1.46f, 0.19f, 0.06f, BCM_STONE, BCM_STONE);
    bcmPlaque(b, i, 0.0f, 0.44f, -0.624f, 0.70f, 0.44f);
    const float Y = 1.23f;                                        // pies de la figura
    const unsigned int M = BCM_MARBLE, M2 = cp_scale(BCM_MARBLE, 0.82f);
    {   // tunica que se acampana hasta el pedestal (musgo abajo)
        const float ys[4] = { Y, Y + 0.50f, Y + 1.00f, Y + 1.36f };
        const float rx[4] = { 0.50f, 0.41f, 0.31f, 0.23f }, rz[4] = { 0.45f, 0.36f, 0.27f, 0.19f }, oz[4] = { 0, 0, 0, 0 };
        const unsigned int c[4] = { cp_lerp(M2, BCM_MOSS, 0.5f), M2, M, M };
        gpLoftE(b, i, 0.0f, 0.0f, 4, ys, rx, rz, oz, c, g_bcmLod ? 8 : 11, 0.0f, GP_TAU);
    }
    {   // torso, inclinado hacia adelante (llora)
        const float ys[3] = { Y + 1.36f, Y + 1.62f, Y + 1.80f };
        const float rx[3] = { 0.22f, 0.24f, 0.19f }, rz[3] = { 0.17f, 0.17f, 0.14f }, oz[3] = { 0.0f, -0.04f, -0.08f };
        const unsigned int c[3] = { M, M, M2 };
        gpLoftE(b, i, 0.0f, 0.0f, 3, ys, rx, rz, oz, c, g_bcmLod ? 8 : 12, 0.0f, GP_TAU);
    }
    addBall(b, i, 0.0f, Y + 1.95f, -0.16f, 0.125f, 0.145f, 0.135f, g_bcmLod ? 3 : 4, g_bcmLod ? 6 : 8, M);    // cabeza gacha
    addBall(b, i, 0.0f, Y + 2.00f, -0.08f, 0.150f, 0.150f, 0.140f, 3, g_bcmLod ? 6 : 8, M2);                  // pelo
    gpTube(b, i, 0.0f, Y + 1.98f, -0.02f, 0.0f, Y + 1.62f, 0.10f, 0.13f, 0.07f, g_bcmLod ? 5 : 8, M2, M2);  // pelo cayendo
    for (int s = -1; s <= 1 && !g_bcmLod; s += 2) {                                        // brazos a la cara
        const float sx = s * 0.22f;
        gpTube(b, i, sx, Y + 1.74f, -0.06f, s * 0.20f, Y + 1.60f, -0.30f, 0.065f, 0.055f, 8, M, M);
        gpTube(b, i, s * 0.20f, Y + 1.60f, -0.30f, s * 0.07f, Y + 1.92f, -0.29f, 0.055f, 0.045f, 8, M, M);
        addBall(b, i, s * 0.065f, Y + 1.95f, -0.285f, 0.060f, 0.075f, 0.045f, 2, 6, M);    // manos sobre el rostro
    }
    for (int s = -1; s <= 1; s += 2) {                                                     // alas plegadas
        GpBasis B;
        B.o[0] = s * 0.13f; B.o[1] = Y + 1.68f; B.o[2] = 0.12f;
        // la espina sube POCO y va hacia atras: el ala queda plegada contra la espalda (con mas
        // subida parecian dos antenas)
        // (espina hacia ATRAS y algo arriba: asi las plumas, perpendiculares a ella, CAEN hacia el suelo)
        float ex = s * 0.20f, ey = 0.55f, ez = 0.81f; float l = sqrtf(ex * ex + ey * ey + ez * ez); ex /= l; ey /= l; ez /= l;
        float yx = 0.0f, yy = 1.0f, yz = 0.0f; const float d = yx * ex + yy * ey + yz * ez;      // -Y = abajo (y algo atras)
        yx -= d * ex; yy -= d * ey; yz -= d * ez; l = sqrtf(yx * yx + yy * yy + yz * yz); yx /= l; yy /= l; yz /= l;
        B.ex[0] = ex; B.ex[1] = ey; B.ex[2] = ez; B.ey[0] = yx; B.ey[1] = yy; B.ey[2] = yz;
        B.ez[0] = ey * yz - ez * yy; B.ez[1] = ez * yx - ex * yz; B.ez[2] = ex * yy - ey * yx;
        gpWing(b, i, B, 0.13f, M, M2, cp_scale(M, 1.06f), 0.75f, 2.3f, g_bcmLod ? 2 : 1);
    }
    return i;
}
// MAUSOLEO gotico: zocalo, muros, cornisa, techo a dos aguas, portico de dos columnas, puerta
// ojival de hierro con su marco, cruz en el remate y escalones. ~1500 v
static int bcmMausoleo(LineVertex *b, int cap) {
    int i = 0; if (cap < 1700) return 0;
    const float W = 5.6f, D = 4.6f, H = 3.2f;
    bcmBox(b, i, 0.0f, 0.0f, 0.0f, W + 0.8f, D + 0.8f, 0.40f, 0.08f, BCM_STONE2, BCM_MOSS);
    bcmBox(b, i, 0.0f, 0.40f, 0.0f, W, D, H, 0.10f, BCM_STONE, cp_lerp(BCM_STONE2, BCM_MOSS, 0.4f));
    bcmBox(b, i, 0.0f, 0.40f + H, 0.0f, W + 0.4f, D + 0.4f, 0.26f, 0.06f, BCM_STONE, BCM_STONE2);
    {   // techo a dos aguas (cumbrera a lo largo de Z) + hastiales
        const float y0 = 0.66f + H, yr = y0 + 1.7f, hx = W * 0.5f + 0.3f, hz = D * 0.5f + 0.3f;
        const unsigned int rL = gpShade(BCM_STONED, -0.7f, 0.7f, 0.0f), rR = gpShade(BCM_STONED, 0.7f, 0.7f, 0.0f);
        gpQuad(b, i, -hx, y0, -hz, rL, 0.0f, yr, -hz, rL, 0.0f, yr, hz, rL, -hx, y0, hz, rL);
        gpQuad(b, i, hx, y0, -hz, rR, 0.0f, yr, -hz, rR, 0.0f, yr, hz, rR, hx, y0, hz, rR);
        const unsigned int gF = gpShade(BCM_STONE, 0, 0, -1), gB = gpShade(BCM_STONE, 0, 0, 1);
        gpTri(b, i, -hx + 0.2f, y0, -hz + 0.12f, gF, hx - 0.2f, y0, -hz + 0.12f, gF, 0.0f, yr - 0.1f, -hz + 0.12f, gF);
        gpTri(b, i, -hx + 0.2f, y0, hz - 0.12f, gB, hx - 0.2f, y0, hz - 0.12f, gB, 0.0f, yr - 0.1f, hz - 0.12f, gB);
        gpTube(b, i, 0.0f, yr, -hz, 0.0f, yr, hz, 0.10f, 0.10f, 6, BCM_STONE2, BCM_STONE2);     // cumbrera
        bcmBox(b, i, 0.0f, yr - 0.05f, -hz + 0.05f, 0.12f, 0.12f, 1.00f, 0.02f, BCM_STONE, BCM_STONE);   // cruz
        bcmBox(b, i, 0.0f, yr + 0.52f, -hz + 0.05f, 0.56f, 0.12f, 0.12f, 0.02f, BCM_STONE, BCM_STONE);
    }
    const float zf = -D * 0.5f;
    if (g_bcmLod) {   // de lejos: solo la puerta (el portico y los escalones no se distinguen)
        gpArchSlab(b, i, 0.0f, 0.40f, zf - 0.04f, 1.40f, 2.50f, 0.06f, 4, 1, BCM_IRONL, BCM_IRON);
        return i;
    }
    for (int s = -1; s <= 1; s += 2) {                                                     // portico
        const float x = s * 1.55f, z = zf - 0.32f;
        bcmBox(b, i, x, 0.40f, z, 0.56f, 0.56f, 0.24f, 0.05f, BCM_STONE2, BCM_STONE2);
        addLimb(b, i, x, 0.64f, z, x, 0.40f + H - 0.26f, z, 0.20f, 0.17f, g_bcmLod ? 6 : 10, BCM_STONE2, BCM_STONE);
        bcmBox(b, i, x, 0.40f + H - 0.30f, z, 0.54f, 0.54f, 0.30f, 0.05f, BCM_STONE, BCM_STONE);
    }
    bcmBox(b, i, 0.0f, 0.40f + H - 0.02f, zf - 0.30f, W * 0.82f, 0.80f, 0.28f, 0.05f, BCM_STONE, BCM_STONE2);   // dintel del portico
    if (!g_bcmLod) gpArchSlab(b, i, 0.0f, 0.40f, zf - 0.02f, 1.90f, 2.85f, 0.16f, 10, 1, BCM_STONE, BCM_STONE2);   // marco
    gpArchSlab(b, i, 0.0f, 0.40f, zf - 0.12f, 1.40f, 2.50f, 0.06f, g_bcmLod ? 4 : 10, 1, BCM_IRONL, BCM_IRON);     // puerta de hierro
    for (int k = 0; k < 2; ++k)                                                            // escalones
        bcmBox(b, i, 0.0f, 0.0f, zf - 0.62f - 0.42f * k, 2.6f + 0.5f * k, 0.48f, 0.40f - 0.18f * k, 0.05f, BCM_STONE2, BCM_MOSS);
    return i;
}
// CIPRES: alto y angosto, follaje con bultos (radio con ruido por anillo y lado). ~520 v
static int bcmCipres(LineVertex *b, int cap, int seed) {
    int i = 0; if (cap < 560) return 0;
    const float H = 8.6f + 1.6f * bcmRnd(seed, 1);
    gpTube(b, i, 0.0f, 0.0f, 0.0f, 0.0f, 1.1f, 0.0f, 0.26f, 0.20f, 6, BCM_BARK, BCM_BARK);
    const int R = g_bcmLod ? 6 : 8, S = g_bcmLod ? 5 : 9;
    const float ykH[8] = { 0.8f, 1.5f, 2.6f, 3.9f, 5.2f, 6.5f, 7.6f, 8.6f }, rkH[8] = { 0.30f, 0.86f, 1.12f, 1.10f, 0.98f, 0.78f, 0.48f, 0.06f };
    const float ykL[8] = { 0.8f, 1.6f, 3.2f, 5.2f, 7.2f, 8.6f, 0, 0 }, rkL[8] = { 0.30f, 0.92f, 1.12f, 0.98f, 0.62f, 0.06f, 0, 0 };
    const float *yk = g_bcmLod ? ykL : ykH, *rk = g_bcmLod ? rkL : rkH;
    float px[8][9], py[8][9], pz[8][9];
    for (int r = 0; r < R; ++r)
        for (int s = 0; s < S; ++s) {
            const float a = GP_TAU * s / S + r * 0.35f, n = 0.80f + 0.36f * bcmRnd(seed * 31 + r, s);
            px[r][s] = cosf(a) * rk[r] * n; pz[r][s] = sinf(a) * rk[r] * n; py[r][s] = yk[r] * H / 8.6f;
        }
    for (int r = 0; r + 1 < R; ++r)
        for (int s = 0; s < S; ++s) {
            const int t = (s + 1) % S;
            const unsigned int c0 = cp_lerp(BCM_CYP, BCM_CYP2, (float)r / R), c1 = cp_lerp(BCM_CYP, BCM_CYP2, (float)(r + 1) / R);
            const float a0 = GP_TAU * s / S, a1 = GP_TAU * t / S;
            gpQuad(b, i, px[r][s], py[r][s], pz[r][s], gpShade(c0, cosf(a0), 0.3f, sinf(a0)), px[r][t], py[r][t], pz[r][t], gpShade(c0, cosf(a1), 0.3f, sinf(a1)),
                         px[r + 1][t], py[r + 1][t], pz[r + 1][t], gpShade(c1, cosf(a1), 0.3f, sinf(a1)), px[r + 1][s], py[r + 1][s], pz[r + 1][s], gpShade(c1, cosf(a0), 0.3f, sinf(a0)));
        }
    return i;
}
// ARBOL SECO retorcido: tronco, 3 ramas grandes, 2 ramas por cada una y ramitas finas. ~600 v
static void bcmBranch(LineVertex *b, int &i, int cap, float x, float y, float z, float dx, float dy, float dz,
                      float len, float r, int depth, int seed) {
    if (i + 60 > cap) return;
    const float x1 = x + dx * len, y1 = y + dy * len, z1 = z + dz * len;
    if (g_bcmLod && depth >= 2) return;   // de lejos, sin ramitas
    if (depth >= 3) {   // ramita: dos triangulos cruzados finisimos
        gpTri(b, i, x - r, y, z, BCM_BARK, x + r, y, z, BCM_BARK, x1, y1, z1, cp_scale(BCM_BARK, 1.2f));
        gpTri(b, i, x, y, z - r, BCM_BARK, x, y, z + r, BCM_BARK, x1, y1, z1, cp_scale(BCM_BARK, 1.2f));
        return;
    }
    gpTube(b, i, x, y, z, x1, y1, z1, r, r * 0.62f, depth == 0 ? 7 : (depth == 1 ? 6 : 4), BCM_BARK, cp_scale(BCM_BARK, 1.15f));
    const int kids = (depth == 0) ? 3 : 2;
    for (int k = 0; k < kids; ++k) {
        const float a = GP_TAU * (k + bcmRnd(seed, k * 7 + depth)) / kids, sp = 0.55f + 0.35f * bcmRnd(seed, k + 11 * depth);
        float ndx = dx * (1.0f - sp) + cosf(a) * sp, ndy = dy * 0.75f + 0.30f, ndz = dz * (1.0f - sp) + sinf(a) * sp;
        const float l = sqrtf(ndx * ndx + ndy * ndy + ndz * ndz); ndx /= l; ndy /= l; ndz /= l;
        bcmBranch(b, i, cap, x1, y1, z1, ndx, ndy, ndz, len * (0.62f + 0.12f * bcmRnd(seed, k + 5)), r * 0.58f, depth + 1, seed * 3 + k + 1);
    }
}
static int bcmArbol(LineVertex *b, int cap, int seed) {
    int i = 0;
    gpTube(b, i, 0.0f, 0.0f, 0.0f, 0.15f, 0.6f, 0.05f, 0.46f, 0.32f, 7, cp_scale(BCM_BARK, 0.8f), BCM_BARK);   // raiz ancha
    bcmBranch(b, i, cap, 0.15f, 0.6f, 0.05f, 0.08f, 0.99f, 0.05f, 3.0f, 0.32f, 0, seed);
    return i;
}
// FAROL de hierro: zocalo, poste, jaula con vidrio calido y techito. ~300 v
static int bcmFarol(LineVertex *b, int cap) {
    int i = 0; if (cap < 320) return 0;
    bcmBox(b, i, 0.0f, 0.0f, 0.0f, 0.50f, 0.50f, 0.30f, 0.05f, BCM_STONE2, BCM_MOSS);
    gpPrism(b, i, 0.0f, 0.30f, 0.0f, 0.085f, 0.065f, 2.60f, 6, 0.0f, BCM_IRON, BCM_IRONL, 0);
    gpPrism(b, i, 0.0f, 2.90f, 0.0f, 0.13f, 0.21f, 0.10f, 4, 0.7854f, BCM_IRON, BCM_IRON, 0);
    gpPrism(b, i, 0.0f, 3.00f, 0.0f, 0.17f, 0.17f, 0.44f, 4, 0.7854f, BCM_GLASS, BCM_GLASS, 0);     // vidrio
    for (int k = 0; k < 4 && !g_bcmLod; ++k) {                                                    // aristas de la jaula
        const float a = 0.7854f + k * 1.5708f, x = cosf(a) * 0.19f, z = sinf(a) * 0.19f;
        gpTube(b, i, x, 3.0f, z, x, 3.44f, z, 0.016f, 0.016f, 3, BCM_IRON, BCM_IRON);
    }
    gpPrism(b, i, 0.0f, 3.44f, 0.0f, 0.27f, 0.0f, 0.26f, 4, 0.7854f, BCM_IRON, BCM_IRONL, 0);        // techito
    if (!g_bcmLod) addBall(b, i, 0.0f, 3.74f, 0.0f, 0.05f, 0.05f, 0.05f, 2, 6, BCM_IRONL);
    return i;
}
// BANCO de piedra. ~270 v
static int bcmBanco(LineVertex *b, int cap) {
    int i = 0; if (cap < 280) return 0;
    for (int s = -1; s <= 1; s += 2) bcmBox(b, i, s * 0.75f, 0.0f, 0.0f, 0.28f, 0.60f, 0.46f, 0.04f, BCM_STONE2, BCM_MOSS);
    bcmBox(b, i, 0.0f, 0.46f, 0.0f, 2.10f, 0.74f, 0.14f, 0.05f, BCM_STONE, BCM_STONE2);
    return i;
}
// LA FIGURA DE LA NIEBLA: una mujer alta de vestido largo, el pelo cayendole sobre la cara. ~700 v
static int bcmFantasma(LineVertex *b, int cap) {
    int i = 0; if (cap < 760) return 0;
    const unsigned int D = BCM_DARK, D2 = cp_scale(BCM_DARK, 1.6f);
    {
        const float ys[5] = { 0.0f, 0.80f, 1.60f, 2.20f, 2.62f };
        const float rx[5] = { 0.56f, 0.42f, 0.30f, 0.23f, 0.26f }, rz[5] = { 0.50f, 0.38f, 0.26f, 0.17f, 0.19f }, oz[5] = { 0, 0, 0, 0, 0 };
        const unsigned int c[5] = { D, D, D2, D2, D2 };
        gpLoftE(b, i, 0.0f, 0.0f, 5, ys, rx, rz, oz, c, 10, 0.0f, GP_TAU);
    }
    gpTube(b, i, 0.0f, 2.62f, 0.0f, 0.0f, 2.86f, -0.02f, 0.07f, 0.06f, 6, D2, D2);
    addBall(b, i, 0.0f, 3.02f, -0.02f, 0.14f, 0.17f, 0.15f, 4, 8, D2);
    gpTube(b, i, 0.0f, 3.10f, -0.06f, 0.0f, 2.50f, -0.16f, 0.15f, 0.08f, 8, D, D);            // pelo sobre la cara
    gpTube(b, i, 0.0f, 3.12f, 0.04f, 0.0f, 2.20f, 0.18f, 0.17f, 0.06f, 8, D, D);              // pelo por la espalda
    for (int s = -1; s <= 1; s += 2)                                                         // brazos caidos
        gpTube(b, i, s * 0.24f, 2.58f, 0.0f, s * 0.30f, 1.55f, -0.04f, 0.055f, 0.035f, 6, D2, D);
    return i;
}
// REJA de hierro (panel de 4): dos postes con remate de lanza, dos travesanos, 9 barrotes con punta. ~440 v
static int bcmReja(LineVertex *b, int cap) {
    int i = 0; if (cap < 460) return 0;
    for (int s = -1; s <= 1; s += 2) {
        gpPrism(b, i, s * 2.0f, 0.0f, 0.0f, 0.075f, 0.065f, 2.10f, 4, 0.7854f, BCM_IRON, BCM_IRONL, 0);
        gpPrism(b, i, s * 2.0f, 2.10f, 0.0f, 0.10f, 0.0f, 0.30f, 4, 0.7854f, BCM_IRONL, BCM_IRONL, 0);
    }
    gpTube(b, i, -2.0f, 0.45f, 0.0f, 2.0f, 0.45f, 0.0f, 0.025f, 0.025f, 3, BCM_IRON, BCM_IRON);
    gpTube(b, i, -2.0f, 1.75f, 0.0f, 2.0f, 1.75f, 0.0f, 0.025f, 0.025f, 3, BCM_IRON, BCM_IRON);
    for (int k = 1; k <= 9; k += g_bcmLod ? 2 : 1) {
        const float x = -2.0f + 4.0f * k / 10.0f;
        gpTube(b, i, x, 0.0f, 0.0f, x, 1.95f, 0.0f, 0.018f, 0.016f, 3, BCM_IRON, BCM_IRONL);
        if (!g_bcmLod) gpPrism(b, i, x, 1.95f, 0.0f, 0.045f, 0.0f, 0.17f, 3, 0.0f, BCM_IRONL, BCM_IRONL, 0);
    }
    return i;
}
// PORTON de entrada: dos pilares con capitel y bola, arco de hierro con su cruz. ~900 v (centrado en x=0)
static int bcmPorton(LineVertex *b, int cap) {
    int i = 0; if (cap < 950) return 0;
    for (int s = -1; s <= 1; s += 2) {
        const float x = s * 3.3f;
        bcmBox(b, i, x, 0.0f, 0.0f, 1.20f, 1.20f, 0.40f, 0.06f, BCM_STONE2, BCM_MOSS);
        bcmBox(b, i, x, 0.40f, 0.0f, 0.95f, 0.95f, 3.00f, 0.08f, BCM_STONE, BCM_STONE2);
        bcmBox(b, i, x, 3.40f, 0.0f, 1.20f, 1.20f, 0.24f, 0.06f, BCM_STONE, BCM_STONE);
        addBall(b, i, x, 3.94f, 0.0f, 0.34f, 0.32f, 0.34f, 4, 10, BCM_STONE);
    }
    const int N = 12; float px = -2.85f, py = 3.2f;                                          // arco de hierro
    for (int k = 1; k <= N; ++k) {
        const float a = GP_PI * k / N, x = -2.85f * cosf(a), y = 3.2f + 1.3f * sinf(a);
        gpTube(b, i, px, py, 0.0f, x, y, 0.0f, 0.05f, 0.05f, 4, BCM_IRON, BCM_IRONL);
        if (k < N && (k & 1)) gpTube(b, i, x, y, 0.0f, x * 0.92f, 3.2f, 0.0f, 0.02f, 0.02f, 3, BCM_IRON, BCM_IRON);
        px = x; py = y;
    }
    gpTube(b, i, -2.85f, 3.2f, 0.0f, 2.85f, 3.2f, 0.0f, 0.04f, 0.04f, 4, BCM_IRON, BCM_IRON);
    gpTube(b, i, 0.0f, 4.5f, 0.0f, 0.0f, 5.3f, 0.0f, 0.04f, 0.04f, 4, BCM_IRONL, BCM_IRONL);   // cruz
    gpTube(b, i, -0.22f, 5.05f, 0.0f, 0.22f, 5.05f, 0.0f, 0.035f, 0.035f, 4, BCM_IRONL, BCM_IRONL);
    return i;
}
// LA CAPILLA del fondo: zocalo, nave con contrafuertes, ventanas ojivales, techo empinado,
// fachada con rosetón, portal ojival con escalinata y una espadana con su campana. ~3000 v
static int bcmCapilla(LineVertex *b, int cap) {
    int i = 0; if (cap < 3200) return 0;
    const float W = 12.0f, D = 14.0f, H = 6.4f, zf = -D * 0.5f;
    const unsigned int S = BCM_STONE, S2 = BCM_STONE2, SM = cp_lerp(BCM_STONE2, BCM_MOSS, 0.5f);
    bcmBox(b, i, 0.0f, 0.0f, 0.0f, W + 1.2f, D + 1.2f, 0.6f, 0.1f, S2, BCM_MOSS);
    bcmBox(b, i, 0.0f, 0.6f, 0.0f, W, D, H, 0.14f, S, SM);
    bcmBox(b, i, 0.0f, 0.6f + H, 0.0f, W + 0.6f, D + 0.6f, 0.36f, 0.08f, S, S2);          // cornisa
    for (int s = -1; s <= 1 && !g_bcmLod; s += 2)                                            // contrafuertes y ventanas
        for (int k = 0; k < 3; ++k) {
            const float z = zf + 2.5f + k * 4.5f;
            bcmBox(b, i, s * (W * 0.5f + 0.35f), 0.6f, z, 0.9f, 1.1f, H - 0.8f, 0.1f, S2, SM);
            gpPrism(b, i, s * (W * 0.5f + 0.35f), 0.6f + H - 0.8f, z, 0.62f, 0.0f, 1.0f, 4, 0.7854f, S2, S2, 0);
            if (k < 2 && !g_bcmLod) {   // ventana ojival oscura entre contrafuertes (un poco afuera del muro)
                const int i0 = i;
                gpArchSlab(b, i, 0.0f, 2.0f, 0.0f, 1.3f, 3.4f, 0.10f, 8, 1, RGBA(40, 44, 52, 255), RGBA(28, 30, 36, 255));
                for (int q = i0; q < i; ++q) {   // girarla 90 grados para que mire al costado
                    const float lx = b[q].x, lz = b[q].z;
                    b[q].x = s * (W * 0.5f + 0.02f) - s * lz; b[q].z = z + 2.25f + lx;
                }
            }
        }
    {   // techo empinado (cumbrera a lo largo de Z)
        const float y0 = 0.96f + H, yr = y0 + 5.2f, hx = W * 0.5f + 0.5f, hz = D * 0.5f + 0.4f;
        const unsigned int rL = gpShade(BCM_STONED, -0.8f, 0.6f, 0.0f), rR = gpShade(BCM_STONED, 0.8f, 0.6f, 0.0f);
        gpQuad(b, i, -hx, y0, -hz, rL, 0.0f, yr, -hz, rL, 0.0f, yr, hz, rL, -hx, y0, hz, rL);
        gpQuad(b, i, hx, y0, -hz, rR, 0.0f, yr, -hz, rR, 0.0f, yr, hz, rR, hx, y0, hz, rR);
        const unsigned int gF = gpShade(S, 0, 0, -1), gB = gpShade(S, 0, 0, 1);
        gpTri(b, i, -hx + 0.4f, y0, zf - 0.05f, gF, hx - 0.4f, y0, zf - 0.05f, gF, 0.0f, yr - 0.2f, zf - 0.05f, gF);
        gpTri(b, i, -hx + 0.4f, y0, -zf + 0.05f, gB, hx - 0.4f, y0, -zf + 0.05f, gB, 0.0f, yr - 0.2f, -zf + 0.05f, gB);
        gpTube(b, i, 0.0f, yr, -hz, 0.0f, yr, hz, 0.16f, 0.16f, 6, S2, S2);
        // ESPADANA sobre la fachada: dos pilares, arco y la campana
        const float ey = yr - 0.6f, ez = zf - 0.2f;
        if (g_bcmLod) { bcmBox(b, i, 0.0f, ey, ez, 2.2f, 0.6f, 2.6f, 0.05f, S, S2); gpPrism(b, i, 0.0f, ey + 2.6f, ez, 0.7f, 0.0f, 0.9f, 4, 0.7854f, S2, S2, 0); }
        else {
        for (int s = -1; s <= 1; s += 2) bcmBox(b, i, s * 0.85f, ey, ez, 0.42f, 0.6f, 2.3f, 0.05f, S, S2);
        bcmBox(b, i, 0.0f, ey + 2.3f, ez, 2.2f, 0.7f, 0.34f, 0.05f, S, S2);
        gpPrism(b, i, 0.0f, ey + 2.64f, ez, 0.7f, 0.0f, 0.9f, 4, 0.7854f, S2, S2, 0);
        gpTube(b, i, 0.0f, ey + 3.5f, ez, 0.0f, ey + 4.3f, ez, 0.06f, 0.06f, 4, BCM_IRONL, BCM_IRONL);   // cruz
        gpTube(b, i, -0.3f, ey + 4.0f, ez, 0.3f, ey + 4.0f, ez, 0.05f, 0.05f, 4, BCM_IRONL, BCM_IRONL);
        const unsigned int BR = RGBA(110, 92, 60, 255);
        gpTube(b, i, 0.0f, ey + 2.2f, ez, 0.0f, ey + 1.4f, ez, 0.12f, 0.42f, 10, BR, cp_scale(BR, 0.7f));   // campana
        addBall(b, i, 0.0f, ey + 2.2f, ez, 0.14f, 0.10f, 0.14f, 2, 8, BR);
        }
    }
    // FACHADA: roseton, portal ojival con marco escalonado, escalinata
    gpRingXY(b, i, 0.0f, 0.6f + H + 2.1f, zf - 0.08f, 1.35f, 0.26f, 0.34f, g_bcmLod ? 10 : 16, S);
    if (!g_bcmLod) {
        const float yc = 0.6f + H + 2.1f, z = zf - 0.06f; const unsigned int g0 = RGBA(70, 30, 40, 255), g1 = RGBA(40, 44, 70, 255);
        for (int k = 0; k < 12; ++k) {   // vidrio oscuro en gajos de dos colores
            const float a0 = GP_TAU * k / 12, a1 = GP_TAU * (k + 1) / 12;
            gpTri(b, i, 0.0f, yc, z, RGBA(60, 54, 50, 255), cosf(a0) * 1.22f, yc + sinf(a0) * 1.22f, z, (k & 1) ? g0 : g1, cosf(a1) * 1.22f, yc + sinf(a1) * 1.22f, z, (k & 1) ? g0 : g1);
        }
    }
    gpArchSlab(b, i, 0.0f, 0.6f, zf - 0.10f, 3.6f, 5.0f, 0.30f, 12, 1, S, S2);                // marco exterior
    if (!g_bcmLod) gpArchSlab(b, i, 0.0f, 0.6f, zf - 0.30f, 2.9f, 4.5f, 0.14f, 12, 1, S2, BCM_STONED);       // marco interior
    gpArchSlab(b, i, 0.0f, 0.6f, zf - 0.40f, 2.3f, 4.0f, 0.06f, 12, 1, RGBA(46, 34, 30, 255), RGBA(30, 22, 20, 255));   // puerta
    for (int k = 0; k < 3 && !g_bcmLod; ++k)
        bcmBox(b, i, 0.0f, 0.0f, zf - 0.9f - 0.55f * k, 5.0f + 0.8f * k, 0.6f, 0.6f - 0.2f * k, 0.06f, S2, BCM_MOSS);
    return i;
}

// ---------------------------------------------------------------- catalogo de modelos
enum { BCM_LAPIDA_R0, BCM_LAPIDA_R1, BCM_LAPIDA_O, BCM_CRUZ, BCM_CELTA, BCM_OBELISCO, BCM_TUMBA_REJA,
       BCM_ANGEL, BCM_MAUSOLEO, BCM_CIPRES0, BCM_CIPRES1, BCM_ARBOL0, BCM_ARBOL1, BCM_FAROL, BCM_BANCO,
       BCM_FANTASMA, BCM_REJA, BCM_PORTON, BCM_CAPILLA, BCM_N };
struct BcmModel { int off, n; float rad, cw, cd, ch; int loOff, loN; };   // radio, caja de colision, version de lejos
static BcmModel g_bcm[BCM_N];
// los que tienen version de LEJOS (las demas se ven igual o son unicos y chicos)
static const unsigned char kBcmHasLo[BCM_N] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1 };
static const char *kBcmName[BCM_N] = { "lapida_r0", "lapida_r1", "lapida_o", "cruz", "celta", "obelisco", "tumba_reja",
                                       "angel", "mausoleo", "cipres0", "cipres1", "arbol0", "arbol1", "farol", "banco",
                                       "fantasma", "reja", "porton", "capilla" };
// colision (ancho, fondo, alto) en coordenadas del modelo (antes del yaw: las tumbas giran 180, es simetrico)
static const float kBcmCol[BCM_N][3] = {
    { 1.3f, 0.5f, 1.9f }, { 1.45f, 0.5f, 2.1f }, { 1.2f, 0.5f, 2.2f }, { 1.1f, 0.8f, 2.6f }, { 1.0f, 0.7f, 2.8f }, { 1.2f, 1.2f, 4.0f },
    { 1.6f, 2.6f, 1.0f }, { 1.7f, 1.7f, 3.4f }, { 6.4f, 5.4f, 5.0f }, { 1.8f, 1.8f, 9.0f }, { 1.8f, 1.8f, 9.0f },
    { 0.9f, 0.9f, 4.0f }, { 0.9f, 0.9f, 4.0f }, { 0.5f, 0.5f, 3.5f }, { 2.1f, 0.74f, 0.6f }, { 0.0f, 0.0f, 0.0f },
    { 4.0f, 0.3f, 2.2f }, { 0.0f, 0.0f, 0.0f }, { 13.2f, 15.2f, 7.0f } };
static int bcmBuildOne(int m, LineVertex *b, int cap) {
    switch (m) {
        case BCM_LAPIDA_R0: return bcmLapidaR(b, cap, 0);
        case BCM_LAPIDA_R1: return bcmLapidaR(b, cap, 1);
        case BCM_LAPIDA_O:  return bcmLapidaO(b, cap);
        case BCM_CRUZ:      return bcmCruz(b, cap);
        case BCM_CELTA:     return bcmCelta(b, cap);
        case BCM_OBELISCO:  return bcmObelisco(b, cap);
        case BCM_TUMBA_REJA:return bcmTumbaReja(b, cap);
        case BCM_ANGEL:     return bcmAngel(b, cap);
        case BCM_MAUSOLEO:  return bcmMausoleo(b, cap);
        case BCM_CIPRES0:   return bcmCipres(b, cap, 3);
        case BCM_CIPRES1:   return bcmCipres(b, cap, 8);
        case BCM_ARBOL0:    return bcmArbol(b, cap, 5);
        case BCM_ARBOL1:    return bcmArbol(b, cap, 12);
        case BCM_FAROL:     return bcmFarol(b, cap);
        case BCM_BANCO:     return bcmBanco(b, cap);
        case BCM_FANTASMA:  return bcmFantasma(b, cap);
        case BCM_REJA:      return bcmReja(b, cap);
        case BCM_PORTON:    return bcmPorton(b, cap);
        case BCM_CAPILLA:   return bcmCapilla(b, cap);
    }
    return 0;
}
// arma TODOS los modelos uno detras de otro en pool; devuelve los vertices usados (o -1 si no entra)
static int bcmBuildAll(LineVertex *pool, int cap) {
    int at = 0;
    for (int m = 0; m < BCM_N; ++m) {
        const int n = bcmBuildOne(m, pool + at, cap - at);
        if (n <= 0) return -1;
        float r2 = 0.0f;
        for (int k = 0; k < n; ++k) { const LineVertex &v = pool[at + k]; const float d = v.x * v.x + v.y * v.y + v.z * v.z; if (d > r2) r2 = d; }
        g_bcm[m] = { at, n, sqrtf(r2), kBcmCol[m][0], kBcmCol[m][1], kBcmCol[m][2], at, n };
        at += n;
        if (kBcmHasLo[m]) {
            g_bcmLod = 1; const int nl = bcmBuildOne(m, pool + at, cap - at); g_bcmLod = 0;
            if (nl <= 0) return -1;
            g_bcm[m].loOff = at; g_bcm[m].loN = nl; at += nl;
        }
    }
    return at;
}
static int bcmBuildByName(const char *name, LineVertex *b, int cap) {
    for (int m = 0; m < BCM_N; ++m) { const char *a = kBcmName[m], *c = name; while (*a && *a == *c) { ++a; ++c; } if (!*a && !*c) return bcmBuildOne(m, b, cap); }
    return 0;
}

// ---------------------------------------------------------------- DISPOSICION
// Planta (x derecha, z hacia la entrada; se llega por el SUR mirando al NORTE, -Z):
//   porton en z=30.5, camino central x=-2..2 hasta la capilla (fachada en z=-43), camino
//   transversal en z=-12, cuatro bloques de tumbas, mausoleos a los costados, cipreses junto al
//   muro, faroles por el camino, angeles en el cruce y frente a la capilla.
static const float BC_SPAWN_Z = 26.0f;
static const float BC_CHAPEL_Z = -50.0f;          // centro de la capilla (fachada en -57+... ver bcmCapilla: D=14)
static const float BC_HALF_X = 32.0f, BC_NORTH_Z = -62.0f, BC_SOUTH_Z = 31.0f;
static const float BC_ELEV_X = 11.0f, BC_ELEV_Z = -38.5f;   // el elevador sube aqui (a la derecha de la capilla)
static const float BC_GHOST_X = 17.0f, BC_GHOST_Z = -3.0f;  // la figura, entre las tumbas del este
static const float BC_TURN_X = 5.6f, BC_TURN_Z = -8.2f;     // el angel que se da vuelta (esquina del cruce)

struct BcmInst { unsigned char m; float x, z, yaw, s; };
static int bcmLayout(BcmInst *out, int cap) {
    int n = 0;
    auto add = [&](int m, float x, float z, float yaw, float s) { if (n < cap) out[n++] = { (unsigned char)m, x, z, yaw, s }; };
    const float PI_ = GP_PI;
    // bloques de tumbas: 4 bloques, filas cada 6, columnas cada 4.6 (tumbas miran al sur: yaw PI)
    const float rowsS[5] = { 20.0f, 14.0f, 8.0f, 2.0f, -4.5f }, rowsN[3] = { -20.0f, -26.0f, -32.0f };
    const int kinds[10] = { BCM_LAPIDA_R0, BCM_LAPIDA_O, BCM_CRUZ, BCM_LAPIDA_R1, BCM_LAPIDA_R0, BCM_TUMBA_REJA,
                            BCM_LAPIDA_O, BCM_CRUZ, BCM_OBELISCO, BCM_LAPIDA_R1 };
    for (int side = -1; side <= 1; side += 2)
        for (int blk = 0; blk < 2; ++blk) {
            const int nr = blk ? 3 : 5;
            for (int r = 0; r < nr; ++r)
                for (int c = 0; c < 5; ++c) {
                    const float x = side * (7.4f + c * 4.6f), z = blk ? rowsN[r] : rowsS[r];
                    const int seed = (side + 2) * 1000 + blk * 100 + r * 10 + c;
                    if (bcmRnd(seed, 1) < 0.16f) continue;                                 // huecos: no es una grilla perfecta
                    if (blk && c >= 3 && r >= 1) continue;                                  // lugar del mausoleo
                    if (!blk && side > 0 && fabsf(x - BC_GHOST_X) < 2.5f && fabsf(z - BC_GHOST_Z) < 2.6f) continue;   // donde esta la figura
                    int m = kinds[(int)(bcmRnd(seed, 2) * 9.99f)];
                    if (bcmRnd(seed, 5) < 0.06f) m = BCM_CELTA;
                    const float jx = (bcmRnd(seed, 3) - 0.5f) * 0.7f, jz = (bcmRnd(seed, 4) - 0.5f) * 0.5f;
                    const float yaw = PI_ + (bcmRnd(seed, 6) - 0.5f) * 0.16f;                // algunas un poco torcidas
                    add(m, x + jx, z + jz, yaw, 0.95f + 0.12f * bcmRnd(seed, 7));
                }
        }
    // mausoleos a los costados (miran al camino transversal: al sur)
    add(BCM_MAUSOLEO, -23.5f, -29.0f, PI_, 1.0f);
    add(BCM_MAUSOLEO, 23.5f, -29.0f, PI_, 1.0f);
    // angeles: en las esquinas del cruce y a los lados de la escalinata de la capilla
    add(BCM_ANGEL, -5.6f, -8.2f, PI_ - 0.35f, 1.0f);
    add(BCM_ANGEL, BC_TURN_X, BC_TURN_Z, 0.0f, 1.0f);                                       // (este gira: el modulo le pisa el yaw)
    add(BCM_ANGEL, -4.6f, -38.0f, PI_ + 0.25f, 1.0f);
    add(BCM_ANGEL, 4.6f, -38.0f, PI_ - 0.25f, 1.0f);
    // cipreses contra el muro y a los lados de la capilla
    for (int k = 0; k < 7; ++k) {
        const float z = 24.0f - k * 12.5f;
        add((k & 1) ? BCM_CIPRES1 : BCM_CIPRES0, -29.5f, z + (k & 1) * 2.0f, k * 0.7f, 0.95f + 0.1f * (k % 3));
        add((k & 1) ? BCM_CIPRES0 : BCM_CIPRES1, 29.5f, z - (k & 1) * 2.0f, k * 1.1f, 1.0f + 0.08f * (k % 2));
    }
    add(BCM_CIPRES0, -10.5f, -46.0f, 0.3f, 1.15f); add(BCM_CIPRES1, 10.5f, -52.0f, 1.3f, 1.1f);
    add(BCM_CIPRES1, -10.5f, -54.0f, 2.1f, 1.05f);
    // arboles secos entre las tumbas
    add(BCM_ARBOL0, -13.0f, 11.0f, 0.4f, 1.0f); add(BCM_ARBOL1, 21.5f, 17.0f, 2.0f, 1.1f);
    add(BCM_ARBOL1, -19.0f, -17.0f, 1.0f, 0.95f); add(BCM_ARBOL0, 17.5f, -38.5f, 3.0f, 1.05f);
    add(BCM_ARBOL0, 26.0f, -8.0f, 5.0f, 0.9f);
    // faroles por el camino, alternados
    const float lz[6] = { 21.0f, 9.0f, -3.0f, -21.0f, -31.0f, -15.5f };
    for (int k = 0; k < 6; ++k) add(BCM_FAROL, (k & 1) ? 2.9f : -2.9f, lz[k], 0.0f, 1.0f);
    // bancos junto al cruce
    add(BCM_BANCO, -10.0f, -15.6f, 0.0f, 1.0f); add(BCM_BANCO, 10.0f, -8.4f, PI_, 1.0f);
    // la figura (siempre la ultima tumba de la lista no: el modulo la busca por modelo)
    add(BCM_FANTASMA, BC_GHOST_X, BC_GHOST_Z, PI_ + 0.5f, 1.0f);
    // reja sobre el muro sur, a los lados del porton, y el porton
    for (int k = 0; k < 4; ++k) { const float x = 6.2f + k * 4.1f; add(BCM_REJA, -x, BC_SOUTH_Z - 0.3f, 0.0f, 1.0f); add(BCM_REJA, x, BC_SOUTH_Z - 0.3f, 0.0f, 1.0f); }
    add(BCM_PORTON, 0.0f, BC_SOUTH_Z - 0.3f, 0.0f, 1.0f);
    add(BCM_CAPILLA, 0.0f, BC_CHAPEL_Z, PI_, 1.0f);              // la fachada mira al sur: al que llega
    return n;
}

// ---------------------------------------------------------------- suelo, caminos, pasto y muro
// (geometria fija del piso: se dibuja de una vez con niebla)
static int bcmBuildGround(LineVertex *b, int cap) {
    int i = 0;
    // pasto: celdas de 6 con color por vertice (manchas de musgo y barro)
    const float X0 = -38.0f, Z0 = -66.0f, CS = 6.0f; const int NX = 13, NZ = 18;
    auto gc = [&](int a, int c) {
        const float t = bcmRnd(a * 7 + 1, c * 13 + 2), m = bcmRnd(a + 40, c + 77);
        unsigned int col = cp_lerp(BCM_GRASSD, BCM_GRASS, t);
        if (m > 0.78f) col = cp_lerp(col, BCM_DIRT, 0.45f);
        return col;
    };
    for (int gz = 0; gz < NZ; ++gz)
        for (int gx = 0; gx < NX && i + 6 <= cap; ++gx) {
            const float x0 = X0 + gx * CS, z0 = Z0 + gz * CS;
            gpQuad(b, i, x0, 0.0f, z0, gc(gx, gz), x0 + CS, 0.0f, z0, gc(gx + 1, gz), x0 + CS, 0.0f, z0 + CS, gc(gx + 1, gz + 1), x0, 0.0f, z0 + CS, gc(gx, gz + 1));
        }
    // caminos de grava (algo por encima del pasto) con cordon de piedra a los lados
    auto path = [&](float ax, float az, float bx, float bz, float w, int seg) {
        const float dx = bx - ax, dz = bz - az, l = sqrtf(dx * dx + dz * dz), nx = -dz / l * w * 0.5f, nz = dx / l * w * 0.5f;
        for (int k = 0; k < seg && i + 30 <= cap; ++k) {
            const float t0 = (float)k / seg, t1 = (float)(k + 1) / seg;
            const float x0 = ax + dx * t0, z0 = az + dz * t0, x1 = ax + dx * t1, z1 = az + dz * t1;
            const unsigned int g0 = cp_scale(BCM_GRAVEL, 0.9f + 0.18f * bcmRnd(k, (int)az)), g1 = cp_scale(BCM_GRAVEL, 0.9f + 0.18f * bcmRnd(k + 1, (int)az));
            gpQuad(b, i, x0 + nx, 0.02f, z0 + nz, g0, x0 - nx, 0.02f, z0 - nz, g0, x1 - nx, 0.02f, z1 - nz, g1, x1 + nx, 0.02f, z1 + nz, g1);
            for (int s = -1; s <= 1; s += 2) {   // cordon: tapa + cara de adentro
                const float ex = nx * s * 1.0f, ez = nz * s * 1.0f, ox = nx * s * 1.12f, oz = nz * s * 1.12f;
                const unsigned int ct = gpShade(BCM_STONE2, 0, 1, 0), cs = gpShade(BCM_STONE2, -nx * s, 0.2f, -nz * s);
                gpQuad(b, i, x0 + ex, 0.12f, z0 + ez, ct, x1 + ex, 0.12f, z1 + ez, ct, x1 + ox, 0.12f, z1 + oz, ct, x0 + ox, 0.12f, z0 + oz, ct);
                gpQuad(b, i, x0 + ex, 0.0f, z0 + ez, cs, x1 + ex, 0.0f, z1 + ez, cs, x1 + ex, 0.12f, z1 + ez, cs, x0 + ex, 0.12f, z0 + ez, cs);
            }
        }
    };
    path(0.0f, BC_SOUTH_Z, 0.0f, -10.0f, 4.0f, 10);
    path(0.0f, -14.0f, 0.0f, -40.0f, 4.0f, 7);
    path(-29.0f, -12.0f, -2.0f, -12.0f, 4.0f, 7);
    path(2.0f, -12.0f, 29.0f, -12.0f, 4.0f, 7);
    {   // el cruce: una losa de grava sin cordon
        const unsigned int g = BCM_GRAVEL;
        gpQuad(b, i, -2.0f, 0.021f, -14.0f, g, 2.0f, 0.021f, -14.0f, g, 2.0f, 0.021f, -10.0f, g, -2.0f, 0.021f, -10.0f, g);
        gpQuad(b, i, -7.0f, 0.021f, -43.0f, g, 7.0f, 0.021f, -43.0f, g, 7.0f, 0.021f, -36.5f, g, -7.0f, 0.021f, -36.5f, g);   // atrio de la capilla
    }
    // muro bajo de piedra alrededor (4 tramos largos biselados + pilares en esquinas y junto al porton)
    const float H = 1.3f;
    bcmBox(b, i, 0.0f, 0.0f, BC_NORTH_Z, 2.0f * BC_HALF_X, 0.8f, H, 0.12f, BCM_STONE2, BCM_MOSS);
    bcmBox(b, i, -BC_HALF_X, 0.0f, (BC_NORTH_Z + BC_SOUTH_Z) * 0.5f, 0.8f, BC_SOUTH_Z - BC_NORTH_Z, H, 0.12f, BCM_STONE2, BCM_MOSS);
    bcmBox(b, i, BC_HALF_X, 0.0f, (BC_NORTH_Z + BC_SOUTH_Z) * 0.5f, 0.8f, BC_SOUTH_Z - BC_NORTH_Z, H, 0.12f, BCM_STONE2, BCM_MOSS);
    bcmBox(b, i, -(BC_HALF_X + 3.9f) * 0.5f, 0.0f, BC_SOUTH_Z, BC_HALF_X - 3.9f, 0.8f, H * 0.6f, 0.1f, BCM_STONE2, BCM_MOSS);
    bcmBox(b, i, (BC_HALF_X + 3.9f) * 0.5f, 0.0f, BC_SOUTH_Z, BC_HALF_X - 3.9f, 0.8f, H * 0.6f, 0.1f, BCM_STONE2, BCM_MOSS);
    const float pxs[4] = { -BC_HALF_X, BC_HALF_X, -BC_HALF_X, BC_HALF_X }, pzs[4] = { BC_NORTH_Z, BC_NORTH_Z, BC_SOUTH_Z, BC_SOUTH_Z };
    for (int k = 0; k < 4; ++k) {
        bcmBox(b, i, pxs[k], 0.0f, pzs[k], 1.3f, 1.3f, H + 0.9f, 0.1f, BCM_STONE, BCM_MOSS);
        gpPrism(b, i, pxs[k], H + 0.9f, pzs[k], 0.85f, 0.0f, 0.6f, 4, 0.7854f, BCM_STONE2, BCM_STONE2, 0);
    }
    // pasto alto en mechones: junto a las tumbas, al pie del muro y a los lados del camino
    for (int k = 0; k < 130 && i + 9 <= cap; ++k) {
        float x, z;
        const float r = bcmRnd(k, 901);
        if (r < 0.35f) { x = (bcmRnd(k, 902) < 0.5f ? -1.0f : 1.0f) * (2.5f + 1.6f * bcmRnd(k, 903)); z = BC_SOUTH_Z - 1.0f - 70.0f * bcmRnd(k, 904); }
        else if (r < 0.55f) { x = (bcmRnd(k, 905) < 0.5f ? -1.0f : 1.0f) * (BC_HALF_X - 0.8f - 1.4f * bcmRnd(k, 906)); z = BC_NORTH_Z + (BC_SOUTH_Z - BC_NORTH_Z) * bcmRnd(k, 907); }
        else { x = -30.0f + 60.0f * bcmRnd(k, 908); z = BC_NORTH_Z + 4.0f + (BC_SOUTH_Z - BC_NORTH_Z - 6.0f) * bcmRnd(k, 909);
               if (fabsf(x) < 2.6f || fabsf(z + 12.0f) < 2.6f) continue; }
        gpTuft(b, i, x, 0.0f, z, 0.35f + 0.35f * bcmRnd(k, 910), (unsigned int)k, BCM_GRASSD, cp_scale(BCM_GRASS, 1.18f));
    }
    return i;
}

#ifdef BCM_PREVIEW
// para el visor de la PC: el piso entero con las instancias ya colocadas
static void bcmBuildScenePreview(LineVertex *out, int cap) {
    static LineVertex pool[30000];
    static BcmInst inst[400];
    int i = bcmBuildGround(out, cap);
    if (bcmBuildAll(pool, 30000) < 0) { fprintf(stderr, "el pool de modelos no alcanza\n"); }
    const int n = bcmLayout(inst, 400);
    std::vector<LineVertex> all(out, out + i);
    for (int k = 0; k < n; ++k) {
        const BcmModel &M = g_bcm[inst[k].m]; const float c = cosf(inst[k].yaw), s = sinf(inst[k].yaw), sc = inst[k].s;
        for (int q = 0; q < M.n; ++q) {
            LineVertex v = pool[M.off + q];
            const float x = v.x * sc, z = v.z * sc;
            v.x = inst[k].x + c * x + s * z; v.y = v.y * sc; v.z = inst[k].z - s * x + c * z;
            all.push_back(v);
        }
    }
    sceGumLoadIdentity(); sceGumDrawArray(GU_TRIANGLES, 0, (int)all.size(), 0, all.data());
}
#endif
