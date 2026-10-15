// ============================================================================
// boss_white.h -- PISO 4 "EL JINETE BLANCO" (primer jinete del Apocalipsis: CONQUISTA)
//
// Escenario: cielo blanco luminoso con brillo dorado, plataforma flotante blanca circular
// (radio 18, bordes dorados, columnas rotas). El jinete galopa EN EL CIELO alrededor de ella.
//
// Jinete: caballo blanco (cuello, cabeza, 4 patas articuladas que galopan, crin y cola al viento),
// jinete con corona, manto blanco al viento y un ARCO.
//  ARCO     : rafaga de flechas apuntadas al jugador (aviso: cinta roja + "!"), 4..6 flechas.
//  LLUVIA   : zonas marcadas en el suelo (anillo que se cierra) y una flecha gigante cae encima.
//  TROMPETA : aviso + temblor; si te alcanza quedas CONFUNDIDO (no te mueves ni disparas, la
//             pantalla ondula y recibes dano leve) hasta mover el analogo (BxMash). Si le
//             disparas durante el aviso, lo interrumpes.
// Vida 100. Al morir cae del cielo y se deshace en luz -> bw_Done() = 1.
//
// Contrato en boss_api.h. Todo se dibuja sin cull, color por vertice, sin texturas.
// NOTA: lvDrawAt se llama SIEMPRE con los 9 argumentos (el valor por defecto de "roll" se
// declara despues de este archivo).
// ============================================================================
#pragma once
#include <string.h>

static const float BW_PI  = 3.14159265f;
static const float BW_TAU = 6.2831853f;
static const float BW_R   = 18.0f;        // radio de la plataforma
static const float BW_S   = 1.4f;         // escala del caballo
static const float BW_BY  = 3.4f;         // altura del centro del torso en el modelo (origen de giro)
static const int   BW_HP  = 100;
static const float BW_MASH_NEED = 6.0f;   // recorrido del analogo para despejarse
#define BW_NARR  16
#define BW_NZONE 12

// paleta
static const unsigned int BW_COAT  = RGBA(230, 228, 222, 255);
static const unsigned int BW_COATD = RGBA(200, 198, 196, 255);
static const unsigned int BW_GOLD  = RGBA(236, 192,  88, 255);
static const unsigned int BW_GOLDL = RGBA(255, 228, 146, 255);
static const unsigned int BW_SKIN  = RGBA(240, 212, 182, 255);
static const unsigned int BW_CLOTH = RGBA(244, 242, 238, 255);

// ---------------------------------------------------------------- estado
struct BwArrow { float x, y, z, vx, vy, vz; int life; };
struct BwZone  { float x, z; int t; int hit; };
static BwArrow bwArr[BW_NARR];
static BwZone  bwZ[BW_NZONE];
static int     bwNZ = 0;

static int   bwT = 0, bwAnim = 0, bwHp = BW_HP, bwState = 0, bwDeadT = 0, bwHurt = 0, bwIFr = 0;
static int   bwAtk = 0, bwAtkT = 0, bwAtkCd = 0, bwLastAtk = 0, bwStun = 0, bwConfCool = 0;
static int   bwConf = 0, bwBlast = 0, bwTrumpDmg = 0;
static BxMash bwMash;
static float bwAng = 0.0f, bwDir = 1.0f, bwGal = 0.0f, bwPull = 0.0f, bwFallV = 0.0f;
static float bwCX = 0.0f, bwCY = 40.0f, bwCZ = -80.0f;           // centro del torso del caballo
static float bwVx = 0.0f, bwVy = 0.0f, bwVz = 0.0f;
static float bwYaw = 0.0f, bwPitch = 0.0f, bwRoll = 0.0f, bwScale = BW_S;
static float bwShakeAmp = 0.0f;
static float bwTrail[8][3];
static int   bwTrHead = 0, bwTrN = 0;
static float bwLastPx = 0.0f, bwLastPz = 0.0f, bwLastHp = 100.0f;
static unsigned int bwSeed = 0x1234ABCDu;
static int   g_bwBuilt = 0, g_bwBodyN = 0, g_bwArrowN = 0;

static LineVertex __attribute__((aligned(16))) g_bwBody[4600];
static LineVertex __attribute__((aligned(16))) g_bwArrowV[160];
static LineVertex __attribute__((aligned(16))) g_bwDyn[3200];

// ---------------------------------------------------------------- azar
static float bwRand() { bwSeed = bwSeed * 1664525u + 1013904223u; return (float)((bwSeed >> 8) & 0xFFFF) / 65535.0f; }
static float bwRnd(int a, int b) {
    unsigned int h = (unsigned int)a * 374761393u + (unsigned int)b * 668265263u + 0x9E3779B9u;
    h = (h ^ (h >> 13)) * 1274126177u; h ^= (h >> 16);
    return (float)(h & 0xFFFF) / 65535.0f;
}
static int bwPhase() { return bwHp > 66 ? 0 : (bwHp > 33 ? 1 : 2); }

// ---------------------------------------------------------------- transformacion del caballo
// modelo local (mira a -Z, centro del torso en y = BW_BY) -> mundo. Igual que bwDrawModel.
static void bwXform(float lx, float ly, float lz, float &ox, float &oy, float &oz) {
    const float x = lx * bwScale, y = (ly - BW_BY) * bwScale, z = lz * bwScale;
    const float cr = cosf(bwRoll), sr = sinf(bwRoll), cp = cosf(bwPitch), sp = sinf(bwPitch);
    const float cw = cosf(bwYaw), sw = sinf(bwYaw);
    const float x1 = x * cr - y * sr, y1 = x * sr + y * cr;
    const float y2 = y1 * cp - z * sp, z2 = y1 * sp + z * cp;
    ox = bwCX + x1 * cw + z2 * sw;
    oz = bwCZ - x1 * sw + z2 * cw;
    oy = bwCY + y2;
}
static void bwDrawModel(LineVertex *v, int n) {
    if (n <= 0) return;
    sceGumLoadIdentity();
    ScePspFVector3 t = { bwCX, bwCY, bwCZ }; sceGumTranslate(&t);
    sceGumRotateY(bwYaw); sceGumRotateX(bwPitch); sceGumRotateZ(bwRoll);
    ScePspFVector3 s = { bwScale, bwScale, bwScale }; sceGumScale(&s);
    ScePspFVector3 o = { 0.0f, -BW_BY, 0.0f }; sceGumTranslate(&o);
    sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, n, 0, v);
}

// ================================================================ MODELOS
// limbo con tope de vertices (para la parte dinamica)
static void bwLimbS(LineVertex *v, int &i, int cap, float x0, float y0, float z0, float x1, float y1, float z1,
                    float r0, float r1, int sides, unsigned int c0, unsigned int c1) {
    if (i + sides * 12 > cap) return;
    addLimb(v, i, x0, y0, z0, x1, y1, z1, r0, r1, sides, c0, c1);
}

