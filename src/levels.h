// ============================================================================
// levels.h -- LA TORRE: cielo -> elevador de 12 pisos -> piso 1 (campo + ojos) ->
//             piso 2 (escalera blanca + serafin). Logica de juego de cada piso.
//
// Piso 0 = el cielo de skyworld.h: se camina viendo el cielo hasta el ELEVADOR, al
//          final de un camino garantizado (celdas (0,0)..(0,SW_ELEV_CZ)).
// Piso 1 = campo con un arbol, muros pintados de cielo (floor1.h). De pronto el cielo
//          se llena de 30 OJOS: se derriban con AUTOAPUNTADO (L). Despues sube un
//          elevador junto al arbol.
// Piso 2 = solo una escalera blanca al cielo abierto (floor2.h). Arriba, el SERAFIN
//          (ofanim: ojo gigante, aros con ojos, alas) habla; eleccion: RENDIRSE o
//          PENSAR EN DIOS. Con Dios: ROSARIO (TRIANGULO, cura, 10 s de espera).
//          Ataques: LUZ (no lo mires al estallar) y PRISION (mueve el analogo).
// Pisos 3..12 = "???" (cerrados).
// Punto de GUARDADO en cada piso (TRIANGULO): guarda con la utilidad de la PSP, con
// icono y fondo propios (aparece en Datos guardados del XMB).
// ============================================================================

#include "floor1.h"
#include "floor2.h"
#include "save_util.h"
#include "icon_data.h"   // g_icon0Png / g_pic1Png (assets/ICON0.PNG y PIC1.PNG en bytes)
// cancion de la pelea con el serafin (src/music_blob.S: .incbin de assets/SERAPH.ADP)
extern "C" { extern const unsigned char g_seraphAdp[]; extern const unsigned char g_seraphAdpEnd[]; }
static const int LV_FADE = 150;   // cambio de piso: 30 a negro + 90 de PANTALLA DE CARGA + 30 de entrada


// ---------------------------------------------------------------- estado general
static int   g_lvFloor = 0, g_lvUnlocked = 1;
static int   g_lvT = 0;                    // frames desde que se entro al piso
static int   g_lvPrevBtn = 0;
static int   g_lvMenu = 0, g_lvMenuSel = 1;
static int   g_lvFade = 0, g_lvFadeTo = -1, g_lvFadeWhite = 0;
static const char *g_lvMsg = 0; static int g_lvMsgT = 0;
static int   g_lvFlash = 0;                // destello blanco (luz del serafin)
static float g_lvElevX = 0, g_lvElevY = 0, g_lvElevZ = 0; static int g_lvElevOn = 0; static float g_lvElevRise = 1.0f;
static float g_lvSaveX = 0, g_lvSaveY = 0, g_lvSaveZ = 0;
static float g_lvSpawnX = 0, g_lvSpawnY = 0, g_lvSpawnZ = 0;
static int   g_lvNear = 0;                 // 0 nada, 1 guardado, 2 elevador
// camara (main la carga despues de sceGumLookAt): proyeccion, autoapuntado y "lo miras?"
static float g_lvEye[3], g_lvFwd[3], g_lvRight[3], g_lvUp[3];

// ---------------------------------------------------------------- geometria
static LineVertex __attribute__((aligned(16))) g_lvGeo[9000];
static LineVertex __attribute__((aligned(16))) g_lvElevV[480];
static LineVertex __attribute__((aligned(16))) g_lvEyeV[560];
static LineVertex __attribute__((aligned(16))) g_lvSerBody[900];
static LineVertex __attribute__((aligned(16))) g_lvSerWing[420];
static LineVertex __attribute__((aligned(16))) g_lvSerRing[3][1000];
static int g_lvGeoN = 0, g_lvElevN = 0, g_lvEyeN = 0, g_lvSerBodyN = 0, g_lvSerWingN = 0, g_lvSerRingN[3];

// ---------------------------------------------------------------- ojos, orbes, jefe
struct LvEye { float bx, by, bz, x, y, z, ph, a0, r0, w; int hp, alive, delay, die, cd; };

// Los ojos ORBITAN en remolino alrededor del arbol (unos a un lado, otros al otro), entran y
// salen y suben y bajan. La posicion es funcion del tiempo, asi el autoapuntado puede
// calcular donde ESTARA el ojo cuando llegue la bala.
static void lvEyePos(const LvEye &e, float tf, float &x, float &y, float &z) {
    const float t = tf * 0.016f;
    const float ang = e.a0 + e.w * t;
    const float r = e.r0 + sinf(t * 0.9f + e.ph) * 5.0f;
    x = sinf(ang) * r; z = cosf(ang) * r;
    y = e.by + sinf(t * 1.6f + e.ph) * 2.5f;
}
static LvEye g_eyes[30];
static int   g_eyesOn = 0, g_eyesKilled = 0;
static float g_f1Dark = 0.0f;   // piso 1: 0 = cielo pintado .. 1 = cielo NEGRO (cuando llegan los ojos)
static int   g_f1DarkSaid = 0;

// mezcla de colores RGBA empaquetados (para apagar el cielo hacia el negro)
static unsigned int lvMix(unsigned int a, unsigned int b, float t) {
    const int ar = a & 0xFF, ag = (a >> 8) & 0xFF, ab = (a >> 16) & 0xFF;
    const int br = b & 0xFF, bg = (b >> 8) & 0xFF, bb = (b >> 16) & 0xFF;
    return RGBA(ar + (int)((br - ar) * t), ag + (int)((bg - ag) * t), ab + (int)((bb - ab) * t), 255);
}
struct LvOrb { float x, y, z, vx, vy, vz; int life; unsigned int col; };
static LvOrb g_orbs[16];

enum { BS_IDLE, BS_TALK, BS_CHOICE, BS_FIGHT, BS_DEAD };
static int   g_bs = BS_IDLE, g_bsLine = 0, g_bsChoice = 1, g_bsHp = 36, g_bsT = 0, g_bsDeadT = 0;
static float g_bsX = 0, g_bsY = 0, g_bsZ = 0;
static int   g_bsCharge = 0, g_bsStun = 0, g_bsOrbs = 0, g_bsHurt = 0;
static int   g_prison = 0; static float g_prX, g_prY, g_prZ, g_prAcc, g_prLX, g_prLY;
static int   g_rosary = 0, g_rosaryCd = 0;
// (UTF-8: drawText dibuja tildes, enie y signos de apertura)
static const char *kBsLines[4] = {
    "Has subido muy alto, soñador.",
    "Todo lo que ves aquí son tus sueños.",
    "No debes tenerme miedo.",
    "Acepta tu muerte, y descansa.",
};

