// ============================================================================
// boss_baphomet.h -- PISO 3 "LA CUEVA": jefe BAPHOMET (prefijo bp_ / bp / g_bp / BP_). Contrato: boss_api.h
//
// Cueva cerrada con fogata al centro. Baphomet (traje de gala negro, camisa blanca, cabeza de cabra)
// dialoga; se le pide ayuda o se le elogia el perfume (3 preguntas). Pelea: solo recibe dano mientras
// CARGA su fuego (parpadea). Ataques: bolas y ondas de fuego, sombras, atrapar la mente.
// Vencido: cae inconsciente -> minijuego del Padre Nuestro -> luz del cielo -> maldicion -> se esfuma.
// Personaje: partes en pose de reposo (coordenadas absolutas) dibujadas con matrices articuladas.
// ============================================================================
#pragma once

static const int   BP_HP     = 60;
static const float BP_HOME_R = 9.0f;       // radio al que rodea la fogata
static const float BP_FIREY  = 1.5f;       // altura del corazon del fuego
static const float BP_TAU    = 6.2831853f;
static const float BP_PIE    = 3.14159265f;

enum { BPS_IDLE, BPS_SAY, BPS_CHOICE, BPS_Q, BPS_MINDIN, BPS_FIGHT, BPS_KOFALL, BPS_PRAY, BPS_LIGHT, BPS_VANISH, BPS_DONE };
enum { BPN_CHOICE, BPN_Q, BPN_REWARD, BPN_NARR, BPN_MINDIN, BPN_PRAY, BPN_CURSE2, BPN_VANISH };
enum { BPA_NONE, BPA_ORBS, BPA_CHARGE, BPA_SHADOWS, BPA_WAVES, BPA_MIND };

// ---------------------------------------------------------------- estado
static int   g_bpSt = BPS_IDLE, g_bpStT = 0, g_bpT = 0, g_bpAnim = 0;
static float g_bpX = 0.0f, g_bpY = 0.0f, g_bpZ = -9.0f, g_bpYaw = 3.14159265f, g_bpScale = 1.0f;
static int   g_bpHp = BP_HP;
static const char *g_bpWho = "BAPHOMET", *g_bpLnA = "", *g_bpLnB = "";
static int   g_bpNext = 0, g_bpSel = 0, g_bpQ = 0, g_bpQok = 0, g_bpCurse = 0;
static int   g_bpAtk = BPA_NONE, g_bpAtkT = 0, g_bpPause = 0, g_bpSeq = 1, g_bpStun = 0;
static int   g_bpCharge = 0, g_bpHurt = 0, g_bpBounce = 0, g_bpOrbN = 0, g_bpOrbTot = 0;
static int   g_bpWaveT = 0, g_bpWaveHit = 0, g_bpWaveN = 0; static float g_bpWaveX = 0.0f, g_bpWaveZ = 0.0f;
static BxMash g_bpMash; static int g_bpTrap = 0, g_bpTrapT = 0;
static float g_bpTint = 0.0f, g_bpLight = 0.0f;
static float g_bpLpx = 0.0f, g_bpLpz = 0.0f; static int g_bpHaveLast = 0;
// pose (suavizada)
static float g_bpArm[2][3], g_bpBow = 0.0f, g_bpHy = 0.0f, g_bpLean = 0.0f, g_bpKneel = 0.0f, g_bpEyes = 1.0f;
// sombras
struct BpShade { float x, z, vx, vz; int life, age; };
static BpShade g_bpSh[4];
// oracion
static int  g_bpPStep = 0, g_bpPOpt[4], g_bpPCorrect = 0, g_bpPSel = 0, g_bpPErr = 0, g_bpPWrong = 0, g_bpPShake = 0;
static char g_bpPLn[24][100]; static int g_bpPLnN = 0;
// colision del escenario
static CityBldg *g_bpBx = 0; static int g_bpNbx = 0, g_bpBxCap = 0;

// ---------------------------------------------------------------- textos (UTF-8)
struct BpQ { const char *a; const char *opt[3]; int ok; const char *good; const char *bad; };
static const BpQ kBpQ[3] = {
    { "Dígame, ¿qué nota distingue bajo el azufre?",
      { "Huele a cabra mojada", "Rosas negras. Muy refinado.", "Lavanda y jabón de pino" }, 1,
      "Rosas negras... Pocos lo notan antes de morir.", "Qué decepción. Mi perfumista llorará esta noche." },
    { "Lo uso hace milenios. ¿Cuánto dura el aroma?",
      { "Hasta el amanecer, como todos", "Dos horas, tres si no suda", "Eterno. Como su elegancia" }, 2,
      "Eterno... qué amable. Me sonrojaría, si pudiera.", "Qué pena. Esperaba más de un alma tan fresca." },
    { "Un caballero sabe elogiar. ¿Qué opina de mi traje?",
      { "Negro como la medianoche. Impecable", "Algo abrigado para una cueva, ¿no?", "¿Es de alquiler?" }, 0,
      "Por fin alguien con ojo. Le deberé un favor.", "Qué descortés. Lo anotaré en su expediente." },
};
static const char *kBpPray[12] = {
    "Padre nuestro", "que estás en el cielo,", "santificado sea tu nombre;", "venga a nosotros tu reino;",
    "hágase tu voluntad", "en la tierra como en el cielo.", "Danos hoy nuestro pan de cada día;",
    "perdona nuestras ofensas,", "como también nosotros perdonamos a los que nos ofenden;",
    "no nos dejes caer en la tentación,", "y líbranos del mal.", "Amén."
};
static const int kBpSeq[8] = { BPA_ORBS, BPA_CHARGE, BPA_SHADOWS, BPA_CHARGE, BPA_WAVES, BPA_CHARGE, BPA_MIND, BPA_CHARGE };

// ---------------------------------------------------------------- utilidades
static float bpClampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
static float bpWrapPi(float a) { while (a > BP_PIE) a -= BP_TAU; while (a < -BP_PIE) a += BP_TAU; return a; }
static float bpRnd(int a, int b, unsigned int s) { return swRand(a, b, s); }
static float bpSg(int a, int b, unsigned int s) { return swRand(a, b, s) * 2.0f - 1.0f; }
static unsigned int bpC(int r, int g, int b, int a) {
    if (r < 0) r = 0;
    if (r > 255) r = 255;
    if (g < 0) g = 0;
    if (g > 255) g = 255;
    if (b < 0) b = 0;
    if (b > 255) b = 255;
    if (a < 0) a = 0;
    if (a > 255) a = 255;
    return (unsigned int)((a << 24) | (b << 16) | (g << 8) | r);
}
static float bpFlick(float a, float ph) { return 0.5f * sinf(a * 0.33f + ph) + 0.3f * sinf(a * 0.71f + 1.7f + ph) + 0.2f * sinf(a * 1.37f + 0.4f + ph * 2.0f); }
static float bpGlow(float x, float y, float z) { const float dy = y - BP_FIREY; return 1.0f / (1.0f + (x * x + dy * dy + z * z) * 0.028f); }

// buffer de vertices con tope
struct BpBuf { LineVertex *v; int n, cap; };
static void bpTriC(BpBuf &b, unsigned int ca, float ax, float ay, float az, unsigned int cb, float bx, float by, float bz,
                   unsigned int cc, float cx, float cy, float cz) {
    if (b.n + 3 > b.cap) return;
    b.v[b.n++] = { ca, ax, ay, az }; b.v[b.n++] = { cb, bx, by, bz }; b.v[b.n++] = { cc, cx, cy, cz };
}
static void bpQuad(BpBuf &b, float ax, float ay, float az, unsigned int ca, float bx, float by, float bz, unsigned int cb,
                   float cx, float cy, float cz, unsigned int cc, float dx, float dy, float dz, unsigned int cd) {
    if (b.n + 6 > b.cap) return;
    swQuadC(b.v, b.n, ax, ay, az, ca, bx, by, bz, cb, cx, cy, cz, cc, dx, dy, dz, cd);
}
static void bpLimb(BpBuf &b, float x0, float y0, float z0, float x1, float y1, float z1, float r0, float r1, int sides,
                   unsigned int cB, unsigned int cT) {
    if (sides < 3) sides = 3;
    if (sides > 16) sides = 16;
    if (b.n + sides * 12 > b.cap) return;
    addLimb(b.v, b.n, x0, y0, z0, x1, y1, z1, r0, r1, sides, cB, cT);
}
static void bpBall(BpBuf &b, float cx, float cy, float cz, float rx, float ry, float rz, int stacks, int slices, unsigned int col) {
    if (stacks < 2) stacks = 2;
    if (slices < 3) slices = 3;
    if (b.n + stacks * slices * 6 > b.cap) return;
    addBall(b.v, b.n, cx, cy, cz, rx, ry, rz, stacks, slices, col);
}
static void bpScaleAbout(BpBuf &b, int i0, int i1, float cx, float cy, float cz, float sx, float sy, float sz) {
    for (int k = i0; k < i1; ++k) {
        b.v[k].x = cx + (b.v[k].x - cx) * sx; b.v[k].y = cy + (b.v[k].y - cy) * sy; b.v[k].z = cz + (b.v[k].z - cz) * sz;
    }
}
static unsigned int bpRim(unsigned int c, float f) {
    int r = (int)(c & 0xFF), g = (int)((c >> 8) & 0xFF), b = (int)((c >> 16) & 0xFF);
    r = (int)(r * (1.0f + 0.10f * f) + 125.0f * f);
    g = (int)(g * (1.0f - 0.16f * f) + 46.0f * f);
    b = (int)(b * (1.0f - 0.45f * f) + 8.0f * f);
    return bpC(r, g, b, 255);
}
// luz calida de la fogata horneada en lo que mira al frente (-Z) y abajo
static void bpRimRange(BpBuf &b, int i0, int i1, float gain) {
    if (gain <= 0.0f) return;
    for (int k = i0; k < i1; ++k) {
        const float f = bpClampf((-b.v[k].z - 0.02f) * 1.5f, 0.0f, 1.0f);
        const float hy = bpClampf(1.15f - b.v[k].y * 0.16f, 0.35f, 1.1f);
        b.v[k].color = bpRim(b.v[k].color, f * hy * 0.8f * gain);
    }
}