// ---- parte ESTATICA: tronco, cuello, cabeza, jinete (sin brazos), corona, carcaj ----
static int bwBuildBody(LineVertex *b) {
    int i = 0;
    // caballo
    addBall(b, i, 0.0f, 3.5f, 0.15f, 1.0f, 1.12f, 2.25f, 6, 10, BW_COAT);          // barril
    addBall(b, i, 0.0f, 3.6f, -1.5f, 0.96f, 1.12f, 1.05f, 5, 9, BW_COAT);          // pecho
    addBall(b, i, 0.0f, 3.62f, 1.8f, 0.98f, 1.08f, 1.1f, 5, 9, BW_COAT);           // grupa
    addLimb(b, i, 0.0f, 4.0f, -1.9f, 0.0f, 5.8f, -3.35f, 0.74f, 0.42f, 9, BW_COAT, BW_COAT);   // cuello
    addLimb(b, i, 0.0f, 6.0f, -3.5f, 0.0f, 5.2f, -4.5f, 0.44f, 0.27f, 8, BW_COAT, RGBA(226, 206, 200, 255));   // cabeza
    addBall(b, i, 0.0f, 5.6f, -3.95f, 0.42f, 0.5f, 0.5f, 4, 8, BW_COAT);           // quijada
    addBall(b, i, 0.0f, 5.15f, -4.55f, 0.28f, 0.24f, 0.3f, 3, 6, RGBA(222, 200, 196, 255));    // hocico
    addBall(b, i, -0.4f, 5.95f, -3.75f, 0.1f, 0.1f, 0.1f, 3, 5, BW_GOLDL);         // ojos
    addBall(b, i,  0.4f, 5.95f, -3.75f, 0.1f, 0.1f, 0.1f, 3, 5, BW_GOLDL);
    addLimb(b, i, -0.2f, 6.15f, -3.4f, -0.24f, 6.8f, -3.3f, 0.13f, 0.02f, 4, BW_COAT, BW_COAT);   // orejas
    addLimb(b, i,  0.2f, 6.15f, -3.4f,  0.24f, 6.8f, -3.3f, 0.13f, 0.02f, 4, BW_COAT, BW_COAT);
    addLimb(b, i, -0.41f, 5.5f, -4.0f, -0.43f, 6.0f, -3.6f, 0.04f, 0.04f, 4, BW_GOLD, BW_GOLD);  // cabezada dorada
    addLimb(b, i,  0.41f, 5.5f, -4.0f,  0.43f, 6.0f, -3.6f, 0.04f, 0.04f, 4, BW_GOLD, BW_GOLD);
    addLimb(b, i, -0.42f, 6.02f, -3.55f, 0.42f, 6.02f, -3.55f, 0.05f, 0.05f, 4, BW_GOLD, BW_GOLD);
    addBall(b, i, 0.0f, 4.55f, 0.1f, 1.12f, 0.18f, 1.05f, 3, 8, BW_GOLD);          // manta de la silla
    // jinete: torso, manto, cabeza con corona
    const float ys[4] = { 4.6f, 5.25f, 6.0f, 6.7f }, rs[4] = { 0.66f, 0.5f, 0.56f, 0.42f };
    addLoft(b, i, 0.0f, -0.1f, ys, rs, 4, 8, RGBA(226, 224, 218, 255), RGBA(250, 248, 244, 255));
    addLimb(b, i, 0.0f, 5.0f, -0.1f, 0.0f, 5.15f, -0.1f, 0.58f, 0.58f, 8, BW_GOLD, BW_GOLD);     // faja
    addBall(b, i, 0.0f, 6.35f, -0.52f, 0.1f, 0.1f, 0.1f, 3, 5, BW_GOLDL);          // broche
    addBall(b, i, -0.5f, 6.4f, -0.1f, 0.27f, 0.22f, 0.27f, 3, 6, BW_CLOTH);        // hombros
    addBall(b, i,  0.5f, 6.4f, -0.1f, 0.27f, 0.22f, 0.27f, 3, 6, BW_CLOTH);
    addLimb(b, i, 0.0f, 6.6f, -0.15f, 0.0f, 6.95f, -0.3f, 0.17f, 0.15f, 6, BW_SKIN, BW_SKIN);   // cuello
    addBall(b, i, 0.0f, 7.3f, -0.35f, 0.33f, 0.4f, 0.34f, 5, 8, BW_SKIN);          // cabeza
    addBall(b, i, 0.0f, 7.38f, -0.12f, 0.31f, 0.33f, 0.28f, 4, 7, RGBA(232, 200, 120, 255));    // pelo
    addBall(b, i, -0.12f, 7.33f, -0.66f, 0.045f, 0.05f, 0.04f, 2, 4, RGBA(60, 70, 110, 255));   // ojos
    addBall(b, i,  0.12f, 7.33f, -0.66f, 0.045f, 0.05f, 0.04f, 2, 4, RGBA(60, 70, 110, 255));
    addLimb(b, i, 0.0f, 7.52f, -0.35f, 0.0f, 7.68f, -0.35f, 0.36f, 0.36f, 8, BW_GOLD, BW_GOLDL);   // CORONA: aro
    for (int k = 0; k < 7; ++k) {                                                                   // y puntas
        const float a = BW_TAU * k / 7.0f, bx = sinf(a) * 0.34f, bz = -0.35f + cosf(a) * 0.34f;
        const float h = (k & 1) ? 0.3f : 0.44f;
        addLimb(b, i, bx, 7.66f, bz, bx, 7.66f + h, bz, 0.085f, 0.012f, 4, BW_GOLD, BW_GOLDL);
    }
    addBall(b, i, 0.0f, 7.6f, -0.71f, 0.06f, 0.06f, 0.06f, 2, 4, RGBA(220, 50, 60, 255));         // joya
    for (int s = -1; s <= 1; s += 2) {                                                            // piernas
        addLimb(b, i, s * 0.42f, 4.78f, -0.1f, s * 0.8f, 4.28f, -0.8f, 0.27f, 0.2f, 6, BW_CLOTH, BW_CLOTH);
        addLimb(b, i, s * 0.8f, 4.28f, -0.8f, s * 0.88f, 3.5f, -0.55f, 0.2f, 0.15f, 6, BW_CLOTH, BW_GOLD);
    }
    addLimb(b, i, 0.32f, 5.9f, 0.38f, 0.52f, 6.95f, 0.62f, 0.21f, 0.19f, 6, RGBA(196, 146, 72, 255), RGBA(214, 170, 96, 255));   // carcaj
    for (int j = 0; j < 3; ++j)
        addLimb(b, i, 0.42f + 0.1f * j, 6.95f, 0.58f + 0.04f * j, 0.34f + 0.1f * j, 7.5f, 0.52f + 0.04f * j, 0.035f, 0.035f, 3, BW_GOLDL, BW_CLOTH);
    return i;
}

// flecha: mira a -Z, centro en el origen (3.5 de largo)
static int bwBuildArrow(LineVertex *b) {
    int i = 0;
    addLimb(b, i, 0.0f, 0.0f, 1.5f, 0.0f, 0.0f, -1.3f, 0.1f, 0.1f, 4, RGBA(255, 238, 170, 255), RGBA(255, 246, 204, 255));
    addLimb(b, i, 0.0f, 0.0f, -1.2f, 0.0f, 0.0f, -2.0f, 0.3f, 0.01f, 4, RGBA(255, 255, 255, 255), RGBA(255, 250, 232, 255));
    for (int k = 0; k < 3; ++k) {
        const float a = k * 2.0944f, fx = sinf(a) * 0.5f, fy = cosf(a) * 0.5f;
        swQuadC(b, i, 0.0f, 0.0f, 1.5f, RGBA(240, 240, 240, 255), fx, fy, 1.6f, RGBA(255, 255, 255, 255),
                      fx, fy, 0.8f, RGBA(255, 255, 255, 255), 0.0f, 0.0f, 0.9f, RGBA(236, 236, 236, 255));
    }
    return i;
}

