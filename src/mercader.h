// =============================================================================
// mercader.h - BOTIN de los jefes + EL MERCADER del cielo
// Benjamin (2026-10-16): "tener un mercader para tener mas armas, que los jefes suelten
// items al ser derrotados".
//
// - ECOS = la moneda. Cada jefe suelta ecos CADA VEZ que cae (Baphomet y los jinetes se
//   vuelven a pelear al reentrar: se puede juntar) y su OBJETO solo la primera vez.
//   Cada ojo del piso 1 suelta 3 al morir.
// - El MERCADER (robot oxidado, el modelo del mecanico de robots.h) vive en el cielo, en la
//   losa del elevador. Cambia ecos + objetos de jefe por las 10 armas a distancia de
//   game_data.h. Los objetos de jefe abren las armas tematicas.
// - Antes TODAS las armas hacian 2 de dano en los pisos: ahora cada una el suyo (kMcDmg).
//
// Guardado (campos del NoctisSave que el mundo del cielo no usaba): money = ecos,
// questDone = objetos (bit = piso del jefe), ownedRanged = armas, curRanged = la equipada,
// upgrades = la lanza de Gabriel (bits 0-1: fe 0..3, bit 2: cargada).
//
// LANZA DE GABRIEL (arma 10, Benjamin): no se vende, la deja el arcangel al caer. Se elige como
// cualquier otra (L + ABAJO). Al impactar hace su dano y, aparte, estalla en LUZ (dano de luz).
// Un tiro por carga: se recarga con FE, rezando 3 rosarios (TRIANGULO, cuando curan).
// Solo se guarda en los pilares (lvSaveNow), igual que el piso y las mejoras.
//
// Se incluye al FINAL de levels.h (usa lvDrawAt, lvLookAngles, lvMessage, swLayout...);
// levels.h declara arriba las funciones mc* que llama.
// =============================================================================
#ifndef MERCADER_H
#define MERCADER_H

// objeto de cada jefe, por piso (LVF_*; Gabriel: su "objeto" es la lanza misma)
static const char *kMcItem[11] = { "", "LÁGRIMA DEL CIELO", "", "PLUMA DEL SERAFÍN", "FRASCO DE AZUFRE", "LANZA DE GABRIEL",
                                   "CORONA DEL CONQUISTADOR", "FILO DE LA GUERRA", "BALANZA ROTA", "CENIZA VERDE", "MÁSCARA DE LOS OJOS" };
static const int kMcBossEcos[11] = { 0, 60, 0, 200, 250, 350, 300, 350, 400, 500, 450 };   // por jefe (el piso 1 ademas 3 por ojo)
static const int MC_SPEAR = 10;      // la lanza de Gabriel (kRanged[10])
// tienda (indice = arma de kRanged): precio en ecos y objeto que pide (piso del jefe; 0 = ninguno)
static const int kMcPrice[11] = { 0, 600, 200, 700, 450, 800, 500, 250, 900, 1500, 0 };
static const int kMcNeed[11]  = { 0,   7,   0,   4,   1,   6,   3,   0,   8,    9, 0 };   // piso del jefe (LVF_*; el cementerio no tiene)
// dano por bala en los pisos (los jefes tienen 100..160). La pistola sigue en 2; la lanza 40 + luz.
static const int kMcDmg[11]   = { 2,   9,   4,  20,   2,  17,   5,   3,  12,   30, 40 };
static const int MC_LIGHT_DMG = 25;  // el estallido de luz de la lanza, aparte del impacto

static int      g_mcEcos = 0;
static uint32_t g_mcItems = 0;       // objetos de jefe (bit = piso 1..9)
static uint32_t g_mcOwned = 1;       // armas a distancia (bit 0 = pistola, de fabrica)
static int      g_mcEquip = -1;      // pedido de arma para main (-1 = nada)
static int      g_mcCur = 0;         // arma equipada (main la copia cada frame, para guardarla)
static int      g_mcSvEcos = 0, g_mcSvCur = 0;
static uint32_t g_mcSvItems = 0, g_mcSvOwned = 1;   // lo de la partida guardada (CARGAR PARTIDA)
static int      g_mcShop = 0, g_mcSel = 0, g_mcNearOn = 0, g_mcSayT = 0;
static const char *g_mcSay = 0;      // respuesta del mercader dentro de la tienda
static float    g_mcX = 0.0f, g_mcY = 0.0f, g_mcZ = 0.0f;
static int      g_mcOn = 0;          // hay mercader en este piso (solo en el cielo)
static LineVertex __attribute__((aligned(16))) g_mcV[420];
static int      g_mcN = 0;
static float    g_mcBX = 0.0f, g_mcBZ = 6.0f;       // ultima posicion conocida del jefe (pisos 3..7)
static int      g_mcBossSeen = 0;
static int      g_mcSpearReady = 0, g_mcFe = 0, g_mcSpearMsgT = 0;   // la lanza: cargada / rosarios rezados (0..3)
static int      g_mcSvUpg = 0;
static float    g_mcLbX = 0.0f, g_mcLbY = 0.0f, g_mcLbZ = 0.0f; static int g_mcLbT = 0, g_mcLbFx = 0;   // estallido de luz

