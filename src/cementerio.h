// =============================================================================
// cementerio.h -- PISO 2: EL CEMENTERIO (piso de AMBIENTE, estilo Silent Hill; sin jefe)
//
// Benjamin (2026-10-16): "borra el mini jefe y necromancia, dejemos que el cementerio sea un lugar
// como de ambiente, mejora mucho mucho su diseno, genera una cinematica, tiene que ser tipo silent
// hill". El Rey Esqueleto, el acertijo de las velas y la necromancia (con Coraje) se fueron; quedan
// en la historia de git (commit 7f75c9e) por si algun dia vuelven.
//
// Lo que hay ahora:
//   - NIEBLA espesa (se ve ~40 unidades), ceniza que cae, grano de pelicula, un zumbido grave.
//   - Cementerio de verdad (modelos en cementerio_models.h): porton, camino de grava, cuatro
//     bloques de tumbas, mausoleos, angeles que lloran, cipreses, faroles y la capilla al fondo.
//   - CINEMATICA al llegar la primera vez (X la salta): negro con una frase, la camara avanza a ras
//     del camino entre la niebla, se acerca a un angel que llora mientras sube el ruido de radio, y
//     se eleva por detras del jugador con el titulo del piso.
//   - LA FIGURA: una mujer de negro entre las tumbas del este. Mientras te acercas sube la estatica;
//     a 13 unidades se desvanece.
//   - EL ANGEL QUE SE DA VUELTA: el de la esquina este del cruce empieza de espaldas. Cada vez que
//     dejas de mirarlo, cuando vuelves a mirar te esta mirando.
//   - LA SIRENA: al llegar al atrio de la capilla dobla la campana, suena la sirena y todo se
//     oscurece (~7 s); despues la niebla vuelve y sube el elevador junto a la capilla.
//
// Se incluye en levels.h con los demas modulos de piso (prefijo bc_, contrato en boss_api.h).
// =============================================================================
#ifndef CEMENTERIO_H
#define CEMENTERIO_H

#include "cementerio_models.h"

#ifndef NOCTIS_TEST_BCCINE
#define NOCTIS_TEST_BCCINE 0   // PRUEBA: 1 = el cementerio se juega como la primera vez aunque el piso ya este abierto
#endif                         // (cinematica, sirena y elevador). goto_floor abre todos los pisos. 0 al commitear
// la primera vez: cinematica al entrar, la sirena en la capilla y el elevador cerrado hasta que suene
static int bcFirstTime() { return g_lvUnlocked <= LVF_CEMETERY || NOCTIS_TEST_BCCINE; }

static LineVertex __attribute__((aligned(16))) g_bcPool[24000];   // todos los modelos, de cerca y de lejos (~22.6k)
static int g_bcPoolN = 0;
static BcmInst g_bcInst[220]; static int g_bcNInst = 0, g_bcGhostI = -1, g_bcTurnI = -1;
static CityBldg *g_bcBx = 0; static int g_bcNbx = 0, g_bcBxCap = 0;
// estado
static int   g_bcT = 0;
static int   g_bcGhost = 0, g_bcGhostT = 0;          // 0 visible, 1 desvaneciendose, 2 ya no esta
static float g_bcTurnYaw = 0.0f; static int g_bcTurned = 0;
static int   g_bcEv = 0, g_bcEvT = 0, g_bcEvDone = 0; // la sirena: 0 nada, 1 sonando
static float g_bcDark = 0.0f;                        // 0 niebla gris .. 1 oscuridad oxidada
static float g_bcStatic = 0.0f;                      // ruido de radio (lo que se oye y se ve)
static int   g_cineMode = 0;                         // 0 = cinematica de jefe (levels.h), 1 = la del cementerio
static const int BC_CINE = 900;                      // 15 s
struct BcAsh { float x, y, z, vx, vz; };
static BcAsh g_bcAsh[56];

static unsigned int bcRng = 0x13579BDFu;
static inline float bcR() { bcRng = bcRng * 1664525u + 1013904223u; return (float)((bcRng >> 8) & 0xFFFF) / 65535.0f; }

