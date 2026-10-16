// ============================================================================
// boss_catedral.h -- PISO 10 "LA CATEDRAL": EL VIGIA (prefijo ct_ / g_ct / CT_). Contrato: boss_api.h
//
// Benjamin (2026-10-16): "quiero que aparezca uno en la catedral, tiene que ser una estructura grande,
// ve de referencia este personaje de fear and hunger Rancid the Sergal". En la historia la catedral era
// "una catedral viva que te persigue y te vigila con campanas": su guardian es EL VIGIA, una bestia
// flaca y encorvada de pelaje gris, con una mascara de hueso llena de OJOS (los de la Torre) y astas, que
// pelea con DOS LANZAS. La nave es enorme (fachada a abside ~140, boveda a 52): src/catedral_models.h.
//
// PELEA (220 de vida, 3 fases):
//   ESTOCADA  se agacha (linea roja hacia ti) y se lanza ~24 en linea recta: hazte a un lado o DASH.
//   GIRO      de cerca: abre las lanzas (anillo de aviso) y gira: SALTA o dash.
//   LANZA     te arroja la derecha (destello en la punta); se clava, y despues VUELVE a su mano.
//   SALTO     (fase 2+) marca un circulo donde estas y cae ahi; la onda del golpe se salta.
//   CAMPANAS  (fase 2+, 66%) las tres campanas doblan: anillos de sonido por el suelo (saltalos).
//   OJOS      (fase 3, 33%) "LA CATEDRAL TE MIRA": los vitrales se vuelven ojos y disparan; el es
//             mas rapido. Un tiro en la MASCARA hace el doble.
// Vencido: se arrodilla, suelta las lanzas y cae. Deja la MASCARA DE LOS OJOS (mercader.h).
// ============================================================================
#pragma once
#include "catedral_models.h"

static const int   CT_HP = 220;
static const float CT_S  = 2.6f;               // escala del Vigia (modelo en metros -> ~6.3 de alto encorvado)
#ifndef NOCTIS_TEST_CTFASE
#define NOCTIS_TEST_CTFASE 0   // PRUEBA: 2 = arranca en la fase 2 (campanas); 3 = en la 3 (ojos). 0 al commitear
#endif

enum { CTS_FIGHT, CTS_DYING, CTS_DONE };
enum { CTA_NONE, CTA_LUNGE, CTA_SWEEP, CTA_THROW, CTA_LEAP };

// ---------------------------------------------------------------- estado
static int   g_ctSt = CTS_FIGHT, g_ctStT = 0, g_ctT = 0, g_ctHp = CT_HP, g_ctPhase = 1, g_ctHurt = 0;
static float g_ctX = 0.0f, g_ctY = 0.0f, g_ctZ = -12.0f, g_ctYaw = 3.14159f, g_ctWalk = 0.0f, g_ctSpd = 0.0f;
static int   g_ctAtk = CTA_NONE, g_ctAtkT = 0, g_ctPause = 90, g_ctHit = 0, g_ctSeq = 0;
static float g_ctDirX = 0.0f, g_ctDirZ = 1.0f;          // direccion fija de la estocada
static float g_ctTgtX = 0.0f, g_ctTgtZ = 0.0f;          // donde cae el salto
static float g_ctSpinA = 0.0f;                          // giro extra del cuerpo (GIRO)
// la lanza arrojada
static int   g_ctSpSt = 0, g_ctSpT = 0;                 // 0 en la mano, 1 volando, 2 clavada, 3 volviendo
static float g_ctSpX, g_ctSpY, g_ctSpZ, g_ctSpVX, g_ctSpVY, g_ctSpVZ;
static int   g_ctSpHit = 0;
// campanas y ondas
static int   g_ctBellT = 0, g_ctBellRing = 0;           // reloj de la proxima campanada / frames sonando
struct CtWave { float x, z, r, v; int on, hit; };
static CtWave g_ctWave[6];
static float g_ctShakeV = 0.0f;
// vitrales que miran (fase 3)
static int   g_ctEyeCd = 0;
// geometria
static LineVertex __attribute__((aligned(16))) g_ctPool[9000];    // modelos (~8.4k)
static LineVertex __attribute__((aligned(16))) g_ctNave[18000];   // la nave en trozos, alto + liviano (~17.3k)
static int g_ctPoolN = 0, g_ctNaveN = 0;
static CtmInst g_ctInst[64]; static int g_ctNInst = 0;
static CityBldg *g_ctBx = 0; static int g_ctNbx = 0, g_ctBxCap = 0;

