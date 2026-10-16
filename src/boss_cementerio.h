// ============================================================================
// boss_cementerio.h -- PISO 2 "EL CEMENTERIO" (prefijo bc_ / g_bc / BC_). Contrato: boss_api.h
//
// Benjamin (2026-10-16): lapidas y angeles de piedra, arboles alrededor, dia nublado pero con el
// verde de los arboles. Un PATRON de tumbas despierta al jefe (el cuervo da los consejos). El REY
// ESQUELETO levanta esqueletos: hay que romperlos para poder herirlo. Al vencerlo: NECROMANCIA.
//
// EL ACERTIJO: los 4 angeles de piedra no rezan, SENALAN: cada uno extiende un brazo hacia una
// tumba. Con TRIANGULO se enciende la vela de la tumba que tienes delante; si no es una senalada,
// se apagan todas. Con las 4 senaladas encendidas, la tierra tiembla y el rey sale de su cripta.
// ============================================================================
#pragma once

static const int   BC_HP = 120;               // el rey (fases: 66% puas de hueso, 33% furia)
static const int   BC_NMINI = 8;
static const int   BC_MINI_HP = 6;
static const float BC_TAU = 6.2831853f, BC_PI = 3.14159265f;
static const float BC_KS = 2.3f;              // escala del rey respecto de un esqueleto comun
static const float BC_CRX = 0.0f, BC_CRZ = -30.0f;   // la cripta del rey

#define NOCTIS_TEST_BCREY 0    // PRUEBA: 1 = el rey despierta al entrar (salta el acertijo); 2 = ademas con 15 de vida. 0 al commitear
#define NOCTIS_TEST_NECRO 0    // PRUEBA: 1 = necromancia y tiempo desde el principio; 2 = ademas SIEMPRE sale Coraje. 0 al commitear

enum { BCS_EXPLORE, BCS_RISE, BCS_FIGHT, BCS_DYING, BCS_DONE };
enum { BCA_NONE, BCA_BONES, BCA_RAISE, BCA_SPIKES };

// ---------------------------------------------------------------- estado
struct BcMini { float x, z, y, yaw; int hp, alive, rise, hitCd, hurt; };
static BcMini g_bcMini[BC_NMINI];
static int   g_bcSt = BCS_EXPLORE, g_bcStT = 0, g_bcT = 0;
static int   g_bcLit = 0;                     // bit = vela encendida en la tumba k
static int   g_bcNear = -1;                   // tumba delante del jugador (-1 = ninguna)
static int   g_bcHp = BC_HP, g_bcPhase = 1, g_bcHurt = 0, g_bcBounce = 0, g_bcExposed = 0, g_bcWave = 0;
static float g_bcKX = BC_CRX, g_bcKZ = BC_CRZ, g_bcKY = -8.0f, g_bcKYaw = 0.0f;
static int   g_bcAtk = BCA_NONE, g_bcAtkT = 0, g_bcPause = 0, g_bcCold = 0, g_bcShieldMsg = 0;
static float g_bcSpX[6], g_bcSpZ[6]; static int g_bcSpT = 0;     // puas de hueso
static float g_bcShake = 0.0f;
// modelos
static LineVertex __attribute__((aligned(16))) g_bcSkelV[1200];  // esqueleto comun (~910 verts; tambien lo usa la necromancia)
static LineVertex __attribute__((aligned(16))) g_bcCrownV[120];  // corona del rey
static int g_bcSkelN = 0, g_bcCrownN = 0;
static CityBldg *g_bcBx = 0; static int g_bcNbx = 0, g_bcBxCap = 0;

// ---------------------------------------------------------------- tumbas y angeles
static const int BC_COLS = 4, BC_ROWS = 5, BC_NT = BC_COLS * BC_ROWS;
static void bcTombPos(int k, float &x, float &z) {
    static const float cx[4] = { -10.5f, -3.5f, 3.5f, 10.5f }, rz[5] = { -16.0f, -9.0f, -2.0f, 5.0f, 12.0f };
    x = cx[k % BC_COLS]; z = rz[k / BC_COLS];
}
// cada angel (en una esquina) senala una tumba: estas 4 son el patron
static const float kBcAngel[4][2] = { { -21.0f, -21.0f }, { 21.0f, -21.0f }, { -21.0f, 20.0f }, { 21.0f, 20.0f } };
static const int   kBcTarget[4] = { 4, 3, 17, 14 };   // (col,fila): (0,1) (3,0) (1,4) (2,3)
static int bcIsTarget(int k) { for (int a = 0; a < 4; ++a) if (kBcTarget[a] == k) return 1; return 0; }
static int bcTargetMask() { int m = 0; for (int a = 0; a < 4; ++a) m |= 1 << kBcTarget[a]; return m; }

