// ============================================================================
// boss_gabriel.h -- PISO 4 "EL ARCANGEL": GABRIEL (prefijo gb_ / g_gb / GB_). Contrato: boss_api.h
//
// Benjamin (2026-10-16): Gabriel esta enojado porque usaste el nombre de Dios (el Padre Nuestro y
// la luz que vencio a Baphomet) para pelear. Amenazante, baja del cielo con LANZA y ESCUDO; hay que
// hablar con el antes de pelear. NO baja nunca: ataca desde muy arriba con luz (rayos como el que
// vencio a Baphomet) y te nubla la vista con su resplandor. Al bajar de cierta vida, cinematica:
// le pregunta a Dios si su voluntad es acabar contigo y saca su LANZA, que cae en picada desde el
// cielo (se ve pequena y crece al acercarse) e impacta 3/4 de la plataforma. Vencido, te agarra y
// caen los dos en picada a la tierra (como Miguel con Satanas): de ahi se pasa al piso 5.
//
// Escala: la plataforma mide 30 de radio. Gabriel mide ~40 y flota a ~125 del centro, mas alla del
// borde: asi entra en cuadro con la camara normal (si estuviera encima no se veria nunca).
// ============================================================================
#pragma once

static const int   GB_HP   = 160;
static const int   GB_PH2  = 88;            // 55%: cinematica de la voluntad + lanzas
static const float GB_R    = 30.0f;         // radio de la plataforma
static const float GB_S    = 8.0f;          // escala de Gabriel (modelo de ~5 de alto -> ~40)
static const float GB_DIST = 125.0f;        // distancia de Gabriel al centro
static const float GB_HOVY = 4.0f;          // altura de sus pies (sobre el vacio): mas arriba se sale del cuadro (la camara
                                            // mira casi al frente) y con 40 de alto igual domina el cielo
#ifndef NOCTIS_TEST_GBFASE
#define NOCTIS_TEST_GBFASE 0   // PRUEBA: 2 = la pelea arranca en la voluntad + lanzas; 3 = arranca la caida final. 0 al commitear
#endif
static const float GB_TAU  = 6.2831853f, GB_PI = 3.14159265f;

enum { GBS_DESCEND, GBS_TALK, GBS_CHOICE, GBS_FIGHT, GBS_WILL, GBS_FALLTALK, GBS_DIVE, GBS_FALLING, GBS_DONE };
enum { GBA_NONE, GBA_BEAMS, GBA_FEATHERS, GBA_GLARE, GBA_SPEAR };

// ---------------------------------------------------------------- estado
static int   g_gbSt = GBS_DESCEND, g_gbStT = 0, g_gbT = 0, g_gbLine = 0, g_gbSel = 0, g_gbChoice = -1;
static int   g_gbHp = GB_HP, g_gbPhase = 1, g_gbHurt = 0, g_gbCineT0 = 0;
static float g_gbAng = 0.0f, g_gbX = 0.0f, g_gbY = 240.0f, g_gbZ = -GB_DIST, g_gbYaw = 0.0f;
static int   g_gbAtk = GBA_NONE, g_gbAtkT = 0, g_gbPause = 0, g_gbSeq = 0;
static float g_gbBlind = 0.0f;                                // 0..1: cuanto te nubla la vista
static float g_gbRaise = 0.0f;                                // 0..1: lanza en alto (pose)
// rayos de luz en cadena
struct GbBeam { float x, z; int t, hit; };
static GbBeam g_gbBm[6]; static int g_gbBmN = 0;
// la LANZA del cielo
static int   g_gbSpT = 0;                                     // 0 = sin lanza en curso
static float g_gbSafeA = 0.0f;                                // angulo del cuarto que queda a salvo
static float g_gbSpSX = 0.0f, g_gbSpSY = 0.0f, g_gbSpSZ = 0.0f; // de donde sale
// caida final
static float g_gbFallX = 0.0f, g_gbFallZ = 0.0f, g_gbFallDX = 0.0f, g_gbFallDZ = 1.0f, g_gbFallY = 0.0f, g_gbFallV = 0.0f;
// modelo
static LineVertex __attribute__((aligned(16))) g_gbBody[1500];
static LineVertex __attribute__((aligned(16))) g_gbWing[260];
static LineVertex __attribute__((aligned(16))) g_gbSpear[220];
static LineVertex __attribute__((aligned(16))) g_gbShield[200];
static int g_gbBodyN = 0, g_gbWingN = 0, g_gbSpearN = 0, g_gbShieldN = 0;
static CityBldg *g_gbBx = 0; static int g_gbNbx = 0, g_gbBxCap = 0;

// ---------------------------------------------------------------- textos
struct GbLine { const char *who, *a, *b; };
static const GbLine kGbTalk[3] = {
    { "GABRIEL", "Detente, soñador.", "" },
    { "GABRIEL", "Vi la luz que hiciste caer sobre la bestia.", "" },
    { "GABRIEL", "Usaste el nombre de Dios como un arma.", "" },
};
static const char *kGbOpt[2] = { "Solo quería salir de esa cueva.", "Dios estaba conmigo." };
static const GbLine kGbReply[2] = {
    { "GABRIEL", "Salir no te da derecho a Su nombre.", "Te enseñaré lo que pesa la luz." },
    { "GABRIEL", "¿Contigo? Él no se presta a nadie.", "Te enseñaré lo que pesa la luz." },
};