static void bcBox(float x, float z, float w, float d, float h) { if (g_bcBx && g_bcNbx < g_bcBxCap) g_bcBx[g_bcNbx++] = { x, z, w, d, h, 0 }; }
// resplandor que siempre mira a la camara (abanico con el centro opaco y el borde transparente)
static void bcGlow(float x, float y, float z, float R, unsigned int cIn, unsigned int cOut) {
    const int N = 12;
    LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 3 * N);
    for (int s = 0; s < N; ++s) {
        const float a0 = GP_TAU * s / N, a1 = GP_TAU * (s + 1) / N;
        const float c0 = cosf(a0) * R, s0 = sinf(a0) * R, c1 = cosf(a1) * R, s1 = sinf(a1) * R;
        v[s * 3 + 0] = { cIn, x, y, z };
        v[s * 3 + 1] = { cOut, x + g_lvRight[0] * c0 + g_lvUp[0] * s0, y + g_lvRight[1] * c0 + g_lvUp[1] * s0, z + g_lvRight[2] * c0 + g_lvUp[2] * s0 };
        v[s * 3 + 2] = { cOut, x + g_lvRight[0] * c1 + g_lvUp[0] * s1, y + g_lvRight[1] * c1 + g_lvUp[1] * s1, z + g_lvRight[2] * c1 + g_lvUp[2] * s1 };
    }
    sceGumLoadIdentity(); sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, 3 * N, 0, v);
}
static float bcClamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
static float bcSmooth(float t) { t = bcClamp01(t); return t * t * (3.0f - 2.0f * t); }

