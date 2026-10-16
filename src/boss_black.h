// ============================================================================
// boss_black.h -- PISO 6: "EL JINETE NEGRO" (el tercer jinete: HAMBRE)
//
// Contrato en boss_api.h (prefijo bk_). Se incluye DESPUES de boss_api.h.
// Usa: addLimb/addBall/addLoft (char_prims.h), swBoxLit/swBoxL/swQuadC/swRand (skyworld.h).
//
// Escenario: noche negro-violeta con polvo dorado y un sol negro; losa de marmol negro de
//   80x80 con lineas doradas (la losa ES la caja de suelo, tope y=0); 8 columnas con brasero.
//   Arena grande a proposito: hay que correr, saltar las ondas y escapar del jinete.
// Jefe: caballo negro de ojos ambar que trota en circulo, jinete encapuchado y una BALANZA
//   dorada (dos platillos que oscilan). La balanza se levanta para el JUEGO DE DADOS.
// Dados: ambos tiran 1..6 (HUD con dados girando). El MAYOR PIERDE: si el tuyo es mayor
//   pierdes 20 de vida; si el del jinete es mayor, el pierde 15; empate: nada.
// Ataques: monedas/plumas oscuras (lvFireOrb) y onda de la balanza (anillos que se saltan).
// Al morir la balanza se rompe, el jinete se desvanece y bk_Done() = 1.
// ============================================================================
#pragma once

// ---------------------------------------------------------------- constantes
static const float BK_PI = 3.14159265f, BK_TAU = 6.2831853f;
static const float BK_CX0 = 0.0f, BK_CZ0 = -4.0f, BK_RADIUS = 22.0f;  // circulo por donde trota (centro inicial)
static const float BK_SPEED = 0.20f, BK_WAVE_V = 0.5f, BK_WAVE_MAX = 60.0f;
static const int   BK_HPMAX = 170;     // era 100. Fases: 66% monedas en abanico, 33% voraz (furia)

static const unsigned int BK_GOLD   = RGBA(222, 178,  78, 255);
static const unsigned int BK_GOLDD  = RGBA(138, 100,  36, 255);
static const unsigned int BK_GOLDB  = RGBA(255, 226, 140, 255);
static const unsigned int BK_COAT   = RGBA( 52,  46,  68, 255);   // pelaje negro violaceo
static const unsigned int BK_COAT2  = RGBA( 34,  30,  46, 255);
static const unsigned int BK_HOOF   = RGBA( 26,  22,  34, 255);
static const unsigned int BK_VIOL   = RGBA( 78,  44, 118, 255);
static const unsigned int BK_VIOLD  = RGBA( 40,  22,  64, 255);
static const unsigned int BK_AMBER  = RGBA(255, 180,  34, 255);
static const unsigned int BK_CLOAK0 = RGBA( 30,  22,  44, 255);
static const unsigned int BK_CLOAK1 = RGBA( 64,  48,  92, 255);

enum { BK_INTRO, BK_FIGHT, BK_CALL, BK_WAIT, BK_ROLL, BK_RESULT, BK_DYING, BK_DEAD };

// ---------------------------------------------------------------- estado
static int   bk_st = BK_INTRO, bk_stT = 0, bk_t = 0, bk_anim = 0;
static int   bk_hp = BK_HPMAX, bk_done = 0, bk_built = 0, bk_ph2Said = 0, bk_fase = 1;
static float bk_bx = 0, bk_by = 0, bk_bz = -26, bk_yaw = 0, bk_ang = 3.14159f, bk_rad = 72.0f;
static float bk_cx = 0, bk_cz = -4;                          // centro del circulo (sigue al jugador, despacio)
static float bk_spd = 0, bk_ph = 0, bk_dir = 1.0f, bk_amp = 0, bk_bob = 0, bk_alpha = 0;
static float bk_raise = 0, bk_raiseT = 0, bk_beamTh = 0, bk_beamTT = 0;
static int   bk_scaleOn = 1;
static int   bk_diceT = 330, bk_atkT = 150, bk_orbs = 0, bk_orbT = 0, bk_windT = 0;
static int   bk_waveT = 0, bk_waveTot = 236, bk_waveHit = 0, bk_waveN = 3, bk_waveSp = 56;
static float bk_waveX = 0, bk_waveZ = 0;
static int   bk_pVal = 1, bk_bVal = 1, bk_res = 0;          // dados y resultado (0 empate, 1 pierdes tu, 2 pierde el)
static int   bk_hurt = 0, bk_shakeT = 0, bk_contactCd = 0;
static float bk_hitX = 0, bk_hitY = 0, bk_hitZ = 0, bk_prevHp = 100.0f;
static unsigned int bk_seed = 24680u;

struct BkFrag { float x, y, z, vx, vy, vz, ax, az, vax, vaz, ymin; };
static BkFrag bk_frag[3];                                    // 0 = anillo+viga, 1 y 2 = platillos

// ---------------------------------------------------------------- modelos
static const int BK_CAP_BODY = 6400, BK_CAP_LEGU = 420, BK_CAP_LEGL = 420, BK_CAP_TAIL = 440;
static const int BK_CAP_TOP = 480, BK_CAP_BEAM = 600, BK_CAP_PAN = 720;
static LineVertex __attribute__((aligned(16))) bk_vBody[6400];
static LineVertex __attribute__((aligned(16))) bk_vLegU[2][420];   // 0 = mano (delantera), 1 = anca (trasera)
static LineVertex __attribute__((aligned(16))) bk_vLegL[420];
static LineVertex __attribute__((aligned(16))) bk_vTail[440];
static LineVertex __attribute__((aligned(16))) bk_vTop[480];       // anillo + vastago de la balanza
static LineVertex __attribute__((aligned(16))) bk_vBeam[600];      // viga
static LineVertex __attribute__((aligned(16))) bk_vPan[720];       // platillo con cadenas y monedas
static int bk_nBody = 0, bk_nLegU[2] = { 0, 0 }, bk_nLegL = 0, bk_nTail = 0, bk_nTop = 0, bk_nBeam = 0, bk_nPan = 0;

// ---------------------------------------------------------------- utilidades
static float bk_Rand() { bk_seed = bk_seed * 1664525u + 1013904223u; return (float)((bk_seed >> 8) & 0xFFFFu) / 65536.0f; }
static float bk_Wrap(float d) { while (d > BK_PI) d -= BK_TAU; while (d < -BK_PI) d += BK_TAU; return d; }
static float bk_Clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
static float bk_Smooth(float v) { v = bk_Clamp01(v); return v * v * (3.0f - 2.0f * v); }
static float bk_D2(float ax, float ay, float az, float bx, float by, float bz) {
    const float dx = ax - bx, dy = ay - by, dz = az - bz; return dx * dx + dy * dy + dz * dz;
}
// punto local del jefe (mira a -Z) -> mundo
static void bk_Loc2World(float lx, float ly, float lz, float &wx, float &wy, float &wz) {
    const float c = cosf(bk_yaw), s = sinf(bk_yaw);
    wx = bk_bx + lx * c + lz * s;
    wz = bk_bz - lx * s + lz * c;
    wy = bk_by + ly;
}

// envoltorios con tope de capacidad (bk_capNow = tamano del arreglo que se esta armando)
static int bk_capNow = 0;
static void bk_limb(LineVertex *b, int &i, float x0, float y0, float z0, float x1, float y1, float z1,
                    float r0, float r1, int sides, unsigned int cB, unsigned int cT) {
    if (i + sides * 12 > bk_capNow) return;
    addLimb(b, i, x0, y0, z0, x1, y1, z1, r0, r1, sides, cB, cT);
}
static void bk_ball(LineVertex *b, int &i, float cx, float cy, float cz, float rx, float ry, float rz,
                    int st, int sl, unsigned int col) {
    if (i + st * sl * 6 > bk_capNow) return;
    addBall(b, i, cx, cy, cz, rx, ry, rz, st, sl, col);
}
static void bk_loft(LineVertex *b, int &i, float cx, float cz, const float *ys, const float *rs, int n, int sides,
                    unsigned int cB, unsigned int cT) {
    if (i + (n - 1) * sides * 12 > bk_capNow) return;
    addLoft(b, i, cx, cz, ys, rs, n, sides, cB, cT);
}