static void lvMessage(const char *m, int frames) { g_lvMsg = m; g_lvMsgT = frames; }
static float lvDist2(float ax, float ay, float az, float bx, float by, float bz) {
    const float dx = ax - bx, dy = ay - by, dz = az - bz; return dx * dx + dy * dy + dz * dz;
}

// ---------------------------------------------------------------- arranque
static void lvInit(uint32_t flags) {
    g_lvElevN = f1BuildElevator(g_lvElevV, 480);
    g_lvEyeN = f1BuildEye(g_lvEyeV, 560);
    g_lvSerBodyN = f2BuildSeraphBody(g_lvSerBody, 900);
    g_lvSerWingN = f2BuildSeraphWing(g_lvSerWing, 420);
    const float rr[3] = { 5.5f, 6.5f, 7.5f };
    for (int k = 0; k < 3; ++k) g_lvSerRingN[k] = f2BuildSeraphRing(g_lvSerRing[k], 1000, rr[k]);
    sceKernelDcacheWritebackAll();
    if (flags & 0x80000000u) {                 // partida del mundo nuevo: piso + desbloqueados
        g_lvUnlocked = (int)((flags >> 4) & 15u); if (g_lvUnlocked < 1) g_lvUnlocked = 1;
        g_lvFloor = (int)(flags & 15u);        if (g_lvFloor > 2) g_lvFloor = 0;
    }
}
static uint32_t lvFlags() { return 0x80000000u | (uint32_t)g_lvFloor | ((uint32_t)g_lvUnlocked << 4); }

// entra a un piso: geometria, colision, spawn, estado de enemigos
static void lvEnter(int floor, float &px, float &py, float &pz, float &heroYaw, float &camYaw) {
    g_lvFloor = floor; g_lvT = 0; g_lvMenu = 0; g_prison = 0; g_bsCharge = 0;
    audioTrackStop(); audioMusic(1);                           // cada piso arranca con la musica tranquila
    for (int k = 0; k < 16; ++k) g_orbs[k].life = 0;
    heroYaw = 0.0f; camYaw = 0.0f;
    if (floor == 0) {
        SwLayout L; swLayout(0, SW_ELEV_CZ, L);
        g_lvElevX = L.x; g_lvElevY = L.top; g_lvElevZ = L.z - L.d * 0.5f + 5.0f; g_lvElevOn = 1; g_lvElevRise = 1.0f;
        SwLayout S; swLayout(0, 0, S);
        g_lvSaveX = 8.0f; g_lvSaveY = S.top; g_lvSaveZ = 7.0f;
        g_lvSpawnX = 0.0f; g_lvSpawnY = S.top; g_lvSpawnZ = 0.0f;
        g_swPcx = 0x7FFFFFF;                                  // fuerza a rehacer colision del cielo
        swUpdate(g_lvSpawnX, g_lvSpawnZ, 99);
    } else if (floor == 1) {
        int nb = 0;
        g_lvGeoN = f1BuildWorld(g_lvGeo, 9000, g_city, &nb, 255);
        g_city[nb++] = { 0.0f, 0.0f, 120.0f, 120.0f, 0.0f, 0 };   // el suelo (techo y=0)
        g_cityCount = nb;
        g_lvSpawnX = 0.0f; g_lvSpawnY = 0.0f; g_lvSpawnZ = 46.0f;
        g_lvSaveX = -9.0f; g_lvSaveY = 0.0f; g_lvSaveZ = 44.0f;
        g_lvElevX = 13.0f; g_lvElevY = 0.0f; g_lvElevZ = 0.0f;
        g_lvElevOn = (g_lvUnlocked >= 2); g_lvElevRise = g_lvElevOn ? 1.0f : 0.0f;
        g_eyesOn = 0; g_eyesKilled = 0; g_f1Dark = 0.0f; g_f1DarkSaid = 0;
        for (int k = 0; k < 30; ++k) {                        // reparto en espiral aurea sobre el campo
            LvEye &e = g_eyes[k];
            const float a = k * 2.39996f, r = 14.0f + 34.0f * ((k * 7) % 30) / 29.0f;
            e.bx = sinf(a) * r; e.bz = cosf(a) * r; e.by = 16.0f + (float)((k * 13) % 29);
            e.a0 = a; e.r0 = r; e.w = (0.35f + 0.35f * ((k * 11) % 17) / 16.0f) * ((k & 1) ? 1.0f : -1.0f);   // rad/s
            // los ojos se abren DESPUES de que el cielo termina de apagarse (~3 s), uno tras otro
            e.ph = k * 0.7f; e.hp = 2; e.alive = (g_lvUnlocked < 2); e.delay = 190 + k * 6; e.die = 0; e.cd = 180 + (k * 37) % 300;
        }
        if (g_lvUnlocked >= 2) g_eyesKilled = 30;
    } else {
        int nb = 0;
        g_lvGeoN = f2BuildStairs(g_lvGeo, 9000, g_city, &nb, 256);
        g_cityCount = nb;
        g_lvSpawnX = 0.0f; g_lvSpawnY = 0.0f; g_lvSpawnZ = 3.0f;
        g_lvSaveX = 5.0f; g_lvSaveY = 0.0f; g_lvSaveZ = 4.0f;
        float lx, ly, lz; f2LandingTop(&lx, &ly, &lz);
        g_bsX = lx; g_bsY = ly + 11.0f; g_bsZ = lz - 6.0f;
        g_lvElevX = lx; g_lvElevY = ly; g_lvElevZ = lz - 8.0f;
        g_lvElevOn = (g_lvUnlocked >= 3); g_lvElevRise = g_lvElevOn ? 1.0f : 0.0f;
        g_bs = (g_lvUnlocked >= 3) ? BS_DEAD : BS_IDLE; g_bsHp = 36; g_bsDeadT = 999; g_bsLine = 0;
    }
    px = g_lvSpawnX; py = g_lvSpawnY + 0.2f; pz = g_lvSpawnZ;
    g_swSafeX = px; g_swSafeY = py; g_swSafeZ = pz;
}

