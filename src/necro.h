// =============================================================================
// necro.h - NECROMANCIA (el poder que deja el Rey Esqueleto) + el easter egg de CORAJE
// Benjamin (2026-10-16): "Nuevo poder Necromancia, puedes invocar Esqueletos para pelear contigo".
// Easter egg: a veces, en vez de esqueletos, acude CORAJE EL PERRO COBARDE con una ametralladora.
//
// CUADRADO con la necromancia elegida (R + ARRIBA cambia de poder): 3 esqueletos salen de la
// tierra, te siguen y le tiran huesos a lo que estes peleando (el blanco del jefe, el ojo mas
// cercano, el serafin). 20 s, y 35 s de espera. 1 de cada 6 veces sale Coraje en su lugar.
// Se incluye al FINAL de levels.h (usa los esqueletos del cementerio y lvShotHit).
// =============================================================================
#ifndef NECRO_H
#define NECRO_H

static const int NK_TIME = 1200, NK_CD = 2100;   // dura 20 s; espera 35 s (desde que se invoca)
struct NkAlly { float x, z, y, yaw; int alive, cd, rise; };
struct NkBone { float x0, y0, z0, x1, y1, z1; int t, on; };
static NkAlly g_nk[3];
static NkBone g_nkBone[8];
static int   g_nkT = 0, g_nkCd = 0, g_nkCoraje = 0, g_nkGunT = 0, g_nkSeq = 0;
static int   g_nkSayT = 0;   // la frase de Coraje: subtitulo propio (los avisos del jefe la tapaban)
static float g_nkCX = 0.0f, g_nkCZ = 0.0f, g_nkCY = 0.0f, g_nkCYaw = 0.0f;
static float g_nkTrX = 0.0f, g_nkTrY = 0.0f, g_nkTrZ = 0.0f; static int g_nkTrFx = 0;   // ultima rafaga (para dibujarla)
static LineVertex __attribute__((aligned(16))) g_nkDogV[2200];   // ~1600 verts
static int g_nkDogN = 0;

// ---------------------------------------------------------------- Coraje (homenaje, low-poly)
// rosado-lila, mancha negra en el lomo, orejas con la punta marron, ojos grandes y un fusil negro.
static void nkBuildDog() {
    int i = 0; LineVertex *b = g_nkDogV;
    const unsigned int PK = RGBA(226, 168, 226, 255), PK2 = RGBA(204, 144, 210, 255), BK = RGBA(20, 18, 24, 255);
    const unsigned int EAR = RGBA(118, 66, 40, 255), WH = RGBA(250, 250, 246, 255), GUN = RGBA(28, 28, 32, 255);
    addBall(b, i, 0.0f, 0.95f, 0.15f, 0.42f, 0.48f, 0.55f, 4, 8, PK);                    // cuerpo (gordo, tipo frijol)
    addBall(b, i, 0.10f, 1.36f, 0.36f, 0.24f, 0.08f, 0.22f, 2, 6, BK);                   // la mancha negra del lomo (asoma ARRIBA)
    addBall(b, i, 0.0f, 1.62f, -0.18f, 0.34f, 0.32f, 0.32f, 4, 8, PK2);                  // cabeza
    addLimb(b, i, 0.0f, 1.58f, -0.40f, 0.0f, 1.52f, -0.82f, 0.16f, 0.10f, 6, PK2, PK);    // hocico
    addBall(b, i, 0.0f, 1.54f, -0.86f, 0.10f, 0.08f, 0.07f, 2, 6, BK);                   // nariz
    for (int s = -1; s <= 1; s += 2) {
        addBall(b, i, s * 0.14f, 1.78f, -0.42f, 0.12f, 0.14f, 0.06f, 3, 6, WH);           // ojos grandes
        addBall(b, i, s * 0.13f, 1.80f, -0.47f, 0.05f, 0.06f, 0.03f, 2, 5, BK);           // pupilas (miran fiero)
        addLimb(b, i, s * 0.22f, 1.88f, -0.10f, s * 0.42f, 2.40f, 0.05f, 0.06f, 0.03f, 4, PK2, PK2);   // orejas largas
        addLimb(b, i, s * 0.42f, 2.40f, 0.05f, s * 0.50f, 2.62f, 0.10f, 0.03f, 0.01f, 4, EAR, EAR);    // punta marron
        addLimb(b, i, s * 0.20f, 0.60f, 0.05f, s * 0.22f, 0.0f, 0.0f, 0.06f, 0.04f, 4, PK2, PK);      // patas
    }
    addLimb(b, i, 0.0f, 0.80f, 0.62f, 0.0f, 1.10f, 0.92f, 0.05f, 0.02f, 4, PK, PK2);      // cola
    // el FUSIL: en el costado derecho, apuntando al frente
    addLimb(b, i, 0.42f, 1.15f, 0.30f, 0.42f, 1.18f, -0.95f, 0.07f, 0.06f, 5, GUN, GUN);  // cuerpo y canon
    addLimb(b, i, 0.42f, 1.18f, -0.95f, 0.42f, 1.19f, -1.35f, 0.03f, 0.03f, 4, GUN, GUN);
    addLimb(b, i, 0.42f, 1.10f, -0.05f, 0.42f, 0.80f, -0.12f, 0.06f, 0.05f, 4, GUN, GUN); // cargador
    addLimb(b, i, 0.42f, 1.12f, 0.30f, 0.42f, 1.02f, 0.62f, 0.07f, 0.08f, 4, GUN, GUN);   // culata
    addLimb(b, i, 0.30f, 1.20f, -0.10f, 0.42f, 1.14f, -0.20f, 0.05f, 0.04f, 4, PK, PK);   // la patita que lo sostiene
    g_nkDogN = i;
}

