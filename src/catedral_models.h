#pragma once
// =============================================================================
// catedral_models.h -- los MODELOS del piso 10, LA CATEDRAL: la nave gotica (estructura grande) y su
// guardian, EL VIGIA. Sin nada del motor: se arman igual en la PSP (src/boss_catedral.h) y en la PC
// (tools/modelview) para disenarlos a la vista.
//
// Benjamin (2026-10-16): "quiero que aparezca uno en la catedral, tiene que ser una estructura grande,
// ve de referencia este personaje de fear and hunger" (Rancid). EL VIGIA es una criatura propia en esa
// linea: flaca y encorvada, piernas de bestia (digitigradas), pelaje gris largo en jirones, una MASCARA
// de hueso alargada llena de OJOS (los ojos de la Torre otra vez) con astas, cola, y DOS LANZAS de hoja
// de hoja de laurel, acero azulado con anillos de oro.
//
// Convenciones: todo en el origen, el frente mira a -Z, pies en y=0. El Vigia se autoria en METROS
// (~2.4 de alto encorvado) y se escala con CT_S al dibujarlo.
// =============================================================================

// ---------------------------------------------------------------- paleta
static const unsigned int CTM_FUR   = RGBA(176, 172, 164, 255);   // pelaje gris claro
static const unsigned int CTM_FURD  = RGBA(104, 100,  96, 255);   // pelaje en sombra
static const unsigned int CTM_FURL  = RGBA(214, 210, 200, 255);   // puntas claras
static const unsigned int CTM_SKIN  = RGBA( 84,  80,  84, 255);   // piel gris oscura (manos, pies)
static const unsigned int CTM_CLAW  = RGBA( 36,  30,  30, 255);
static const unsigned int CTM_BONE  = RGBA(222, 210, 182, 255);   // la mascara
static const unsigned int CTM_BONED = RGBA(160, 146, 118, 255);
static const unsigned int CTM_EYE   = RGBA( 30,  22,  22, 255);
static const unsigned int CTM_IRIS  = RGBA(236,  70,  40, 255);
static const unsigned int CTM_STEEL = RGBA( 70,  84, 112, 255);   // hoja de la lanza
static const unsigned int CTM_STEELL= RGBA(150, 170, 204, 255);
static const unsigned int CTM_GOLD  = RGBA(196, 158,  72, 255);
static const unsigned int CTM_SHAFT = RGBA( 34,  34,  42, 255);
// la catedral
static const unsigned int CTM_STONE = RGBA(120, 116, 112, 255);
static const unsigned int CTM_STONEL= RGBA(150, 146, 140, 255);
static const unsigned int CTM_STONED= RGBA( 70,  68,  70, 255);
static const unsigned int CTM_VAULT = RGBA( 54,  52,  60, 255);

static float ctmRnd(int a, int b) {
    unsigned int h = (unsigned int)a * 2654435761u ^ (unsigned int)b * 40503u ^ 0x7F4A7C15u;
    h ^= h >> 15; h *= 0x2C1B3C6Du; h ^= h >> 12;
    return (float)(h & 0xFFFF) / 65535.0f;
}
// mechon de PELAJE alrededor de un miembro: n mechas que nacen en un anillo (centro cx,cz a la altura y)
// y caen ABRIENDOSE hacia afuera (flare), cada una con su largo, ancho y tono. Dos triangulos cruzados
// por mecha (6 v): de cualquier lado se ve volumen, no una cortina.
static void ctmFur(LineVertex *b, int &i, float cx, float y, float cz, float r, int n, float len, unsigned int seed) {
    for (int k = 0; k < n; ++k) {
        const float a = GP_TAU * (k + 0.5f * ctmRnd(seed, k)) / n, ca = cosf(a), sa = sinf(a);
        const float L = len * (0.55f + 0.6f * ctmRnd(seed + 7, k)), fl = 0.35f + 0.45f * ctmRnd(seed + 3, k);
        const float w = (0.35f + 0.25f * ctmRnd(seed + 5, k)) * GP_TAU * r / n + 0.012f;
        const float bx = cx + ca * r, bz = cz + sa * r, tx = cx + ca * (r + L * fl), tz = cz + sa * (r + L * fl), ty = y - L;
        const unsigned int c0 = cp_lerp(CTM_FURD, CTM_FUR, 0.5f + 0.5f * ctmRnd(seed + 9, k)), c1 = cp_lerp(CTM_FUR, CTM_FURL, ctmRnd(seed + 11, k));
        gpTri(b, i, bx - sa * w, y, bz + ca * w, gpShade(c0, ca, 0.3f, sa), bx + sa * w, y, bz - ca * w, gpShade(c0, ca, 0.3f, sa), tx, ty, tz, gpShade(c1, ca, -0.2f, sa));
        gpTri(b, i, bx, y + 0.02f, bz, gpShade(c0, ca, 0.6f, sa), cx + ca * (r * 0.6f), y - 0.02f, cz + sa * (r * 0.6f), gpShade(c0, ca, 0.3f, sa), tx, ty, tz, gpShade(c1, ca, 0.0f, sa));
    }
}

// ============================================================================ EL VIGIA
// PIEZAS (rangos en g_ctMark, las articula boss_catedral.h):
//   [0]muslo izq [1]cana+pata izq [2]muslo der [3]cana+pata der [4]torso+cuello+melena [5]cola
//   [6]brazo izq [7]antebrazo+mano izq [8]brazo der [9]antebrazo+mano der [10]cabeza/mascara [11]fin
// Las LANZAS son otro modelo (ctmBuildSpear): se dibujan en cada mano y la derecha se puede arrojar.
static const int CTM_NMARK = 12;
static int g_ctMark[CTM_NMARK];
// articulaciones (metros)
static const float CTJ_HIP[3]   = { 0.13f, 1.05f,  0.05f };   // x se espeja
static const float CTJ_KNEE[3]  = { 0.16f, 0.62f, -0.16f };
static const float CTJ_SHL[3]   = { 0.25f, 1.74f, -0.17f };
static const float CTJ_ELB[3]   = { 0.39f, 1.32f, -0.12f };
static const float CTJ_HAND[3]  = { 0.41f, 0.94f, -0.34f };
static const float CTJ_NECK[3]  = { 0.00f, 1.90f, -0.36f };
static const float CTJ_TAIL[3]  = { 0.00f, 1.02f,  0.16f };