// ---------------------------------------------------------------- guardado
static void lvSaveFrame() {
    sceGuStart(GU_DIRECT, g_list);
    sceGuClearColor(RGBA(14, 16, 22, 255)); sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
    sceGuDisable(GU_DEPTH_TEST);
    fontTexOn(); drawText(192, 128, 1.0f, RGBA(220, 228, 236, 255), "GUARDANDO...");
    sceGuDisable(GU_TEXTURE_2D); sceGuEnable(GU_DEPTH_TEST);
    sceGuFinish(); sceGuSync(0, 0); sceDisplayWaitVblankStart(); sceGuSwapBuffers();
}
static void lvSaveNow(NoctisSave &sv) {
    sv.eventFlags = lvFlags();
    saveAuto(sv);                                              // copia rapida propia (la que carga el juego)
    const bool ok = saveUtilWrite(&sv, sizeof(NoctisSave), g_icon0Png, g_icon0PngSize, g_pic1Png, g_pic1PngSize,
                                  "PROJECT NOCTIS", "La Torre",
                                  g_lvFloor == 0 ? "El cielo. Camino al elevador." :
                                  g_lvFloor == 1 ? "Piso 1. El campo y el arbol." : "Piso 2. La escalera blanca.",
                                  lvSaveFrame);
    lvMessage(ok ? "PARTIDA GUARDADA" : "GUARDADO RAPIDO (SIN XMB)", 120);
}

// ---------------------------------------------------------------- autoapuntado
// Blanco = el que queda mas cerca del centro de la mirada (cono de ~40 grados), a <95u.
// shotSpeed > 0: devuelve donde ESTARA el ojo cuando llegue la bala (los ojos giran rapido).
static int lvAimTarget(float out[3], float shotSpeed = 0.0f) {
    float best = 0.766f; int found = 0, eye = -1;
    auto consider = [&](float x, float y, float z, int k) {
        const float dx = x - g_lvEye[0], dy = y - g_lvEye[1], dz = z - g_lvEye[2];
        const float d = sqrtf(dx * dx + dy * dy + dz * dz); if (d < 0.5f || d > 95.0f) return;
        const float c = (dx * g_lvFwd[0] + dy * g_lvFwd[1] + dz * g_lvFwd[2]) / d;
        if (c > best) { best = c; out[0] = x; out[1] = y; out[2] = z; found = 1; eye = k; }
    };
    if (g_lvFloor == 1 && g_eyesOn)
        for (int k = 0; k < 30; ++k) { const LvEye &e = g_eyes[k]; if (e.alive && e.delay <= 0 && !e.die) consider(e.x, e.y, e.z, k); }
    if (g_lvFloor == 2 && g_bs == BS_FIGHT) consider(g_bsX, g_bsY, g_bsZ, -1);
    if (found && eye >= 0 && shotSpeed > 0.0f) {
        for (int it = 0; it < 2; ++it) {            // 2 pasadas de "donde estara" bastan
            const float dx = out[0] - g_lvEye[0], dy = out[1] - g_lvEye[1], dz = out[2] - g_lvEye[2];
            const float dt = sqrtf(dx * dx + dy * dy + dz * dz) / shotSpeed;
            lvEyePos(g_eyes[eye], (float)g_lvT + dt, out[0], out[1], out[2]);
        }
    }
    return found;
}
// la bala toco un blanco? (aplica el dano)
static bool lvShotHit(float x, float y, float z) {
    if (g_lvFloor == 1 && g_eyesOn) {
        for (int k = 0; k < 30; ++k) {
            LvEye &e = g_eyes[k];
            if (!e.alive || e.delay > 0 || e.die) continue;
            if (lvDist2(x, y, z, e.x, e.y, e.z) < 2.4f * 2.4f) {
                if (--e.hp <= 0) { e.die = 14; g_eyesKilled++; }
                return true;
            }
        }
    }
    if (g_lvFloor == 2 && g_bs == BS_FIGHT && lvDist2(x, y, z, g_bsX, g_bsY, g_bsZ) < 5.0f * 5.0f) {
        g_bsHurt = 6;
        if (--g_bsHp <= 0) { g_bs = BS_DEAD; g_bsDeadT = 0; g_bsCharge = 0; g_prison = 0; audioTrackStop(); audioMusic(1); }
        return true;
    }
    return false;
}

static int g_orbCd = 0;   // pausa global entre disparos enemigos
static void lvFireOrb(float x, float y, float z, float tx, float ty, float tz, float spd, unsigned int col) {
    int alive = 0; for (int k = 0; k < 16; ++k) if (g_orbs[k].life > 0) ++alive;
    if (g_orbCd > 0 || alive >= 3) return;   // como mucho 3 en el aire y 0.75 s entre uno y otro
    g_orbCd = 45;
    for (int k = 0; k < 16; ++k) if (g_orbs[k].life <= 0) {
        float dx = tx - x, dy = ty - y, dz = tz - z, d = sqrtf(dx * dx + dy * dy + dz * dz); if (d < 0.01f) d = 0.01f;
        g_orbs[k] = { x, y, z, dx / d * spd, dy / d * spd, dz / d * spd, 520, col };
        return;
    }
}

// ---------------------------------------------------------------- logica por frame
// Devuelve 1 si el jugador NO debe moverse ni disparar (menu, dialogo, prision, fundido).
static int lvBlocks() { return g_lvMenu || g_lvFade > 0 || g_prison > 0 || g_bs == BS_TALK || g_bs == BS_CHOICE; }