// ---- parte DINAMICA (cada frame): patas, crin, cola, manto, brazos, arco o trompeta ----
static void bwLeg(LineVertex *v, int &i, int cap, float hx, float hz, float ph, int front) {
    const float hy = 3.2f, L1 = 1.55f, L2 = 1.55f;
    const float a = front ? 0.12f + 0.85f * sinf(ph) : -0.18f + 0.8f * sinf(ph);          // giro del muslo (+ = adelante)
    const float bend = 0.18f + 0.95f * (0.5f + 0.5f * sinf(ph + (front ? 1.7f : 2.3f)));    // rodilla / corvejon
    const float b = front ? a - bend : a - bend - 0.25f;
    const float ky = hy - L1 * cosf(a), kz = hz - L1 * sinf(a);
    const float fy = ky - L2 * cosf(b), fz = kz - L2 * sinf(b);
    const float c = b + 0.3f;
    const float py2 = fy - 0.45f * cosf(c), pz2 = fz - 0.45f * sinf(c);
    bwLimbS(v, i, cap, hx, hy, hz, hx, ky, kz, 0.38f, 0.24f, 6, BW_COAT, BW_COAT);
    bwLimbS(v, i, cap, hx, ky, kz, hx, fy, fz, 0.23f, 0.15f, 6, BW_COAT, BW_COATD);
    bwLimbS(v, i, cap, hx, fy, fz, hx, py2, pz2, 0.17f, 0.21f, 6, BW_GOLD, BW_GOLDL);       // casco dorado
}
static void bwMane(LineVertex *v, int &i, int cap, float tt) {
    for (int k = 0; k < 7; ++k) {
        const float u = 0.08f + k * 0.14f;
        const float ay = 4.0f + 1.8f * u, az = -1.9f - 1.45f * u, rad = 0.74f - 0.32f * u;
        const float bx = 0.0f, by = ay + rad * 0.85f, bz = az + 0.12f;
        const float w = tt * 7.0f - k * 0.8f;
        const float tx = bx + 0.14f * sinf(w * 0.7f), ty = by - 0.1f + 0.22f * sinf(w), tz = bz + 0.95f + 0.28f * sinf(w * 0.9f + 1.0f);
        bwLimbS(v, i, cap, bx, by, bz, tx, ty, tz, 0.16f, 0.03f, 4, BW_COAT, lvMix(BW_COAT, BW_GOLDL, 0.6f));
    }
}
static void bwTail(LineVertex *v, int &i, int cap, float tt) {
    for (int j = -1; j <= 1; ++j) {
        const float rx = j * 0.12f, ry = 4.15f, rz = 2.75f, w = tt * 6.0f + j * 1.3f;
        const float mx = rx + j * 0.22f + 0.18f * sinf(w), my = ry - 0.45f + 0.12f * sinf(w + 1.0f), mz = rz + 1.2f;
        const float ex = mx + j * 0.2f + 0.4f * sinf(w * 0.8f + 0.7f), ey = my - 1.0f + 0.25f * sinf(w + 2.0f), ez = mz + 1.3f + 0.3f * sinf(w);
        bwLimbS(v, i, cap, rx, ry, rz, mx, my, mz, 0.2f, 0.15f, 5, BW_COAT, BW_COAT);
        bwLimbS(v, i, cap, mx, my, mz, ex, ey, ez, 0.15f, 0.03f, 5, BW_COAT, lvMix(BW_COAT, BW_GOLDL, 0.5f));
    }
}
// manto blanco al viento, dos paneles con pliegue central, del blanco al dorado
static void bwCloak(LineVertex *v, int &i, int cap, float tt) {
    float px[6], py[6], pz[6], hw[6];
    for (int j = 0; j < 6; ++j) {
        const float f = (float)j;
        pz[j] = 0.1f + f * 0.85f;
        py[j] = 6.45f - f * 0.38f + 0.22f * sinf(tt * 6.0f - f * 0.9f) * (0.3f + f * 0.2f);
        px[j] = 0.28f * sinf(tt * 4.5f - f * 0.8f) * f * 0.25f;
        hw[j] = 0.5f + f * 0.13f;
    }
    for (int j = 0; j < 5; ++j) {
        if (i + 12 > cap) return;
        const float sh = 0.84f + 0.24f * sinf(tt * 6.0f - j * 0.9f + 1.0f);
        const unsigned int ca = lvMix(BW_CLOTH, BW_GOLDL, j / 5.0f), cb = lvMix(BW_CLOTH, BW_GOLDL, (j + 1) / 5.0f);
        const unsigned int la = brighten(ca, sh * 0.92f), lb = brighten(cb, sh * 0.92f), ra = brighten(ca, sh * 1.06f), rb = brighten(cb, sh * 1.06f);
        const float s0 = 0.18f, s1 = 0.18f;
        swQuadC(v, i, px[j] - hw[j], py[j], pz[j], la,  px[j], py[j] - s0, pz[j], ca,
                      px[j + 1], py[j + 1] - s1, pz[j + 1], cb,  px[j + 1] - hw[j + 1], py[j + 1], pz[j + 1], lb);
        swQuadC(v, i, px[j], py[j] - s0, pz[j], ca,  px[j] + hw[j], py[j], pz[j], ra,
                      px[j + 1] + hw[j + 1], py[j + 1], pz[j + 1], rb,  px[j + 1], py[j + 1] - s1, pz[j + 1], cb);
    }
}
// brazo con codo (cinematica de 2 huesos): el codo cae hacia el vector "polo" (ox,oy,oz)
static void bwArm(LineVertex *v, int &i, int cap, float sx, float sy, float sz, float hx, float hy, float hz,
                  float ox, float oy, float oz) {
    float dx = hx - sx, dy = hy - sy, dz = hz - sz;
    float d = sqrtf(dx * dx + dy * dy + dz * dz); if (d < 0.01f) d = 0.01f;
    const float nx = dx / d, ny = dy / d, nz = dz / d;
    const float dot = ox * nx + oy * ny + oz * nz;
    float qx = ox - nx * dot, qy = oy - ny * dot, qz = oz - nz * dot;
    float ql = sqrtf(qx * qx + qy * qy + qz * qz); if (ql < 0.001f) { qx = 0.0f; qy = 1.0f; qz = 0.0f; ql = 1.0f; }
    qx /= ql; qy /= ql; qz /= ql;
    const float L = 1.3f, a = d * 0.5f, h2 = L * L - a * a, hh = h2 > 0.0f ? sqrtf(h2) : 0.0f;
    const float ex = (sx + hx) * 0.5f + qx * hh, ey = (sy + hy) * 0.5f + qy * hh, ez = (sz + hz) * 0.5f + qz * hh;
    bwLimbS(v, i, cap, sx, sy, sz, ex, ey, ez, 0.21f, 0.17f, 6, BW_CLOTH, BW_CLOTH);
    bwLimbS(v, i, cap, ex, ey, ez, hx, hy, hz, 0.17f, 0.12f, 6, BW_CLOTH, BW_SKIN);
    if (i + 90 <= cap) addBall(v, i, hx, hy, hz, 0.13f, 0.13f, 0.13f, 3, 5, BW_SKIN);
}
// mode 0 = arco (pull 0..1 = cuanto tensa la cuerda), mode 1 = trompeta (blast 0..1 = estruendo)
static int bwBuildDyn(LineVertex *v, int cap, float tt, float gal, int mode, float pull, float blast) {
    int i = 0;
    bwLeg(v, i, cap, -0.52f, -1.55f, gal, 1);
    bwLeg(v, i, cap,  0.52f, -1.55f, gal + 0.55f, 1);
    bwLeg(v, i, cap, -0.56f,  1.7f, gal + 2.6f, 0);
    bwLeg(v, i, cap,  0.56f,  1.7f, gal + 3.2f, 0);
    bwMane(v, i, cap, tt);
    bwTail(v, i, cap, tt);
    bwCloak(v, i, cap, tt);
    if (mode == 1) {                                         // TROMPETA
        const float m0x = 0.05f, m0y = 7.05f, m0z = -0.7f;
        const float p1x = -0.12f, p1y = 7.42f, p1z = -3.0f, p2x = -0.18f, p2y = 7.7f, p2z = -4.4f;
        bwLimbS(v, i, cap, m0x, m0y, m0z, p1x, p1y, p1z, 0.07f, 0.1f, 6, BW_GOLD, BW_GOLD);
        bwLimbS(v, i, cap, p1x, p1y, p1z, p2x, p2y, p2z, 0.1f, 0.58f + 0.12f * blast, 8, BW_GOLD, BW_GOLDL);   // pabellon
        if (blast > 0.01f && i + 144 <= cap)
            addBall(v, i, p2x, p2y, p2z - 0.15f, 0.2f + 0.45f * blast, 0.2f + 0.45f * blast, 0.12f, 4, 8, RGBA(255, 250, 210, 255));
        bwArm(v, i, cap,  0.55f, 6.35f, -0.2f,  0.11f, 6.95f, -0.9f,  0.5f, -0.2f, 0.4f);
        bwArm(v, i, cap, -0.55f, 6.35f, -0.2f, -0.1f, 7.2f, -1.9f, -0.5f, -0.2f, 0.3f);
    } else {                                                 // ARCO
        const float bx = -1.05f;                              // el arco, a la izquierda, fuera del cuello
        float py[7], pz[7];
        if (pull < 0.0f) pull = 0.0f; if (pull > 1.0f) pull = 1.0f;
        for (int q = 0; q < 7; ++q) {
            const float f = (q - 3) / 3.0f;
            py[q] = 6.15f + f * 1.75f;
            pz[q] = -2.0f - 0.78f * (1.0f - f * f) + 0.38f * pull * f * f;
        }
        for (int q = 0; q < 6; ++q) {
            const float f0 = (q - 3) / 3.0f, f1 = (q + 1 - 3) / 3.0f;
            bwLimbS(v, i, cap, bx, py[q], pz[q], bx, py[q + 1], pz[q + 1],
                    0.05f + 0.065f * (1.0f - f0 * f0), 0.05f + 0.065f * (1.0f - f1 * f1), 5,
                    (q == 0) ? BW_GOLDL : BW_GOLD, (q == 5) ? BW_GOLDL : BW_GOLD);
        }
        const float zM = pz[0] + 1.25f * pull;                // la mano de la cuerda
        bwLimbS(v, i, cap, bx, py[0], pz[0], bx, 6.15f, zM, 0.025f, 0.025f, 3, BW_CLOTH, BW_CLOTH);
        bwLimbS(v, i, cap, bx, 6.15f, zM, bx, py[6], pz[6], 0.025f, 0.025f, 3, BW_CLOTH, BW_CLOTH);
        if (pull > 0.04f) {                                   // flecha en la cuerda
            bwLimbS(v, i, cap, bx, 6.15f, zM, bx, 6.15f, zM - 3.3f, 0.05f, 0.05f, 4, BW_GOLD, BW_GOLDL);
            bwLimbS(v, i, cap, bx, 6.15f, zM - 2.75f, bx, 6.15f, zM - 3.4f, 0.15f, 0.01f, 4, BW_CLOTH, BW_CLOTH);
            bwLimbS(v, i, cap, bx, 6.15f, zM - 0.05f, bx, 6.15f, zM + 0.4f, 0.1f, 0.08f, 4, BW_CLOTH, BW_CLOTH);
        }
        bwArm(v, i, cap, -0.55f, 6.35f, -0.2f, bx, 6.15f, pz[3] + 0.2f, -0.3f, -0.3f, 0.2f);    // brazo del arco
        bwArm(v, i, cap,  0.55f, 6.35f, -0.2f, bx, 6.15f, zM, 0.5f, 0.35f, 0.8f);               // brazo que tensa
    }
    return i;
}