// ---------------------------------------------------------------- utilidades
static unsigned int gbC(int r, int g, int b, int a) {
    r = r < 0 ? 0 : (r > 255 ? 255 : r); g = g < 0 ? 0 : (g > 255 ? 255 : g);
    b = b < 0 ? 0 : (b > 255 ? 255 : b); a = a < 0 ? 0 : (a > 255 ? 255 : a);
    return (unsigned int)((a << 24) | (b << 16) | (g << 8) | r);
}
static float gbClamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
static float gbWrap(float a) { while (a > GB_PI) a -= GB_TAU; while (a < -GB_PI) a += GB_TAU; return a; }
static void gbBox(float x, float z, float w, float d, float h) {
    if (g_gbBx && g_gbNbx < g_gbBxCap) g_gbBx[g_gbNbx++] = { x, z, w, d, h, 0 };
}
// resplandor que mira a la camara
static void gbGlow(float x, float y, float z, float R, unsigned int cIn, unsigned int cOut) {
    const int N = 14;
    LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 3 * N);
    for (int s = 0; s < N; ++s) {
        const float a0 = GB_TAU * s / N, a1 = GB_TAU * (s + 1) / N;
        const float c0 = cosf(a0) * R, s0 = sinf(a0) * R, c1 = cosf(a1) * R, s1 = sinf(a1) * R;
        v[s * 3 + 0] = { cIn, x, y, z };
        v[s * 3 + 1] = { cOut, x + g_lvRight[0] * c0 + g_lvUp[0] * s0, y + g_lvRight[1] * c0 + g_lvUp[1] * s0, z + g_lvRight[2] * c0 + g_lvUp[2] * s0 };
        v[s * 3 + 2] = { cOut, x + g_lvRight[0] * c1 + g_lvUp[0] * s1, y + g_lvRight[1] * c1 + g_lvUp[1] * s1, z + g_lvRight[2] * c1 + g_lvUp[2] * s1 };
    }
    sceGumLoadIdentity(); sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, 3 * N, 0, v);
}
static void gbAdd()   { sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_FIX, 0, 0xFFFFFFFF); sceGuDepthMask(GU_TRUE); }
static void gbAlpha() { sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0); sceGuDepthMask(GU_TRUE); }
static void gbOff()   { sceGuDepthMask(GU_FALSE); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0); sceGuDisable(GU_BLEND); }
// punto del pecho (blanco del autoapuntado) y de la mano de la lanza, en el mundo
static void gbChest(float *o) { o[0] = g_gbX; o[1] = g_gbY + 3.2f * GB_S; o[2] = g_gbZ; }
static void gbHand(float *o) {
    const float rx = cosf(g_gbYaw), rz = -sinf(g_gbYaw);
    o[0] = g_gbX + rx * 0.9f * GB_S; o[1] = g_gbY + (3.3f + 1.6f * g_gbRaise) * GB_S; o[2] = g_gbZ + rz * 0.9f * GB_S;
}

// ---------------------------------------------------------------- modelo (escala 1, mira a -Z, pies en y=0)
static void gbBuildModel() {
    int i = 0;
    LineVertex *b = g_gbBody;
    const unsigned int ROBE = gbC(238, 234, 222, 255), ROBE2 = gbC(255, 250, 238, 255), GOLD = gbC(226, 190, 104, 255);
    const unsigned int GOLD2 = gbC(255, 226, 140, 255), SKIN = gbC(238, 222, 204, 255), HAIR = gbC(214, 176, 92, 255);
    addLimb(b, i, 0.0f, 0.0f, 0.0f, 0.0f, 1.4f, 0.0f, 1.25f, 0.85f, 10, ROBE, ROBE2);       // tunica (falda ancha)
    addLimb(b, i, 0.0f, 1.4f, 0.0f, 0.0f, 2.6f, 0.0f, 0.85f, 0.55f, 10, ROBE2, ROBE);
    addLimb(b, i, 0.0f, 2.6f, 0.0f, 0.0f, 3.75f, 0.0f, 0.56f, 0.62f, 8, GOLD, GOLD2);        // coraza
    addLimb(b, i, 0.0f, 2.62f, 0.0f, 0.0f, 2.78f, 0.0f, 0.62f, 0.62f, 8, GOLD2, GOLD2);       // cinto
    addBall(b, i, 0.0f, 3.62f, 0.0f, 0.78f, 0.24f, 0.46f, 3, 8, GOLD2);                       // hombreras
    addLimb(b, i, 0.0f, 3.75f, 0.0f, 0.0f, 3.95f, 0.0f, 0.16f, 0.14f, 6, SKIN, SKIN);         // cuello
    addBall(b, i, 0.0f, 4.18f, 0.0f, 0.27f, 0.32f, 0.29f, 4, 8, SKIN);                         // cabeza
    addBall(b, i, 0.0f, 4.30f, 0.07f, 0.31f, 0.27f, 0.31f, 3, 8, HAIR);                        // cabello
    // brazo derecho (lanza): del hombro al codo y a la mano, adelante
    addLimb(b, i, 0.62f, 3.65f, 0.0f, 0.86f, 2.95f, -0.12f, 0.17f, 0.14f, 6, GOLD, GOLD2);
    addLimb(b, i, 0.86f, 2.95f, -0.12f, 0.90f, 3.30f, -0.45f, 0.14f, 0.12f, 6, SKIN, SKIN);
    // brazo izquierdo (escudo)
    addLimb(b, i, -0.62f, 3.65f, 0.0f, -0.88f, 3.00f, -0.18f, 0.17f, 0.14f, 6, GOLD, GOLD2);
    addLimb(b, i, -0.88f, 3.00f, -0.18f, -0.92f, 3.05f, -0.50f, 0.14f, 0.12f, 6, SKIN, SKIN);
    // aureola: anillo plano detras de la cabeza
    for (int s = 0; s < 16 && i + 6 <= 1500; ++s) {
        const float a0 = GB_TAU * s / 16, a1 = GB_TAU * (s + 1) / 16, ri = 0.48f, ro = 0.62f, y = 4.25f, z = 0.18f;
        swQuadC(b, i, cosf(a0) * ri, y + sinf(a0) * ri, z, GOLD2, cosf(a1) * ri, y + sinf(a1) * ri, z, GOLD2,
                      cosf(a1) * ro, y + sinf(a1) * ro, z, gbC(255, 240, 180, 255), cosf(a0) * ro, y + sinf(a0) * ro, z, gbC(255, 240, 180, 255));
    }
    g_gbBodyN = i;
    // ALA (derecha; la izquierda se dibuja en espejo): plumas largas en abanico, hacia arriba y afuera
    i = 0; LineVertex *w = g_gbWing;
    for (int row = 0; row < 2; ++row)
        for (int k = 0; k < 9 && i + 6 <= 260; ++k) {
            const float an = 0.25f + k * 0.17f + row * 0.06f;              // de casi horizontal a vertical
            const float L = (row ? 2.6f : 4.2f) - k * (row ? 0.10f : 0.20f), wd = row ? 0.30f : 0.42f;
            const float rx = 0.40f, ry = 3.55f, rz = 0.32f + row * 0.05f;
            const float tx = rx + cosf(an) * L, ty = ry + sinf(an) * L;
            const float nx = -sinf(an) * wd, ny = cosf(an) * wd;
            const unsigned int cR = row ? gbC(255, 250, 238, 255) : gbC(244, 240, 230, 255), cT = gbC(255, 226, 150, 255);
            swQuadC(w, i, rx, ry, rz, cR, rx + cosf(an) * L * 0.5f + nx, ry + sinf(an) * L * 0.5f + ny, rz + 0.1f, cR,
                          tx, ty, rz + 0.2f, cT, rx + cosf(an) * L * 0.5f - nx, ry + sinf(an) * L * 0.5f - ny, rz + 0.1f, cR);
        }
    g_gbWingN = i;
    // LANZA (en la mano, vertical; y=0 en la mano): asta dorada, guarda y hoja de luz
    i = 0; LineVertex *sp = g_gbSpear;
    addLimb(sp, i, 0.0f, -1.6f, 0.0f, 0.0f, 4.4f, 0.0f, 0.07f, 0.06f, 6, gbC(200, 160, 80, 255), gbC(255, 220, 130, 255));
    addLimb(sp, i, -0.35f, 4.4f, 0.0f, 0.35f, 4.4f, 0.0f, 0.07f, 0.07f, 5, GOLD2, GOLD2);
    {   // hoja: rombo de 4 caras, blanco caliente
        const float y0 = 4.4f, y1 = 5.1f, y2 = 6.3f, r = 0.22f;
        const unsigned int cA = gbC(255, 255, 240, 255), cB = gbC(255, 236, 170, 255);
        for (int k = 0; k < 4 && i + 6 <= 220; ++k) {
            const float a0 = k * GB_PI * 0.5f, a1 = a0 + GB_PI * 0.5f;
            sp[i++] = { cB, 0.0f, y0, 0.0f }; sp[i++] = { cA, cosf(a0) * r, y1, sinf(a0) * r }; sp[i++] = { cA, cosf(a1) * r, y1, sinf(a1) * r };
            sp[i++] = { cA, cosf(a0) * r, y1, sinf(a0) * r }; sp[i++] = { cB, 0.0f, y2, 0.0f }; sp[i++] = { cA, cosf(a1) * r, y1, sinf(a1) * r };
        }
    }
    g_gbSpearN = i;
    // ESCUDO: disco con sol de oro, mira a -Z
    i = 0; LineVertex *sh = g_gbShield;
    for (int s = 0; s < 16 && i + 9 <= 200; ++s) {
        const float a0 = GB_TAU * s / 16, a1 = GB_TAU * (s + 1) / 16, R = 0.95f;
        sh[i++] = { gbC(250, 246, 236, 255), 0.0f, 0.0f, -0.05f };
        sh[i++] = { gbC(214, 210, 200, 255), cosf(a0) * R, sinf(a0) * R, 0.0f };
        sh[i++] = { gbC(214, 210, 200, 255), cosf(a1) * R, sinf(a1) * R, 0.0f };
        swQuadC(sh, i, cosf(a0) * R, sinf(a0) * R, 0.0f, GOLD, cosf(a1) * R, sinf(a1) * R, 0.0f, GOLD,
                       cosf(a1) * (R + 0.12f), sinf(a1) * (R + 0.12f), 0.02f, GOLD2, cosf(a0) * (R + 0.12f), sinf(a0) * (R + 0.12f), 0.02f, GOLD2);
    }
    for (int k = 0; k < 8 && i + 3 <= 200; ++k) {                        // rayos del sol grabado
        const float a = GB_TAU * k / 8;
        sh[i++] = { GOLD2, 0.0f, 0.0f, -0.08f };
        sh[i++] = { GOLD, cosf(a - 0.12f) * 0.25f, sinf(a - 0.12f) * 0.25f, -0.07f };
        sh[i++] = { GOLD2, cosf(a) * 0.80f, sinf(a) * 0.80f, -0.06f };
    }
    g_gbShieldN = i;
}