// ---------------------------------------------------------------- armado de modelos
static void bk_BuildModels() {
    int i = 0;
    // =============== CABALLO + JINETE (todo lo que no se mueve respecto al cuerpo) ===============
    bk_capNow = BK_CAP_BODY;
    // torso: pecho, barril y anca
    bk_ball(bk_vBody, i, 0.0f, 3.75f, 0.2f, 1.15f, 1.2f, 2.5f, 6, 10, BK_COAT);
    bk_ball(bk_vBody, i, 0.0f, 3.95f, -1.9f, 1.05f, 1.3f, 1.3f, 6, 9, BK_COAT);
    bk_ball(bk_vBody, i, 0.0f, 3.95f, 2.2f, 1.12f, 1.28f, 1.35f, 6, 9, BK_COAT);
    // pechera dorada
    bk_ball(bk_vBody, i, 0.0f, 3.8f, -2.95f, 0.98f, 0.72f, 0.34f, 4, 8, BK_GOLDD);
    // cuello, cabeza, hocico
    bk_limb(bk_vBody, i, 0.0f, 4.5f, -2.3f, 0.0f, 6.3f, -3.6f, 0.9f, 0.55f, 9, BK_COAT, BK_COAT2);
    bk_ball(bk_vBody, i, 0.0f, 6.55f, -3.95f, 0.52f, 0.58f, 0.8f, 5, 8, BK_COAT);
    bk_limb(bk_vBody, i, 0.0f, 6.45f, -4.4f, 0.0f, 5.55f, -5.3f, 0.5f, 0.36f, 8, BK_COAT, BK_COAT2);
    bk_ball(bk_vBody, i, 0.0f, 5.5f, -5.35f, 0.38f, 0.34f, 0.4f, 4, 7, BK_COAT2);
    bk_ball(bk_vBody, i, -0.18f, 5.52f, -5.62f, 0.07f, 0.07f, 0.06f, 3, 5, RGBA(4, 3, 8, 255));
    bk_ball(bk_vBody, i, 0.18f, 5.52f, -5.62f, 0.07f, 0.07f, 0.06f, 3, 5, RGBA(4, 3, 8, 255));
    // ojos de ambar y orejas
    bk_ball(bk_vBody, i, -0.5f, 6.7f, -4.1f, 0.14f, 0.14f, 0.17f, 3, 6, BK_AMBER);
    bk_ball(bk_vBody, i, 0.5f, 6.7f, -4.1f, 0.14f, 0.14f, 0.17f, 3, 6, BK_AMBER);
    bk_limb(bk_vBody, i, -0.3f, 7.0f, -3.5f, -0.36f, 7.8f, -3.55f, 0.16f, 0.03f, 5, BK_COAT2, BK_COAT);
    bk_limb(bk_vBody, i, 0.3f, 7.0f, -3.5f, 0.36f, 7.8f, -3.55f, 0.16f, 0.03f, 5, BK_COAT2, BK_COAT);
    // testera dorada y crin violeta
    bk_limb(bk_vBody, i, 0.0f, 6.92f, -4.15f, 0.0f, 5.98f, -5.15f, 0.2f, 0.15f, 6, BK_GOLDD, BK_GOLD);
    for (int k = 0; k < 7; ++k) {
        const float t = k / 6.0f, ny = 4.5f + 1.8f * t, nz = -2.3f - 1.3f * t;
        bk_limb(bk_vBody, i, 0.0f, ny + 0.55f, nz + 0.2f, 0.0f, ny + 1.05f + 0.15f * (1.0f - t), nz + 1.1f, 0.30f, 0.03f, 5, BK_VIOL, BK_VIOLD);
    }
    bk_limb(bk_vBody, i, 0.0f, 7.15f, -3.75f, 0.0f, 6.65f, -4.35f, 0.2f, 0.03f, 5, BK_VIOL, BK_VIOLD);
    // gualdrapa violeta con ribete dorado, y silla
    bk_ball(bk_vBody, i, 0.0f, 4.35f, 0.3f, 1.32f, 0.62f, 1.75f, 5, 9, BK_VIOLD);
    bk_ball(bk_vBody, i, -1.12f, 3.55f, 0.3f, 0.26f, 1.0f, 1.9f, 4, 8, BK_VIOL);
    bk_ball(bk_vBody, i, 1.12f, 3.55f, 0.3f, 0.26f, 1.0f, 1.9f, 4, 8, BK_VIOL);
    bk_limb(bk_vBody, i, -1.32f, 2.62f, -1.4f, -1.32f, 2.62f, 2.0f, 0.07f, 0.07f, 5, BK_GOLD, BK_GOLD);
    bk_limb(bk_vBody, i, 1.32f, 2.62f, -1.4f, 1.32f, 2.62f, 2.0f, 0.07f, 0.07f, 5, BK_GOLD, BK_GOLD);
    bk_ball(bk_vBody, i, 0.0f, 4.75f, 0.3f, 0.8f, 0.28f, 1.0f, 4, 8, RGBA(70, 34, 40, 255));
    // ---- jinete: capa (loft), capa trasera, capucha sin rostro
    static const float ys[5] = { 4.2f, 4.9f, 5.9f, 6.7f, 7.1f };
    static const float rs[5] = { 1.2f, 1.0f, 0.72f, 0.85f, 0.5f };
    bk_loft(bk_vBody, i, 0.0f, 0.15f, ys, rs, 5, 9, BK_CLOAK0, BK_CLOAK1);
    bk_limb(bk_vBody, i, 0.0f, 6.7f, 0.7f, 0.0f, 3.8f, 2.9f, 0.75f, 0.35f, 7, BK_VIOL, BK_VIOLD);
    bk_ball(bk_vBody, i, 0.0f, 7.45f, 0.05f, 0.62f, 0.72f, 0.66f, 5, 9, RGBA(38, 30, 56, 255));
    bk_limb(bk_vBody, i, 0.0f, 7.9f, 0.2f, 0.0f, 8.9f, 0.85f, 0.5f, 0.04f, 7, RGBA(38, 30, 56, 255), RGBA(52, 40, 76, 255));
    bk_ball(bk_vBody, i, 0.0f, 7.4f, -0.5f, 0.32f, 0.42f, 0.22f, 4, 7, RGBA(2, 2, 4, 255));
    bk_ball(bk_vBody, i, -0.15f, 7.5f, -0.74f, 0.06f, 0.05f, 0.05f, 3, 5, RGBA(255, 236, 170, 255));
    bk_ball(bk_vBody, i, 0.15f, 7.5f, -0.74f, 0.06f, 0.05f, 0.05f, 3, 5, RGBA(255, 236, 170, 255));
    // hombreras y gorguera dorada
    bk_ball(bk_vBody, i, -0.85f, 6.55f, 0.05f, 0.42f, 0.3f, 0.4f, 4, 7, BK_GOLDD);
    bk_ball(bk_vBody, i, 0.85f, 6.55f, 0.05f, 0.42f, 0.3f, 0.4f, 4, 7, BK_GOLDD);
    bk_limb(bk_vBody, i, 0.0f, 6.8f, 0.1f, 0.0f, 7.1f, 0.1f, 0.8f, 0.5f, 8, BK_GOLDD, BK_GOLD);
    // brazo izquierdo (sostiene las riendas) y rienda
    bk_limb(bk_vBody, i, -0.85f, 6.45f, 0.05f, -1.15f, 5.5f, -0.5f, 0.28f, 0.24f, 7, BK_CLOAK1, BK_CLOAK0);
    bk_limb(bk_vBody, i, -1.15f, 5.5f, -0.5f, -0.65f, 5.1f, -1.5f, 0.24f, 0.18f, 7, BK_CLOAK0, BK_CLOAK1);
    bk_ball(bk_vBody, i, -1.15f, 5.5f, -0.5f, 0.26f, 0.26f, 0.26f, 3, 6, BK_CLOAK0);
    bk_ball(bk_vBody, i, -0.65f, 5.1f, -1.5f, 0.2f, 0.2f, 0.2f, 3, 6, BK_GOLDD);
    bk_limb(bk_vBody, i, -0.65f, 5.1f, -1.5f, -0.22f, 5.75f, -4.9f, 0.035f, 0.035f, 4, BK_GOLDD, BK_GOLDD);
    bk_nBody = i;

    // =============== PATAS: parte alta (hanging desde el origen hacia -Y) ===============
    for (int f = 0; f < 2; ++f) {
        i = 0; bk_capNow = BK_CAP_LEGU;
        if (f == 1) bk_ball(bk_vLegU[f], i, 0.0f, -0.3f, 0.15f, 0.62f, 0.9f, 0.7f, 4, 7, BK_COAT);   // muslo
        bk_limb(bk_vLegU[f], i, 0.0f, 0.0f, 0.0f, 0.0f, -1.7f, 0.0f, f ? 0.42f : 0.50f, 0.30f, 8, BK_COAT, BK_COAT2);
        bk_ball(bk_vLegU[f], i, 0.0f, -1.7f, 0.0f, 0.30f, 0.30f, 0.30f, 3, 6, BK_COAT2);
        bk_nLegU[f] = i;
    }
    // parte baja: canon, menudillo con argolla dorada y casco
    i = 0; bk_capNow = BK_CAP_LEGL;
    bk_limb(bk_vLegL, i, 0.0f, 0.0f, 0.0f, 0.0f, -1.25f, 0.0f, 0.26f, 0.20f, 8, BK_COAT2, BK_COAT2);
    bk_ball(bk_vLegL, i, 0.0f, -1.25f, 0.0f, 0.24f, 0.22f, 0.24f, 3, 6, BK_COAT);
    bk_limb(bk_vLegL, i, 0.0f, -1.02f, 0.0f, 0.0f, -0.94f, 0.0f, 0.27f, 0.27f, 8, BK_GOLD, BK_GOLD);
    bk_limb(bk_vLegL, i, 0.0f, -1.25f, 0.0f, 0.0f, -1.7f, -0.05f, 0.22f, 0.31f, 8, BK_HOOF, BK_HOOF);
    bk_nLegL = i;

    // =============== COLA ===============
    i = 0; bk_capNow = BK_CAP_TAIL;
    bk_limb(bk_vTail, i, 0.0f, 0.0f, 0.0f, 0.0f, -1.4f, 0.4f, 0.32f, 0.30f, 7, BK_VIOL, BK_VIOL);
    bk_limb(bk_vTail, i, 0.0f, -1.4f, 0.4f, 0.0f, -2.8f, 0.5f, 0.30f, 0.20f, 7, BK_VIOL, BK_VIOLD);
    bk_limb(bk_vTail, i, 0.0f, -2.8f, 0.5f, 0.0f, -3.8f, 0.2f, 0.20f, 0.02f, 7, BK_VIOLD, BK_VIOLD);
    bk_limb(bk_vTail, i, 0.18f, -0.2f, 0.0f, 0.3f, -3.3f, 0.6f, 0.15f, 0.02f, 5, BK_VIOL, BK_VIOLD);
    bk_limb(bk_vTail, i, -0.18f, -0.2f, 0.0f, -0.3f, -3.3f, 0.6f, 0.15f, 0.02f, 5, BK_VIOL, BK_VIOLD);
    bk_nTail = i;

    // =============== BALANZA (origen: la argolla que sujeta la mano) ===============
    i = 0; bk_capNow = BK_CAP_TOP;
    bk_ball(bk_vTop, i, 0.0f, 0.0f, 0.0f, 0.28f, 0.34f, 0.09f, 4, 8, BK_GOLD);
    bk_limb(bk_vTop, i, 0.0f, -0.1f, 0.0f, 0.0f, -0.8f, 0.0f, 0.09f, 0.12f, 6, BK_GOLD, BK_GOLD);
    bk_ball(bk_vTop, i, 0.0f, -0.85f, 0.0f, 0.24f, 0.24f, 0.24f, 4, 7, BK_GOLDB);
    bk_nTop = i;
    // viga (origen: el pivote)
    i = 0; bk_capNow = BK_CAP_BEAM;
    bk_limb(bk_vBeam, i, 0.0f, 0.0f, 0.0f, -1.7f, 0.0f, 0.0f, 0.16f, 0.08f, 6, BK_GOLD, BK_GOLDD);
    bk_limb(bk_vBeam, i, 0.0f, 0.0f, 0.0f, 1.7f, 0.0f, 0.0f, 0.16f, 0.08f, 6, BK_GOLD, BK_GOLDD);
    bk_ball(bk_vBeam, i, -1.7f, 0.0f, 0.0f, 0.16f, 0.16f, 0.16f, 3, 6, BK_GOLDB);
    bk_ball(bk_vBeam, i, 1.7f, 0.0f, 0.0f, 0.16f, 0.16f, 0.16f, 3, 6, BK_GOLDB);
    bk_ball(bk_vBeam, i, 0.0f, 0.0f, 0.0f, 0.28f, 0.2f, 0.2f, 4, 7, BK_GOLD);
    bk_nBeam = i;
    // platillo (origen: el gancho de la viga; cuelga hacia -Y)
    i = 0; bk_capNow = BK_CAP_PAN;
    for (int k = 0; k < 3; ++k) {
        const float a = k * 2.0944f;
        bk_limb(bk_vPan, i, 0.0f, 0.0f, 0.0f, 0.7f * cosf(a), -2.0f, 0.7f * sinf(a), 0.03f, 0.03f, 4, BK_GOLDD, BK_GOLDD);
    }
    bk_limb(bk_vPan, i, 0.0f, -2.45f, 0.0f, 0.0f, -2.05f, 0.0f, 0.28f, 0.9f, 12, BK_GOLDD, BK_GOLD);
    bk_limb(bk_vPan, i, 0.0f, -2.07f, 0.0f, 0.0f, -2.03f, 0.0f, 0.82f, 0.82f, 12, BK_VIOLD, BK_VIOLD);
    bk_limb(bk_vPan, i, 0.0f, -2.04f, 0.0f, 0.0f, -1.92f, 0.0f, 0.36f, 0.36f, 10, BK_GOLD, BK_GOLDB);   // monedas
    bk_ball(bk_vPan, i, 0.0f, 0.0f, 0.0f, 0.1f, 0.1f, 0.1f, 3, 5, BK_GOLDB);
    bk_nPan = i;
    sceKernelDcacheWritebackAll();
}