static void lvUpdate(const SceCtrlData &pad, float &px, float &py, float &pz, float &vy, float &hp, float hpMax,
                     int grounded, float &heroYaw, float &camYaw, NoctisSave &sv) {
    const int btn = pad.Buttons, edge = btn & ~g_lvPrevBtn; g_lvPrevBtn = btn;
    ++g_lvT;
    if (g_lvMsgT > 0) --g_lvMsgT;
    if (g_lvFlash > 0) --g_lvFlash;
    if (g_rosaryCd > 0) --g_rosaryCd;
    if (g_orbCd > 0) --g_orbCd;
    sv.eventFlags = lvFlags();

    // ---- fundido entre pisos (y "rendirse") ----
    if (g_lvFade > 0) {
        --g_lvFade;
        if (g_lvFade == 100 && g_lvFadeTo >= 0) { lvEnter(g_lvFadeTo, px, py, pz, heroYaw, camYaw); vy = 0.0f; hp = hpMax; g_lvFadeTo = -1; }
        if (g_lvFade == 0) g_lvFadeWhite = 0;
        return;
    }
    // ---- menu del elevador ----
    if (g_lvMenu) {
        if (edge & PSP_CTRL_UP)   { if (--g_lvMenuSel < 1) g_lvMenuSel = 12; }
        if (edge & PSP_CTRL_DOWN) { if (++g_lvMenuSel > 12) g_lvMenuSel = 1; }
        if (edge & PSP_CTRL_CIRCLE) g_lvMenu = 0;
        if ((edge & PSP_CTRL_SELECT) && g_lvUnlocked < 2) { g_lvUnlocked = 2; lvMessage("PRUEBAS: PISO 2 ABIERTO", 90); }   // atajo de desarrollo
        if (edge & PSP_CTRL_CROSS) {
            if (g_lvMenuSel > g_lvUnlocked) lvMessage("ESE PISO SIGUE CERRADO", 90);
            else if (g_lvMenuSel > 2)       lvMessage("PISO EN CONSTRUCCION", 90);
            else if (g_lvMenuSel == g_lvFloor) g_lvMenu = 0;
            else { g_lvMenu = 0; g_lvFade = LV_FADE; g_lvFadeTo = g_lvMenuSel; g_lvFadeWhite = 0; }
        }
        return;
    }

    // ---- cercania: guardado / elevador ----
    g_lvNear = 0;
    if (lvDist2(px, py, pz, g_lvSaveX, g_lvSaveY, g_lvSaveZ) < 3.5f * 3.5f) g_lvNear = 1;
    if (g_lvElevOn && g_lvElevRise >= 1.0f && fabsf(px - g_lvElevX) < 3.4f && fabsf(pz - g_lvElevZ) < 3.4f && fabsf(py - g_lvElevY) < 2.0f) g_lvNear = 2;
    if (edge & PSP_CTRL_TRIANGLE) {
        if (g_lvNear == 1) { lvSaveNow(sv); return; }
        if (g_lvNear == 2) { g_lvMenu = 1; g_lvMenuSel = (g_lvFloor < 12) ? g_lvFloor + 1 : 1; return; }
        if (g_lvFloor == 2 && g_rosary) {                     // ROSARIO
            if (g_rosaryCd == 0 && hp < hpMax) { hp += 35.0f; if (hp > hpMax) hp = hpMax; g_rosaryCd = 600; lvMessage("EL ROSARIO TE SOSTIENE", 70); }
            else if (g_rosaryCd > 0) lvMessage("EL ROSARIO AUN DESCANSA", 50);
        }
    }

    // ---- orbes enemigos ----
    for (int k = 0; k < 16; ++k) {
        LvOrb &o = g_orbs[k]; if (o.life <= 0) continue;
        o.x += o.vx; o.y += o.vy; o.z += o.vz; --o.life;
        if (lvDist2(o.x, o.y, o.z, px, py + 1.7f, pz) < 1.3f * 1.3f) { hp -= 5.0f; o.life = 0; g_lvFlash = 6; }
    }

    // ---- piso 1: los ojos ----
    if (g_lvFloor == 1 && g_lvUnlocked < 2) {
        if (!g_eyesOn && (g_lvT > 600 || px * px + pz * pz < 30.0f * 30.0f)) {
            g_eyesOn = 1; lvMessage("EL CIELO SE APAGA", 150);
        }
        if (g_eyesOn && g_eyesKilled < 30) {          // el cielo se va a NEGRO en ~3 s
            g_f1Dark += 1.0f / 180.0f; if (g_f1Dark > 1.0f) g_f1Dark = 1.0f;
            if (g_f1Dark >= 1.0f && !g_f1DarkSaid) { g_f1DarkSaid = 1; lvMessage("Y SE LLENA DE OJOS", 150); }
        }
        if (g_eyesOn) {
            int volleys = 0;
            for (int k = 0; k < 30; ++k) {
                LvEye &e = g_eyes[k]; if (!e.alive) continue;
                if (e.delay > 0) { --e.delay; continue; }
                if (e.die > 0) { if (--e.die == 0) e.alive = 0; continue; }
                lvEyePos(e, (float)g_lvT, e.x, e.y, e.z);
                if (--e.cd <= 0) {
                    e.cd = 240 + (int)(swRand(k, g_lvT, 5) * 300.0f);
                    if (volleys++ < 1) lvFireOrb(e.x, e.y, e.z, px, py + 1.7f, pz, 0.20f, RGBA(220, 40, 52, 255));   // se esquiva caminando
                }
            }
            if (g_eyesKilled >= 30 && g_lvUnlocked < 2) {
                g_lvUnlocked = 2; g_lvElevOn = 1; g_lvElevRise = 0.0f;
                lvMessage("LOS OJOS SE CIERRAN. SE ABRE UN ELEVADOR", 180);
            }
        }
    }
    if (g_lvElevOn && g_lvElevRise < 1.0f) { g_lvElevRise += 1.0f / 120.0f; if (g_lvElevRise > 1.0f) g_lvElevRise = 1.0f; }
    // sin ojos, el cielo vuelve a aclararse (~4 s)
    if (g_lvFloor == 1 && g_eyesKilled >= 30 && g_f1Dark > 0.0f) { g_f1Dark -= 1.0f / 240.0f; if (g_f1Dark < 0.0f) g_f1Dark = 0.0f; }

    // ---- piso 2: el serafin ----
    if (g_lvFloor == 2) {
        float lx, ly, lz; f2LandingTop(&lx, &ly, &lz);
        if (g_bs == BS_IDLE && py > ly - 0.6f && fabsf(pz - lz) < 12.0f) { g_bs = BS_TALK; g_bsLine = 0; }
        else if (g_bs == BS_TALK) {
            if (edge & PSP_CTRL_CROSS) { if (++g_bsLine >= 4) { g_bs = BS_CHOICE; g_bsChoice = 1; } }
        } else if (g_bs == BS_CHOICE) {
            if (edge & (PSP_CTRL_UP | PSP_CTRL_DOWN)) g_bsChoice ^= 1;
            if (edge & PSP_CTRL_CROSS) {
                if (g_bsChoice == 0) {                       // RENDIRSE
                    lvMessage("TE RENDISTE. DESPIERTAS AL PIE DE LA ESCALERA", 200);
                    g_bs = BS_IDLE; g_lvFade = LV_FADE; g_lvFadeTo = 2; g_lvFadeWhite = 1;
                } else {                                      // PENSAR EN DIOS
                    lvMessage("UNA LUZ CÁLIDA TE SOSTIENE. ROSARIO: TRIÁNGULO", 200);
                    g_rosary = 1; g_bs = BS_FIGHT; g_bsT = 240; g_bsHp = 36;
                    audioMusic(0);                            // la cancion de la pelea
                    audioTrackPlay(g_seraphAdp, (unsigned int)(g_seraphAdpEnd - g_seraphAdp), 32000);
                }
            }
        } else if (g_bs == BS_FIGHT) {
            if (g_bsStun > 0) --g_bsStun;
            if (g_bsHurt > 0) --g_bsHurt;
            // ataque de LUZ: carga y estalla; si lo estas mirando, te quema
            if (g_bsCharge > 0 && --g_bsCharge == 0) {
                const float dx = g_bsX - g_lvEye[0], dy = g_bsY - g_lvEye[1], dz = g_bsZ - g_lvEye[2];
                const float d = sqrtf(dx * dx + dy * dy + dz * dz) + 0.001f;
                const float look = (dx * g_lvFwd[0] + dy * g_lvFwd[1] + dz * g_lvFwd[2]) / d;
                if (look > 0.45f) { hp -= 30.0f; g_lvFlash = 45; lvMessage("LA LUZ TE QUEMA", 90); }
                else              { g_lvFlash = 14; lvMessage("NO LO MIRASTE", 70); }
            }
            // PRISION de gravedad: inmovil y levantado; moviendo el analogo te sueltas
            if (g_prison > 0) {
                const float sx = ((int)pad.Lx - 128) / 128.0f, sy = ((int)pad.Ly - 128) / 128.0f;
                g_prAcc += sqrtf((sx - g_prLX) * (sx - g_prLX) + (sy - g_prLY) * (sy - g_prLY));
                g_prLX = sx; g_prLY = sy;
                if (g_prison % 60 == 0) hp -= 5.0f;
                if (g_prAcc >= 7.0f) { g_prison = 0; g_bsStun = 120; lvMessage("TE LIBERASTE", 70); }
                else if (--g_prison == 0) { hp -= 10.0f; lvMessage("LA GRAVEDAD TE SUELTA", 70); }
            }
            if (g_bsOrbs > 0 && g_lvT % 14 == 0) { --g_bsOrbs; lvFireOrb(g_bsX, g_bsY, g_bsZ, px, py + 1.7f, pz, 0.26f, RGBA(255, 226, 150, 255)); }
            if (g_bsCharge == 0 && g_prison == 0 && g_bsStun == 0 && --g_bsT <= 0) {
                const int r = (int)(swRand(g_lvT, 2, 9) * 100.0f);
                if (r < 45)      { g_bsCharge = 150; lvMessage("¡NO LO MIRES!", 150); }
                else if (r < 80) { g_prison = 240; g_prX = px; g_prY = py + 1.4f; g_prZ = pz; g_prAcc = 0.0f;
                                   g_prLX = g_prLY = 0.0f; lvMessage("LA GRAVEDAD TE APRISIONA: MUEVE EL ANÁLOGO", 150); }
                else             { g_bsOrbs = 6; }
                g_bsT = 260 + (int)(swRand(g_lvT, 3, 9) * 140.0f);
            }
        } else if (g_bs == BS_DEAD && g_bsDeadT < 999) {
            if (++g_bsDeadT == 150) {
                if (g_lvUnlocked < 3) g_lvUnlocked = 3;
                g_lvElevOn = 1; g_lvElevRise = 0.0f; g_rosary = 0;
                lvMessage("EL SERAFÍN SE DESVANECE. SE ABRE EL ELEVADOR", 180);
                g_bsDeadT = 999;
            }
        }
    }

    // ---- muerte / caida al vacio ----
    if (hp <= 0.0f || py < g_lvSpawnY - 60.0f) {
        if (g_lvFloor == 2 && g_bs == BS_FIGHT) { float lx, ly, lz; f2LandingTop(&lx, &ly, &lz); px = lx; py = ly + 0.2f; pz = lz + 8.0f; }
        else { px = g_lvSpawnX; py = g_lvSpawnY + 0.2f; pz = g_lvSpawnZ; }
        vy = 0.0f; hp = hpMax; g_prison = 0; g_bsCharge = 0;
        for (int k = 0; k < 16; ++k) g_orbs[k].life = 0;
        lvMessage("DESPIERTAS", 90);
    }
}