// ================================================================ ESCENARIO
static unsigned int bwLit(unsigned int c, float a, float k) { return brighten(c, 1.0f + k * cosf(a + 2.36f)); }

// banda anular (puede ser pared si r0 == r1) con la luz del sol arriba a la izquierda
static void bwBand(LineVertex *g, int &n, int cap, int N, float r0, float r1, float y0, float y1,
                   unsigned int c0, unsigned int c1, float lk) {
    for (int k = 0; k < N; ++k) {
        if (n + 6 > cap) return;
        const float a0 = BW_TAU * k / N, a1 = BW_TAU * (k + 1) / N;
        const float s0 = sinf(a0), q0 = cosf(a0), s1 = sinf(a1), q1 = cosf(a1);
        swQuadC(g, n, s0 * r0, y0, q0 * r0, bwLit(c0, a0, lk),  s0 * r1, y1, q0 * r1, bwLit(c1, a0, lk),
                      s1 * r1, y1, q1 * r1, bwLit(c1, a1, lk),  s1 * r0, y0, q1 * r0, bwLit(c0, a1, lk));
    }
}
static float bwUr(int ring, int k) {   // radio (con irregularidad) de los anillos de la roca de abajo
    static const float ur[4] = { 18.0f, 13.5f, 7.5f, 0.0f };
    if (ring == 0) return ur[0];
    if (ring == 3) return 0.0f;
    return ur[ring] * (0.86f + 0.28f * bwRnd(k, ring));
}
static void bwBuildStage(LineVertex *g, int cap, int &n) {
    const int N = 28;
    const unsigned int cC = RGBA(252, 250, 244, 255), c1 = RGBA(246, 243, 235, 255), c2 = RGBA(238, 234, 223, 255), c3 = RGBA(228, 222, 206, 255);
    // suelo: abanico central + 2 bandas
    for (int k = 0; k < N && n + 3 <= cap; ++k) {
        const float a0 = BW_TAU * k / N, a1 = BW_TAU * (k + 1) / N;
        g[n++] = { cC, 0.0f, 0.05f, 0.0f };
        g[n++] = { bwLit(c1, a0, 0.05f), sinf(a0) * 7.0f, 0.05f, cosf(a0) * 7.0f };
        g[n++] = { bwLit(c1, a1, 0.05f), sinf(a1) * 7.0f, 0.05f, cosf(a1) * 7.0f };
    }
    bwBand(g, n, cap, N, 7.0f, 13.0f, 0.05f, 0.05f, c1, c2, 0.05f);
    bwBand(g, n, cap, N, 13.0f, 17.2f, 0.05f, 0.05f, c2, c3, 0.05f);
    // dorados: borde, anillo interior, emblema central y 8 rayos
    bwBand(g, n, cap, N, 17.2f, 18.0f, 0.07f, 0.07f, BW_GOLD, BW_GOLDL, 0.05f);
    bwBand(g, n, cap, N, 11.6f, 12.0f, 0.06f, 0.06f, BW_GOLD, BW_GOLD, 0.05f);
    bwBand(g, n, cap, N, 2.2f, 2.6f, 0.06f, 0.06f, BW_GOLD, BW_GOLDL, 0.05f);
    for (int s = 0; s < 8 && n + 6 <= cap; ++s) {
        const float a = BW_TAU * s / 8.0f + 0.3927f, rx = sinf(a), rz = cosf(a), px = rz * 0.13f, pz = -rx * 0.13f;
        swQuadC(g, n, rx * 2.9f - px, 0.06f, rz * 2.9f - pz, BW_GOLD,  rx * 2.9f + px, 0.06f, rz * 2.9f + pz, BW_GOLD,
                      rx * 11.2f + px, 0.06f, rz * 11.2f + pz, BW_GOLDL,  rx * 11.2f - px, 0.06f, rz * 11.2f - pz, BW_GOLDL);
    }
    // pared del borde
    bwBand(g, n, cap, N, 18.0f, 18.0f, 0.0f, -1.8f, RGBA(226, 220, 204, 255), RGBA(176, 174, 176, 255), 0.10f);
    // roca flotante de abajo: 3 anillos irregulares que se cierran en punta, azulando hacia el cielo
    const float uy[4] = { -1.8f, -5.4f, -9.0f, -14.5f };
    const unsigned int uc[4] = { RGBA(190, 188, 190, 255), RGBA(166, 168, 180, 255), RGBA(176, 186, 206, 255), RGBA(214, 226, 246, 255) };
    for (int j = 0; j < 3; ++j) {
        for (int k = 0; k < N; ++k) {
            if (n + 6 > cap) return;
            const int k1 = (k + 1) % N;
            const float a0 = BW_TAU * k / N, a1 = BW_TAU * (k + 1) / N;
            const float ra0 = bwUr(j, k), ra1 = bwUr(j, k1), rb0 = bwUr(j + 1, k), rb1 = bwUr(j + 1, k1);
            swQuadC(g, n, sinf(a0) * ra0, uy[j], cosf(a0) * ra0, bwLit(uc[j], a0, 0.10f),
                          sinf(a1) * ra1, uy[j], cosf(a1) * ra1, bwLit(uc[j], a1, 0.10f),
                          sinf(a1) * rb1, uy[j + 1], cosf(a1) * rb1, bwLit(uc[j + 1], a1, 0.10f),
                          sinf(a0) * rb0, uy[j + 1], cosf(a0) * rb0, bwLit(uc[j + 1], a0, 0.10f));
        }
    }
}
// suelo (franjas en z que aproximan el circulo) y columnas rotas (decoracion + colision)
static void bwBuildColliders(CityBldg *bx, int &nb, int boxCap) {
    for (int s = 0; s < 18 && nb < boxCap; ++s) {
        const float zc = -17.0f + 2.0f * s;
        float hw = BW_R * BW_R - zc * zc; hw = hw > 1.0f ? sqrtf(hw) : 1.0f;
        bx[nb++] = { 0.0f, zc, hw * 2.0f + 0.6f, 2.0f, 0.0f, 0 };
    }
}
static void bwBuildColumns(LineVertex *g, int cap, int &n, CityBldg *bx, int &nb, int boxCap) {
    // el arco de adelante (donde estan el spawn, el guardado y el elevador) queda libre
    static const float kAng[7] = { 70.0f, 106.7f, 143.3f, 180.0f, 216.7f, 253.3f, 290.0f };
    static const float kH[7]   = { 3.4f, 1.3f, 2.8f, 3.8f, 1.1f, 2.4f, 3.0f };
    const unsigned int cm = RGBA(226, 222, 212, 255), cw = RGBA(248, 246, 240, 255), cp = RGBA(214, 208, 192, 255);
    for (int k = 0; k < 7; ++k) {
        if (n + 520 > cap) break;
        const float a = kAng[k] * 0.0174533f, rad = (k & 1) ? 12.6f : 14.4f, h = kH[k];
        const float x = sinf(a) * rad, z = cosf(a) * rad;
        addLimb(g, n, x, 0.0f, z, x, 0.38f, z, 1.0f, 0.9f, 8, cp, RGBA(230, 224, 208, 255));          // plinto
        addLimb(g, n, x, 0.38f, z, x + ((k & 1) ? 0.0f : 0.1f), h, z, 0.64f, 0.56f, 8, cm, cw);      // fuste
        addLimb(g, n, x, 0.55f, z, x, 0.75f, z, 0.67f, 0.66f, 8, BW_GOLD, BW_GOLD);                   // banda dorada
        if (k == 3) addLimb(g, n, x, h, z, x, h + 0.3f, z, 0.95f, 0.8f, 8, BW_GOLD, BW_GOLDL);        // la unica con capitel
        else if (h > 2.2f) addLimb(g, n, x + 0.15f, h, z, x + 0.32f, h + 0.38f, z - 0.1f, 0.34f, 0.05f, 5, cm, cw);   // fractura
        swBoxL(g, n, x + sinf(a + 0.5f) * 1.5f, 0.0f, z + cosf(a + 0.5f) * 1.5f, 0.7f, 0.6f, 0.45f, cp);   // escombro
        if (k == 1 || k == 4) addLimb(g, n, x + 1.4f, 0.5f, z - 0.6f, x + 2.9f, 0.5f, z - 0.2f, 0.5f, 0.5f, 8, cm, cw);   // tambor caido
        if (nb < boxCap) bx[nb++] = { x, z, 1.6f, 1.6f, h, cm };
    }
}