// ---------------------------------------------------------------- utilidades
static unsigned int bcC(int r, int g, int b, int a) {
    r = r < 0 ? 0 : (r > 255 ? 255 : r); g = g < 0 ? 0 : (g > 255 ? 255 : g);
    b = b < 0 ? 0 : (b > 255 ? 255 : b); a = a < 0 ? 0 : (a > 255 ? 255 : a);
    return (unsigned int)((a << 24) | (b << 16) | (g << 8) | r);
}
static float bcRnd(int a, int b) { return swRand(a, b, 131u); }
static void bcBox(float x, float z, float w, float d, float h) { if (g_bcBx && g_bcNbx < g_bcBxCap) g_bcBx[g_bcNbx++] = { x, z, w, d, h, 0 }; }
static void bcGlow(float x, float y, float z, float R, unsigned int cIn, unsigned int cOut) {
    const int N = 12;
    LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 3 * N);
    for (int s = 0; s < N; ++s) {
        const float a0 = BC_TAU * s / N, a1 = BC_TAU * (s + 1) / N;
        const float c0 = cosf(a0) * R, s0 = sinf(a0) * R, c1 = cosf(a1) * R, s1 = sinf(a1) * R;
        v[s * 3 + 0] = { cIn, x, y, z };
        v[s * 3 + 1] = { cOut, x + g_lvRight[0] * c0 + g_lvUp[0] * s0, y + g_lvRight[1] * c0 + g_lvUp[1] * s0, z + g_lvRight[2] * c0 + g_lvUp[2] * s0 };
        v[s * 3 + 2] = { cOut, x + g_lvRight[0] * c1 + g_lvUp[0] * s1, y + g_lvRight[1] * c1 + g_lvUp[1] * s1, z + g_lvRight[2] * c1 + g_lvUp[2] * s1 };
    }
    sceGumLoadIdentity(); sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, 3 * N, 0, v);
}
static void bcAdd() { sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_FIX, 0, 0xFFFFFFFF); sceGuDepthMask(GU_TRUE); }
static void bcOff() { sceGuDepthMask(GU_FALSE); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0); sceGuDisable(GU_BLEND); }