static float ctClamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
static float ctWrap(float a) { while (a > GP_PI) a -= GP_TAU; while (a < -GP_PI) a += GP_TAU; return a; }
static void ctBox(float x, float z, float w, float d, float h) { if (g_ctBx && g_ctNbx < g_ctBxCap) g_ctBx[g_ctNbx++] = { x, z, w, d, h, 0 }; }
static void ctAdd()   { sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_FIX, 0, 0xFFFFFFFF); sceGuDepthMask(GU_TRUE); }
static void ctAlpha() { sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0); sceGuDepthMask(GU_TRUE); }
static void ctOff()   { sceGuDepthMask(GU_FALSE); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0); sceGuDisable(GU_BLEND); }
static float ctFogFar() { return 135.0f; }
// pecho y mascara en el mundo (blancos del tiro)
static void ctChest(float *o) { o[0] = g_ctX; o[1] = g_ctY + 1.35f * CT_S; o[2] = g_ctZ; }
static void ctHead(float *o) {
    const float fx = -sinf(g_ctYaw), fz = -cosf(g_ctYaw);   // adelante del Vigia (el modelo mira a -Z)
    o[0] = g_ctX + fx * 0.6f * CT_S; o[1] = g_ctY + 1.95f * CT_S; o[2] = g_ctZ + fz * 0.6f * CT_S;
}
static void ctHand(int right, float *o) {   // aproximada: delante y al costado, a la altura de la cintura
    const float fx = -sinf(g_ctYaw), fz = -cosf(g_ctYaw), rx = cosf(g_ctYaw), rz = -sinf(g_ctYaw), s = right ? 1.0f : -1.0f;
    o[0] = g_ctX + (fx * 0.4f + rx * s * 0.45f) * CT_S; o[1] = g_ctY + 1.1f * CT_S; o[2] = g_ctZ + (fz * 0.4f + rz * s * 0.45f) * CT_S;
}
// dentro de la nave y fuera de los pilares
static void ctKeepInside(float &x, float &z) {
    x = ctClamp(x, -19.0f, 19.0f); z = ctClamp(z, kCtPierZ[CT_NPIER - 1] - 8.0f, CT_ZIN - 4.0f);
    for (int k = 0; k < CT_NPIER; ++k)
        for (int s = -1; s <= 1; s += 2) {
            const float dx = x - s * CT_NAVE, dz = z - kCtPierZ[k], d2 = dx * dx + dz * dz, R = 3.4f;
            if (d2 < R * R && d2 > 1e-4f) { const float d = sqrtf(d2); x = s * CT_NAVE + dx / d * R; z = kCtPierZ[k] + dz / d * R; }
        }
}