struct McDrop { float x, y, z; int item, ecos, on; };
static McDrop g_mcDrop[2];

// ---------------------------------------------------------------- estado y guardado
static void mcInit() {
    int i = 0;
    buildMechanic(g_mcV, i, rFrame(0.0f, 0.0f, 0.0f, -1.0f));   // mira a -Z: lvLookAngles lo gira hacia el jugador
    g_mcN = i;
}
static void mcLoadSave(int ecos, uint32_t items, uint32_t owned, int cur, uint32_t upg) {
    g_mcSvEcos = (ecos < 0) ? 0 : (ecos > 99999 ? 99999 : ecos);
    g_mcSvItems = items & 0x7FEu;   // bits 1..10 (la catedral es el 10)
    g_mcSvOwned = (owned & 0x7FFu) | 1u;
    g_mcSvUpg = (int)(upg & 7u);
    g_mcSvCur = (cur >= 0 && cur < kRangedCount && ((g_mcSvOwned >> cur) & 1u)) ? cur : 0;
}
static void mcNewGame()   { g_mcEcos = 0; g_mcItems = 0; g_mcOwned = 1; g_mcEquip = 0; g_mcSpearReady = 0; g_mcFe = 0; }
static void mcApplySave() {
    g_mcEcos = g_mcSvEcos; g_mcItems = g_mcSvItems; g_mcOwned = g_mcSvOwned; g_mcEquip = g_mcSvCur;
    g_mcFe = g_mcSvUpg & 3; if (g_mcFe > 3) g_mcFe = 3; g_mcSpearReady = (g_mcSvUpg >> 2) & 1;
}
static void mcSave(NoctisSave &sv) {
    sv.money = g_mcEcos; sv.questDone = g_mcItems; sv.ownedRanged = g_mcOwned; sv.curRanged = g_mcCur;
    sv.upgrades = (uint32_t)((g_mcFe & 3) | (g_mcSpearReady ? 4 : 0));
}
// ---------------------------------------------------------------- la lanza de Gabriel
static void mcOnRosary() {   // cada rosario rezado (cuando cura) suma fe; con 3, la lanza vuelve
    if (!((g_mcOwned >> MC_SPEAR) & 1u) || g_mcSpearReady) return;
    if (++g_mcFe >= 3) { g_mcFe = 0; g_mcSpearReady = 1; lvMessage("TU FE RECARGÓ LA LANZA DE GABRIEL", 160); audioBell(); }
}
static void mcSpearThrown() { g_mcSpearReady = 0; g_mcFe = 0; }
static void mcSpearEmpty() {
    if (g_mcSpearMsgT > 0) return;
    static char m[48] = "LA LANZA ESPERA TU FE: 0/3 ROSARIOS";
    m[22] = (char)('0' + g_mcFe);
    lvMessage(m, 110); g_mcSpearMsgT = 120;
}
static void mcLightBurst(float x, float y, float z) { g_mcLbX = x; g_mcLbY = y; g_mcLbZ = z; g_mcLbT = 10; g_mcLbFx = 36; }
static void mcAddEcos(int n) { g_mcEcos += n; if (g_mcEcos > 99999) g_mcEcos = 99999; }