// ---------------------------------------------------------------- roca facetada (luz de la fogata)
static unsigned int bpRockCol(unsigned int base, float lit, float warm) {
    const float rr = (float)(base & 0xFF), gg = (float)((base >> 8) & 0xFF), bb = (float)((base >> 16) & 0xFF);
    return bpC((int)(rr * lit * (1.0f + 0.45f * warm)), (int)(gg * lit * (1.0f + 0.08f * warm)), (int)(bb * lit * (1.0f - 0.32f * warm)), 255);
}
static void bpFacet(BpBuf &b, const float *p, const float *q, const float *r, unsigned int base, int seed) {
    if (b.n + 3 > b.cap) return;
    const float cx = (p[0] + q[0] + r[0]) * 0.33333f, cy = (p[1] + q[1] + r[1]) * 0.33333f, cz = (p[2] + q[2] + r[2]) * 0.33333f;
    const float ux = q[0] - p[0], uy = q[1] - p[1], uz = q[2] - p[2], vx = r[0] - p[0], vy = r[1] - p[1], vz = r[2] - p[2];
    float nx = uy * vz - uz * vy, ny = uz * vx - ux * vz, nz = ux * vy - uy * vx;
    const float nl = sqrtf(nx * nx + ny * ny + nz * nz) + 0.0001f;
    nx /= nl; ny /= nl; nz /= nl;
    const float lx = -cx, ly = BP_FIREY - cy, lz = -cz;
    const float ll = sqrtf(lx * lx + ly * ly + lz * lz) + 0.0001f;
    const float ndl = fabsf((nx * lx + ny * ly + nz * lz) / ll);
    const float gl = bpGlow(cx, cy, cz);
    float lit = (0.42f + 1.30f * gl * (0.40f + 0.60f * ndl)) * (0.82f + 0.36f * bpRnd(seed, 17, 9));
    lit *= 1.0f - 0.014f * bpClampf(cy, 0.0f, 30.0f);
    const unsigned int c = bpRockCol(base, lit, gl);
    b.v[b.n++] = { c, p[0], p[1], p[2] }; b.v[b.n++] = { c, q[0], q[1], q[2] }; b.v[b.n++] = { c, r[0], r[1], r[2] };
}
static void bpRock(BpBuf &b, float cx, float cy, float cz, float rx, float ry, float rz, int seed, unsigned int base) {
    if (b.n + 54 > b.cap) return;
    float rg[6][3], rb[6][3], ap[3];
    for (int k = 0; k < 6; ++k) {
        const float a = BP_TAU * (k + 0.35f * bpSg(seed, k, 1)) / 6.0f;
        const float rr = 1.0f + 0.22f * bpSg(seed, k, 2);
        const float x = cx + cosf(a) * rx * rr, z = cz + sinf(a) * rz * rr;
        rb[k][0] = x; rb[k][1] = cy; rb[k][2] = z;
        rg[k][0] = x; rg[k][1] = cy + ry * (0.35f + 0.25f * bpRnd(seed, k, 3)); rg[k][2] = z;
    }
    ap[0] = cx + rx * 0.25f * bpSg(seed, 7, 4); ap[1] = cy + ry; ap[2] = cz + rz * 0.25f * bpSg(seed, 8, 5);
    for (int k = 0; k < 6; ++k) {
        const int k2 = (k + 1) % 6;
        bpFacet(b, rg[k], rg[k2], ap, base, seed * 13 + k);
        bpFacet(b, rb[k], rb[k2], rg[k2], base, seed * 13 + k + 6);
        bpFacet(b, rb[k], rg[k2], rg[k], base, seed * 13 + k + 12);
    }
}
// h > 0 estalagmita, h < 0 estalactita (cuelga)
static void bpSpike(BpBuf &b, float cx, float cy, float cz, float r, float h, int seed, unsigned int base) {
    if (b.n + 45 > b.cap) return;
    float p0[5][3], p1[5][3], tip[3];
    for (int k = 0; k < 5; ++k) {
        const float a = BP_TAU * (k + 0.3f * bpSg(seed, k, 11)) / 5.0f;
        p0[k][0] = cx + cosf(a) * r; p0[k][1] = cy; p0[k][2] = cz + sinf(a) * r;
        p1[k][0] = cx + cosf(a) * r * 0.55f + 0.1f * r * bpSg(seed, k, 12); p1[k][1] = cy + h * 0.5f; p1[k][2] = cz + sinf(a) * r * 0.55f;
    }
    tip[0] = cx + 0.2f * r * bpSg(seed, 3, 13); tip[1] = cy + h; tip[2] = cz + 0.2f * r * bpSg(seed, 4, 14);
    for (int k = 0; k < 5; ++k) {
        const int k2 = (k + 1) % 5;
        bpFacet(b, p0[k], p0[k2], p1[k2], base, seed * 7 + k);
        bpFacet(b, p0[k], p1[k2], p1[k], base, seed * 7 + k + 5);
        bpFacet(b, p1[k], p1[k2], tip, base, seed * 7 + k + 10);
    }
}

// ---------------------------------------------------------------- escenario
static const int BP_NS = 32, BP_NR = 8;
static const float kBpShY[8] = { 0.0f, 2.5f, 6.0f, 10.0f, 14.0f, 17.5f, 20.5f, 22.8f };
static const float kBpShR[8] = { 24.0f, 25.2f, 24.2f, 21.5f, 17.0f, 11.5f, 6.0f, 1.0f };

static void bpBox(float x, float z, float w, float d, float h, unsigned int col) {
    if (!g_bpBx || g_bpNbx >= g_bpBxCap) return;
    CityBldg &c = g_bpBx[g_bpNbx++];
    c.x = x; c.z = z; c.w = w; c.d = d; c.h = h; c.color = col;
}
static float bpCeilY(float rho) {
    for (int k = 0; k < BP_NR - 1; ++k)
        if (rho <= kBpShR[k] && rho >= kBpShR[k + 1]) {
            const float t = (kBpShR[k] - rho) / (kBpShR[k] - kBpShR[k + 1]);
            return kBpShY[k] + (kBpShY[k + 1] - kBpShY[k]) * t;
        }
    return kBpShY[BP_NR - 1];
}
static void bpBuildFloor(BpBuf &b) {
    static const float RR[10] = { 0.0f, 1.5f, 3.2f, 5.2f, 8.0f, 11.0f, 14.5f, 18.0f, 21.5f, 26.5f };
    const int NS = 24;
    float pt[10][24][3];
    for (int k = 0; k < 10; ++k)
        for (int j = 0; j < NS; ++j) {
            const float a = BP_TAU * (j + ((k & 1) ? 0.5f : 0.0f) + 0.12f * bpSg(j, k, 31)) / NS;
            const float r = (k == 0 || k == 9) ? RR[k] : RR[k] + 0.3f * bpSg(j, k, 32);
            pt[k][j][0] = cosf(a) * r; pt[k][j][1] = 0.0f; pt[k][j][2] = sinf(a) * r;
        }
    for (int k = 0; k < 9; ++k)
        for (int j = 0; j < NS; ++j) {
            const int j2 = (j + 1) % NS;
            const unsigned int base = (k < 2) ? bpC(46, 38, 32, 255) : (k < 3 ? bpC(78, 66, 56, 255) : bpC(104, 92, 80, 255));
            bpFacet(b, pt[k][j], pt[k + 1][j], pt[k + 1][j2], base, k * 50 + j);
            if (k > 0) bpFacet(b, pt[k][j], pt[k + 1][j2], pt[k][j2], base, k * 50 + j + 25);
        }
}
static void bpShellPt(int j, int k, float *o) {
    const int jj = j % BP_NS;
    const float a = BP_TAU * (jj + 0.25f * bpSg(jj, k, 41)) / BP_NS;
    float r = kBpShR[k] + ((k == 0) ? 0.9f : (1.0f + 0.35f * k)) * bpSg(jj, k, 42);
    float y = kBpShY[k] + ((k == 0) ? 0.0f : 0.9f * bpSg(jj, k, 43));
    if (k == BP_NR - 1) { r = 1.0f; y = kBpShY[k]; }
    o[0] = cosf(a) * r; o[1] = y; o[2] = sinf(a) * r;
}
static void bpBuildShell(BpBuf &b) {
    const unsigned int base = bpC(112, 94, 78, 255);
    float top[3] = { 0.0f, 23.6f, 0.0f };
    for (int j = 0; j < BP_NS; ++j) {
        for (int k = 0; k < BP_NR - 1; ++k) {
            float p00[3], p10[3], p01[3], p11[3];
            bpShellPt(j, k, p00); bpShellPt(j + 1, k, p10); bpShellPt(j, k + 1, p01); bpShellPt(j + 1, k + 1, p11);
            bpFacet(b, p00, p10, p11, base, j * 20 + k);
            bpFacet(b, p00, p11, p01, base, j * 20 + k + 10);
        }
        float a[3], c[3];
        bpShellPt(j, BP_NR - 1, a); bpShellPt(j + 1, BP_NR - 1, c);
        bpFacet(b, a, c, top, base, j + 700);
    }
}
static void bpBuildPit(BpBuf &b) {
    for (int i = 0; i < 8; ++i) {
        const float a = BP_TAU * i / 8.0f + 0.2f;
        bpRock(b, cosf(a) * 1.9f, 0.0f, sinf(a) * 1.9f, 0.62f, 0.62f + 0.2f * bpRnd(i, 1, 61), 0.58f, 200 + i, bpC(98, 92, 88, 255));
    }
    for (int i = 0; i < 5; ++i) {
        const float a = BP_TAU * i / 5.0f + 0.4f;
        bpLimb(b, cosf(a) * 1.45f, 0.26f, sinf(a) * 1.45f, cosf(a) * 0.18f, 1.9f, sinf(a) * 0.18f, 0.27f, 0.20f, 6,
               bpC(58, 36, 22, 255), bpC(150, 62, 18, 255));
    }
    bpLimb(b, -1.25f, 0.32f, 0.55f, 1.2f, 0.36f, -0.5f, 0.30f, 0.28f, 6, bpC(50, 32, 20, 255), bpC(110, 48, 16, 255));
    bpLimb(b, 0.7f, 0.30f, 1.2f, -0.8f, 0.34f, -1.0f, 0.28f, 0.27f, 6, bpC(50, 32, 20, 255), bpC(110, 48, 16, 255));
}
static void bpBuildRocks(BpBuf &b) {
    for (int i = 0; i < 20; ++i) {            // pie de la pared
        const float a = BP_TAU * (i + 0.5f * bpSg(i, 3, 51)) / 20.0f;
        const float r = 23.2f + 1.4f * bpRnd(i, 4, 52), s = 1.2f + 0.7f * bpRnd(i, 5, 53);
        bpRock(b, cosf(a) * r, 0.0f, sinf(a) * r, s, s * (1.0f + 1.2f * bpRnd(i, 6, 54)), s * 0.9f, 100 + i, bpC(112, 92, 76, 255));
    }
    for (int i = 0; i < 12; ++i) {            // guijarros sueltos
        const float a = BP_TAU * bpRnd(i, 9, 55), r = 4.0f + 16.0f * bpRnd(i, 10, 56), s = 0.25f + 0.3f * bpRnd(i, 11, 57);
        bpRock(b, cosf(a) * r, 0.0f, sinf(a) * r, s, s * 0.8f, s, 300 + i, bpC(100, 86, 72, 255));
    }
    for (int i = 0; i < 26; ++i) {            // estalactitas
        const float a = BP_TAU * bpRnd(i, 12, 58), rho = 3.0f + 17.0f * bpRnd(i, 13, 59);
        const float len = 2.5f + 3.5f * bpRnd(i, 14, 60), r = 0.5f + 0.7f * bpRnd(i, 15, 62);
        bpSpike(b, cosf(a) * rho, bpCeilY(rho) - 0.6f, sinf(a) * rho, r, -len, 400 + i, bpC(104, 88, 74, 255));
    }
    for (int i = 0; i < 8; ++i) {             // estalagmitas (con colision)
        const float a = BP_TAU * (i + 0.37f) / 8.0f + 0.2f * bpSg(i, 16, 63), rho = 16.5f + 3.5f * bpRnd(i, 17, 64);
        const float x = cosf(a) * rho, z = sinf(a) * rho;
        if (fabsf(x) < 3.5f && z > 6.0f) continue;       // no tapar el camino del spawn
        const float h = 1.8f + 2.4f * bpRnd(i, 18, 65), r = 0.6f + 0.5f * bpRnd(i, 19, 66);
        bpSpike(b, x, 0.0f, z, r, h, 500 + i, bpC(108, 90, 76, 255));
        bpBox(x, z, r * 1.6f, r * 1.6f, h * 0.7f, bpC(108, 90, 76, 255));
    }
}

// ---------------------------------------------------------------- personaje: partes en reposo
static const float kBpTY[6] = { 2.05f, 2.30f, 2.75f, 3.35f, 3.80f, 4.02f };
static const float kBpTR[6] = { 0.50f, 0.45f, 0.42f, 0.54f, 0.60f, 0.24f };
static LineVertex __attribute__((aligned(16))) g_bpTorso[2][1200];
static LineVertex __attribute__((aligned(16))) g_bpHeadV[2][1700];
static LineVertex __attribute__((aligned(16))) g_bpEyeV[240];
static LineVertex __attribute__((aligned(16))) g_bpUpArm[2][300];
static LineVertex __attribute__((aligned(16))) g_bpForeArm[2][520];
static LineVertex __attribute__((aligned(16))) g_bpThigh[2][240];
static LineVertex __attribute__((aligned(16))) g_bpShin[2][420];
static LineVertex __attribute__((aligned(16))) g_bpShadeV[700];
static int g_bpTorsoN = 0, g_bpHeadN = 0, g_bpEyeN = 0, g_bpUpN = 0, g_bpForeN = 0, g_bpThighN = 0, g_bpShinN = 0, g_bpShadeN = 0;