// ---------------------------------------------------------------- escenario
static void gbBuildArena(LineVertex *geo, int cap, int *n) {
    int i = 0;
    const float rings[5] = { 0.0f, 6.0f, 14.0f, 22.0f, GB_R };
    const int SEG = 28;
    for (int r = 0; r < 4; ++r)                                        // marmol en anillos (paneles <= 8u)
        for (int s = 0; s < SEG && i + 6 <= cap; ++s) {
            const float a0 = GB_TAU * s / SEG, a1 = GB_TAU * (s + 1) / SEG, r0 = rings[r], r1 = rings[r + 1];
            const unsigned int c = ((s + r) & 1) ? gbC(236, 232, 222, 255) : gbC(222, 218, 208, 255);
            swQuadC(geo, i, cosf(a0) * r0, 0.0f, sinf(a0) * r0, c, cosf(a1) * r0, 0.0f, sinf(a1) * r0, c,
                            cosf(a1) * r1, 0.0f, sinf(a1) * r1, c, cosf(a0) * r1, 0.0f, sinf(a0) * r1, c);
        }
    for (int r = 1; r < 5; ++r)                                        // anillos de oro incrustados
        for (int s = 0; s < SEG && i + 6 <= cap; ++s) {
            const float a0 = GB_TAU * s / SEG, a1 = GB_TAU * (s + 1) / SEG, rr = rings[r], w = 0.3f;
            const unsigned int c = gbC(226, 192, 110, 255);
            swQuadC(geo, i, cosf(a0) * (rr - w), 0.02f, sinf(a0) * (rr - w), c, cosf(a1) * (rr - w), 0.02f, sinf(a1) * (rr - w), c,
                            cosf(a1) * rr, 0.02f, sinf(a1) * rr, c, cosf(a0) * rr, 0.02f, sinf(a0) * rr, c);
        }
    for (int s = 0; s < SEG && i + 12 <= cap; ++s) {                   // canto y la roca de abajo (cono)
        const float a0 = GB_TAU * s / SEG, a1 = GB_TAU * (s + 1) / SEG;
        const unsigned int cE = gbC(206, 176, 104, 255), cU = gbC(150, 146, 156, 255), cD = gbC(70, 66, 80, 255);
        swQuadC(geo, i, cosf(a0) * GB_R, 0.0f, sinf(a0) * GB_R, cE, cosf(a1) * GB_R, 0.0f, sinf(a1) * GB_R, cE,
                        cosf(a1) * GB_R, -1.2f, sinf(a1) * GB_R, cU, cosf(a0) * GB_R, -1.2f, sinf(a0) * GB_R, cU);
        swQuadC(geo, i, cosf(a0) * GB_R, -1.2f, sinf(a0) * GB_R, cU, cosf(a1) * GB_R, -1.2f, sinf(a1) * GB_R, cU,
                        cosf(a1) * 4.0f, -34.0f, sinf(a1) * 4.0f, cD, cosf(a0) * 4.0f, -34.0f, sinf(a0) * 4.0f, cD);
    }
    *n = i;
    // colision: la plataforma redonda en franjas (cajas con techo y=0); fuera de ellas, el vacio
    const int ST = 12;
    for (int k = 0; k < ST; ++k) {
        const float z0 = -GB_R + 2.0f * GB_R * k / ST, z1 = z0 + 2.0f * GB_R / ST, zm = 0.5f * (z0 + z1);
        const float zz = fabsf(zm) < fabsf(z0) && fabsf(zm) < fabsf(z1) ? 0.0f : (fabsf(z0) < fabsf(z1) ? z0 : z1);
        const float half = sqrtf(gbClamp(GB_R * GB_R - zz * zz, 0.0f, GB_R * GB_R));
        gbBox(0.0f, zm, 2.0f * half, z1 - z0, 0.0f);
    }
}