static int ctmBuildVigia(LineVertex *b, int cap) {
    int i = 0; if (cap < 4200) return 0;
    g_ctMark[0] = 0;
    // ---- PIERNAS digitigradas: muslo hacia adelante, cana hacia atras, metatarso largo, garras
    for (int side = -1; side <= 1; side += 2) {
        const float s = (float)side;
        gpTube(b, i, s * CTJ_HIP[0], CTJ_HIP[1], CTJ_HIP[2], s * CTJ_KNEE[0], CTJ_KNEE[1], CTJ_KNEE[2], 0.095f, 0.062f, 8, CTM_FURD, CTM_FUR);
        ctmFur(b, i, s * 0.14f, 1.02f, 0.02f, 0.12f, 9, 0.30f, 11u + side);
        ctmFur(b, i, s * 0.15f, 0.82f, -0.08f, 0.09f, 8, 0.24f, 21u + side);
        g_ctMark[side < 0 ? 1 : 3] = i;
        const float hx = s * 0.15f, hy = 0.30f, hz = 0.14f, fx = s * 0.15f, fy = 0.05f, fz = -0.03f;
        gpTube(b, i, s * CTJ_KNEE[0], CTJ_KNEE[1], CTJ_KNEE[2], hx, hy, hz, 0.062f, 0.046f, 8, CTM_FUR, CTM_FURD);   // cana (hacia atras)
        addBall(b, i, s * CTJ_KNEE[0], CTJ_KNEE[1], CTJ_KNEE[2], 0.064f, 0.064f, 0.064f, 2, 8, CTM_FUR);
        gpTube(b, i, hx, hy, hz, fx, fy, fz, 0.046f, 0.040f, 8, CTM_SKIN, CTM_SKIN);                               // metatarso
        addBall(b, i, hx, hy, hz, 0.048f, 0.048f, 0.048f, 2, 6, CTM_SKIN);
        ctmFur(b, i, hx, hy + 0.06f, hz, 0.06f, 6, 0.14f, 31u + side);                                             // mechon del corvejon
        for (int k = -1; k <= 1; ++k)                                                                               // tres dedos con garra
            addLimb(b, i, fx + k * 0.035f, 0.035f, fz, fx + k * 0.05f, 0.012f, fz - 0.17f, 0.022f, 0.006f, 5, CTM_SKIN, CTM_CLAW);
        g_ctMark[side < 0 ? 2 : 4] = i;
    }
    // ---- TORSO encorvado (anillos que se van adelante), costillas que asoman, melena de hombros
    {
        const float ys[5] = { 0.98f, 1.18f, 1.40f, 1.60f, 1.80f };
        const float rx[5] = { 0.150f, 0.138f, 0.170f, 0.205f, 0.170f }, rz[5] = { 0.120f, 0.112f, 0.140f, 0.150f, 0.120f };
        const float oz[5] = { 0.050f, 0.020f, -0.040f, -0.110f, -0.180f };
        const unsigned int c[5] = { CTM_FURD, CTM_FUR, CTM_FUR, CTM_FUR, CTM_FURL };
        gpLoftE(b, i, 0.0f, 0.0f, 5, ys, rx, rz, oz, c, 10, 0.0f, GP_TAU);
    }
    ctmFur(b, i, 0.0f, 1.86f, -0.17f, 0.26f, 18, 0.62f, 41u);                     // melena de hombros (larga)
    ctmFur(b, i, 0.0f, 1.66f, -0.08f, 0.24f, 14, 0.50f, 43u);
    ctmFur(b, i, 0.0f, 1.44f, -0.02f, 0.19f, 11, 0.40f, 47u);                      // pelaje del lomo y el vientre
    ctmFur(b, i, 0.0f, 1.20f, 0.03f, 0.16f, 9, 0.34f, 53u);
    ctmFur(b, i, 0.0f, 1.02f, 0.06f, 0.16f, 9, 0.30f, 57u);
    gpTube(b, i, 0.0f, 1.74f, -0.20f, CTJ_NECK[0], CTJ_NECK[1] + 0.02f, CTJ_NECK[2], 0.090f, 0.070f, 8, CTM_FUR, CTM_FURD);   // cuello
    ctmFur(b, i, 0.0f, 1.88f, -0.30f, 0.11f, 9, 0.30f, 59u);
    g_ctMark[5] = i;
    // ---- COLA larga y peluda
    {
        const float tx[4] = { 0.0f, 0.0f, 0.04f, 0.10f }, ty[4] = { CTJ_TAIL[1], 0.78f, 0.50f, 0.30f }, tz[4] = { CTJ_TAIL[2], 0.46f, 0.74f, 0.98f };
        const float tr[4] = { 0.080f, 0.066f, 0.050f, 0.024f };
        for (int k = 0; k < 3; ++k) {
            gpTube(b, i, tx[k], ty[k], tz[k], tx[k + 1], ty[k + 1], tz[k + 1], tr[k], tr[k + 1], 6, CTM_FUR, CTM_FURL);
            ctmFur(b, i, (tx[k] + tx[k + 1]) * 0.5f, (ty[k] + ty[k + 1]) * 0.5f + 0.08f, (tz[k] + tz[k + 1]) * 0.5f, tr[k] + 0.02f, 8, 0.24f, 61u + k);
        }
    }
    g_ctMark[6] = i;
    // ---- BRAZOS largos: brazo | antebrazo con mano de garras (agarra la lanza)
    for (int side = -1; side <= 1; side += 2) {
        const float s = (float)side;
        gpTube(b, i, s * CTJ_SHL[0], CTJ_SHL[1], CTJ_SHL[2], s * CTJ_ELB[0], CTJ_ELB[1], CTJ_ELB[2], 0.062f, 0.050f, 8, CTM_FUR, CTM_FURD);
        ctmFur(b, i, s * 0.30f, 1.68f, -0.15f, 0.085f, 9, 0.30f, 71u + side);
        ctmFur(b, i, s * 0.35f, 1.48f, -0.14f, 0.065f, 8, 0.26f, 75u + side);
        g_ctMark[side < 0 ? 7 : 9] = i;
        gpTube(b, i, s * CTJ_ELB[0], CTJ_ELB[1], CTJ_ELB[2], s * CTJ_HAND[0], CTJ_HAND[1] + 0.05f, CTJ_HAND[2], 0.050f, 0.038f, 8, CTM_FURD, CTM_SKIN);
        ctmFur(b, i, s * 0.395f, 1.26f, -0.16f, 0.06f, 8, 0.24f, 81u + side);
        ctmFur(b, i, s * 0.40f, 1.08f, -0.25f, 0.05f, 7, 0.20f, 85u + side);
        addBall(b, i, s * CTJ_HAND[0], CTJ_HAND[1], CTJ_HAND[2], 0.055f, 0.065f, 0.050f, 3, 8, CTM_SKIN);    // mano
        for (int k = 0; k < 4; ++k) {                                                    // dedos que rodean el asta
            const float y = CTJ_HAND[1] + 0.045f - k * 0.030f;
            addLimb(b, i, s * (CTJ_HAND[0] - 0.05f), y, CTJ_HAND[2] - 0.02f, s * (CTJ_HAND[0] + 0.03f), y - 0.01f, CTJ_HAND[2] - 0.08f, 0.016f, 0.007f, 5, CTM_SKIN, CTM_CLAW);
        }
        g_ctMark[side < 0 ? 8 : 10] = i;
    }
    // ---- CABEZA: craneo peludo, MASCARA de hueso larga con OJOS, mandibula, astas y la melena larga
    {
        const float nx = CTJ_NECK[0], ny = CTJ_NECK[1], nz = CTJ_NECK[2];
        addBall(b, i, nx, ny + 0.08f, nz - 0.04f, 0.125f, 0.135f, 0.140f, 4, 8, CTM_FURD);              // craneo
        ctmFur(b, i, nx, ny + 0.18f, nz + 0.02f, 0.13f, 16, 0.72f, 91u);                                  // melena larga
        ctmFur(b, i, nx, ny + 0.04f, nz + 0.06f, 0.15f, 14, 0.58f, 93u);
        addBall(b, i, nx, ny + 0.13f, nz - 0.10f, 0.150f, 0.110f, 0.160f, 4, 10, CTM_BONE);              // frente de la mascara
        addLimb(b, i, nx, ny + 0.11f, nz - 0.12f, nx, ny - 0.02f, nz - 0.68f, 0.135f, 0.030f, 10, CTM_BONE, CTM_BONED);   // pico
        addLimb(b, i, nx, ny - 0.02f, nz - 0.14f, nx, ny - 0.10f, nz - 0.52f, 0.075f, 0.018f, 8, CTM_BONED, CTM_BONED);  // mandibula
        // OJOS sobre la mascara: tres filas (el de arriba mas grande), cada uno cuenca oscura + iris rojo
        const float L = 0.56f;
        for (int row = 0; row < 3; ++row)
            for (int k = 0; k < 3 - (row == 2); ++k) {
                const float t = 0.12f + 0.20f * row + 0.06f * (k & 1), rr = 0.135f * (1.0f - t) + 0.03f * t;   // radio del pico ahi
                const float a = (row == 2) ? 0.0f : (k - 1) * 0.62f;                                          // angulo sobre el pico (0 = arriba)
                const float cx = nx + sinf(a) * rr, cy = ny + 0.11f - 0.13f * t + cosf(a) * rr, cz = nz - 0.12f - L * t;
                const float er = (row == 0 && k == 1) ? 0.046f : 0.034f;
                const float ux = sinf(a), uy = cosf(a);                                                       // normal (hacia afuera)
                for (int q = 0; q < 6; ++q) {
                    const float b0 = GP_TAU * q / 6, b1 = GP_TAU * (q + 1) / 6;
                    // disco en el plano perpendicular a la normal (u) usando ejes (uy,-ux,0) y (0,0,1)
                    const float p0x = cx + (uy * cosf(b0)) * er, p0y = cy + (-ux * cosf(b0)) * er, p0z = cz + sinf(b0) * er * 1.3f;
                    const float p1x = cx + (uy * cosf(b1)) * er, p1y = cy + (-ux * cosf(b1)) * er, p1z = cz + sinf(b1) * er * 1.3f;
                    gpTri(b, i, cx + ux * 0.004f, cy + uy * 0.004f, cz, CTM_EYE, p0x, p0y, p0z, CTM_EYE, p1x, p1y, p1z, CTM_EYE);
                }
                const float ir = er * 0.55f;
                for (int q = 0; q < 4; ++q) {
                    const float b0 = GP_TAU * q / 4, b1 = GP_TAU * (q + 1) / 4;
                    gpTri(b, i, cx + ux * 0.010f, cy + uy * 0.010f, cz, CTM_IRIS,
                                cx + ux * 0.008f + uy * cosf(b0) * ir, cy + uy * 0.008f - ux * cosf(b0) * ir, cz + sinf(b0) * ir, CTM_IRIS,
                                cx + ux * 0.008f + uy * cosf(b1) * ir, cy + uy * 0.008f - ux * cosf(b1) * ir, cz + sinf(b1) * ir, CTM_IRIS);
                }
            }
        // ASTAS: dos ramas principales que suben y se abren, con puntas
        for (int s = -1; s <= 1; s += 2) {
            const float ax0 = nx + s * 0.07f, ay0 = ny + 0.20f, az0 = nz - 0.06f;
            const float ax1 = nx + s * 0.15f, ay1 = ny + 0.46f, az1 = nz + 0.02f;
            const float ax2 = nx + s * 0.11f, ay2 = ny + 0.66f, az2 = nz + 0.10f;
            gpTube(b, i, ax0, ay0, az0, ax1, ay1, az1, 0.030f, 0.022f, 5, CTM_BONE, CTM_BONE);
            gpTube(b, i, ax1, ay1, az1, ax2, ay2, az2, 0.022f, 0.008f, 5, CTM_BONE, CTM_BONED);
            addLimb(b, i, ax1, ay1, az1, ax1 + s * 0.14f, ay1 + 0.10f, az1 - 0.06f, 0.016f, 0.004f, 4, CTM_BONE, CTM_BONED);
            addLimb(b, i, (ax0 + ax1) * 0.5f, (ay0 + ay1) * 0.5f, (az0 + az1) * 0.5f, (ax0 + ax1) * 0.5f - s * 0.02f, (ay0 + ay1) * 0.5f + 0.12f, (az0 + az1) * 0.5f - 0.10f, 0.014f, 0.004f, 4, CTM_BONE, CTM_BONED);
        }
    }
    g_ctMark[11] = i;
    return i;
}
// LANZA: asta oscura (eje Y, la hoja hacia -Y), mango con dos anillos de oro, cubo dorado y una hoja
// de laurel de acero azulado con nervio claro. La mano la toma en y=0. ~360 v
static int ctmBuildSpear(LineVertex *b, int cap) {
    int i = 0; if (cap < 400) return 0;
    gpTube(b, i, 0.0f, 1.55f, 0.0f, 0.0f, -1.05f, 0.0f, 0.028f, 0.030f, 6, CTM_SHAFT, cp_scale(CTM_SHAFT, 1.5f));
    gpTube(b, i, 0.0f, 0.17f, 0.0f, 0.0f, 0.11f, 0.0f, 0.040f, 0.040f, 6, CTM_GOLD, CTM_GOLD);     // anillos del mango
    gpTube(b, i, 0.0f, -0.11f, 0.0f, 0.0f, -0.17f, 0.0f, 0.040f, 0.040f, 6, CTM_GOLD, CTM_GOLD);
    gpTube(b, i, 0.0f, -1.05f, 0.0f, 0.0f, -1.24f, 0.0f, 0.040f, 0.050f, 8, CTM_GOLD, cp_scale(CTM_GOLD, 1.2f));   // cubo
    gpTube(b, i, 0.0f, -1.22f, 0.0f, 0.0f, -1.27f, 0.0f, 0.062f, 0.050f, 8, cp_scale(CTM_GOLD, 1.25f), CTM_GOLD);
    // hoja: seccion de rombo (ancho en X, delgada en Z) que crece y afina hasta la punta
    const int N = 6; const float y0 = -1.24f, y1 = -1.95f;
    float pw[N + 1], py[N + 1];
    for (int k = 0; k <= N; ++k) { const float t = (float)k / N; py[k] = y0 + (y1 - y0) * t; pw[k] = 0.10f * sinf(GP_PI * (0.12f + 0.88f * t)) * (1.0f - 0.25f * t); }
    pw[N] = 0.0f;
    for (int k = 0; k < N; ++k) {
        const float th0 = 0.022f * (1.0f - (float)k / N), th1 = 0.022f * (1.0f - (float)(k + 1) / N);
        const unsigned int c0 = cp_lerp(CTM_STEEL, CTM_STEELL, (float)k / N), c1 = cp_lerp(CTM_STEEL, CTM_STEELL, (float)(k + 1) / N);
        for (int s = -1; s <= 1; s += 2) for (int f = -1; f <= 1; f += 2) {   // 4 caras
            gpQuad(b, i, s * pw[k], py[k], 0.0f, gpShade(c0, s * 0.6f, 0.0f, f * 0.8f), 0.0f, py[k], f * th0, gpShade(c0, s * 0.3f, 0.0f, f * 1.0f),
                         0.0f, py[k + 1], f * th1, gpShade(c1, s * 0.3f, 0.0f, f * 1.0f), s * pw[k + 1], py[k + 1], 0.0f, gpShade(c1, s * 0.6f, 0.0f, f * 0.8f));
        }
    }
    // nervio claro y grabado de oro (dos rayitas) en las dos caras
    for (int f = -1; f <= 1; f += 2) {
        gpQuad(b, i, -0.006f, y0, f * 0.024f, CTM_STEELL, 0.006f, y0, f * 0.024f, CTM_STEELL, 0.003f, y1 + 0.05f, f * 0.004f, CTM_STEELL, -0.003f, y1 + 0.05f, f * 0.004f, CTM_STEELL);
        for (int k = 0; k < 2; ++k) {
            const float y = y0 - 0.10f - 0.12f * k, w = 0.05f - 0.012f * k;
            gpQuad(b, i, -w, y, f * 0.022f, CTM_GOLD, w, y, f * 0.022f, CTM_GOLD, w, y - 0.012f, f * 0.022f, CTM_GOLD, -w, y - 0.012f, f * 0.022f, CTM_GOLD);
        }
    }
    return i;
}