// ---------------------------------------------------------------- la cinematica
static void bcCineStart() { g_cine = BC_CINE; g_cineMode = 1; }
static int bc_CineCam(ScePspFVector3 &eye, ScePspFVector3 &ctr) {
    const int t = BC_CINE - g_cine;
    if (t < 430) {                     // 1) a ras del camino, avanzando en la niebla
        const float u = bcSmooth(t / 430.0f);
        eye.x = 0.7f - 0.4f * u; eye.y = 1.45f + 0.15f * sinf(t * 0.02f); eye.z = 21.0f - 16.0f * u;
        ctr.x = 0.2f; ctr.y = 1.9f; ctr.z = eye.z - 12.0f;
    } else if (t < 650) {              // 2) el angel que llora, de frente, girando despacio
        const float u = bcSmooth((t - 430) / 220.0f), ax = -5.6f, az = -8.2f;
        const float a = -0.95f + 0.95f * u, R = 3.1f - 0.6f * u;
        eye.x = ax + sinf(a) * R; eye.y = 3.55f - 0.25f * u; eye.z = az + cosf(a) * R;
        ctr.x = ax; ctr.y = 3.05f; ctr.z = az;
    } else {                           // 3) grua por detras del jugador, hasta ver el cementerio
        const float u = bcSmooth((t - 650) / 250.0f);
        eye.x = 0.6f - 0.6f * u; eye.y = 1.9f + 9.6f * u; eye.z = 33.0f + 7.0f * u;
        ctr.x = 0.0f; ctr.y = 2.3f - 1.3f * u; ctr.z = 24.0f - 24.0f * u;
    }
    return 1;
}
static void bcCineTick() {             // (levels.h la llama cada frame de la cinematica)
    const int t = BC_CINE - g_cine;
    const float st = (t >= 430 && t < 650) ? 0.15f + 0.6f * ((t - 430) / 220.0f) : 0.0f;
    g_bcStatic += (st - g_bcStatic) * 0.08f;
    audioStatic(g_bcStatic); audioDrone(0.45f);
}
// texto centrado con espaciado extra (titulos)
static void bcSpaced(int y, float sc, int gap, unsigned int col, const char *s) {
    int n = textLen(s); const int w = n * (int)(8 * sc) + (n - 1) * gap;
    int x = 240 - w / 2; char one[4] = { 0, 0, 0, 0 };
    for (const char *p = s; *p; ) {
        int k = 1; if ((unsigned char)*p >= 0xC0) k = 2;   // UTF-8 de 2 bytes (tildes, enie)
        for (int q = 0; q < k; ++q) one[q] = p[q]; one[k] = 0; p += k;
        drawText(x, y, sc, col, one); x += (int)(8 * sc) + gap;
    }
}
// frases de la cinematica (Benjamin: son tus suenos, cambialas por las tuyas)
static const char *kBcCine[3] = {
    "Siempre vuelvo a este lugar.",
    "La niebla no deja ver dónde termina.",
    "Alguien me espera al otro lado.",
};
static int bcFadeA(int t, int a, int b, int in, int out) {   // alfa de una frase visible entre a y b
    if (t < a || t > b) return 0;
    int v = 255; if (t < a + in) v = (t - a) * 255 / in; if (t > b - out) v = (b - t) * 255 / out;
    return v < 0 ? 0 : (v > 255 ? 255 : v);
}
static void bcGrain(int n, int amp);
static void bc_CineHud() {
    const int t = BC_CINE - g_cine;
    sceGuDisable(GU_TEXTURE_2D);
    sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    // negro: el comienzo, entre tomas y el final
    int blk = 0;
    if (t < 150) blk = 255; else if (t < 200) blk = 255 - (t - 150) * 255 / 50;
    if (t >= 410 && t < 430) blk = (t - 410) * 255 / 20; if (t >= 430 && t < 450) blk = 255 - (t - 430) * 255 / 20;
    if (t >= 630 && t < 650) blk = (t - 630) * 255 / 20; if (t >= 650 && t < 670) blk = 255 - (t - 650) * 255 / 20;
    drawRect(0, 0, SCR_WIDTH, SCR_HEIGHT, RGBA(150, 150, 146, 30));                  // lavado gris (desatura)
    bcGrain(260, 46);
    if (g_bcStatic > 0.05f)                                                           // lineas de interferencia
        for (int k = 0; k < (int)(g_bcStatic * 26.0f); ++k)
            drawRect(0, (int)(bcR() * SCR_HEIGHT), SCR_WIDTH, 1 + (int)(bcR() * 2.0f), RGBA(220, 220, 220, (int)(40 + 80 * bcR())));
    if (blk > 0) drawRect(0, 0, SCR_WIDTH, SCR_HEIGHT, RGBA(0, 0, 0, blk));
    drawRect(0, 0, SCR_WIDTH, 26, RGBA(0, 0, 0, 255));                                // franjas de cine
    drawRect(0, SCR_HEIGHT - 26, SCR_WIDTH, 26, RGBA(0, 0, 0, 255));
    fontTexOn();
    int a;
    if ((a = bcFadeA(t, 20, 150, 40, 30)) > 0) bcSpaced(132, 1.0f, 1, RGBA(214, 210, 200, a), kBcCine[0]);
    if ((a = bcFadeA(t, 230, 405, 40, 40)) > 0) bcSpaced(220, 1.0f, 1, RGBA(214, 210, 200, a), kBcCine[1]);
    if ((a = bcFadeA(t, 470, 628, 40, 40)) > 0) bcSpaced(220, 1.0f, 1, RGBA(214, 210, 200, a), kBcCine[2]);
    if ((a = bcFadeA(t, 720, 892, 50, 40)) > 0) {
        bcSpaced(108, 2.0f, 6, RGBA(0, 0, 0, a / 2), "EL CEMENTERIO");
        bcSpaced(106, 2.0f, 6, RGBA(232, 228, 220, a), "EL CEMENTERIO");
        bcSpaced(138, 1.0f, 8, RGBA(170, 164, 150, a), "PISO 2");
    }
    drawText(398, 258, 1.0f, RGBA(140, 140, 150, 200), "X SALTAR");
    sceGuDisable(GU_TEXTURE_2D);
}

// ---------------------------------------------------------------- lo que dice el cuervo aqui
static const char *bcCuervoLine(int k) {
    static const char *kAntes[3] = { "Sigue el camino hasta la capilla. Cra.", "Si oyes ruido en el aire... no te detengas.", "Los ángeles no lloran por ti. Todavía." };
    static const char *kDespues[3] = { "La campana ya sonó. El elevador te espera.", "Está junto a la capilla, a la derecha.", "Y no mires atrás al salir. Cra." };
    return (g_bcEvDone || !bcFirstTime()) ? kDespues[k] : kAntes[k];
}