static unsigned int bpV(unsigned int c, int hot, float k) { return hot ? cp_lerp(c, bpC(255, 112, 30, 255), k) : c; }
static float bpTorsoR(float y) {
    if (y <= kBpTY[0]) return kBpTR[0];
    for (int k = 0; k < 5; ++k)
        if (y <= kBpTY[k + 1]) { const float t = (y - kBpTY[k]) / (kBpTY[k + 1] - kBpTY[k]); return kBpTR[k] + (kBpTR[k + 1] - kBpTR[k]) * t; }
    return kBpTR[5];
}
static float bpFrontZ(float x, float y, float off) {
    const float r = bpTorsoR(y), hx = r * 1.32f, hz = r * 0.74f;
    const float u = bpClampf(x / hx, -0.98f, 0.98f);
    return -hz * sqrtf(1.0f - u * u) - off;
}
// franja pegada al frente del torso (curvada) entre las alturas ya0 e yb0, con bordes x lineales en y
static void bpFrontBand(BpBuf &b, float yA, float yB, float xi0, float xi1, float xo0, float xo1, float off, unsigned int cA, unsigned int cB) {
    static const float ky[4] = { 2.75f, 3.35f, 3.80f, 3.97f };
    for (int s = 0; s < 3; ++s) {
        const float y0 = ky[s], y1 = ky[s + 1];
        if (y1 < yA || y0 > yB) continue;
        const float t0 = (y0 - yA) / (yB - yA), t1 = (y1 - yA) / (yB - yA);
        const float i0 = xi0 + (xi1 - xi0) * t0, i1 = xi0 + (xi1 - xi0) * t1, o0 = xo0 + (xo1 - xo0) * t0, o1 = xo0 + (xo1 - xo0) * t1;
        const unsigned int c0 = cp_lerp(cA, cB, t0), c1 = cp_lerp(cA, cB, t1);
        for (int k = 0; k < 2; ++k) {
            const float f0 = k * 0.5f, f1 = (k + 1) * 0.5f;
            const float a0 = i0 + (o0 - i0) * f0, a1 = i0 + (o0 - i0) * f1, b0 = i1 + (o1 - i1) * f0, b1 = i1 + (o1 - i1) * f1;
            bpQuad(b, a0, y0, bpFrontZ(a0, y0, off), c0, a1, y0, bpFrontZ(a1, y0, off), c0,
                      b1, y1, bpFrontZ(b1, y1, off), c1, b0, y1, bpFrontZ(b0, y1, off), c1);
        }
    }
}
static void bpBuildTorso(BpBuf &b, int hot) {
    const unsigned int JK = bpV(bpC(26, 24, 32, 255), hot, 0.80f), JK2 = bpV(bpC(40, 38, 50, 255), hot, 0.80f);
    const unsigned int SAT = bpV(bpC(58, 56, 70, 255), hot, 0.80f), FUR = bpV(bpC(26, 22, 27, 255), hot, 0.80f);
    const unsigned int WHT = bpV(bpC(244, 242, 236, 255), hot, 0.45f), WHD = bpV(bpC(198, 196, 196, 255), hot, 0.45f);
    const int i0 = b.n;
    if (b.n + 480 <= b.cap) { addLoft(b.v, b.n, 0.0f, 0.0f, kBpTY, kBpTR, 6, 8, JK, JK2); bpScaleAbout(b, i0, b.n, 0.0f, 0.0f, 0.0f, 1.32f, 1.0f, 0.74f); }
    const int ib = b.n;
    bpLimb(b, 0.0f, 2.26f, 0.0f, 0.0f, 2.60f, 0.0f, 0.505f, 0.46f, 8, JK2, SAT);          // faja
    bpScaleAbout(b, ib, b.n, 0.0f, 0.0f, 0.0f, 1.32f, 1.0f, 0.74f);
    for (int s = -1; s <= 1; s += 2) {                                                     // faldones del frac
        const int t0 = b.n;
        bpLimb(b, s * 0.26f, 2.34f, 0.28f, s * 0.30f, 1.22f, 0.50f, 0.25f, 0.19f, 6, JK, JK);
        bpScaleAbout(b, t0, b.n, s * 0.28f, 0.0f, 0.39f, 1.0f, 1.0f, 0.32f);
    }
    bpLimb(b, 0.0f, 3.98f, 0.0f, 0.0f, 4.30f, 0.0f, 0.22f, 0.17f, 7, FUR, FUR);            // cuello
    bpLimb(b, 0.0f, 3.97f, 0.0f, 0.0f, 4.10f, 0.0f, 0.235f, 0.215f, 8, WHT, WHT);         // cuello de camisa
    bpFrontBand(b, 2.72f, 3.98f, -0.20f, -0.14f, 0.20f, 0.14f, 0.07f, WHT, WHD);           // camisa blanca
    for (int s = -1; s <= 1; s += 2) {                                                     // solapas de raso
        bpFrontBand(b, 2.95f, 3.97f, s * 0.15f, s * 0.17f, s * 0.34f, s * 0.52f, 0.11f, SAT, JK2);
    }
    const float zk = bpFrontZ(0.0f, 3.95f, 0.15f);                                         // lazo negro
    bpTriC(b, SAT, 0.0f, 3.95f, zk, JK2, -0.30f, 4.05f, bpFrontZ(0.3f, 4.0f, 0.12f), JK2, -0.30f, 3.85f, bpFrontZ(0.3f, 3.9f, 0.12f));
    bpTriC(b, SAT, 0.0f, 3.95f, zk, JK2, 0.30f, 4.05f, bpFrontZ(0.3f, 4.0f, 0.12f), JK2, 0.30f, 3.85f, bpFrontZ(0.3f, 3.9f, 0.12f));
    bpQuad(b, -0.04f, 3.99f, zk - 0.01f, JK, 0.04f, 3.99f, zk - 0.01f, JK, 0.04f, 3.91f, zk - 0.01f, JK, -0.04f, 3.91f, zk - 0.01f, JK);
    for (int k = 0; k < 3; ++k) {                                                          // botones
        const float y = 3.70f - 0.28f * k, z = bpFrontZ(0.0f, y, 0.09f);
        bpQuad(b, -0.03f, y, z, JK, 0.0f, y + 0.03f, z, JK, 0.03f, y, z, JK, 0.0f, y - 0.03f, z, JK);
    }
    {   const float zp = bpFrontZ(-0.54f, 3.5f, 0.07f);                                    // panuelo de bolsillo
        bpTriC(b, WHT, -0.62f, 3.45f, zp, WHT, -0.46f, 3.45f, zp, WHD, -0.54f, 3.60f, zp);
        bpTriC(b, WHD, -0.62f, 3.45f, zp, WHT, -0.54f, 3.60f, zp, WHT, -0.58f, 3.52f, zp); }
    bpRimRange(b, i0, b.n, hot ? 0.0f : 1.0f);
}
static void bpBuildHead(BpBuf &b, int hot) {
    const unsigned int FUR = bpV(bpC(30, 26, 31, 255), hot, 0.78f), FUR2 = bpV(bpC(44, 38, 44, 255), hot, 0.78f), DRK = bpV(bpC(14, 12, 16, 255), hot, 0.78f);
    const unsigned int H0 = bpV(bpC(30, 25, 27, 255), hot, 0.70f), H1 = bpV(bpC(88, 74, 64, 255), hot, 0.70f);
    const int i0 = b.n;
    bpBall(b, 0.0f, 4.62f, -0.02f, 0.33f, 0.36f, 0.34f, 5, 8, FUR);                         // craneo
    bpLimb(b, 0.0f, 4.52f, -0.24f, 0.0f, 4.31f, -0.80f, 0.21f, 0.12f, 7, FUR2, FUR2);       // hocico
    bpBall(b, 0.0f, 4.31f, -0.84f, 0.13f, 0.10f, 0.09f, 3, 6, DRK);                         // nariz
    bpLimb(b, 0.0f, 4.34f, -0.30f, 0.0f, 4.12f, -0.68f, 0.14f, 0.08f, 6, FUR, FUR2);        // mandibula
    bpLimb(b, 0.0f, 4.16f, -0.60f, 0.0f, 3.62f, -0.50f, 0.10f, 0.015f, 5, FUR, DRK);        // barba
    for (int s = -1; s <= 1; s += 2) {
        bpLimb(b, s * 0.28f, 4.72f, 0.02f, s * 0.80f, 4.60f, 0.10f, 0.10f, 0.035f, 5, FUR, FUR2);   // orejas
        bpLimb(b, s * 0.05f, 4.73f, -0.31f, s * 0.31f, 4.86f, -0.19f, 0.06f, 0.07f, 5, DRK, DRK);   // ceja airada
        float px = s * 0.20f, py = 4.88f, pz = 0.02f;                                      // cuerno curvo hacia atras
        for (int k = 1; k <= 6; ++k) {
            const float t = k / 6.0f, tp = (k - 1) / 6.0f;
            const float nx = s * (0.20f + 0.55f * t - 0.38f * t * t), ny = 4.88f + 0.95f * t - 0.28f * t * t, nz = 0.02f + 1.05f * t * t;
            bpLimb(b, px, py, pz, nx, ny, nz, 0.13f * (1.0f - tp) + 0.025f, 0.13f * (1.0f - t) + 0.025f, 5, cp_lerp(H0, H1, tp), cp_lerp(H0, H1, t));
            px = nx; py = ny; pz = nz;
        }
    }
    bpRimRange(b, i0, b.n, hot ? 0.0f : 1.0f);
}
static void bpBuildEyes(BpBuf &b) {
    for (int s = -1; s <= 1; s += 2) bpBall(b, s * 0.17f, 4.63f, -0.30f, 0.085f, 0.052f, 0.05f, 3, 6, bpC(255, 46, 30, 255));
}
static void bpBuildUpArm(BpBuf &b, int hot) {
    const unsigned int JK = bpV(bpC(26, 24, 32, 255), hot, 0.80f), JK2 = bpV(bpC(40, 38, 50, 255), hot, 0.80f);
    const int i0 = b.n;
    bpBall(b, 0.80f, 3.88f, 0.0f, 0.26f, 0.24f, 0.24f, 4, 7, JK);
    bpLimb(b, 0.80f, 3.86f, 0.0f, 0.80f, 2.95f, 0.0f, 0.20f, 0.16f, 7, JK, JK2);
    bpRimRange(b, i0, b.n, hot ? 0.0f : 0.8f);
}
static void bpBuildForeArm(BpBuf &b, int hot) {
    const unsigned int JK = bpV(bpC(26, 24, 32, 255), hot, 0.80f), JK2 = bpV(bpC(40, 38, 50, 255), hot, 0.80f);
    const unsigned int WHT = bpV(bpC(244, 242, 236, 255), hot, 0.45f), GLV = bpV(bpC(16, 14, 18, 255), hot, 0.80f);
    const int i0 = b.n;
    bpBall(b, 0.80f, 2.95f, 0.0f, 0.17f, 0.17f, 0.17f, 3, 6, JK);
    bpLimb(b, 0.80f, 2.95f, 0.0f, 0.80f, 2.20f, 0.0f, 0.16f, 0.125f, 7, JK, JK2);
    bpLimb(b, 0.80f, 2.24f, 0.0f, 0.80f, 2.12f, 0.0f, 0.135f, 0.14f, 7, WHT, WHT);          // puno blanco
    bpBall(b, 0.80f, 1.95f, -0.01f, 0.12f, 0.20f, 0.10f, 4, 6, GLV);                        // mano
    bpLimb(b, 0.73f, 2.06f, -0.07f, 0.70f, 1.86f, -0.15f, 0.04f, 0.03f, 4, GLV, GLV);       // pulgar
    bpRimRange(b, i0, b.n, hot ? 0.0f : 0.8f);
}
static void bpBuildThigh(BpBuf &b, int hot) {
    const unsigned int TR = bpV(bpC(22, 20, 27, 255), hot, 0.80f), TR2 = bpV(bpC(32, 30, 40, 255), hot, 0.80f);
    const int i0 = b.n;
    bpBall(b, 0.32f, 2.18f, 0.0f, 0.25f, 0.22f, 0.24f, 3, 6, TR);
    bpLimb(b, 0.32f, 2.18f, 0.0f, 0.30f, 1.15f, 0.0f, 0.24f, 0.17f, 7, TR, TR2);
    bpRimRange(b, i0, b.n, hot ? 0.0f : 0.8f);
}
static void bpBuildShin(BpBuf &b, int hot) {
    const unsigned int TR = bpV(bpC(22, 20, 27, 255), hot, 0.80f), TR2 = bpV(bpC(32, 30, 40, 255), hot, 0.80f), SH = bpV(bpC(12, 10, 14, 255), hot, 0.80f);
    const int i0 = b.n;
    bpBall(b, 0.30f, 1.15f, 0.0f, 0.18f, 0.18f, 0.18f, 3, 6, TR);
    bpLimb(b, 0.30f, 1.15f, 0.0f, 0.30f, 0.30f, 0.0f, 0.17f, 0.11f, 7, TR, TR2);
    bpBall(b, 0.30f, 0.17f, -0.16f, 0.14f, 0.12f, 0.30f, 4, 7, SH);                         // zapato
    bpRimRange(b, i0, b.n, hot ? 0.0f : 0.9f);
}
// clon de sombra: capa negra con cuernos y ojos rojos
static void bpBuildShade(BpBuf &b) {
    static const float ys[6] = { 0.0f, 0.5f, 1.6f, 2.6f, 3.2f, 3.45f };
    static const float rs[6] = { 0.95f, 0.62f, 0.48f, 0.55f, 0.44f, 0.18f };
    const unsigned int K0 = bpC(8, 5, 14, 255), K1 = bpC(30, 18, 44, 255);
    const int i0 = b.n;
    addLoft(b.v, b.n, 0.0f, 0.0f, ys, rs, 6, 5, K0, K1);
    bpScaleAbout(b, i0, b.n, 0.0f, 0.0f, 0.0f, 1.15f, 1.0f, 0.8f);
    bpBall(b, 0.0f, 3.85f, 0.0f, 0.28f, 0.34f, 0.30f, 3, 6, K0);
    for (int s = -1; s <= 1; s += 2) {
        bpLimb(b, s * 0.15f, 4.05f, 0.0f, s * 0.35f, 4.55f, 0.15f, 0.07f, 0.02f, 4, K0, K1);
        bpLimb(b, s * 0.45f, 3.0f, 0.0f, s * 0.55f, 2.3f, -0.9f, 0.12f, 0.07f, 4, K0, K1);
        bpQuad(b, s * 0.12f - 0.05f, 3.90f, -0.27f, bpC(255, 50, 40, 255), s * 0.12f + 0.05f, 3.90f, -0.27f, bpC(255, 50, 40, 255),
               s * 0.12f + 0.05f, 3.84f, -0.27f, bpC(255, 50, 40, 255), s * 0.12f - 0.05f, 3.84f, -0.27f, bpC(255, 50, 40, 255));
    }
}
static void bpBuildChar() {
    for (int h = 0; h < 2; ++h) {
        BpBuf t = { g_bpTorso[h], 0, 1200 }; bpBuildTorso(t, h); g_bpTorsoN = t.n;
        BpBuf hd = { g_bpHeadV[h], 0, 1700 }; bpBuildHead(hd, h); g_bpHeadN = hd.n;
        BpBuf u = { g_bpUpArm[h], 0, 300 }; bpBuildUpArm(u, h); g_bpUpN = u.n;
        BpBuf f = { g_bpForeArm[h], 0, 520 }; bpBuildForeArm(f, h); g_bpForeN = f.n;
        BpBuf th = { g_bpThigh[h], 0, 240 }; bpBuildThigh(th, h); g_bpThighN = th.n;
        BpBuf sh = { g_bpShin[h], 0, 420 }; bpBuildShin(sh, h); g_bpShinN = sh.n;
    }
    BpBuf e = { g_bpEyeV, 0, 240 }; bpBuildEyes(e); g_bpEyeN = e.n;
    BpBuf s = { g_bpShadeV, 0, 700 }; bpBuildShade(s); g_bpShadeN = s.n;
}