// ================================================================ ATAQUES
static void bwClearShots() {
    for (int k = 0; k < BW_NARR; ++k) bwArr[k].life = 0;
    bwNZ = 0;
}
static void bwPlayerReset() {   // el jugador reaparecio: nada de flechas ni confusion colgando
    bwClearShots();
    bwAtk = 0; bwAtkT = 0; bwAtkCd = 150; bwConf = 0; bwMash.frames = 0; bwPull = 0.0f; bwBlast = 0;
}
static void bwShoot(BossIO &io, float lateral) {
    float ox, oy, oz; bwXform(-1.05f, 6.15f, -3.3f, ox, oy, oz);     // la mano del arco
    const float spd = 1.0f + 0.1f * bwPhase();
    float dx = io.px + lateral - ox, dy = io.py + 1.7f - oy, dz = io.pz + lateral * 0.4f - oz;
    const float d = sqrtf(dx * dx + dy * dy + dz * dz) + 0.001f;
    for (int k = 0; k < BW_NARR; ++k) if (bwArr[k].life <= 0) {
        bwArr[k].x = ox; bwArr[k].y = oy; bwArr[k].z = oz;
        bwArr[k].vx = dx / d * spd; bwArr[k].vy = dy / d * spd; bwArr[k].vz = dz / d * spd;
        bwArr[k].life = 260;
        return;
    }
}
static void bwEndAttack() {
    const int ph = bwPhase();
    bwAtk = 0; bwAtkT = 0;
    bwAtkCd = (ph == 0 ? 170 : (ph == 1 ? 130 : 95)) + (int)(bwRand() * 50.0f);
}
static void bwStartAttack(BossIO &io) {
    const int ph = bwPhase();
    int pick;
    const int r = (int)(bwRand() * 100.0f);
    if (bwLastAtk == 0) pick = 1;                                   // el primero ensenia: flechas
    else if (bwConfCool <= 0 && r < 24) pick = 3;
    else pick = (r < 62) ? 1 : 2;
    if (pick == bwLastAtk && pick != 3) pick = (pick == 1) ? 2 : 1; // no repite
    bwAtk = pick; bwAtkT = 0; bwLastAtk = pick; bwTrumpDmg = 0;
    if (pick == 2) {                                                // lluvia: la primera cae sobre ti
        bwNZ = 6 + 3 * ph;
        for (int k = 0; k < bwNZ; ++k) {
            float x = io.px, z = io.pz;
            if (k > 0) { x += (bwRand() - 0.5f) * 20.0f; z += (bwRand() - 0.5f) * 20.0f; }
            const float d = sqrtf(x * x + z * z);
            if (d > 16.0f) { x *= 16.0f / d; z *= 16.0f / d; }
            bwZ[k].x = x; bwZ[k].z = z; bwZ[k].t = -k * 6; bwZ[k].hit = 0;
        }
        lvMessage("¡LLUVIA DE FLECHAS!", 100);
    } else if (pick == 1) {
        lvMessage("¡FLECHAS!", 80);
    } else {
        bwConfCool = 300;
        lvMessage("¡LA TROMPETA!", 90);
    }
}
static void bwConfStep(BossIO &io, int hurt) {
    if (hurt) io.hp -= 0.05f;                                       // dano leve por segundo (~3)
    const int r = bxMashUpdate(bwMash, io.stickX, io.stickY, BW_MASH_NEED);
    // ondulacion de la camara (el yaw oscila y vuelve: no se acumula)
    io.camYaw += 0.035f * (sinf(g_lvT * 0.17f) - sinf((g_lvT - 1) * 0.17f)) * 3.0f;
    if (r == 1) { bwConf = 0; bwStun = 110; bwConfCool = 420; lvMessage("TE DESPEJASTE", 70); }
    else if (r == -1) { bwConf = 0; io.hp -= 6.0f; bwStun = 60; bwConfCool = 420; lvMessage("LA CONFUSIÓN PASA", 70); }
}
static void bwAttackStep(BossIO &io) {
    const int ph = bwPhase();
    ++bwAtkT;
    if (bwAtk == 1) {                                               // ARCO: aviso 48f + rafaga
        const int N = 4 + ph, iv = 12 - 2 * ph, T0 = 48, e = bwAtkT - T0;
        if (e < 0) bwPull = bwAtkT / (float)T0;
        else {
            bwPull = (float)(e % iv) / (float)iv;
            if (e % iv == 0 && e / iv < N) {
                bwShoot(io, 0.0f);
                if (ph >= 2) bwShoot(io, (e / iv & 1) ? 3.0f : -3.0f);
                bwPull = 0.0f;
            }
            if (e > N * iv + 10) bwEndAttack();
        }
    } else if (bwAtk == 2) {                                        // LLUVIA: cada zona cierra su anillo y cae la flecha
        bwPull += (0.7f - bwPull) * 0.1f;
        int alive = 0;
        for (int k = 0; k < bwNZ; ++k) {
            BwZone &z = bwZ[k];
            ++z.t;
            if (z.t == 78 && !z.hit) {
                z.hit = 1;
                const float dx = io.px - z.x, dz = io.pz - z.z;
                if (dx * dx + dz * dz < 2.7f * 2.7f && io.py < 3.2f) { io.hp -= 14.0f; g_lvFlash = 8; }
            }
            if (z.t < 96) ++alive;
        }
        if (!alive) { bwNZ = 0; bwEndAttack(); }
    } else if (bwAtk == 3) {                                        // TROMPETA: aviso 70f, estruendo, confusion
        bwPull += (0.0f - bwPull) * 0.2f;
        if (bwAtkT == 70) {
            bwBlast = 30; g_lvFlash = 26;
            bwConf = 1; bxMashStart(bwMash, 360);
            lvMessage("¡CONFUNDIDO! MUEVE EL ANÁLOGO", 130);
        }
        if (bwAtkT >= 90) bwEndAttack();
    }
}

// ---------------------------------------------------------------- orbita del jinete
static void bwPlace(float spdMul, int init) {
    const int ph = bwPhase();
    bwAng += bwDir * (0.0115f + 0.0025f * ph) * spdMul;
    const float tt = bwAnim * 0.016f;
    float k = bwAnim / 160.0f; if (k > 1.0f) k = 1.0f; k = k * k * (3.0f - 2.0f * k);       // la entrada en picada
    float R = 30.0f + 3.0f * sinf(tt * 0.7f), cy = 8.0f + 2.3f * sinf(tt * 1.1f + 1.0f);
    R += (88.0f - R) * (1.0f - k);
    cy += (48.0f - cy) * (1.0f - k);
    const float a = bwAng - (1.0f - k) * 1.2f;
    const float nx = sinf(a) * R, nz = cosf(a) * R, ny = cy + 0.3f * sinf(bwGal * 2.0f);
    if (init) { bwVx = cosf(a) * 0.35f * bwDir; bwVz = -sinf(a) * 0.35f * bwDir; bwVy = 0.0f; }
    else { bwVx = nx - bwCX; bwVy = ny - bwCY; bwVz = nz - bwCZ; }
    bwCX = nx; bwCY = ny; bwCZ = nz;
    const float hv = sqrtf(bwVx * bwVx + bwVz * bwVz);
    if (hv > 0.02f) {
        const float ty = atan2f(-bwVx, -bwVz);
        float d = ty - bwYaw;
        while (d > BW_PI) d -= BW_TAU;
        while (d < -BW_PI) d += BW_TAU;
        bwYaw += init ? d : d * 0.12f;
        float tp = atan2f(bwVy, hv) * 0.8f;
        if (tp > 0.7f) tp = 0.7f;
        if (tp < -0.7f) tp = -0.7f;
        bwPitch += init ? (tp - bwPitch) : (tp - bwPitch) * 0.1f;
    }
    bwRoll = bwDir * 0.17f * (0.4f + 0.6f * k) + 0.04f * sinf(bwGal);
}
static void bwReset() {
    bwT = 0; bwAnim = 0; bwHp = BW_HP; bwState = 0; bwDeadT = 0; bwHurt = 0; bwIFr = 0;
    bwAtk = 0; bwAtkT = 0; bwAtkCd = 270; bwLastAtk = 0; bwStun = 0; bwConfCool = 0;
    bwConf = 0; bwBlast = 0; bwTrumpDmg = 0; bwMash.frames = 0; bwMash.acc = 0.0f; bwMash.lx = bwMash.ly = 0.0f;
    bwPull = 0.0f; bwFallV = 0.0f; bwShakeAmp = 0.0f; bwScale = BW_S; bwGal = 0.0f;
    bwAng = BW_PI + 0.6f; bwDir = 1.0f; bwYaw = 0.0f; bwPitch = 0.0f; bwRoll = 0.0f;
    bwTrHead = 0; bwTrN = 0; bwSeed = 0x1234ABCDu;
    bwLastPx = 0.0f; bwLastPz = 9.0f; bwLastHp = 100.0f;
    bwClearShots();
    bwCX = bwCY = bwCZ = 0.0f;
    bwPlace(1.0f, 1);
}