// despues de la fisica: la prision te deja clavado en el aire
static void lvPost(float &px, float &py, float &pz, float &vx, float &vy, float &vz) {
    if (g_prison > 0) {
        const float s = sinf(g_lvT * 0.9f) * 0.05f;              // tiembla apretado
        px = g_prX + s; py = g_prY; pz = g_prZ - s; vx = vy = vz = 0.0f;
    }
}

// ---------------------------------------------------------------- dibujo 3D
static void lvDrawAt(LineVertex *v, int n, float x, float y, float z, float yaw, float pitch, float scale, float roll = 0.0f) {
    if (n <= 0) return;
    sceGumLoadIdentity();
    ScePspFVector3 t = { x, y, z }; sceGumTranslate(&t);
    if (yaw != 0.0f) sceGumRotateY(yaw);
    if (pitch != 0.0f) sceGumRotateX(pitch);
    if (roll != 0.0f) sceGumRotateZ(roll);           // giro sobre su propio eje de mirada
    if (scale != 1.0f) { ScePspFVector3 s = { scale, scale, scale }; sceGumScale(&s); }
    sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, n, 0, v);
}
// orientacion para que un modelo que mira a -Z mire al punto (tx,ty,tz)
static void lvLookAngles(float x, float y, float z, float tx, float ty, float tz, float &yaw, float &pitch) {
    const float dx = tx - x, dy = ty - y, dz = tz - z;
    yaw = atan2f(-dx, -dz); pitch = atan2f(dy, sqrtf(dx * dx + dz * dz));
}
static void lvBox(float x, float y, float z, float w, float h, unsigned int col) {
    LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 30); int i = 0;
    swBoxL(v, i, x, y, z, w, w, h, col);
    sceGumLoadIdentity(); sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, i, 0, v);
}