// ============================================================================ LA CATEDRAL
// Planta: la nave corre a lo largo de Z. Entrada (fachada) en z = +CT_ZIN; abside al fondo (-Z).
//   nave central x = -12..12, naves laterales hasta +-22. Pilares en x = +-12 cada 18 en z.
//   boveda de la nave en y = 38..50 (ojival); techo de las laterales en y = 24.
static const float CT_ZIN = 44.0f, CT_ZAPSE = -84.0f, CT_HALF = 22.0f, CT_NAVE = 12.0f;
static const float CT_SPRING = 38.0f, CT_APEX = 52.0f, CT_AISLE_H = 24.0f;
static const int   CT_NPIER = 6;
static const float kCtPierZ[CT_NPIER] = { 26.0f, 8.0f, -10.0f, -28.0f, -46.0f, -64.0f };
static const float CT_BELL_Z = -19.0f;   // las campanas cuelgan sobre el centro de la nave

// pilar gotico en haz: nucleo octogonal + 4 columnillas adosadas, basa y capitel biselados. ~370 v
static int ctmBuildPier(LineVertex *b, int cap, int lod) {
    int i = 0; if (cap < 420) return 0;
    const float H = 26.0f;
    if (lod) {
        gpBox(b, i, 0.0f, 0.0f, 0.0f, 2.8f, 2.8f, 1.2f, CTM_STONE, CTM_STONED);
        for (int k = 0; k < 2; ++k) {
            const float y0 = 1.2f + (H - 2.4f) * k / 2.0f, y1 = 1.2f + (H - 2.4f) * (k + 1) / 2.0f;
            gpPrism(b, i, 0.0f, y0, 0.0f, 1.3f, 1.3f, y1 - y0, 6, 0.0f, CTM_STONED, CTM_STONE, 0);
        }
        gpBox(b, i, 0.0f, H - 1.2f, 0.0f, 3.0f, 3.0f, 1.2f, CTM_STONEL, CTM_STONE);
        return i;
    }
    gpBox(b, i, 0.0f, 0.0f, 0.0f, 3.0f, 3.0f, 0.7f, CTM_STONE, CTM_STONED);
    gpBox(b, i, 0.0f, 0.7f, 0.0f, 2.5f, 2.5f, 0.5f, CTM_STONE, CTM_STONE);
    // nucleo y 4 columnillas en tramos de <= 8 (el GE descarta triangulos enormes junto a la camara)
    for (int k = 0; k < 2; ++k) {
        const float y0 = 1.2f + (H - 2.4f) * k / 2.0f, y1 = 1.2f + (H - 2.4f) * (k + 1) / 2.0f;
        gpPrism(b, i, 0.0f, y0, 0.0f, 1.04f, 1.02f, y1 - y0, 8, 0.3927f, cp_lerp(CTM_STONED, CTM_STONE, k / 2.0f), cp_lerp(CTM_STONED, CTM_STONE, (k + 1) / 2.0f), 0);
        for (int q = 0; q < 4; ++q) {
            const float a = GP_PI * 0.5f * q, x = cosf(a) * 1.05f, z = sinf(a) * 1.05f;
            gpTube(b, i, x, y0, z, x, y1, z, 0.31f, 0.31f, 5, CTM_STONED, CTM_STONEL);
        }
    }
    gpBox(b, i, 0.0f, H - 1.2f, 0.0f, 2.9f, 2.9f, 0.6f, CTM_STONEL, CTM_STONE);
    gpBox(b, i, 0.0f, H - 0.6f, 0.0f, 3.4f, 3.4f, 0.6f, CTM_STONEL, CTM_STONEL);
    return i;
}
// vitral ojival (mira a -Z en su modelo): marco de piedra + vidrio de colores en 3 franjas y un
// parteluz; c = paleta (0 azul/rojo, 1 oro/verde, 2 violeta/ambar). ~260 v
static int ctmBuildWindow(LineVertex *b, int cap, float w, float h, int c) {
    int i = 0; if (cap < 300) return 0;
    static const unsigned int kG[3][3] = {
        { RGBA(40, 70, 170, 255), RGBA(170, 40, 46, 255), RGBA(60, 110, 200, 255) },
        { RGBA(200, 156, 50, 255), RGBA(50, 130, 80, 255), RGBA(220, 190, 90, 255) },
        { RGBA(110, 60, 170, 255), RGBA(210, 140, 50, 255), RGBA(150, 90, 200, 255) } };
    const int small = h < 10.0f;   // los del claristorio, altos y lejos: solo el vidrio y el parteluz
    if (!small) gpArchSlab(b, i, 0.0f, 0.0f, 0.06f, w + 0.6f, h + 0.5f, 0.24f, 4, 1, CTM_STONE, CTM_STONED);    // marco
    // vidrio: solo la cara de adelante (un abanico), en tres franjas de colores
    const int i0 = i;
    {
        const int seg = 6; const float r = w * 0.5f, R = w * 1.4f, ca = (R - r) / R, amax = acosf(ca), apex = R * sinf(amax), hs = h - apex;
        float px[16], py[16]; int n = 0;
        px[n] = -r; py[n++] = 0.0f; px[n] = r; py[n++] = 0.0f;
        for (int k = 0; k <= seg / 2; ++k) { const float a = amax * k / (seg / 2); px[n] = r - R + R * cosf(a); py[n++] = hs + R * sinf(a); }
        for (int k = seg / 2 - 1; k >= 0; --k) { const float a = amax * k / (seg / 2); px[n] = -(r - R + R * cosf(a)); py[n++] = hs + R * sinf(a); }
        for (int k = 0; k < n; ++k) { const int j = (k + 1) % n;
            gpTri(b, i, 0.0f, 0.25f + h * 0.45f, -0.08f, kG[c][1], px[k], 0.25f + py[k], -0.08f, kG[c][0], px[j], 0.25f + py[j], -0.08f, kG[c][0]); }
    }
    for (int k = i0; k < i; ++k) {   // colorear por altura: abajo, medio y arriba
        const float t = (b[k].y - 0.25f) / h;
        const unsigned int base = t < 0.33f ? kG[c][0] : (t < 0.66f ? kG[c][1] : kG[c][2]);
        b[k].color = cp_scale(base, 0.85f + 0.3f * ctmRnd((int)(b[k].x * 40.0f), (int)(b[k].y * 7.0f)));
    }
    // parteluz (y en los grandes, dos travesanos) de piedra: caras de adelante
    const unsigned int st = gpShade(CTM_STONEL, 0, 0, -1);
    gpQuad(b, i, -0.08f, 0.25f, -0.12f, st, 0.08f, 0.25f, -0.12f, st, 0.08f, 0.25f + h * 0.78f, -0.12f, st, -0.08f, 0.25f + h * 0.78f, -0.12f, st);
    for (int k = 1; k <= 2 && !small; ++k) { const float y = 0.25f + h * 0.26f * k;
        gpQuad(b, i, -w * 0.5f, y, -0.12f, st, w * 0.5f, y, -0.12f, st, w * 0.5f, y + 0.12f, -0.12f, st, -w * 0.5f, y + 0.12f, -0.12f, st); }
    return i;
}
// CAMPANA de bronce colgando de su yugo (la boca abajo, y=0 en el yugo). ~420 v
static int ctmBuildBell(LineVertex *b, int cap) {
    int i = 0; if (cap < 460) return 0;
    const unsigned int BR = RGBA(130, 104, 58, 255), BRL = RGBA(186, 150, 86, 255), BRD = RGBA(70, 56, 34, 255);
    gpBox(b, i, 0.0f, 0.0f, 0.0f, 3.6f, 0.5f, 0.5f, RGBA(60, 46, 34, 255), RGBA(40, 30, 22, 255));         // yugo de madera
    for (int s = -1; s <= 1; s += 2) gpTube(b, i, s * 1.5f, 0.5f, 0.0f, s * 1.2f, 16.0f, 0.0f, 0.06f, 0.06f, 3, RGBA(40, 38, 42, 255), RGBA(40, 38, 42, 255));   // cadenas
    const float ys[5] = { -0.1f, -1.0f, -2.4f, -3.3f, -3.7f }, rr[5] = { 0.5f, 0.95f, 1.25f, 1.75f, 2.0f };
    for (int k = 0; k < 4; ++k) gpTube(b, i, 0.0f, ys[k], 0.0f, 0.0f, ys[k + 1], 0.0f, rr[k], rr[k + 1], 12, k < 2 ? BRL : BR, k < 2 ? BR : BRD);
    gpTube(b, i, 0.0f, -3.40f, 0.0f, 0.0f, -3.75f, 0.0f, 1.95f, 2.08f, 12, BRL, BR);                         // labio
    addBall(b, i, 0.0f, -3.1f, 0.0f, 0.32f, 0.32f, 0.32f, 2, 6, BRD);                                         // badajo
    return i;
}
// ARANA de hierro: dos aros con velas, colgando de cadenas (y=0 arriba). ~330 v
static int ctmBuildChandelier(LineVertex *b, int cap) {
    int i = 0; if (cap < 380) return 0;
    const unsigned int IR = RGBA(40, 38, 42, 255), WAX = RGBA(232, 224, 200, 255);
    gpTube(b, i, 0.0f, 0.0f, 0.0f, 0.0f, 22.0f, 0.0f, 0.06f, 0.06f, 3, IR, IR);   // la cadena hasta la boveda
    for (int k = 0; k < 3; ++k) { const float a = GP_TAU * k / 3; gpTube(b, i, 0.0f, 0.0f, 0.0f, cosf(a) * 2.2f, -5.0f, sinf(a) * 2.2f, 0.04f, 0.04f, 3, IR, IR); }
    for (int k = 0; k < 8; ++k) {   // aro (8 tramos) y una vela en cada vertice
        const float a = GP_TAU * k / 8, a1 = GP_TAU * (k + 1) / 8, x = cosf(a) * 2.3f, z = sinf(a) * 2.3f;
        gpTube(b, i, x, -5.0f, z, cosf(a1) * 2.3f, -5.0f, sinf(a1) * 2.3f, 0.07f, 0.07f, 3, IR, IR);
        gpTube(b, i, x, -4.95f, z, x, -4.45f, z, 0.07f, 0.06f, 3, WAX, WAX);
    }
    return i;
}