// ---------------------------------------------------------------- logica
static void bcResetAsh() {
    for (int k = 0; k < 56; ++k) g_bcAsh[k] = { (bcR() - 0.5f) * 36.0f, bcR() * 12.0f, (bcR() - 0.5f) * 36.0f, (bcR() - 0.5f) * 0.01f, (bcR() - 0.5f) * 0.01f };
}
static void bc_Update(BossIO &io) {
    ++g_bcT;
    const float px = io.px, pz = io.pz;
    // LA FIGURA: la estatica sube al acercarte; a 13 se desvanece
    float st = 0.0f;
    if (g_bcGhost < 2) {
        const float dx = px - BC_GHOST_X, dz = pz - BC_GHOST_Z, d = sqrtf(dx * dx + dz * dz);
        if (g_bcGhost == 0) {
            st = bcClamp01((34.0f - d) / 21.0f) * 0.85f;
            if (d < 13.0f) { g_bcGhost = 1; g_bcGhostT = 0; }
            else if (pz < BC_GHOST_Z - 9.0f) {           // ya la pasaste: si no la estas mirando, ya no esta
                const float ex = BC_GHOST_X - g_lvEye[0], ez = BC_GHOST_Z - g_lvEye[2], el = sqrtf(ex * ex + ez * ez) + 1e-3f;
                if ((ex * g_lvFwd[0] + ez * g_lvFwd[2]) / el < 0.5f) g_bcGhost = 2;
            }
        } else {
            st = 1.0f - g_bcGhostT / 50.0f;
            if (++g_bcGhostT >= 50) g_bcGhost = 2;
        }
    }
    // EL ANGEL QUE SE DA VUELTA: si no lo estas mirando, cuando vuelvas a mirar te estara mirando
    if (g_bcTurnI >= 0 && (g_bcGhost == 2 || pz < BC_TURN_Z + 4.0f)) {
        const float dx = BC_TURN_X - g_lvEye[0], dz = BC_TURN_Z - g_lvEye[2], dy = 3.0f - g_lvEye[1];
        const float d = sqrtf(dx * dx + dy * dy + dz * dz) + 1e-3f;
        const float look = (dx * g_lvFwd[0] + dy * g_lvFwd[1] + dz * g_lvFwd[2]) / d;
        if (look < 0.55f && d < 34.0f) {                             // fuera de la vista
            const float want = atan2f(-(px - BC_TURN_X), -(pz - BC_TURN_Z));
            if (fabsf(want - g_bcTurnYaw) > 0.2f) { g_bcTurnYaw = want; if (!g_bcTurned) { g_bcTurned = 1; st = 0.6f; } }
        }
    }
    // LA SIRENA: al llegar al atrio de la capilla (la primera vez)
    if (!g_bcEv && !g_bcEvDone && bcFirstTime() && fabsf(px) < 7.5f && pz < -35.0f && pz > -45.0f) {
        g_bcEv = 1; g_bcEvT = 0; audioBell(); audioSiren(1);
    }
    if (g_bcEv) {
        ++g_bcEvT;
        g_bcDark += (1.0f - g_bcDark) * 0.03f;
        if (g_bcEvT == 200) audioBell();
        if (g_bcEvT >= 430) {
            g_bcEv = 0; g_bcEvDone = 1; audioSiren(0); audioBell();
            if (NOCTIS_TEST_BCCINE && !g_lvElevOn) { g_lvElevOn = 1; g_lvElevRise = 0.0f; lvMessage("SE ABRE EL ELEVADOR", 150); }
        }
    } else g_bcDark += (0.0f - g_bcDark) * 0.02f;
    // ceniza: cae lenta alrededor de la camara (se recicla al salir de su caja)
    for (int k = 0; k < 56; ++k) {
        BcAsh &a = g_bcAsh[k];
        a.x += a.vx + 0.004f * sinf(g_bcT * 0.013f + k); a.z += a.vz; a.y -= 0.012f + 0.004f * (k & 3);
        const float rx = a.x - g_lvEye[0], rz = a.z - g_lvEye[2];
        if (a.y < 0.0f || fabsf(rx) > 18.0f || fabsf(rz) > 18.0f) {
            a.x = g_lvEye[0] + (bcR() - 0.5f) * 34.0f; a.z = g_lvEye[2] + (bcR() - 0.5f) * 34.0f; a.y = 6.0f + bcR() * 7.0f;
        }
    }
    g_bcStatic += (st - g_bcStatic) * 0.1f;
    audioStatic(g_bcStatic); audioDrone(g_bcEv ? 0.75f : 0.45f);
}