static void lvDraw(float px, float py, float pz) {
    sceGuDisable(GU_CULL_FACE);
    if (g_lvFloor >= 1) {
        // piso 1 con el cielo apagado: la niebla se vuelve NEGRA y corta -> los muros de cielo
        // pintado se hunden en la oscuridad y solo queda el pasto y el arbol cerca
        const float k = (g_lvFloor == 1) ? g_f1Dark : 0.0f;
        // piso 2: neblina de LUZ (blanco calido) que se traga la escalera hacia lo lejos
        sceGuFog(g_lvFloor == 1 ? 30.0f - 20.0f * k : 15.0f, g_lvFloor == 1 ? 170.0f - 110.0f * k : 170.0f,
                 g_lvFloor == 1 ? lvMix(RGBA(196, 214, 232, 255), RGBA(0, 0, 0, 255), k) : RGBA(252, 246, 226, 255));
        sceGuEnable(GU_FOG);
        sceGumLoadIdentity(); sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g_lvGeoN, 0, g_lvGeo);
        sceGuDisable(GU_FOG);
        // VELO NEGRO sobre el escenario (pasto incluido) cuando el cielo se apaga. Va ANTES de
        // los ojos y del personaje: ellos quedan encendidos sobre la oscuridad. Sin tocar la Z.
        if (g_lvFloor == 1 && g_f1Dark > 0.0f) {
            sceGuDisable(GU_DEPTH_TEST); sceGuEnable(GU_BLEND);
            sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
            drawRect(0, 0, SCR_WIDTH, SCR_HEIGHT, RGBA(0, 0, 4, (int)(195.0f * g_f1Dark)));
            sceGuDisable(GU_BLEND); sceGuEnable(GU_DEPTH_TEST);
        }
    }
    // punto de guardado: pilar oscuro con un cristal frio que gira y respira
    {
        const float b = 0.5f + 0.5f * sinf(g_lvT * 0.06f);
        lvBox(g_lvSaveX, g_lvSaveY, g_lvSaveZ, 1.2f, 2.6f, RGBA(52, 58, 70, 255));
        lvBox(g_lvSaveX, g_lvSaveY + 3.2f + 0.3f * b, g_lvSaveZ, 0.8f, 0.8f, RGBA(150 + (int)(80 * b), 230, 250, 255));
    }
    // elevador (sube del suelo al aparecer)
    if (g_lvElevOn) lvDrawAt(g_lvElevV, g_lvElevN, g_lvElevX, g_lvElevY - 9.0f * (1.0f - g_lvElevRise), g_lvElevZ, 0.0f, 0.0f, 1.0f);
    // ojos
    if (g_lvFloor == 1 && g_eyesOn) {
        for (int k = 0; k < 30; ++k) {
            const LvEye &e = g_eyes[k]; if (!e.alive || e.delay > 0) continue;
            float yaw, pitch; lvLookAngles(e.x, e.y, e.z, px, py + 1.7f, pz, yaw, pitch);
            const float s = e.die ? 1.0f + (14 - e.die) * 0.08f : 1.0f;
            lvDrawAt(g_lvEyeV, g_lvEyeN, e.x, e.y, e.z, yaw, pitch, s, g_lvT * 0.06f * (k & 1 ? 1.0f : -1.0f) + e.ph);
        }
    }
    // serafin: el ojo mira al jugador, tres aros giran en ejes distintos, seis alas baten
    if (g_lvFloor == 2 && (g_bs != BS_DEAD || g_bsDeadT < 150)) {
        const float t = g_lvT * 0.016f;
        const float fade = (g_bs == BS_DEAD) ? 1.0f + g_bsDeadT * 0.02f : 1.0f;
        const float y = g_bsY + sinf(t * 0.9f) * 0.8f;
        float yaw, pitch; lvLookAngles(g_bsX, y, g_bsZ, px, py + 1.7f, pz, yaw, pitch);
        const float pulse = (g_bsCharge > 0) ? 1.0f + 0.12f * sinf(g_lvT * 0.5f) : 1.0f;
        lvDrawAt(g_lvSerBody, g_lvSerBodyN, g_bsX, y, g_bsZ, yaw, pitch, pulse * fade);
        const float spin = (g_bs == BS_DEAD) ? 6.0f : 1.0f;
        for (int k = 0; k < 3; ++k) {
            sceGumLoadIdentity();
            ScePspFVector3 tr = { g_bsX, y, g_bsZ }; sceGumTranslate(&tr);
            if (k == 0) { sceGumRotateX(t * 0.7f * spin); }
            if (k == 1) { sceGumRotateY(t * 0.9f * spin); sceGumRotateX(1.1f); }
            if (k == 2) { sceGumRotateZ(t * 0.5f * spin); sceGumRotateY(1.9f); }
            if (fade != 1.0f) { ScePspFVector3 s = { fade, fade, fade }; sceGumScale(&s); }
            sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g_lvSerRingN[k], 0, g_lvSerRing[k]);
        }
        for (int k = 0; k < 6; ++k) {
            const float side = (k & 1) ? -1.0f : 1.0f, elev = (k / 2 - 1) * 0.65f + sinf(t * 2.2f + k) * 0.12f;
            sceGumLoadIdentity();
            ScePspFVector3 tr = { g_bsX, y, g_bsZ }; sceGumTranslate(&tr);
            sceGumRotateY(yaw);
            ScePspFVector3 back = { 0.0f, 0.0f, 2.5f }; sceGumTranslate(&back);
            if (side < 0.0f) sceGumRotateY(3.14159f);
            sceGumRotateZ(elev);
            if (fade != 1.0f) { ScePspFVector3 s = { fade, fade, fade }; sceGumScale(&s); }
            sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g_lvSerWingN, 0, g_lvSerWing);
        }
    }
    // orbes
    for (int k = 0; k < 16; ++k) if (g_orbs[k].life > 0) lvBox(g_orbs[k].x, g_orbs[k].y - 0.35f, g_orbs[k].z, 0.7f, 0.7f, g_orbs[k].col);
    sceGumLoadIdentity();
}

// abanico 2D con degradado del centro al borde (resplandores y nubes)
static void lvFan2D(int cx, int cy, int r, unsigned int cIn, unsigned int cOut, int seg) {
    GradVertex *v = (GradVertex *)sceGuGetMemory(sizeof(GradVertex) * 3 * seg);
    for (int s = 0; s < seg; ++s) {
        const float a0 = 6.2831853f * s / seg, a1 = 6.2831853f * (s + 1) / seg;
        v[s * 3 + 0] = { cIn,  (short)cx, (short)cy, 0 };
        v[s * 3 + 1] = { cOut, (short)(cx + cosf(a0) * r), (short)(cy + sinf(a0) * r * 0.62f), 0 };
        v[s * 3 + 2] = { cOut, (short)(cx + cosf(a1) * r), (short)(cy + sinf(a1) * r * 0.62f), 0 };
    }
    sceGuDrawArray(GU_TRIANGLES, GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 3 * seg, 0, v);
}