// ---------------------------------------------------------------- botin
// suelo (techo de la caja mas alta) bajo (x,z); -1e6 si no hay nada
static float mcGroundAt(float x, float z) {
    float top = -1.0e6f;
    for (int k = 0; k < g_cityCount; ++k) {
        const CityBldg &b = g_city[k];
        if (fabsf(x - b.x) <= b.w * 0.5f && fabsf(z - b.z) <= b.d * 0.5f && b.h > top) top = b.h;
    }
    return top;
}
// deja el botin en el suelo cerca de (x,z). Si ahi no hay suelo alcanzable (el jefe cayo
// sobre el vacio o sobre una roca alta) lo pone frente al centro, y si no, junto a la llegada.
static void mcDrop(float x, float z, float refY, int item, int ecos) {
    float g = mcGroundAt(x, z);
    if (g < refY - 3.0f || g > refY + 6.0f) {
        x = 0.0f; z = 7.0f; g = mcGroundAt(x, z);
        if (g < refY - 3.0f || g > refY + 6.0f) { x = g_lvSpawnX; z = g_lvSpawnZ - 6.0f; g = refY; }
    }
    for (int k = 0; k < 2; ++k)
        if (!g_mcDrop[k].on) { g_mcDrop[k].x = x; g_mcDrop[k].y = g + 1.4f; g_mcDrop[k].z = z;
                               g_mcDrop[k].item = item; g_mcDrop[k].ecos = ecos; g_mcDrop[k].on = 1; return; }
}
// un jefe cayo: ecos siempre, su objeto solo si todavia no lo tienes
static void mcBossDown(int floor, float x, float z, float refY) {
    if (floor < 1 || floor > LVF_LAST) return;
    const int item = ((g_mcItems >> floor) & 1u) ? 0 : floor;
    mcDrop(x, z, refY, item, kMcBossEcos[floor]);
}
static void mcTrackBoss(float x, float z) { g_mcBX = x; g_mcBZ = z; }
// pisos 3..7: llamar cada frame con LV_BOSS(Done); suelta el botin en el frame en que cae
#ifndef NOCTIS_TEST_BOTIN
#define NOCTIS_TEST_BOTIN 0   // PRUEBA: 1 = el jefe "cae" a los 3 s de entrar (ver el botin sin pelear). 0 al commitear
#endif
static void mcBossCheck(int done, int floor, float refY) {
    if (NOCTIS_TEST_BOTIN && g_lvT >= 180) done = 1;
    if (done && !g_mcBossSeen && floor == LVF_GABRIEL) {   // GABRIEL: ecos y su lanza, en la mano
        g_mcBossSeen = 1; mcAddEcos(kMcBossEcos[LVF_GABRIEL]);
        if (!((g_mcOwned >> MC_SPEAR) & 1u)) { g_mcOwned |= 1u << MC_SPEAR; g_mcItems |= 1u << LVF_GABRIEL; g_mcSpearReady = 1; g_mcFe = 0; }
        return;
    }
    if (done && !g_mcBossSeen) { g_mcBossSeen = 1; mcBossDown(floor, g_mcBX, g_mcBZ, refY); }
}
static void mcEnterFloor(int floor) {
    g_mcDrop[0].on = g_mcDrop[1].on = 0; g_mcBossSeen = 0; g_mcBX = 0.0f; g_mcBZ = 7.0f;
    g_mcShop = 0; g_mcNearOn = 0;
    g_mcOn = (floor == 0);
    if (g_mcOn) {   // en la losa del elevador, a la izquierda del camino, antes de llegar
        SwLayout L; swLayout(0, SW_ELEV_CZ, L);
        g_mcX = L.x - L.w * 0.28f; g_mcZ = L.z + L.d * 0.18f; g_mcY = L.top;
    }
}