// ================================================================ API (contrato de boss_api.h)
static void bw_Enter(BossIO &io, LineVertex *geo, int geoCap, int *geoN, CityBldg *boxes, int *nBoxes, int boxCap,
                     float *sx, float *sy, float *sz) {
    (void)io;
    if (!g_bwBuilt) {
        g_bwBodyN = bwBuildBody(g_bwBody);
        g_bwArrowN = bwBuildArrow(g_bwArrowV);
        sceKernelDcacheWritebackAll();
        g_bwBuilt = 1;
    }
    int n = 0, nb = 0;
    bwBuildStage(geo, geoCap, n);
    bwBuildColliders(boxes, nb, boxCap);
    bwBuildColumns(geo, geoCap, n, boxes, nb, boxCap);
    *geoN = n; *nBoxes = nb;
    *sx = 0.0f; *sy = 0.0f; *sz = 9.0f;                 // el jugador de cara al cielo (-Z)
    bwReset();
}

static void bw_Update(BossIO &io) {
    ++bwT;
    if (bwHurt > 0) --bwHurt;
    if (bwIFr > 0) --bwIFr;
    if (bwBlast > 0) --bwBlast;
    if (bwState == 2) { bwShakeAmp = 0.0f; return; }
    // el jugador reaparecio (murio o cayo): se limpian flechas y confusion
    {
        const float jx = io.px - bwLastPx, jz = io.pz - bwLastPz;
        if (bwT > 3 && (jx * jx + jz * jz > 144.0f || io.hp > bwLastHp + 20.0f)) bwPlayerReset();
        bwLastPx = io.px; bwLastPz = io.pz; bwLastHp = io.hp;
    }
    if (bwT == 20) lvMessage("EL PRIMER JINETE: CONQUISTA", 170);

    // ---- tiempo detenido: todo congelado (salvo poder despejarse la confusion) ----
    if (g_lvTimeStop > 0) {
        if (bwConf && bwState == 0) bwConfStep(io, 0);
        return;
    }

    // ---- muriendo: cae del cielo y se deshace en luz ----
    if (bwState == 1) {
        ++bwDeadT; ++bwAnim;
        bwGal += 0.5f;
        bwFallV += 0.006f;
        bwCY -= bwFallV; bwCX += bwVx * 0.6f; bwCZ += bwVz * 0.6f;
        bwPitch += (-1.0f - bwPitch) * 0.03f;
        bwRoll += 0.045f;
        bwShakeAmp = (bwDeadT < 20) ? 0.12f : 0.03f;
        if (bwDeadT >= 100) { bwScale = BW_S * (1.0f - (bwDeadT - 100) / 32.0f); if (bwScale < 0.0f) bwScale = 0.0f; }
        if (bwDeadT == 118) { g_lvFlash = 45; bwShakeAmp = 0.3f; }
        if (bwDeadT >= 135) { bwState = 2; bwShakeAmp = 0.0f; }
        return;
    }

    // ---- vivo ----
    ++bwAnim;
    float spd = 1.0f;
    if (bwAtk == 3) spd = 0.3f; else if (bwConf) spd = 0.55f;
    bwGal += (bwAtk == 3) ? 0.07f : 0.2f;
    bwPlace(spd, 0);
    if ((bwAnim & 3) == 0) {                                          // estela de luz
        bwTrHead = (bwTrHead + 1) & 7;
        bwTrail[bwTrHead][0] = bwCX; bwTrail[bwTrHead][1] = bwCY; bwTrail[bwTrHead][2] = bwCZ;
        if (bwTrN < 8) ++bwTrN;
    }
    // con L la camara se fija en el jinete (orbita rapido)
    if (io.aiming && !bwConf) {
        float d = atan2f(bwCX - io.px, -(bwCZ - io.pz)) - io.camYaw;
        while (d > BW_PI) d -= BW_TAU;
        while (d < -BW_PI) d += BW_TAU;
        io.camYaw += d * 0.08f;
    }
    if (bwConfCool > 0) --bwConfCool;
    if (bwConf) bwConfStep(io, 1);

    // flechas en vuelo
    for (int k = 0; k < BW_NARR; ++k) {
        BwArrow &a = bwArr[k];
        if (a.life <= 0) continue;
        a.x += a.vx; a.y += a.vy; a.z += a.vz; --a.life;
        const float dx = a.x - io.px, dy = a.y - (io.py + 1.7f), dz = a.z - io.pz;
        if (dx * dx + dy * dy + dz * dz < 1.6f * 1.6f) { io.hp -= 7.0f; g_lvFlash = 6; a.life = 0; continue; }
        if (a.y < -0.2f && a.x * a.x + a.z * a.z < (BW_R + 1.0f) * (BW_R + 1.0f)) a.life = 0;   // se clava en el suelo
        else if (a.y < -40.0f) a.life = 0;
    }

    // ataques
    if (bwAtk == 0) {
        bwPull += (0.0f - bwPull) * 0.15f;
        if (!bwConf) {
            if (bwStun > 0) --bwStun;
            else if (bwT > 60 && --bwAtkCd <= 0) bwStartAttack(io);
        }
    } else bwAttackStep(io);

    // temblor de camara
    float sh = 0.0f;
    if (bwAtk == 3 && bwAtkT < 70) sh = 0.03f + 0.07f * bwAtkT / 70.0f;
    if (bwBlast > 0) { const float b = 0.32f * bwBlast / 30.0f; if (b > sh) sh = b; }
    if (bwConf && sh < 0.05f) sh = 0.05f;
    bwShakeAmp = sh;
}

// profundidad respecto a la camara. Un poligono que cruza el plano cercano o queda detras del ojo
// se rasteriza como un TRIANGULO GIGANTE: todo lo dinamico se omite o se recorta cerca del ojo.
static float bwDepth(float x, float y, float z) {
    return (x - g_lvEye[0]) * g_lvFwd[0] + (y - g_lvEye[1]) * g_lvFwd[1] + (z - g_lvEye[2]) * g_lvFwd[2];
}
// cinta (dos planos cruzados) de a a b, recortada contra el plano cercano; ca/cb = color de cada extremo
static void bwRibbon(float ax, float ay, float az, unsigned int ca, float bx, float by, float bz, unsigned int cb, float hw) {
    const float ND = 2.5f;
    const float da = bwDepth(ax, ay, az), db = bwDepth(bx, by, bz);
    if (da < ND && db < ND) return;
    if (da < ND) { const float t = (ND - da) / (db - da); ax += (bx - ax) * t; ay += (by - ay) * t; az += (bz - az) * t; }
    else if (db < ND) { const float t = (ND - db) / (da - db); bx += (ax - bx) * t; by += (ay - by) * t; bz += (az - bz) * t; }
    const float dx = bx - ax, dz = bz - az, ll = sqrtf(dx * dx + dz * dz);
    float lx = hw, lz = 0.0f;
    if (ll > 0.001f) { lx = -dz / ll * hw; lz = dx / ll * hw; }
    LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 12); int i = 0;
    swQuadC(v, i, ax - lx, ay, az - lz, ca, ax + lx, ay, az + lz, ca, bx + lx, by, bz + lz, cb, bx - lx, by, bz - lz, cb);
    swQuadC(v, i, ax, ay - hw, az, ca, ax, ay + hw, az, ca, bx, by + hw, bz, cb, bx, by - hw, bz, cb);
    sceGumLoadIdentity(); sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, i, 0, v);
}

