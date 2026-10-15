// ============================================================================
// boss_pale.h -- PISO 7: "EL JINETE PALIDO" (Muerte) + HADES.        Prefijo: bv_
//
// Contrato en boss_api.h. Escenario: ruinas quemadas bajo un cielo gris con brillo
// radioactivo y un hongo nuclear lejano. El jinete (caballo verde-amarillo enfermo,
// jinete esqueletico con guadana) CARGA (aviso en el piso; si te toca: io.hp = 0), da
// TAJOS, e invoca una HORDA de 7 MOSCAS: mientras haya una viva es INVULNERABLE.
// HADES (figura oscura con llamas verdes) lo sigue y lanza orbes verdes.
// No deja curarte (g_lvNoHeal = 1 en Enter; 0 al morir). Vida 120.
//
// Se dibuja SIN cull, color por vertice. Usa addLimb/addBall/addLoft (char_prims.h),
// swBoxL/swQuadC/swRand (skyworld.h) y los helpers lv* / bx* de levels.h / boss_api.h.
// ============================================================================
#pragma once

static const float BV_PI    = 3.14159265f;
static const int   BV_HPMAX = 120;
static const int   BV_NFLY  = 7;
static const float BV_ARENA = 41.0f;     // limite de la arena para el jefe y las moscas (radio jugable ~42)
static const float BV_WALL  = 46.0f;     // centro de los muros perimetrales
static const int   BV_CW_T  = 130;       // aviso de la carga (frames)
static const int   BV_CW_LOCK = 45;      // frames antes de salir en que se bloquea la direccion

// paleta (RGBA empaquetado 0xAABBGGRR)
static const unsigned int BV_FOG    = RGBA(88, 106, 66, 255);     // niebla gris-verdosa
static const unsigned int BV_HIDE   = RGBA(166, 176, 78, 255);    // piel verde-amarilla enferma
static const unsigned int BV_HIDED  = RGBA(104, 118, 50, 255);
static const unsigned int BV_BONE   = RGBA(228, 226, 194, 255);
static const unsigned int BV_BONED  = RGBA(168, 170, 130, 255);
static const unsigned int BV_DARK   = RGBA(30, 34, 24, 255);
static const unsigned int BV_CLOAK  = RGBA(26, 32, 26, 255);
static const unsigned int BV_CLOAKT = RGBA(54, 64, 48, 255);
static const unsigned int BV_TOXIC  = RGBA(132, 255, 64, 255);
static const unsigned int BV_STEEL  = RGBA(150, 170, 150, 255);

// ---------------------------------------------------------------- estado
enum { BV_INTRO, BV_STALK, BV_CWIND, BV_CRUN, BV_CREC, BV_SAPP, BV_SWING, BV_SUMMON, BV_DEAD };
static int   bv_t = 0, bv_state = BV_INTRO, bv_st = 0, bv_hp = BV_HPMAX, bv_hurt = 0, bv_cd = 0;
static int   bv_done = 0, bv_grace = 0, bv_hitCd = 0, bv_swingN = 0, bv_swingT = 0, bv_swingHit = 0, bv_horde = 0;
static float bv_x = 0.0f, bv_z = -26.0f, bv_yaw = 3.14159265f, bv_spd = 0.0f, bv_gp = 0.0f, bv_orb = 0.0f, bv_orbDir = 1.0f;
static float bv_dx = 0.0f, bv_dz = 1.0f;                 // direccion bloqueada de la carga
static float bv_pitch = 0.0f, bv_pitchT = 0.0f, bv_arm = 0.15f, bv_armT = 0.15f, bv_armK = 0.2f, bv_roll = 0.0f, bv_rollT = 0.0f;
static float bv_shake = 0.0f, bv_dis = 0.0f;             // temblor; disolucion 0..1 (muerte)
static float bv_hx = 0.0f, bv_hy = 3.0f, bv_hz = -30.0f, bv_hYaw = 0.0f, bv_hFlare = 0.0f;
static int   bv_hOrbCd = 240, bv_blockMsg = 0;
static float bv_lpx = 0.0f, bv_lpz = 0.0f; static int bv_lpOk = 0;   // ultima posicion del jugador (detecta respawn)

struct BvFly { float x, y, z, vx, vy, vz, ph; int hp, alive, die, delay, cd, dash; };
static BvFly bv_fly[BV_NFLY];
struct BvPart { float x, y, z, vx, vy, vz, gy, sz; int life, max; unsigned int col; };
static BvPart bv_part[72];

// modelos estaticos (se arman en Enter)
static LineVertex __attribute__((aligned(16))) bv_mHorse[3800];
static LineVertex __attribute__((aligned(16))) bv_mLeg[440];
static LineVertex __attribute__((aligned(16))) bv_mRider[2800];
static LineVertex __attribute__((aligned(16))) bv_mArm[1300];
static LineVertex __attribute__((aligned(16))) bv_mHades[2900];
static LineVertex __attribute__((aligned(16))) bv_mFly[820];
static LineVertex __attribute__((aligned(16))) bv_mAura[900];
static int bv_nHorse = 0, bv_nLeg = 0, bv_nRider = 0, bv_nArm = 0, bv_nHades = 0, bv_nFly = 0, bv_nAura = 0;

static const int kBvHordeHp[4] = { 120, 88, 58, 28 };   // la horda llega al empezar y al bajar de estos HP