// ---------------------------------------------------------------- dibujo 3D
static float bcFogFar() { return 40.0f - 16.0f * g_bcDark; }
static void bc_Draw(float px, float py, float pz) {
    (void)px; (void)py; (void)pz;
    sceGuDisable(GU_TEXTURE_2D);
    sceGuEnable(GU_FOG);
    const float far = bcFogFar() + 2.0f;
    for (int k = 0; k < g_bcNInst; ++k) {
        const BcmInst &I = g_bcInst[k];
        if (k == g_bcGhostI && g_bcGhost == 2) continue;
        const BcmModel &M = g_bcm[I.m];
        const float dx = I.x - g_lvEye[0], dz = I.z - g_lvEye[2], r = M.rad * I.s;
        const float d = sqrtf(dx * dx + dz * dz);
        // en la niebla no se ve: lo grande (capilla, mausoleos, cipreses) hasta el final de la niebla,
        // lo chico se suelta 8 antes (ya esta casi todo gris y no se nota)
        if (d - r > (r > 4.0f ? far : far - 8.0f)) continue;
        if (dx * g_lvFwd[0] + dz * g_lvFwd[2] < -r - 2.0f) continue;              // detras de la camara
        const float yaw = (k == g_bcTurnI) ? g_bcTurnYaw : I.yaw;
        if (k == g_bcGhostI && g_bcGhost == 1) {                                  // se desvanece
            const int a = 255 - g_bcGhostT * 5; const unsigned int f = (unsigned int)(a < 0 ? 0 : a);
            sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_FIX, GU_FIX, f | (f << 8) | (f << 16), (255 - f) | ((255 - f) << 8) | ((255 - f) << 16));
            lvDrawAt(g_bcPool + M.off, M.n, I.x, 0.0f, I.z, yaw, 0.0f, I.s, 0.0f);
            sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0); sceGuDisable(GU_BLEND);
            continue;
        }
        // de lejos, la version liviana (entre la niebla no se distingue y la PSP lo agradece)
        const int far = d > (I.m == BCM_ANGEL ? 13.0f : 18.0f) + 0.3f * r;
        lvDrawAt(g_bcPool + (far ? M.loOff : M.off), far ? M.loN : M.n, I.x, 0.0f, I.z, yaw, 0.0f, I.s, 0.0f);
    }
    sceGuDisable(GU_FOG);
    // luz de los faroles (suma, sin escribir Z): en la oscuridad de la sirena parpadean
    sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_FIX, 0, 0xFFFFFFFF); sceGuDepthMask(GU_TRUE);
    for (int k = 0; k < g_bcNInst; ++k) {
        const BcmInst &I = g_bcInst[k]; if (I.m != BCM_FAROL) continue;
        const float dx = I.x - g_lvEye[0], dz = I.z - g_lvEye[2], d = sqrtf(dx * dx + dz * dz);
        if (d > bcFogFar() + 6.0f) continue;
        if (g_bcEv && ((g_bcT + k * 7) % 23) < 4) continue;
        const float fade = 1.0f - bcClamp01((d - 8.0f) / (bcFogFar() - 4.0f)) * 0.7f, fl = 0.85f + 0.15f * sinf(g_bcT * 0.21f + k);
        const int A = (int)((70.0f + 70.0f * g_bcDark) * fade * fl);
        bcGlow(I.x, 3.22f * I.s, I.z, 0.55f, RGBA(255, 214, 150, A + 60 > 255 ? 255 : A + 60), RGBA(255, 180, 100, 0));
        bcGlow(I.x, 3.22f * I.s, I.z, 2.6f, RGBA(255, 196, 120, A / 2), RGBA(255, 160, 80, 0));
    }
    sceGuDepthMask(GU_FALSE); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0); sceGuDisable(GU_BLEND);
    sceGumLoadIdentity();
}