// ---------------------------------------------------------------- ataques
static void ctStartAtk(BossIO &io, int a) {
    g_ctAtk = a; g_ctAtkT = 0; g_ctHit = 0;
    float dx = io.px - g_ctX, dz = io.pz - g_ctZ; const float l = sqrtf(dx * dx + dz * dz) + 1e-3f;
    g_ctDirX = dx / l; g_ctDirZ = dz / l;
    if (a == CTA_LEAP) { g_ctTgtX = io.px; g_ctTgtZ = io.pz; ctKeepInside(g_ctTgtX, g_ctTgtZ); lvMessage("¡VA A CAER SOBRE TI!", 90); }
    if (a == CTA_SWEEP) g_ctSpinA = 0.0f;
}
static int ctWind(int base) { return g_ctPhase >= 3 ? base * 7 / 10 : base; }   // preparacion (mas corta en la fase 3)
static void ctEndAtk(int pause) { g_ctAtk = CTA_NONE; g_ctAtkT = 0; g_ctPause = g_ctPhase >= 3 ? pause / 2 : (g_ctPhase == 2 ? pause * 3 / 4 : pause); }
static void ctWaveAt(float x, float z, float v) {
    for (int k = 0; k < 6; ++k) if (!g_ctWave[k].on) { g_ctWave[k] = { x, z, 0.0f, v, 1, 0 }; return; }
}
static void ctRunAtk(BossIO &io) {
    ++g_ctAtkT;
    const int W = (g_ctAtk == CTA_LUNGE) ? ctWind(45) : (g_ctAtk == CTA_SWEEP ? ctWind(32) : (g_ctAtk == CTA_THROW ? ctWind(34) : ctWind(30)));
    switch (g_ctAtk) {
    case CTA_LUNGE:
        if (g_ctAtkT < W - 12) {   // apunta (sigue un poco al jugador) mientras se agacha
            float dx = io.px - g_ctX, dz = io.pz - g_ctZ; const float l = sqrtf(dx * dx + dz * dz) + 1e-3f;
            g_ctDirX += (dx / l - g_ctDirX) * 0.1f; g_ctDirZ += (dz / l - g_ctDirZ) * 0.1f;
            const float n = sqrtf(g_ctDirX * g_ctDirX + g_ctDirZ * g_ctDirZ) + 1e-4f; g_ctDirX /= n; g_ctDirZ /= n;
        } else if (g_ctAtkT >= W && g_ctAtkT < W + 20) {   // la estocada
            g_ctX += g_ctDirX * 1.2f; g_ctZ += g_ctDirZ * 1.2f; ctKeepInside(g_ctX, g_ctZ);
            const float dx = io.px - g_ctX, dz = io.pz - g_ctZ;
            if (!g_ctHit && !g_lvDashInv && dx * dx + dz * dz < 3.0f * 3.0f && io.py < 4.0f) { g_ctHit = 1; io.hp -= 22.0f; g_lvFlash = 12; }
        } else if (g_ctAtkT >= W + 55) ctEndAtk(70);
        break;
    case CTA_SWEEP:
        if (g_ctAtkT >= W && g_ctAtkT < W + 26) {
            g_ctSpinA += GP_TAU / 26.0f;
            const float dx = io.px - g_ctX, dz = io.pz - g_ctZ;
            if (!g_ctHit && !g_lvDashInv && dx * dx + dz * dz < 7.6f * 7.6f && io.py < 1.6f) { g_ctHit = 1; io.hp -= 18.0f; g_lvFlash = 10; }
        } else if (g_ctAtkT >= W + 56) { g_ctSpinA = 0.0f; ctEndAtk(60); }
        break;
    case CTA_THROW:
        if (g_ctAtkT == W && g_ctSpSt == 0) {   // la suelta
            float h[3]; ctHand(1, h);
            float dx = io.px - h[0], dy = io.py + 1.7f - h[1], dz = io.pz - h[2]; const float l = sqrtf(dx * dx + dy * dy + dz * dz) + 1e-3f;
            const float v = g_ctPhase >= 3 ? 1.45f : 1.15f;
            g_ctSpSt = 1; g_ctSpT = 0; g_ctSpHit = 0; g_ctSpX = h[0]; g_ctSpY = h[1]; g_ctSpZ = h[2];
            g_ctSpVX = dx / l * v; g_ctSpVY = dy / l * v; g_ctSpVZ = dz / l * v;
        }
        if (g_ctAtkT >= W + 30) ctEndAtk(60);
        break;
    case CTA_LEAP: {
        const int AIR = 50;
        if (g_ctAtkT >= W && g_ctAtkT < W + AIR) {   // vuela en parabola hasta el circulo
            const float t = (float)(g_ctAtkT - W) / AIR;
            g_ctX += (g_ctTgtX - g_ctX) * (1.0f / (AIR - (g_ctAtkT - W)));
            g_ctZ += (g_ctTgtZ - g_ctZ) * (1.0f / (AIR - (g_ctAtkT - W)));
            g_ctY = 18.0f * 4.0f * t * (1.0f - t);
        } else if (g_ctAtkT == W + AIR) {            // cae
            g_ctY = 0.0f; g_ctShakeV = 0.7f; g_lvFlash = 6;
            const float dx = io.px - g_ctX, dz = io.pz - g_ctZ;
            if (!g_lvDashInv && dx * dx + dz * dz < 6.5f * 6.5f && io.py < 3.0f) { io.hp -= 26.0f; g_lvFlash = 16; io.vy = 5.0f; }
            ctWaveAt(g_ctX, g_ctZ, 0.42f);
        } else if (g_ctAtkT >= W + AIR + 40) ctEndAtk(70);
    } break;
    default: break;
    }
}
// lanza arrojada: vuela, se clava y vuelve a la mano (pega a la ida y a la vuelta)
static void ctSpearUpdate(BossIO &io) {
    if (g_ctSpSt == 0) return;
    ++g_ctSpT;
    if (g_ctSpSt == 1 || g_ctSpSt == 3) {
        if (g_ctSpSt == 3) {   // vuelve: sigue a la mano
            float h[3]; ctHand(1, h);
            float dx = h[0] - g_ctSpX, dy = h[1] - g_ctSpY, dz = h[2] - g_ctSpZ; const float l = sqrtf(dx * dx + dy * dy + dz * dz) + 1e-3f;
            if (l < 2.0f) { g_ctSpSt = 0; return; }
            g_ctSpVX = dx / l * 1.5f; g_ctSpVY = dy / l * 1.5f; g_ctSpVZ = dz / l * 1.5f;
        }
        g_ctSpX += g_ctSpVX; g_ctSpY += g_ctSpVY; g_ctSpZ += g_ctSpVZ;
        const float dx = io.px - g_ctSpX, dy = io.py + 1.6f - g_ctSpY, dz = io.pz - g_ctSpZ;
        if (!g_ctSpHit && !g_lvDashInv && dx * dx + dy * dy + dz * dz < 1.7f * 1.7f) { g_ctSpHit = 1; io.hp -= 18.0f; g_lvFlash = 10; }
        if (g_ctSpSt == 1 && (g_ctSpY <= 0.4f || g_ctSpT > 90 || fabsf(g_ctSpX) > 21.0f)) { g_ctSpSt = 2; g_ctSpT = 0; if (g_ctSpY < 0.4f) g_ctSpY = 0.4f; }
    } else if (g_ctSpSt == 2 && g_ctSpT > (g_ctPhase >= 3 ? 60 : 100)) { g_ctSpSt = 3; g_ctSpT = 0; g_ctSpHit = 0; }
}
static void ctWaves(BossIO &io) {
    for (int k = 0; k < 6; ++k) {
        CtWave &w = g_ctWave[k]; if (!w.on) continue;
        w.r += w.v;
        const float d = sqrtf((io.px - w.x) * (io.px - w.x) + (io.pz - w.z) * (io.pz - w.z));
        if (!w.hit && !g_lvDashInv && fabsf(d - w.r) < 1.1f && io.py < 1.2f) { w.hit = 1; io.hp -= 12.0f; g_lvFlash = 8; }
        if (w.r > 75.0f) w.on = 0;
    }
}
static void ctFight(BossIO &io) {
    if (io.aiming) { const float d = ctWrap(atan2f(g_ctX - io.px, -(g_ctZ - io.pz)) - io.camYaw); io.camYaw += d * 0.09f; }
    if (g_lvTimeStop > 0) return;
    // mira al jugador (salvo mientras se lanza o gira)
    if (!(g_ctAtk == CTA_LUNGE && g_ctAtkT >= ctWind(45)) && !(g_ctAtk == CTA_LEAP && g_ctAtkT >= ctWind(30))) {
        const float want = atan2f(-(io.px - g_ctX), -(io.pz - g_ctZ));
        g_ctYaw += ctWrap(want - g_ctYaw) * (g_ctAtk == CTA_NONE ? 0.08f : 0.03f);
    }
    if (g_ctAtk == CTA_LUNGE && g_ctAtkT >= ctWind(45) - 12) g_ctYaw = atan2f(-g_ctDirX, -g_ctDirZ);
    const float dx = io.px - g_ctX, dz = io.pz - g_ctZ, d = sqrtf(dx * dx + dz * dz) + 1e-3f;
    g_ctSpd = 0.0f;
    if (g_ctAtk == CTA_NONE) {
        // acecha: se acerca si estas lejos, rodea si estas a tiro, se aparta si estas encima
        const float sp = g_ctPhase >= 3 ? 0.17f : 0.12f, side = (g_ctSeq & 1) ? 1.0f : -1.0f;
        float mx = 0.0f, mz = 0.0f;
        if (d > 17.0f) { mx = dx / d; mz = dz / d; }
        else if (d < 7.0f) { mx = -dx / d * 0.6f + (-dz / d) * side * 0.6f; mz = -dz / d * 0.6f + (dx / d) * side * 0.6f; }
        else { mx = -dz / d * side; mz = dx / d * side; }
        g_ctX += mx * sp; g_ctZ += mz * sp; ctKeepInside(g_ctX, g_ctZ);
        g_ctSpd = sqrtf(mx * mx + mz * mz) * sp;
        if (--g_ctPause <= 0) {
            int a;
            if (d < 8.0f) a = CTA_SWEEP;
            else if (g_ctPhase >= 2 && (g_ctSeq % 3) == 2) a = CTA_LEAP;
            else if (g_ctSpSt == 0 && (g_ctSeq % 4) == 1) a = CTA_THROW;
            else a = CTA_LUNGE;
            ++g_ctSeq; ctStartAtk(io, a);
        }
    } else ctRunAtk(io);
    // LAS CAMPANAS (fase 2+): doblan cada tanto y mandan dos anillos por el suelo
    if (g_ctPhase >= 2) {
        if (--g_ctBellT <= 0) { g_ctBellT = g_ctPhase >= 3 ? 380 : 540; g_ctBellRing = 120; audioBell(); ctWaveAt(0.0f, CT_BELL_Z, 0.55f); g_ctShakeV = 0.25f; }
        if (g_ctBellRing > 0 && --g_ctBellRing == 75) { audioBell(); ctWaveAt(0.0f, CT_BELL_Z, 0.55f); }
    }
    // LOS VITRALES QUE MIRAN (fase 3): un ojo dispara cada tanto
    if (g_ctPhase >= 3 && --g_ctEyeCd <= 0) {
        g_ctEyeCd = 150;
        const int k = (int)(swRand(g_ctT, 3, 77) * (CT_NPIER - 1)) % (CT_NPIER - 1), s = (g_ctT & 64) ? 1 : -1;
        const float wz = (kCtPierZ[k] + kCtPierZ[k + 1]) * 0.5f;
        lvFireOrb(s * (CT_HALF - 0.8f), 12.0f, wz, io.px, io.py + 1.7f, io.pz, 0.9f, RGBA(236, 60, 40, 255));
    }
    ctSpearUpdate(io);
    ctWaves(io);
}