// ---------------------------------------------------------------- utilidades
static float bvClamp(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
static float bvR(int a, int b) { return swRand(a, b, 77u); }
static float bvYawTo(float dx, float dz) { return atan2f(-dx, -dz); }      // el modelo mira a -Z
static float bvTurn(float cur, float tgt, float rate) {
    float d = tgt - cur;
    while (d > BV_PI) d -= 2.0f * BV_PI;
    while (d < -BV_PI) d += 2.0f * BV_PI;
    if (d > rate) d = rate;
    if (d < -rate) d = -rate;
    return cur + d;
}
static void bvT(float x, float y, float z) { ScePspFVector3 v = { x, y, z }; sceGumTranslate(&v); }
static void bvS(float x, float y, float z) { ScePspFVector3 v = { x, y, z }; sceGumScale(&v); }
// local del jinete (mira a -Z) -> mundo (solo giro en Y)
static void bvW(float lx, float ly, float lz, float &wx, float &wy, float &wz) {
    const float s = sinf(bv_yaw), c = cosf(bv_yaw);
    wx = bv_x + lx * c + lz * s; wz = bv_z - lx * s + lz * c; wy = ly;
}
static int bvFlyAlive() {
    int n = 0;
    for (int k = 0; k < BV_NFLY; ++k) if (bv_fly[k].alive && !bv_fly[k].die) ++n;
    return n;
}

// ---------------------------------------------------------------- particulas
static void bvEmit(float x, float y, float z, float spread, float up, float gy, int life, unsigned int col, float sz) {
    for (int k = 0; k < 72; ++k) if (bv_part[k].life <= 0) {
        BvPart &p = bv_part[k];
        p.x = x; p.y = y; p.z = z;
        p.vx = (bvR(bv_t * 7 + k, 1) - 0.5f) * spread; p.vz = (bvR(bv_t * 7 + k, 2) - 0.5f) * spread;
        p.vy = up * (0.4f + bvR(bv_t * 7 + k, 3));
        p.gy = gy; p.sz = sz; p.life = p.max = life; p.col = col;
        return;
    }
}
static void bvPartUpdate() {
    for (int k = 0; k < 72; ++k) {
        BvPart &p = bv_part[k]; if (p.life <= 0) continue;
        p.x += p.vx; p.y += p.vy; p.z += p.vz; p.vy += p.gy; p.vx *= 0.985f; p.vz *= 0.985f;
        if (p.y < 0.05f && p.gy < 0.0f) { p.y = 0.05f; p.vy = 0.0f; }
        --p.life;
    }
}

// ---------------------------------------------------------------- constructores de modelos
static void bvLimb(LineVertex *b, int &i, int cap, float x0, float y0, float z0, float x1, float y1, float z1,
                   float r0, float r1, int sides, unsigned int c0, unsigned int c1) {
    if (i + sides * 12 > cap) return;
    addLimb(b, i, x0, y0, z0, x1, y1, z1, r0, r1, sides, c0, c1);
}
static void bvBall(LineVertex *b, int &i, int cap, float cx, float cy, float cz, float rx, float ry, float rz,
                   int st, int sl, unsigned int c) {
    if (i + st * sl * 6 > cap) return;
    addBall(b, i, cx, cy, cz, rx, ry, rz, st, sl, c);
}

// CABALLO (pies en y=0, mira a -Z). Sin patas: se dibujan aparte, articuladas.
static int bvBuildHorse(LineVertex *b, int cap) {
    int i = 0;
    // tronco: pecho + barril flaco + grupa
    bvBall(b, i, cap, 0.0f, 3.35f, -1.45f, 0.95f, 1.20f, 1.15f, 6, 10, BV_HIDE);
    bvBall(b, i, cap, 0.0f, 3.25f, 1.75f, 0.88f, 1.10f, 1.05f, 6, 10, BV_HIDED);
    bvLimb(b, i, cap, 0.0f, 3.10f, -1.0f, 0.0f, 3.05f, 1.3f, 0.80f, 0.78f, 8, BV_HIDED, BV_HIDED);
    // costillas a la vista (5 por lado, curvas en 2 tramos)
    for (int s = -1; s <= 1; s += 2) {
        for (int k = 0; k < 5; ++k) {
            const float z = -0.55f + k * 0.42f, sf = (float)s;
            bvLimb(b, i, cap, sf * 0.50f, 3.80f, z, sf * 0.90f, 3.20f, z + 0.06f, 0.065f, 0.060f, 4, BV_BONE, BV_BONE);
            bvLimb(b, i, cap, sf * 0.90f, 3.20f, z + 0.06f, sf * 0.72f, 2.58f, z + 0.14f, 0.060f, 0.04f, 4, BV_BONE, BV_BONED);
        }
    }
    // columna vertebral y huesos de la cadera
    for (int k = 0; k < 4; ++k) bvBall(b, i, cap, 0.0f, 3.95f, -0.7f + k * 0.55f, 0.13f, 0.13f, 0.17f, 3, 4, BV_BONE);
    bvBall(b, i, cap, -0.78f, 4.30f, 1.55f, 0.20f, 0.20f, 0.24f, 3, 5, BV_BONE);
    bvBall(b, i, cap, 0.78f, 4.30f, 1.55f, 0.20f, 0.20f, 0.24f, 3, 5, BV_BONE);
    // cuello, cabeza, hocico, mandibula, orejas
    bvLimb(b, i, cap, 0.0f, 4.10f, -2.25f, 0.0f, 4.95f, -3.05f, 0.62f, 0.46f, 7, BV_HIDE, BV_HIDE);
    bvLimb(b, i, cap, 0.0f, 4.95f, -3.05f, 0.0f, 5.50f, -3.65f, 0.46f, 0.38f, 7, BV_HIDE, BV_HIDE);
    bvBall(b, i, cap, 0.0f, 5.65f, -4.0f, 0.42f, 0.50f, 0.72f, 5, 8, BV_HIDE);
    bvLimb(b, i, cap, 0.0f, 5.55f, -4.4f, 0.0f, 5.10f, -5.15f, 0.30f, 0.22f, 6, BV_HIDE, BV_HIDED);
    bvLimb(b, i, cap, 0.0f, 5.30f, -4.2f, 0.0f, 4.92f, -5.0f, 0.18f, 0.14f, 6, BV_BONED, BV_BONED);
    bvLimb(b, i, cap, -0.2f, 6.05f, -3.7f, -0.28f, 6.60f, -3.6f, 0.12f, 0.02f, 4, BV_HIDED, BV_DARK);
    bvLimb(b, i, cap, 0.2f, 6.05f, -3.7f, 0.28f, 6.60f, -3.6f, 0.12f, 0.02f, 4, BV_HIDED, BV_DARK);
    bvBall(b, i, cap, -0.36f, 5.80f, -3.95f, 0.10f, 0.10f, 0.10f, 3, 5, BV_TOXIC);    // ojos de brasa verde
    bvBall(b, i, cap, 0.36f, 5.80f, -3.95f, 0.10f, 0.10f, 0.10f, 3, 5, BV_TOXIC);
    // crin hecha jirones
    for (int k = 0; k < 5; ++k) {
        const float t = k * 0.2f;
        bvLimb(b, i, cap, 0.0f, 5.45f - t * 1.3f, -3.4f + t * 1.2f, 0.12f * (k & 1 ? 1 : -1), 4.55f - t * 1.2f, -2.5f + t * 1.1f,
               0.12f, 0.01f, 4, BV_DARK, BV_CLOAKT);
    }
    // cola raida
    bvLimb(b, i, cap, 0.0f, 3.90f, 2.7f, 0.0f, 3.20f, 3.7f, 0.18f, 0.12f, 4, BV_DARK, BV_DARK);
    bvLimb(b, i, cap, 0.0f, 3.20f, 3.7f, 0.0f, 2.00f, 4.1f, 0.12f, 0.08f, 4, BV_DARK, BV_CLOAKT);
    bvLimb(b, i, cap, 0.0f, 2.00f, 4.1f, 0.1f, 0.90f, 3.9f, 0.08f, 0.01f, 4, BV_CLOAKT, BV_DARK);
    return i;
}
// una pata colgando desde la cadera (origen); delgada, con rodilla y casco
static int bvBuildLeg(LineVertex *b, int cap) {
    int i = 0;
    bvLimb(b, i, cap, 0.0f, 0.0f, 0.0f, 0.0f, -1.25f, 0.12f, 0.30f, 0.20f, 5, BV_HIDE, BV_HIDED);
    bvBall(b, i, cap, 0.0f, -1.25f, 0.12f, 0.22f, 0.22f, 0.22f, 3, 5, BV_BONED);
    bvLimb(b, i, cap, 0.0f, -1.25f, 0.12f, 0.0f, -2.50f, 0.0f, 0.17f, 0.12f, 5, BV_HIDED, BV_BONE);
    bvLimb(b, i, cap, 0.0f, -2.48f, 0.0f, 0.0f, -2.62f, -0.04f, 0.20f, 0.23f, 5, BV_DARK, BV_DARK);
    return i;
}
// JINETE sentado en la silla (coordenadas del caballo)
static int bvBuildRider(LineVertex *b, int cap) {
    int i = 0;
    // capa: torso en loft, abierta sobre la grupa
    const float ys[4] = { 3.95f, 4.60f, 5.50f, 6.30f }, rs[4] = { 1.05f, 0.78f, 0.60f, 0.52f };
    if (i + 3 * 96 <= cap) addLoft(b, i, 0.0f, 0.25f, ys, rs, 4, 8, BV_CLOAK, BV_CLOAKT);
    for (int k = 0; k < 6; ++k) {                       // jirones de la capa hacia atras
        const float x = -0.5f + k * 0.2f;
        bvLimb(b, i, cap, x, 5.8f, 0.55f, x * 2.0f, 3.2f + (k & 1) * 0.5f, 2.5f + (k % 3) * 0.4f, 0.17f, 0.03f, 4, BV_CLOAKT, BV_DARK);
    }
    bvBall(b, i, cap, -0.58f, 6.15f, 0.2f, 0.30f, 0.22f, 0.30f, 4, 6, BV_CLOAKT);    // hombros
    bvBall(b, i, cap, 0.58f, 6.15f, 0.2f, 0.30f, 0.22f, 0.30f, 4, 6, BV_CLOAKT);
    // craneo, mandibula, cuencas con brasas, nariz
    bvBall(b, i, cap, 0.0f, 6.95f, -0.10f, 0.40f, 0.48f, 0.44f, 5, 8, BV_BONE);
    bvBall(b, i, cap, 0.0f, 6.50f, -0.20f, 0.26f, 0.18f, 0.30f, 3, 6, BV_BONED);
    bvBall(b, i, cap, -0.17f, 7.00f, -0.46f, 0.12f, 0.13f, 0.08f, 3, 5, RGBA(6, 8, 6, 255));
    bvBall(b, i, cap, 0.17f, 7.00f, -0.46f, 0.12f, 0.13f, 0.08f, 3, 5, RGBA(6, 8, 6, 255));
    bvBall(b, i, cap, -0.17f, 7.00f, -0.52f, 0.055f, 0.055f, 0.04f, 3, 4, BV_TOXIC);
    bvBall(b, i, cap, 0.17f, 7.00f, -0.52f, 0.055f, 0.055f, 0.04f, 3, 4, BV_TOXIC);
    bvBall(b, i, cap, 0.0f, 6.82f, -0.52f, 0.06f, 0.07f, 0.04f, 3, 4, RGBA(10, 12, 8, 255));
    // capucha (deja ver la cara) y punta que ondea
    bvBall(b, i, cap, 0.0f, 7.00f, 0.22f, 0.62f, 0.76f, 0.62f, 6, 8, BV_CLOAK);
    bvBall(b, i, cap, 0.0f, 7.45f, -0.08f, 0.50f, 0.26f, 0.50f, 3, 8, BV_CLOAK);
    bvLimb(b, i, cap, 0.0f, 7.60f, 0.5f, 0.0f, 7.90f, 1.6f, 0.40f, 0.02f, 5, BV_CLOAK, BV_CLOAKT);
    // brazo izquierdo con las riendas
    bvLimb(b, i, cap, -0.60f, 6.10f, 0.10f, -0.95f, 5.35f, -0.35f, 0.17f, 0.13f, 5, BV_CLOAKT, BV_CLOAK);
    bvLimb(b, i, cap, -0.95f, 5.35f, -0.35f, -0.55f, 4.75f, -1.15f, 0.11f, 0.09f, 5, BV_BONED, BV_BONE);
    bvBall(b, i, cap, -0.55f, 4.75f, -1.15f, 0.13f, 0.13f, 0.13f, 3, 5, BV_BONE);
    bvLimb(b, i, cap, -0.55f, 4.75f, -1.15f, 0.0f, 5.10f, -4.9f, 0.03f, 0.03f, 3, BV_DARK, BV_DARK);   // riendas
    // piernas a horcajadas
    for (int s = -1; s <= 1; s += 2) {
        const float sf = (float)s;
        bvLimb(b, i, cap, sf * 0.50f, 4.10f, 0.30f, sf * 1.00f, 3.70f, -0.50f, 0.30f, 0.22f, 5, BV_CLOAK, BV_CLOAK);
        bvLimb(b, i, cap, sf * 1.00f, 3.70f, -0.50f, sf * 1.05f, 2.60f, -0.45f, 0.20f, 0.10f, 5, BV_BONED, BV_DARK);
    }
    return i;
}
// brazo de la guadana: el origen es el HOMBRO derecho (se gira para tajar)
static int bvBuildArm(LineVertex *b, int cap) {
    int i = 0;
    bvLimb(b, i, cap, 0.0f, 0.0f, 0.0f, 0.35f, -0.85f, -0.30f, 0.17f, 0.13f, 5, BV_CLOAKT, BV_CLOAK);
    bvLimb(b, i, cap, 0.35f, -0.85f, -0.30f, 0.15f, -1.40f, -1.00f, 0.12f, 0.09f, 5, BV_BONED, BV_BONE);
    bvBall(b, i, cap, 0.15f, -1.40f, -1.00f, 0.13f, 0.13f, 0.13f, 3, 5, BV_BONE);
    // asta (pasa por la mano) y cuchilla curva que sale hacia adelante/adentro
    const float bx = 0.15f, by = -2.8f, bz = -0.8f, tx = 0.15f, ty = 3.0f, tz = -1.5f;
    bvLimb(b, i, cap, bx, by, bz, tx, ty, tz, 0.075f, 0.06f, 6, RGBA(70, 52, 36, 255), RGBA(96, 74, 50, 255));
    bvBall(b, i, cap, bx, by, bz, 0.12f, 0.12f, 0.12f, 3, 5, BV_DARK);
    bvBall(b, i, cap, tx, ty, tz, 0.16f, 0.16f, 0.16f, 3, 5, BV_STEEL);
    const float cx[6] = { 0.0f, -0.35f, -1.00f, -1.80f, -2.70f, -3.40f };
    const float cy[6] = { 0.0f, 0.05f, 0.10f, 0.05f, -0.10f, -0.38f };
    const float cz[6] = { 0.0f, -0.60f, -1.10f, -1.45f, -1.55f, -1.30f };
    for (int k = 0; k < 5; ++k) {                       // lomo de la cuchilla
        const float r0 = 0.16f * (1.0f - k / 5.0f) + 0.03f, r1 = 0.16f * (1.0f - (k + 1) / 5.0f) + 0.02f;
        bvLimb(b, i, cap, tx + cx[k], ty + cy[k], tz + cz[k], tx + cx[k + 1], ty + cy[k + 1], tz + cz[k + 1], r0, r1, 4,
               RGBA(96, 118, 100, 255), RGBA(140, 170, 140, 255));
    }
    for (int k = 0; k < 5; ++k) {                       // filo: tira plana que cuelga del lomo
        if (i + 6 > cap) break;
        const float w0 = 0.62f * (1.0f - k / 5.0f), w1 = 0.62f * (1.0f - (k + 1) / 5.0f);
        const unsigned int ce = RGBA(214, 244, 204, 255), cs = RGBA(120, 150, 120, 255), ct = RGBA(150, 255, 110, 255);
        swQuadC(b, i, tx + cx[k], ty + cy[k], tz + cz[k], cs,
                      tx + cx[k + 1], ty + cy[k + 1], tz + cz[k + 1], cs,
                      tx + cx[k + 1], ty + cy[k + 1] - w1, tz + cz[k + 1], k == 4 ? ct : ce,
                      tx + cx[k], ty + cy[k] - w0, tz + cz[k], ce);
    }
    return i;
}
// HADES (origen en la base de la tunica; flota). Las llamas verdes se dibujan aparte.
static int bvBuildHades(LineVertex *b, int cap) {
    int i = 0;
    const float ys[5] = { 0.0f, 0.9f, 2.0f, 3.2f, 4.1f }, rs[5] = { 0.15f, 0.70f, 1.05f, 0.95f, 0.55f };
    if (i + 4 * 96 <= cap) addLoft(b, i, 0.0f, 0.0f, ys, rs, 5, 8, RGBA(8, 6, 16, 255), RGBA(40, 32, 62, 255));
    bvBall(b, i, cap, 0.0f, 3.10f, -0.85f, 0.32f, 0.38f, 0.20f, 4, 6, RGBA(90, 240, 70, 255));      // grieta verde en el pecho
    for (int s = -1; s <= 1; s += 2) {
        const float sf = (float)s;
        bvBall(b, i, cap, sf * 0.85f, 3.90f, 0.0f, 0.45f, 0.30f, 0.45f, 4, 6, RGBA(24, 20, 40, 255));   // hombreras
        bvLimb(b, i, cap, sf * 0.95f, 4.10f, 0.0f, sf * 1.50f, 5.20f, -0.10f, 0.20f, 0.02f, 4, RGBA(30, 40, 30, 255), RGBA(80, 120, 60, 255));
        bvLimb(b, i, cap, sf * 0.85f, 3.90f, 0.0f, sf * 1.50f, 3.20f, -0.70f, 0.28f, 0.22f, 5, RGBA(30, 26, 50, 255), RGBA(18, 14, 30, 255));
        bvLimb(b, i, cap, sf * 1.50f, 3.20f, -0.70f, sf * 1.55f, 3.00f, -1.50f, 0.20f, 0.12f, 5, RGBA(18, 14, 30, 255), RGBA(110, 120, 100, 255));
        bvBall(b, i, cap, sf * 1.55f, 3.00f, -1.50f, 0.16f, 0.16f, 0.16f, 3, 5, RGBA(150, 160, 130, 255));
        bvLimb(b, i, cap, sf * 1.50f, 3.00f, -0.70f, sf * 1.70f, 1.60f, -0.50f, 0.22f, 0.02f, 4, RGBA(20, 16, 34, 255), RGBA(8, 6, 16, 255));
        bvLimb(b, i, cap, sf * 0.30f, 5.40f, -0.10f, sf * 0.70f, 6.20f, -0.05f, 0.12f, 0.08f, 4, RGBA(70, 66, 76, 255), RGBA(70, 66, 76, 255));   // cuernos
        bvLimb(b, i, cap, sf * 0.70f, 6.20f, -0.05f, sf * 0.55f, 6.90f, 0.10f, 0.08f, 0.01f, 4, RGBA(70, 66, 76, 255), RGBA(150, 150, 130, 255));
    }
    bvBall(b, i, cap, 0.0f, 4.90f, -0.05f, 0.55f, 0.68f, 0.58f, 6, 8, RGBA(14, 12, 22, 255));      // capucha
    bvBall(b, i, cap, 0.0f, 4.85f, -0.32f, 0.38f, 0.45f, 0.30f, 4, 6, RGBA(3, 3, 7, 255));          // rostro vacio
    bvBall(b, i, cap, -0.20f, 4.95f, -0.58f, 0.12f, 0.07f, 0.04f, 3, 5, RGBA(196, 255, 100, 255));  // ojos
    bvBall(b, i, cap, 0.20f, 4.95f, -0.58f, 0.12f, 0.07f, 0.04f, 3, 5, RGBA(196, 255, 100, 255));
    return i;
}
// MOSCA (cuerpo; las alas se dibujan aparte, aleteando)
static int bvBuildFly(LineVertex *b, int cap) {
    int i = 0;
    bvBall(b, i, cap, 0.0f, 0.0f, 0.1f, 0.22f, 0.20f, 0.34f, 4, 6, RGBA(36, 52, 30, 255));          // torax
    bvBall(b, i, cap, 0.0f, -0.02f, 0.46f, 0.20f, 0.18f, 0.32f, 4, 6, RGBA(80, 130, 40, 255));       // abdomen verde brillante
    bvBall(b, i, cap, 0.0f, 0.02f, -0.26f, 0.17f, 0.16f, 0.15f, 3, 5, RGBA(26, 34, 22, 255));        // cabeza
    bvBall(b, i, cap, -0.12f, 0.05f, -0.34f, 0.09f, 0.10f, 0.08f, 3, 4, RGBA(255, 60, 40, 255));     // ojos rojos
    bvBall(b, i, cap, 0.12f, 0.05f, -0.34f, 0.09f, 0.10f, 0.08f, 3, 4, RGBA(255, 60, 40, 255));
    for (int s = -1; s <= 1; s += 2) for (int k = 0; k < 3; ++k)
        bvLimb(b, i, cap, s * 0.12f, -0.10f, -0.05f + k * 0.16f, s * 0.34f, -0.34f, -0.10f + k * 0.2f, 0.025f, 0.012f, 3, BV_DARK, BV_DARK);
    return i;
}
// escudo de moscas: dos elipsoides verdes translucidos (caballo y jinete)
static int bvBuildAura(LineVertex *b, int cap) {
    int i = 0;
    bvBall(b, i, cap, 0.0f, 3.4f, 0.1f, 1.7f, 1.9f, 3.4f, 5, 10, BV_TOXIC);
    bvBall(b, i, cap, 0.0f, 6.4f, 0.2f, 1.5f, 2.0f, 1.5f, 5, 8, BV_TOXIC);
    for (int k = 0; k < i; ++k) b[k].color = (b[k].color & 0x00FFFFFFu) | (62u << 24);
    return i;
}

// ---------------------------------------------------------------- arena (geometria + colision)
static LineVertex *bvgB = 0; static int bvgN = 0, bvgCap = 0;
static CityBldg *bvbB = 0; static int *bvbN = 0; static int bvbCap = 0;
static void bvBox(float cx, float y0, float cz, float w, float d, float h, unsigned int col, int solid) {
    if (bvgN + 30 <= bvgCap) swBoxL(bvgB, bvgN, cx, y0, cz, w, d, h, col);
    if (solid && *bvbN < bvbCap) {
        CityBldg &c = bvbB[*bvbN];
        c.x = cx; c.z = cz; c.w = w; c.d = d; c.h = y0 + h; c.color = col; ++(*bvbN);
    }
}
static void bvDisc(float cx, float cz, float r, float y, unsigned int cIn, unsigned int cOut, int seg) {
    if (bvgN + seg * 3 > bvgCap) return;
    for (int s = 0; s < seg; ++s) {
        const float a0 = 2.0f * BV_PI * s / seg, a1 = 2.0f * BV_PI * (s + 1) / seg;
        bvgB[bvgN++] = { cIn, cx, y, cz };
        bvgB[bvgN++] = { cOut, cx + sinf(a0) * r, y, cz + cosf(a0) * r };
        bvgB[bvgN++] = { cOut, cx + sinf(a1) * r, y, cz + cosf(a1) * r };
    }
}
static const float kBvFire[6][2] = { { -32.0f, -26.0f }, { 32.0f, -26.0f }, { -38.0f, 12.0f }, { 38.0f, 12.0f }, { -40.0f, 36.0f }, { 40.0f, 36.0f } };

static void bvBuildArena() {
    // suelo visual enorme (se funde con la niebla) + colision de la arena (radio jugable ~42)
    if (bvgN + 6 <= bvgCap)
        swQuadC(bvgB, bvgN, -400.0f, -0.06f, -400.0f, RGBA(44, 46, 36, 255), -400.0f, -0.06f, 400.0f, RGBA(44, 46, 36, 255),
                400.0f, -0.06f, 400.0f, RGBA(44, 46, 36, 255), 400.0f, -0.06f, -400.0f, RGBA(44, 46, 36, 255));
    if (*bvbN < bvbCap) { bvbB[*bvbN].x = 0.0f; bvbB[*bvbN].z = 0.0f; bvbB[*bvbN].w = 104.0f; bvbB[*bvbN].d = 104.0f; bvbB[*bvbN].h = 0.0f; bvbB[*bvbN].color = RGBA(60, 60, 50, 255); ++(*bvbN); }
    // losas agrietadas del patio (4x4 de 21u, con juntas)
    for (int a = 0; a < 4; ++a) for (int c = 0; c < 4; ++c) {
        const float x = -31.5f + a * 21.0f, z = -31.5f + c * 21.0f, v = bvR(a * 4 + c, 5);
        const unsigned int col = RGBA(74 + (int)(26 * v), 74 + (int)(24 * v), 62 + (int)(18 * v), 255);
        bvBox(x, -0.7f, z, 21.0f, 21.0f, 0.7f, col, 0);
    }
    if (bvgN + 36 <= bvgCap) {                                                     // juntas oscuras entre las losas
        const unsigned int cj = RGBA(18, 18, 16, 255);
        for (int q = -1; q <= 1; ++q) {
            const float u = q * 21.0f;
            swQuadC(bvgB, bvgN, -42.0f, 0.10f, u - 0.3f, cj, -42.0f, 0.10f, u + 0.3f, cj, 42.0f, 0.10f, u + 0.3f, cj, 42.0f, 0.10f, u - 0.3f, cj);
            swQuadC(bvgB, bvgN, u - 0.3f, 0.10f, -42.0f, cj, u + 0.3f, 0.10f, -42.0f, cj, u + 0.3f, 0.10f, 42.0f, cj, u - 0.3f, 0.10f, 42.0f, cj);
        }
    }
    // manchas de quemadura y charcos radioactivos
    for (int k = 0; k < 7; ++k) bvDisc(-34.0f + bvR(k, 11) * 68.0f, -34.0f + bvR(k, 12) * 60.0f, 5.0f + bvR(k, 13) * 7.0f, 0.14f, RGBA(16, 16, 14, 255), RGBA(40, 40, 34, 255), 9);
    for (int k = 0; k < 5; ++k) {
        const float x = -32.0f + bvR(k, 21) * 64.0f, z = -36.0f + bvR(k, 22) * 44.0f;
        bvDisc(x, z, 3.2f + bvR(k, 23) * 3.6f, 0.18f, RGBA(176, 255, 80, 255), RGBA(40, 120, 30, 255), 9);
    }
    // muro perimetral en ruinas (alto >= 13: no se salta) con coronas rotas; cara interior a +-44.5
    for (int side = 0; side < 4; ++side) {
        for (int k = 0; k < 6; ++k) {
            const float t = -40.0f + k * 16.0f, h = 13.0f + bvR(side * 6 + k, 31) * 8.0f, v = bvR(side * 6 + k, 32);
            const unsigned int col = RGBA(52 + (int)(18 * v), 52 + (int)(16 * v), 46 + (int)(12 * v), 255);
            float x, z, w, d;
            if (side < 2) { x = t; z = side == 0 ? -BV_WALL : BV_WALL; w = 16.0f; d = 3.0f; }
            else          { x = side == 2 ? -BV_WALL : BV_WALL; z = t; w = 3.0f; d = 16.0f; }
            bvBox(x, 0.0f, z, w, d, h, col, 1);
            const float ox = (side < 2) ? (bvR(side * 6 + k, 33) - 0.5f) * 6.0f : 0.0f, oz = (side < 2) ? 0.0f : (bvR(side * 6 + k, 33) - 0.5f) * 6.0f;
            bvBox(x + ox, h, z + oz, w * 0.55f, d * 0.8f, 2.0f + bvR(side * 6 + k, 34) * 5.0f, brighten(col, 0.85f), 0);
        }
    }
    for (int cx = -1; cx <= 1; cx += 2) for (int cz = -1; cz <= 1; cz += 2) {     // torres de las esquinas
        bvBox(cx * BV_WALL, 0.0f, cz * BV_WALL, 7.0f, 7.0f, 28.0f, RGBA(46, 46, 40, 255), 1);
        bvBox(cx * BV_WALL, 28.0f, cz * BV_WALL, 4.6f, 4.6f, 5.0f, RGBA(40, 42, 36, 255), 0);
    }
    // columnas rotas (techo 4.6..6.0: se puede subir de un salto) y columnas caidas (saltables)
    static const float kCol[8][2] = { { -22.0f, -12.0f }, { 22.0f, -12.0f }, { -28.0f, 8.0f }, { 28.0f, 8.0f },
                                      { -12.0f, -32.0f }, { 12.0f, -32.0f }, { -14.0f, 20.0f }, { 14.0f, 20.0f } };
    for (int k = 0; k < 8; ++k) {
        const float h = 4.6f + bvR(k, 41) * 1.4f;
        bvBox(kCol[k][0], 0.0f, kCol[k][1], 5.0f, 5.0f, 1.0f, RGBA(88, 86, 74, 255), 1);
        bvBox(kCol[k][0], 1.0f, kCol[k][1], 3.3f, 3.3f, h - 1.0f, RGBA(104, 102, 88, 255), 1);
        bvBox(kCol[k][0] + 0.8f, h, kCol[k][1] - 0.4f, 1.9f, 1.6f, 1.4f, RGBA(96, 94, 80, 255), 0);
    }
    bvBox(-15.0f, 0.0f, -2.0f, 14.0f, 3.2f, 2.4f, RGBA(100, 98, 84, 255), 1);
    bvBox(16.0f, 0.0f, -20.0f, 12.0f, 3.2f, 2.4f, RGBA(100, 98, 84, 255), 1);
    bvBox(-6.0f, 0.0f, 14.0f, 12.0f, 3.2f, 2.4f, RGBA(100, 98, 84, 255), 1);
    // escombros sueltos (sin colision), barriles radioactivos junto a los fuegos
    for (int k = 0; k < 28; ++k) {
        const float x = -42.0f + bvR(k, 51) * 84.0f, z = -42.0f + bvR(k, 52) * 84.0f;
        if (z > 24.0f && x > -12.0f && x < 12.0f) continue;                        // la entrada queda libre
        const float s = 0.9f + bvR(k, 53) * 1.8f, v = bvR(k, 54);
        bvBox(x, 0.0f, z, s * 1.6f, s * 1.2f, s * 0.55f, RGBA(66 + (int)(30 * v), 64 + (int)(28 * v), 54 + (int)(22 * v), 255), 0);
    }
    for (int k = 0; k < 6; ++k) {
        bvBox(kBvFire[k][0], 0.0f, kBvFire[k][1], 1.8f, 1.8f, 2.0f, RGBA(96, 112, 40, 255), 0);
        bvBox(kBvFire[k][0], 0.95f, kBvFire[k][1], 1.95f, 1.95f, 0.35f, RGBA(220, 210, 50, 255), 0);
        bvBox(kBvFire[k][0] + 2.0f, 0.0f, kBvFire[k][1] + 1.1f, 1.4f, 1.4f, 1.6f, RGBA(80, 94, 34, 255), 0);
    }
    // horizonte: torres calcinadas en la niebla (r 100..170), con una ventana verde encendida
    for (int k = 0; k < 14; ++k) {
        const float a = 2.0f * BV_PI * k / 14.0f + (bvR(k, 61) - 0.5f) * 0.3f, r = 100.0f + bvR(k, 62) * 70.0f;
        const float x = sinf(a) * r, z = cosf(a) * r, w = 12.0f + bvR(k, 63) * 14.0f, d = 12.0f + bvR(k, 64) * 14.0f, h = 36.0f + bvR(k, 65) * 70.0f;
        const float v = bvR(k, 66);
        const unsigned int col = RGBA(34 + (int)(14 * v), 40 + (int)(14 * v), 34 + (int)(10 * v), 255);
        bvBox(x, 0.0f, z, w, d, h, col, 0);
        bvBox(x + w * 0.2f, h, z - d * 0.1f, w * 0.6f, d * 0.6f, 6.0f + bvR(k, 67) * 13.0f, brighten(col, 0.8f), 0);
        bvBox(x - w * 0.3f, 0.0f, z + d * 0.3f, 3.6f, 3.6f, h + 14.0f + bvR(k, 68) * 16.0f, brighten(col, 0.9f), 0);
        const float nx = -sinf(a), nz = -cosf(a);                                  // cara que mira a la arena
        const float wy = 9.0f + bvR(k, 69) * (h - 14.0f);
        bvBox(x + nx * (w * 0.5f + 0.1f), wy, z + nz * (d * 0.5f + 0.1f), 2.4f, 2.4f, 3.2f, RGBA(150, 255, 70, 255), 0);
    }
}

// ---------------------------------------------------------------- moscas, muerte del jefe, respawn
static void bvKillFly(BvFly &F) {
    F.die = 12; bv_blockMsg = 0;
    for (int q = 0; q < 5; ++q) bvEmit(F.x, F.y, F.z, 0.4f, 0.15f, -0.004f, 26, RGBA(150, 255, 90, 255), 0.18f);
}
static void bvSummon() {
    bv_horde++; bv_blockMsg = 0;
    for (int k = 0; k < BV_NFLY; ++k) {
        BvFly &F = bv_fly[k];
        F.alive = 1; F.hp = 3; F.die = 0; F.delay = 30 + k * 8; F.cd = 0; F.dash = 0;
        F.ph = k * 0.9f + bvR(k, 91) * 2.0f; F.vx = F.vy = F.vz = 0.0f; F.x = bv_hx; F.y = bv_hy + 3.4f; F.z = bv_hz;
    }
    lvMessage("¡LA HORDA! DERRIBA LAS 7 MOSCAS", 170);
}
static void bvKillBoss() {
    bv_state = BV_DEAD; bv_st = 0; bv_dis = 0.0f; bv_spd = 0.0f;
    g_lvNoHeal = 0;
    g_lvFlash = 30; bv_shake = 0.6f;
    for (int k = 0; k < BV_NFLY; ++k) if (bv_fly[k].alive && !bv_fly[k].die) { bv_fly[k].alive = 0; }
    lvMessage("HADES: ¡¡¡NOOOO!!! ¡MI SEÑOR!", 150);
}
// el jugador reaparecio (murio o cayo): el jinete se retira al otro lado y el jugador tiene un respiro
static void bvOnRespawn() {
    if (bv_state == BV_DEAD) return;
    bv_grace = 150;
    if (bv_state != BV_INTRO && bv_state != BV_SUMMON) {
        bv_state = BV_STALK; bv_st = 0; bv_cd = 150; bv_swingN = 0;
        bv_x = 0.0f; bv_z = -26.0f; bv_yaw = BV_PI; bv_spd = 0.0f; bv_pitchT = 0.0f;
    }
}
static void bvHurtPlayer(BossIO &io, float amount) {
    if (bv_grace > 0 || bv_hitCd > 0) return;
    io.hp -= amount; g_lvFlash = 8; bv_hitCd = 24; bv_shake = 0.3f;
    if (io.hp <= 0.0f) bvOnRespawn();
}

// ---------------------------------------------------------------- Enter
static void bv_Enter(BossIO &io, LineVertex *geo, int geoCap, int *geoN,
                     CityBldg *boxes, int *nBoxes, int boxCap, float *sx, float *sy, float *sz) {
    (void)io;
    bvgB = geo; bvgN = 0; bvgCap = geoCap; bvbB = boxes; bvbN = nBoxes; bvbCap = boxCap; *nBoxes = 0;
    bvBuildArena();
    *geoN = bvgN;
    bv_nHorse = bvBuildHorse(bv_mHorse, 3800); bv_nLeg = bvBuildLeg(bv_mLeg, 440);
    bv_nRider = bvBuildRider(bv_mRider, 2800);  bv_nArm = bvBuildArm(bv_mArm, 1300);
    bv_nHades = bvBuildHades(bv_mHades, 2900);  bv_nFly = bvBuildFly(bv_mFly, 820);
    bv_nAura = bvBuildAura(bv_mAura, 900);
    sceKernelDcacheWritebackAll();
    // estado
    bv_t = 0; bv_state = BV_INTRO; bv_st = 0; bv_hp = BV_HPMAX; bv_hurt = 0; bv_cd = 0; bv_done = 0; bv_grace = 0; bv_hitCd = 0;
    bv_x = 0.0f; bv_z = -26.0f; bv_yaw = BV_PI; bv_spd = 0.0f; bv_gp = 0.0f; bv_orb = 3.14159f; bv_orbDir = 1.0f;
    bv_dx = 0.0f; bv_dz = 1.0f; bv_pitch = bv_pitchT = 0.0f; bv_arm = bv_armT = 0.15f; bv_armK = 0.2f; bv_roll = bv_rollT = 0.0f;
    bv_swingN = bv_swingT = bv_swingHit = 0; bv_horde = 0; bv_shake = 0.0f; bv_dis = 0.0f;
    bv_hx = -6.0f; bv_hy = 3.0f; bv_hz = -30.0f; bv_hYaw = 0.0f; bv_hFlare = 0.0f; bv_hOrbCd = 300; bv_blockMsg = 0; bv_lpOk = 0;
    for (int k = 0; k < BV_NFLY; ++k) { bv_fly[k].alive = 0; bv_fly[k].die = 0; bv_fly[k].delay = 0; }
    for (int k = 0; k < 72; ++k) bv_part[k].life = 0;
    g_lvNoHeal = 1;                                           // el rosario no cura en este piso
    lvMessage("EL CUARTO JINETE: MUERTE. HADES LO SIGUE", 200);
    *sx = 0.0f; *sy = 0.0f; *sz = 32.0f;
}

// ---------------------------------------------------------------- Update
static void bvUpdateFlies(BossIO &io) {
    const float tpx = io.px, tpy = io.py + 1.9f, tpz = io.pz;
    for (int k = 0; k < BV_NFLY; ++k) {
        BvFly &F = bv_fly[k];
        if (!F.alive) continue;
        if (F.die > 0) {                                       // cae girando y se deshace en ceniza
            F.y -= 0.08f; F.vy -= 0.01f; F.y += F.vy;
            if (--F.die == 0) {
                F.alive = 0;
                for (int q = 0; q < 6; ++q) bvEmit(F.x, F.y + 0.3f, F.z, 0.5f, 0.12f, -0.003f, 30, RGBA(130, 230, 80, 255), 0.2f);
            }
            continue;
        }
        if (F.delay > 0) {                                     // todavia dentro de Hades
            if (--F.delay == 0) {
                F.x = bv_hx; F.y = bv_hy + 3.4f; F.z = bv_hz;
                F.vx = (bvR(k, 92) - 0.5f) * 0.5f; F.vy = 0.15f; F.vz = (bvR(k, 93) - 0.5f) * 0.5f;
                F.dash = 0; F.cd = 60 + k * 20;
                for (int q = 0; q < 4; ++q) bvEmit(F.x, F.y, F.z, 0.5f, 0.2f, -0.004f, 22, RGBA(150, 255, 90, 255), 0.22f);
            }
            continue;
        }
        // zumbido: gira alrededor del jugador y de vez en cuando se lanza en picada
        const float sw = 2.8f + 1.4f * sinf(bv_t * 0.04f + F.ph);
        float ox = sinf(bv_t * 0.21f + F.ph * 3.1f) * sw, oy = sinf(bv_t * 0.33f + F.ph * 1.7f) * 0.9f, oz = cosf(bv_t * 0.19f + F.ph * 2.3f) * sw;
        float spd = 0.18f + 0.03f * sinf(bv_t * 0.1f + F.ph);
        if (F.dash > 0) { --F.dash; ox = oy = oz = 0.0f; spd = 0.34f; }
        else if (--F.cd <= 0) { F.dash = 42; F.cd = 130 + (int)(bvR(bv_t, k + 100) * 170.0f); }
        float dx = tpx + ox - F.x, dy = tpy + oy - F.y, dz = tpz + oz - F.z;
        const float d = sqrtf(dx * dx + dy * dy + dz * dz) + 0.001f;
        F.vx += (dx / d * spd - F.vx) * 0.12f + (bvR(bv_t, k * 3 + 1) - 0.5f) * 0.07f;
        F.vy += (dy / d * spd - F.vy) * 0.12f + (bvR(bv_t, k * 3 + 2) - 0.5f) * 0.05f;
        F.vz += (dz / d * spd - F.vz) * 0.12f + (bvR(bv_t, k * 3 + 3) - 0.5f) * 0.07f;
        for (int j = 0; j < BV_NFLY; ++j) {                    // no se amontonan
            if (j == k) continue;
            const BvFly &G = bv_fly[j]; if (!G.alive || G.delay > 0 || G.die) continue;
            const float ex = F.x - G.x, ey = F.y - G.y, ez = F.z - G.z, e2 = ex * ex + ey * ey + ez * ez;
            if (e2 < 1.5f && e2 > 0.0001f) { const float e = 0.02f / sqrtf(e2); F.vx += ex * e; F.vy += ey * e; F.vz += ez * e; }
        }
        F.x += F.vx; F.y += F.vy; F.z += F.vz;
        if (F.x > BV_ARENA + 0.5f) { F.x = BV_ARENA + 0.5f; F.vx = -F.vx; } if (F.x < -BV_ARENA - 0.5f) { F.x = -BV_ARENA - 0.5f; F.vx = -F.vx; }
        if (F.z > BV_ARENA + 0.5f) { F.z = BV_ARENA + 0.5f; F.vz = -F.vz; } if (F.z < -BV_ARENA - 0.5f) { F.z = -BV_ARENA - 0.5f; F.vz = -F.vz; }
        if (F.y < 0.7f) { F.y = 0.7f; F.vy = fabsf(F.vy); } if (F.y > 16.0f) { F.y = 16.0f; F.vy = -fabsf(F.vy); }
        // contacto: pica y se aparta
        const float cx = F.x - tpx, cy = F.y - (io.py + 1.7f), cz = F.z - tpz;
        if (cx * cx + cy * cy + cz * cz < 1.5f * 1.5f && bv_grace == 0 && bv_hitCd == 0) {
            bvHurtPlayer(io, 5.0f);
            F.vx = -F.vx * 0.6f; F.vy = 0.12f; F.vz = -F.vz * 0.6f; F.dash = 0; F.cd = 70;
            bvEmit(F.x, F.y, F.z, 0.5f, 0.1f, -0.003f, 18, RGBA(255, 90, 60, 255), 0.16f);
        }
    }
}

static void bv_Update(BossIO &io) {
    if (g_lvTimeStop > 0) return;                              // poder de Baphomet: todo congelado
    ++bv_t;
    if (bv_hurt > 0) --bv_hurt;
    if (bv_grace > 0) --bv_grace;
    if (bv_hitCd > 0) --bv_hitCd;
    if (bv_lpOk) { const float ex = io.px - bv_lpx, ez = io.pz - bv_lpz; if (ex * ex + ez * ez > 36.0f) bvOnRespawn(); }
    bv_lpx = io.px; bv_lpz = io.pz; bv_lpOk = 1;
    bvPartUpdate();
    bv_shake *= 0.90f;
    bv_hFlare *= 0.94f;

    const float pdx = io.px - bv_x, pdz = io.pz - bv_z, pd = sqrtf(pdx * pdx + pdz * pdz) + 0.001f;
    const float fx = -sinf(bv_yaw), fz = -cosf(bv_yaw);        // hacia donde mira el jinete

    // ---- muerte: Hades grita, ambos se hacen ceniza verde ----
    if (bv_state == BV_DEAD) {
        ++bv_st;
        bv_armT = -0.3f; bv_pitchT = -0.28f; bv_armK = 0.1f;
        if (bv_st < 100) {
            bv_shake = bvClamp(0.35f, 0.0f, 0.6f);
            bv_hFlare = 1.5f;
            if (bv_st % 6 == 0) { float wx, wy, wz; bvW(0.0f, 4.0f + bvR(bv_st, 1) * 3.5f, 0.0f, wx, wy, wz); bvEmit(wx, wy, wz, 1.2f, 0.12f, 0.0f, 34, RGBA(150, 255, 90, 255), 0.22f); }
        } else {
            if (bv_st == 100) { lvMessage("MUERTE Y HADES SE DESHACEN EN CENIZA VERDE", 130); g_lvFlash = 24; }
            bv_dis = bvClamp((bv_st - 100) / 90.0f, 0.0f, 1.0f);
            bv_shake = 0.16f;
            for (int q = 0; q < 3; ++q) {
                float wx, wy, wz; bvW((bvR(bv_st, q + 5) - 0.5f) * 2.4f, (1.0f - bv_dis) * (1.0f + 6.5f * bvR(bv_st, q + 9)), (bvR(bv_st, q + 13) - 0.5f) * 5.0f, wx, wy, wz);
                bvEmit(wx, wy, wz, 0.5f, 0.10f, 0.0035f, 46, (q & 1) ? RGBA(120, 235, 70, 255) : RGBA(96, 104, 84, 255), 0.26f);
                bvEmit(bv_hx + (bvR(bv_st, q + 17) - 0.5f) * 2.0f, bv_hy + (1.0f - bv_dis) * 5.0f * bvR(bv_st, q + 21), bv_hz + (bvR(bv_st, q + 25) - 0.5f) * 2.0f,
                       0.4f, 0.10f, 0.0035f, 46, (q & 1) ? RGBA(120, 235, 70, 255) : RGBA(70, 62, 90, 255), 0.26f);
            }
            if (bv_st >= 200) bv_done = 1;
        }
        bv_arm += (bv_armT - bv_arm) * bv_armK; bv_pitch += (bv_pitchT - bv_pitch) * 0.08f;
        bv_hYaw = bvYawTo(io.px - bv_hx, io.pz - bv_hz);
        bv_hx += (bvR(bv_t, 31) - 0.5f) * 0.25f; bv_hz += (bvR(bv_t, 32) - 0.5f) * 0.25f;   // Hades se retuerce
        return;
    }

    bvUpdateFlies(io);
    const int nf = bvFlyAlive();

    // ---- maquina de estados del jinete ----
    bv_armT = 0.15f; bv_armK = 0.2f; bv_rollT = 0.0f; bv_pitchT = 0.0f;
    switch (bv_state) {
    case BV_INTRO: {
        ++bv_st;
        bv_yaw = bvTurn(bv_yaw, bvYawTo(pdx, pdz), 0.04f);
        bv_spd *= 0.9f;
        if (bv_st >= 190) { bv_state = BV_STALK; bv_st = 0; bv_cd = 20; }
        break;
    }
    case BV_STALK: {
        if (bv_cd > 0) --bv_cd;
        bv_orb += bv_orbDir * 0.008f;
        const float R = 26.0f, tx = sinf(bv_orb) * R, tz = cosf(bv_orb) * R;
        const float ddx = tx - bv_x, ddz = tz - bv_z, d = sqrtf(ddx * ddx + ddz * ddz);
        const float sp = bvClamp(d * 0.12f, 0.0f, 0.60f);
        if (sp > 0.06f) bv_yaw = bvTurn(bv_yaw, bvYawTo(ddx, ddz), 0.07f);
        else bv_yaw = bvTurn(bv_yaw, bvYawTo(pdx, pdz), 0.05f);
        bv_spd += (sp - bv_spd) * 0.15f;
        bv_x += fx * bv_spd; bv_z += fz * bv_spd;
        if (bv_cd <= 0) {
            const int r = (int)(bvR(bv_t, 3) * 100.0f);
            if (nf == 0 && bv_horde < 4 && bv_hp <= kBvHordeHp[bv_horde]) { bv_state = BV_SUMMON; bv_st = 0; bvSummon(); }
            else if (pd < 10.0f ? r < 55 : r < 30) { bv_state = BV_SAPP; bv_st = 0; }
            else { bv_state = BV_CWIND; bv_st = 0; bv_orbDir = (bvR(bv_t, 4) < 0.5f) ? -1.0f : 1.0f; }
        }
        break;
    }
    case BV_CWIND: {                                           // aviso: sigue tu posicion y se clava en el piso
        const int T = BV_CW_T;
        if (bv_st == 0) lvMessage("¡ESQUIVA!", 90);
        ++bv_st;
        bv_spd *= 0.85f;
        if (bv_st < T - BV_CW_LOCK) {
            bv_yaw = bvTurn(bv_yaw, bvYawTo(pdx, pdz), 0.10f);
            bv_dx = pdx / pd; bv_dz = pdz / pd;
        } else {
            bv_yaw = bvYawTo(bv_dx, bv_dz);                    // direccion BLOQUEADA
        }
        bv_armT = 0.95f; bv_armK = 0.15f;
        bv_pitchT = (bv_st > 10 && bv_st < T - 8) ? 0.42f : 0.0f;
        if (bv_st >= T) { bv_state = BV_CRUN; bv_st = 0; bv_spd = 0.1f; }
        break;
    }
    case BV_CRUN: {                                            // CARGA: si te toca, mueres
        ++bv_st;
        bv_armT = -1.25f; bv_armK = 0.3f;
        bv_spd += (1.30f - bv_spd) * 0.08f;
        bv_yaw = bvYawTo(bv_dx, bv_dz);
        bv_x += bv_dx * bv_spd; bv_z += bv_dz * bv_spd;
        if (bv_t % 2 == 0) bvEmit(bv_x - bv_dx * 1.5f + (bvR(bv_t, 41) - 0.5f) * 2.0f, 0.3f, bv_z - bv_dz * 1.5f, 0.3f, 0.07f, 0.0f, 26, RGBA(110, 150, 70, 255), 0.35f);
        if (bv_grace == 0) {
            static const float kAt[4] = { 3.8f, 1.6f, -0.6f, -2.4f };
            for (int q = 0; q < 4; ++q) {
                const float hx = bv_x + bv_dx * kAt[q], hz = bv_z + bv_dz * kAt[q];
                const float ex = io.px - hx, ez = io.pz - hz;
                if (ex * ex + ez * ez < 1.9f * 1.9f && io.py < 4.6f) {
                    io.hp = 0.0f; g_lvFlash = 28; bv_shake = 0.6f;
                    for (int s = 0; s < 6; ++s) bvEmit(io.px, io.py + 1.5f, io.pz, 1.0f, 0.2f, -0.004f, 24, RGBA(255, 80, 50, 255), 0.25f);
                    bvOnRespawn();
                    break;
                }
            }
        }
        if (bv_state == BV_CRUN && (bv_x > BV_ARENA || bv_x < -BV_ARENA || bv_z > BV_ARENA || bv_z < -BV_ARENA || bv_st > 140)) {
            bv_x = bvClamp(bv_x, -BV_ARENA, BV_ARENA); bv_z = bvClamp(bv_z, -BV_ARENA, BV_ARENA);
            bv_state = BV_CREC; bv_st = 0; bv_shake = 0.25f;
            for (int s = 0; s < 5; ++s) bvEmit(bv_x, 0.4f, bv_z, 1.0f, 0.1f, -0.002f, 28, RGBA(110, 130, 80, 255), 0.5f);
        }
        break;
    }
    case BV_CREC: {
        ++bv_st;
        bv_spd *= 0.88f; bv_x += bv_dx * bv_spd; bv_z += bv_dz * bv_spd;
        bv_x = bvClamp(bv_x, -BV_ARENA, BV_ARENA); bv_z = bvClamp(bv_z, -BV_ARENA, BV_ARENA);
        bv_pitchT = 0.0f;
        if (bv_st >= 55) { bv_state = BV_STALK; bv_st = 0; bv_cd = 70 + (int)(bv_hp * 0.5f); }
        break;
    }
    case BV_SAPP: {                                            // se acerca a tajar
        ++bv_st;
        bv_yaw = bvTurn(bv_yaw, bvYawTo(pdx, pdz), 0.12f);
        bv_spd += (0.70f - bv_spd) * 0.15f;
        bv_x += fx * bv_spd; bv_z += fz * bv_spd;
        bv_x = bvClamp(bv_x, -BV_ARENA, BV_ARENA); bv_z = bvClamp(bv_z, -BV_ARENA, BV_ARENA);
        bv_armT = 0.5f;
        if (pd < 6.8f) { bv_state = BV_SWING; bv_st = 0; bv_swingN = (bv_hp > 60) ? 2 : 3; bv_swingT = 0; bv_swingHit = 0; lvMessage("¡ALÉJATE!", 70); }
        else if (bv_st > 240) { bv_state = BV_STALK; bv_st = 0; bv_cd = 40; }
        break;
    }
    case BV_SWING: {                                           // tajos: aviso con abanico, golpe, recuperacion
        ++bv_swingT;
        bv_spd *= 0.8f;
        if (bv_swingT < 22) bv_yaw = bvTurn(bv_yaw, bvYawTo(pdx, pdz), 0.13f);
        if (bv_swingT < 26) { bv_armT = 1.2f; bv_rollT = -0.45f; bv_armK = 0.2f; }
        else if (bv_swingT <= 32) { bv_armT = -1.45f; bv_rollT = -0.45f; bv_armK = 0.55f; }
        else { bv_armT = 0.15f; bv_armK = 0.2f; }
        if (bv_swingT == 26) bv_spd = 0.35f;                   // pequena embestida
        bv_x += fx * bv_spd; bv_z += fz * bv_spd;
        bv_x = bvClamp(bv_x, -BV_ARENA, BV_ARENA); bv_z = bvClamp(bv_z, -BV_ARENA, BV_ARENA);
        if (bv_swingT == 29 && !bv_swingHit) {
            bv_swingHit = 1; bv_shake = 0.25f;
            const float c = (pdx * fx + pdz * fz) / pd;
            if (pd < 8.6f && c > 0.18f && io.py < 7.0f) {
                bvHurtPlayer(io, io.hpMax * 0.35f > 30.0f ? io.hpMax * 0.35f : 30.0f);
                for (int s = 0; s < 4; ++s) bvEmit(io.px, io.py + 1.7f, io.pz, 0.8f, 0.15f, -0.004f, 20, RGBA(230, 255, 190, 255), 0.2f);
            }
        }
        if (bv_swingT >= 44) {
            if (--bv_swingN > 0) { bv_swingT = 0; bv_swingHit = 0; }
            else { bv_state = BV_STALK; bv_st = 0; bv_cd = 95; }
        }
        break;
    }
    case BV_SUMMON: {                                          // alza la guadana y Hades invoca la horda
        ++bv_st;
        bv_spd *= 0.85f;
        bv_yaw = bvTurn(bv_yaw, bvYawTo(pdx, pdz), 0.06f);
        bv_pitchT = (bv_st > 8 && bv_st < 80) ? 0.5f : 0.0f;
        bv_armT = 0.0f; bv_armK = 0.1f;
        bv_hFlare = 1.2f;
        if (bv_st == 38) { g_lvFlash = 8; bv_shake = 0.3f; }
        if (bv_st >= 100) { bv_state = BV_STALK; bv_st = 0; bv_cd = 110; }
        break;
    }
    default: break;
    }
    bv_gp += 0.16f + bv_spd * 0.55f;
    bv_arm += (bv_armT - bv_arm) * bv_armK;
    bv_roll += (bv_rollT - bv_roll) * 0.25f;
    bv_pitch += (bv_pitchT - bv_pitch) * 0.12f;
    if (bv_state == BV_CRUN && bv_spd > 0.3f && pd < 60.0f) bv_shake = bv_shake > 0.10f ? bv_shake : 0.10f;

    // ---- Hades: flota detras-izquierda del jinete, te mira y lanza orbes verdes ----
    {
        const float s = sinf(bv_yaw), c = cosf(bv_yaw);
        float wx = bv_x + (-6.0f) * c + 7.0f * s, wz = bv_z - (-6.0f) * s + 7.0f * c;
        wx = bvClamp(wx, -BV_ARENA, BV_ARENA); wz = bvClamp(wz, -BV_ARENA, BV_ARENA);
        float hy = 2.6f + 0.5f * sinf(bv_t * 0.05f);
        if (bv_state == BV_SUMMON) hy += 1.8f * sinf(BV_PI * bvClamp(bv_st / 100.0f, 0.0f, 1.0f));
        bv_hx += (wx - bv_hx) * 0.05f; bv_hz += (wz - bv_hz) * 0.05f; bv_hy += (hy - bv_hy) * 0.06f;
        bv_hYaw = bvYawTo(io.px - bv_hx, io.pz - bv_hz);
        if (bv_state != BV_INTRO) {
            if (--bv_hOrbCd <= 0) {
                lvFireOrb(bv_hx, bv_hy + 3.4f, bv_hz, io.px, io.py + 1.7f, io.pz, 0.24f, RGBA(110, 255, 70, 255));
                bv_hOrbCd = (bv_hp > 60 ? 120 : 85) + (int)(bvR(bv_t, 6) * 70.0f);
            }
            if (bv_hOrbCd < 24) { const float f = (24 - bv_hOrbCd) / 24.0f; if (f > bv_hFlare) bv_hFlare = f; }
        }
    }
}

// ---------------------------------------------------------------- Draw
static void bvXf(float lift) {                                 // base del jinete: posicion, giro, galope y encabritada
    sceGumLoadIdentity();
    bvT(bv_x, lift, bv_z);
    sceGumRotateY(bv_yaw);
    if (bv_dis > 0.0f) bvS(1.0f - 0.3f * bv_dis, 1.0f - bv_dis, 1.0f - 0.3f * bv_dis);
    else if (bv_hurt > 0) bvS(1.03f, 1.03f, 1.03f);
    if (bv_pitch != 0.0f) { bvT(0.0f, 0.0f, 2.0f); sceGumRotateX(bv_pitch); bvT(0.0f, 0.0f, -2.0f); }
}
static void bvFlame(LineVertex *v, int &i, float x, float y, float z, float h, float r, float ph, unsigned int cBase, unsigned int cTip) {
    const float fl = 0.78f + 0.28f * sinf(bv_t * 0.37f + ph * 5.0f);
    const float tx = x + sinf(bv_t * 0.23f + ph) * r * 0.7f, tz = z + cosf(bv_t * 0.19f + ph * 2.0f) * r * 0.7f, ty = y + h * fl;
    for (int s = 0; s < 5; ++s) {
        const float a0 = 2.0f * BV_PI * s / 5 + ph, a1 = 2.0f * BV_PI * (s + 1) / 5 + ph;
        v[i++] = { cBase, x + cosf(a0) * r, y, z + sinf(a0) * r };
        v[i++] = { cBase, x + cosf(a1) * r, y, z + sinf(a1) * r };
        v[i++] = { cTip, tx, ty, tz };
    }
}
static void bvAddQuadA(LineVertex *v, int &i, float x0, float y0, float z0, float x1, float y1, float z1,
                       float x2, float y2, float z2, float x3, float y3, float z3, unsigned int c) {
    swQuadC(v, i, x0, y0, z0, c, x1, y1, z1, c, x2, y2, z2, c, x3, y3, z3, c);
}

static void bv_Draw(float px, float py, float pz) {
    sceGuDisable(GU_CULL_FACE);
    sceGuDisable(GU_FOG);
    const int alive = (bv_state != BV_DEAD) || (bv_dis < 1.0f);

    // ---- jinete + caballo ----
    if (alive) {
        const float gallop = bvClamp(bv_spd * 1.5f, 0.0f, 1.0f);
        const float bob = sinf(bv_gp * 2.0f) * 0.14f * bvClamp(bv_spd * 2.5f, 0.0f, 1.0f);
        const float amp = 0.12f + 0.75f * gallop;
        const int rear = (bv_pitch > 0.15f);
        bvXf(bob);  sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, bv_nHorse, 0, bv_mHorse);
        bvXf(bob);  sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, bv_nRider, 0, bv_mRider);
        static const float kLeg[4][3] = { { -0.55f, -1.5f, 0.0f }, { 0.55f, -1.5f, 3.14159f }, { -0.55f, 1.9f, 3.14159f }, { 0.55f, 1.9f, 0.0f } };
        for (int k = 0; k < 4; ++k) {
            float th = amp * sinf(bv_gp + kLeg[k][2]);
            if (rear && k < 2) th = 1.0f + 0.25f * sinf(bv_t * 0.4f + k);          // patas delanteras recogidas
            if (bv_state == BV_CWIND && k == 0 && !rear) th = 0.6f + 0.5f * sinf(bv_t * 0.5f);   // escarba el suelo
            bvXf(bob); bvT(kLeg[k][0], 2.62f, kLeg[k][1]); sceGumRotateX(th);
            sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, bv_nLeg, 0, bv_mLeg);
        }
        bvXf(bob); bvT(0.6f, 6.1f, 0.1f);
        if (bv_roll != 0.0f) sceGumRotateZ(bv_roll);
        sceGumRotateX(bv_arm + 0.05f * sinf(bv_gp * 2.0f));
        sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, bv_nArm, 0, bv_mArm);
    }
    // ---- Hades ----
    if (alive) {
        sceGumLoadIdentity();
        bvT(bv_hx, bv_hy, bv_hz);
        sceGumRotateY(bv_hYaw);
        if (bv_dis > 0.0f) bvS(1.0f - 0.3f * bv_dis, 1.0f - bv_dis, 1.0f - 0.3f * bv_dis);
        else if (bv_state == BV_DEAD) { const float g = 1.0f + 0.15f * sinf(bv_t * 0.9f); bvS(g, g, g); }
        sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, bv_nHades, 0, bv_mHades);
    }
    // ---- moscas ----
    for (int k = 0; k < BV_NFLY; ++k) {
        const BvFly &F = bv_fly[k];
        if (!F.alive || F.delay > 0) continue;
        float yaw = bvYawTo(F.vx, F.vz); const float jx = (bvR(bv_t, k + 40) - 0.5f) * 0.1f, jy = (bvR(bv_t, k + 60) - 0.5f) * 0.1f;
        sceGumLoadIdentity();
        bvT(F.x + jx, F.y + jy, F.z);
        sceGumRotateY(yaw);
        if (F.die > 0) { sceGumRotateZ(F.die * 0.7f); sceGumRotateX(0.8f); }
        sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, bv_nFly, 0, bv_mFly);
    }

    // ---- translucidos: escudo, alas, llamas, avisos, particulas, ceniza ----
    sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0); sceGuDepthMask(GU_TRUE);

    if (alive && bvFlyAlive() > 0 && bv_state != BV_INTRO) {                      // aura de invulnerabilidad
        const float pul = 1.0f + 0.05f * sinf(bv_t * 0.2f);
        bvXf(0.0f); bvS(pul, pul, pul); sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, bv_nAura, 0, bv_mAura);
        bvXf(0.0f); bvS(pul * 1.14f, pul * 1.14f, pul * 1.14f); sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, bv_nAura, 0, bv_mAura);
    }
    {   // alas de las moscas (aletean tan rapido que son un borron translucido)
        LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 12 * BV_NFLY); int n = 0;
        for (int k = 0; k < BV_NFLY; ++k) {
            const BvFly &F = bv_fly[k]; if (!F.alive || F.delay > 0 || F.die > 0) continue;
            const float yaw = bvYawTo(F.vx, F.vz), s = sinf(yaw), c = cosf(yaw);
            const float fl = sinf(bv_t * 1.9f + k) * 0.7f + 0.2f;
            for (int sd = -1; sd <= 1; sd += 2) {
                const float sf = (float)sd, ex = sf * 0.95f * cosf(fl), ey = 0.55f * sinf(fl) + 0.1f;
                // local (x, y, z) -> mundo con yaw
                #define BVWX(lx, lz) (F.x + (lx) * c + (lz) * s)
                #define BVWZ(lx, lz) (F.z - (lx) * s + (lz) * c)
                swQuadC(v, n, BVWX(sf * 0.1f, -0.05f), F.y + 0.1f, BVWZ(sf * 0.1f, -0.05f), RGBA(200, 240, 200, 120),
                              BVWX(ex, -0.25f), F.y + ey, BVWZ(ex, -0.25f), RGBA(200, 240, 200, 20),
                              BVWX(ex, 0.55f), F.y + ey, BVWZ(ex, 0.55f), RGBA(200, 240, 200, 20),
                              BVWX(sf * 0.1f, 0.35f), F.y + 0.1f, BVWZ(sf * 0.1f, 0.35f), RGBA(200, 240, 200, 120));
                #undef BVWX
                #undef BVWZ
            }
        }
        sceGumLoadIdentity(); if (n > 0) sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, n, 0, v);
    }
    {   // llamas verdes: Hades (manos, corona, hombros, dobladillo) y los fuegos de las ruinas
        LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 15 * 56); int n = 0;
        const unsigned int cB = RGBA(190, 255, 110, 230), cT = RGBA(20, 150, 40, 0), cB2 = RGBA(120, 255, 70, 220);
        if (alive) {
            const float s = sinf(bv_hYaw), c = cosf(bv_hYaw);
            const float sc = (bv_dis > 0.0f) ? (1.0f - bv_dis * 0.7f) : 1.0f, fl = 1.0f + bv_hFlare * 0.9f;
            #define HX(lx, lz) (bv_hx + (lx) * c + (lz) * s)
            #define HZ(lx, lz) (bv_hz - (lx) * s + (lz) * c)
            for (int sd = -1; sd <= 1; sd += 2) {
                const float sf = (float)sd;
                bvFlame(v, n, HX(sf * 1.55f, -1.5f), bv_hy + 3.0f, HZ(sf * 1.55f, -1.5f), 1.5f * fl * sc, 0.38f * fl, sf, cB, cT);
                bvFlame(v, n, HX(sf * 0.9f, 0.0f), bv_hy + 4.2f * sc, HZ(sf * 0.9f, 0.0f), 1.0f * sc, 0.3f, sf * 2.0f, cB2, cT);
            }
            for (int k = 0; k < 6; ++k) {
                const float a = 2.0f * BV_PI * k / 6.0f;
                bvFlame(v, n, HX(sinf(a) * 0.5f, cosf(a) * 0.4f), bv_hy + 5.5f * sc, HZ(sinf(a) * 0.5f, cosf(a) * 0.4f), (0.9f + 0.4f * (k & 1)) * fl * sc, 0.2f, (float)k, cB2, cT);
            }
            for (int k = 0; k < 10; ++k) {
                const float a = 2.0f * BV_PI * k / 10.0f;
                bvFlame(v, n, HX(sinf(a) * 0.85f, cosf(a) * 0.85f), bv_hy + 0.5f * sc, HZ(sinf(a) * 0.85f, cosf(a) * 0.85f), 1.9f * sc, 0.42f, (float)k * 0.7f, cB, cT);
            }
            #undef HX
            #undef HZ
        }
        for (int k = 0; k < 6; ++k) {                                              // braseros verdes
            bvFlame(v, n, kBvFire[k][0], 1.7f, kBvFire[k][1], 2.4f, 0.55f, k * 1.3f, cB, cT);
            bvFlame(v, n, kBvFire[k][0] + 0.3f, 1.7f, kBvFire[k][1] - 0.2f, 1.5f, 0.35f, k * 1.3f + 2.0f, cB2, cT);
        }
        sceGumLoadIdentity(); if (n > 0) sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, n, 0, v);
    }
    // aviso de la CARGA: carril en el piso (naranja mientras te sigue, rojo cuando se bloquea)
    if (bv_state == BV_CWIND) {
        const int locked = (bv_st >= BV_CW_T - BV_CW_LOCK);
        const float a = atan2f(bv_dx, bv_dz);
        const int al = locked ? 235 : 130 + ((bv_t & 6) ? 40 : 0);
        const unsigned int col = locked ? RGBA(255, 40, 30, al) : RGBA(255, 150, 40, al);
        const float len = 120.0f, ox = cosf(a) * 1.9f, oz = -sinf(a) * 1.9f;
        lvBeam(bv_x, 0.15f, bv_z, a, len, 1.0f, col);
        lvBeam(bv_x + ox, 0.15f, bv_z + oz, a, len, 0.35f, col);
        lvBeam(bv_x - ox, 0.15f, bv_z - oz, a, len, 0.35f, col);
    }
    // aviso del TAJO: abanico frente al jinete; golpe: media luna de filo
    if (bv_state == BV_SWING) {
        const float a0 = atan2f(-sinf(bv_yaw), -cosf(bv_yaw));
        if (bv_swingT > 3 && bv_swingT < 30) {
            const int al = 110 + (bv_swingT > 18 ? 90 : 0);
            for (int k = -2; k <= 2; ++k) lvBeam(bv_x, 0.15f, bv_z, a0 + k * 0.62f, 8.6f, 0.3f, RGBA(255, 70, 40, al));
        }
        if (bv_swingT >= 27 && bv_swingT <= 36) {
            LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 6 * 10); int n = 0;
            const float prog = bvClamp((bv_swingT - 27) / 4.0f, 0.0f, 1.0f);
            const int al = 230 - (bv_swingT - 27) * 22;
            for (int k = 0; k < 10; ++k) {
                const float u0 = -1.3f + 2.6f * k / 10.0f * prog + 1.3f * (1.0f - prog) * 0.0f, u1 = -1.3f + 2.6f * (k + 1) / 10.0f * prog;
                const float r0 = 3.2f, r1 = 8.0f;
                const float x0 = bv_x + sinf(a0 + u0) * r0, z0 = bv_z + cosf(a0 + u0) * r0, x1 = bv_x + sinf(a0 + u0) * r1, z1 = bv_z + cosf(a0 + u0) * r1;
                const float x2 = bv_x + sinf(a0 + u1) * r1, z2 = bv_z + cosf(a0 + u1) * r1, x3 = bv_x + sinf(a0 + u1) * r0, z3 = bv_z + cosf(a0 + u1) * r0;
                swQuadC(v, n, x0, 3.0f, z0, RGBA(150, 255, 110, al / 3), x1, 3.4f, z1, RGBA(236, 255, 214, al), x2, 3.4f, z2, RGBA(236, 255, 214, al), x3, 3.0f, z3, RGBA(150, 255, 110, al / 3));
            }
            sceGumLoadIdentity(); sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, n, 0, v);
        }
    }
    {   // particulas (cruces de 3 cuadros) y ceniza que cae por toda la arena
        LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * (18 * 72 + 12 * 40)); int n = 0;
        for (int k = 0; k < 72; ++k) {
            const BvPart &p = bv_part[k]; if (p.life <= 0) continue;
            const int a = (int)((p.col >> 24) * (float)p.life / (float)p.max);
            const unsigned int c = (p.col & 0x00FFFFFFu) | ((unsigned int)a << 24);
            const float s = p.sz * (0.6f + 0.6f * (1.0f - (float)p.life / (float)p.max));
            bvAddQuadA(v, n, p.x - s, p.y, p.z - s, p.x - s, p.y, p.z + s, p.x + s, p.y, p.z + s, p.x + s, p.y, p.z - s, c);
            bvAddQuadA(v, n, p.x - s, p.y - s, p.z, p.x + s, p.y - s, p.z, p.x + s, p.y + s, p.z, p.x - s, p.y + s, p.z, c);
            bvAddQuadA(v, n, p.x, p.y - s, p.z - s, p.x, p.y - s, p.z + s, p.x, p.y + s, p.z + s, p.x, p.y + s, p.z - s, c);
        }
        for (int k = 0; k < 40; ++k) {                                              // ceniza ambiente (ancla en el mundo)
            float rx = swRand(k, 1, 211u) * 60.0f - 30.0f - px, rz = swRand(k, 2, 211u) * 60.0f - 30.0f - pz;
            rx -= 60.0f * floorf((rx + 30.0f) / 60.0f); rz -= 60.0f * floorf((rz + 30.0f) / 60.0f);
            const float sp = 0.7f + swRand(k, 3, 211u), fy = swRand(k, 4, 211u) * 22.0f;
            float y = 18.0f - fmodf(bv_t * 0.06f * sp + fy, 22.0f);
            const float x = px + rx + sinf(bv_t * 0.02f + k) * 1.5f, z = pz + rz + cosf(bv_t * 0.017f + k * 2.0f) * 1.5f;
            const unsigned int c = (k % 5 == 0) ? RGBA(150, 255, 90, 200) : RGBA(150, 150, 138, 190);
            const float s = (k % 5 == 0) ? 0.09f : 0.12f; y += 0.0f;
            bvAddQuadA(v, n, x - s, y, z - s, x - s, y, z + s, x + s, y, z + s, x + s, y, z - s, c);
            bvAddQuadA(v, n, x - s, y - s, z, x + s, y - s, z, x + s, y + s, z, x - s, y + s, z, c);
        }
        sceGumLoadIdentity(); if (n > 0) sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, n, 0, v);
    }
    sceGuDepthMask(GU_FALSE); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0); sceGuDisable(GU_BLEND);
    sceGumLoadIdentity();
}