static void bw_Draw(float px, float py, float pz) {
    sceGuDisable(GU_CULL_FACE);
    const float tt = bwAnim * 0.016f;
    // motas de luz dorada que suben alrededor de la plataforma
    for (int k = 0; k < 10; ++k) {
        const float a = k * 2.2f + g_lvT * 0.004f * (1.0f + (float)(k % 3)), r = 5.0f + 12.0f * bwRnd(k, 3);
        const float y = fmodf(g_lvT * 0.02f * (0.6f + bwRnd(k, 4)) + k * 1.7f, 11.0f) - 0.5f;
        const float mx = sinf(a) * r, mz = cosf(a) * r;
        if (bwDepth(mx, y, mz) < 3.0f) continue;
        lvBox(mx, y, mz, 0.2f, 0.2f, lvMix(BW_GOLD, BW_GOLDL, bwRnd(k, 5)));
    }
    if (bwState < 2) {
        bwDrawModel(g_bwBody, g_bwBodyN);
        const int n = bwBuildDyn(g_bwDyn, 3200, tt, bwGal, (bwAtk == 3) ? 1 : 0, bwPull, bwBlast > 0 ? bwBlast / 30.0f : 0.0f);
        if (n > 0) {
            LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * n);
            memcpy(v, g_bwDyn, sizeof(LineVertex) * n);
            bwDrawModel(v, n);
        }
    }
    // luz que se deshace al morir
    if (bwState == 1) {
        const float f = 1.0f - bwDeadT / 135.0f;
        for (int k = 0; k < 16; ++k) {
            const float a = k * 2.399f, up = bwRnd(k, 7) * 2.0f - 0.6f, r = 1.0f + bwDeadT * (0.08f + 0.1f * bwRnd(k, 8));
            const float s = 0.7f * f + 0.1f;
            const float mx = bwCX + sinf(a) * r, my = bwCY + up * r * 0.6f, mz = bwCZ + cosf(a) * r;
            if (bwDepth(mx, my, mz) < 3.0f) continue;
            lvBox(mx, my, mz, s, s, lvMix(BW_GOLDL, RGBA(255, 255, 255, 255), bwRnd(k, 9)));
        }
    }
    // flechas en vuelo (modelo opaco; si pasan pegadas a la camara no se dibujan)
    for (int k = 0; k < BW_NARR; ++k) {
        const BwArrow &a = bwArr[k];
        if (a.life <= 0 || bwDepth(a.x, a.y, a.z) < 4.0f) continue;
        float yaw, pitch; lvLookAngles(a.x, a.y, a.z, a.x + a.vx, a.y + a.vy, a.z + a.vz, yaw, pitch);
        lvDrawAt(g_bwArrowV, g_bwArrowN, a.x, a.y, a.z, yaw, pitch, 1.0f, 0.0f);
    }
    // flechas gigantes de la lluvia cayendo
    for (int k = 0; k < bwNZ; ++k) {
        const BwZone &z = bwZ[k];
        if (z.t < 60 || z.t >= 78) continue;
        const float ay = 44.0f * (1.0f - (z.t - 60) / 18.0f) + 3.5f;
        if (bwDepth(z.x, ay, z.z) < 6.0f) continue;
        lvDrawAt(g_bwArrowV, g_bwArrowN, z.x, ay, z.z, 0.0f, -BW_PI * 0.5f, 1.8f, 0.0f);
    }
    // ---- pase con mezcla (sin escribir Z): avisos, estela, columnas de luz ----
    sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0); sceGuDepthMask(GU_TRUE);
    // estela de luz del caballo
    if (bwState == 0 && bwTrN > 1) {
        float ax = bwCX, ay = bwCY, az = bwCZ;
        for (int s = 0; s < bwTrN && s < 7; ++s) {
            const int idx = (bwTrHead - s) & 7;
            const int a0 = 150 - s * 21, a1 = 150 - (s + 1) * 21;
            bwRibbon(ax, ay, az, RGBA(255, 238, 176, a0 > 0 ? a0 : 0),
                     bwTrail[idx][0], bwTrail[idx][1], bwTrail[idx][2], RGBA(255, 238, 176, a1 > 0 ? a1 : 0), 0.45f);
            ax = bwTrail[idx][0]; ay = bwTrail[idx][1]; az = bwTrail[idx][2];
        }
    }
    // zonas de la lluvia: anillo fijo + anillo que se cierra; despues, un haz de luz
    for (int k = 0; k < bwNZ; ++k) {
        const BwZone &z = bwZ[k];
        if (z.t < 0 || z.t >= 92) continue;
        if (bwDepth(z.x, 1.0f, z.z) < 5.0f) continue;               // pegada a la camara: se omite
        if (z.t < 78) {
            const float e = z.t / 60.0f > 1.0f ? 1.0f : z.t / 60.0f;
            lvRing3D(z.x, 0.1f, z.z, 2.7f, 0.3f, RGBA(240, 96, 40, 150 + (int)(70.0f * e)));
            lvRing3D(z.x, 0.12f, z.z, 2.7f * (1.0f - e) + 0.25f, 0.6f, RGBA(255, 196, 96, 200));
        } else {
            const float f = 1.0f - (z.t - 78) / 14.0f;
            lvBox(z.x, 0.0f, z.z, 1.8f, 16.0f * f + 0.5f, RGBA(255, 244, 200, (int)(200.0f * f)));
            lvRing3D(z.x, 0.12f, z.z, 2.7f * (2.0f - f), 0.4f * f + 0.1f, RGBA(255, 220, 140, (int)(200.0f * f)));
        }
    }
    // cinta de punteria hacia el jugador durante el aviso de las flechas
    if (bwState == 0 && bwAtk == 1 && bwAtkT < 48) {
        float ox, oy, oz; bwXform(-1.05f, 6.15f, -3.3f, ox, oy, oz);
        const int al = 70 + (int)(60.0f * sinf(g_lvT * 0.5f) + 60.0f);
        bwRibbon(ox, oy, oz, RGBA(255, 80, 40, al > 255 ? 255 : al), px, py + 1.7f, pz, RGBA(255, 200, 100, al / 2), 0.12f);
    }
    // estelas doradas de las flechas
    for (int k = 0; k < BW_NARR; ++k) {
        const BwArrow &a = bwArr[k];
        if (a.life <= 0) continue;
        bwRibbon(a.x, a.y, a.z, RGBA(255, 236, 160, 210), a.x - a.vx * 6.0f, a.y - a.vy * 6.0f, a.z - a.vz * 6.0f, RGBA(255, 236, 160, 0), 0.16f);
    }
    sceGuDepthMask(GU_FALSE); sceGuDisable(GU_BLEND);
    sceGumLoadIdentity();
}

// ---------------------------------------------------------------- HUD (2D). io.* aqui son simulados: no usarlos
static void bw_Hud(BossIO &io) {
    (void)io;
    sceGuDisable(GU_DEPTH_TEST); sceGuDisable(GU_TEXTURE_2D);
    sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    int sx = 0, sy = 0;
    const int vis = (bwState < 2) ? lvProject(bwCX, bwCY + 1.5f, bwCZ, sx, sy) : 0;
    if (vis && bwState == 0) {                                 // aureola dorada: se lee sobre el cielo blanco
        const float p = sinf(g_lvT * 0.07f);
        lvFan2D(sx, sy, 96 + (int)(8.0f * p), RGBA(255, 226, 140, 78), RGBA(255, 226, 140, 0), 22);
        if (bwHurt > 0) lvFan2D(sx, sy, 62, RGBA(255, 255, 255, 40 * bwHurt), RGBA(255, 255, 255, 0), 16);
        if (bwBlast > 0) {                                      // onda del estruendo
            int r = (30 - bwBlast) * 9 + 10; if (r > 260) r = 260;      // acotado: nada de vertices 2D lejisimos
            lvFan2D(sx, sy, r, RGBA(255, 240, 180, 0), RGBA(255, 240, 180, 110), 28);
        }
    }
    if (bwState == 1 && vis) {                                  // se deshace en luz
        int a = 40 + bwDeadT * 2; if (a > 220) a = 220;
        lvFan2D(sx, sy, 30 + (int)(bwDeadT * 1.6f), RGBA(255, 250, 220, a), RGBA(255, 240, 170, 0), 24);
    }
    // avisos de ataque: "!" sobre el jinete, o franja en el borde si esta fuera de pantalla
    const int warn = bwState == 0 && ((bwAtk == 1 && bwAtkT < 48) || (bwAtk == 3 && bwAtkT < 70) || (bwAtk == 2 && bwAtkT < 24));
    const int wa = (g_lvT & 8) ? 255 : 150;
    int edge = 0;
    if (bwState == 0 && bwT > 40 && !vis) {
        const float dx = bwCX - g_lvEye[0], dy = bwCY - g_lvEye[1], dz = bwCZ - g_lvEye[2];
        edge = (dx * g_lvRight[0] + dy * g_lvRight[1] + dz * g_lvRight[2]) >= 0.0f ? 1 : -1;
        if (warn) drawRect(edge > 0 ? SCR_WIDTH - 14 : 0, 0, 14, SCR_HEIGHT, RGBA(255, 70, 50, wa / 2));
    }
    if (warn && vis) {
        drawRect(sx - 3, sy - 66, 6, 16, RGBA(255, 70, 50, wa));
        drawRect(sx - 3, sy - 46, 6, 6, RGBA(255, 70, 50, wa));
    }
    // confusion: tinte violeta-dorado y franjas que ondulan
    if (bwConf) {
        const float w = g_lvT * 0.12f;
        bxTint(RGBA(200, 170, 255, 34 + (int)(14.0f * sinf(w))));
        for (int k = 0; k < 8; ++k)
            drawRect((int)(k * 64 + 22.0f * sinf(w + k * 0.9f)), 0, 28, SCR_HEIGHT, RGBA(255, 255, 255, 16));
        for (int k = 0; k < 6; ++k)
            drawRect(0, (int)(k * 48 + 14.0f * sinf(w * 1.3f + k)), SCR_WIDTH, 14, RGBA(255, 228, 160, 14));
        bxMashHud(bwMash, BW_MASH_NEED, "MUEVE EL ANÁLOGO");
    }
    // flecha al borde: hacia donde esta el jinete cuando no se ve
    if (edge != 0) {
        fontTexOn();
        drawText(edge > 0 ? SCR_WIDTH - 20 : 10, 126, 2.0f, RGBA(255, 214, 110, 230), edge > 0 ? ">" : "<");
        sceGuDisable(GU_TEXTURE_2D);
    }
    if (bwState < 2 && bwT > 30) bxBossBar("JINETE BLANCO", bwHp, BW_HP);
    sceGuDisable(GU_TEXTURE_2D);
}