// ---------------------------------------------------------------- blanco
static int nkTarget(float out[3], float px, float pz) {
    if (lvBossFloor()) return LV_BOSS(AimTarget, out);
    if (g_lvFloor == LVF_SERAPH && g_bs == BS_FIGHT) { out[0] = g_bsX; out[1] = g_bsY; out[2] = g_bsZ; return 1; }
    if (g_lvFloor == LVF_EYES && g_eyesOn) {            // el ojo vivo mas cercano
        int best = -1; float bd = 1e9f;
        for (int k = 0; k < 30; ++k) {
            const LvEye &e = g_eyes[k]; if (!e.alive || e.delay > 0 || e.die) continue;
            const float d = (e.x - px) * (e.x - px) + (e.z - pz) * (e.z - pz);
            if (d < bd) { bd = d; best = k; }
        }
        if (best >= 0) { out[0] = g_eyes[best].x; out[1] = g_eyes[best].y; out[2] = g_eyes[best].z; return 1; }
    }
    return 0;
}

// ---------------------------------------------------------------- invocar
static void nkSummon(float px, float py, float pz) {
    if (g_nkCd > 0) { lvMessage("LA NECROMANCIA AÚN NO RESPONDE", 60); return; }
    g_nkT = NK_TIME; g_nkCd = NK_CD; ++g_nkSeq;
    g_nkCoraje = (((unsigned int)g_lvT * 2654435761u + (unsigned int)g_nkSeq * 40503u) >> 16) % 6 == 0;
    if (NOCTIS_TEST_NECRO == 2) g_nkCoraje = 1;
    if (g_nkCoraje) {
        g_nkCX = px - 2.5f; g_nkCZ = pz + 1.5f; g_nkCY = py; g_nkCYaw = 0.0f; g_nkGunT = 0;
        g_nkSayT = 260;
    } else {
        for (int k = 0; k < 3; ++k) {
            NkAlly &a = g_nk[k];
            const float an = 2.4f + k * 0.7f;
            a.x = px + cosf(an) * 3.0f; a.z = pz + sinf(an) * 3.0f; a.y = py - 2.5f; a.yaw = 0.0f; a.alive = 1; a.cd = 40 + k * 20; a.rise = 45;
        }
        lvMessage("¡LEVÁNTENSE! LOS MUERTOS PELEAN CONTIGO", 160);
    }
    g_lvFlash = 6;
}
static void nkClear() { g_nkT = 0; g_nkSayT = 0; for (int k = 0; k < 3; ++k) g_nk[k].alive = 0; for (int k = 0; k < 8; ++k) g_nkBone[k].on = 0; g_nkCoraje = 0; }

// ---------------------------------------------------------------- cada frame
static void nkUpdate(float px, float py, float pz) {
    if (g_nkCd > 0) --g_nkCd;
    if (g_nkTrFx > 0) --g_nkTrFx;
    if (g_nkSayT > 0) --g_nkSayT;
    float t[3]; const int has = nkTarget(t, px, pz);
    for (int k = 0; k < 8; ++k) {   // huesos en vuelo: PERSIGUEN al blanco (los siervos caminan mientras vuelan) y al llegar golpean
        NkBone &b = g_nkBone[k]; if (!b.on) continue;
        if (has) { b.x1 += (t[0] - b.x1) * 0.25f; b.y1 += (t[1] - b.y1) * 0.25f; b.z1 += (t[2] - b.z1) * 0.25f; }
        if (++b.t >= 26) { b.on = 0; lvShotHit(b.x1, b.y1, b.z1, 4); }
    }
    if (g_nkT <= 0) return;
    if (--g_nkT == 0) { lvMessage(g_nkCoraje ? "CORAJE VUELVE A CASA CORRIENDO" : "LOS MUERTOS VUELVEN A LA TIERRA", 90); nkClear(); return; }
    if (g_nkCoraje) {   // Coraje: se queda a tu lado y vacia el cargador contra el blanco
        g_nkCX += (px - 2.5f - g_nkCX) * 0.05f; g_nkCZ += (pz + 1.5f - g_nkCZ) * 0.05f; g_nkCY = py;
        if (has) {
            float yaw, pitch; lvLookAngles(g_nkCX, g_nkCY + 1.2f, g_nkCZ, t[0], t[1], t[2], yaw, pitch); g_nkCYaw = yaw;
            if (++g_nkGunT % 5 == 0) {   // rafaga: una traza cada 5 frames, dano en una de cada dos
                g_nkTrX = t[0]; g_nkTrY = t[1]; g_nkTrZ = t[2]; g_nkTrFx = 3;
                if ((g_nkGunT / 5) & 1) lvShotHit(t[0], t[1], t[2], 2);
            }
        }
        return;
    }
    for (int k = 0; k < 3; ++k) {
        NkAlly &a = g_nk[k]; if (!a.alive) continue;
        if (a.rise > 0) { --a.rise; a.y = py - 2.5f * (a.rise / 45.0f); continue; }
        const float an = 2.4f + k * 0.7f, tx = px + cosf(an) * 3.0f, tz = pz + sinf(an) * 3.0f;   // en abanico, a tu lado
        a.x += (tx - a.x) * 0.04f; a.z += (tz - a.z) * 0.04f; a.y += (py - a.y) * 0.2f;
        if (has) {
            float yaw, pitch; lvLookAngles(a.x, a.y + 2.0f, a.z, t[0], t[1], t[2], yaw, pitch); a.yaw = yaw;
            if (--a.cd <= 0) {
                a.cd = 70;
                for (int q = 0; q < 8; ++q) if (!g_nkBone[q].on) {
                    NkBone &b = g_nkBone[q];
                    b.x0 = a.x; b.y0 = a.y + 2.3f; b.z0 = a.z; b.x1 = t[0]; b.y1 = t[1]; b.z1 = t[2]; b.t = 0; b.on = 1; break;
                }
            }
        }
    }
}