// ---------------------------------------------------------------- logica
static int  mcNear()   { return g_mcNearOn && !g_mcShop; }
static int  mcBlocks() { return g_mcShop; }
static void mcOpen() {
    g_mcShop = 1; g_mcSel = 0; g_mcSayT = 0;
    for (int w = 1; w < kRangedCount; ++w) if (!((g_mcOwned >> w) & 1u)) { g_mcSel = w; break; }   // la primera que falta
}
static void mcSay(const char *s) { g_mcSay = s; g_mcSayT = 150; }
// devuelve 1 si la tienda esta abierta (consume los botones del frame)
static int mcUpdate(float px, float py, float pz, int edge) {
    for (int k = 0; k < 2; ++k) {                    // recoger botin
        McDrop &d = g_mcDrop[k]; if (!d.on) continue;
        if (lvDist2(px, py + 1.0f, pz, d.x, d.y, d.z) < 2.6f * 2.6f) {
            static char msg[72]; int n = 0;
            auto put = [&](const char *s) { while (*s && n < 70) msg[n++] = *s++; };
            mcAddEcos(d.ecos);
            if (d.item) { g_mcItems |= 1u << d.item; put("OBTIENES: "); put(kMcItem[d.item]); put("  "); }
            char num[8]; int q = d.ecos, t = 0; do { num[t++] = (char)('0' + q % 10); q /= 10; } while (q && t < 7);
            msg[n++] = '+'; while (t) msg[n++] = num[--t]; put(" ECOS"); msg[n] = 0;
            lvMessage(msg, 170); audioBell();
            d.on = 0;
        }
    }
    if (g_mcSpearMsgT > 0) --g_mcSpearMsgT;
    if (g_mcLbFx > 0) --g_mcLbFx;
    if (g_mcLbT > 0 && --g_mcLbT == 0) lvShotHit(g_mcLbX, g_mcLbY, g_mcLbZ, MC_LIGHT_DMG);   // la LUZ, aparte del impacto
    g_mcNearOn = g_mcOn && lvDist2(px, 0.0f, pz, g_mcX, 0.0f, g_mcZ) < 4.6f * 4.6f && fabsf(py - g_mcY) < 2.5f;
    if (g_mcSayT > 0) --g_mcSayT;
    if (!g_mcShop) return 0;
    if (edge & PSP_CTRL_UP)   { if (--g_mcSel < 0) g_mcSel = kRangedCount - 1; }
    if (edge & PSP_CTRL_DOWN) { if (++g_mcSel >= kRangedCount) g_mcSel = 0; }
    if (edge & PSP_CTRL_CIRCLE) { g_mcShop = 0; return 1; }
    if (edge & PSP_CTRL_CROSS) {
        const int w = g_mcSel, need = kMcNeed[w];
        if (w == MC_SPEAR && !((g_mcOwned >> w) & 1u)) mcSay("Esa no se vende. Pertenece a un arcángel.");
        else if ((g_mcOwned >> w) & 1u) {
            if (w == g_mcCur) mcSay("Ya la lleva en la mano.");
            else { g_mcEquip = w; mcSay("Bien. Esa es la suya ahora."); }
        } else if (need && !((g_mcItems >> need) & 1u)) mcSay("Sin lo que pide, no la abro. Mire abajo.");
        else if (g_mcEcos < kMcPrice[w]) mcSay("No le alcanzan los ecos. Vuelva con mas.");
        else {
            g_mcEcos -= kMcPrice[w]; g_mcOwned |= 1u << w; g_mcEquip = w;
            mcSay("Hecho. Usela antes de que despierte."); audioBell();
        }
    }
    return 1;
}