static void ct_Update(BossIO &io) {
    ++g_ctT; ++g_ctStT;
    if (g_ctHurt > 0) --g_ctHurt;
    g_ctShakeV *= 0.9f;
    switch (g_ctSt) {
    case CTS_FIGHT: ctFight(io); g_ctWalk += g_ctSpd * 1.6f; break;   // el paso avanza con lo que camina
    case CTS_DYING:
        g_ctSpd = 0.0f;
        if (g_ctStT == 1) { g_lvFlash = 30; audioBell(); }
        if (g_ctStT == 70) { audioBell(); g_ctShakeV = 0.4f; }
        if (g_ctStT >= 170) g_ctSt = CTS_DONE;
        break;
    default: break;
    }
}

// ---------------------------------------------------------------- pose y dibujo del Vigia
struct CtPose { float pitch, drop, legL, kneeL, legR, kneeR, armL, elbL, outL, armR, elbR, outR, spL, spR, head, tail; };
static CtPose ctPoseNow() {
    CtPose p;
    const float w = g_ctWalk, sp = ctClamp(g_ctSpd / 0.12f, 0.0f, 1.3f), br = sinf(g_ctT * 0.05f);
    p.pitch = 0.18f + 0.02f * br; p.drop = 0.0f;
    p.legL = 0.45f * sinf(w) * sp; p.legR = -0.45f * sinf(w) * sp;
    p.kneeL = 0.25f + 0.35f * (cosf(w) > 0.0f ? cosf(w) : 0.0f) * sp; p.kneeR = 0.25f + 0.35f * (-cosf(w) > 0.0f ? -cosf(w) : 0.0f) * sp;
    p.armL = 0.15f - 0.25f * sinf(w) * sp; p.armR = 0.15f + 0.25f * sinf(w) * sp;
    p.elbL = p.elbR = 0.35f; p.outL = p.outR = 0.12f;
    p.spL = p.spR = 0.85f; p.head = -0.10f + 0.04f * br; p.tail = 0.2f * sinf(g_ctT * 0.04f);
    if (g_ctSt == CTS_FIGHT && g_ctAtk != CTA_NONE) {
        const int t = g_ctAtkT;
        if (g_ctAtk == CTA_LUNGE) {
            const int W = ctWind(45);
            if (t < W) { const float k = ctClamp((float)t / W, 0.0f, 1.0f); p.pitch += 0.35f * k; p.drop = 0.28f * k; p.kneeL = p.kneeR = 0.25f + 0.7f * k; p.legL = p.legR = 0.35f * k; p.armL = p.armR = -0.6f * k; p.spL = p.spR = 0.85f + 0.4f * k; }
            else if (t < W + 20) { p.pitch += 0.5f; p.legL = 0.7f; p.legR = -0.6f; p.kneeL = 0.3f; p.kneeR = 0.8f; p.armL = p.armR = 1.25f; p.elbL = p.elbR = 0.05f; p.spL = p.spR = 1.5f; p.head = -0.3f; }
            else { const float k = ctClamp((t - W - 20) / 35.0f, 0.0f, 1.0f); p.pitch += 0.5f * (1.0f - k); p.armL = p.armR = 1.25f * (1.0f - k); p.spL = p.spR = 1.5f - 0.65f * k; }
        } else if (g_ctAtk == CTA_SWEEP) {
            const int W = ctWind(32); const float k = ctClamp((float)t / W, 0.0f, 1.0f);
            p.outL = p.outR = 0.12f + 1.25f * k; p.armL = p.armR = 0.35f * k; p.elbL = p.elbR = 0.35f * (1.0f - k);
            p.spL = p.spR = 0.85f + 0.7f * k; p.drop = 0.12f * k; p.kneeL = p.kneeR = 0.25f + 0.3f * k;
        } else if (g_ctAtk == CTA_THROW) {
            const int W = ctWind(34);
            if (t < W) { const float k = ctClamp((float)t / W, 0.0f, 1.0f); p.armR = -1.1f * k; p.elbR = 0.35f + 0.6f * k; p.spR = 0.85f - 1.6f * k; p.pitch -= 0.08f * k; }
            else { const float k = ctClamp((t - W) / 12.0f, 0.0f, 1.0f); p.armR = -1.1f + 2.6f * k; p.elbR = 0.95f * (1.0f - k); p.pitch += 0.15f * k; }
        } else if (g_ctAtk == CTA_LEAP) {
            const int W = ctWind(30);
            if (t < W) { const float k = (float)t / W; p.drop = 0.35f * k; p.kneeL = p.kneeR = 0.25f + 0.9f * k; p.legL = p.legR = 0.45f * k; p.pitch += 0.3f * k; p.armL = p.armR = -0.5f * k; }
            else if (t < W + 50) { p.kneeL = p.kneeR = 1.2f; p.legL = p.legR = 0.9f; p.armL = p.armR = 1.0f; p.outL = p.outR = 0.5f; p.spL = p.spR = 2.4f; p.tail = 0.6f; }
            else { const float k = ctClamp((t - W - 50) / 40.0f, 0.0f, 1.0f); p.drop = 0.35f * (1.0f - k); p.kneeL = p.kneeR = 1.1f - 0.85f * k; p.pitch += 0.35f * (1.0f - k); }
        }
    }
    if (g_ctSt != CTS_FIGHT) {   // muere: se arrodilla, suelta las lanzas y cae adelante
        const float k = ctClamp(g_ctStT / 70.0f, 0.0f, 1.0f), f = ctClamp((g_ctStT - 70) / 60.0f, 0.0f, 1.0f);
        p.drop = 0.45f * k; p.kneeL = p.kneeR = 0.25f + 1.2f * k; p.legL = p.legR = 0.6f * k;
        p.armL = p.armR = 0.2f - 0.4f * k; p.outL = p.outR = 0.3f * k; p.head = 0.5f * k;
        p.pitch += 1.1f * f; p.drop += 0.5f * f;
    }
    return p;
}
static void ctPivot(float x, float y, float z, float rx, float rz) {
    ScePspFVector3 t = { x, y, z }; sceGumTranslate(&t);
    if (rx != 0.0f) sceGumRotateX(rx);
    if (rz != 0.0f) sceGumRotateZ(rz);
    ScePspFVector3 b = { -x, -y, -z }; sceGumTranslate(&b);
}
static void ctPart(int a, int b) { if (b > a) sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, b - a, 0, g_ctPool + g_ctm[CTM_VIGIA].off + a); }
static void ctDrawSpearAt(int right, float tilt) {
    sceGumPushMatrix();
    ScePspFVector3 h = { (right ? 1.0f : -1.0f) * CTJ_HAND[0], CTJ_HAND[1], CTJ_HAND[2] }; sceGumTranslate(&h);
    sceGumRotateX(tilt);
    sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g_ctm[CTM_SPEAR].n, 0, g_ctPool + g_ctm[CTM_SPEAR].off);
    sceGumPopMatrix();
}
static void ctDrawVigia() {
    if (g_ctSt == CTS_DONE) return;
    const CtPose p = ctPoseNow();
    const int *M = g_ctMark;
    const int hot = g_ctHurt > 0 && ((g_ctHurt >> 1) & 1);
    sceGumLoadIdentity();
    { ScePspFVector3 t = { g_ctX, g_ctY, g_ctZ }; sceGumTranslate(&t); }
    sceGumRotateY(g_ctYaw + g_ctSpinA);
    { ScePspFVector3 s = { CT_S, CT_S, CT_S }; sceGumScale(&s); }
    { ScePspFVector3 d = { 0.0f, -p.drop, 0.0f }; sceGumTranslate(&d); }
    ctPivot(0.0f, CTJ_HIP[1], CTJ_HIP[2], -p.pitch, 0.0f);          // el cuerpo se encorva desde la cadera
    if (hot) ctAdd();
    ctPart(M[4], M[5]);                                                 // torso
    sceGumPushMatrix(); ctPivot(CTJ_TAIL[0], CTJ_TAIL[1], CTJ_TAIL[2], 0.25f * p.pitch, p.tail); ctPart(M[5], M[6]); sceGumPopMatrix();
    // las piernas no se encorvan con el torso: se deshace el pitch en la cadera
    for (int s = 0; s < 2; ++s) {
        const float sx = s ? CTJ_HIP[0] : -CTJ_HIP[0], kx = s ? CTJ_KNEE[0] : -CTJ_KNEE[0];
        sceGumPushMatrix();
        ctPivot(sx, CTJ_HIP[1], CTJ_HIP[2], p.pitch + (s ? p.legR : p.legL), 0.0f);
        ctPart(M[s ? 2 : 0], M[s ? 3 : 1]);
        ctPivot(kx, CTJ_KNEE[1], CTJ_KNEE[2], -(s ? p.kneeR : p.kneeL), 0.0f);
        ctPart(M[s ? 3 : 1], M[s ? 4 : 2]);
        sceGumPopMatrix();
    }
    // brazos (con su lanza)
    for (int s = 0; s < 2; ++s) {
        const float sg = s ? 1.0f : -1.0f;
        sceGumPushMatrix();
        ctPivot(sg * CTJ_SHL[0], CTJ_SHL[1], CTJ_SHL[2], s ? p.armR : p.armL, sg * (s ? p.outR : p.outL));
        ctPart(M[s ? 8 : 6], M[s ? 9 : 7]);
        ctPivot(sg * CTJ_ELB[0], CTJ_ELB[1], CTJ_ELB[2], s ? p.elbR : p.elbL, 0.0f);
        ctPart(M[s ? 9 : 7], M[s ? 10 : 8]);
        if (g_ctSt == CTS_FIGHT && !(s == 1 && g_ctSpSt != 0)) ctDrawSpearAt(s, s ? p.spR : p.spL);
        sceGumPopMatrix();
    }
    // cabeza
    sceGumPushMatrix(); ctPivot(CTJ_NECK[0], CTJ_NECK[1], CTJ_NECK[2], -p.head, 0.0f); ctPart(M[10], M[11]); sceGumPopMatrix();
    if (hot) ctOff();
    sceGumLoadIdentity();
}