// TEMBLOR de camara: mientras el serafin habla, y un golpe cuando estalla su luz
static float lvShakeAmp() {
    if (g_lvFloor != 2) return 0.0f;
    float a = 0.0f;
    if (g_bs == BS_TALK || g_bs == BS_CHOICE) a = 0.11f;
    if (g_bsCharge > 0) a += 0.05f;
    if (g_lvFlash > 20) a += 0.30f;
    return a;
}

// cielo 2D de cada piso
static void lvBackdrop() {
    if (g_lvFloor == 0) { swBackdrop(); return; }
    if (g_lvFloor == 1) {   // cielo "pintado": azul limpio, como un decorado... hasta que se APAGA
        const unsigned int K = RGBA(0, 0, 0, 255);
        const float k = g_f1Dark;
        gradQuad(0, 140, lvMix(RGBA(92, 140, 196, 255), K, k), lvMix(RGBA(196, 214, 232, 255), K, k));
        gradQuad(140, SCR_HEIGHT, lvMix(RGBA(196, 214, 232, 255), K, k), lvMix(RGBA(170, 190, 210, 255), K, k));
    } else {                // cielo de LUZ: blanco-amarillo, nubes y neblina que pasan lento
        gradQuad(0, 120, RGBA(226, 222, 206, 255), RGBA(255, 250, 232, 255));
        gradQuad(120, SCR_HEIGHT, RGBA(255, 250, 232, 255), RGBA(242, 236, 218, 255));
        sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
        for (int k = 0; k < 9; ++k) {
            const int x = ((k * 131 + (int)(g_lvT * 0.18f * (1 + k % 3))) % 680) - 100;
            const int y = 24 + (k * 53) % 190, r = 60 + (k * 29) % 70;
            lvFan2D(x, y, r, RGBA(255, 255, 250, 150), RGBA(255, 252, 240, 0), 16);
        }
        sceGuDisable(GU_BLEND);
    }
}

// ---------------------------------------------------------------- proyeccion + HUD
static int lvProject(float x, float y, float z, int &sx, int &sy) {
    const float dx = x - g_lvEye[0], dy = y - g_lvEye[1], dz = z - g_lvEye[2];
    const float zv = dx * g_lvFwd[0] + dy * g_lvFwd[1] + dz * g_lvFwd[2]; if (zv < 0.5f) return 0;
    const float xv = dx * g_lvRight[0] + dy * g_lvRight[1] + dz * g_lvRight[2];
    const float yv = dx * g_lvUp[0] + dy * g_lvUp[1] + dz * g_lvUp[2];
    sx = 240 + (int)(207.9f * xv / zv); sy = 136 - (int)(209.4f * yv / zv);
    return (sx > -20 && sx < 500 && sy > -20 && sy < 292);
}
static void lvSetCamera(const ScePspFVector3 &eye, const ScePspFVector3 &ctr, const ScePspFVector3 &up) {
    float f[3] = { ctr.x - eye.x, ctr.y - eye.y, ctr.z - eye.z };
    float l = sqrtf(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]) + 1e-6f; for (int k = 0; k < 3; ++k) f[k] /= l;
    float r[3] = { f[1] * up.z - f[2] * up.y, f[2] * up.x - f[0] * up.z, f[0] * up.y - f[1] * up.x };
    l = sqrtf(r[0] * r[0] + r[1] * r[1] + r[2] * r[2]) + 1e-6f; for (int k = 0; k < 3; ++k) r[k] /= l;
    g_lvEye[0] = eye.x; g_lvEye[1] = eye.y; g_lvEye[2] = eye.z;
    for (int k = 0; k < 3; ++k) { g_lvFwd[k] = f[k]; g_lvRight[k] = r[k]; }
    g_lvUp[0] = r[1] * f[2] - r[2] * f[1]; g_lvUp[1] = r[2] * f[0] - r[0] * f[2]; g_lvUp[2] = r[0] * f[1] - r[1] * f[0];
}