// ---------------------------------------------------------------- logica
static void gbResetAll() {
    g_gbSt = GBS_DESCEND; g_gbStT = 0; g_gbT = 0; g_gbLine = 0; g_gbSel = 0; g_gbChoice = -1;
    g_gbHp = GB_HP; g_gbPhase = 1; g_gbHurt = 0;
    g_gbAng = 0.0f; g_gbX = 0.0f; g_gbY = 240.0f; g_gbZ = -GB_DIST; g_gbYaw = 0.0f;
    g_gbAtk = GBA_NONE; g_gbAtkT = 0; g_gbPause = 100; g_gbSeq = 0; g_gbBlind = 0.0f; g_gbRaise = 0.0f;
    g_gbBmN = 0; g_gbSpT = 0;
    g_lvNoFallDeath = 0;
}
static void gbEndAtk(int pause) { g_gbAtk = GBA_NONE; g_gbAtkT = 0; g_gbPause = (g_gbPhase == 1) ? pause : pause * 3 / 4; }
static void gbStartAtk(BossIO &io, int a) {
    g_gbAtk = a; g_gbAtkT = 0;
    switch (a) {
    case GBA_BEAMS: g_gbBmN = 0; lvMessage("¡LA LUZ CAE SOBRE TI!", 90); break;
    case GBA_FEATHERS: lvMessage("¡PLUMAS DE LUZ!", 80); break;
    case GBA_GLARE: lvMessage("¡APARTA LA VISTA DE SU LUZ!", 130); break;
    case GBA_SPEAR: {
        // el cuarto a salvo: nunca donde estas parado (hay que correr)
        const float pa = atan2f(io.pz, io.px);
        g_gbSafeA = pa + GB_PI * (0.5f + swRand(g_gbT, 4, 41));
        g_gbSpT = 1; lvMessage("¡LA LANZA DEL CIELO! BUSCA LA LUZ DORADA", 170);
    } break;
    default: break;
    }
}
static int gbInSafe(float x, float z) {
    if (x * x + z * z < 4.0f * 4.0f) return 0;                      // el centro nunca es seguro
    return fabsf(gbWrap(atan2f(z, x) - g_gbSafeA)) < GB_PI * 0.25f;   // cuarto (90 grados)
}
static void gbRunAtk(BossIO &io) {
    ++g_gbAtkT;
    switch (g_gbAtk) {
    case GBA_BEAMS:   // 5 rayos en cadena sobre donde estas (con un poco de adelanto)
        if (g_gbAtkT % 28 == 1 && g_gbBmN < 5) {
            GbBeam &b = g_gbBm[g_gbBmN++];
            b.x = io.px; b.z = io.pz; b.t = 1; b.hit = 0;
        }
        if (g_gbAtkT > 28 * 5 + 80) gbEndAtk(90);
        break;
    case GBA_FEATHERS: {
        float h[3]; gbHand(h);
        if (g_gbAtkT % 11 == 5 && g_gbAtkT < 130)
            lvFireOrb(h[0], h[1], h[2], io.px + (swRand(g_gbAtkT, 1, 43) - 0.5f) * 4.0f, io.py + 1.7f, io.pz + (swRand(g_gbAtkT, 2, 43) - 0.5f) * 4.0f,
                      1.25f, RGBA(255, 244, 200, 255));
        if (g_gbAtkT > 170) gbEndAtk(80);
    } break;
    case GBA_GLARE: if (g_gbAtkT > 200) gbEndAtk(70); break;
    case GBA_SPEAR: if (g_gbSpT == 0) gbEndAtk(90); break;
    default: break;
    }
}
// rayos y lanza: tiempos y dano (corre aunque el ataque ya haya terminado)
static void gbHazards(BossIO &io) {
    for (int k = 0; k < g_gbBmN; ++k) {
        GbBeam &b = g_gbBm[k]; if (b.t <= 0) continue;
        ++b.t;
        if (b.t == 52 && !b.hit) {                                     // cae el rayo
            b.hit = 1; g_lvFlash = 4;
            const float dx = io.px - b.x, dz = io.pz - b.z;
            if (!g_lvDashInv && dx * dx + dz * dz < 3.4f * 3.4f && io.py < 9.0f) { io.hp -= 18.0f; g_lvFlash = 12; }
        }
        if (b.t > 76) b.t = 0;
    }
    if (g_gbSpT > 0) {
        ++g_gbSpT;
        // sale de MUY lejos, detras de el y algo alta: entra en cuadro como un punto (unos 12 grados sobre
        // el horizonte) y crece hasta ser gigante. Si saliera de encima, la camara no la veria hasta el final.
        if (g_gbSpT == 2) { g_gbSpSX = g_gbX * 1.9f; g_gbSpSY = 62.0f; g_gbSpSZ = g_gbZ * 1.9f; }
        if (g_gbSpT == 210) {                                          // IMPACTO: 3/4 de la plataforma
            g_lvFlash = 40;
            if (!g_lvDashInv && !gbInSafe(io.px, io.pz) && io.py < 12.0f) { io.hp -= 42.0f; io.vy = 6.0f; }
        }
        if (g_gbSpT > 330) g_gbSpT = 0;
    }
}
static void gbFightUpdate(BossIO &io) {
    // flota en el cielo: deriva lenta frente a ti, y siempre te mira
    g_gbAng += 0.0016f * sinf(g_gbT * 0.004f);
    const float hy = GB_HOVY + (g_gbPhase >= 2 ? 10.0f : 0.0f);
    g_gbX += (sinf(g_gbAng) * GB_DIST - g_gbX) * 0.02f; g_gbZ += (-cosf(g_gbAng) * GB_DIST - g_gbZ) * 0.02f;
    g_gbY += (hy + 3.0f * sinf(g_gbT * 0.02f) - g_gbY) * 0.03f;
    if (io.aiming) { const float d = gbWrap(atan2f(g_gbX - io.px, -(g_gbZ - io.pz)) - io.camYaw); io.camYaw += d * 0.10f; }
    else if (g_gbSpT > 0 && g_gbSpT < 100) {   // la lanza parte: la camara se vuelve hacia el (suave) para verla venir
        const float d = gbWrap(atan2f(g_gbX - io.px, -(g_gbZ - io.pz)) - io.camYaw); io.camYaw += d * 0.035f;
    }
    if (g_lvTimeStop > 0) return;
    // resplandor: si lo miras mientras brilla, la vista se te nubla
    {
        float c[3]; gbChest(c);
        const float dx = c[0] - g_lvEye[0], dy = c[1] - g_lvEye[1], dz = c[2] - g_lvEye[2], d = sqrtf(dx * dx + dy * dy + dz * dz) + 0.001f;
        const float look = (dx * g_lvFwd[0] + dy * g_lvFwd[1] + dz * g_lvFwd[2]) / d;
        if (g_gbAtk == GBA_GLARE && g_gbAtkT > 40 && look > 0.80f) g_gbBlind += 0.022f;
        else g_gbBlind -= 0.005f;
        g_gbBlind = gbClamp(g_gbBlind, 0.0f, 1.0f);
    }
    if (g_gbAtk == GBA_NONE) {
        if (--g_gbPause <= 0) {
            static const int kSeq1[4] = { GBA_BEAMS, GBA_FEATHERS, GBA_GLARE, GBA_FEATHERS };
            static const int kSeq2[5] = { GBA_SPEAR, GBA_BEAMS, GBA_SPEAR, GBA_GLARE, GBA_FEATHERS };
            const int a = (g_gbPhase == 1) ? kSeq1[g_gbSeq % 4] : kSeq2[g_gbSeq % 5];
            ++g_gbSeq; gbStartAtk(io, a);
        }
    } else gbRunAtk(io);
    gbHazards(io);
}