// ---------------------------------------------------------------- estado: reinicio y helpers de logica
static void bpClearAttacks() {
    g_bpAtk = BPA_NONE; g_bpAtkT = 0; g_bpCharge = 0; g_bpTrap = 0; g_bpMash.frames = 0; g_bpOrbN = 0; g_bpWaveT = 0;
    for (int i = 0; i < 4; ++i) g_bpSh[i].life = 0;
}
static void bpResetAll() {
    g_bpSt = BPS_IDLE; g_bpStT = 0; g_bpT = 0; g_bpAnim = 0; g_bpX = 0.0f; g_bpY = 0.0f; g_bpZ = -BP_HOME_R;
    g_bpYaw = BP_PIE; g_bpScale = 1.0f; g_bpHp = BP_HP; g_bpCurse = 0; g_bpQ = 0; g_bpQok = 0; g_bpSel = 0;
    g_bpPause = 100; g_bpSeq = 1; g_bpStun = 0; g_bpHurt = 0; g_bpBounce = 0; g_bpTint = 0.0f; g_bpLight = 0.0f; g_bpHaveLast = 0;
    g_bpPStep = 0; g_bpPErr = 0; g_bpPWrong = 0; g_bpPShake = 0; g_bpPLnN = 0;
    for (int k = 0; k < 3; ++k) { g_bpArm[0][k] = (k == 0) ? 0.2f : (k == 1 ? 0.22f : 0.55f); g_bpArm[1][k] = g_bpArm[0][k]; }
    g_bpBow = 0.04f; g_bpHy = 0.0f; g_bpLean = 0.0f; g_bpKneel = 0.0f; g_bpEyes = 1.0f;
    bpClearAttacks();
}
static void bpSay(const char *who, const char *a, const char *b, int next) {
    g_bpWho = who; g_bpLnA = a; g_bpLnB = b ? b : ""; g_bpNext = next; g_bpSt = BPS_SAY; g_bpStT = 0;
}
static void bpWrap(const char *text, int maxCols) {
    g_bpPLnN = 0;
    char line[100]; int ll = 0, cols = 0;
    const char *p = text;
    while (*p && g_bpPLnN < 20) {
        const char *q = p; int wcols = 0;
        while (*q && *q != ' ') { if ((((unsigned char)*q) & 0xC0) != 0x80) ++wcols; ++q; }
        const int wbytes = (int)(q - p), need = (cols > 0 ? 1 : 0) + wcols;
        if (cols > 0 && cols + need > maxCols) {
            line[ll] = 0; for (int k = 0; k <= ll; ++k) g_bpPLn[g_bpPLnN][k] = line[k];
            ++g_bpPLnN; ll = 0; cols = 0;
        }
        if (cols > 0) { line[ll++] = ' '; ++cols; }
        for (int k = 0; k < wbytes && ll < 94; ++k) line[ll++] = p[k];
        cols += wcols;
        p = q; while (*p == ' ') ++p;
    }
    if (ll > 0 && g_bpPLnN < 20) { line[ll] = 0; for (int k = 0; k <= ll; ++k) g_bpPLn[g_bpPLnN][k] = line[k]; ++g_bpPLnN; }
}
static void bpPrayRebuild() {
    static char buf[600]; int n = 0;
    for (int i = 0; i < g_bpPStep && i < 12; ++i) {
        if (i > 0 && n < 590) buf[n++] = ' ';
        for (const char *s = kBpPray[i]; *s && n < 590; ++s) buf[n++] = *s;
    }
    buf[n] = 0;
    bpWrap(buf, 50);
}
static void bpPrayOptions() {
    int opts[3], n = 0;
    for (int tries = 1; n < 3 && tries < 80; ++tries) {
        const int c = (int)(swRand(g_bpT, g_bpPStep * 7 + tries, 71) * 12.0f) % 12;
        if (c == g_bpPStep) continue;
        bool dup = false; for (int k = 0; k < n; ++k) if (opts[k] == c) dup = true;
        if (!dup) opts[n++] = c;
    }
    for (int c = 0; n < 3 && c < 12; ++c) {
        if (c == g_bpPStep) continue;
        bool dup = false; for (int k = 0; k < n; ++k) if (opts[k] == c) dup = true;
        if (!dup) opts[n++] = c;
    }
    const int pos = (int)(swRand(g_bpT, g_bpPStep, 72) * 4.0f) % 4;
    int w = 0;
    for (int k = 0; k < 4; ++k) g_bpPOpt[k] = (k == pos) ? g_bpPStep : opts[w++];
    g_bpPCorrect = pos; g_bpPSel = 0; g_bpPWrong = 0;
}
static void bpEnterKO() {
    g_bpSt = BPS_KOFALL; g_bpStT = 0; bpClearAttacks();
    lvMessage("BAPHOMET CAE DE RODILLAS", 150); g_lvFlash = 10;
}
static void bpAdvance(int code) {
    switch (code) {
    case BPN_CHOICE: g_bpSt = BPS_CHOICE; g_bpStT = 0; g_bpSel = 0; break;
    case BPN_Q: g_bpSt = BPS_Q; g_bpStT = 0; g_bpSel = 0; break;
    case BPN_REWARD:
        if (g_bpQok >= 2) {
            g_lvGiveTimePower = 1;
            lvMessage("BAPHOMET TE CONCEDE: DETENER EL TIEMPO (CUADRADO)", 280);
            bpSay("BAPHOMET", "Ha sido encantador. Acepte este obsequio:", "el tiempo, a su antojo.", BPN_NARR);
        } else bpSay("BAPHOMET", "Qué aburrimiento. No merece nada de mí.", "", BPN_NARR);
        break;
    case BPN_NARR: bpSay("", "Sientes que está controlando tu mente...", "", BPN_MINDIN); break;
    case BPN_MINDIN: g_bpSt = BPS_MINDIN; g_bpStT = 0; break;
    case BPN_PRAY: g_bpSt = BPS_PRAY; g_bpStT = 0; g_bpPStep = 0; g_bpPErr = 0; bpPrayRebuild(); bpPrayOptions(); break;
    case BPN_CURSE2: bpSay("BAPHOMET", "Jamás dormirás tranquilo. Nos veremos... abajo.", "", BPN_VANISH); break;
    case BPN_VANISH: g_bpSt = BPS_VANISH; g_bpStT = 0; lvMessage("BAPHOMET SE ESFUMA", 150); g_lvFlash = 12; break;
    default: break;
    }
}