// ---------------------------------------------------------------- modelos
// esqueleto de pie, pies en y=0, mira a -Z, ~3.2 de alto
static void bcBuildSkel() {
    int i = 0; LineVertex *b = g_bcSkelV;
    const unsigned int BO = bcC(226, 220, 198, 255), BO2 = bcC(196, 188, 162, 255), DK = bcC(30, 24, 22, 255);
    addBall(b, i, 0.0f, 2.86f, 0.0f, 0.27f, 0.30f, 0.30f, 3, 6, BO);                     // craneo
    addBall(b, i, -0.10f, 2.88f, -0.24f, 0.07f, 0.07f, 0.04f, 2, 4, DK);                 // cuencas
    addBall(b, i, 0.10f, 2.88f, -0.24f, 0.07f, 0.07f, 0.04f, 2, 4, DK);
    addBall(b, i, 0.0f, 2.62f, -0.12f, 0.17f, 0.08f, 0.15f, 2, 5, BO2);                  // mandibula
    addLimb(b, i, 0.0f, 1.25f, 0.06f, 0.0f, 2.56f, 0.04f, 0.06f, 0.06f, 4, BO2, BO);      // columna
    for (int r = 0; r < 3; ++r) {                                                         // costillas
        const float y = 2.30f - r * 0.22f, w = 0.36f - r * 0.05f;
        addLimb(b, i, -w, y, -0.02f, w, y, -0.02f, 0.035f, 0.035f, 3, BO, BO);
    }
    addLimb(b, i, -0.40f, 2.42f, 0.0f, 0.40f, 2.42f, 0.0f, 0.05f, 0.05f, 4, BO, BO);       // hombros
    addBall(b, i, 0.0f, 1.20f, 0.04f, 0.30f, 0.12f, 0.18f, 2, 5, BO2);                    // pelvis
    for (int s = -1; s <= 1; s += 2) {
        addLimb(b, i, s * 0.42f, 2.40f, 0.0f, s * 0.52f, 1.80f, -0.10f, 0.05f, 0.04f, 4, BO, BO2);   // brazo
        addLimb(b, i, s * 0.52f, 1.80f, -0.10f, s * 0.50f, 1.30f, -0.40f, 0.04f, 0.035f, 4, BO2, BO); // antebrazo, adelante
        addLimb(b, i, s * 0.18f, 1.15f, 0.04f, s * 0.20f, 0.60f, 0.0f, 0.06f, 0.05f, 4, BO, BO2);    // muslo
        addLimb(b, i, s * 0.20f, 0.60f, 0.0f, s * 0.20f, 0.05f, 0.04f, 0.05f, 0.04f, 4, BO2, BO);    // canilla
    }
    g_bcSkelN = i;
    i = 0; LineVertex *c = g_bcCrownV;                                                    // corona (del rey)
    for (int k = 0; k < 8 && i + 12 <= 120; ++k) {
        const float a0 = BC_TAU * k / 8, a1 = BC_TAU * (k + 1) / 8, r = 0.31f, y0 = 3.05f, y1 = 3.25f;
        const unsigned int G = bcC(214, 170, 60, 255), G2 = bcC(255, 214, 110, 255);
        swQuadC(c, i, cosf(a0) * r, y0, sinf(a0) * r, G, cosf(a1) * r, y0, sinf(a1) * r, G, cosf(a1) * r, y1, sinf(a1) * r, G2, cosf(a0) * r, y1, sinf(a0) * r, G2);
        const float am = (a0 + a1) * 0.5f;
        c[i++] = { G2, cosf(a0) * r, y1, sinf(a0) * r }; c[i++] = { G2, cosf(a1) * r, y1, sinf(a1) * r }; c[i++] = { bcC(255, 236, 160, 255), cosf(am) * r, y1 + 0.22f, sinf(am) * r };
        c[i++] = { G2, cosf(a1) * r, y1, sinf(a1) * r }; c[i++] = { G2, cosf(a0) * r, y1, sinf(a0) * r }; c[i++] = { bcC(255, 236, 160, 255), cosf(am) * r, y1 + 0.22f, sinf(am) * r };
    }
    g_bcCrownN = i;
}
// angel de piedra: pedestal, tunica, alas plegadas y un brazo que SENALA (dir en XZ, unitaria)
static void bcAngel(LineVertex *b, int &i, int cap, float x, float z, float dx, float dz) {
    if (i + 620 > cap) return;
    const unsigned int ST = bcC(178, 176, 168, 255), ST2 = bcC(150, 148, 142, 255), MS = bcC(110, 126, 96, 255);
    swBoxL(b, i, x, 0.0f, z, 2.4f, 2.4f, 1.4f, ST2);                                       // pedestal
    const float y0 = 1.4f, fx = dx, fz = dz, rx = -dz, rz = dx;                             // frente = hacia la tumba
    addLimb(b, i, x, y0, z, x, y0 + 2.0f, z, 0.85f, 0.45f, 8, MS, ST);                      // tunica (con musgo abajo)
    addLimb(b, i, x, y0 + 2.0f, z, x, y0 + 2.9f, z, 0.45f, 0.42f, 6, ST, ST);               // torso
    addBall(b, i, x + fx * 0.05f, y0 + 3.25f, z + fz * 0.05f, 0.24f, 0.28f, 0.24f, 3, 6, ST); // cabeza
    for (int s = -1; s <= 1; s += 2) {                                                      // alas plegadas atras
        const float wx = x - fx * 0.35f + rx * s * 0.25f, wz = z - fz * 0.35f + rz * s * 0.25f;
        swQuadC(b, i, wx, y0 + 2.8f, wz, ST, wx - fx * 0.4f + rx * s * 0.9f, y0 + 3.6f, wz - fz * 0.4f + rz * s * 0.9f, ST,
                      wx - fx * 0.6f + rx * s * 0.7f, y0 + 1.2f, wz - fz * 0.6f + rz * s * 0.7f, ST2, wx - fx * 0.2f, y0 + 1.0f, wz - fz * 0.2f, ST2);
    }
    // el brazo que SENALA: del hombro derecho hacia la tumba, casi horizontal
    const float sx = x + rx * 0.45f, sz = z + rz * 0.45f, sy = y0 + 2.75f;
    addLimb(b, i, sx, sy, sz, sx + fx * 1.5f, sy - 0.15f, sz + fz * 1.5f, 0.12f, 0.09f, 5, ST, ST);
    addBall(b, i, sx + fx * 1.6f, sy - 0.17f, sz + fz * 1.6f, 0.10f, 0.08f, 0.10f, 2, 5, ST);   // la mano
    // el otro brazo, recogido sobre el pecho
    addLimb(b, i, x - rx * 0.45f, sy, z - rz * 0.45f, x + fx * 0.35f, y0 + 2.4f, z + fz * 0.35f, 0.11f, 0.09f, 5, ST2, ST);
}
static void bcTree(LineVertex *b, int &i, int cap, float x, float z, int seed) {
    if (i + 340 > cap) return;
    const float h = 7.0f + bcRnd(seed, 1) * 3.0f;
    addLimb(b, i, x, 0.0f, z, x + 0.2f, h, z, 0.55f, 0.32f, 5, bcC(72, 56, 44, 255), bcC(96, 78, 60, 255));
    for (int k = 0; k < 3; ++k) {
        const float a = k * 2.1f + seed, r = 1.6f + bcRnd(seed, k + 3) * 1.0f;
        const unsigned int col = bcC(52 + (int)(bcRnd(seed, k + 7) * 30), 92 + (int)(bcRnd(seed, k + 9) * 30), 50, 255);
        addBall(b, i, x + cosf(a) * 1.4f, h + 0.6f + k * 0.8f, z + sinf(a) * 1.4f, r * 1.5f, r * 1.2f, r * 1.5f, 3, 5, col);
    }
    bcBox(x, z, 1.2f, 1.2f, h);
}
static void bcBuildArena(LineVertex *g, int cap, int *n) {
    int i = 0;
    // pasto: paneles de 8u (mas grandes los descarta el GE pegados a la camara)
    for (int gz = 0; gz < 15; ++gz) for (int gx = 0; gx < 15 && i + 6 <= cap; ++gx) {
        const float x0 = -60.0f + gx * 8.0f, z0 = -60.0f + gz * 8.0f;
        auto gc = [&](int a, int b) { const float t = bcRnd(gx + a, gz + b);
            return bcC(78 + (int)(t * 26), 108 + (int)(t * 28), 62 + (int)(t * 16), 255); };
        swQuadC(g, i, x0, 0.0f, z0, gc(0, 0), x0 + 8.0f, 0.0f, z0, gc(1, 0), x0 + 8.0f, 0.0f, z0 + 8.0f, gc(1, 1), x0, 0.0f, z0 + 8.0f, gc(0, 1));
    }
    // muro bajo de piedra (con su colision)
    for (int s = 0; s < 4; ++s) {
        for (int k = 0; k < 11; ++k) {
            const float t = -40.0f + k * 8.0f;
            const float x = (s < 2) ? t : (s == 2 ? -44.0f : 44.0f), z = (s < 2) ? (s == 0 ? -44.0f : 44.0f) : t;
            if (i + 30 <= cap) swBoxL(g, i, x, 0.0f, z, (s < 2) ? 8.2f : 1.2f, (s < 2) ? 1.2f : 8.2f, 1.5f + 0.15f * bcRnd(s, k), bcC(126, 124, 118, 255));
        }
        bcBox((s < 2) ? 0.0f : (s == 2 ? -44.0f : 44.0f), (s < 2) ? (s == 0 ? -44.0f : 44.0f) : 0.0f, (s < 2) ? 90.0f : 1.2f, (s < 2) ? 1.2f : 90.0f, 1.6f);
    }
    // tumbas: lapida + monticulo; algunas con cruz
    for (int k = 0; k < BC_NT; ++k) {
        float x, z; bcTombPos(k, x, z);
        const unsigned int S = bcC(150 + (int)(bcRnd(k, 21) * 20), 148 + (int)(bcRnd(k, 22) * 18), 140, 255);
        if (i + 100 > cap) break;
        swBoxL(g, i, x, 0.0f, z, 1.7f, 0.42f, 1.9f, S);                                      // lapida
        swBoxL(g, i, x, 0.0f, z + 1.6f, 1.5f, 2.6f, 0.28f, bcC(96, 84, 64, 255));            // tierra del monticulo
        if ((k * 7) % 3 == 0) {                                                              // cruz encima
            swBoxL(g, i, x, 1.9f, z, 0.22f, 0.22f, 0.9f, S);
            swBoxL(g, i, x, 2.45f, z, 0.8f, 0.22f, 0.2f, S);
        }
        bcBox(x, z, 1.7f, 0.5f, 1.9f);
    }
    // la cripta del rey (al fondo): escalones y una losa grande
    swBoxL(g, i, BC_CRX, 0.0f, BC_CRZ, 9.0f, 6.0f, 0.5f, bcC(120, 118, 112, 255));
    swBoxL(g, i, BC_CRX, 0.5f, BC_CRZ, 7.0f, 4.4f, 0.5f, bcC(132, 130, 124, 255));
    swBoxL(g, i, BC_CRX, 0.0f, BC_CRZ - 3.6f, 9.0f, 1.2f, 4.5f, bcC(110, 108, 104, 255));   // muro de la cripta
    bcBox(BC_CRX, BC_CRZ - 3.6f, 9.0f, 1.2f, 4.5f);
    // angeles: cada uno mira y senala su tumba
    for (int a = 0; a < 4; ++a) {
        float tx, tz; bcTombPos(kBcTarget[a], tx, tz);
        float dx = tx - kBcAngel[a][0], dz = tz - kBcAngel[a][1]; const float l = sqrtf(dx * dx + dz * dz) + 1e-4f;
        bcAngel(g, i, cap, kBcAngel[a][0], kBcAngel[a][1], dx / l, dz / l);
        bcBox(kBcAngel[a][0], kBcAngel[a][1], 2.4f, 2.4f, 1.4f);
    }
    // arboles alrededor (afuera del muro)
    for (int k = 0; k < 12; ++k) {
        const float a = BC_TAU * k / 12 + 0.2f, r = 52.0f + bcRnd(k, 31) * 5.0f;
        bcTree(g, i, cap, cosf(a) * r, sinf(a) * r, k);
    }
    bcBox(0.0f, 0.0f, 120.0f, 120.0f, 0.0f);                                                 // el suelo
    *n = i;
}