// ---------------------------------------------------------------- escenario
static void bk_FlatQuad(LineVertex *g, int &i, int cap, float x0, float z0, float x1, float z1, float w, float y, unsigned int col) {
    if (i + 6 > cap) return;
    const float dx = x1 - x0, dz = z1 - z0, l = sqrtf(dx * dx + dz * dz);
    if (l < 1e-4f) return;
    const float nx = -dz / l * w * 0.5f, nz = dx / l * w * 0.5f;
    swQuadC(g, i, x0 - nx, y, z0 - nz, col, x1 - nx, y, z1 - nz, col, x1 + nx, y, z1 + nz, col, x0 + nx, y, z0 + nz, col);
}
static void bk_FlatRing(LineVertex *g, int &i, int cap, float cx, float cz, float R, float w, float y, unsigned int col, int seg) {
    const float r0 = R - w * 0.5f, r1 = R + w * 0.5f;
    for (int s = 0; s < seg; ++s) {
        if (i + 6 > cap) return;
        const float a0 = BK_TAU * s / seg, a1 = BK_TAU * (s + 1) / seg;
        swQuadC(g, i, cx + sinf(a0) * r0, y, cz + cosf(a0) * r0, col, cx + sinf(a0) * r1, y, cz + cosf(a0) * r1, col,
                      cx + sinf(a1) * r1, y, cz + cosf(a1) * r1, col, cx + sinf(a1) * r0, y, cz + cosf(a1) * r0, col);
    }
}
static void bk_FlatDisc(LineVertex *g, int &i, int cap, float cx, float cz, float R, float y, unsigned int col, int seg) {
    for (int s = 0; s < seg; ++s) {
        if (i + 3 > cap) return;
        const float a0 = BK_TAU * s / seg, a1 = BK_TAU * (s + 1) / seg;
        g[i++] = { col, cx, y, cz };
        g[i++] = { col, cx + sinf(a0) * R, y, cz + cosf(a0) * R };
        g[i++] = { col, cx + sinf(a1) * R, y, cz + cosf(a1) * R };
    }
}

static void bk_BuildStage(LineVertex *g, int &i, int cap) {
    const unsigned int MT = RGBA(58, 56, 76, 255), MB = RGBA(26, 24, 38, 255);
    // losa de marmol negro de 80x80 (techo y=0) y la base escalonada que se hunde en la oscuridad
    if (i + 36 <= cap) swBoxLit(g, i, 0.0f, -3.0f, 0.0f, 80.0f, 80.0f, 3.0f, MT, MB);
    if (i + 36 <= cap) swBoxLit(g, i, 0.0f, -9.0f, 0.0f, 68.0f, 68.0f, 6.0f, RGBA(34, 30, 46, 255), RGBA(14, 12, 22, 255));
    if (i + 36 <= cap) swBoxLit(g, i, 0.0f, -17.0f, 0.0f, 52.0f, 52.0f, 8.0f, RGBA(28, 24, 40, 255), RGBA(10, 8, 18, 255));
    if (i + 36 <= cap) swBoxLit(g, i, 0.0f, -27.0f, 0.0f, 32.0f, 32.0f, 10.0f, RGBA(22, 18, 32, 255), RGBA(8, 6, 14, 255));
    // ribete dorado en el borde de la losa
    if (i + 120 <= cap) {
        swBoxLit(g, i, 0.0f, -0.02f, -39.4f, 80.0f, 1.2f, 0.2f, BK_GOLD, BK_GOLDD);
        swBoxLit(g, i, 0.0f, -0.02f, 39.4f, 80.0f, 1.2f, 0.2f, BK_GOLD, BK_GOLDD);
        swBoxLit(g, i, -39.4f, -0.02f, 0.0f, 1.2f, 77.6f, 0.2f, BK_GOLD, BK_GOLDD);
        swBoxLit(g, i, 39.4f, -0.02f, 0.0f, 1.2f, 77.6f, 0.2f, BK_GOLD, BK_GOLDD);
    }
    // vetas claras del marmol
    for (int k = 0; k < 24; ++k) {
        const float x0 = (swRand(k, 1, 41u) - 0.5f) * 70.0f, z0 = (swRand(k, 2, 42u) - 0.5f) * 70.0f;
        const float a = swRand(k, 3, 43u) * BK_TAU, l = 6.0f + 12.0f * swRand(k, 4, 44u);
        bk_FlatQuad(g, i, cap, x0, z0, x0 + sinf(a) * l, z0 + cosf(a) * l, 0.14f, 0.10f, RGBA(92, 88, 116, 255));
    }
    // losetas: cuadricula tenue cada 8u (da sensacion de distancia al correr)
    for (int k = -4; k <= 4; ++k) {
        const float v = k * 8.0f;
        bk_FlatQuad(g, i, cap, v, -38.0f, v, 38.0f, 0.12f, 0.09f, RGBA(74, 70, 96, 255));
        bk_FlatQuad(g, i, cap, -38.0f, v, 38.0f, v, 0.12f, 0.09f, RGBA(74, 70, 96, 255));
    }
    // lineas doradas: anillos, estrella {8/3}, rayos y emblema de la balanza al centro
    const unsigned int GL = RGBA(206, 160, 64, 255), GB = RGBA(244, 204, 100, 255);
    bk_FlatRing(g, i, cap, 0.0f, 0.0f, 35.0f, 0.9f, 0.14f, GL, 72);
    bk_FlatRing(g, i, cap, 0.0f, 0.0f, 24.0f, 0.6f, 0.14f, GL, 56);
    bk_FlatRing(g, i, cap, 0.0f, 0.0f, 12.0f, 0.6f, 0.14f, GL, 40);
    for (int k = 0; k < 8; ++k) {
        const float a0 = BK_TAU * k / 8.0f + 0.3927f, a1 = BK_TAU * ((k + 3) % 8) / 8.0f + 0.3927f;
        bk_FlatQuad(g, i, cap, sinf(a0) * 24.0f, cosf(a0) * 24.0f, sinf(a1) * 24.0f, cosf(a1) * 24.0f, 0.44f, 0.14f, GL);
        const float b = BK_TAU * k / 8.0f;
        bk_FlatQuad(g, i, cap, sinf(b) * 12.0f, cosf(b) * 12.0f, sinf(b) * 35.0f, cosf(b) * 35.0f, 0.4f, 0.14f, GL);
    }
    bk_FlatQuad(g, i, cap, -4.8f, -2.0f, 4.8f, -2.0f, 0.44f, 0.16f, GB);            // viga
    bk_FlatQuad(g, i, cap, 0.0f, -2.0f, 0.0f, 4.4f, 0.52f, 0.16f, GB);              // poste
    bk_FlatQuad(g, i, cap, -4.8f, -2.0f, -4.8f, -0.2f, 0.16f, 0.16f, GB);           // cadenas
    bk_FlatQuad(g, i, cap, 4.8f, -2.0f, 4.8f, -0.2f, 0.16f, 0.16f, GB);
    bk_FlatRing(g, i, cap, -4.8f, 0.8f, 1.4f, 0.4f, 0.16f, GB, 24);                  // platillos
    bk_FlatRing(g, i, cap, 4.8f, 0.8f, 1.4f, 0.4f, 0.16f, GB, 24);
    bk_FlatDisc(g, i, cap, 0.0f, 5.2f, 0.9f, 0.16f, GB, 12);                        // base
    // 8 columnas negras con bandas doradas y un brasero de brasas, fuera de la losa
    for (int a = 0; a < 8; ++a) {
        float x, z;
        if (a < 4) { x = (a & 1) ? 50.0f : -50.0f; z = (a & 2) ? 50.0f : -50.0f; }
        else       { x = (a == 4) ? 54.0f : (a == 5) ? -54.0f : 0.0f; z = (a == 6) ? 54.0f : (a == 7) ? -54.0f : 0.0f; }
        if (i + 200 > cap) break;
        swBoxLit(g, i, x, -16.0f, z, 5.0f, 5.0f, 72.0f, RGBA(44, 40, 60, 255), RGBA(14, 12, 22, 255));
        swBoxLit(g, i, x, 6.0f, z, 5.8f, 5.8f, 1.0f, BK_GOLD, BK_GOLDD);
        swBoxLit(g, i, x, 22.0f, z, 5.8f, 5.8f, 1.0f, BK_GOLD, BK_GOLDD);
        swBoxLit(g, i, x, 38.0f, z, 5.8f, 5.8f, 1.0f, BK_GOLD, BK_GOLDD);
        swBoxLit(g, i, x, 56.0f, z, 7.2f, 7.2f, 1.6f, BK_GOLD, BK_GOLDD);
        swBoxLit(g, i, x, 57.6f, z, 3.0f, 3.0f, 2.2f, RGBA(255, 160, 44, 255), RGBA(200, 90, 20, 255));
    }
    // monolitos negros flotando a lo lejos (profundidad en la niebla)
    for (int k = 0; k < 16; ++k) {
        if (i + 40 > cap) break;
        const float a = k * 0.3927f + swRand(k, 5, 51u) * 0.3f, r = 85.0f + 55.0f * swRand(k, 6, 52u);
        const float w = 10.0f + 14.0f * swRand(k, 7, 53u), h = 30.0f + 40.0f * swRand(k, 8, 54u), y0 = -60.0f + 50.0f * swRand(k, 9, 55u);
        swBoxLit(g, i, sinf(a) * r, y0, cosf(a) * r, w, w * (0.6f + 0.5f * swRand(k, 10, 56u)), h, RGBA(36, 30, 52, 255), RGBA(12, 8, 20, 255));
    }
}