// ---------------------------------------------------------------- cielo blanco luminoso (2D, sin Z)
static short bwSh(float v) { if (v < -120.0f) v = -120.0f; if (v > 600.0f) v = 600.0f; return (short)v; }
static void bwRay2D(int cx, int cy, float a, float half, int len, unsigned int cIn, unsigned int cOut) {
    GradVertex *v = (GradVertex *)sceGuGetMemory(sizeof(GradVertex) * 3);
    v[0] = { cIn,  (short)cx, (short)cy, 0 };
    v[1] = { cOut, bwSh(cx + cosf(a - half) * len), bwSh(cy + sinf(a - half) * len), 0 };
    v[2] = { cOut, bwSh(cx + cosf(a + half) * len), bwSh(cy + sinf(a + half) * len), 0 };
    sceGuDrawArray(GU_TRIANGLES, GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 3, 0, v);
}
static void bwCloud(int x, int y, int r, int a) {
    lvFan2D(x, y + r / 6, r, RGBA(255, 238, 200, a * 2 / 3), RGBA(255, 238, 200, 0), 14);       // base calida
    lvFan2D(x, y, r, RGBA(255, 255, 255, a), RGBA(255, 255, 255, 0), 16);
    lvFan2D(x - r * 2 / 3, y + r / 8, r * 3 / 4, RGBA(255, 255, 255, a), RGBA(255, 255, 255, 0), 14);
    lvFan2D(x + r * 3 / 4, y + r / 10, r * 2 / 3, RGBA(255, 255, 255, a), RGBA(255, 255, 255, 0), 14);
}
static void bw_Backdrop() {
    sceGuDisable(GU_TEXTURE_2D);
    gradQuad(0, 100, RGBA(206, 218, 240, 255), RGBA(230, 236, 247, 255));
    gradQuad(100, 176, RGBA(230, 236, 247, 255), RGBA(255, 250, 238, 255));
    gradQuad(176, SCR_HEIGHT, RGBA(255, 250, 238, 255), RGBA(255, 238, 200, 255));
    sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    const float t = g_lvT * 0.016f;
    lvFan2D(240, 176, 330, RGBA(255, 236, 176, 120), RGBA(255, 236, 176, 0), 24);               // resplandor del horizonte
    const int sunX = 352, sunY = 58;                                                           // sol: rayos que giran lento
    for (int k = 0; k < 9; ++k) bwRay2D(sunX, sunY, k * 0.7f + t * 0.05f, 0.05f, 420, RGBA(255, 246, 200, 38), RGBA(255, 246, 200, 0));
    lvFan2D(sunX, sunY, 170, RGBA(255, 246, 208, 190), RGBA(255, 246, 208, 0), 24);
    lvFan2D(sunX, sunY, 56, RGBA(255, 255, 242, 230), RGBA(255, 255, 242, 0), 20);
    // nubes en 3 capas con paralaje segun hacia donde miras (periodo = vuelta completa: sin saltos)
    const float yaw = atan2f(g_lvFwd[0], -g_lvFwd[2]);
    static const float kF[3] = { 120.0f, 170.0f, 230.0f };
    static const int kY[3] = { 150, 92, 40 }, kR[3] = { 58, 84, 112 }, kA[3] = { 120, 150, 110 };
    for (int l = 0; l < 3; ++l) {
        const int P = (int)(BW_TAU * kF[l] + 0.5f);
        for (int c = 0; c < 4; ++c) {
            float v = c * (P / 4.0f) + 37.0f * l + g_lvT * (0.10f + 0.05f * l) - yaw * kF[l];
            int x = (int)fmodf(v, (float)P); if (x < 0) x += P;
            x -= kR[l] + 20;
            bwCloud(x, kY[l] + (int)(bwRnd(c, l) * 26.0f) + (int)(4.0f * sinf(t * 0.4f + c + l)), kR[l] - c * 4, kA[l]);
        }
    }
    // polvo dorado
    for (int k = 0; k < 26; ++k) {
        const int x = (int)fmodf(k * 61.0f + g_lvT * (0.2f + 0.1f * (k % 3)), 480.0f);
        const int y = (int)fmodf(k * 47.0f + g_lvT * 0.12f * (1.0f + (k % 2)), 272.0f);
        const int a = 60 + (int)(90.0f * (0.5f + 0.5f * sinf(t * 2.0f + k)));
        drawRect(x, y, 2, 2, RGBA(255, 236, 170, a));
    }
    sceGuDisable(GU_BLEND);
}

// niebla blanca luminosa: la plataforma (a <45 u) apenas se vela
static unsigned int bw_Fog(float *nearF, float *farF) { *nearF = 40.0f; *farF = 200.0f; return RGBA(252, 248, 238, 255); }

// blanco del autoapuntado: el jinete, un poco adelantado a donde va (la bala tarda)
static int bw_AimTarget(float out[3]) {
    if (bwState != 0 || bwT < 40) return 0;
    const float ex = bwCX - g_lvEye[0], ey = bwCY - g_lvEye[1], ez = bwCZ - g_lvEye[2];
    float lead = sqrtf(ex * ex + ey * ey + ez * ez) / 1.8f;
    if (lead < 8.0f) lead = 8.0f;
    if (lead > 22.0f) lead = 22.0f;
    out[0] = bwCX + bwVx * lead; out[1] = bwCY + 1.5f + bwVy * lead; out[2] = bwCZ + bwVz * lead;
    return 1;
}
// impacto: esfera generosa (radio 6.5) sobre el conjunto caballo + jinete
static bool bw_ShotHit(float x, float y, float z, int dmg) {
    if (bwState != 0 || bwT < 60) return false;
    const float dx = x - bwCX, dy = y - (bwCY + 1.5f), dz = z - bwCZ;
    if (dx * dx + dy * dy + dz * dz > 6.5f * 6.5f) return false;
    if (bwIFr > 0) return true;                                // la bala se gasta, sin dano doble
    if (dmg < 1) dmg = 1;
    bwHp -= dmg; bwHurt = 6; bwIFr = 2;
    if (bwAtk == 3 && bwAtkT < 70) {                           // disparar durante el aviso interrumpe la trompeta
        bwTrumpDmg += dmg;
        if (bwTrumpDmg >= 12) { bwEndAttack(); bwAtkCd += 60; bwStun = 60; lvMessage("¡LE INTERRUMPISTE!", 70); }
    }
    if (bwHp <= 0) {
        bwHp = 0; bwState = 1; bwDeadT = 0; bwFallV = 0.0f;
        bwAtk = 0; bwConf = 0; bwMash.frames = 0; bwClearShots();
        g_lvFlash = 22;
        lvMessage("EL JINETE BLANCO CAE", 160);
    }
    return true;
}
static int bw_Blocks() { return bwConf ? 1 : 0; }     // confundido: ni te mueves ni disparas
static int bw_Done() { return bwState == 2 ? 1 : 0; }
static float bw_Shake() { return bwShakeAmp; }