// ---------------------------------------------------------------- HUD
static void bvStr(char *d, const char *a, int n) {
    int k = 0; while (*a && k < 40) d[k++] = *a++;
    if (n >= 10) d[k++] = (char)('0' + (n / 10) % 10);
    d[k++] = (char)('0' + n % 10); d[k] = 0;
}
static void bv_Hud(BossIO &io) {
    (void)io;
    sceGuDisable(GU_TEXTURE_2D);
    sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    bxTint(RGBA(70, 120, 20, 16));                                                   // bruma radioactiva sobre todo
    // aviso de carga: bordes rojos que parpadean
    if (bv_state == BV_CWIND && bv_st > 60 && (bv_t & 4)) {
        drawRect(0, 0, SCR_WIDTH, 6, RGBA(255, 40, 30, 150)); drawRect(0, SCR_HEIGHT - 6, SCR_WIDTH, 6, RGBA(255, 40, 30, 150));
        drawRect(0, 0, 6, SCR_HEIGHT, RGBA(255, 40, 30, 110)); drawRect(SCR_WIDTH - 6, 0, 6, SCR_HEIGHT, RGBA(255, 40, 30, 110));
    }
    const int nf = bvFlyAlive();
    if (bv_state != BV_DEAD || bv_dis < 1.0f) {
        bxBossBar("JINETE PÁLIDO", bv_hp, BV_HPMAX);
        if (nf > 0) drawRect(120, 10, 240, 16, RGBA(110, 255, 70, 34 + (int)(26.0f * (0.5f + 0.5f * sinf(bv_t * 0.2f)))));   // escudo verde sobre la barra
    }
    // marcas verdes sobre las moscas visibles
    for (int k = 0; k < BV_NFLY; ++k) {
        const BvFly &F = bv_fly[k]; if (!F.alive || F.delay > 0 || F.die > 0) continue;
        int sx, sy;
        if (lvProject(F.x, F.y, F.z, sx, sy)) {
            const unsigned int c = RGBA(150, 255, 90, 220);
            drawRect(sx - 8, sy - 8, 4, 1, c); drawRect(sx - 8, sy - 8, 1, 4, c);
            drawRect(sx + 4, sy - 8, 4, 1, c); drawRect(sx + 7, sy - 8, 1, 4, c);
            drawRect(sx - 8, sy + 7, 4, 1, c); drawRect(sx - 8, sy + 4, 1, 4, c);
            drawRect(sx + 4, sy + 7, 4, 1, c); drawRect(sx + 7, sy + 4, 1, 4, c);
        }
    }
    fontTexOn();
    if (nf > 0) {
        char b[48]; bvStr(b, "MOSCAS: ", nf);
        drawText(240 - textLen(b) * 4, 30, 1.0f, RGBA(150, 255, 90, 255), b);
        drawText(240 - textLen("ESCUDO DE MOSCAS: INVULNERABLE") * 4, 42, 1.0f, RGBA(120, 210, 90, 255), "ESCUDO DE MOSCAS: INVULNERABLE");
    }
    sceGuDisable(GU_TEXTURE_2D);
}