// ---------------------------------------------------------------- ataques
static void bpWaveStart(float x, float z, int n) { g_bpWaveT = 1; g_bpWaveHit = 0; g_bpWaveN = n; g_bpWaveX = x; g_bpWaveZ = z; }
static float bpWaveR(int w) { const int el = g_bpWaveT - w * 42; return el > 0 ? el * 0.34f : -1.0f; }
static void bpEndAtk(int pause) { g_bpAtk = BPA_NONE; g_bpAtkT = 0; g_bpPause = (g_bpHp < BP_HP / 2) ? pause * 7 / 10 : pause; }
static int bpShadesAlive() { int n = 0; for (int i = 0; i < 4; ++i) if (g_bpSh[i].life > 0) ++n; return n; }
static void bpStartAtk(BossIO &io, int a) {
    g_bpAtk = a; g_bpAtkT = 0;
    switch (a) {
    case BPA_ORBS: g_bpOrbTot = 5; g_bpOrbN = 5; break;
    case BPA_CHARGE: g_bpCharge = 200; lvMessage("¡CARGA SU FUEGO! ¡DISPÁRALE!", 120); break;
    case BPA_SHADOWS:
        for (int i = 0; i < 4; ++i) {
            const float ang = BP_TAU * (i + 0.5f * bpRnd(g_bpT, i, 81)) / 4.0f;
            BpShade &s = g_bpSh[i];
            s.x = g_bpX + cosf(ang) * 3.0f; s.z = g_bpZ + sinf(ang) * 3.0f; s.vx = s.vz = 0.0f; s.life = 520; s.age = 0;
        }
        lvMessage("¡LAS SOMBRAS!", 90); break;
    case BPA_WAVES: bpWaveStart(g_bpX, g_bpZ, 3); lvMessage("¡SALTA LAS ONDAS!", 100); break;
    case BPA_MIND:
        g_bpTrap = 1; g_bpTrapT = 0; bxMashStart(g_bpMash, 330);
        lvMessage("ATRAPA TU MENTE: MUEVE EL ANÁLOGO", 150); break;
    default: break;
    }
    (void)io;
}
static void bpFireOrb(BossIO &io) {
    lvFireOrb(g_bpX, 3.7f, g_bpZ, io.px, io.py + 1.7f, io.pz, 0.26f, RGBA(255, 140, 30, 255));
}
static void bpRunAtk(BossIO &io) {
    ++g_bpAtkT;
    switch (g_bpAtk) {
    case BPA_ORBS:
        if (g_bpOrbN > 0 && g_bpAtkT % 46 == 10) { --g_bpOrbN; bpFireOrb(io); }
        if (g_bpAtkT >= 46 * g_bpOrbTot + 20) bpEndAtk(80);
        break;
    case BPA_CHARGE:
        if (g_bpCharge > 0) --g_bpCharge;
        if (g_bpCharge == 0) {          // suelta la onda de fuego y una descarga de bolas
            bpWaveStart(g_bpX, g_bpZ, 2); g_bpAtk = BPA_ORBS; g_bpAtkT = 0; g_bpOrbTot = 3; g_bpOrbN = 3; g_lvFlash = 8;
        }
        break;
    case BPA_SHADOWS: if (g_bpAtkT > 70) bpEndAtk(150); break;
    case BPA_WAVES: if (g_bpAtkT > 60 && g_bpWaveT == 0) bpEndAtk(110); break;
    default: break;
    }
}
static void bpFightUpdate(BossIO &io) {
    if (io.px * io.px + io.pz * io.pz < 1.7f * 1.7f && io.py < 2.6f) io.hp -= 0.25f;     // quemarse en la fogata
    if (io.aiming) { const float d = bpWrapPi(atan2f(g_bpX - io.px, -(g_bpZ - io.pz)) - io.camYaw); io.camYaw += d * 0.10f; }
    if (g_lvTimeStop > 0) return;                                                         // tiempo detenido
    { const float t = bpWrapPi(atan2f(-(io.px - g_bpX), -(io.pz - g_bpZ))); g_bpYaw += bpWrapPi(t - g_bpYaw) * 0.10f; }
    if (g_bpTrap) {                                                                       // mente atrapada
        ++g_bpTrapT;
        const int r = bxMashUpdate(g_bpMash, io.stickX, io.stickY, 7.0f);
        if (g_bpTrapT % 60 == 0) io.hp -= 5.0f;
        if (r == 1) { g_bpTrap = 0; g_bpStun = 150; lvMessage("TE LIBERASTE", 80); bpEndAtk(60); }
        else if (r == -1) { g_bpTrap = 0; io.hp -= 10.0f; lvMessage("TU MENTE CEDE", 80); bpEndAtk(90); }
        return;
    }
    if (g_bpAtk != BPA_CHARGE) {                                                          // rodea la fogata al lado opuesto
        float dx = io.px, dz = io.pz, l = sqrtf(dx * dx + dz * dz);
        if (l < 0.5f) { dx = 0.0f; dz = 1.0f; l = 1.0f; }
        g_bpX += bpClampf((-dx / l * BP_HOME_R - g_bpX) * 0.03f, -0.07f, 0.07f);
        g_bpZ += bpClampf((-dz / l * BP_HOME_R - g_bpZ) * 0.03f, -0.07f, 0.07f);
    }
    if (g_bpAtk == BPA_NONE) {
        if (g_bpStun > 0) --g_bpStun;
        else if (--g_bpPause <= 0) {
            const int a = kBpSeq[g_bpSeq & 7];
            if (a == BPA_MIND && (bpShadesAlive() > 0 || g_bpWaveT > 0)) g_bpPause = 40;
            else { ++g_bpSeq; bpStartAtk(io, a); }
        }
    } else bpRunAtk(io);
    // ondas de fuego por el piso: hay que saltarlas
    if (g_bpWaveT > 0) {
        const float d = sqrtf((io.px - g_bpWaveX) * (io.px - g_bpWaveX) + (io.pz - g_bpWaveZ) * (io.pz - g_bpWaveZ));
        for (int w = 0; w < g_bpWaveN; ++w) {
            const float R = bpWaveR(w);
            if (!g_lvDashInv && R > 0.0f && !(g_bpWaveHit & (1 << w)) && fabsf(d - R) < 1.0f && io.py < 1.4f) { g_bpWaveHit |= 1 << w; io.hp -= 12.0f; g_lvFlash = 10; }   // dash: la atraviesas
        }
        ++g_bpWaveT;
        if (g_bpWaveT > (g_bpWaveN - 1) * 42 + 76) g_bpWaveT = 0;
    }
    // sombras: se deslizan hacia el jugador
    for (int i = 0; i < 4; ++i) {
        BpShade &s = g_bpSh[i]; if (s.life <= 0) continue;
        ++s.age; --s.life;
        if (s.age > 45) {
            float dx = io.px - s.x, dz = io.pz - s.z; const float l = sqrtf(dx * dx + dz * dz) + 0.001f;
            const float sp = (g_bpHp < BP_HP / 2 ? 0.14f : 0.11f) + 0.01f * (i & 1);
            s.vx += (dx / l * sp - s.vx) * 0.05f; s.vz += (dz / l * sp - s.vz) * 0.05f;
            s.x += s.vx; s.z += s.vz;
            if (!g_lvDashInv && l < 1.3f && io.py < 3.8f) { io.hp -= 9.0f; g_lvFlash = 8; s.life = 0; }   // dash: la sombra te atraviesa
        }
    }
}

// ---------------------------------------------------------------- pose
static void bpUpdatePose() {
    const float t = g_bpAnim * 0.0167f;
    float R[3] = { 0.20f, 0.22f, 0.55f }, L[3] = { 0.14f, 0.22f, 0.45f };
    float bow = 0.04f + 0.02f * sinf(t * 1.7f), hy = 0.0f, lean = 0.0f, kneel = 0.0f, eyes = 1.0f;
    const int fight = (g_bpSt == BPS_FIGHT);
    if (g_bpSt == BPS_IDLE || g_bpSt == BPS_SAY || g_bpSt == BPS_CHOICE || g_bpSt == BPS_Q) {
        R[0] = 0.55f + 0.18f * sinf(t * 1.1f); R[1] = 0.30f; R[2] = 1.0f + 0.3f * sinf(t * 1.1f + 0.6f);
        L[0] = 0.18f; L[1] = 0.18f; L[2] = 0.55f; hy = 0.12f * sinf(t * 0.7f);
        if (g_bpCurse) { R[0] = 1.5f; R[1] = 0.1f; R[2] = 0.1f; L[0] = 0.7f; L[2] = 1.7f; eyes = 1.6f; }
    } else if (g_bpSt == BPS_MINDIN) {
        R[0] = 1.5f; R[1] = 0.15f; R[2] = 0.2f; L[1] = 0.5f; eyes = 1.7f;
    } else if (fight) {
        R[0] = L[0] = 0.25f; R[1] = L[1] = 0.55f; R[2] = L[2] = 0.5f;
        if (g_bpStun > 0) { R[0] = L[0] = 0.1f; R[1] = L[1] = 0.15f; R[2] = L[2] = 0.3f; bow = 0.5f; }
        if (g_bpAtk == BPA_CHARGE) { R[0] = L[0] = 2.55f + 0.1f * sinf(t * 9.0f); R[1] = L[1] = 0.38f; R[2] = L[2] = 0.45f; bow = -0.25f; lean = -0.08f; eyes = 1.8f; }
        else if (g_bpAtk == BPA_ORBS) { R[0] = 0.4f + 1.1f * ((g_bpAtkT % 46) < 16 ? 1.0f : 0.0f); R[1] = 0.15f; R[2] = 0.3f; }
        else if (g_bpAtk == BPA_WAVES) { const float up = (g_bpAtkT < 40) ? 2.6f : 0.4f; R[0] = L[0] = up; R[1] = L[1] = 0.3f; lean = (g_bpAtkT < 40) ? 0.0f : 0.35f; }
        else if (g_bpAtk == BPA_SHADOWS) { R[0] = L[0] = 0.3f; R[1] = L[1] = 1.1f; R[2] = L[2] = 0.25f; }
        if (g_bpTrap) { R[0] = 1.45f; R[1] = 0.12f; R[2] = 0.15f; L[0] = 0.6f; L[2] = 1.8f; eyes = 2.0f; }
    } else if (g_bpSt == BPS_KOFALL || g_bpSt == BPS_PRAY) {
        R[0] = 0.12f; R[1] = 0.14f; R[2] = 0.35f; L[0] = 0.10f; L[1] = 0.14f; L[2] = 0.40f; bow = 0.65f; lean = 0.25f; kneel = 1.0f; eyes = 0.0f;
    } else if (g_bpSt == BPS_LIGHT) {
        kneel = (g_bpStT < 70) ? 1.0f : 0.0f; R[0] = L[0] = 0.1f; R[1] = L[1] = 1.25f; R[2] = L[2] = 0.2f; bow = -0.55f; eyes = 2.2f;
        if (g_bpStT < 70) { bow = 0.65f; lean = 0.25f; eyes = 0.0f; R[1] = L[1] = 0.14f; }
    } else if (g_bpSt == BPS_VANISH) {
        R[0] = L[0] = 0.2f; R[1] = L[1] = 1.2f + 0.1f * sinf(t * 8.0f); R[2] = L[2] = 0.2f; bow = -0.3f; eyes = 1.5f;
    }
    for (int k = 0; k < 3; ++k) { g_bpArm[0][k] += (R[k] - g_bpArm[0][k]) * 0.10f; g_bpArm[1][k] += (L[k] - g_bpArm[1][k]) * 0.10f; }
    g_bpBow += (bow - g_bpBow) * 0.08f; g_bpHy += (hy - g_bpHy) * 0.08f; g_bpLean += (lean - g_bpLean) * 0.08f;
    g_bpKneel += (kneel - g_bpKneel) * 0.06f; g_bpEyes += (eyes - g_bpEyes) * 0.15f;
}

// ---------------------------------------------------------------- dibujo 3D
static void bpBlendAdd() { sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_FIX, 0, 0xFFFFFFFF); sceGuDepthMask(GU_TRUE); }
static void bpBlendAlpha() { sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0); sceGuDepthMask(GU_TRUE); }
static void bpBlendOff() { sceGuDepthMask(GU_FALSE); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0); sceGuDisable(GU_BLEND); }
static void bpPivot(float px, float py, float pz, float rx, float ry, float rz) {
    ScePspFVector3 t = { px, py, pz }; sceGumTranslate(&t);
    if (ry != 0.0f) sceGumRotateY(ry);
    if (rx != 0.0f) sceGumRotateX(rx);
    if (rz != 0.0f) sceGumRotateZ(rz);
    ScePspFVector3 u = { -px, -py, -pz }; sceGumTranslate(&u);
}
static void bpPart(LineVertex *v, int n) { if (n > 0) sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, n, 0, v); }
static void bpMirror(int side) { if (side) { ScePspFVector3 m = { -1.0f, 1.0f, 1.0f }; sceGumScale(&m); } }