// ---------------------------------------------------------------- logica
static int bcMinisAlive() { int n = 0; for (int k = 0; k < BC_NMINI; ++k) if (g_bcMini[k].alive) ++n; return n; }
static void bcRaise(int count) {   // el rey levanta una oleada alrededor suyo
    int made = 0;
    for (int k = 0; k < BC_NMINI && made < count; ++k) if (!g_bcMini[k].alive) {
        BcMini &m = g_bcMini[k];
        const float a = BC_TAU * made / count + g_bcT * 0.01f;
        m.x = g_bcKX + cosf(a) * 6.0f; m.z = g_bcKZ + 6.0f + sinf(a) * 5.0f; m.y = -2.5f;
        m.hp = BC_MINI_HP; m.alive = 1; m.rise = 50; m.hitCd = 60; m.hurt = 0; m.yaw = 0.0f; ++made;
    }
    g_bcExposed = 0; g_bcShake = 0.3f;
}
static void bcResetAll() {
    g_bcSt = BCS_EXPLORE; g_bcStT = 0; g_bcT = 0; g_bcLit = 0; g_bcNear = -1;
    g_bcHp = BC_HP; g_bcPhase = 1; g_bcHurt = 0; g_bcBounce = 0; g_bcExposed = 0; g_bcWave = 0;
    g_bcKX = BC_CRX; g_bcKZ = BC_CRZ; g_bcKY = -8.0f; g_bcKYaw = 0.0f;
    g_bcAtk = BCA_NONE; g_bcAtkT = 0; g_bcPause = 0; g_bcCold = 0; g_bcShieldMsg = 0; g_bcSpT = 0; g_bcShake = 0.0f;
    for (int k = 0; k < BC_NMINI; ++k) g_bcMini[k].alive = 0;
}
static void bcStartAtk(BossIO &io, int a) {
    g_bcAtk = a; g_bcAtkT = 0;
    if (a == BCA_SPIKES) {   // puas de hueso: alrededor de donde estas, y una debajo
        for (int k = 0; k < 6; ++k) {
            const float an = BC_TAU * k / 5 + g_bcT * 0.1f;
            g_bcSpX[k] = io.px + (k ? cosf(an) * 3.6f : 0.0f); g_bcSpZ[k] = io.pz + (k ? sinf(an) * 3.6f : 0.0f);
        }
        g_bcSpT = 1; lvMessage("¡PÚAS DE HUESO!", 90);
    } else if (a == BCA_RAISE) {
        static const int kWave[3] = { 4, 5, 7 };
        bcRaise(kWave[g_bcPhase - 1]); ++g_bcWave;
        lvMessage("EL REY LEVANTA A SUS MUERTOS", 120);
    }
}
static void bcFightUpdate(BossIO &io) {
    if (io.aiming) { float d = atan2f(g_bcKX - io.px, -(g_bcKZ - io.pz)) - io.camYaw;
                     while (d > BC_PI) d -= BC_TAU; while (d < -BC_PI) d += BC_TAU; io.camYaw += d * 0.08f; }
    if (g_lvTimeStop > 0) return;
    { const float t = atan2f(-(io.px - g_bcKX), -(io.pz - g_bcKZ)); float d = t - g_bcKYaw;
      while (d > BC_PI) d -= BC_TAU; while (d < -BC_PI) d += BC_TAU; g_bcKYaw += d * 0.05f; }
    // el rey ronda su cripta (no se aleja: es su tumba)
    g_bcKX += (sinf(g_bcT * 0.006f) * 8.0f - g_bcKX) * 0.01f;
    g_bcKZ += (BC_CRZ + 4.0f + cosf(g_bcT * 0.005f) * 3.0f - g_bcKZ) * 0.01f;
    // los esqueletos: salen de la tierra, caminan hacia ti y golpean
    const float spd = (g_bcPhase >= 3) ? 0.085f : 0.06f;
    for (int k = 0; k < BC_NMINI; ++k) {
        BcMini &m = g_bcMini[k]; if (!m.alive) continue;
        if (m.hurt > 0) --m.hurt;
        if (m.rise > 0) { --m.rise; m.y = -2.5f * (m.rise / 50.0f); continue; }
        float dx = io.px - m.x, dz = io.pz - m.z; const float l = sqrtf(dx * dx + dz * dz) + 0.001f;
        m.yaw = atan2f(-dx, -dz);
        if (l > 1.4f) { m.x += dx / l * spd; m.z += dz / l * spd; }
        // que no se encimen
        for (int j = 0; j < BC_NMINI; ++j) if (j != k && g_bcMini[j].alive) {
            const float ex = m.x - g_bcMini[j].x, ez = m.z - g_bcMini[j].z, e2 = ex * ex + ez * ez;
            if (e2 < 1.4f && e2 > 1e-4f) { m.x += ex * 0.05f; m.z += ez * 0.05f; }
        }
        if (m.hitCd > 0) --m.hitCd;
        else if (l < 1.8f && io.py < 2.5f && !g_lvDashInv) { io.hp -= 8.0f; g_lvFlash = 8; m.hitCd = 70; }   // dash: no te alcanza
    }
    // sin siervos: queda EXPUESTO un rato y vuelve a levantarlos
    if (bcMinisAlive() == 0 && g_bcAtk != BCA_RAISE) {
        if (!g_bcExposed) { g_bcExposed = (g_bcPhase >= 3) ? 300 : 400; lvMessage("¡EL REY ESTÁ EXPUESTO! ¡DISPÁRALE!", 140); }
        else if (--g_bcExposed <= 1) { g_bcExposed = 0; bcStartAtk(io, BCA_RAISE); }
    }
    // ataques del rey
    if (g_bcAtk == BCA_NONE) {
        if (--g_bcPause <= 0) {
            const float r = bcRnd(g_bcT, 7);
            if (g_bcPhase >= 2 && r < 0.45f) bcStartAtk(io, BCA_SPIKES);
            else bcStartAtk(io, BCA_BONES);
        }
    } else {
        ++g_bcAtkT;
        if (g_bcAtk == BCA_BONES) {   // le tira huesos (abanico desde la fase 3)
            if (g_bcAtkT % 22 == 10 && g_bcAtkT < 90) {
                g_lvOrbFree = (g_bcPhase >= 3);
                for (int k = (g_bcPhase >= 3 ? -1 : 0); k <= (g_bcPhase >= 3 ? 1 : 0); ++k)
                    lvFireOrb(g_bcKX, 5.0f, g_bcKZ, io.px + k * 3.0f, io.py + 1.7f, io.pz, 0.34f, RGBA(236, 228, 200, 255));
                g_lvOrbFree = 0;
            }
            if (g_bcAtkT > 110) { g_bcAtk = BCA_NONE; g_bcPause = (g_bcPhase >= 3) ? 70 : 120; }
        } else if (g_bcAtk == BCA_SPIKES) {
            if (g_bcSpT == 0) { g_bcAtk = BCA_NONE; g_bcPause = (g_bcPhase >= 3) ? 60 : 100; }
        } else { g_bcAtk = BCA_NONE; g_bcPause = 90; }   // RAISE: ya los levanto
    }
    if (g_bcSpT > 0) {   // 60 de aviso, 25 de puas
        if (g_bcSpT == 60) {
            g_bcShake = 0.25f;
            for (int k = 0; k < 6; ++k) {
                const float dx = io.px - g_bcSpX[k], dz = io.pz - g_bcSpZ[k];
                if (!g_lvDashInv && dx * dx + dz * dz < 1.8f * 1.8f && io.py < 3.0f) { io.hp -= 14.0f; g_lvFlash = 12; break; }
            }
        }
        if (++g_bcSpT > 85) g_bcSpT = 0;
    }
}