// ================================================================ CONTRATO ===================================
static void bk_Enter(BossIO &io, LineVertex *geo, int geoCap, int *geoN,
                     CityBldg *boxes, int *nBoxes, int boxCap, float *sx, float *sy, float *sz) {
    if (!bk_built) { bk_BuildModels(); bk_built = 1; }
    int i = 0;
    bk_BuildStage(geo, i, geoCap);
    if (geoN) *geoN = i;
    int nb = 0;
    if (boxes && boxCap > 0) boxes[nb++] = { 0.0f, 0.0f, 80.0f, 80.0f, 0.0f, RGBA(52, 50, 68, 255) };   // el suelo (techo y=0)
    if (nBoxes) *nBoxes = nb;
    if (sx) *sx = 0.0f;
    if (sy) *sy = 0.2f;
    if (sz) *sz = 30.0f;
    io.camYaw = 0.0f;
    sceKernelDcacheWritebackAll();

    bk_st = BK_INTRO; bk_stT = 0; bk_t = 0; bk_anim = 0;
    bk_hp = BK_HPMAX; bk_done = 0; bk_ph2Said = 0; bk_fase = 1;
    bk_cx = BK_CX0; bk_cz = BK_CZ0;
    bk_ang = BK_PI; bk_rad = 72.0f; bk_dir = 1.0f; bk_spd = 0.0f; bk_ph = 0.0f; bk_amp = 0.0f; bk_bob = 0.0f;
    bk_bx = bk_cx + sinf(bk_ang) * bk_rad; bk_bz = bk_cz + cosf(bk_ang) * bk_rad; bk_by = 0.0f;
    bk_yaw = atan2f(-bk_dir * cosf(bk_ang), bk_dir * sinf(bk_ang));
    bk_alpha = 0.0f; bk_raise = 0.0f; bk_raiseT = 0.0f; bk_beamTh = 0.0f; bk_beamTT = 0.0f; bk_scaleOn = 1;
    bk_diceT = 330; bk_atkT = 170; bk_orbs = 0; bk_orbT = 0; bk_windT = 0; bk_waveT = 0; bk_waveHit = 0;
    bk_pVal = bk_bVal = 1; bk_res = 0; bk_hurt = 0; bk_shakeT = 0; bk_contactCd = 0;
    bk_prevHp = io.hp;
    bk_seed ^= (unsigned int)(g_lvT * 2654435761u + 977u);
}

// ---- movimiento: trota en circulo; "stop" lo frena; "faceP" lo gira hacia el jugador
static void bk_Move(BossIO &io, int stop, int faceP) {
    const float tgt = stop ? 0.0f : BK_SPEED;
    bk_spd += (tgt - bk_spd) * (stop ? 0.07f : 0.04f);
    if (stop && bk_spd < 0.003f) bk_spd = 0.0f;
    bk_ang += bk_dir * bk_spd / bk_rad;
    if (bk_ang > BK_TAU) bk_ang -= BK_TAU;
    if (bk_ang < 0.0f) bk_ang += BK_TAU;
    bk_ph += bk_spd * 1.6f;
    if (bk_ph > BK_TAU) bk_ph -= BK_TAU;
    bk_amp = bk_spd / BK_SPEED;
    if (bk_amp > 1.2f) bk_amp = 1.2f;
    bk_bx = bk_cx + sinf(bk_ang) * bk_rad;
    bk_bz = bk_cz + cosf(bk_ang) * bk_rad;
    float want;
    if (faceP) want = atan2f(-(io.px - bk_bx), -(io.pz - bk_bz));
    else       want = atan2f(-bk_dir * cosf(bk_ang), bk_dir * sinf(bk_ang));
    bk_yaw += bk_Wrap(want - bk_yaw) * (faceP ? 0.10f : 0.16f);
    bk_bob = 0.22f * bk_amp * fabsf(cosf(bk_ph)) + 0.03f * sinf(bk_anim * 0.05f);
}
static void bk_Cam(BossIO &io, float k) {
    const float d = bk_Wrap(atan2f(bk_bx - io.px, -(bk_bz - io.pz)) - io.camYaw);
    io.camYaw += d * k;
}
static void bk_ScaleHand(float &hx, float &hy, float &hz) {
    const float e = bk_Smooth(bk_raise);
    hx = 4.0f + (3.4f - 4.0f) * e;
    hy = 5.6f + (9.0f - 5.6f) * e + 0.12f * sinf(bk_anim * 0.07f);
    hz = -1.4f + (-1.2f + 1.4f) * e;
    const float dx = hx - 0.85f, dy = hy - 6.45f, dz = hz - 0.05f, d = sqrtf(dx * dx + dy * dy + dz * dz);
    if (d > 3.85f) { const float k = 3.85f / d; hx = 0.85f + dx * k; hy = 6.45f + dy * k; hz = 0.05f + dz * k; }
}

static void bk_StartDeath() {
    bk_st = BK_DYING; bk_stT = 0;
    bk_waveT = 0; bk_windT = 0; bk_orbs = 0;
    float hx, hy, hz, wx, wy, wz;
    bk_ScaleHand(hx, hy, hz);
    // anillo + viga
    bk_Loc2World(hx, hy - 0.85f, hz, wx, wy, wz);
    bk_frag[0] = { wx, wy, wz, (bk_Rand() - 0.5f) * 0.2f, 0.28f, (bk_Rand() - 0.5f) * 0.2f, 0.0f, 0.0f, 0.07f, 0.05f, 1.2f };
    // platillos
    for (int k = 0; k < 2; ++k) {
        const float side = k ? 1.0f : -1.0f;
        bk_Loc2World(hx + side * 1.7f * cosf(bk_beamTh), hy - 0.85f + side * 1.7f * sinf(bk_beamTh), hz, wx, wy, wz);
        bk_frag[1 + k] = { wx, wy, wz, side * (0.12f + 0.1f * bk_Rand()), 0.2f, (bk_Rand() - 0.5f) * 0.2f,
                           0.0f, 0.0f, (bk_Rand() - 0.5f) * 0.12f, side * 0.1f, 2.5f };
    }
    bk_scaleOn = 0;
    bk_raiseT = 0.0f;
    bk_shakeT = 40; g_lvFlash = 22;
    lvMessage("LA BALANZA SE ROMPE", 150);
}