static void lvHud(int aiming, float hp, float hpMax) {
    (void)hp; (void)hpMax;
    sceGuDisable(GU_DEPTH_TEST); sceGuDisable(GU_TEXTURE_2D);
    sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    // LUZ CEGADORA del serafin: resplandor blanco-amarillo SUMADO alrededor de el (late; al
    // cargar su ataque se enciende al maximo) + un velo calido sobre toda la pantalla
    if (g_lvFloor == 2 && (g_bs != BS_DEAD || g_bsDeadT < 150)) {
        const float p = sinf(g_lvT * 0.05f);
        int sx, sy;
        if (lvProject(g_bsX, g_bsY, g_bsZ, sx, sy)) {
            // "un poco" cegadora: el ofanim se tiene que seguir leyendo dentro de su luz
            const int I = (g_bsCharge > 0) ? 255 : 105 + (int)(30.0f * p);
            sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_FIX, 0, 0xFFFFFFFF);    // aditivo: suma luz
            lvFan2D(sx, sy, 190 + (int)(22.0f * p), RGBA(255, 242, 196, I), RGBA(255, 232, 160, 0), 24);
            lvFan2D(sx, sy, 70 + (int)(10.0f * p), RGBA(255, 255, 238, I * 3 / 4), RGBA(255, 250, 220, 0), 20);
            sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
        }
        drawRect(0, 0, SCR_WIDTH, SCR_HEIGHT, RGBA(255, 246, 214, 34 + (int)(16.0f * p)));
    }
    // marcador de autoapuntado
    float tp[3];
    if (aiming && lvAimTarget(tp)) {
        int sx, sy;
        if (lvProject(tp[0], tp[1], tp[2], sx, sy)) {
            const unsigned int c = RGBA(255, 70, 70, 230);
            drawRect(sx - 10, sy - 10, 6, 2, c); drawRect(sx - 10, sy - 10, 2, 6, c);
            drawRect(sx + 4, sy - 10, 6, 2, c);  drawRect(sx + 8, sy - 10, 2, 6, c);
            drawRect(sx - 10, sy + 8, 6, 2, c);  drawRect(sx - 10, sy + 4, 2, 6, c);
            drawRect(sx + 4, sy + 8, 6, 2, c);   drawRect(sx + 8, sy + 4, 2, 6, c);
        }
    }
    if (g_lvFlash > 0) drawRect(0, 0, SCR_WIDTH, SCR_HEIGHT, RGBA(255, 250, 236, (g_lvFlash * 255) / 45 > 255 ? 255 : (g_lvFlash * 255) / 45));
    if (g_bsCharge > 0 && (g_lvT & 8)) drawRect(0, 0, SCR_WIDTH, SCR_HEIGHT, RGBA(255, 244, 210, 40));
    // paneles
    const int dialog = (g_bs == BS_TALK || g_bs == BS_CHOICE) && g_lvFloor == 2;
    if (dialog) drawRect(30, 190, 420, 60, RGBA(10, 12, 18, 210));
    if (g_lvMenu) drawRect(150, 34, 180, 204, RGBA(10, 12, 18, 225));
    if (g_prison > 0) { drawRect(140, 150, 200, 8, RGBA(30, 30, 40, 220)); drawRect(140, 150, (int)(200.0f * (g_prAcc / 7.0f > 1.0f ? 1.0f : g_prAcc / 7.0f)), 8, RGBA(170, 140, 255, 240)); }
    if (g_lvFloor == 2 && g_rosary) {
        drawRect(8, 58, 84, 12, RGBA(10, 12, 18, 190));
        drawRect(10, 66, (int)(80.0f * (1.0f - g_rosaryCd / 600.0f)), 2, RGBA(236, 206, 120, 255));
    }
    // fundido + PANTALLA DE CARGA entre pisos (cada piso es un capitulo)
    const int loading = (g_lvFade > 30 && g_lvFade <= 120 && !g_lvFadeWhite);
    const int shownFloor = (g_lvFadeTo >= 0) ? g_lvFadeTo : g_lvFloor;
    if (g_lvFade > 0) {
        int a = (g_lvFade > 120) ? (LV_FADE - g_lvFade) * 255 / 30 : (g_lvFade > 30 ? 255 : g_lvFade * 255 / 30);
        if (a < 0) a = 0; if (a > 255) a = 255;
        drawRect(0, 0, SCR_WIDTH, SCR_HEIGHT, g_lvFadeWhite ? RGBA(250, 250, 255, a) : RGBA(6, 7, 10, a));
        if (loading) {
            drawRect(140, 200, 200, 3, RGBA(40, 44, 54, 255));
            drawRect(140, 200, (120 - g_lvFade) * 200 / 90, 3, RGBA(200, 176, 120, 255));
        }
    }
    // textos
    fontTexOn();
    if (g_lvFloor == 2 && g_rosary) drawText(12, 59, 1.0f, RGBA(236, 216, 160, 255), "ROSARIO");
    if (dialog) {
        drawText(40, 198, 1.0f, RGBA(236, 206, 120, 255), "SERAFÍN");
        if (g_bs == BS_TALK) { drawText(40, 214, 1.0f, RGBA(240, 240, 246, 255), kBsLines[g_bsLine]); drawText(380, 236, 1.0f, RGBA(160, 170, 190, 255), "X >"); }
        else {
            drawText(40, 212, 1.0f, g_bsChoice == 0 ? RGBA(255, 255, 255, 255) : RGBA(130, 136, 150, 255), g_bsChoice == 0 ? "> RENDIRSE" : "  RENDIRSE");
            drawText(40, 226, 1.0f, g_bsChoice == 1 ? RGBA(255, 236, 170, 255) : RGBA(130, 136, 150, 255), g_bsChoice == 1 ? "> PENSAR EN DIOS" : "  PENSAR EN DIOS");
        }
    }
    if (g_lvMenu) {
        drawText(198, 40, 1.0f, RGBA(236, 206, 120, 255), "ELEVADOR");
        static const char *kNames[13] = { "", "EL CAMPO", "LA ESCALERA", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???" };
        for (int f = 1; f <= 12; ++f) {
            char b[32]; int n = 0;
            b[n++] = (f == g_lvMenuSel) ? '>' : ' '; b[n++] = ' ';
            if (f >= 10) b[n++] = '1'; b[n++] = (char)('0' + f % 10); b[n++] = ' '; b[n++] = ' ';
            const char *nm = (f <= g_lvUnlocked) ? kNames[f] : "CERRADO";
            while (*nm && n < 30) b[n++] = *nm++;
            b[n] = 0;
            const unsigned int c = (f <= g_lvUnlocked) ? (f == g_lvMenuSel ? RGBA(255, 255, 255, 255) : RGBA(200, 206, 220, 255)) : RGBA(110, 114, 126, 255);
            drawText(166, 46 + f * 14, 1.0f, c, b);
        }
        drawText(160, 226, 1.0f, RGBA(150, 158, 176, 255), "X SUBIR   O SALIR");
    }
    if (g_lvNear && !g_lvMenu && !dialog && g_lvFade == 0)
        drawText(150, 172, 1.0f, RGBA(230, 236, 246, 255), g_lvNear == 1 ? "TRIÁNGULO: GUARDAR" : "TRIÁNGULO: ELEVADOR");
    if (g_prison > 0) drawText(150, 138, 1.0f, RGBA(220, 210, 255, 255), "¡MUEVE EL ANÁLOGO!");
    if (loading) {
        static const char *kTitle[3] = { "EL CIELO", "EL CAMPO", "LA ESCALERA BLANCA" };
        char b[12] = "PISO  ";
        b[5] = (char)('0' + shownFloor % 10); b[6] = 0;
        drawText(240 - 6 * 8, 100, 2.0f, RGBA(236, 238, 244, 255), b);
        const char *t = kTitle[shownFloor > 2 ? 0 : shownFloor];
        const int n = textLen(t);
        drawText(240 - n * 4, 140, 1.0f, RGBA(200, 176, 120, 255), t);
        drawText(240 - 11 * 4, 210, 1.0f, RGBA(120, 126, 140, 255), "CARGANDO...");
    }
    if (g_lvMsgT > 0 && g_lvMsg && !loading) {
        const int n = textLen(g_lvMsg);
        drawText(240 - n * 4, 96, 1.0f, RGBA(250, 250, 255, g_lvMsgT < 30 ? g_lvMsgT * 8 : 240), g_lvMsg);
    }
    // la mezcla queda ENCENDIDA: el texto que se dibuja despues (dialogos, SELECT) la necesita
    sceGuDisable(GU_TEXTURE_2D); sceGuEnable(GU_DEPTH_TEST);
}