static void bc_Update(BossIO &io) {
    ++g_bcT; ++g_bcStT;
    if (g_bcHurt > 0) --g_bcHurt;
    if (g_bcBounce > 0) --g_bcBounce;
    if (g_bcCold > 0) --g_bcCold;
    if (g_bcShieldMsg > 0) --g_bcShieldMsg;
    g_bcShake *= 0.92f;
    switch (g_bcSt) {
    case BCS_EXPLORE: {
        // la tumba que tienes delante (a menos de 2.6 de su frente)
        g_bcNear = -1;
        for (int k = 0; k < BC_NT; ++k) {
            float x, z; bcTombPos(k, x, z);
            const float dx = io.px - x, dz = io.pz - (z + 1.3f);
            if (dx * dx + dz * dz < 2.6f * 2.6f) { g_bcNear = k; break; }
        }
        if (g_bcNear >= 0 && (io.edge & PSP_CTRL_TRIANGLE)) {
            if (bcIsTarget(g_bcNear)) {
                g_bcLit |= 1 << g_bcNear; audioBell();
                if ((g_bcLit & bcTargetMask()) == bcTargetMask()) {
                    g_bcSt = BCS_RISE; g_bcStT = 0; g_bcShake = 0.6f;
                    lvMessage("LA TIERRA TIEMBLA...", 160);
                } else lvMessage("LA VELA SE ENCIENDE", 70);
            } else {
                g_bcLit = 0; g_bcCold = 40;
                lvMessage("UN FRÍO TE RECORRE. LAS VELAS SE APAGAN", 140);
            }
        }
    } break;
    case BCS_RISE:   // el rey sale de la cripta (con cinematica)
        g_bcShake = 0.35f;
        if (g_bcStT == 60) lvCineBoss(g_bcKX, 4.0f, g_bcKZ, 0.9f, 0.6f, -6.0f, "EL REY ESQUELETO", "SEÑOR DE LAS TUMBAS");
        g_bcKY += (0.6f - g_bcKY) * 0.03f;
        if (g_bcStT >= 90) { g_bcSt = BCS_FIGHT; g_bcStT = 0; g_bcKY = 0.6f; g_bcPause = 60; bcStartAtk(io, BCA_RAISE); }
        break;
    case BCS_FIGHT: bcFightUpdate(io); break;
    case BCS_DYING:
        g_bcShake = 0.2f * (1.0f - g_bcStT / 150.0f);
        if (g_bcStT == 40) { g_lvGiveNecro = 1; }
        if (g_bcStT >= 150) { g_bcSt = BCS_DONE; }
        break;
    default: break;
    }
}

