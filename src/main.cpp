// PROJECT NOCTIS - Motor + mundo navegable + gravedad + HUD (vertical slice).
// Distrito "Campanario": torres goticas en wireframe, jugador movible con el
// stick, salto/gravedad vertical, camara 3a persona, HUD estilo mockup
// (barras HP/EN/GRV + arma). Render: GU_LINES (3D) y GU_SPRITES (2D).

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspgu.h>
#include <pspgum.h>
#include <cmath>
#include <cstdio>

#include "font8x8_basic.h" // unsigned char font8x8_basic[128][8], dominio publico
#include "world_data.h"    // kStructures[], kResources[]
#include "game_data.h"     // kRanged[], kMelee[], kMaterials[], kEnemies[]

PSP_MODULE_INFO("NOCTIS", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

#define BUF_WIDTH  512
#define SCR_WIDTH  480
#define SCR_HEIGHT 272

#define DEG2RAD(d) ((d) * 0.0174532925f)
#define RGBA(r, g, b, a) (((a) << 24) | ((b) << 16) | ((g) << 8) | (r)) // 0xAABBGGRR

static unsigned int __attribute__((aligned(16))) g_list[262144];
static const unsigned int CLEAR_COLOR = RGBA(16, 14, 20, 255);

#define LINE_FLAGS (GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D)

// ================= geometria (lineas 3D) =================
struct LineVertex { unsigned int color; float x, y, z; };

// --- piso ---
#define GRID_HALF 48
static LineVertex __attribute__((aligned(16))) g_grid[(2 * GRID_HALF + 1) * 4];
static int g_gridVerts = 0;

static void buildGrid() {
    int i = 0;
    const float h = (float)GRID_HALF;
    const unsigned int col     = RGBA(40, 48, 66, 255);
    const unsigned int colAxis = RGBA(64, 72, 96, 255);
    for (int k = -GRID_HALF; k <= GRID_HALF; ++k) {
        const float f = (float)k;
        const unsigned int c = (k == 0) ? colAxis : col;
        g_grid[i++] = { c, -h, 0.0f, f };
        g_grid[i++] = { c,  h, 0.0f, f };
        g_grid[i++] = { c, f, 0.0f, -h };
        g_grid[i++] = { c, f, 0.0f,  h };
    }
    g_gridVerts = i;
}

// --- aristas de caja (12 aristas -> 24 vertices) ---
static void addBoxEdges(LineVertex *buf, int &i, float cx, float baseY, float cz,
                        float w, float d, float h, unsigned int col) {
    const float x0 = cx - w * 0.5f, x1 = cx + w * 0.5f;
    const float z0 = cz - d * 0.5f, z1 = cz + d * 0.5f;
    const float y0 = baseY,          y1 = baseY + h;
    const float X[8] = { x0, x1, x1, x0, x0, x1, x1, x0 };
    const float Y[8] = { y0, y0, y0, y0, y1, y1, y1, y1 };
    const float Z[8] = { z0, z0, z1, z1, z0, z0, z1, z1 };
    static const int E[12][2] = {
        {0,1},{1,2},{2,3},{3,0}, {4,5},{5,6},{6,7},{7,4}, {0,4},{1,5},{2,6},{3,7}
    };
    for (int e = 0; e < 12; ++e) {
        buf[i++] = { col, X[E[e][0]], Y[E[e][0]], Z[E[e][0]] };
        buf[i++] = { col, X[E[e][1]], Y[E[e][1]], Z[E[e][1]] };
    }
}

// aclara un color RGBA multiplicando el RGB (para que el wireframe se lea)
static unsigned int brighten(unsigned int c, float f) {
    int r = (int)((c & 0xFF) * f);
    int g = (int)(((c >> 8) & 0xFF) * f);
    int b = (int)(((c >> 16) & 0xFF) * f);
    if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
    return RGBA(r, g, b, 255);
}

// --- distrito ---
static LineVertex __attribute__((aligned(16))) g_world[64 * 24];
static int g_worldVerts = 0;

static void buildWorld() {
    int i = 0;
    for (int s = 0; s < kStructureCount && s < 63; ++s) {
        const Structure &st = kStructures[s];
        addBoxEdges(g_world, i, st.x, st.y, st.z, st.w, st.d, st.h, brighten(st.color, 2.6f));
    }
    // The Cathedral: silueta monumental y lejana (placeholder, seccion 26)
    addBoxEdges(g_world, i, 0.0f, 0.0f, -240.0f, 70.0f, 70.0f, 380.0f, RGBA(120, 108, 150, 255));
    g_worldVerts = i;
}

// altura del suelo bajo el jugador (0, o el techo de una estructura si esta
// por encima de el). Permite pararse en azoteas al caer sobre ellas.
static float groundHeight(float px, float pz, float py) {
    float g = 0.0f;
    for (int s = 0; s < kStructureCount; ++s) {
        const Structure &st = kStructures[s];
        const float x0 = st.x - st.w * 0.5f, x1 = st.x + st.w * 0.5f;
        const float z0 = st.z - st.d * 0.5f, z1 = st.z + st.d * 0.5f;
        if (px >= x0 && px <= x1 && pz >= z0 && pz <= z1) {
            const float top = st.y + st.h;
            if (top > g && top <= py + 1.0f) g = top;
        }
    }
    return g;
}

// --- caja del jugador (local, se traslada con MODEL) ---
static LineVertex __attribute__((aligned(16))) g_playerBox[24];
static int g_playerBoxVerts = 0;

static void buildPlayerBox() {
    int i = 0;
    addBoxEdges(g_playerBox, i, 0.0f, 0.0f, 0.0f, 1.4f, 1.4f, 3.0f, RGBA(235, 130, 90, 255));
    g_playerBoxVerts = i;
}

// --- NPCs roboticos (cuerpo + cabeza, wireframe) ---
struct Npc { float x, z; };
static const Npc kNpcs[] = {
    { 6.0f, 4.0f }, { -5.0f, 7.0f }, { 8.0f, -3.0f },
    { -7.0f, -6.0f }, { 3.5f, 11.0f }, { -10.0f, 2.5f }
};
static const int kNpcCount = (int)(sizeof(kNpcs) / sizeof(kNpcs[0]));
static LineVertex __attribute__((aligned(16))) g_npc[kNpcCount * 48];
static int g_npcVerts = 0;

static void buildNpcs() {
    int i = 0;
    const unsigned int body = RGBA(80, 190, 180, 255);
    const unsigned int head = RGBA(150, 235, 225, 255);
    for (int n = 0; n < kNpcCount; ++n) {
        addBoxEdges(g_npc, i, kNpcs[n].x, 0.0f, kNpcs[n].z, 0.8f, 0.6f, 1.5f, body);
        addBoxEdges(g_npc, i, kNpcs[n].x, 1.5f, kNpcs[n].z, 0.55f, 0.55f, 0.55f, head);
    }
    g_npcVerts = i;
}

// ================= fuente bitmap y rectangulos (sprites 2D) =================
static unsigned int __attribute__((aligned(16))) g_fontAtlas[128 * 128];

static void buildFontAtlas() {
    for (int i = 0; i < 128 * 128; ++i) g_fontAtlas[i] = 0;
    for (int c = 0; c < 128; ++c) {
        const int cx = (c % 16) * 8, cy = (c / 16) * 8;
        for (int row = 0; row < 8; ++row) {
            const unsigned char bits = font8x8_basic[c][row];
            for (int col = 0; col < 8; ++col)
                if (bits & (1 << col))
                    g_fontAtlas[(cy + row) * 128 + (cx + col)] = 0xFFFFFFFF;
        }
    }
    sceKernelDcacheWritebackAll();
}

struct SpriteVertex { unsigned short u, v; short x, y, z; };
struct PlainVertex  { short x, y, z; };

// rectangulo solido 2D (textura desactivada). color con alpha para paneles.
static void drawRect(int x, int y, int w, int h, unsigned int color) {
    sceGuColor(color);
    PlainVertex *v = (PlainVertex *)sceGuGetMemory(sizeof(PlainVertex) * 2);
    v[0] = { (short)x, (short)y, 0 };
    v[1] = { (short)(x + w), (short)(y + h), 0 };
    sceGuDrawArray(GU_SPRITES, GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, v);
}

// texto 2D (requiere textura de fuente activada por el que llama)
static void drawText(int x, int y, float scale, unsigned int color, const char *text) {
    int len = 0;
    for (const char *p = text; *p; ++p) len++;
    if (len == 0) return;
    sceGuColor(color);
    SpriteVertex *v = (SpriteVertex *)sceGuGetMemory(sizeof(SpriteVertex) * 2 * len);
    const int cw = (int)(8 * scale), ch = (int)(8 * scale);
    int n = 0, px = x;
    for (const char *p = text; *p; ++p) {
        unsigned char c = (unsigned char)*p;
        if (c >= 128) c = '?';
        const int cx = (c % 16) * 8, cy = (c / 16) * 8;
        v[n].u = cx;     v[n].v = cy;     v[n].x = px;      v[n].y = y;      v[n].z = 0; n++;
        v[n].u = cx + 8; v[n].v = cy + 8; v[n].x = px + cw; v[n].y = y + ch; v[n].z = 0; n++;
        px += cw;
    }
    sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D, n, 0, v);
}