static void bpDrawBoss() {
    if (g_bpSt == BPS_DONE || g_bpScale <= 0.02f) return;
    const int hot = (g_bpHurt > 0 || (g_bpCharge > 0 && g_bpSt == BPS_FIGHT && ((g_bpAnim >> 2) & 1))) ? 1 : 0;
    const float drop = 0.9f * g_bpKneel, br = sinf(g_bpAnim * 0.045f), trem = g_bpLight * 0.06f * sinf(g_bpAnim * 2.9f);
    sceGumLoadIdentity();
    { ScePspFVector3 t = { g_bpX + trem, g_bpY, g_bpZ - trem }; sceGumTranslate(&t); }
    sceGumRotateY(g_bpYaw);
    if (g_bpScale != 1.0f) { ScePspFVector3 sc = { g_bpScale, g_bpScale, g_bpScale }; sceGumScale(&sc); }
    for (int side = 0; side < 2; ++side) {                      // piernas (de rodillas: la rodilla en el suelo)
        sceGumPushMatrix(); bpMirror(side);
        { ScePspFVector3 d = { 0.0f, -drop, 0.0f }; sceGumTranslate(&d); }
        bpPivot(0.32f, 2.20f, 0.0f, 0.03f * sinf(g_bpAnim * 0.03f + side), 0.0f, 0.0f);
        bpPart(g_bpThigh[hot], g_bpThighN);
        bpPivot(0.30f, 1.15f, 0.0f, -1.5f * g_bpKneel, 0.0f, 0.0f);
        bpPart(g_bpShin[hot], g_bpShinN);
        sceGumPopMatrix();
    }
    sceGumPushMatrix();                                         // parte alta
    { ScePspFVector3 d = { 0.0f, -drop, 0.0f }; sceGumTranslate(&d); }
    bpPivot(0.0f, 2.20f, 0.0f, -g_bpLean, 0.0f, 0.0f);
    { ScePspFVector3 a = { 0.0f, 2.20f, 0.0f }; sceGumTranslate(&a);
      ScePspFVector3 s = { 1.0f + 0.010f * br, 1.0f + 0.016f * br, 1.0f + 0.010f * br }; sceGumScale(&s);   // respiracion
      ScePspFVector3 c = { 0.0f, -2.20f, 0.0f }; sceGumTranslate(&c); }
    bpPart(g_bpTorso[hot], g_bpTorsoN);
    sceGumPushMatrix();
    bpPivot(0.0f, 4.15f, -0.02f, -g_bpBow, g_bpHy, 0.0f);
    bpPart(g_bpHeadV[hot], g_bpHeadN);
    if (g_bpEyes > 0.25f) bpPart(g_bpEyeV, g_bpEyeN);
    sceGumPopMatrix();
    for (int side = 0; side < 2; ++side) {                      // brazos
        const float *A = g_bpArm[side];
        sceGumPushMatrix(); bpMirror(side);
        bpPivot(0.80f, 3.88f, 0.0f, A[0], 0.0f, A[1]);
        bpPart(g_bpUpArm[hot], g_bpUpN);
        bpPivot(0.80f, 2.95f, 0.0f, A[2], 0.0f, 0.0f);
        bpPart(g_bpForeArm[hot], g_bpForeN);
        sceGumPopMatrix();
    }
    sceGumPopMatrix();
    sceGumLoadIdentity();
}
// punto de la cabeza (reposo) -> mundo
static void bpHeadPt(float lx, float ly, float lz, float *o) {
    float x = lx, y = ly - 4.15f, z = lz + 0.02f;
    { const float a = -g_bpBow, c = cosf(a), s = sinf(a); const float ny = y * c - z * s, nz = y * s + z * c; y = ny; z = nz; }
    { const float c = cosf(g_bpHy), s = sinf(g_bpHy); const float nx = x * c + z * s, nz = -x * s + z * c; x = nx; z = nz; }
    y += 4.15f; z -= 0.02f;
    { const float yy = y - 2.20f, a = -g_bpLean, c = cosf(a), s = sinf(a); const float ny = yy * c - z * s, nz = yy * s + z * c; y = ny + 2.20f; z = nz; }
    y -= 0.9f * g_bpKneel;
    x *= g_bpScale; y *= g_bpScale; z *= g_bpScale;
    { const float c = cosf(g_bpYaw), s = sinf(g_bpYaw); const float nx = x * c + z * s, nz = -x * s + z * c; x = nx; z = nz; }
    o[0] = g_bpX + x; o[1] = g_bpY + y; o[2] = g_bpZ + z;
}
static void bpGlow3D(float x, float y, float z, float R, unsigned int cIn, unsigned int cOut) {
    const int N = 12;
    LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 3 * N);
    for (int s = 0; s < N; ++s) {
        const float a0 = BP_TAU * s / N, a1 = BP_TAU * (s + 1) / N;
        const float c0 = cosf(a0) * R, s0 = sinf(a0) * R, c1 = cosf(a1) * R, s1 = sinf(a1) * R;
        v[s * 3 + 0] = { cIn, x, y, z };
        v[s * 3 + 1] = { cOut, x + g_lvRight[0] * c0 + g_lvUp[0] * s0, y + g_lvRight[1] * c0 + g_lvUp[1] * s0, z + g_lvRight[2] * c0 + g_lvUp[2] * s0 };
        v[s * 3 + 2] = { cOut, x + g_lvRight[0] * c1 + g_lvUp[0] * s1, y + g_lvRight[1] * c1 + g_lvUp[1] * s1, z + g_lvRight[2] * c1 + g_lvUp[2] * s1 };
    }
    sceGumLoadIdentity(); sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, 3 * N, 0, v);
}
static void bpGroundFan(float cx, float y, float cz, float R, unsigned int cIn, unsigned int cOut, int seg) {
    if (seg > 32) seg = 32;
    LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 3 * seg);
    for (int s = 0; s < seg; ++s) {
        const float a0 = BP_TAU * s / seg, a1 = BP_TAU * (s + 1) / seg;
        v[s * 3 + 0] = { cIn, cx, y, cz };
        v[s * 3 + 1] = { cOut, cx + cosf(a0) * R, y, cz + sinf(a0) * R };
        v[s * 3 + 2] = { cOut, cx + cosf(a1) * R, y, cz + sinf(a1) * R };
    }
    sceGumLoadIdentity(); sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, 3 * seg, 0, v);
}
// lengua de fuego: dos planos cruzados; base clara -> punta roja transparente
static void bpTongue(BpBuf &b, float x, float y, float z, float h, float w, float sx, float sz, int core) {
    const unsigned int cB = core ? bpC(255, 245, 190, 235) : bpC(255, 190, 70, 230);
    const unsigned int cM = core ? bpC(255, 210, 110, 200) : bpC(255, 110, 24, 185);
    const unsigned int cT = core ? bpC(255, 150, 40, 0) : bpC(170, 24, 6, 0);
    const float ym = y + h * 0.45f, wm = w * 0.8f, mx = sx * 0.4f, mz = sz * 0.4f;
    bpTriC(b, cB, x - w, y, z, cB, x + w, y, z, cM, x - wm + mx, ym, z + mz);
    bpTriC(b, cB, x + w, y, z, cM, x + wm + mx, ym, z + mz, cM, x - wm + mx, ym, z + mz);
    bpTriC(b, cM, x - wm + mx, ym, z + mz, cM, x + wm + mx, ym, z + mz, cT, x + sx, y + h, z + sz);
    bpTriC(b, cB, x, y, z - w, cB, x, y, z + w, cM, x + mx, ym, z - wm + mz);
    bpTriC(b, cB, x, y, z + w, cM, x + mx, ym, z + wm + mz, cM, x + mx, ym, z - wm + mz);
    bpTriC(b, cM, x + mx, ym, z - wm + mz, cM, x + mx, ym, z + wm + mz, cT, x + sx, y + h, z + sz);
}
static void bpBill(BpBuf &b, float x, float y, float z, float s, unsigned int c) {
    const float rx = g_lvRight[0] * s, ry = g_lvRight[1] * s, rz = g_lvRight[2] * s, ux = g_lvUp[0] * s, uy = g_lvUp[1] * s, uz = g_lvUp[2] * s;
    bpQuad(b, x - rx - ux, y - ry - uy, z - rz - uz, c, x + rx - ux, y + ry - uy, z + rz - uz, c,
              x + rx + ux, y + ry + uy, z + rz + uz, c, x - rx + ux, y - ry + uy, z - rz + uz, c);
}
static void bpDrawFire() {
    const float a = (float)g_bpAnim;
    LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 330);
    BpBuf b = { v, 0, 330 };
    for (int i = 0; i < 12; ++i) {
        const float ang = BP_TAU * i / 12.0f + 0.7f * i;
        const float rad = (i % 3 == 0) ? 0.05f : 0.30f + 0.08f * (float)((i * 5) % 5);
        const float fl = bpFlick(a * (0.9f + 0.04f * i), (float)i);
        const float h = ((i % 3 == 0) ? 3.1f : 2.1f) + 0.6f * fl + 0.12f * (float)((i * 7) % 5);
        const float w = 0.33f + 0.04f * (float)((i * 3) % 4);
        const float sx = h * 0.22f * sinf(a * 0.10f * (float)(1 + i % 3) + (float)i);
        const float sz = h * 0.22f * cosf(a * 0.09f * (float)(1 + (i + 1) % 3) + (float)i * 1.3f);
        bpTongue(b, cosf(ang) * rad, 0.30f, sinf(ang) * rad, h, w, sx, sz, 0);
    }
    for (int i = 0; i < 5; ++i) {
        const float ang = BP_TAU * i / 5.0f, fl = bpFlick(a * 1.3f, (float)i * 2.0f);
        bpTongue(b, cosf(ang) * 0.18f, 0.35f, sinf(ang) * 0.18f, 1.5f + 0.5f * fl + 0.2f * i, 0.22f, 0.10f * fl, -0.10f * fl, 1);
    }
    sceGumLoadIdentity(); sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, b.n, 0, v);
    const float fl0 = bpFlick(a, 0.0f);
    bpGroundFan(0.0f, 0.05f, 0.0f, 6.5f + 0.6f * fl0, bpC(255, 130, 40, 100), bpC(255, 90, 20, 0), 20);
    bpGroundFan(0.0f, 0.07f, 0.0f, 1.8f, bpC(255, 150, 50, 150), bpC(255, 90, 20, 0), 12);
    LineVertex *ev = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 120);    // chispas
    BpBuf e = { ev, 0, 120 };
    for (int i = 0; i < 20; ++i) {
        const float ph = fmodf(a * 0.012f * (0.7f + 0.05f * (float)(i % 5)) + i * 0.173f, 1.0f);
        const float rr = 0.6f + ph * 2.2f;
        bpBill(e, sinf(i * 2.3f + ph * 5.0f) * rr, 0.8f + ph * 9.5f, cosf(i * 1.7f + ph * 4.0f) * rr, 0.10f * (1.0f - ph) + 0.03f,
               bpC(255, 170 - (int)(ph * 100.0f), 40, (int)(255.0f * (1.0f - ph))));
    }
    sceGumLoadIdentity(); sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, e.n, 0, ev);
}
static void bpDrawShades(float px, float pz) {
    for (int i = 0; i < 4; ++i) {
        const BpShade &s = g_bpSh[i]; if (s.life <= 0) continue;
        const float rise = bpClampf(s.age / 40.0f, 0.0f, 1.0f);
        sceGumLoadIdentity();
        ScePspFVector3 t = { s.x, -3.6f * (1.0f - rise) + 0.12f * sinf(g_bpAnim * 0.1f + i), s.z }; sceGumTranslate(&t);
        sceGumRotateY(atan2f(-(px - s.x), -(pz - s.z)));
        sceGumRotateZ(0.06f * sinf(g_bpAnim * 0.15f + i));
        bpPart(g_bpShadeV, g_bpShadeN);
    }
}
static void bpDrawShadeAuras() {
    for (int i = 0; i < 4; ++i) {
        const BpShade &s = g_bpSh[i]; if (s.life <= 0) continue;
        bpGroundFan(s.x, 0.06f, s.z, 1.9f, bpC(110, 30, 170, 110), bpC(40, 0, 80, 0), 12);
    }
}
static void bpDrawWaves() {
    if (g_bpWaveT <= 0) return;
    const float a = (float)g_bpAnim;
    for (int w = 0; w < g_bpWaveN; ++w) {
        const float R = bpWaveR(w); if (R <= 0.0f || R > 25.0f) continue;
        lvRing3D(g_bpWaveX, 0.08f, g_bpWaveZ, R, 1.1f, bpC(255, 110, 24, 200));
        lvRing3D(g_bpWaveX, 0.11f, g_bpWaveZ, R, 0.36f, bpC(255, 225, 140, 240));
        LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 16 * 18);
        BpBuf b = { v, 0, 16 * 18 };
        for (int i = 0; i < 16; ++i) {
            const float an = BP_TAU * i / 16.0f + w, fl = bpFlick(a * 1.1f, (float)i);
            bpTongue(b, g_bpWaveX + sinf(an) * R, 0.1f, g_bpWaveZ + cosf(an) * R, 1.1f + 0.4f * fl, 0.5f, 0.1f * fl, 0.1f, 0);
        }
        sceGumLoadIdentity(); sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, b.n, 0, v);
    }
}
static void bpDrawCharge() {
    if (g_bpSt != BPS_FIGHT || g_bpCharge <= 0) return;
    const float p = 1.0f - g_bpCharge / 200.0f, R = 0.5f + 1.3f * p, oy = 6.3f;
    const int blink = (g_bpAnim >> 2) & 1;
    bpGlow3D(g_bpX, oy, g_bpZ, R * 2.3f, bpC(255, 90, 20, 110), bpC(255, 60, 10, 0));
    bpGlow3D(g_bpX, oy, g_bpZ, R * 1.3f, bpC(255, 160, 50, 190), bpC(255, 110, 20, 0));
    bpGlow3D(g_bpX, oy, g_bpZ, R * 0.6f, bpC(255, 240, 180, 240), bpC(255, 200, 100, 0));
    bpGlow3D(g_bpX, 3.2f, g_bpZ, 3.4f, bpC(255, 40, 20, blink ? 90 : 30), bpC(255, 30, 10, 0));
    lvRing3D(g_bpX, 0.07f, g_bpZ, 2.4f + 0.5f * sinf(g_bpAnim * 0.3f), 0.4f, bpC(255, 110, 30, 210));
    LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 8 * 6);
    BpBuf b = { v, 0, 48 };
    for (int i = 0; i < 8; ++i) {                      // chispas que convergen en la bola
        const float ph = fmodf(g_bpAnim * 0.02f + i * 0.125f, 1.0f), an = BP_TAU * i / 8.0f + g_bpAnim * 0.05f;
        bpBill(b, g_bpX + cosf(an) * (1.0f - ph) * 4.0f, oy - 0.5f + (1.0f - ph) * 1.0f * sinf(an), g_bpZ + sinf(an) * (1.0f - ph) * 4.0f,
               0.12f, bpC(255, 200, 90, (int)(255.0f * ph)));
    }
    sceGumLoadIdentity(); sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, b.n, 0, v);
}
static void bpDrawEyes() {
    if (g_bpSt == BPS_DONE || g_bpEyes < 0.25f || g_bpScale < 0.1f) return;
    float e[3];
    for (int s = -1; s <= 1; s += 2) {
        bpHeadPt(s * 0.17f, 4.63f, -0.32f, e);
        bpGlow3D(e[0], e[1], e[2], 0.28f * g_bpEyes * g_bpScale, bpC(255, 60, 40, 230), bpC(255, 20, 10, 0));
    }
    if (g_bpBounce > 0) { float c[3]; bpHeadPt(0.0f, 3.4f, -0.5f, c); bpGlow3D(c[0], c[1], c[2], 1.3f, bpC(220, 230, 255, 170), bpC(150, 170, 255, 0)); }
}
static void bpDrawLight() {
    const float L = g_bpLight; if (L <= 0.01f) return;
    const int N = 14;
    for (int sh = 0; sh < 3; ++sh) {
        static const float rad[3] = { 2.4f, 1.5f, 0.8f }; static const int al[3] = { 70, 110, 170 };
        const float r = rad[sh] * (0.5f + 0.5f * L);
        LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * N * 12);
        BpBuf b = { v, 0, N * 12 };
        const float ys[3] = { 0.0f, 9.0f, 19.5f }; const int aa[3] = { (int)(al[sh] * L), (int)(al[sh] * L * 0.8f), 0 };
        for (int j = 0; j < N; ++j) {
            const float a0 = BP_TAU * j / N, a1 = BP_TAU * (j + 1) / N;
            for (int k = 0; k < 2; ++k) {
                const unsigned int c0 = bpC(255, 248, 215, aa[k]), c1 = bpC(255, 248, 215, aa[k + 1]);
                bpQuad(b, g_bpX + cosf(a0) * r, ys[k], g_bpZ + sinf(a0) * r, c0, g_bpX + cosf(a1) * r, ys[k], g_bpZ + sinf(a1) * r, c0,
                          g_bpX + cosf(a1) * r, ys[k + 1], g_bpZ + sinf(a1) * r, c1, g_bpX + cosf(a0) * r, ys[k + 1], g_bpZ + sinf(a0) * r, c1);
            }
        }
        sceGumLoadIdentity(); sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, b.n, 0, v);
    }
    bpGlow3D(g_bpX, 19.3f, g_bpZ, 3.0f, bpC(255, 250, 225, (int)(200 * L)), bpC(255, 240, 190, 0));
    bpGroundFan(g_bpX, 0.06f, g_bpZ, 4.5f * L, bpC(255, 250, 220, (int)(200 * L)), bpC(255, 230, 160, 0), 24);
    for (int i = 0; i < 8; ++i) lvBeam(g_bpX, 0.35f, g_bpZ, BP_TAU * i / 8.0f + g_bpAnim * 0.01f, 7.0f * L, 0.25f, bpC(255, 244, 190, 170));
    lvRing3D(g_bpX, 0.10f, g_bpZ, fmodf(g_bpAnim * 0.12f, 9.0f) + 0.5f, 0.5f, bpC(255, 240, 190, (int)(160 * L)));
}
static void bpDrawSmoke() {
    if (g_bpSt != BPS_VANISH) return;
    for (int i = 0; i < 18; ++i) {
        const int age = g_bpStT - 10 - i * 6; if (age <= 0 || age >= 70) continue;
        const float p = age / 70.0f;
        bpGlow3D(g_bpX + sinf(i * 2.1f) * 1.0f * p, 0.5f + 4.5f * (float)(i % 5) / 5.0f + p * 3.0f, g_bpZ + cosf(i * 1.3f) * 1.0f * p,
                 0.8f + 2.0f * p, bpC(8, 6, 12, (int)(210.0f * (1.0f - p))), bpC(8, 6, 12, 0));
    }
}