// ---------------------------------------------------------------- cielo 2D
static int bvProj(float x, float y, float z, float &sx, float &sy, float &zv) {
    const float dx = x - g_lvEye[0], dy = y - g_lvEye[1], dz = z - g_lvEye[2];
    zv = dx * g_lvFwd[0] + dy * g_lvFwd[1] + dz * g_lvFwd[2]; if (zv < 0.5f) return 0;
    const float xv = dx * g_lvRight[0] + dy * g_lvRight[1] + dz * g_lvRight[2];
    const float yv = dx * g_lvUp[0] + dy * g_lvUp[1] + dz * g_lvUp[2];
    sx = 240.0f + 207.9f * xv / zv; sy = 136.0f - 209.4f * yv / zv;
    return 1;
}
static void bvBand(int y0, int y1, unsigned int c0, unsigned int c1) {      // gradQuad recortado a la pantalla
    if (y1 <= y0 || y1 <= 0 || y0 >= SCR_HEIGHT) return;
    int a = y0, b = y1; unsigned int ca = c0, cb = c1; const float h = (float)(y1 - y0);
    if (a < 0) { ca = lvMix(c0, c1, (0 - y0) / h); a = 0; }
    if (b > SCR_HEIGHT) { cb = lvMix(c0, c1, (SCR_HEIGHT - y0) / h); b = SCR_HEIGHT; }
    gradQuad(a, b, ca, cb);
}
static void bvQuad2D(float x0, float y0, unsigned int c0, float x1, float y1, unsigned int c1,
                     float x2, float y2, unsigned int c2, float x3, float y3, unsigned int c3) {
    GradVertex *v = (GradVertex *)sceGuGetMemory(sizeof(GradVertex) * 6);
    v[0] = { c0, (short)x0, (short)y0, 0 }; v[1] = { c1, (short)x1, (short)y1, 0 }; v[2] = { c2, (short)x2, (short)y2, 0 };
    v[3] = { c0, (short)x0, (short)y0, 0 }; v[4] = { c2, (short)x2, (short)y2, 0 }; v[5] = { c3, (short)x3, (short)y3, 0 };
    sceGuDrawArray(GU_TRIANGLES, GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 6, 0, v);
}
static void bv_Backdrop() {
    // el horizonte sigue la inclinacion de la camara
    float fl = sqrtf(g_lvFwd[0] * g_lvFwd[0] + g_lvFwd[2] * g_lvFwd[2]); if (fl < 0.12f) fl = 0.12f;
    const int H = (int)bvClamp(136.0f + 209.4f * g_lvFwd[1] / fl, -300.0f, 600.0f);
    const unsigned int top = RGBA(26, 30, 28, 255), mid = RGBA(58, 68, 52, 255);
    bvBand(0, H - 260, top, top);
    bvBand(H - 260, H - 100, top, mid);
    bvBand(H - 100, H, mid, BV_FOG);
    bvBand(H, SCR_HEIGHT + 200, BV_FOG, RGBA(60, 72, 48, 255));

    sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    // hongo nuclear lejano (azimut fijo en el mundo: se mueve con la camara)
    float mx = -9999.0f, my = 0.0f, zv = 1.0f;
    const float az = -0.55f, D = 420.0f;
    const int ok = bvProj(g_lvEye[0] + sinf(az) * D, 0.0f, g_lvEye[2] - cosf(az) * D, mx, my, zv);
    // resplandor verde del horizonte
    const float gx = ok ? mx : 240.0f;
    lvFan2D((int)bvClamp(gx, -400.0f, 880.0f), H - 8, 300, RGBA(170, 235, 60, 150), RGBA(110, 190, 50, 0), 28);
    lvFan2D(60, H - 4, 200, RGBA(120, 200, 50, 80), RGBA(100, 170, 40, 0), 20);
    lvFan2D(430, H - 4, 200, RGBA(120, 200, 50, 80), RGBA(100, 170, 40, 0), 20);
    if (ok && mx > -200.0f && mx < 680.0f) {
        const float k = 207.9f / zv, br = 0.03f * sinf(bv_t * 0.02f);
        const int bx = (int)mx, by = (int)my;
        lvFan2D(bx, by - (int)(150 * k), (int)(330 * k), RGBA(150, 230, 60, 90), RGBA(120, 200, 50, 0), 22);              // halo
        lvFan2D(bx, by, (int)(220 * k), RGBA(60, 56, 40, 230), RGBA(40, 44, 34, 0), 20);                                  // falda de polvo
        lvFan2D(bx, by, (int)(120 * k), RGBA(255, 150, 40, 130), RGBA(200, 120, 40, 0), 16);                              // brasa de la base
        // tallo (se ensancha hacia arriba, iluminado)
        const float h = 240.0f * k, w0 = 30.0f * k, w1 = 20.0f * k;
        bvQuad2D(bx - w0, by, RGBA(44, 48, 36, 255), bx + w0, by, RGBA(44, 48, 36, 255),
                 bx + w1, by - h, RGBA(118, 160, 54, 255), bx - w1, by - h, RGBA(118, 160, 54, 255));
        // sombrero: bolsas de humo oscuro con nucleo toxico
        const int cy = by - (int)(h + 26.0f * k);
        lvFan2D(bx, cy, (int)((150 + 150 * br) * k), RGBA(34, 40, 30, 255), RGBA(30, 36, 28, 0), 26);
        lvFan2D(bx - (int)(95 * k), cy + (int)(22 * k), (int)(80 * k), RGBA(40, 46, 34, 255), RGBA(30, 36, 28, 0), 18);
        lvFan2D(bx + (int)(95 * k), cy + (int)(22 * k), (int)(80 * k), RGBA(40, 46, 34, 255), RGBA(30, 36, 28, 0), 18);
        lvFan2D(bx, cy + (int)(10 * k), (int)(105 * k), RGBA(160, 230, 70, 210), RGBA(110, 170, 50, 0), 22);
        lvFan2D(bx, cy + (int)(4 * k), (int)(55 * k), RGBA(240, 255, 150, 220), RGBA(180, 240, 90, 0), 16);
        lvFan2D(bx, cy - (int)(48 * k), (int)(80 * k), RGBA(70, 80, 56, 230), RGBA(50, 58, 44, 0), 18);
    }
    // nubes oscuras que pasan lento, con la panza iluminada de verde
    for (int c = 0; c < 10; ++c) {
        const int x = ((c * 131 + (int)(bv_t * 0.2f * (1 + c % 3))) % 760) - 140;
        const int y = H - 60 - (c * 37) % 150, r = 70 + (c * 29) % 70;
        lvFan2D(x, y, r, RGBA(18, 22, 18, 190), RGBA(24, 28, 22, 0), 16);
    }
    for (int c = 0; c < 6; ++c) {
        const int x = ((c * 197 + 60 + (int)(bv_t * 0.13f * (1 + c % 2))) % 700) - 100;
        const int y = H - 26 - (c * 23) % 40, r = 80 + (c * 41) % 60;
        lvFan2D(x, y, r, RGBA(110, 160, 54, 90), RGBA(100, 150, 50, 0), 16);
    }
    // relampago radioactivo: destello verde en las nubes de vez en cuando
    {
        const int ph = bv_t % 520;
        if (ph < 12) { const int a = (ph < 3 ? ph * 22 : (12 - ph) * 7); drawRect(0, 0, SCR_WIDTH, SCR_HEIGHT, RGBA(150, 255, 90, a)); }
    }
    sceGuDisable(GU_BLEND);
}