static void fontTexOn() {
    sceGuEnable(GU_TEXTURE_2D);
    sceGuTexMode(GU_PSM_8888, 0, 0, 0);
    sceGuTexImage(0, 128, 128, 128, g_fontAtlas);
    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
    sceGuTexFilter(GU_NEAREST, GU_NEAREST);
}

// ================= salida (boton HOME) =================
static volatile int g_exit = 0;
static int exitCallback(int, int, void *) { g_exit = 1; return 0; }
static int callbackThread(SceSize, void *) {
    int cbid = sceKernelCreateCallback("Exit Callback", exitCallback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}
static void setupCallbacks() {
    int thid = sceKernelCreateThread("cb_thread", callbackThread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0) sceKernelStartThread(thid, 0, 0);
}

// ================= GU init =================
static void *g_fbp0, *g_fbp1, *g_zbp;

static void initGu() {
    g_fbp0 = guGetStaticVramBuffer(BUF_WIDTH, SCR_HEIGHT, GU_PSM_8888);
    g_fbp1 = guGetStaticVramBuffer(BUF_WIDTH, SCR_HEIGHT, GU_PSM_8888);
    g_zbp  = guGetStaticVramBuffer(BUF_WIDTH, SCR_HEIGHT, GU_PSM_4444);

    sceGuInit();
    sceGuStart(GU_DIRECT, g_list);
    sceGuDrawBuffer(GU_PSM_8888, g_fbp0, BUF_WIDTH);
    sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, g_fbp1, BUF_WIDTH);
    sceGuDepthBuffer(g_zbp, BUF_WIDTH);
    sceGuOffset(2048 - (SCR_WIDTH / 2), 2048 - (SCR_HEIGHT / 2));
    sceGuViewport(2048, 2048, SCR_WIDTH, SCR_HEIGHT);
    sceGuDepthRange(65535, 0);
    sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuDepthFunc(GU_GEQUAL);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuDisable(GU_CULL_FACE);
    sceGuDisable(GU_TEXTURE_2D);
    sceGuShadeModel(GU_SMOOTH);
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
}