// ---------------------------------------------------------------- dibujo 3D
static void ctDrawInst(int m, float x, float y, float z, float yaw) {
    lvDrawAt(g_ctPool + g_ctm[m].off, g_ctm[m].n, x, y, z, yaw, 0.0f, 1.0f, 0.0f);
}
static void ct_Draw(float px, float py, float pz) {
    (void)px; (void)py; (void)pz;
    sceGuDisable(GU_TEXTURE_2D);
    sceGuEnable(GU_FOG);
    const float ex = g_lvEye[0], ez = g_lvEye[2], fx = g_lvFwd[0], fz = g_lvFwd[2];
    // la nave por trozos: solo lo que queda delante; de lejos, la version liviana
    sceGumLoadIdentity();
    for (int k = 0; k < g_ctNChunk; ++k) {
        const CtmChunk &C = g_ctChunk[k];
        const float zc = (C.z0 + C.z1) * 0.5f, dz = zc - ez;
        // delante de la camara si alguna esquina de su caja (x +-24, z0..z1) lo esta
        float ahead = -1e9f;
        for (int q = 0; q < 4; ++q) {
            const float cx = (q & 1) ? 24.0f : -24.0f, cz = (q & 2) ? C.z0 : C.z1, a = (cx - ex) * fx + (cz - ez) * fz;
            if (a > ahead) ahead = a;
        }
        if (ahead < 0.5f) continue;
        if (fabsf(dz) - (C.z0 - C.z1) * 0.5f > ctFogFar() + 10.0f) continue;
        const int lo = fabsf(dz) > 32.0f;
        sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, lo ? C.loN : C.n, 0, g_ctNave + (lo ? C.loOff : C.off));
    }
    for (int k = 0; k < g_ctNInst; ++k) {
        const CtmInst &I = g_ctInst[k];
        const float dx = I.x - ex, dz = I.z - ez, d = sqrtf(dx * dx + dz * dz);
        if (dx * fx + dz * fz < -8.0f) continue;
        int m = I.m; if (m == CTM_PIER && d > 34.0f) m = CTM_PIER_LO;
        ctDrawInst(m, I.x, I.y, I.z, I.yaw);
    }
    // campanas: se mecen al sonar
    for (int k = -1; k <= 1; ++k) {
        const float sw = (g_ctBellRing > 0) ? 0.35f * sinf(g_ctT * 0.12f + k) * (g_ctBellRing / 120.0f) : 0.0f;
        sceGumLoadIdentity();
        ScePspFVector3 t = { k * 7.0f, 36.0f, CT_BELL_Z }; sceGumTranslate(&t);
        sceGumRotateX(sw);
        sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g_ctm[CTM_BELL].n, 0, g_ctPool + g_ctm[CTM_BELL].off);
    }
    ctDrawVigia();
    // la lanza arrojada (orientada a su vuelo; clavada, quieta)
    if (g_ctSpSt != 0 && g_ctSt == CTS_FIGHT) {
        // el eje +Y del modelo va CONTRA el vuelo: asi la punta (-Y) va adelante
        const float vx = g_ctSpVX, vy = g_ctSpVY, vz = g_ctSpVZ, l = sqrtf(vx * vx + vy * vy + vz * vz) + 1e-4f;
        const float yaw = atan2f(-vx / l, -vz / l), pitch = acosf(ctClamp(vy / l, -1.0f, 1.0f));
        sceGumLoadIdentity();
        ScePspFVector3 t = { g_ctSpX, g_ctSpY, g_ctSpZ }; sceGumTranslate(&t);
        sceGumRotateY(yaw); sceGumRotateX(GP_PI - pitch);
        ScePspFVector3 s = { CT_S, CT_S, CT_S }; sceGumScale(&s);
        ScePspFVector3 o = { 0.0f, 1.95f, 0.0f }; sceGumTranslate(&o);   // la punta en la posicion
        sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g_ctm[CTM_SPEAR].n, 0, g_ctPool + g_ctm[CTM_SPEAR].off);
        sceGumLoadIdentity();
    }
    sceGuDisable(GU_FOG);
    // LUZ: velas de las aranas, rayos de luz de los vitrales, avisos y ondas (suma, sin escribir Z)
    ctAdd();
    for (int k = 0; k < g_ctNInst; ++k) {
        const CtmInst &I = g_ctInst[k];
        if (I.m == CTM_CHAND) {
            for (int q = 0; q < 8; q += 2) { const float a = GP_TAU * q / 8; bcGlow(I.x + cosf(a) * 2.3f, I.y - 4.3f, I.z + sinf(a) * 2.3f, 0.9f, RGBA(255, 200, 120, 150), RGBA(255, 160, 80, 0)); }
            bcGlow(I.x, I.y - 4.6f, I.z, 5.5f, RGBA(255, 180, 100, 45), RGBA(255, 150, 70, 0));
        } else if (I.m >= CTM_WIN0 && I.m <= CTM_WIN2) {   // rayo de luz: del vitral al suelo, hacia la nave
            static const unsigned int kL[3] = { RGBA(90, 120, 255, 26), RGBA(255, 210, 110, 26), RGBA(190, 120, 255, 26) };
            const unsigned int c = kL[I.m - CTM_WIN0], c0 = c & 0x00FFFFFFu;
            const float s = (I.x > 0.0f) ? 1.0f : -1.0f, x0 = I.x - s * 0.5f, x1 = I.x - s * 14.0f;
            LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 12); int n = 0;
            gpQuad(v, n, x0, 17.0f, I.z - 1.8f, c, x0, 17.0f, I.z + 1.8f, c, x1, 0.1f, I.z + 2.6f, c0, x1, 0.1f, I.z - 2.6f, c0);
            gpQuad(v, n, x0, 6.0f, I.z, c, x0, 18.0f, I.z, c, x1, 3.0f, I.z, c0, x1, 0.1f, I.z, c0);
            sceGumLoadIdentity(); sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, n, 0, v);
            if (g_ctPhase >= 3 && g_ctSt == CTS_FIGHT) {   // el vitral es un OJO que te mira
                const float pulse = 0.6f + 0.4f * sinf(g_ctT * 0.1f + I.z);
                bcGlow(I.x - s * 0.3f, 11.5f, I.z, 2.2f, RGBA(255, 220, 200, (int)(200 * pulse)), RGBA(255, 60, 30, 0));
                bcGlow(I.x - s * 0.35f, 11.5f, I.z, 0.8f, RGBA(255, 40, 20, 255), RGBA(120, 0, 0, 0));
            }
        }
    }
    if (g_ctSt == CTS_FIGHT) {
        // avisos de los ataques
        if (g_ctAtk == CTA_LUNGE && g_ctAtkT < ctWind(45)) {   // la linea de la estocada
            const float k = (float)g_ctAtkT / ctWind(45), len = 24.0f;
            const float x1 = g_ctX + g_ctDirX * len, z1 = g_ctZ + g_ctDirZ * len, nx = -g_ctDirZ * 1.2f, nz = g_ctDirX * 1.2f;
            LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 6); int n = 0;
            const unsigned int c = RGBA(255, 40, 30, 60 + (int)(150 * k)), c1 = RGBA(255, 40, 30, 0);
            gpQuad(v, n, g_ctX + nx, 0.08f, g_ctZ + nz, c, g_ctX - nx, 0.08f, g_ctZ - nz, c, x1 - nx, 0.08f, z1 - nz, c1, x1 + nx, 0.08f, z1 + nz, c1);
            sceGumLoadIdentity(); sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, n, 0, v);
        }
        if (g_ctAtk == CTA_SWEEP && g_ctAtkT < ctWind(32) + 26)
            lvRing3D(g_ctX, 0.08f, g_ctZ, 7.6f, 0.5f, RGBA(255, 60, 40, g_ctAtkT < ctWind(32) ? 80 + 4 * g_ctAtkT : 220));
        if (g_ctAtk == CTA_LEAP && g_ctAtkT < ctWind(30) + 50) {
            const float k = ctClamp((float)g_ctAtkT / (ctWind(30) + 50), 0.0f, 1.0f);
            lvRing3D(g_ctTgtX, 0.08f, g_ctTgtZ, 6.5f * (1.4f - 0.4f * k), 0.5f, RGBA(255, 60, 40, 80 + (int)(150 * k)));
        }
        if (g_ctAtk == CTA_THROW && g_ctAtkT < ctWind(34) && g_ctSpSt == 0) {   // destello en la punta
            float h[3]; ctHand(1, h);
            bcGlow(h[0], h[1] + 2.5f, h[2], 1.6f, RGBA(220, 236, 255, 120 + 3 * g_ctAtkT), RGBA(150, 180, 255, 0));
        }
        for (int k = 0; k < 6; ++k) {   // ondas por el suelo
            const CtWave &w = g_ctWave[k]; if (!w.on) continue;
            lvRing3D(w.x, 0.1f, w.z, w.r, 0.9f, RGBA(255, 210, 150, (int)(200.0f * (1.0f - w.r / 75.0f))));
        }
        if (g_ctPhase >= 3) {   // los ojos de la mascara se encienden
            float h[3]; ctHead(h);
            bcGlow(h[0], h[1], h[2], 1.4f, RGBA(255, 60, 30, 140 + (int)(60 * sinf(g_ctT * 0.2f))), RGBA(255, 20, 10, 0));
        }
    }
    ctOff();
    sceGumLoadIdentity();
}