// ---------------------------------------------------------------- dibujo 3D
static void bcDrawSkel(float x, float y, float z, float yaw, float s, int hurt, int crown) {
    const float bob = 0.04f * sinf(g_lvT * 0.3f + x);
    if (hurt) bcAdd();
    lvDrawAt(g_bcSkelV, g_bcSkelN, x, y + bob, z, yaw, 0.0f, s, 0.0f);
    if (crown) lvDrawAt(g_bcCrownV, g_bcCrownN, x, y + bob, z, yaw, 0.0f, s, 0.0f);
    if (hurt) bcOff();
}
static void bc_Draw(float px, float py, float pz) {
    (void)px; (void)py; (void)pz;
    sceGuDisable(GU_TEXTURE_2D);
    // velas: las encendidas tienen llama; la tumba delante tuyo, un brillo leve
    bcAdd();
    for (int k = 0; k < BC_NT; ++k) {
        float x, z; bcTombPos(k, x, z);
        if ((g_bcLit >> k) & 1) {
            const float fl = 0.5f + 0.5f * sinf(g_lvT * 0.4f + k);
            bcGlow(x, 2.25f, z + 0.25f, 0.35f + 0.08f * fl, bcC(255, 220, 140, 230), bcC(255, 140, 40, 0));
            bcGlow(x, 2.1f, z + 0.25f, 1.4f, bcC(255, 170, 70, 70), bcC(255, 120, 30, 0));
        }
        if (k == g_bcNear && g_bcSt == BCS_EXPLORE) lvRing3D(x, 0.05f, z + 1.3f, 1.2f, 0.12f, bcC(200, 220, 255, 90));
    }
    if (g_bcSpT > 0) {   // puas de hueso: aviso (grietas) y despues las puas
        for (int k = 0; k < 6; ++k) {
            if (g_bcSpT < 60) lvRing3D(g_bcSpX[k], 0.06f, g_bcSpZ[k], 1.8f * (1.2f - 0.2f * g_bcSpT / 60.0f), 0.2f, bcC(230, 220, 190, 80 + g_bcSpT * 2));
        }
    }
    if (g_bcExposed && g_bcSt == BCS_FIGHT) bcGlow(g_bcKX, 4.2f * BC_KS * 0.6f, g_bcKZ, 4.5f, bcC(255, 80, 60, 70 + (g_lvT & 8) * 6), bcC(255, 40, 30, 0));
    bcOff();
    if (g_bcSpT >= 60) {
        const float f = 1.0f - (g_bcSpT - 60) / 25.0f;
        for (int k = 0; k < 6; ++k) for (int q = 0; q < 3; ++q) {
            const float a = q * 2.1f + k, ox = cosf(a) * 0.5f, oz = sinf(a) * 0.5f;
            LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 3 * 4); int n = 0;
            const float bx = g_bcSpX[k] + ox, bz = g_bcSpZ[k] + oz, h = 2.6f * f + 0.2f;
            const unsigned int B = bcC(226, 218, 196, 255), T = bcC(250, 246, 230, 255);
            for (int e = 0; e < 4; ++e) {
                const float e0 = e * BC_PI * 0.5f, e1 = e0 + BC_PI * 0.5f;
                v[n++] = { B, bx + cosf(e0) * 0.25f, 0.0f, bz + sinf(e0) * 0.25f };
                v[n++] = { B, bx + cosf(e1) * 0.25f, 0.0f, bz + sinf(e1) * 0.25f };
                v[n++] = { T, bx, h, bz };
            }
            sceGumLoadIdentity(); sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, n, 0, v);
        }
    }
    // el rey y sus esqueletos
    if (g_bcSt == BCS_RISE || g_bcSt == BCS_FIGHT || (g_bcSt == BCS_DYING && g_bcStT < 140)) {
        float s = BC_KS, y = g_bcKY;
        if (g_bcSt == BCS_DYING) { const float t = g_bcStT / 140.0f; s = BC_KS * (1.0f - t * t); y = g_bcKY - t * 2.0f; }
        bcDrawSkel(g_bcKX, y, g_bcKZ, g_bcKYaw, s, g_bcHurt > 0 && ((g_bcHurt >> 1) & 1), 1);
        bcAdd();   // ojos rojos del rey
        bcGlow(g_bcKX, y + 2.88f * s, g_bcKZ, 0.35f * s, bcC(255, 40, 30, 230), bcC(255, 20, 10, 0));
        if (bcMinisAlive() > 0 && g_bcSt == BCS_FIGHT)   // escudo de huesos: aura palida mientras haya siervos
            bcGlow(g_bcKX, y + 1.7f * s, g_bcKZ, 2.2f * s, bcC(200, 220, 255, 40 + (int)(20.0f * sinf(g_lvT * 0.1f))), bcC(160, 190, 255, 0));
        bcOff();
    }
    for (int k = 0; k < BC_NMINI; ++k) {
        const BcMini &m = g_bcMini[k]; if (!m.alive) continue;
        bcDrawSkel(m.x, m.y, m.z, m.yaw, 1.0f, m.hurt > 0, 0);
    }
    sceGumLoadIdentity();
}