// ---------------------------------------------------------------- dibujo 3D
// octaedro + columna de luz: oro si trae objeto, cian si son solo ecos
static void mcDrawLoot(float x, float y, float z, int gold) {
    const float t = g_lvT * 0.05f, r = 0.55f, h = 0.8f, yy = y + 0.25f * sinf(t * 1.3f);
    const unsigned int cT = gold ? RGBA(255, 226, 130, 255) : RGBA(170, 236, 255, 255);
    const unsigned int cS = gold ? RGBA(200, 150, 60, 255)  : RGBA(90, 160, 210, 255);
    LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 24);
    const float c = cosf(t), s = sinf(t);
    float px[4], pz[4];
    for (int k = 0; k < 4; ++k) { const float a = k * 1.5707963f; px[k] = (cosf(a) * c - sinf(a) * s) * r; pz[k] = (cosf(a) * s + sinf(a) * c) * r; }
    int n = 0;
    for (int k = 0; k < 4; ++k) {
        const int j = (k + 1) & 3; const unsigned int cs = (k & 1) ? cS : brighten(cS, 1.25f);
        v[n++] = { cT, x, yy + h, z }; v[n++] = { cs, x + px[k], yy, z + pz[k] }; v[n++] = { cs, x + px[j], yy, z + pz[j] };
        v[n++] = { cS, x, yy - h, z }; v[n++] = { cs, x + px[j], yy, z + pz[j] }; v[n++] = { cs, x + px[k], yy, z + pz[k] };
    }
    sceGumLoadIdentity(); sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, n, 0, v);
    // columna de luz SUMADA que se ve de lejos (dos cintas cruzadas que se apagan hacia arriba)
    LineVertex *b = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 12); int m = 0;
    const unsigned int c0 = gold ? RGBA(255, 220, 120, 120) : RGBA(150, 220, 255, 110), c1 = (c0 & 0x00FFFFFFu);
    swQuadC(b, m, x - 0.45f, y - 1.3f, z, c0, x + 0.45f, y - 1.3f, z, c0, x + 0.45f, y + 9.0f, z, c1, x - 0.45f, y + 9.0f, z, c1);
    swQuadC(b, m, x, y - 1.3f, z - 0.45f, c0, x, y - 1.3f, z + 0.45f, c0, x, y + 9.0f, z + 0.45f, c1, x, y + 9.0f, z - 0.45f, c1);
    sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_FIX, 0, 0xFFFFFFFF); sceGuDepthMask(GU_TRUE);
    sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, m, 0, b);
    sceGuDepthMask(GU_FALSE); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0); sceGuDisable(GU_BLEND);
}
static void mcDraw(float px, float py, float pz) {
    for (int k = 0; k < 2; ++k) if (g_mcDrop[k].on) mcDrawLoot(g_mcDrop[k].x, g_mcDrop[k].y, g_mcDrop[k].z, g_mcDrop[k].item != 0);
    if (g_mcLbFx > 0) {   // estallido de LUZ de la lanza: esfera blanca que crece y anillos
        const float p = 1.0f - g_mcLbFx / 36.0f, R = 1.0f + 7.0f * p;
        const unsigned int cIn = RGBA(255, 252, 235, (int)(230.0f * (1.0f - p))), cOut = RGBA(255, 240, 190, 0);
        LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 3 * 16);
        for (int s = 0; s < 16; ++s) {
            const float a0 = 6.2831853f * s / 16, a1 = 6.2831853f * (s + 1) / 16;
            const float c0 = cosf(a0) * R, s0 = sinf(a0) * R, c1 = cosf(a1) * R, s1 = sinf(a1) * R;
            v[s * 3 + 0] = { cIn, g_mcLbX, g_mcLbY, g_mcLbZ };
            v[s * 3 + 1] = { cOut, g_mcLbX + g_lvRight[0] * c0 + g_lvUp[0] * s0, g_mcLbY + g_lvRight[1] * c0 + g_lvUp[1] * s0, g_mcLbZ + g_lvRight[2] * c0 + g_lvUp[2] * s0 };
            v[s * 3 + 2] = { cOut, g_mcLbX + g_lvRight[0] * c1 + g_lvUp[0] * s1, g_mcLbY + g_lvRight[1] * c1 + g_lvUp[1] * s1, g_mcLbZ + g_lvRight[2] * c1 + g_lvUp[2] * s1 };
        }
        sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_FIX, 0, 0xFFFFFFFF); sceGuDepthMask(GU_TRUE);
        sceGumLoadIdentity(); sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, 48, 0, v);
        lvRing3D(g_mcLbX, g_mcLbY, g_mcLbZ, R * 1.3f, 0.4f, RGBA(255, 246, 200, (int)(200.0f * (1.0f - p))));
        sceGuDepthMask(GU_FALSE); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0); sceGuDisable(GU_BLEND);
    }
    if (!g_mcOn || g_menu) return;
    float yaw, pitch; lvLookAngles(g_mcX, g_mcY + 1.5f, g_mcZ, px, py + 1.7f, pz, yaw, pitch);
    lvDrawAt(g_mcV, g_mcN, g_mcX, g_mcY, g_mcZ, yaw, 0.0f, 1.3f);
    // farol: una luz calida que cuelga sobre el, para encontrarlo desde el camino
    const float b = 0.5f + 0.5f * sinf(g_lvT * 0.07f);
    lvBox(g_mcX, g_mcY + 3.6f + 0.1f * b, g_mcZ, 0.42f, 0.42f, RGBA(255, 200 + (int)(40 * b), 120, 255));
}