static void bp_Draw(float px, float py, float pz) {
    (void)py;
    sceGuDisable(GU_TEXTURE_2D);
    bpDrawBoss();
    bpDrawShades(px, pz);
    bpBlendAdd();
    bpDrawFire();
    bpDrawShadeAuras();
    bpDrawWaves();
    bpDrawCharge();
    bpDrawEyes();
    bpDrawLight();
    bpBlendAlpha();
    bpDrawSmoke();
    bpBlendOff();
    sceGumLoadIdentity();
}

// ---------------------------------------------------------------- logica por frame
static void bp_Update(BossIO &io) {
    ++g_bpT; ++g_bpStT;
    if (g_lvTimeStop <= 0) ++g_bpAnim;
    if (g_bpHurt > 0) --g_bpHurt;
    if (g_bpBounce > 0) --g_bpBounce;
    if (g_bpPErr > 0) --g_bpPErr;
    if (g_bpPShake > 0) --g_bpPShake;
    if (g_bpSt == BPS_FIGHT && g_bpTint > 0.0f) g_bpTint -= 0.02f;
    if (g_bpTint < 0.0f) g_bpTint = 0.0f;
    // el jugador reaparecio (muerte): limpiar ataques
    if (g_bpHaveLast && (fabsf(io.px - g_bpLpx) + fabsf(io.pz - g_bpLpz)) > 5.0f && g_bpSt == BPS_FIGHT) { bpClearAttacks(); g_bpPause = 120; g_bpStun = 0; }
    g_bpLpx = io.px; g_bpLpz = io.pz; g_bpHaveLast = 1;
    // luz del cielo
    if (g_bpSt == BPS_LIGHT) g_bpLight = bpClampf(g_bpStT / 60.0f, 0.0f, 1.0f);
    else if (g_bpSt == BPS_VANISH) g_bpLight = bpClampf(1.0f - g_bpStT / 90.0f, 0.0f, 1.0f);
    else if (g_bpCurse && g_bpSt == BPS_SAY) g_bpLight = 1.0f;
    else g_bpLight = 0.0f;
    const int facing = (g_bpSt != BPS_FIGHT && g_bpSt != BPS_DONE);
    if (facing) { const float t = atan2f(-(io.px - g_bpX), -(io.pz - g_bpZ)); g_bpYaw += bpWrapPi(t - g_bpYaw) * 0.05f; }
    const int lookCam = (g_bpSt == BPS_SAY || g_bpSt == BPS_CHOICE || g_bpSt == BPS_Q || g_bpSt == BPS_MINDIN || g_bpSt == BPS_KOFALL || g_bpSt == BPS_PRAY || g_bpSt == BPS_LIGHT);
    if (lookCam) { const float d = bpWrapPi(atan2f(g_bpX - io.px, -(g_bpZ - io.pz)) - io.camYaw); io.camYaw += d * 0.05f; }
    switch (g_bpSt) {
    case BPS_IDLE: {
        const float dx = io.px - g_bpX, dz = io.pz - g_bpZ;
        if (dx * dx + dz * dz < 17.5f * 17.5f) bpSay("BAPHOMET", "¿Por qué estás aquí? Conversemos.", "", BPN_CHOICE);
    } break;
    case BPS_SAY:
        if (io.edge & PSP_CTRL_CROSS) bpAdvance(g_bpNext);
        break;
    case BPS_CHOICE:
        if (io.edge & (PSP_CTRL_UP | PSP_CTRL_DOWN)) g_bpSel ^= 1;
        if (io.edge & PSP_CTRL_CROSS) {
            if (g_bpSel == 0) bpSay("BAPHOMET", "¿Ayuda? Qué conmovedor...", "Mi mente siempre tiene un lugar para ti.", BPN_MINDIN);
            else { g_bpQ = 0; g_bpQok = 0; bpSay("BAPHOMET", "Oh, mi Éter de Azufre. Cosecha del año 666.", "Veamos si sabe apreciarlo.", BPN_Q); }
        }
        break;
    case BPS_Q:
        if (io.edge & PSP_CTRL_UP) { if (--g_bpSel < 0) g_bpSel = 2; }
        if (io.edge & PSP_CTRL_DOWN) { if (++g_bpSel > 2) g_bpSel = 0; }
        if (io.edge & PSP_CTRL_CROSS) {
            const BpQ &q = kBpQ[g_bpQ];
            const int ok = (g_bpSel == q.ok);
            if (ok) ++g_bpQok;
            ++g_bpQ;
            bpSay("BAPHOMET", ok ? q.good : q.bad, "", g_bpQ >= 3 ? BPN_REWARD : BPN_Q);
        }
        break;
    case BPS_MINDIN:
        g_bpTint = bpClampf(g_bpTint + 0.02f, 0.0f, 1.0f);
        if (g_bpStT == 2) lvMessage("TE ATRAPA EN SU MENTE", 140);
        if (g_bpStT >= 140) { g_bpSt = BPS_FIGHT; g_bpStT = 0; g_bpPause = 90; g_bpSeq = 1; g_bpHp = BP_HP; bpClearAttacks(); }
        break;
    case BPS_FIGHT: bpFightUpdate(io); break;
    case BPS_KOFALL:
        if (g_bpStT >= 130) bpSay("", "Baphomet cae de rodillas, inconsciente.", "Sólo una oración puede sellarlo. Rece.", BPN_PRAY);
        break;
    case BPS_PRAY:
        if (io.edge & PSP_CTRL_UP) g_bpPSel = (g_bpPSel + 3) & 3;
        if (io.edge & PSP_CTRL_DOWN) g_bpPSel = (g_bpPSel + 1) & 3;
        if ((io.edge & PSP_CTRL_CROSS) && !(g_bpPWrong & (1 << g_bpPSel))) {
            if (g_bpPSel == g_bpPCorrect) {
                ++g_bpPStep; bpPrayRebuild();
                if (g_bpPStep >= 12) { g_bpSt = BPS_LIGHT; g_bpStT = 0; g_bpCurse = 0; }
                else bpPrayOptions();
            } else {
                g_bpPErr = 100; g_bpPWrong |= 1 << g_bpPSel; g_bpPShake = 10;
                if (io.hp - 3.0f < 1.0f) io.hp = 1.0f; else io.hp -= 3.0f;
            }
        }
        break;
    case BPS_LIGHT:
        if (g_bpStT == 60) g_lvFlash = 70;
        if (g_bpStT >= 170) { g_bpCurse = 1; bpSay("BAPHOMET", "Maldito seas, portador de la luz...", "", BPN_CURSE2); }
        break;
    case BPS_VANISH: {
        const float vt = bpClampf((g_bpStT - 20) / 150.0f, 0.0f, 1.0f);
        g_bpScale = 1.0f - vt * vt * (3.0f - 2.0f * vt);
        if (g_bpStT >= 200) { g_bpSt = BPS_DONE; g_bpScale = 0.0f; g_bpLight = 0.0f; }
    } break;
    default: break;
    }
    if (g_lvTimeStop <= 0) bpUpdatePose();
}