// ---------------------------------------------------------------- dibujo
static void nkDraw() {
    if (g_nkT > 0 && g_nkCoraje) {
        lvDrawAt(g_nkDogV, g_nkDogN, g_nkCX, g_nkCY + 0.05f * fabsf(sinf(g_lvT * 0.5f)), g_nkCZ, g_nkCYaw, 0.0f, 1.4f);
        if (g_nkTrFx > 0) {   // la traza de la rafaga y el fogonazo
            const float rx = cosf(g_nkCYaw), rz = -sinf(g_nkCYaw), fx = -sinf(g_nkCYaw), fz = -cosf(g_nkCYaw);
            const float mx = g_nkCX + rx * 0.6f + fx * 1.9f, my = g_nkCY + 1.65f, mz = g_nkCZ + rz * 0.6f + fz * 1.9f;
            LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 6); int n = 0;
            const unsigned int c0 = RGBA(255, 236, 150, 230), c1 = RGBA(255, 200, 90, 120);
            swQuadC(v, n, mx, my - 0.06f, mz, c0, mx, my + 0.06f, mz, c0, g_nkTrX, g_nkTrY + 0.06f, g_nkTrZ, c1, g_nkTrX, g_nkTrY - 0.06f, g_nkTrZ, c1);
            sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_FIX, 0, 0xFFFFFFFF); sceGuDepthMask(GU_TRUE);
            sceGumLoadIdentity(); sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, n, 0, v);
            lvBox(mx, my - 0.15f, mz, 0.3f, 0.3f, RGBA(255, 230, 140, 255));
            sceGuDepthMask(GU_FALSE); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0); sceGuDisable(GU_BLEND);
        }
    }
    if (g_nkT > 0 && !g_nkCoraje)
        for (int k = 0; k < 3; ++k) {
            const NkAlly &a = g_nk[k]; if (!a.alive) continue;
            lvDrawAt(g_bcSkelV, g_bcSkelN, a.x, a.y, a.z, a.yaw, 0.0f, 1.0f);
            const float fx = -sinf(a.yaw), fz = -cosf(a.yaw);   // ojos verdes de necromancia
            lvBox(a.x + fx * 0.2f, a.y + 2.83f, a.z + fz * 0.2f, 0.18f, 0.08f, RGBA(120, 255, 140, 255));
        }
    for (int k = 0; k < 8; ++k) {   // huesos volando en arco
        const NkBone &b = g_nkBone[k]; if (!b.on) continue;
        const float t = b.t / 26.0f, arc = 4.0f * t * (1.0f - t) * 3.0f;
        lvBox(b.x0 + (b.x1 - b.x0) * t, b.y0 + (b.y1 - b.y0) * t + arc - 0.15f, b.z0 + (b.z1 - b.z0) * t, 0.35f, 0.3f, RGBA(236, 230, 206, 255));
    }
}

static void nkHud() {
    if (g_nkSayT <= 0 || g_menu || g_cine > 0) return;
    static const char *kSay = "CORAJE: ¡LAS COSAS QUE TENGO QUE HACER POR AMOR!";
    const int n = textLen(kSay), a = g_nkSayT < 30 ? g_nkSayT * 8 : 240;
    sceGuDisable(GU_TEXTURE_2D); drawRect(240 - n * 4 - 6, 198, n * 8 + 12, 14, RGBA(10, 12, 18, a * 3 / 4));
    fontTexOn(); drawText(240 - n * 4, 201, 1.0f, RGBA(236, 176, 236, a), kSay); sceGuDisable(GU_TEXTURE_2D);
}

#endif // NECRO_H