// ---------------------------------------------------------------- HUD
static int mcItoa(char *b, int q) {
    char t[8]; int k = 0, n = 0; if (q < 0) q = 0;
    do { t[k++] = (char)('0' + q % 10); q /= 10; } while (q && k < 7);
    while (k) b[n++] = t[--k]; b[n] = 0; return n;
}
static void mcHud() {
    if (g_menu || g_cine > 0) return;
    char b[40];
    {   // contador de ecos (arriba a la izquierda, bajo ROSARIO y TIEMPO)
        int n = 0; const char *l = "ECOS "; while (*l) b[n++] = *l++;
        mcItoa(b + n, g_mcEcos);
        fontTexOn(); drawText(12, 87, 1.0f, RGBA(150, 220, 255, 255), b); sceGuDisable(GU_TEXTURE_2D);
    }
    if (mcNear() && !g_lvMenu) { fontTexOn(); drawText(150, 172, 1.0f, RGBA(230, 236, 246, 255), "TRIÁNGULO: COMERCIAR"); sceGuDisable(GU_TEXTURE_2D); }
    if (g_mcCur == MC_SPEAR && !g_mcShop) {   // la lanza en la mano: cargada o cuanta fe le falta
        int n = 0; const char *l = g_mcSpearReady ? "LANZA LISTA" : "FE "; while (*l) b[n++] = *l++;
        if (!g_mcSpearReady) { b[n++] = (char)('0' + g_mcFe); b[n++] = '/'; b[n++] = '3'; }
        b[n] = 0;
        fontTexOn(); drawText(12, 101, 1.0f, g_mcSpearReady ? RGBA(255, 236, 160, 255) : RGBA(170, 176, 196, 255), b); sceGuDisable(GU_TEXTURE_2D);
    }
    if (!g_mcShop) return;
    const unsigned int GOLD = RGBA(236, 206, 120, 255), WHITE = RGBA(240, 240, 246, 255), DIM = RGBA(120, 126, 142, 255);
    sceGuDisable(GU_TEXTURE_2D);
    drawRect(36, 18, 408, 238, RGBA(8, 10, 16, 232));
    drawRect(36, 18, 408, 1, RGBA(200, 176, 120, 200));
    fontTexOn();
    drawText(48, 26, 1.0f, GOLD, "EL MERCADER");
    { int n = 0; const char *l = "ECOS "; while (*l) b[n++] = *l++; n += mcItoa(b + n, g_mcEcos);
      drawText(432 - n * 8, 26, 1.0f, RGBA(150, 220, 255, 255), b); }
    drawText(48, 42, 1.0f, RGBA(196, 200, 212, 255),
             (g_mcSayT > 0 && g_mcSay) ? g_mcSay : "Nadie firmó la orden de parar. Sigo vendiendo.");
    for (int w = 0; w < kRangedCount; ++w) {
        const int y = 62 + w * 13;
        const int owned = (g_mcOwned >> w) & 1u, need = kMcNeed[w], have = !need || ((g_mcItems >> need) & 1u);
        const unsigned int c = (w == g_mcSel) ? WHITE : (owned || have) ? RGBA(190, 196, 210, 255) : DIM;
        if (w == g_mcSel) drawText(44, y, 1.0f, GOLD, ">");
        drawText(56, y, 1.0f, c, (w == MC_SPEAR && !owned) ? "???" : kRanged[w].name);
        if (w == g_mcCur) drawText(340, y, 1.0f, GOLD, "EN MANO");
        else if (w == MC_SPEAR && !owned) drawText(340, y, 1.0f, DIM, "NO SE VENDE");
        else if (owned) drawText(340, y, 1.0f, RGBA(140, 210, 150, 255), "TUYA");
        else { int n = mcItoa(b, kMcPrice[w]); if (need) { b[n++] = ' '; b[n++] = '+'; b[n] = 0; }
               drawText(340, y, 1.0f, have ? RGBA(150, 220, 255, 255) : DIM, b); }
    }
    {   // detalle del arma elegida: dano real en los pisos, cargador y lo que pide
        const int w = g_mcSel, need = kMcNeed[w];
        int n = 0; const char *l = "DAÑO "; while (*l) b[n++] = *l++;
        n += mcItoa(b + n, kMcDmg[w]);
        if (w == MC_SPEAR) { l = " + LUZ "; while (*l) b[n++] = *l++; n += mcItoa(b + n, MC_LIGHT_DMG); l = "   FE "; while (*l) b[n++] = *l++;
                             b[n++] = (char)('0' + (g_mcSpearReady ? 3 : g_mcFe)); b[n++] = '/'; b[n++] = '3'; b[n] = 0; }
        else { l = "   CARGADOR "; while (*l) b[n++] = *l++; mcItoa(b + n, kRanged[w].magazine); }
        drawText(48, 198, 1.0f, RGBA(196, 200, 212, 255), b);
        if (need) {
            const int have = (g_mcItems >> need) & 1u;
            drawText(48, 212, 1.0f, have ? RGBA(140, 210, 150, 255) : RGBA(220, 120, 110, 255), "PIDE: ");
            drawText(96, 212, 1.0f, have ? RGBA(140, 210, 150, 255) : RGBA(220, 120, 110, 255), kMcItem[need]);
        }
    }
    drawText(48, 234, 1.0f, DIM, "X COMPRAR / EQUIPAR    O SALIR");
    sceGuDisable(GU_TEXTURE_2D);
}

#endif // MERCADER_H