int main(void) {
    setupCallbacks();
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    buildGrid();
    buildWorld();
    buildPlayerBox();
    buildNpcs();
    buildFontAtlas();
    initGu();

    SceCtrlData pad;
    float playerX = 0.0f, playerY = 0.0f, playerZ = 0.0f;
    float velY = 0.0f;
    int   grounded = 1;
    float camYaw = 0.0f;

    int fps = 0, frameAccum = 0;
    long long lastTick = sceKernelGetSystemTimeWide();
    char hud[80];

    int collected[64] = {0};
    int collectedCount = 0, pickTimer = 0, pickedType = 0;

    while (!g_exit) {
        sceCtrlReadBufferPositive(&pad, 1);
        if (pad.Buttons & PSP_CTRL_START) g_exit = 1;

        // camara (D-pad izq/der)
        if (pad.Buttons & PSP_CTRL_LEFT)  camYaw -= 0.03f;
        if (pad.Buttons & PSP_CTRL_RIGHT) camYaw += 0.03f;

        // movimiento horizontal (stick, ejes de mundo)
        int lx = (int)pad.Lx - 128, ly = (int)pad.Ly - 128;
        const int dead = 24;
        float mx = (lx > dead || lx < -dead) ? lx / 128.0f : 0.0f;
        float mz = (ly > dead || ly < -dead) ? ly / 128.0f : 0.0f;
        const float speed = 0.35f;
        playerX += mx * speed;
        playerZ += mz * speed;

        // salto + gravedad vertical
        if (grounded && (pad.Buttons & PSP_CTRL_CROSS)) { velY = 0.55f; grounded = 0; }
        velY -= 0.02f;
        playerY += velY;
        float gh = groundHeight(playerX, playerZ, playerY);
        if (playerY <= gh) { playerY = gh; velY = 0.0f; grounded = 1; }

        // recoleccion de recursos por proximidad
        for (int r = 0; r < kResourceCount && r < 64; ++r) {
            if (collected[r]) continue;
            float dx = kResources[r].x - playerX, dz = kResources[r].z - playerZ;
            if (dx * dx + dz * dz < 2.6f * 2.6f) {
                collected[r] = 1;
                collectedCount++;
                pickedType = kResources[r].type;
                if (pickedType < 0 || pickedType >= kMaterialCount) pickedType = 0;
                pickTimer = 120;
            }
        }
        if (pickTimer > 0) pickTimer--;

        // FPS
        frameAccum++;
        long long now = sceKernelGetSystemTimeWide();
        if (now - lastTick >= 1000000) { fps = frameAccum; frameAccum = 0; lastTick = now; }

        // ---------- render ----------
        sceGuStart(GU_DIRECT, g_list);
        sceGuClearColor(CLEAR_COLOR);
        sceGuClearDepth(0);
        sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

        sceGumMatrixMode(GU_PROJECTION);
        sceGumLoadIdentity();
        sceGumPerspective(75.0f, 16.0f / 9.0f, 0.5f, 1000.0f);

        sceGumMatrixMode(GU_VIEW);
        sceGumLoadIdentity();
        {
            ScePspFVector3 rot    = { DEG2RAD(24.0f), camYaw, 0.0f };
            ScePspFVector3 camOff = { 0.0f, -4.5f, -11.0f };
            ScePspFVector3 pOff   = { -playerX, -playerY, -playerZ };
            sceGumRotateXYZ(&rot);
            sceGumTranslate(&camOff);
            sceGumTranslate(&pOff);
        }

        sceGumMatrixMode(GU_MODEL);
        sceGumLoadIdentity();
        sceGumDrawArray(GU_LINES, LINE_FLAGS, g_gridVerts, 0, g_grid);
        sceGumDrawArray(GU_LINES, LINE_FLAGS, g_worldVerts, 0, g_world);
        sceGumDrawArray(GU_LINES, LINE_FLAGS, g_npcVerts, 0, g_npc);

        sceGumLoadIdentity();
        {
            ScePspFVector3 pp = { playerX, playerY, playerZ };
            sceGumTranslate(&pp);
        }
        sceGumDrawArray(GU_LINES, LINE_FLAGS, g_playerBoxVerts, 0, g_playerBox);

        // recursos: brillan al acercarse (seccion 12)
        for (int r = 0; r < kResourceCount && r < 64; ++r) {
            if (collected[r]) continue;
            float dx = kResources[r].x - playerX, dz = kResources[r].z - playerZ;
            float d = sqrtf(dx * dx + dz * dz);
            float t = (d < 14.0f) ? (1.0f - d / 14.0f) : 0.0f; // 0 lejos .. 1 cerca
            int br = 55 + (int)(190 * t);
            unsigned int col = RGBA(br, 90 + (int)(90 * t), 130 + (int)(90 * t), 255);
            LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 24);
            int vi = 0;
            addBoxEdges(v, vi, kResources[r].x, kResources[r].y + 0.3f, kResources[r].z,
                        0.8f, 0.8f, 0.8f, col);
            sceGumLoadIdentity();
            sceGumDrawArray(GU_LINES, LINE_FLAGS, vi, 0, v);
        }

        // ---------- HUD (2D) ----------
        sceGuDisable(GU_DEPTH_TEST);
        sceGuEnable(GU_BLEND);
        sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
        sceGuDisable(GU_TEXTURE_2D);

        // paneles + barras (rectangulos)
        const int barX = 44, barW = 118;
        drawRect(6, 6, 182, 46, RGBA(8, 8, 14, 150));         // panel stats
        drawRect(barX, 12, barW, 6, RGBA(40, 20, 24, 220));   // HP fondo
        drawRect(barX, 12, barW, 6, RGBA(205, 55, 65, 255));  // HP lleno
        drawRect(barX, 24, barW, 6, RGBA(20, 28, 40, 220));   // EN fondo
        drawRect(barX, 24, barW, 6, RGBA(65, 140, 220, 255)); // EN lleno
        drawRect(barX, 36, barW, 6, RGBA(30, 20, 40, 220));   // GRV fondo
        drawRect(barX, 36, barW, 6, RGBA(160, 120, 215, 255));// GRV lleno
        drawRect(298, 234, 176, 32, RGBA(8, 8, 14, 165));     // panel arma

        // minimapa (arriba der): estructuras como puntos + jugador
        const int mmX = 362, mmY = 10, mmS = 100;
        drawRect(mmX - 3, mmY - 3, mmS + 6, mmS + 6, RGBA(160, 175, 215, 255)); // borde
        drawRect(mmX, mmY, mmS, mmS, RGBA(20, 24, 38, 255));                    // fondo opaco
        for (int s = 0; s < kStructureCount; ++s) {
            int sx = mmX + (int)((kStructures[s].x + 50.0f) * mmS / 100.0f);
            int sy = mmY + (int)((kStructures[s].z + 50.0f) * mmS / 100.0f);
            if (sx >= mmX && sx < mmX + mmS - 2 && sy >= mmY && sy < mmY + mmS - 2)
                drawRect(sx, sy, 3, 3, RGBA(140, 155, 205, 255));
        }
        {
            int pxm = mmX + (int)((playerX + 50.0f) * mmS / 100.0f);
            int pym = mmY + (int)((playerZ + 50.0f) * mmS / 100.0f);
            if (pxm >= mmX && pxm < mmX + mmS && pym >= mmY && pym < mmY + mmS)
                drawRect(pxm - 2, pym - 2, 4, 4, RGBA(245, 140, 95, 255));
        }

        // texto
        fontTexOn();
        drawText(10, 11, 1.0f, RGBA(230, 120, 130, 255), "HP");
        drawText(10, 23, 1.0f, RGBA(120, 170, 230, 255), "EN");
        drawText(10, 35, 1.0f, RGBA(185, 150, 230, 255), "GRV");
        drawText(barX + barW + 4, 11, 1.0f, RGBA(220, 220, 230, 255), "1200");
        drawText(barX + barW + 4, 23, 1.0f, RGBA(220, 220, 230, 255), "780");
        drawText(barX + barW + 4, 35, 1.0f, RGBA(220, 220, 230, 255), "100%");

        snprintf(hud, sizeof(hud), "DISTRITO: Campanario    FPS %d", fps);
        drawText(8, 58, 1.0f, RGBA(150, 160, 190, 255), hud);

        drawText(304, 239, 1.0f, RGBA(222, 210, 188, 255), kRanged[1].name);
        drawText(304, 252, 1.0f, RGBA(150, 175, 215, 255), "40 / 280");

        // aviso de objeto obtenido + contador de materiales
        if (pickTimer > 0) {
            snprintf(hud, sizeof(hud), "OBJETO OBTENIDO: %s", kMaterials[pickedType].name);
            drawText(8, 150, 1.0f, RGBA(120, 220, 150, 255), hud);
        }
        snprintf(hud, sizeof(hud), "MATERIALES: %d", collectedCount);
        drawText(8, 200, 1.0f, RGBA(150, 200, 170, 255), hud);

        snprintf(hud, sizeof(hud), "X %d  Z %d  Y %d", (int)playerX, (int)playerZ, (int)playerY);
        drawText(8, 230, 1.0f, RGBA(110, 130, 160, 255), hud);
        drawText(8, 244, 1.0f, RGBA(110, 130, 160, 255),
                 "Stick mover  Dpad camara  X saltar  START salir");

        sceGuDisable(GU_TEXTURE_2D);
        sceGuDisable(GU_BLEND);
        sceGuEnable(GU_DEPTH_TEST);

        sceGuFinish();
        sceGuSync(0, 0);
        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();
    }

    sceGuTerm();
    sceKernelExitGame();
    return 0;
}