// ---------------------------------------------------------------- HUD
static void bc_Hud(BossIO &io) {
    (void)io;
    sceGuDisable(GU_DEPTH_TEST); sceGuDisable(GU_TEXTURE_2D);
    sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    if (g_bcCold > 0) bxTint(RGBA(150, 190, 255, g_bcCold * 3));                // el frio de equivocarse
    if (g_bcSt == BCS_EXPLORE && g_bcNear >= 0) {
        fontTexOn();
        drawText(240 - textLen("TRIÁNGULO: ENCENDER LA VELA") * 4, 172, 1.0f, RGBA(230, 236, 246, 255),
                 ((g_bcLit >> g_bcNear) & 1) ? "LA VELA YA ARDE" : "TRIÁNGULO: ENCENDER LA VELA");
        sceGuDisable(GU_TEXTURE_2D);
    }
    if (g_bcSt == BCS_FIGHT) {
        if (g_bcPhase >= 3) bxRage(g_bcT);
        bxBossBar("EL REY ESQUELETO", g_bcHp, BC_HP);
        const int n = bcMinisAlive();
        fontTexOn();
        if (n > 0) { char b[24] = "SIERVOS: 0"; b[9] = (char)('0' + n); drawText(240 - 5 * 8, 30, 1.0f, RGBA(220, 226, 240, 255), b); }
        else if (g_bcExposed && ((g_bcT >> 3) & 1)) drawText(240 - textLen("¡EXPUESTO!") * 4, 30, 1.0f, RGBA(255, 120, 90, 255), "¡EXPUESTO!");
        sceGuDisable(GU_TEXTURE_2D);
    }
    sceGuDisable(GU_TEXTURE_2D); sceGuEnable(GU_DEPTH_TEST);
}