// ---- la balanza se queda en alto durante los dados; al volver a FIGHT baja
static void bk_Update(BossIO &io) {
    if (g_lvTimeStop > 0) return;                      // tiempo detenido: todo congelado
    ++bk_t; ++bk_anim;
    bk_Rand();
    if (bk_hurt > 0) --bk_hurt;
    if (bk_shakeT > 0) --bk_shakeT;
    if (bk_contactCd > 0) --bk_contactCd;

    // el jugador murio y reaparecio (levels.h le devuelve toda la vida): se cancelan los ataques
    if (io.hp > bk_prevHp + 25.0f && bk_st != BK_INTRO && bk_st != BK_DYING && bk_st != BK_DEAD) {
        bk_orbs = 0; bk_waveT = 0; bk_windT = 0;
        if (bk_st >= BK_CALL && bk_st <= BK_RESULT) { bk_st = BK_FIGHT; bk_raiseT = 0.0f; bk_diceT = 420; bk_atkT = 120; }
    }

    // balanza: suave subida/bajada y viga que oscila (o se inclina con el resultado)
    bk_raise += (bk_raiseT - bk_raise) * 0.08f;
    if (bk_st != BK_RESULT) bk_beamTT = 0.10f * sinf(bk_anim * 0.05f);
    bk_beamTh += (bk_beamTT - bk_beamTh) * 0.10f;

    switch (bk_st) {
    // -------------------------------------------------------------- entrada: galopa desde el vacio
    case BK_INTRO: {
        if (bk_t == 20) lvMessage("EL TERCER JINETE: HAMBRE", 170);
        const float e = bk_Smooth(bk_t / 240.0f);
        bk_rad = 72.0f - (72.0f - BK_RADIUS) * e;
        bk_alpha = bk_Clamp01(bk_t / 90.0f);
        bk_Move(io, 0, 0);
        if (bk_t >= 260) { bk_st = BK_FIGHT; bk_stT = 0; bk_alpha = 1.0f; bk_rad = BK_RADIUS; }
        break;
    }
    // -------------------------------------------------------------- pelea
    case BK_FIGHT: {
        {   // FASES (tambien cuenta el dano de los dados): 66% monedas en abanico, 33% voraz
            const int ph = (bk_hp * 3 > BK_HPMAX * 2) ? 1 : (bk_hp * 3 > BK_HPMAX ? 2 : 3);
            if (ph > bk_fase) { bk_fase = ph; g_lvFlash = 14; if (ph == 2) lvMessage("EL JINETE NEGRO ARROJA SUS MONEDAS", 170); }
        }
        const int ph2 = (bk_fase >= 3);
        if (ph2 && !bk_ph2Said) { bk_ph2Said = 1; lvMessage("EL HAMBRE SE VUELVE VORAZ", 150); }
        const int busy = (bk_windT > 0 || bk_waveT > 0 || bk_orbs > 0);
        // el circulo de trote sigue al jugador despacio: no hay rincon donde quedarse quieto
        {
            float tx = io.px * 0.6f, tz = io.pz * 0.6f - 4.0f;
            if (tx > 14.0f) tx = 14.0f;
            if (tx < -14.0f) tx = -14.0f;
            if (tz > 14.0f) tz = 14.0f;
            if (tz < -14.0f) tz = -14.0f;
            float ddx = tx - bk_cx, ddz = tz - bk_cz;
            if (ddx > 0.06f) ddx = 0.06f;
            if (ddx < -0.06f) ddx = -0.06f;
            if (ddz > 0.06f) ddz = 0.06f;
            if (ddz < -0.06f) ddz = -0.06f;
            bk_cx += ddx; bk_cz += ddz;
        }
        bk_Move(io, bk_windT > 0, bk_windT > 0);
        if (io.aiming) bk_Cam(io, 0.10f);              // con L la camara se fija en el

        // golpe de la balanza: se levanta, mira al jugador y golpea el suelo
        if (bk_windT > 0) {
            bk_raiseT = 1.0f;
            if (--bk_windT == 0) {
                bk_waveN = ph2 ? 4 : 3; bk_waveSp = ph2 ? 48 : 56; bk_waveTot = ph2 ? 268 : 236;
                bk_waveT = bk_waveTot; bk_waveHit = 0; bk_waveX = bk_bx; bk_waveZ = bk_bz;
                bk_raiseT = 0.0f; bk_shakeT = 18; g_lvFlash = 5;
            }
        }
        // anillos de la onda: salta para esquivarlos (solo daan a ras del suelo)
        if (bk_waveT > 0) {
            const int el = bk_waveTot - bk_waveT;
            const float d = sqrtf((io.px - bk_waveX) * (io.px - bk_waveX) + (io.pz - bk_waveZ) * (io.pz - bk_waveZ));
            for (int w = 0; w < bk_waveN; ++w) {
                const int st = w * bk_waveSp; if (el < st) continue;
                const float R = (el - st) * BK_WAVE_V; if (R > BK_WAVE_MAX) continue;
                if (!g_lvDashInv && !(bk_waveHit & (1 << w)) && fabsf(d - R) < 1.2f && io.py < 0.9f) {   // dash: la atraviesas
                    bk_waveHit |= 1 << w; io.hp -= 12.0f; g_lvFlash = 10;
                }
            }
            --bk_waveT;
        }
        // monedas y plumas oscuras
        if (bk_orbs > 0 && --bk_orbT <= 0) {
            float wx, wy, wz; bk_Loc2World(0.9f, 6.6f, -0.8f, wx, wy, wz);
            if (bk_fase >= 2) {   // MONEDAS EN ABANICO (desde el 66%)
                g_lvOrbFree = 1;
                for (int k = -1; k <= 1; ++k)
                    lvFireOrb(wx, wy, wz, io.px + k * 3.5f, io.py + 1.7f, io.pz + k * 1.5f, 0.32f, (bk_orbs & 1) ? RGBA(210, 160, 40, 255) : RGBA(92, 52, 150, 255));
                g_lvOrbFree = 0;
            } else
            lvFireOrb(wx, wy, wz, io.px, io.py + 1.7f, io.pz, 0.30f, (bk_orbs & 1) ? RGBA(210, 160, 40, 255) : RGBA(92, 52, 150, 255));
            --bk_orbs; bk_orbT = 48;
        }
        // atropello: si te quedas en su camino
        if (!g_lvDashInv && bk_contactCd == 0 && bk_spd > 0.05f && io.py < 6.0f) {   // dash: pasas entre las patas
            const float dx = io.px - bk_bx, dz = io.pz - bk_bz, l2 = dx * dx + dz * dz;
            if (l2 < 3.5f * 3.5f) {
                const float l = sqrtf(l2) + 0.001f;
                io.hp -= 10.0f; g_lvFlash = 8; bk_contactCd = 70;
                io.px += dx / l * 4.0f; io.pz += dz / l * 4.0f;
                if (io.px > 38.0f) io.px = 38.0f;
                if (io.px < -38.0f) io.px = -38.0f;
                if (io.pz > 38.0f) io.pz = 38.0f;
                if (io.pz < -38.0f) io.pz = -38.0f;
            }
        }
        // temporizadores: siguiente ataque / siguiente juego de dados
        if (!busy) {
            if (--bk_diceT <= 0) {
                bk_st = BK_CALL; bk_stT = 0; bk_raiseT = 1.0f;
                lvMessage("¡EL JINETE TE RETA A LOS DADOS!", 110);
                break;
            }
            if (--bk_atkT <= 0) {
                if (bk_Rand() < 0.55f) bk_orbs = ph2 ? 5 : 3;
                else { bk_windT = 50; lvMessage("¡SALTA LA ONDA!", 90); }
                bk_orbT = 20;
                bk_atkT = (ph2 ? 110 : 160) + (int)(bk_Rand() * 90.0f);
            }
        }
        break;
    }
    // -------------------------------------------------------------- dados: llamada
    case BK_CALL: {
        bk_Move(io, 1, 1);
        bk_Cam(io, 0.08f);
        bk_raiseT = 1.0f;
        if (++bk_stT >= 85) { bk_st = BK_WAIT; bk_stT = 0; }
        break;
    }
    // -------------------------------------------------------------- dados: espera a que tires con X
    case BK_WAIT: {
        bk_Move(io, 1, 1);
        bk_Cam(io, 0.06f);
        bk_raiseT = 1.0f;
        ++bk_stT;
        if ((io.edge & PSP_CTRL_CROSS) || bk_stT >= 600) {
            bk_pVal = 1 + (int)(bk_Rand() * 6.0f); if (bk_pVal > 6) bk_pVal = 6;
            bk_bVal = 1 + (int)(bk_Rand() * 6.0f); if (bk_bVal > 6) bk_bVal = 6;
            bk_st = BK_ROLL; bk_stT = 0;
        }
        break;
    }
    // -------------------------------------------------------------- dados: ruedan (0..45 el tuyo, 70..115 el suyo)
    case BK_ROLL: {
        bk_Move(io, 1, 1);
        bk_Cam(io, 0.06f);
        bk_raiseT = 1.0f;
        if (++bk_stT >= 118) {
            bk_st = BK_RESULT; bk_stT = 0;
            if (bk_pVal > bk_bVal)      { bk_res = 1; io.hp -= 20.0f; g_lvFlash = 8; bk_beamTT = 0.30f; }
            else if (bk_bVal > bk_pVal) { bk_res = 2; bk_hp -= 15; bk_hurt = 14; bk_beamTT = -0.30f; bk_shakeT = 20;
                                          bk_hitX = bk_bx; bk_hitY = bk_by + 5.0f; bk_hitZ = bk_bz; }
            else                        { bk_res = 0; bk_beamTT = 0.0f; }
        }
        break;
    }
    // -------------------------------------------------------------- dados: resultado
    case BK_RESULT: {
        bk_Move(io, 1, 1);
        bk_Cam(io, 0.06f);
        bk_raiseT = 1.0f;
        if (++bk_stT >= 135) {
            bk_beamTT = 0.0f;
            if (bk_hp <= 0) { bk_StartDeath(); break; }
            bk_st = BK_FIGHT; bk_stT = 0; bk_raiseT = 0.0f;
            bk_diceT = (bk_fase >= 3 ? 640 : 900) + (int)(bk_Rand() * 240.0f);
            bk_atkT = 110;
            if (bk_Rand() < 0.5f) bk_dir = -bk_dir;
        }
        break;
    }
    // -------------------------------------------------------------- muerte: la balanza se rompe y se desvanece
    case BK_DYING: {
        ++bk_stT;
        bk_spd *= 0.90f; bk_amp = bk_spd / BK_SPEED;
        for (int k = 0; k < 3; ++k) {
            BkFrag &f = bk_frag[k];
            f.vy -= 0.03f; f.x += f.vx; f.y += f.vy; f.z += f.vz; f.ax += f.vax; f.az += f.vaz;
            if (f.y < f.ymin) { f.y = f.ymin; f.vy = 0.0f; f.vx *= 0.8f; f.vz *= 0.8f; f.vax = f.vaz = 0.0f; f.ax *= 0.9f; f.az *= 0.9f; }
        }
        bk_alpha = (bk_stT < 40) ? 1.0f : bk_Clamp01(1.0f - (bk_stT - 40) / 110.0f);
        if (bk_stT >= 150) { bk_st = BK_DEAD; bk_alpha = 0.0f; bk_done = 1; }
        break;
    }
    default: break;
    }
    bk_prevHp = io.hp;
}

// ---------------------------------------------------------------- dibujo 3D
static void bk_BaseM(float lean) {
    sceGumLoadIdentity();
    ScePspFVector3 t = { bk_bx, bk_by + bk_bob, bk_bz };
    sceGumTranslate(&t);
    sceGumRotateY(bk_yaw);
    if (lean != 0.0f) sceGumRotateZ(lean);
}
static void bk_Tr(float x, float y, float z) { ScePspFVector3 t = { x, y, z }; sceGumTranslate(&t); }