// ---------------------------------------------------------------- HUD (2D)
// grano de pelicula: n puntos claros y oscuros en UNA sola llamada (sprites)
static void bcGrain(int n, int amp) {
    GradVertex *v = (GradVertex *)sceGuGetMemory(sizeof(GradVertex) * 2 * n);
    for (int k = 0; k < n; ++k) {
        const short x = (short)(bcR() * SCR_WIDTH), y = (short)(bcR() * SCR_HEIGHT);
        const int light = (k & 1), a = (int)(amp * (0.4f + 0.6f * bcR()));
        const unsigned int c = light ? RGBA(235, 235, 230, a) : RGBA(0, 0, 0, a);
        v[k * 2] = { c, x, y, 0 }; v[k * 2 + 1] = { c, (short)(x + 1 + (k % 3 == 0)), (short)(y + 1), 0 };
    }
    sceGuDrawArray(GU_SPRITES, GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2 * n, 0, v);
}
static void bc_Hud(BossIO &io) {
    (void)io;
    sceGuDisable(GU_DEPTH_TEST); sceGuDisable(GU_TEXTURE_2D);
    sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    // ceniza que cae (proyectada: cerca es mas grande y mas clara)
    {
        GradVertex *v = (GradVertex *)sceGuGetMemory(sizeof(GradVertex) * 2 * 56); int n = 0;
        for (int k = 0; k < 56; ++k) {
            int sx, sy; const BcAsh &a = g_bcAsh[k];
            if (!lvProject(a.x, a.y, a.z, sx, sy)) continue;
            const float dx = a.x - g_lvEye[0], dy = a.y - g_lvEye[1], dz = a.z - g_lvEye[2], d = sqrtf(dx * dx + dy * dy + dz * dz);
            const int s = d < 6.0f ? 2 : 1, al = (int)(200.0f * (1.0f - bcClamp01(d / 20.0f)));
            if (al <= 8) continue;
            const unsigned int c = g_bcEv ? RGBA(120, 70, 60, al) : RGBA(222, 220, 214, al);
            v[n++] = { c, (short)sx, (short)sy, 0 }; v[n++] = { c, (short)(sx + s), (short)(sy + s), 0 };
        }
        if (n) sceGuDrawArray(GU_SPRITES, GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, n, 0, v);
    }
    bcGrain(160, 34);
    if (g_bcStatic > 0.05f) {                                                     // interferencia
        for (int k = 0; k < (int)(g_bcStatic * 22.0f); ++k)
            drawRect(0, (int)(bcR() * SCR_HEIGHT), SCR_WIDTH, 1 + (int)(bcR() * 2.0f), RGBA(220, 220, 220, (int)(30 + 70 * bcR() * g_bcStatic)));
        if (bcR() < g_bcStatic * 0.15f) drawRect(0, 0, SCR_WIDTH, SCR_HEIGHT, RGBA(200, 200, 200, 26));
    }
    if (g_bcDark > 0.02f) {                                                       // la sirena: oxido y oscuridad
        const int a = (int)(g_bcDark * (105.0f + 30.0f * sinf(g_bcT * 0.09f)));
        drawRect(0, 0, SCR_WIDTH, SCR_HEIGHT, RGBA(48, 8, 4, a));
        if (g_bcEv && (g_bcT % 37) < 3) drawRect(0, 0, SCR_WIDTH, SCR_HEIGHT, RGBA(0, 0, 0, 200));   // parpadeo
    }
    sceGuDisable(GU_TEXTURE_2D);
}

// ---------------------------------------------------------------- fondo y niebla
static unsigned int bcFogCol() { return lvMix(RGBA(176, 178, 180, 255), RGBA(50, 36, 34, 255), g_bcDark); }
static void bc_Backdrop() {
    const unsigned int f = bcFogCol();
    gradQuad(0, 140, lvMix(f, RGBA(120, 122, 126, 255), 0.25f), f);
    gradQuad(140, SCR_HEIGHT, f, f);
    // bancos de niebla que pasan despacio (le dan movimiento al aire)
    sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    for (int k = 0; k < 7; ++k) {
        const int x = ((k * 157 + (int)(g_lvT * 0.12f * (1 + k % 3))) % 720) - 120, y = 70 + (k * 41) % 110, r = 90 + (k * 37) % 80;
        lvFan2D(x, y, r, lvMix(RGBA(214, 214, 214, 70), RGBA(70, 50, 46, 70), g_bcDark), RGBA(200, 200, 200, 0), 16);
    }
    sceGuDisable(GU_BLEND);
}
static unsigned int bc_Fog(float *nearF, float *farF) {
    *nearF = 5.0f - 3.0f * g_bcDark; *farF = bcFogFar();
    return bcFogCol();
}
static int bc_AimTarget(float out[3]) { (void)out; return 0; }   // aqui no hay a quien dispararle
static bool bc_ShotHit(float x, float y, float z, int dmg) { (void)x; (void)y; (void)z; (void)dmg; return false; }
static int bc_Blocks() { return 0; }
static int bc_Done() { return (g_bcEvDone || !bcFirstTime()) ? 1 : 0; }
static float bc_Shake() { return g_bcEv ? 0.05f + 0.05f * g_bcDark : 0.0f; }