static bool gb_ShotHit(float x, float y, float z, int dmg);
static void gb_Update(BossIO &io) {
    ++g_gbT; ++g_gbStT;
    if (g_gbHurt > 0) --g_gbHurt;
    { const float t = atan2f(-(io.px - g_gbX), -(io.pz - g_gbZ)); g_gbYaw += gbWrap(t - g_gbYaw) * 0.05f; }
    const float raiseWant = (g_gbAtk == GBA_SPEAR && g_gbSpT > 0 && g_gbSpT < 60) || g_gbSt == GBS_WILL ? 1.0f : 0.0f;
    g_gbRaise += (raiseWant - g_gbRaise) * 0.06f;
    switch (g_gbSt) {
    case GBS_DESCEND:   // la cinematica de entrada ya lo mostro bajando: termina de posarse en el cielo
        g_gbY += (GB_HOVY - g_gbY) * 0.08f;
        if (g_gbStT > 20) { g_gbSt = GBS_TALK; g_gbStT = 0; g_gbLine = 0; }
        break;
    case GBS_TALK:
        { const float d = gbWrap(atan2f(g_gbX - io.px, -(g_gbZ - io.pz)) - io.camYaw); io.camYaw += d * 0.06f; }
        if (io.edge & PSP_CTRL_CROSS) {
            if (g_gbChoice < 0) { if (++g_gbLine >= 3) { g_gbSt = GBS_CHOICE; g_gbSel = 0; } }
            else {
                g_gbSt = GBS_FIGHT; g_gbStT = 0; g_gbPause = 70; g_gbSeq = 0; lvMessage("¡GABRIEL ATACA DESDE EL CIELO!", 120);
                if (NOCTIS_TEST_GBFASE == 2) gb_ShotHit(g_gbX, g_gbY + 3.2f * GB_S, g_gbZ, g_gbHp - GB_PH2);
                if (NOCTIS_TEST_GBFASE == 3) gb_ShotHit(g_gbX, g_gbY + 3.2f * GB_S, g_gbZ, 999);
            }
        }
        break;
    case GBS_CHOICE:
        if (io.edge & (PSP_CTRL_UP | PSP_CTRL_DOWN)) g_gbSel ^= 1;
        if (io.edge & PSP_CTRL_CROSS) { g_gbChoice = g_gbSel; g_gbSt = GBS_TALK; }
        break;
    case GBS_FIGHT: gbFightUpdate(io); break;
    case GBS_WILL:      // la cinematica ya paso (Update no corre durante ella): "Que asi sea" y lanzas
        if (io.edge & PSP_CTRL_CROSS) { g_gbSt = GBS_FIGHT; g_gbStT = 0; g_gbPause = 30; g_gbSeq = 0; g_gbPhase = 2; }
        break;
    case GBS_FALLTALK:
        if (io.edge & PSP_CTRL_CROSS) {
            g_gbSt = GBS_DIVE; g_gbStT = 0;
            g_gbFallX = io.px; g_gbFallZ = io.pz; g_gbFallY = io.py;
            float dx = io.px, dz = io.pz, l = sqrtf(dx * dx + dz * dz);
            if (l < 1.0f) { dx = 0.0f; dz = 1.0f; l = 1.0f; }
            g_gbFallDX = dx / l; g_gbFallDZ = dz / l;
            lvMessage("GABRIEL SE LANZA SOBRE TI", 120);
        }
        break;
    case GBS_DIVE: {    // baja en picada, te agarra y te saca de la plataforma
        const float t = gbClamp(g_gbStT / 70.0f, 0.0f, 1.0f), e = t * t;
        const float tx = io.px + g_gbFallDX * 1.2f, ty = io.py + 2.6f, tz = io.pz + g_gbFallDZ * 1.2f;
        g_gbX += (tx - g_gbX) * (0.04f + 0.3f * e); g_gbY += (ty - g_gbY) * (0.04f + 0.3f * e); g_gbZ += (tz - g_gbZ) * (0.04f + 0.3f * e);
        if (g_gbStT >= 70) {
            g_lvNoFallDeath = 1;
            const float k = gbClamp((g_gbStT - 70) / 50.0f, 0.0f, 1.0f);    // te arrastra afuera del borde
            io.px = g_gbFallX + g_gbFallDX * (GB_R + 34.0f - g_gbFallX * g_gbFallDX - g_gbFallZ * g_gbFallDZ) * k;   // lejos del borde: si no,
            io.pz = g_gbFallZ + g_gbFallDZ * (GB_R + 34.0f - g_gbFallX * g_gbFallDX - g_gbFallZ * g_gbFallDZ) * k;   // al caer solo se ve la roca de abajo
            io.py = g_gbFallY + 6.0f * sinf(k * GB_PI * 0.5f); io.vy = 0.0f;
            g_gbX = io.px + g_gbFallDX * 1.2f; g_gbY = io.py + 2.6f; g_gbZ = io.pz + g_gbFallDZ * 1.2f;   // encima, agarrandote
        }
        if (g_gbStT == 70) g_lvFlash = 20;
        if (g_gbStT >= 120) { g_gbSt = GBS_FALLING; g_gbStT = 0; g_gbFallV = 0.0f; }
    } break;
    case GBS_FALLING:   // en picada los dos, a traves de las nubes, hasta la tierra
        g_gbFallV += 0.035f; if (g_gbFallV > 2.2f) g_gbFallV = 2.2f;
        io.py -= g_gbFallV; io.vy = 0.0f;
        g_gbX = io.px + g_gbFallDX * 1.2f; g_gbY = io.py + 2.6f; g_gbZ = io.pz + g_gbFallDZ * 1.2f;
        if (g_gbStT == 150) lvMessage("CAEN LOS DOS. LA TIERRA SE ACERCA", 150);
        if (g_gbStT >= 260) { g_gbSt = GBS_DONE; g_lvFlash = 60; }
        break;
    default: break;
    }
}