// brazo derecho (hombro -> codo -> mano): se arma cada cuadro en coordenadas del jefe
static void bk_DrawArm(float hx, float hy, float hz) {
    const float sx = 0.85f, sy = 6.45f, sz = 0.05f, L = 2.0f;
    float ax = hx - sx, ay = hy - sy, az = hz - sz;
    const float d = sqrtf(ax * ax + ay * ay + az * az);
    if (d < 0.05f) return;
    ax /= d; ay /= d; az /= d;
    float px = 0.5f, py = -1.0f, pz = 0.3f;                 // el codo apunta hacia abajo y afuera
    const float dp = px * ax + py * ay + pz * az;
    px -= ax * dp; py -= ay * dp; pz -= az * dp;
    float pl = sqrtf(px * px + py * py + pz * pz);
    if (pl < 1e-4f) { px = 0.0f; py = 0.0f; pz = 1.0f; pl = 1.0f; }
    px /= pl; py /= pl; pz /= pl;
    float hh = L * L - d * d * 0.25f; hh = hh > 0.0f ? sqrtf(hh) : 0.0f;
    const float ex = sx + (hx - sx) * 0.5f + px * hh, ey = sy + (hy - sy) * 0.5f + py * hh, ez = sz + (hz - sz) * 0.5f + pz * hh;
    LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 420);
    int i = 0; bk_capNow = 420;
    bk_limb(v, i, sx, sy, sz, ex, ey, ez, 0.30f, 0.24f, 7, BK_CLOAK1, BK_CLOAK0);
    bk_limb(v, i, ex, ey, ez, hx, hy, hz, 0.24f, 0.18f, 7, BK_CLOAK0, BK_CLOAK1);
    bk_ball(v, i, ex, ey, ez, 0.26f, 0.26f, 0.26f, 3, 6, BK_CLOAK0);
    bk_ball(v, i, hx, hy, hz, 0.22f, 0.22f, 0.22f, 3, 6, BK_GOLDD);
    sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, i, 0, v);
}

// fundido global con GU_FIX (sin alpha por vertice)
static void bk_BlendOn(float a) {
    if (a >= 0.995f) return;
    int k = (int)(a * 255.0f); if (k < 0) k = 0; if (k > 255) k = 255;
    const unsigned int src = ((unsigned int)k << 16) | ((unsigned int)k << 8) | (unsigned int)k;
    const unsigned int dst = ((unsigned int)(255 - k) << 16) | ((unsigned int)(255 - k) << 8) | (unsigned int)(255 - k);
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_FIX, GU_FIX, src, dst);
}
static void bk_BlendOff(float a) {
    if (a >= 0.995f) return;
    sceGuDisable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
}

static void bk_DrawBoss() {
    if (bk_alpha <= 0.02f) return;
    const float lean = 0.06f * bk_dir * bk_amp;
    bk_BlendOn(bk_alpha);
    // cuerpo: caballo + jinete
    bk_BaseM(lean);
    sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, bk_nBody, 0, bk_vBody);
    // 4 patas: trote en diagonal (FL+BR contra FR+BL)
    for (int k = 0; k < 4; ++k) {
        const int front = (k < 2);
        const float hx = (k & 1) ? 0.6f : -0.6f, hz = front ? -1.6f : 2.2f;
        const float off = (k == 0 || k == 3) ? 0.0f : BK_PI;
        const float s = sinf(bk_ph + off), c = cosf(bk_ph + off);
        const float a1 = bk_amp * 0.55f * s;
        const float a2 = -bk_amp * (front ? 1.25f : 0.95f) * (c > 0.0f ? c : 0.0f);
        bk_BaseM(lean);
        bk_Tr(hx, 3.4f, hz);
        sceGumRotateX(a1);
        sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, bk_nLegU[front ? 0 : 1], 0, bk_vLegU[front ? 0 : 1]);
        bk_Tr(0.0f, -1.7f, 0.0f);
        sceGumRotateX(a2);
        sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, bk_nLegL, 0, bk_vLegL);
    }
    // cola al viento
    bk_BaseM(lean);
    bk_Tr(0.0f, 4.4f, 3.35f);
    sceGumRotateX(-(0.35f + 0.5f * bk_amp) + 0.10f * sinf(bk_anim * 0.07f));
    sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, bk_nTail, 0, bk_vTail);
    // brazo derecho + balanza
    float hx, hy, hz; bk_ScaleHand(hx, hy, hz);
    if (bk_scaleOn) {
        bk_BaseM(lean);
        bk_DrawArm(hx, hy, hz);
        bk_BaseM(lean);
        bk_Tr(hx, hy, hz);
        sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, bk_nTop, 0, bk_vTop);
        bk_Tr(0.0f, -0.85f, 0.0f);
        sceGumRotateZ(bk_beamTh);
        sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, bk_nBeam, 0, bk_vBeam);
        for (int k = 0; k < 2; ++k) {              // platillos colgando de los extremos, oscilando
            const float side = k ? 1.0f : -1.0f;
            const float ex = hx + side * 1.7f * cosf(bk_beamTh), ey = hy - 0.85f + side * 1.7f * sinf(bk_beamTh);
            const float sway = 0.14f * sinf(bk_anim * 0.08f + k * 1.7f) + 0.08f * bk_amp * sinf(bk_ph + k);
            bk_BaseM(lean);
            bk_Tr(ex, ey, hz);
            sceGumRotateZ(sway);
            sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, bk_nPan, 0, bk_vPan);
        }
    } else {
        bk_BaseM(lean);
        bk_DrawArm(4.0f, 5.4f, -1.2f);             // brazo vacio
    }
    // restos de la balanza rota
    if (bk_st == BK_DYING || bk_st == BK_DEAD) {
        for (int k = 0; k < 3; ++k) {
            const BkFrag &f = bk_frag[k];
            sceGumLoadIdentity();
            ScePspFVector3 t = { f.x, f.y, f.z }; sceGumTranslate(&t);
            sceGumRotateX(f.ax); sceGumRotateZ(f.az);
            if (k == 0) {
                sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, bk_nTop, 0, bk_vTop);
                ScePspFVector3 t2 = { 0.0f, -0.85f, 0.0f }; sceGumTranslate(&t2);
                sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, bk_nBeam, 0, bk_vBeam);
            } else {
                sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, bk_nPan, 0, bk_vPan);
            }
        }
    }
    bk_BlendOff(bk_alpha);
}

// sombra suave bajo el caballo
static void bk_DrawShadow() {
    if (bk_alpha <= 0.05f || bk_st == BK_DEAD) return;
    const int N = 14;
    LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 3 * N); int i = 0;
    const float c = cosf(bk_yaw), s = sinf(bk_yaw);
    const unsigned int col = RGBA(0, 0, 0, (int)(120.0f * bk_alpha));
    for (int k = 0; k < N; ++k) {
        const float a0 = BK_TAU * k / N, a1 = BK_TAU * (k + 1) / N;
        const float l0x = sinf(a0) * 1.7f, l0z = cosf(a0) * 3.4f + 0.3f, l1x = sinf(a1) * 1.7f, l1z = cosf(a1) * 3.4f + 0.3f;
        v[i++] = { col, bk_bx + 0.3f * s, 0.18f, bk_bz + 0.3f * c };
        v[i++] = { col, bk_bx + l0x * c + l0z * s, 0.18f, bk_bz - l0x * s + l0z * c };
        v[i++] = { col, bk_bx + l1x * c + l1z * s, 0.18f, bk_bz - l1x * s + l1z * c };
    }
    sceGumLoadIdentity();
    sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, i, 0, v);
}

// polvo dorado flotando por toda la losa (y chispas al morir)
static void bk_DrawMotes() {
    const int N = 56;
    LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 30 * (N + 28)); int i = 0;
    for (int k = 0; k < N; ++k) {
        const float a = swRand(k, 1, 5u) * BK_TAU, r = 4.0f + swRand(k, 2, 6u) * 50.0f;
        const float x = sinf(a) * r + 1.5f * sinf(bk_anim * 0.02f + k), z = cosf(a) * r + 1.5f * cosf(bk_anim * 0.017f + k * 1.3f);
        const float y = fmodf(swRand(k, 3, 7u) * 20.0f + bk_anim * (0.02f + 0.03f * swRand(k, 4, 8u)), 20.0f);
        const float sz = 0.10f + 0.10f * swRand(k, 5, 9u);
        swBoxL(v, i, x, y, z, sz, sz, sz, (k & 3) ? RGBA(236, 190, 80, 255) : RGBA(150, 100, 190, 255));
    }
    if (bk_st == BK_DYING) {
        const float t = (float)bk_stT;
        for (int k = 0; k < 28; ++k) {
            const float a = k * 2.399f, sr = swRand(k, 7, 31u), sr2 = swRand(k, 8, 32u);
            const float x = bk_bx + sinf(a + t * 0.02f) * (1.5f + 3.0f * sr), z = bk_bz + cosf(a + t * 0.02f) * (1.5f + 3.0f * sr);
            const float y = 2.0f + t * (0.05f + 0.05f * sr) + 3.0f * sr2;
            float sz = 0.4f * (1.0f - t / 150.0f); if (sz < 0.05f) sz = 0.05f;
            swBoxL(v, i, x, y, z, sz, sz, sz, (k & 1) ? BK_GOLDB : RGBA(170, 110, 220, 255));
        }
    }
    sceGumLoadIdentity();
    sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, i, 0, v);
}

static void bk_Draw(float px, float py, float pz) {
    (void)px; (void)py; (void)pz;
    sceGuDisable(GU_CULL_FACE);
    bk_DrawMotes();
    if (bk_st != BK_DEAD) {
        // sombra y ondas: color con alpha, sin escribir Z
        sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0); sceGuDepthMask(GU_TRUE);
        bk_DrawShadow();
        if (bk_windT > 0) {                                   // aviso: la balanza va a golpear
            const int a = 110 + (int)(90.0f * sinf(bk_anim * 0.5f));
            lvRing3D(bk_bx, 0.2f, bk_bz, 3.0f + (50 - bk_windT) * 0.05f, 0.5f, RGBA(255, 200, 80, a));
        }
        if (bk_waveT > 0) {
            const int el = bk_waveTot - bk_waveT;
            for (int w = 0; w < bk_waveN; ++w) {
                const int st = w * bk_waveSp; if (el < st) continue;
                const float R = (el - st) * BK_WAVE_V; if (R > BK_WAVE_MAX) continue;
                lvRing3D(bk_waveX, 0.22f, bk_waveZ, R, 1.2f, RGBA(176, 120, 28, 230));
                lvRing3D(bk_waveX, 0.26f, bk_waveZ, R, 0.3f, RGBA(255, 214, 120, 255));
            }
        }
        sceGuDepthMask(GU_FALSE); sceGuDisable(GU_BLEND);
        bk_DrawBoss();
        if (bk_hurt > 0 && (bk_hurt & 2)) lvBox(bk_hitX, bk_hitY - 0.5f, bk_hitZ, 1.0f, 1.0f, RGBA(255, 236, 170, 255));   // chispa del impacto
    }
    sceGumLoadIdentity();
}