// ---------------------------------------------------------------- API
static void bc_Backdrop() {   // dia nublado: gris claro arriba, mas palido en el horizonte
    gradQuad(0, 136, RGBA(142, 148, 156, 255), RGBA(196, 200, 204, 255));
    gradQuad(136, SCR_HEIGHT, RGBA(196, 200, 204, 255), RGBA(168, 174, 170, 255));
}
static unsigned int bc_Fog(float *nearF, float *farF) { *nearF = 30.0f; *farF = 130.0f; return RGBA(184, 190, 192, 255); }
static int bc_AimTarget(float out[3]) {
    if (g_bcSt != BCS_FIGHT) { if (g_bcSt != BCS_RISE) return 0; out[0] = g_bcKX; out[1] = 4.0f; out[2] = g_bcKZ; return 1; }   // muerto: nadie le sigue disparando
    // primero los siervos (el mas cercano a la mira), despues el rey
    int best = -1; float bd = 1e9f;
    for (int k = 0; k < BC_NMINI; ++k) {
        const BcMini &m = g_bcMini[k]; if (!m.alive || m.rise > 0) continue;
        const float dx = m.x - g_lvEye[0], dz = m.z - g_lvEye[2], d = dx * dx + dz * dz;
        if (d < bd) { bd = d; best = k; }
    }
    if (best >= 0) { out[0] = g_bcMini[best].x; out[1] = 1.8f; out[2] = g_bcMini[best].z; return 1; }
    out[0] = g_bcKX; out[1] = g_bcKY + 1.9f * BC_KS; out[2] = g_bcKZ; return 1;
}
static bool bc_ShotHit(float x, float y, float z, int dmg) {
    if (g_bcSt != BCS_FIGHT) return false;
    if (dmg < 1) dmg = 1;
    for (int k = 0; k < BC_NMINI; ++k) {
        BcMini &m = g_bcMini[k]; if (!m.alive || m.rise > 0) continue;
        const float dx = x - m.x, dz = z - m.z;
        if (dx * dx + dz * dz < 1.2f * 1.2f && y > -0.3f && y < 3.4f) {
            m.hp -= dmg; m.hurt = 6;
            if (m.hp <= 0) { m.alive = 0; g_lvFlash = 4; }
            return true;
        }
    }
    const float dx = x - g_bcKX, dz = z - g_bcKZ;
    if (dx * dx + dz * dz < (0.9f * BC_KS) * (0.9f * BC_KS) && y > -0.3f && y < 3.4f * BC_KS) {
        if (bcMinisAlive() > 0) {   // escudo de huesos
            g_bcBounce = 8;
            if (!g_bcShieldMsg) { lvMessage("¡ROMPE A SUS SIERVOS PRIMERO!", 90); g_bcShieldMsg = 120; }
            return true;
        }
        g_bcHp -= dmg; g_bcHurt = 6;
        {   // FASES: 66% puas de hueso, 33% furia
            const int ph = (g_bcHp * 3 > BC_HP * 2) ? 1 : (g_bcHp * 3 > BC_HP ? 2 : 3);
            if (ph > g_bcPhase && g_bcHp > 0) { g_bcPhase = ph; g_lvFlash = 16; lvMessage(ph == 2 ? "EL REY ALZA SU CETRO DE HUESO" : "EL REY ESQUELETO SE ENFURECE", 170); }
        }
        if (g_bcHp <= 0) {
            g_bcHp = 0; g_bcSt = BCS_DYING; g_bcStT = 0; g_lvFlash = 30;
            for (int k = 0; k < BC_NMINI; ++k) g_bcMini[k].alive = 0;
            lvMessage("EL REY ESQUELETO SE DESMORONA", 160);
        }
        return true;
    }
    return false;
}
static int bc_Blocks() { return (g_bcSt == BCS_RISE) ? 1 : 0; }
static int bc_Done() { return g_bcSt == BCS_DONE ? 1 : 0; }
static float bc_Shake() { return g_bcShake; }
static void bc_Enter(BossIO &io, LineVertex *geo, int geoCap, int *geoN, CityBldg *boxes, int *nBoxes, int boxCap,
                     float *sx, float *sy, float *sz) {
    bcResetAll();
    g_bcBx = boxes; g_bcNbx = 0; g_bcBxCap = boxCap;
    bcBuildArena(geo, geoCap, geoN);
    *nBoxes = g_bcNbx;
    *sx = 0.0f; *sy = 0.0f; *sz = 26.0f;   // con 34 la camara quedaba pegada al muro sur
    io.camYaw = 0.0f;
    bcBuildSkel();
#if NOCTIS_TEST_BCREY
    g_bcLit = bcTargetMask(); g_bcSt = BCS_RISE; g_bcStT = 0;
    if (NOCTIS_TEST_BCREY == 2) g_bcHp = 15;
#endif
    sceKernelDcacheWritebackAll();
}