// geometria FIJA de la nave, en TROZOS a lo largo de Z: cada tramo entre pilares, la fachada y el
// abside. boss_catedral.h dibuja solo los trozos que quedan delante de la camara (desde el medio de la
// nave, media catedral queda a la espalda). Los pilares, vitrales, campanas y aranas van por instancia.
struct CtmChunk { int off, n, loOff, loN; float z0, z1; };   // z0 > z1; lo = el mismo trozo, liviano (de lejos)
static const int CTM_NCHUNK = CT_NPIER + 3;     // 7 tramos + fachada + abside
static CtmChunk g_ctChunk[CTM_NCHUNK]; static int g_ctNChunk = 0;

// una moldura corrida (imposta/cornisa) sobre el muro alto: cara de adentro + tapa, en paneles
static void ctmLedge(LineVertex *b, int &i, float s, float xWall, float y0, float h, float out, float z0, float z1) {
    const float xi = xWall - s * out;
    gpPanel(b, i, xi, y0, z0, xi, y0, z1, xi, y0 + h, z1, xi, y0 + h, z0, gpShade(CTM_STONE, -s, 0, 0), gpShade(CTM_STONEL, -s, 0, 0), 8.0f);
    gpPanel(b, i, xWall, y0 + h, z0, xWall, y0 + h, z1, xi, y0 + h, z1, xi, y0 + h, z0, gpShade(CTM_STONEL, 0, 1, 0), gpShade(CTM_STONEL, 0, 1, 0), 8.0f);
}
// un TRAMO de la nave entre z0 y z1 (z0 > z1): suelo, muros, techo de las laterales, arcada, muro alto
// con imposta, cornisa y triforio, y su tramo de boveda con nervios y clave
static void ctmBay(LineVertex *b, int &i, float z0, float z1, int idx, int lod) {
    // suelo: damero de 8 (el camino del centro mas claro)
    const int nz = (int)((z0 - z1) / 8.0f + 0.999f);
    for (int r = 0; r < nz; ++r)
        for (int gx = 0; gx < 6; ++gx) {
            const float za = z0 - (z0 - z1) * r / nz, zb = z0 - (z0 - z1) * (r + 1) / nz, x0 = -24.0f + gx * 8.0f;
            const int dark = (gx + r + idx * 3) & 1;
            unsigned int c = dark ? RGBA(58, 56, 62, 255) : RGBA(118, 114, 110, 255);
            if (gx == 2 || gx == 3) c = dark ? RGBA(74, 70, 72, 255) : RGBA(138, 132, 124, 255);
            gpQuad(b, i, x0, 0.0f, za, c, x0 + 8.0f, 0.0f, za, c, x0 + 8.0f, 0.0f, zb, c, x0, 0.0f, zb, c);
        }
    for (int s = -1; s <= 1; s += 2) {
        const float x = s * CT_HALF, xn = s * CT_NAVE;
        // muro exterior de la lateral, su zocalo y el techo de la lateral
        gpPanel(b, i, x, 0.0f, z0, x, 0.0f, z1, x, CT_AISLE_H, z1, x, CT_AISLE_H, z0, gpShade(CTM_STONED, -s, 0, 0), gpShade(CTM_STONE, -s, 0, 0), 8.0f);
        if (!lod) ctmLedge(b, i, s, x, 0.0f, 1.4f, 1.2f, z0, z1);
        const unsigned int cr = RGBA(46, 44, 50, 255);
        gpPanel(b, i, xn, CT_AISLE_H, z0, x, CT_AISLE_H, z0, x, CT_AISLE_H, z1, xn, CT_AISLE_H, z1, cr, cr, 8.0f);
        // ARCADA: arco ojival de pilar a pilar (con espesor) y la enjuta hasta el techo de la lateral
        {
            const float za = z0 - 1.5f, zb = z1 + 1.5f, zc = (za + zb) * 0.5f, hw = (za - zb) * 0.5f;
            const float spring = 16.0f, R = hw * 1.5f, amax = acosf((R - hw) / R);
            const int SEG = 6;
            float pz[2 * SEG + 1], py[2 * SEG + 1];
            for (int q = 0; q <= SEG; ++q) { const float a = amax * q / SEG; pz[q] = zc + hw - R + R * cosf(a); py[q] = spring + R * sinf(a); }
            for (int q = 0; q < SEG; ++q) { pz[SEG + 1 + q] = 2.0f * zc - pz[SEG - 1 - q]; py[SEG + 1 + q] = py[SEG - 1 - q]; }
            const unsigned int cs = gpShade(CTM_STONE, -s, 0, 0), ci = gpShade(CTM_STONED, 0, -1, 0);
            for (int q = 0; q < 2 * SEG; ++q) {
                gpQuad(b, i, xn, py[q], pz[q], cs, xn, py[q + 1], pz[q + 1], cs, xn, CT_AISLE_H, pz[q + 1], cs, xn, CT_AISLE_H, pz[q], cs);
                if (!lod) gpQuad(b, i, xn - 0.7f, py[q], pz[q], ci, xn + 0.7f, py[q], pz[q], ci, xn + 0.7f, py[q + 1], pz[q + 1], ci, xn - 0.7f, py[q + 1], pz[q + 1], ci);
            }
            // entre el arco y el pilar (los extremos del tramo) el muro baja hasta el arranque
            gpPanel(b, i, xn, spring, z0, xn, spring, za, xn, CT_AISLE_H, za, xn, CT_AISLE_H, z0, cs, cs, 8.0f);
            gpPanel(b, i, xn, spring, zb, xn, spring, z1, xn, CT_AISLE_H, z1, xn, CT_AISLE_H, zb, cs, cs, 8.0f);
        }
        // muro alto (triforio + claristorio) con imposta abajo y cornisa arriba
        gpPanel(b, i, xn, CT_AISLE_H, z0, xn, CT_AISLE_H, z1, xn, CT_SPRING, z1, xn, CT_SPRING, z0, gpShade(CTM_STONED, -s, 0, 0), gpShade(CTM_STONE, -s, 0, 0), 8.0f);
        if (lod) continue;
        ctmLedge(b, i, s, xn, CT_AISLE_H - 0.2f, 0.8f, 0.8f, z0, z1);
        ctmLedge(b, i, s, xn, CT_SPRING - 0.6f, 0.8f, 0.8f, z0, z1);
        for (int k = 0; k < 3; ++k) {   // triforio: tres arquitos oscuros por tramo (rectangulo + punta)
            const float z = z0 - (z0 - z1) * (k + 1) / 4.0f, x = xn - s * 0.12f, y0 = CT_AISLE_H + 1.0f, y1 = y0 + 2.2f;
            const unsigned int d = RGBA(18, 16, 22, 255);
            gpQuad(b, i, x, y0, z - 0.7f, d, x, y0, z + 0.7f, d, x, y1, z + 0.7f, d, x, y1, z - 0.7f, d);
            gpTri(b, i, x, y1, z - 0.7f, d, x, y1, z + 0.7f, d, x, y1 + 0.9f, z, d);
        }
    }
    // BOVEDA: dos faldones hasta la clave, la X de nervios, el arco fajon en z0, la clave y su nervio
    const unsigned int cv = CTM_VAULT;
    gpPanel(b, i, -CT_NAVE, CT_SPRING, z0, -CT_NAVE, CT_SPRING, z1, 0.0f, CT_APEX, z1, 0.0f, CT_APEX, z0, gpShade(cv, 0.7f, -0.7f, 0), cv, 8.0f);
    gpPanel(b, i, CT_NAVE, CT_SPRING, z0, CT_NAVE, CT_SPRING, z1, 0.0f, CT_APEX, z1, 0.0f, CT_APEX, z0, gpShade(cv, -0.7f, -0.7f, 0), cv, 8.0f);
    const float zm = (z0 + z1) * 0.5f;
    for (int s = -1; s <= 1 && !lod; s += 2) {
        gpTube(b, i, s * CT_NAVE, CT_SPRING, z0, 0.0f, CT_APEX - 0.4f, zm, 0.28f, 0.22f, 3, CTM_STONE, CTM_STONEL);
        gpTube(b, i, s * CT_NAVE, CT_SPRING, z1, 0.0f, CT_APEX - 0.4f, zm, 0.28f, 0.22f, 3, CTM_STONE, CTM_STONEL);
    }
    float px = -CT_NAVE, py = CT_SPRING;
    for (int q = 1; q <= 6; ++q) {
        const float t = (float)q / 6, x = -CT_NAVE + 2.0f * CT_NAVE * t, y = CT_SPRING + (CT_APEX - CT_SPRING) * (1.0f - (2.0f * t - 1.0f) * (2.0f * t - 1.0f));
        gpTube(b, i, px, py, z0, x, y, z0, 0.36f, 0.36f, 3, CTM_STONE, CTM_STONEL);
        px = x; py = y;
    }
    if (lod) return;
    addBall(b, i, 0.0f, CT_APEX - 0.6f, zm, 0.6f, 0.4f, 0.6f, 2, 5, CTM_GOLD);
    gpTube(b, i, 0.0f, CT_APEX - 0.3f, z0, 0.0f, CT_APEX - 0.3f, z1, 0.3f, 0.3f, 3, CTM_STONE, CTM_STONE);
}
static int ctmBuildNave(LineVertex *b, int cap) {
    int i = 0; g_ctNChunk = 0;
    const float zA = CT_ZIN, zB = kCtPierZ[CT_NPIER - 1] - 12.0f;   // de la fachada al abside
    float zs[CT_NPIER + 2]; zs[0] = zA; for (int k = 0; k < CT_NPIER; ++k) zs[k + 1] = kCtPierZ[k]; zs[CT_NPIER + 1] = zB;
    for (int k = 0; k + 1 < CT_NPIER + 2 && i + 3200 < cap; ++k) {
        const int i0 = i; ctmBay(b, i, zs[k], zs[k + 1], k, 0);
        const int i1 = i; ctmBay(b, i, zs[k], zs[k + 1], k, 1);
        g_ctChunk[g_ctNChunk++] = { i0, i1 - i0, i1, i - i1, zs[k], zs[k + 1] };
    }
    // FACHADA (por dentro): muro con la PUERTA grande cerrada y un ROSETON
    {
        const int c0 = i;
        const unsigned int cf = gpShade(CTM_STONE, 0, 0, -1);
        gpPanel(b, i, -CT_HALF, 0.0f, zA, CT_HALF, 0.0f, zA, CT_HALF, CT_AISLE_H, zA, -CT_HALF, CT_AISLE_H, zA, cf, cf, 8.0f);
        gpPanel(b, i, -CT_NAVE, CT_AISLE_H, zA, CT_NAVE, CT_AISLE_H, zA, CT_NAVE, CT_SPRING, zA, -CT_NAVE, CT_SPRING, zA, cf, cf, 8.0f);
        gpTri(b, i, -CT_NAVE, CT_SPRING, zA, cf, CT_NAVE, CT_SPRING, zA, cf, 0.0f, CT_APEX, zA, cf);
        int i0 = i; gpArchSlab(b, i, 0.0f, 0.0f, 0.0f, 9.0f, 15.0f, 0.6f, 10, 1, RGBA(52, 38, 30, 255), RGBA(36, 26, 20, 255));   // puerta
        for (int q = i0; q < i; ++q) { b[q].x = -b[q].x; b[q].z = zA - 0.4f - b[q].z; }
        i0 = i; gpArchSlab(b, i, 0.0f, 0.0f, 0.0f, 10.6f, 16.4f, 0.5f, 10, 1, CTM_STONEL, CTM_STONE);                          // su marco
        for (int q = i0; q < i; ++q) { b[q].x = -b[q].x; b[q].z = zA - 0.2f - b[q].z; }
        gpRingXY(b, i, 0.0f, 30.0f, zA - 0.3f, 6.0f, 0.8f, 0.6f, 14, CTM_STONEL);
        static const unsigned int kRose[4] = { RGBA(40, 70, 170, 255), RGBA(170, 40, 46, 255), RGBA(200, 156, 50, 255), RGBA(110, 60, 170, 255) };
        for (int k = 0; k < 16; ++k) {
            const float a0 = GP_TAU * k / 16, a1 = GP_TAU * (k + 1) / 16;
            gpTri(b, i, 0.0f, 30.0f, zA - 0.25f, RGBA(220, 200, 150, 255), cosf(a0) * 5.6f, 30.0f + sinf(a0) * 5.6f, zA - 0.25f, kRose[k & 3], cosf(a1) * 5.6f, 30.0f + sinf(a1) * 5.6f, zA - 0.25f, kRose[k & 3]);
        }
        g_ctChunk[g_ctNChunk++] = { c0, i - c0, c0, i - c0, zA + 1.0f, zA - 1.0f };
    }
    // ABSIDE: medio decagono al fondo (continua los muros de la nave), suelo, gradas, el ALTAR con su
    // cruz y un roseton enorme en la cara del centro; muros que cierran las laterales y la cupula
    {
        const int c0 = i;
        const float cz = zB, R = CT_NAVE;
        for (int r = 0; r < 2; ++r)
            for (int gx = 0; gx < 3; ++gx) {
                const float x0 = -12.0f + gx * 8.0f, za = cz - r * 6.0f, zb = cz - (r + 1) * 6.0f;
                const unsigned int c = ((gx + r) & 1) ? RGBA(74, 70, 72, 255) : RGBA(138, 132, 124, 255);
                gpQuad(b, i, x0, 0.0f, za, c, x0 + 8.0f, 0.0f, za, c, x0 + 8.0f, 0.0f, zb, c, x0, 0.0f, zb, c);
            }
        for (int s = -1; s <= 1; s += 2) {
            const unsigned int c = gpShade(CTM_STONE, 0, 0, 1);
            gpPanel(b, i, s * CT_NAVE, 0.0f, cz, s * CT_HALF, 0.0f, cz, s * CT_HALF, CT_AISLE_H, cz, s * CT_NAVE, CT_AISLE_H, cz, c, c, 8.0f);
        }
        for (int k = 0; k < 5; ++k) {   // muros del abside (5 caras: la del medio mira de frente)
            const float a0 = GP_PI * k / 5, a1 = GP_PI * (k + 1) / 5;
            const float x0 = -cosf(a0) * R, z0 = cz - sinf(a0) * R, x1 = -cosf(a1) * R, z1 = cz - sinf(a1) * R;
            const float nx = -(cosf(a0) + cosf(a1)) * 0.5f, nz = -(sinf(a0) + sinf(a1)) * 0.5f;
            const unsigned int c = gpShade(CTM_STONE, -nx, 0, -nz), cd = gpShade(CTM_STONED, -nx, 0, -nz);
            gpPanel(b, i, x0, 0.0f, z0, x1, 0.0f, z1, x1, CT_APEX, z1, x0, CT_APEX, z0, cd, c, 8.0f);
            gpTri(b, i, x0, CT_APEX, z0, CTM_VAULT, x1, CT_APEX, z1, CTM_VAULT, 0.0f, CT_APEX + 3.0f, cz, CTM_VAULT);   // cupula
        }
        for (int k = 0; k < 3; ++k) gpBevelBox(b, i, 0.0f, 0.0f, cz - 1.4f - 1.4f * k, 14.0f - 2.0f * k, 2.8f, 0.6f * (k + 1), 0.12f, CTM_STONEL, CTM_STONE);   // gradas
        gpBevelBox(b, i, 0.0f, 1.8f, cz - 6.0f, 6.0f, 2.4f, 1.6f, 0.15f, RGBA(196, 188, 170, 255), CTM_STONE);   // altar
        gpBevelBox(b, i, 0.0f, 3.4f, cz - 6.0f, 6.6f, 2.8f, 0.25f, 0.08f, CTM_GOLD, CTM_GOLD);
        gpBevelBox(b, i, 0.0f, 1.8f, cz - 8.5f, 0.5f, 0.5f, 9.0f, 0.06f, CTM_GOLD, CTM_GOLD);                    // cruz alta
        gpBevelBox(b, i, 0.0f, 8.4f, cz - 8.5f, 4.2f, 0.5f, 0.5f, 0.06f, CTM_GOLD, CTM_GOLD);
        const float rz = cz - R * 0.951f + 0.6f;   // roseton en la cara del medio
        gpRingXY(b, i, 0.0f, 28.0f, rz, 8.0f, 1.0f, 0.8f, 12, CTM_STONEL);
        static const unsigned int kRose2[4] = { RGBA(170, 40, 46, 255), RGBA(40, 70, 170, 255), RGBA(110, 60, 170, 255), RGBA(200, 156, 50, 255) };
        for (int k = 0; k < 16; ++k) {
            const float a0 = GP_TAU * k / 16, a1 = GP_TAU * (k + 1) / 16;
            gpTri(b, i, 0.0f, 28.0f, rz + 0.1f, RGBA(240, 220, 170, 255), cosf(a0) * 7.5f, 28.0f + sinf(a0) * 7.5f, rz + 0.1f, kRose2[k & 3], cosf(a1) * 7.5f, 28.0f + sinf(a1) * 7.5f, rz + 0.1f, kRose2[k & 3]);
        }
        for (int k = 0; k < 8; ++k) {   // radios de piedra del roseton
            const float a = GP_TAU * k / 8;
            gpTube(b, i, 0.0f, 28.0f, rz + 0.2f, cosf(a) * 7.6f, 28.0f + sinf(a) * 7.6f, rz + 0.2f, 0.18f, 0.18f, 3, CTM_STONEL, CTM_STONEL);
        }
        g_ctChunk[g_ctNChunk++] = { c0, i - c0, c0, i - c0, cz, cz - R };
    }
    return i;
}