// ---------------------------------------------------------------- 2D: cielo
static void bk_Disc2D(float cx, float cy, float r, unsigned int col, int seg) {
    GradVertex *v = (GradVertex *)sceGuGetMemory(sizeof(GradVertex) * 3 * seg);
    for (int s = 0; s < seg; ++s) {
        const float a0 = BK_TAU * s / seg, a1 = BK_TAU * (s + 1) / seg;
        v[s * 3 + 0] = { col, (short)cx, (short)cy, 0 };
        v[s * 3 + 1] = { col, (short)(cx + cosf(a0) * r), (short)(cy + sinf(a0) * r), 0 };
        v[s * 3 + 2] = { col, (short)(cx + cosf(a1) * r), (short)(cy + sinf(a1) * r), 0 };
    }
    sceGuDrawArray(GU_TRIANGLES, GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 3 * seg, 0, v);
}
static void bk_Glow2D(float cx, float cy, float r, unsigned int cIn, unsigned int cOut, int seg) {
    GradVertex *v = (GradVertex *)sceGuGetMemory(sizeof(GradVertex) * 3 * seg);
    for (int s = 0; s < seg; ++s) {
        const float a0 = BK_TAU * s / seg, a1 = BK_TAU * (s + 1) / seg;
        v[s * 3 + 0] = { cIn, (short)cx, (short)cy, 0 };
        v[s * 3 + 1] = { cOut, (short)(cx + cosf(a0) * r), (short)(cy + sinf(a0) * r), 0 };
        v[s * 3 + 2] = { cOut, (short)(cx + cosf(a1) * r), (short)(cy + sinf(a1) * r), 0 };
    }
    sceGuDrawArray(GU_TRIANGLES, GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 3 * seg, 0, v);
}
static void bk_Q2(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, unsigned int c) {
    GradVertex *v = (GradVertex *)sceGuGetMemory(sizeof(GradVertex) * 6);
    v[0] = { c, (short)x0, (short)y0, 0 }; v[1] = { c, (short)x1, (short)y1, 0 }; v[2] = { c, (short)x2, (short)y2, 0 };
    v[3] = { c, (short)x0, (short)y0, 0 }; v[4] = { c, (short)x2, (short)y2, 0 }; v[5] = { c, (short)x3, (short)y3, 0 };
    sceGuDrawArray(GU_TRIANGLES, GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 6, 0, v);
}

// noche negro-violeta, sol negro con corona dorada y polvo dorado que se eleva
static void bk_Backdrop() {
    sceGuDisable(GU_TEXTURE_2D);
    gradQuad(0, 110, RGBA(3, 2, 9, 255), RGBA(30, 14, 42, 255));
    gradQuad(110, 170, RGBA(30, 14, 42, 255), RGBA(60, 30, 58, 255));
    gradQuad(170, SCR_HEIGHT, RGBA(60, 30, 58, 255), RGBA(14, 7, 22, 255));
    sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    const float p = 0.5f + 0.5f * sinf(g_lvT * 0.03f);
    bk_Glow2D(360.0f, 86.0f, 120.0f, RGBA(255, 190, 70, 70 + (int)(30.0f * p)), RGBA(255, 150, 40, 0), 28);
    bk_Glow2D(360.0f, 86.0f, 44.0f, RGBA(255, 214, 120, 190), RGBA(255, 170, 60, 0), 24);
    bk_Disc2D(360.0f, 86.0f, 28.0f, RGBA(255, 204, 100, 255), 28);       // aro dorado
    bk_Disc2D(360.0f, 86.0f, 25.0f, RGBA(2, 1, 6, 255), 28);             // sol negro
    for (int k = 0; k < 48; ++k) {                                       // polvo dorado
        const int x = ((k * 97 + (int)(g_lvT * 0.25f * (1 + k % 3))) % 500) - 10;
        const int y = 272 - ((k * 53 + (int)(g_lvT * (0.15f + 0.07f * (k % 4)))) % 290);
        drawRect(x, y, 2, 2, RGBA(236, 190, 90, 60 + (k * 13) % 150));
    }
    sceGuDisable(GU_BLEND);
}

static unsigned int bk_Fog(float *nearF, float *farF) {
    *nearF = 45.0f; *farF = 180.0f;
    return RGBA(40, 20, 50, 255);
}

// ---------------------------------------------------------------- blanco, danio, estados
static int bk_AimTarget(float out[3]) {
    if (bk_st != BK_FIGHT) return 0;
    const float k = bk_spd * 7.0f;                          // un poco adelante, trota rapido
    out[0] = bk_bx + bk_dir * cosf(bk_ang) * k;
    out[1] = bk_by + 5.0f;
    out[2] = bk_bz - bk_dir * sinf(bk_ang) * k;
    return 1;
}
static bool bk_ShotHit(float x, float y, float z, int dmg) {
    if (bk_st != BK_FIGHT) return false;
    const float dx = x - bk_bx, dz = z - bk_bz;
    if (dx * dx + dz * dz > 3.8f * 3.8f || y < bk_by - 0.6f || y > bk_by + 9.8f) return false;   // radio generoso
    if (dmg < 1) dmg = 1;
    bk_hp -= dmg;
    bk_hurt = 8; bk_hitX = x; bk_hitY = y; bk_hitZ = z;
    if (bk_hp <= 0) { bk_hp = 0; bk_StartDeath(); }
    return true;
}
static int bk_Blocks() { return (bk_st >= BK_CALL && bk_st <= BK_RESULT) ? 1 : 0; }
static int bk_Done() { return bk_done; }
static float bk_Shake() {
    float a = 0.0f;
    if (bk_st == BK_CALL) a = 0.10f;
    if (bk_shakeT > 0) { const float s = 0.18f * (bk_shakeT > 20 ? 1.0f : bk_shakeT / 20.0f); if (s > a) a = s; }
    if (bk_st == BK_DYING && bk_stT < 120) { const float s = 0.22f * (1.0f - bk_stT / 120.0f); if (s > a) a = s; }
    return a;
}

// ---------------------------------------------------------------- HUD: dados
static void bk_TextC(int cx, int y, float sc, unsigned int col, const char *s) {
    drawText(cx - (int)(textLen(s) * 4.0f * sc), y, sc, col, s);
}
// pips de cada cara en [-0.5, 0.5]
static int bk_PipPos(int face, float *pu, float *pv) {
    const float a = 0.5f;
    switch (face) {
    case 1: pu[0] = 0; pv[0] = 0; return 1;
    case 2: pu[0] = -a; pv[0] = -a; pu[1] = a; pv[1] = a; return 2;
    case 3: pu[0] = -a; pv[0] = -a; pu[1] = 0; pv[1] = 0; pu[2] = a; pv[2] = a; return 3;
    case 4: pu[0] = -a; pv[0] = -a; pu[1] = a; pv[1] = -a; pu[2] = -a; pv[2] = a; pu[3] = a; pv[3] = a; return 4;
    case 5: pu[0] = -a; pv[0] = -a; pu[1] = a; pv[1] = -a; pu[2] = 0; pv[2] = 0; pu[3] = -a; pv[3] = a; pu[4] = a; pv[4] = a; return 5;
    case 6: pu[0] = -a; pv[0] = -a; pu[1] = -a; pv[1] = 0; pu[2] = -a; pv[2] = a;
            pu[3] = a; pv[3] = -a; pu[4] = a; pv[4] = 0; pu[5] = a; pv[5] = a; return 6;
    }
    return 0;
}
// dado en 2D con volumen (cara frontal + tapa + costado), girando en el plano y aplastado al "rodar"
static void bk_Die(float cx, float cy, float sz, int face, float ang, float sq, int boss) {
    const unsigned int cBody = boss ? RGBA(40, 34, 56, 255) : RGBA(236, 228, 206, 255);
    const unsigned int cBev  = boss ? RGBA(66, 56, 90, 255) : RGBA(252, 248, 236, 255);
    const unsigned int cTop  = boss ? RGBA(84, 70, 112, 255) : RGBA(212, 202, 174, 255);
    const unsigned int cSide = boss ? RGBA(22, 18, 34, 255) : RGBA(150, 140, 118, 255);
    const unsigned int cPip  = boss ? RGBA(240, 196, 92, 255) : RGBA(24, 18, 30, 255);
    const float ca = cosf(ang), sa = sinf(ang), h = sz * 0.5f;
    auto PX = [&](float u, float v) { return cx + (u * h * sq) * ca - (v * h) * sa; };
    auto PY = [&](float u, float v) { return cy + (u * h * sq) * sa + (v * h) * ca; };
    const float e0x = sz * 0.20f, e0y = -sz * 0.18f;
    const float ex = e0x * ca - e0y * sa, ey = e0x * sa + e0y * ca;
    // sombra (la silueta corrida abajo a la derecha)
    bk_Q2(PX(-1, -1) + 7, PY(-1, -1) + 10, PX(1, -1) + 7, PY(1, -1) + 10, PX(1, 1) + 7, PY(1, 1) + 10, PX(-1, 1) + 7, PY(-1, 1) + 10, RGBA(0, 0, 0, 110));
    // tapa y costado (la profundidad sube a la derecha)
    bk_Q2(PX(-1, -1), PY(-1, -1), PX(1, -1), PY(1, -1) , PX(1, -1) + ex, PY(1, -1) + ey, PX(-1, -1) + ex, PY(-1, -1) + ey, cTop);
    bk_Q2(PX(1, -1), PY(1, -1), PX(1, 1), PY(1, 1), PX(1, 1) + ex, PY(1, 1) + ey, PX(1, -1) + ex, PY(1, -1) + ey, cSide);
    // cara frontal + bisel
    bk_Q2(PX(-1, -1), PY(-1, -1), PX(1, -1), PY(1, -1), PX(1, 1), PY(1, 1), PX(-1, 1), PY(-1, 1), cBody);
    bk_Q2(PX(-0.84f, -0.84f), PY(-0.84f, -0.84f), PX(0.84f, -0.84f), PY(0.84f, -0.84f),
          PX(0.84f, 0.84f), PY(0.84f, 0.84f), PX(-0.84f, 0.84f), PY(-0.84f, 0.84f), cBev);
    float pu[6], pv[6];
    const int n = bk_PipPos(face, pu, pv);
    const float pr = sz * 0.075f * (0.65f + 0.35f * sq);
    for (int k = 0; k < n; ++k) {
        const unsigned int pc = (face == 1 && !boss) ? RGBA(200, 30, 44, 255) : cPip;
        bk_Disc2D(PX(pu[k] * 1.1f, pv[k] * 1.1f), PY(pu[k] * 1.1f, pv[k] * 1.1f), (face == 1) ? pr * 1.6f : pr, pc, 10);
    }
}
// animacion de un dado: tr = cuadros desde que empezo a rodar (<0 = aun no tirado)
static void bk_DieAnim(int tr, int val, int &face, float &ang, float &sq, float &yo, float &sc) {
    const int N = 46;
    face = 0; ang = 0.0f; sq = 1.0f; yo = 0.0f; sc = 1.0f;
    if (tr < 0) return;
    if (tr >= N) { face = val; const int lt = tr - N; sc = (lt < 12) ? 1.0f + 0.22f * (12 - lt) / 12.0f : 1.0f; return; }
    const float prog = (float)tr / N, inv = 1.0f - prog;
    int f = 1 + (int)(swRand(tr / 3, val, 91u) * 6.0f); if (f > 6) f = 6;
    face = (tr >= N - 5) ? val : f;
    ang = inv * inv * tr * 0.45f;
    sq = 1.0f - inv * 0.5f * fabsf(sinf(tr * 0.55f));
    yo = -inv * 40.0f * fabsf(sinf(tr * 0.3f));
}