// ---------------------------------------------------------------- dibujo 3D
static void gbDrawGabriel() {
    if (g_gbSt == GBS_DONE) return;
    // al entrar baja del cielo (la cinematica de levels la muestra): curva lenta al principio
    if (g_gbSt == GBS_DESCEND) { const float t = gbClamp(g_lvT / 300.0f, 0.0f, 1.0f); g_gbY = GB_HOVY + 210.0f * (1.0f - t) * (1.0f - t); }
    float S = GB_S;                                                    // al agarrarte se achica: si no, su tunica
    if (g_gbSt == GBS_DIVE) { const float k = gbClamp(g_gbStT / 70.0f, 0.0f, 1.0f); S = GB_S + (1.6f - GB_S) * k; }   // tapa la pantalla
    else if (g_gbSt == GBS_FALLING) S = 1.6f;
    const float flap = sinf(g_lvT * (g_gbSt == GBS_FALLING ? 0.25f : 0.05f)) * (g_gbSt == GBS_FALLING ? 0.05f : 0.12f);
    const int hot = g_gbHurt > 0 && ((g_gbHurt >> 1) & 1);
    sceGumLoadIdentity();
    { ScePspFVector3 t = { g_gbX, g_gbY, g_gbZ }; sceGumTranslate(&t); }
    sceGumRotateY(g_gbYaw);
    if (g_gbSt == GBS_FALLING) sceGumRotateX(-1.2f);                  // cabeza abajo, en picada
    { ScePspFVector3 s = { S, S, S }; sceGumScale(&s); }
    if (hot) { gbAdd(); }
    sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g_gbBodyN, 0, g_gbBody);
    for (int side = 0; side < 2; ++side) {                            // alas (la izquierda en espejo)
        sceGumPushMatrix();
        if (side) { ScePspFVector3 m = { -1.0f, 1.0f, 1.0f }; sceGumScale(&m); }
        { ScePspFVector3 p = { 0.4f, 3.55f, 0.3f }; sceGumTranslate(&p); }
        sceGumRotateZ(flap); sceGumRotateY(-0.25f);
        { ScePspFVector3 p = { -0.4f, -3.55f, -0.3f }; sceGumTranslate(&p); }
        sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g_gbWingN, 0, g_gbWing);
        sceGumPopMatrix();
    }
    sceGumPushMatrix();                                                // escudo en la izquierda
    { ScePspFVector3 p = { -0.98f, 3.05f, -0.62f }; sceGumTranslate(&p); }
    sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g_gbShieldN, 0, g_gbShield);
    sceGumPopMatrix();
    // la lanza en la mano (no mientras vuela hacia la plataforma)
    if (!(g_gbSpT > 60 && g_gbSpT < 330)) {
        sceGumPushMatrix();
        { ScePspFVector3 p = { 0.9f, 3.3f + 1.6f * g_gbRaise, -0.45f }; sceGumTranslate(&p); }
        sceGumRotateX(-0.15f * (1.0f - g_gbRaise));
        sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g_gbSpearN, 0, g_gbSpear);
        sceGumPopMatrix();
    }
    if (hot) gbOff();
    sceGumLoadIdentity();
    // halo de luz alrededor (se ve de lejos)
    gbAdd();
    float c[3]; gbChest(c);
    if (g_gbSt == GBS_DIVE || g_gbSt == GBS_FALLING) { c[0] = g_gbX; c[1] = g_gbY + 3.0f * S; c[2] = g_gbZ; }
    const int glare = (g_gbAtk == GBA_GLARE && g_gbSt == GBS_FIGHT) ? (g_gbAtkT < 40 ? g_gbAtkT * 4 : 160) : 0;
    gbGlow(c[0], c[1], c[2], 3.2f * S, gbC(255, 248, 220, 70 + glare / 2), gbC(255, 240, 190, 0));
    gbGlow(c[0], c[1] + 1.0f * S, c[2], 1.0f * S, gbC(255, 255, 240, 120 + glare / 2), gbC(255, 240, 200, 0));
    gbOff();
}
// columna de luz que baja del cielo (la misma que vencio a Baphomet)
static void gbLightColumn(float x, float z, float r, float a) {
    LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 12 * 6);
    int n = 0; const int N = 6;
    for (int k = 0; k < N; ++k) {
        const float a0 = GB_TAU * k / N, a1 = GB_TAU * (k + 1) / N;
        const unsigned int c0 = gbC(255, 250, 225, (int)(220 * a)), c1 = gbC(255, 244, 200, 0);
        swQuadC(v, n, x + cosf(a0) * r, 0.0f, z + sinf(a0) * r, c0, x + cosf(a1) * r, 0.0f, z + sinf(a1) * r, c0,
                      x + cosf(a1) * r, 160.0f, z + sinf(a1) * r, c1, x + cosf(a0) * r, 160.0f, z + sinf(a0) * r, c1);
    }
    sceGumLoadIdentity(); sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, n, 0, v);
}
static void gbDrawHazards() {
    gbAdd();
    for (int k = 0; k < g_gbBmN; ++k) {                                // rayos: aviso en el suelo y la columna
        const GbBeam &b = g_gbBm[k]; if (b.t <= 0) continue;
        if (b.t < 52) {
            const float p = b.t / 52.0f;
            lvRing3D(b.x, 0.06f, b.z, 3.4f * (1.3f - 0.3f * p), 0.25f, gbC(255, 236, 160, 90 + (int)(140 * p)));
        } else {
            const float f = 1.0f - (b.t - 52) / 24.0f;
            gbLightColumn(b.x, b.z, 2.6f, f);
            lvRing3D(b.x, 0.06f, b.z, 3.4f + (b.t - 52) * 0.3f, 0.5f, gbC(255, 250, 220, (int)(200 * f)));
        }
    }
    gbOff();
    if (g_gbSpT > 0) {
        // aviso: 3/4 de la plataforma se tiñen de ROJO; el cuarto a salvo, de ORO (mezcla normal: sobre
        // marmol blanco la luz sumada no se distingue)
        gbAlpha();
        const float pulse = 0.5f + 0.5f * sinf(g_lvT * 0.25f);
        const int SEG = 24;
        LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * SEG * 3);
        int n = 0;
        const float lit = (g_gbSpT < 210) ? 1.0f : gbClamp(1.0f - (g_gbSpT - 210) / 40.0f, 0.0f, 1.0f);
        for (int s = 0; s < SEG; ++s) {
            const float a0 = g_gbSafeA + GB_PI * 0.25f + (GB_TAU * 0.75f) * s / SEG, a1 = g_gbSafeA + GB_PI * 0.25f + (GB_TAU * 0.75f) * (s + 1) / SEG;
            const unsigned int cc = gbC(170, 14, 10, (int)((90 + 80 * pulse) * lit)), ce = gbC(200, 30, 20, (int)((70 + 60 * pulse) * lit));
            v[n++] = { cc, 0.0f, 0.08f, 0.0f }; v[n++] = { ce, cosf(a0) * GB_R, 0.08f, sinf(a0) * GB_R }; v[n++] = { ce, cosf(a1) * GB_R, 0.08f, sinf(a1) * GB_R };
        }
        sceGumLoadIdentity(); sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, n, 0, v);
        if (g_gbSpT < 210) {
            LineVertex *w = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 8 * 3); int m = 0;
            for (int s = 0; s < 8; ++s) {
                const float a0 = g_gbSafeA - GB_PI * 0.25f + (GB_PI * 0.5f) * s / 8, a1 = g_gbSafeA - GB_PI * 0.25f + (GB_PI * 0.5f) * (s + 1) / 8;
                const unsigned int cg = gbC(236, 168, 30, 120 + (int)(90 * pulse));
                w[m++] = { cg, cosf(a0) * 4.0f, 0.09f, sinf(a0) * 4.0f }; w[m++] = { cg, cosf(a0) * GB_R, 0.09f, sinf(a0) * GB_R }; w[m++] = { cg, cosf(a1) * GB_R, 0.09f, sinf(a1) * GB_R };
            }
            sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, m, 0, w);
        } else {
            gbAdd();
            lvRing3D(0.0f, 0.1f, 0.0f, (g_gbSpT - 210) * 0.9f, 1.2f, gbC(255, 250, 220, (int)(220 * lit)));   // onda del impacto
        }
        gbOff();
    }
    // la LANZA en picada: sale de su mano (lejos, pequena) y crece al caer sobre el centro
    // (la estela y el brillo van sumados; el cuerpo de la lanza, opaco y grueso)
    if (g_gbSpT > 60 && g_gbSpT < 330) {
        float px, py, pz, dx, dy, dz;
        if (g_gbSpT <= 210) {
            const float t = (g_gbSpT - 60) / 150.0f, e = t * t;         // picada: lenta al principio, brutal al final
            px = g_gbSpSX + (0.0f - g_gbSpSX) * e; py = g_gbSpSY + (0.0f - g_gbSpSY) * e; pz = g_gbSpSZ + (0.0f - g_gbSpSZ) * e;
            dx = -g_gbSpSX; dy = -g_gbSpSY; dz = -g_gbSpSZ;
        } else { px = 0.0f; py = 0.0f; pz = 0.0f; dx = -g_gbSpSX; dy = -g_gbSpSY; dz = -g_gbSpSZ; }   // clavada
        const float l = sqrtf(dx * dx + dy * dy + dz * dz) + 1e-4f; dx /= l; dy /= l; dz /= l;
        // orientar el modelo (eje +Y) a lo largo de la direccion: la punta (y alto) adelante
        const float yaw = atan2f(dx, dz), pitch = acosf(gbClamp(-dy, -1.0f, 1.0f));
        const float fade = (g_gbSpT > 290) ? gbClamp(1.0f - (g_gbSpT - 290) / 40.0f, 0.0f, 1.0f) : 1.0f;
        const float SL = 7.5f * fade;                                    // ~47 de largo: gigante contra la plataforma
        sceGumLoadIdentity();
        { ScePspFVector3 t = { px, py, pz }; sceGumTranslate(&t); }
        sceGumRotateY(yaw); sceGumRotateX(GB_PI - pitch);
        { ScePspFVector3 s = { SL * 4.0f, SL, SL * 4.0f }; sceGumScale(&s); }   // gruesa: es gigante
        { ScePspFVector3 t = { 0.0f, -6.3f, 0.0f }; sceGumTranslate(&t); }   // la punta en (px,py,pz)
        sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g_gbSpearN, 0, g_gbSpear);
        sceGumLoadIdentity();
        gbAdd();
        gbGlow(px, py + 2.0f, pz, (g_gbSpT <= 210 ? 6.0f : 10.0f) * fade, gbC(255, 250, 220, 160), gbC(255, 240, 190, 0));
        if (g_gbSpT <= 210) {   // estela: de donde salio hasta la punta, se ve aunque este lejos
            const float tx = px - dx * 60.0f, ty = py - dy * 60.0f, tz = pz - dz * 60.0f;
            LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 12); int n = 0;
            const unsigned int c0 = gbC(255, 240, 180, 170), c1 = gbC(255, 230, 160, 0);
            swQuadC(v, n, px - g_lvRight[0] * 2.2f, py - g_lvRight[1] * 2.2f, pz - g_lvRight[2] * 2.2f, c0,
                          px + g_lvRight[0] * 2.2f, py + g_lvRight[1] * 2.2f, pz + g_lvRight[2] * 2.2f, c0,
                          tx + g_lvRight[0] * 0.5f, ty + g_lvRight[1] * 0.5f, tz + g_lvRight[2] * 0.5f, c1,
                          tx - g_lvRight[0] * 0.5f, ty - g_lvRight[1] * 0.5f, tz - g_lvRight[2] * 0.5f, c1);
            sceGumLoadIdentity(); sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, n, 0, v);
        }
        gbOff();
    }
}
// nubes bajo la plataforma (y alrededor): se ven al caer
static void gbDrawClouds() {
    gbAlpha();
    for (int k = 0; k < 22; ++k) {
        const float a = k * 2.39996f, r = 40.0f + (k * 37) % 140, y = -30.0f - (float)((k * 53) % 260);
        const float x = cosf(a) * r, z = sinf(a) * r, R = 26.0f + (k * 17) % 30;
        LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 10 * 3);
        for (int s = 0; s < 10; ++s) {
            const float a0 = GB_TAU * s / 10, a1 = GB_TAU * (s + 1) / 10;
            v[s * 3] = { gbC(255, 252, 244, 150), x, y, z };
            v[s * 3 + 1] = { gbC(250, 246, 236, 0), x + cosf(a0) * R, y, z + sinf(a0) * R * 0.7f };
            v[s * 3 + 2] = { gbC(250, 246, 236, 0), x + cosf(a1) * R, y, z + sinf(a1) * R * 0.7f };
        }
        sceGumLoadIdentity(); sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, 30, 0, v);
    }
    gbOff();
}
static void gb_Draw(float px, float py, float pz) {
    (void)px; (void)py; (void)pz;
    sceGuDisable(GU_TEXTURE_2D);
    gbDrawClouds();
    gbDrawGabriel();
    gbDrawHazards();
    sceGumLoadIdentity();
}