// ---------------------------------------------------------------- instancias de la catedral
enum { CTM_PIER, CTM_PIER_LO, CTM_WIN0, CTM_WIN1, CTM_WIN2, CTM_CLER0, CTM_CLER1, CTM_BELL, CTM_CHAND, CTM_SPEAR, CTM_VIGIA, CTM_N };
struct CtmModel { int off, n; };
static CtmModel g_ctm[CTM_N];
static int ctmBuildAll(LineVertex *pool, int cap) {
    int at = 0, n;
#define CTM_ADD(id, call) n = (call); if (n <= 0) return -1; g_ctm[id] = { at, n }; at += n;
    CTM_ADD(CTM_PIER, ctmBuildPier(pool + at, cap - at, 0));
    CTM_ADD(CTM_PIER_LO, ctmBuildPier(pool + at, cap - at, 1));
    CTM_ADD(CTM_WIN0, ctmBuildWindow(pool + at, cap - at, 4.0f, 13.0f, 0));
    CTM_ADD(CTM_WIN1, ctmBuildWindow(pool + at, cap - at, 4.0f, 13.0f, 1));
    CTM_ADD(CTM_WIN2, ctmBuildWindow(pool + at, cap - at, 4.0f, 13.0f, 2));
    CTM_ADD(CTM_CLER0, ctmBuildWindow(pool + at, cap - at, 3.2f, 8.0f, 0));
    CTM_ADD(CTM_CLER1, ctmBuildWindow(pool + at, cap - at, 3.2f, 8.0f, 2));
    CTM_ADD(CTM_BELL, ctmBuildBell(pool + at, cap - at));
    CTM_ADD(CTM_CHAND, ctmBuildChandelier(pool + at, cap - at));
    CTM_ADD(CTM_SPEAR, ctmBuildSpear(pool + at, cap - at));
    CTM_ADD(CTM_VIGIA, ctmBuildVigia(pool + at, cap - at));
#undef CTM_ADD
    return at;
}
struct CtmInst { unsigned char m; float x, y, z, yaw; };
// pilares, vitrales (de las laterales y del claristorio), aranas; las campanas las mueve el jefe
static int ctmLayout(CtmInst *out, int cap) {
    int n = 0;
    auto add = [&](int m, float x, float y, float z, float yaw) { if (n < cap) out[n++] = { (unsigned char)m, x, y, z, yaw }; };
    for (int k = 0; k < CT_NPIER; ++k) for (int s = -1; s <= 1; s += 2) add(CTM_PIER, s * CT_NAVE, 0.0f, kCtPierZ[k], 0.0f);
    for (int k = 0; k + 1 < CT_NPIER; ++k) {
        const float z = (kCtPierZ[k] + kCtPierZ[k + 1]) * 0.5f;
        for (int s = -1; s <= 1; s += 2) {
            add(CTM_WIN0 + (k % 3), s * (CT_HALF - 0.3f), 5.0f, z, s * GP_PI * 0.5f);                  // laterales (miran adentro)
            add(CTM_CLER0 + (k & 1), s * (CT_NAVE + 0.1f), CT_AISLE_H + 4.6f, z, s * GP_PI * 0.5f);    // claristorio
        }
    }
    for (int k = 0; k < 3; ++k) add(CTM_CHAND, 0.0f, 30.0f, 17.0f - k * 36.0f, 0.0f);
    return n;
}