// ---------------------------------------------------------------- resto del contrato
static unsigned int bv_Fog(float *nearF, float *farF) { *nearF = 30.0f; *farF = 195.0f; return BV_FOG; }

static int bv_AimTarget(float out[3]) {
    if (bv_state == BV_DEAD) return 0;
    float best = 0.5f; int found = 0;
    for (int k = 0; k < BV_NFLY; ++k) {                       // la mosca viva mas cercana a la mirada
        const BvFly &F = bv_fly[k]; if (!F.alive || F.die > 0 || F.delay > 0) continue;
        const float dx = F.x - g_lvEye[0], dy = F.y - g_lvEye[1], dz = F.z - g_lvEye[2];
        const float d = sqrtf(dx * dx + dy * dy + dz * dz); if (d < 0.5f || d > 150.0f) continue;
        const float c = (dx * g_lvFwd[0] + dy * g_lvFwd[1] + dz * g_lvFwd[2]) / d;
        if (c > best) { best = c; out[0] = F.x + F.vx * 3.0f; out[1] = F.y + F.vy * 3.0f; out[2] = F.z + F.vz * 3.0f; found = 1; }
    }
    if (found) return 1;
    float wx, wy, wz; bvW(0.0f, 5.4f, 0.1f, wx, wy, wz);       // si no, el jinete (pecho)
    const float dx = wx - g_lvEye[0], dy = wy - g_lvEye[1], dz = wz - g_lvEye[2];
    const float d = sqrtf(dx * dx + dy * dy + dz * dz); if (d < 0.5f || d > 150.0f) return 0;
    if ((dx * g_lvFwd[0] + dy * g_lvFwd[1] + dz * g_lvFwd[2]) / d < 0.4f) return 0;
    out[0] = wx; out[1] = wy; out[2] = wz;
    return 1;
}
static bool bv_ShotHit(float x, float y, float z, int dmg) {
    if (bv_state == BV_DEAD) return false;
    if (dmg < 1) dmg = 1;
    for (int k = 0; k < BV_NFLY; ++k) {                       // moscas: caen con 1-2 tiros
        BvFly &F = bv_fly[k]; if (!F.alive || F.die > 0 || F.delay > 0) continue;
        const float dx = x - F.x, dy = y - F.y, dz = z - F.z;
        if (dx * dx + dy * dy + dz * dz < 1.8f * 1.8f) {
            F.hp -= dmg; bvEmit(F.x, F.y, F.z, 0.3f, 0.05f, 0.0f, 10, RGBA(200, 255, 150, 255), 0.15f);
            if (F.hp <= 0) bvKillFly(F);
            return true;
        }
    }
    // el jinete y su caballo: esferas sobre cabeza, pecho, pecho y grupa del caballo, y cabeza del caballo
    static const float kS[5][4] = { { 0.0f, 5.8f, 0.15f, 2.1f }, { 0.0f, 7.0f, -0.1f, 1.1f }, { 0.0f, 3.4f, -1.5f, 1.9f },
                                    { 0.0f, 3.3f, 1.8f, 1.9f }, { 0.0f, 5.7f, -4.2f, 1.1f } };
    for (int q = 0; q < 5; ++q) {
        float wx, wy, wz; bvW(kS[q][0], kS[q][1], kS[q][2], wx, wy, wz);
        const float dx = x - wx, dy = y - wy, dz = z - wz;
        if (dx * dx + dy * dy + dz * dz < kS[q][3] * kS[q][3]) {
            if (bvFlyAlive() > 0 || bv_state == BV_INTRO) {   // escudo de moscas: la bala rebota
                bvEmit(x, y, z, 0.8f, 0.1f, -0.002f, 14, RGBA(120, 255, 70, 255), 0.22f);
                if (bv_blockMsg <= 0 && bv_state != BV_INTRO) { lvMessage("¡INVULNERABLE! ¡DERRIBA LAS MOSCAS!", 80); bv_blockMsg = 1; }
                return true;
            }
            bv_hp -= dmg; bv_hurt = 8;
            bvEmit(x, y, z, 0.8f, 0.1f, -0.002f, 14, RGBA(255, 230, 150, 255), 0.22f);
            if (bv_hp <= 0) { bv_hp = 0; bvKillBoss(); }
            return true;
        }
    }
    return false;
}
static int   bv_Blocks() { return 0; }
static int   bv_Done()   { return bv_done; }
static float bv_Shake()  { return bv_shake; }