// ---------------------------------------------------------------- HUD
static void gb_Hud(BossIO &io) {
    (void)io;
    sceGuDisable(GU_DEPTH_TEST); sceGuDisable(GU_TEXTURE_2D);
    sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    {   // resplandor de Gabriel en pantalla (suma luz); durante GLARE, enorme
        float c[3]; gbChest(c); int sx, sy;
        if (g_gbSt != GBS_DONE && g_gbSt != GBS_FALLING && lvProject(c[0], c[1], c[2], sx, sy)) {
            const int I = (g_gbAtk == GBA_GLARE && g_gbSt == GBS_FIGHT) ? 230 : 70;
            sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_FIX, 0, 0xFFFFFFFF);
            lvFan2D(sx, sy, (g_gbAtk == GBA_GLARE ? 260 : 120), gbC(255, 246, 210, I), gbC(255, 236, 180, 0), 24);
            sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
        }
    }
    if (g_gbBlind > 0.01f) drawRect(0, 0, SCR_WIDTH, SCR_HEIGHT, gbC(255, 252, 240, (int)(245.0f * g_gbBlind)));   // te nubla la vista
    if (g_gbSt == GBS_FALLING || g_gbSt == GBS_DIVE) {                 // velocidad: rayas que suben
        const int t = g_gbStT;
        for (int k = 0; k < 18; ++k) {
            const int x = (k * 73 + 19) % 480, y = 272 - ((t * (9 + k % 5) + k * 41) % 330);
            drawRect(x, y, 2, 30 + (k % 4) * 14, gbC(255, 255, 255, g_gbSt == GBS_FALLING ? 150 : 60));
        }
        if (g_gbSt == GBS_FALLING && g_gbStT > 200) drawRect(0, 0, SCR_WIDTH, SCR_HEIGHT, gbC(255, 255, 255, (g_gbStT - 200) * 4));
    }
    if (g_gbSt == GBS_FIGHT || g_gbSt == GBS_WILL) bxBossBar("EL ARCÁNGEL GABRIEL", g_gbHp, GB_HP);
    switch (g_gbSt) {
    case GBS_TALK:
        if (g_gbChoice < 0) { const GbLine &l = kGbTalk[g_gbLine < 3 ? g_gbLine : 2]; bxDialog(l.who, l.a, 1); }
        else {
            const GbLine &l = kGbReply[g_gbChoice];
            sceGuDisable(GU_TEXTURE_2D);
            drawRect(30, 186, 420, 66, RGBA(8, 10, 16, 215)); drawRect(30, 186, 420, 1, RGBA(200, 176, 120, 200));
            fontTexOn();
            drawText(40, 194, 1.0f, RGBA(236, 206, 120, 255), l.who);
            drawText(40, 212, 1.0f, RGBA(240, 240, 246, 255), l.a);
            drawText(40, 226, 1.0f, RGBA(240, 240, 246, 255), l.b);
            drawText(392, 238, 1.0f, RGBA(160, 170, 190, 255), "X >");
            sceGuDisable(GU_TEXTURE_2D);
        }
        break;
    case GBS_CHOICE:
        bxDialog("GABRIEL", "Usaste el nombre de Dios como un arma.", 0);
        bxChoices("¿Qué le respondes?", kGbOpt, 2, g_gbSel);
        break;
    case GBS_WILL: bxDialog("GABRIEL", "Que así sea.", 1); break;
    case GBS_FALLTALK: bxDialog("GABRIEL", "Si he de caer... caerás conmigo.", 1); break;
    default: break;
    }
    sceGuDisable(GU_TEXTURE_2D); sceGuEnable(GU_DEPTH_TEST);
}