static int ctmBuildByName(const char *name, LineVertex *b, int cap) {
    static const char *kN[CTM_N] = { "pilar", "pilar_lejos", "vitral0", "vitral1", "vitral2", "clarist0", "clarist1", "campana", "arana", "lanza", "vigia" };
    if (ctmBuildAll(b, cap) < 0) return 0;
    for (int m = 0; m < CTM_N; ++m) {
        const char *a = kN[m], *c = name; while (*a && *a == *c) { ++a; ++c; }
        if (!*a && !*c) { for (int k = 0; k < g_ctm[m].n; ++k) b[k] = b[g_ctm[m].off + k]; return g_ctm[m].n; }
    }
    return 0;
}

#ifdef BCM_PREVIEW
// para el visor de la PC: la nave con todo colocado y el Vigia de pie en el centro
static void ctmBuildScenePreview(LineVertex *out, int cap, float vigiaScale) {
    static LineVertex pool[30000];
    static CtmInst inst[100];
    ctmBuildNave(out, cap);
    if (ctmBuildAll(pool, 30000) < 0) fprintf(stderr, "el pool de la catedral no alcanza\n");
    const int n = ctmLayout(inst, 100);
    std::vector<LineVertex> all;
    for (int k = 0; k < g_ctNChunk; ++k) all.insert(all.end(), out + g_ctChunk[k].off, out + g_ctChunk[k].off + g_ctChunk[k].n);
    auto put = [&](int m, float x, float y, float z, float yaw, float sc) {
        const CtmModel &M = g_ctm[m]; const float c = cosf(yaw), s = sinf(yaw);
        for (int q = 0; q < M.n; ++q) {
            LineVertex v = pool[M.off + q]; const float lx = v.x * sc, lz = v.z * sc;
            v.x = x + c * lx + s * lz; v.y = y + v.y * sc; v.z = z - s * lx + c * lz; all.push_back(v);
        }
    };
    for (int k = 0; k < n; ++k) put(inst[k].m, inst[k].x, inst[k].y, inst[k].z, inst[k].yaw, 1.0f);
    for (int k = -1; k <= 1; ++k) put(CTM_BELL, k * 7.0f, 36.0f, CT_BELL_Z, 0.0f, 1.0f);
    put(CTM_VIGIA, 0.0f, 0.0f, -10.0f, 0.0f, vigiaScale);
    sceGumLoadIdentity(); sceGumDrawArray(GU_TRIANGLES, 0, (int)all.size(), 0, all.data());
}
#endif