// ---------------------------------------------------------------- HUD
static void bpHudPray() {
    sceGuDisable(GU_TEXTURE_2D);
    drawRect(14, 4, 452, 92, RGBA(8, 10, 16, 220));
    drawRect(14, 4, 452, 1, RGBA(200, 176, 120, 200));
    drawRect(14, 154, 452, 104, RGBA(8, 10, 16, 225));
    drawRect(14, 154, 452, 1, RGBA(200, 176, 120, 200));
    fontTexOn();
    drawText(24, 10, 1.0f, RGBA(236, 206, 120, 255), "PADRE NUESTRO");
    char cnt[8]; int n = 0;
    if (g_bpPStep >= 10) cnt[n++] = '1';
    cnt[n++] = (char)('0' + g_bpPStep % 10); cnt[n++] = '/'; cnt[n++] = '1'; cnt[n++] = '2'; cnt[n] = 0;
    drawText(430, 10, 1.0f, RGBA(160, 170, 190, 255), cnt);
    const int first = g_bpPLnN > 6 ? g_bpPLnN - 6 : 0;
    for (int k = first; k < g_bpPLnN; ++k) drawText(24, 24 + (k - first) * 11, 1.0f, RGBA(240, 240, 246, 255), g_bpPLn[k]);
    if (g_bpPErr > 0) drawText(24, 160, 1.0f, RGBA(255, 110, 100, 255), "...esa no es la palabra");
    else drawText(24, 160, 1.0f, RGBA(236, 206, 120, 255), "¿Cuál es el siguiente fragmento?");
    for (int k = 0; k < 4; ++k) {
        const int wrong = (g_bpPWrong >> k) & 1;
        const unsigned int c = wrong ? RGBA(150, 70, 66, 255) : (k == g_bpPSel ? RGBA(255, 255, 255, 255) : RGBA(140, 146, 160, 255));
        drawText(26, 178 + k * 17, 1.0f, c, kBpPray[g_bpPOpt[k]]);
        if (k == g_bpPSel) drawText(16, 178 + k * 17, 1.0f, RGBA(255, 220, 140, 255), ">");
    }
    drawText(300, 246, 1.0f, RGBA(120, 126, 140, 255), "ARRIBA/ABAJO + X");
    sceGuDisable(GU_TEXTURE_2D);
}
static void bp_Hud(BossIO &io) {
    (void)io;
    sceGuDisable(GU_DEPTH_TEST); sceGuDisable(GU_TEXTURE_2D);
    sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    int fx, fy;                                              // resplandor de la fogata (suma luz)
    if (lvProject(0.0f, 2.0f, 0.0f, fx, fy)) {
        const float fl = bpFlick((float)g_bpAnim, 0.0f);
        sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_FIX, 0, 0xFFFFFFFF);
        lvFan2D(fx, fy, 150 + (int)(16.0f * fl), RGBA(255, 120, 40, 62), RGBA(255, 90, 20, 0), 22);
        lvFan2D(fx, fy, 52 + (int)(8.0f * fl), RGBA(255, 190, 90, 78), RGBA(255, 140, 40, 0), 16);
        sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    }
    if (g_bpTint > 0.0f) { bxTint(RGBA(60, 8, 90, (int)(120.0f * g_bpTint))); bxTint(RGBA(0, 0, 0, (int)(70.0f * g_bpTint))); }
    if (g_bpSt == BPS_FIGHT && g_bpTrap) {                   // pantalla distorsionada
        bxTint(RGBA(70, 10, 110, 90 + (int)(40.0f * sinf(g_bpAnim * 0.2f))));
        for (int k = 0; k < 7; ++k) drawRect((int)(8.0f * sinf(g_bpAnim * 0.3f + k)), (g_bpAnim * 3 + k * 41) % 272, SCR_WIDTH, 4 + (k * 7) % 10, RGBA(150, 90, 220, 60));
        drawRect(0, 0, SCR_WIDTH, 22, RGBA(0, 0, 0, 150)); drawRect(0, 250, SCR_WIDTH, 22, RGBA(0, 0, 0, 150));
        bxMashHud(g_bpMash, 7.0f, "MUEVE EL ANÁLOGO");
    }
    if (g_bpSt == BPS_FIGHT || g_bpSt == BPS_KOFALL) bxBossBar("BAPHOMET", g_bpHp, BP_HP);
    if (g_bpSt == BPS_FIGHT && g_bpCharge > 0 && ((g_bpAnim >> 3) & 1)) {
        fontTexOn(); drawText(240 - 7 * 8, 30, 1.0f, RGBA(255, 200, 90, 255), "¡DISPÁRALE!"); sceGuDisable(GU_TEXTURE_2D);
    }
    switch (g_bpSt) {
    case BPS_SAY:
        if (g_bpLnB[0]) {
            sceGuDisable(GU_TEXTURE_2D);
            drawRect(30, 186, 420, 66, RGBA(8, 10, 16, 215)); drawRect(30, 186, 420, 1, RGBA(200, 176, 120, 200));
            fontTexOn();
            if (g_bpWho[0]) drawText(40, 194, 1.0f, RGBA(236, 206, 120, 255), g_bpWho);
            drawText(40, 212, 1.0f, RGBA(240, 240, 246, 255), g_bpLnA);
            drawText(40, 226, 1.0f, RGBA(240, 240, 246, 255), g_bpLnB);
            drawText(392, 238, 1.0f, RGBA(160, 170, 190, 255), "X >");
            sceGuDisable(GU_TEXTURE_2D);
        } else bxDialog(g_bpWho, g_bpLnA, 1);
        break;
    case BPS_CHOICE: {
        const char *o[2] = { "Pedir ayuda", "¡Qué agradable perfume tiene!" };
        bxDialog("BAPHOMET", "¿Por qué estás aquí? Conversemos.", 0);
        bxChoices("¿Qué le dices?", o, 2, g_bpSel);
    } break;
    case BPS_Q: {
        const BpQ &q = kBpQ[g_bpQ < 3 ? g_bpQ : 2];
        const char *o[3] = { q.opt[0], q.opt[1], q.opt[2] };
        bxDialog("BAPHOMET", q.a, 0);
        bxChoices("Responda:", o, 3, g_bpSel);
    } break;
    case BPS_PRAY: bpHudPray(); break;
    default: break;
    }
    sceGuDisable(GU_TEXTURE_2D); sceGuEnable(GU_DEPTH_TEST);
}

// ---------------------------------------------------------------- API
static void bp_Backdrop() {
    gradQuad(0, 136, RGBA(10, 6, 5, 255), RGBA(26, 14, 9, 255));
    gradQuad(136, SCR_HEIGHT, RGBA(26, 14, 9, 255), RGBA(12, 7, 5, 255));
}
static unsigned int bp_Fog(float *nearF, float *farF) { *nearF = 18.0f; *farF = 120.0f; return RGBA(26, 16, 10, 255); }
static int bp_AimTarget(float out[3]) {
    if (g_bpSt == BPS_FIGHT && g_bpCharge > 0) { out[0] = g_bpX; out[1] = 3.5f; out[2] = g_bpZ; return 1; }
    return 0;
}
static bool bp_ShotHit(float x, float y, float z, int dmg) {
    if (g_bpSt != BPS_FIGHT) return false;
    for (int i = 0; i < 4; ++i) {                            // una bala disipa una sombra
        BpShade &s = g_bpSh[i]; if (s.life <= 0) continue;
        if ((x - s.x) * (x - s.x) + (z - s.z) * (z - s.z) < 1.4f * 1.4f && y > -0.5f && y < 4.4f) { s.life = 0; return true; }
    }
    const float dx = x - g_bpX, dz = z - g_bpZ;
    if (dx * dx + dz * dz < 2.0f * 2.0f && y > -0.5f && y < 6.4f) {
        if (g_bpCharge > 0) {                                // solo cuenta mientras carga su fuego
            g_bpHp -= (dmg < 1 ? 1 : dmg); g_bpHurt = 6;
            if (g_bpHp <= 0) { g_bpHp = 0; bpEnterKO(); }
        } else g_bpBounce = 8;                               // rebota
        return true;
    }
    return false;
}
static int bp_Blocks() {
    switch (g_bpSt) {
    case BPS_SAY: case BPS_CHOICE: case BPS_Q: case BPS_MINDIN: case BPS_KOFALL: case BPS_PRAY: case BPS_LIGHT: case BPS_VANISH: return 1;
    case BPS_FIGHT: return g_bpTrap ? 1 : 0;
    default: return 0;
    }
}
static int bp_Done() { return g_bpSt == BPS_DONE ? 1 : 0; }
static float bp_Shake() {
    float a = 0.0f;
    if (g_bpSt == BPS_FIGHT && g_bpTrap) a = 0.12f;
    if (g_bpSt == BPS_LIGHT) a = 0.05f + 0.30f * g_bpLight;
    if (g_bpCurse && g_bpSt == BPS_SAY) a = 0.30f;
    if (g_bpSt == BPS_VANISH) a = 0.18f * bpClampf(1.0f - g_bpStT / 100.0f, 0.0f, 1.0f);
    if (g_bpSt == BPS_KOFALL && g_bpStT < 30) a = 0.20f;
    if (g_bpPShake > 0) a += 0.12f;
    return a;
}
static void bp_Enter(BossIO &io, LineVertex *geo, int geoCap, int *geoN, CityBldg *boxes, int *nBoxes, int boxCap,
                     float *sx, float *sy, float *sz) {
    bpResetAll();
    g_bpBx = boxes; g_bpNbx = 0; g_bpBxCap = boxCap;
    BpBuf g = { geo, 0, geoCap };
    bpBuildFloor(g);
    bpBuildShell(g);
    bpBuildPit(g);
    bpBuildRocks(g);
    for (int i = 0; i < 32; ++i) {                           // pared de roca: anillo de bloques (arena ~22)
        const float a = BP_TAU * i / 32.0f;
        bpBox(cosf(a) * 27.5f, sinf(a) * 27.5f, 8.5f, 8.5f, 40.0f, bpC(100, 84, 70, 255));
    }
    bpBox(0.0f, 0.0f, 3.4f, 3.4f, 1.1f, bpC(98, 92, 88, 255));          // fogata
    bpBox(0.0f, 0.0f, 120.0f, 120.0f, 0.0f, 0);                         // suelo (techo y=0)
    *geoN = g.n; *nBoxes = g_bpNbx;
    *sx = 0.0f; *sy = 0.0f; *sz = 16.0f;
    io.camYaw = 0.0f;
    bpBuildChar();
    sceKernelDcacheWritebackAll();
}