// ---------------------------------------------------------------- entrada al piso
static void bc_Enter(BossIO &io, LineVertex *geo, int geoCap, int *geoN, CityBldg *boxes, int *nBoxes, int boxCap,
                     float *sx, float *sy, float *sz) {
    if (g_bcPoolN <= 0) g_bcPoolN = bcmBuildAll(g_bcPool, 24000);                // los modelos se arman una vez
    *geoN = bcmBuildGround(geo, geoCap);
    g_bcNInst = bcmLayout(g_bcInst, 220);
    g_bcGhostI = g_bcTurnI = -1;
    g_bcBx = boxes; g_bcNbx = 0; g_bcBxCap = boxCap;
    bcBox(0.0f, 0.0f, 120.0f, 140.0f, 0.0f);                                      // el suelo
    // muro
    bcBox(0.0f, BC_NORTH_Z, 2.0f * BC_HALF_X + 2.0f, 1.0f, 1.4f);
    bcBox(-BC_HALF_X, (BC_NORTH_Z + BC_SOUTH_Z) * 0.5f, 1.0f, BC_SOUTH_Z - BC_NORTH_Z + 2.0f, 1.4f);
    bcBox(BC_HALF_X, (BC_NORTH_Z + BC_SOUTH_Z) * 0.5f, 1.0f, BC_SOUTH_Z - BC_NORTH_Z + 2.0f, 1.4f);
    bcBox(-(BC_HALF_X + 3.9f) * 0.5f, BC_SOUTH_Z, BC_HALF_X - 3.9f, 1.0f, 1.4f);
    bcBox((BC_HALF_X + 3.9f) * 0.5f, BC_SOUTH_Z, BC_HALF_X - 3.9f, 1.0f, 1.4f);
    bcBox(0.0f, BC_SOUTH_Z + 1.5f, 8.0f, 1.0f, 6.0f);                             // detras del porton no se sale
    for (int k = 0; k < g_bcNInst; ++k) {
        const BcmInst &I = g_bcInst[k];
        if (I.m == BCM_FANTASMA) g_bcGhostI = k;
        if (I.m == BCM_ANGEL && fabsf(I.x - BC_TURN_X) < 0.1f && fabsf(I.z - BC_TURN_Z) < 0.1f) g_bcTurnI = k;
        if (I.m == BCM_PORTON) { bcBox(I.x - 3.3f, I.z, 1.2f, 1.2f, 4.2f); bcBox(I.x + 3.3f, I.z, 1.2f, 1.2f, 4.2f); continue; }
        if (I.m == BCM_REJA) { bcBox(I.x, I.z, 4.0f, 0.4f, 2.2f); continue; }
        if (I.m == BCM_CAPILLA) { bcBox(I.x, I.z, 13.4f, 15.4f, 7.0f); continue; }
        if (I.m == BCM_TUMBA_REJA) {   // la reja rodea la losa, que esta DELANTE de la cabecera (local -Z)
            bcBox(I.x - 0.95f * sinf(I.yaw), I.z - 0.95f * cosf(I.yaw), 1.7f * I.s, 2.6f * I.s, 1.0f); continue; }
        const BcmModel &M = g_bcm[I.m];
        if (M.cw <= 0.0f) continue;
        // las tumbas: la caja es la lapida (la losa/monticulo de adelante se pisa); giradas ~180, simetricas
        bcBox(I.x, I.z, M.cw * I.s, M.cd * I.s, M.ch * I.s);
    }
    *nBoxes = g_bcNbx;
    *sx = 0.0f; *sy = 0.0f; *sz = BC_SPAWN_Z;
    io.camYaw = 0.0f;
    g_bcT = 0; g_bcGhost = 0; g_bcGhostT = 0; g_bcTurned = 0; g_bcTurnYaw = 0.0f;   // el angel empieza de espaldas
    g_bcEv = 0; g_bcEvT = 0; g_bcEvDone = 0; g_bcDark = 0.0f; g_bcStatic = 0.0f;
    bcResetAsh();
    audioDrone(0.45f);
    if (NOCTIS_TEST_BCCINE) g_lvElevOn = 0;   // (lvEnter lo vuelve a poner despues: ver levels.h)
    sceKernelDcacheWritebackAll();
}

#endif // CEMENTERIO_H