// ---------------------------------------------------------------- HUD
static void ct_Hud(BossIO &io) {
    (void)io;
    sceGuDisable(GU_DEPTH_TEST); sceGuDisable(GU_TEXTURE_2D);
    sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    if (g_ctSt == CTS_FIGHT) {
        if (g_ctPhase >= 3) bxRage(g_ctT);
        bxBossBar("EL VIGÍA", g_ctHp, CT_HP);
    }
    sceGuDisable(GU_TEXTURE_2D);
}

// ---------------------------------------------------------------- API
static void ct_Backdrop() {
    gradQuad(0, 136, RGBA(22, 20, 30, 255), RGBA(14, 13, 20, 255));
    gradQuad(136, SCR_HEIGHT, RGBA(14, 13, 20, 255), RGBA(10, 10, 14, 255));
}
static unsigned int ct_Fog(float *nearF, float *farF) { *nearF = 30.0f; *farF = ctFogFar(); return RGBA(16, 15, 22, 255); }
static int ct_AimTarget(float out[3]) { if (g_ctSt != CTS_FIGHT) return 0; ctChest(out); return 1; }
static bool ct_ShotHit(float x, float y, float z, int dmg) {
    if (g_ctSt != CTS_FIGHT) return false;
    float c[3], h[3]; ctChest(c); ctHead(h);
    const float R = 0.95f * CT_S, RH = 0.42f * CT_S;
    const float dh = (x - h[0]) * (x - h[0]) + (y - h[1]) * (y - h[1]) + (z - h[2]) * (z - h[2]);
    const float dc = (x - c[0]) * (x - c[0]) + (y - c[1]) * (y - c[1]) + (z - c[2]) * (z - c[2]);
    if (dh > RH * RH && dc > R * R) return false;
    if (dmg < 1) dmg = 1;
    if (dh <= RH * RH) dmg *= 2;                         // en la MASCARA: los ojos sienten el dolor
    g_ctHp -= dmg; g_ctHurt = 8;
    const int ph = (g_ctHp * 3 > CT_HP * 2) ? 1 : (g_ctHp * 3 > CT_HP ? 2 : 3);
    if (ph > g_ctPhase && g_ctHp > 0) {
        g_ctPhase = ph; g_lvFlash = 18;
        if (ph == 2) { lvMessage("LAS CAMPANAS DESPIERTAN", 160); g_ctBellT = 30; }
        else { lvMessage("LA CATEDRAL TE MIRA", 170); g_ctEyeCd = 90; audioBell(); }
    }
    if (g_ctHp <= 0) {
        g_ctHp = 0; g_ctSt = CTS_DYING; g_ctStT = 0; g_ctAtk = CTA_NONE; g_ctSpSt = 0; g_ctY = 0.0f; g_ctSpinA = 0.0f;
        for (int k = 0; k < 6; ++k) g_ctWave[k].on = 0;
        lvMessage("EL VIGÍA CAE. SUS OJOS SE CIERRAN", 180);
    }
    return true;
}
static int ct_Blocks() { return 0; }
static int ct_Done() { return g_ctSt == CTS_DONE ? 1 : 0; }
static float ct_Shake() { return g_ctShakeV; }
static void ct_Enter(BossIO &io, LineVertex *geo, int geoCap, int *geoN, CityBldg *boxes, int *nBoxes, int boxCap,
                     float *sx, float *sy, float *sz) {
    (void)geo; (void)geoCap;
    if (g_ctPoolN <= 0) {   // se arma una sola vez: los modelos y la nave entera (en trozos)
        g_ctPoolN = ctmBuildAll(g_ctPool, 9000);
        g_ctNaveN = ctmBuildNave(g_ctNave, 18000);
    }
    *geoN = 0;              // la nave la dibuja este modulo por trozos (no va en la geometria fija del piso)
    g_ctNInst = ctmLayout(g_ctInst, 64);
    g_ctBx = boxes; g_ctNbx = 0; g_ctBxCap = boxCap;
    const float zA = CT_ZIN, zB = kCtPierZ[CT_NPIER - 1] - 12.0f;
    ctBox(0.0f, (zA + zB) * 0.5f - 6.0f, 60.0f, zA - zB + 30.0f, 0.0f);                 // el suelo
    ctBox(-(CT_HALF + 0.6f), (zA + zB) * 0.5f, 1.2f, zA - zB + 4.0f, 60.0f);            // muros de las laterales
    ctBox(CT_HALF + 0.6f, (zA + zB) * 0.5f, 1.2f, zA - zB + 4.0f, 60.0f);
    ctBox(0.0f, zA + 0.6f, 2.0f * CT_HALF + 4.0f, 1.2f, 60.0f);                         // fachada
    ctBox(-(CT_NAVE + CT_HALF) * 0.5f - 0.5f, zB - 0.6f, CT_HALF - CT_NAVE + 1.0f, 1.2f, 60.0f);   // fondo de las laterales
    ctBox((CT_NAVE + CT_HALF) * 0.5f + 0.5f, zB - 0.6f, CT_HALF - CT_NAVE + 1.0f, 1.2f, 60.0f);
    ctBox(0.0f, zB - CT_NAVE - 0.6f, 2.0f * CT_NAVE + 2.0f, 1.2f, 60.0f);               // fondo del abside
    ctBox(-(CT_NAVE + 0.6f), zB - CT_NAVE * 0.5f, 1.2f, CT_NAVE, 60.0f);                // costados del abside
    ctBox(CT_NAVE + 0.6f, zB - CT_NAVE * 0.5f, 1.2f, CT_NAVE, 60.0f);
    for (int k = 0; k < CT_NPIER; ++k) for (int s = -1; s <= 1; s += 2) ctBox(s * CT_NAVE, kCtPierZ[k], 2.8f, 2.8f, 40.0f);
    ctBox(0.0f, zB - 6.0f, 6.2f, 2.6f, 3.6f);                                            // el altar
    *nBoxes = g_ctNbx;
    *sx = 0.0f; *sy = 0.0f; *sz = 32.0f;
    io.camYaw = 0.0f;
    // estado de la pelea
    g_ctSt = CTS_FIGHT; g_ctStT = 0; g_ctT = 0; g_ctHp = CT_HP; g_ctPhase = 1; g_ctHurt = 0;
    g_ctX = 0.0f; g_ctY = 0.0f; g_ctZ = -12.0f; g_ctYaw = GP_PI; g_ctWalk = 0.0f; g_ctSpd = 0.0f;   // mira hacia la entrada (+Z)
    g_ctAtk = CTA_NONE; g_ctAtkT = 0; g_ctPause = 120; g_ctSeq = 0; g_ctSpinA = 0.0f;
    g_ctSpSt = 0; g_ctBellT = 200; g_ctBellRing = 0; g_ctEyeCd = 120; g_ctShakeV = 0.0f;
    for (int k = 0; k < 6; ++k) g_ctWave[k].on = 0;
    if (NOCTIS_TEST_CTFASE >= 2) { g_ctPhase = NOCTIS_TEST_CTFASE; g_ctHp = (g_ctPhase == 2) ? CT_HP * 2 / 3 - 1 : CT_HP / 3 - 1; g_ctBellT = 60; }
    audioDrone(0.30f);
    sceKernelDcacheWritebackAll();
}