static void bk_HudDice() {
    if (bk_st != BK_WAIT && bk_st != BK_ROLL && bk_st != BK_RESULT) return;
    const float DX0 = 150.0f, DX1 = 330.0f, DY = 132.0f, DS = 64.0f;
    bxTint(RGBA(8, 4, 18, 110));
    sceGuDisable(GU_TEXTURE_2D);
    drawRect(70, 44, 340, 204, RGBA(10, 6, 20, 205));
    drawRect(70, 44, 340, 2, RGBA(222, 178, 78, 230));
    drawRect(70, 246, 340, 2, RGBA(222, 178, 78, 230));
    // tiempos de cada dado
    int trP = -1, trB = -1;
    if (bk_st == BK_ROLL) { trP = bk_stT; trB = bk_stT - 70; }
    if (bk_st == BK_RESULT) { trP = 200; trB = 200; }
    int fP, fB; float aP, sP, yP, cP, aB, sB, yB, cB;
    bk_DieAnim(trP, bk_pVal, fP, aP, sP, yP, cP);
    bk_DieAnim(trB, bk_bVal, fB, aB, sB, yB, cB);
    // marco del perdedor
    if (bk_st == BK_RESULT && bk_res != 0) {
        const float fx = (bk_res == 1) ? DX0 : DX1;
        const unsigned int fc = (bk_res == 1) ? RGBA(255, 90, 90, 230) : RGBA(255, 214, 110, 230);
        const int x0 = (int)(fx - DS * 0.5f - 12), y0 = (int)(DY - DS * 0.5f - 10), w = (int)(DS + 36), h = (int)(DS + 22);
        drawRect(x0, y0, w, 2, fc); drawRect(x0, y0 + h, w, 2, fc);
        drawRect(x0, y0, 2, h, fc); drawRect(x0 + w, y0, 2, h + 2, fc);
    }
    // dados (la cara 0 = "?" se escribe despues como texto)
    bk_Die(DX0, DY + yP, DS * cP, fP, aP, sP, 0);
    bk_Die(DX1, DY + yB, DS * cB, fB, aB, sB, 1);

    fontTexOn();
    bk_TextC(240, 52, 1.0f, RGBA(236, 206, 120, 255), "DADOS DEL JINETE NEGRO");
    bk_TextC(240, 66, 1.0f, RGBA(190, 184, 210, 255), "EL NÚMERO MAYOR PIERDE VIDA");
    bk_TextC(240, 122, 2.0f, RGBA(150, 140, 170, 255), "VS");
    bk_TextC((int)DX0, 176, 1.0f, RGBA(240, 240, 246, 255), "TÚ");
    bk_TextC((int)DX1, 176, 1.0f, RGBA(236, 206, 120, 255), "JINETE");
    if (fP == 0) bk_TextC((int)DX0, (int)DY - 12, 3.0f, RGBA(120, 110, 140, 255), "?");
    if (fB == 0) bk_TextC((int)DX1, (int)DY - 12, 3.0f, RGBA(236, 206, 120, 255), "?");
    if (bk_st == BK_WAIT) {
        const unsigned int c = (g_lvT & 16) ? RGBA(255, 255, 255, 255) : RGBA(190, 196, 214, 255);
        bk_TextC(240, 204, 1.0f, c, "PRESIONA X PARA TIRAR TU DADO");
        // reloj: si no tiras, tira solo
        sceGuDisable(GU_TEXTURE_2D);
        drawRect(140, 228, 200, 4, RGBA(40, 34, 60, 255));
        drawRect(140, 228, 200 * (600 - bk_stT) / 600, 4, RGBA(222, 178, 78, 255));
        fontTexOn();
    } else if (bk_st == BK_ROLL) {
        bk_TextC(240, 204, 1.0f, RGBA(220, 214, 236, 255), bk_stT < 70 ? "TIRAS EL DADO..." : "EL JINETE TIRA...");
    } else {
        char b[32]; int n = 0;
        b[n++] = 'T'; b[n++] = '\xC3'; b[n++] = '\x9A'; b[n++] = ':'; b[n++] = ' '; b[n++] = (char)('0' + bk_pVal);
        b[n++] = ' '; b[n++] = ' '; b[n++] = 'J'; b[n++] = 'I'; b[n++] = 'N'; b[n++] = 'E'; b[n++] = 'T'; b[n++] = 'E'; b[n++] = ':';
        b[n++] = ' '; b[n++] = (char)('0' + bk_bVal); b[n] = 0;
        bk_TextC(240, 196, 1.0f, RGBA(240, 240, 246, 255), b);
        if (bk_res == 1) {
            bk_TextC(240, 212, 1.0f, RGBA(255, 120, 120, 255), "¡SACASTE MÁS! PIERDES 20 DE VIDA");
            bk_TextC((int)DX0, 74, 2.0f, RGBA(255, 90, 90, 255), "-20");
        } else if (bk_res == 2) {
            bk_TextC(240, 212, 1.0f, RGBA(255, 214, 110, 255), "¡EL JINETE SACÓ MÁS! PIERDE 15");
            bk_TextC((int)DX1, 74, 2.0f, RGBA(255, 214, 110, 255), "-15");
        } else {
            bk_TextC(240, 212, 1.0f, RGBA(220, 220, 232, 255), "EMPATE: NADIE PIERDE VIDA");
        }
    }
    sceGuDisable(GU_TEXTURE_2D);
}

// brillo ambar de los ojos y de la balanza (suma luz)
static void bk_HudGlows() {
    if (bk_alpha < 0.3f || bk_st == BK_DEAD) return;
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_FIX, 0, 0xFFFFFFFF);
    const float fx = -sinf(bk_yaw), fz = -cosf(bk_yaw);
    float cdx = g_lvEye[0] - bk_bx, cdz = g_lvEye[2] - bk_bz;
    const float cl = sqrtf(cdx * cdx + cdz * cdz) + 0.001f;
    const float facing = (fx * cdx + fz * cdz) / cl;          // 1 = el caballo te mira de frente
    if (facing > 0.15f) {
        const float I = bk_Clamp01((facing - 0.15f) * 2.5f) * bk_alpha;
        for (int s = -1; s <= 1; s += 2) {
            float wx, wy, wz; bk_Loc2World(0.5f * s, 6.7f, -4.15f, wx, wy, wz);
            int sx, sy;
            if (lvProject(wx, wy, wz, sx, sy)) {
                const float dist = sqrtf(bk_D2(wx, wy, wz, g_lvEye[0], g_lvEye[1], g_lvEye[2])) + 0.1f;
                float r = 150.0f / dist; if (r < 3.0f) r = 3.0f; if (r > 16.0f) r = 16.0f;
                bk_Glow2D((float)sx, (float)sy, r, RGBA(255, 190, 50, (int)(210.0f * I)), RGBA(255, 120, 10, 0), 10);
            }
        }
    }
    if (bk_scaleOn && bk_raise > 0.15f) {
        float hx, hy, hz, wx, wy, wz; bk_ScaleHand(hx, hy, hz);
        bk_Loc2World(hx, hy - 1.6f, hz, wx, wy, wz);
        int sx, sy;
        if (lvProject(wx, wy, wz, sx, sy)) {
            const float p = 0.5f + 0.5f * sinf(g_lvT * 0.1f);
            bk_Glow2D((float)sx, (float)sy, 46.0f + 10.0f * p, RGBA(255, 214, 110, (int)(110.0f * bk_raise * bk_alpha)), RGBA(255, 180, 60, 0), 18);
        }
    }
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
}

static void bk_Hud(BossIO &io) {
    (void)io;
    if (bk_st == BK_DEAD) return;
    sceGuDisable(GU_TEXTURE_2D);
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    bk_HudGlows();
    bk_HudDice();
    if (bk_fase >= 3 && bk_st == BK_FIGHT) bxRage(bk_t);
    if (bk_st != BK_INTRO || bk_t > 30) bxBossBar("JINETE NEGRO", bk_hp, BK_HPMAX);
    sceGuDisable(GU_TEXTURE_2D);
}