// ---------------------------------------------------------------- API
static void gb_Backdrop() {
    gradQuad(0, 140, RGBA(176, 190, 226, 255), RGBA(255, 244, 214, 255));      // cielo alto: oro y celeste
    gradQuad(140, SCR_HEIGHT, RGBA(255, 244, 214, 255), RGBA(236, 232, 240, 255));
}
static unsigned int gb_Fog(float *nearF, float *farF) { *nearF = 70.0f; *farF = 520.0f; return RGBA(252, 244, 222, 255); }
static int gb_AimTarget(float out[3]) {
    if (g_gbSt != GBS_FIGHT && g_gbSt != GBS_DESCEND && g_gbSt != GBS_TALK) return 0;
    gbChest(out); return 1;
}
static bool gb_ShotHit(float x, float y, float z, int dmg) {
    if (g_gbSt != GBS_FIGHT) return false;
    float c[3]; gbChest(c);
    const float dx = x - c[0], dy = y - c[1], dz = z - c[2];
    if (dx * dx + dy * dy + dz * dz > (1.7f * GB_S) * (1.7f * GB_S)) return false;
    g_gbHp -= (dmg < 1 ? 1 : dmg); g_gbHurt = 8;
    if (g_gbPhase == 1 && g_gbHp <= GB_PH2) {                         // LA VOLUNTAD: cinematica y lanzas
        g_gbSt = GBS_WILL; g_gbStT = 0; g_gbAtk = GBA_NONE; g_gbBmN = 0; g_gbSpT = 0; g_gbBlind = 0.0f;
        lvCineBoss(c[0], c[1], c[2], 1.4f, 0.5f, 0.0f, "GABRIEL", "SEÑOR, ¿ES TU VOLUNTAD QUE ACABE CON ÉL?");
    }
    if (g_gbHp <= 0) {
        g_gbHp = 0; g_gbSt = GBS_FALLTALK; g_gbStT = 0; g_gbAtk = GBA_NONE; g_gbBmN = 0; g_gbSpT = 0; g_gbBlind = 0.0f;
        lvMessage("EL ARCÁNGEL CAE DEL CIELO", 140); g_lvFlash = 30;
    }
    return true;
}
static int gb_Blocks() {
    return (g_gbSt == GBS_TALK || g_gbSt == GBS_CHOICE || g_gbSt == GBS_WILL || g_gbSt == GBS_FALLTALK ||
            g_gbSt == GBS_DIVE || g_gbSt == GBS_FALLING) ? 1 : 0;
}
static int gb_Done() { return g_gbSt == GBS_DONE ? 1 : 0; }
static float gb_Shake() {
    if (g_gbSpT >= 210 && g_gbSpT < 250) return 0.6f * (1.0f - (g_gbSpT - 210) / 40.0f);
    if (g_gbSt == GBS_FALLING) return 0.18f;
    if (g_gbSt == GBS_DIVE && g_gbStT > 60) return 0.3f;
    return 0.0f;
}
static void gb_Enter(BossIO &io, LineVertex *geo, int geoCap, int *geoN, CityBldg *boxes, int *nBoxes, int boxCap,
                     float *sx, float *sy, float *sz) {
    gbResetAll();
    g_gbBx = boxes; g_gbNbx = 0; g_gbBxCap = boxCap;
    gbBuildArena(geo, geoCap, geoN);
    *nBoxes = g_gbNbx;
    *sx = 0.0f; *sy = 0.0f; *sz = 20.0f;
    io.camYaw = 0.0f;
    gbBuildModel();
    sceKernelDcacheWritebackAll();
}
