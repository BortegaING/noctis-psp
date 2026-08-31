// PROJECT NOCTIS - Motor 3D base.
// Hito 1 de motor: sceGu init, camara en perspectiva (orbital), piso en
// cuadricula que se funde en la oscuridad (niebla horneada por vertice) y
// texto/FPS con una fuente bitmap propia (sin dependencias de flash0).
// Todo en un archivo a proposito: primero corre y se prueba; luego se
// refactoriza en modulos (gfx/, player/, world/...) para repartir agentes.

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspgu.h>
#include <pspgum.h>
#include <cmath>
#include <cstdio>

#include "font8x8_basic.h" // char font8x8_basic[128][8], dominio publico

PSP_MODULE_INFO("NOCTIS", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

// ----- pantalla / buffers -----
#define BUF_WIDTH  512
#define SCR_WIDTH  480
#define SCR_HEIGHT 272

#define DEG2RAD(d) ((d) * 0.0174532925f)
// GU_COLOR_8888 se almacena como 0xAABBGGRR (little-endian)
#define RGBA(r, g, b, a) (((a) << 24) | ((b) << 16) | ((g) << 8) | (r))

static unsigned int __attribute__((aligned(16))) g_list[262144];

static const unsigned int CLEAR_COLOR = RGBA(16, 14, 20, 255); // casi negro, frio

// ================= geometria del piso =================
struct GridVertex {
    unsigned int color;
    float x, y, z;
};

#define GRID_HALF 48
static GridVertex __attribute__((aligned(16))) g_grid[(2 * GRID_HALF + 1) * 4];
static int g_gridVerts = 0;

static void buildGrid() {
    int i = 0;
    const float h = (float)GRID_HALF;
    const unsigned int col     = RGBA(70, 85, 120, 255);   // gris-azulado frio
    const unsigned int colAxis = RGBA(150, 60, 60, 255);   // eje central rojizo
    for (int k = -GRID_HALF; k <= GRID_HALF; ++k) {
        const float f = (float)k;
        const unsigned int c = (k == 0) ? colAxis : col;
        // linea paralela al eje X (z fijo)
        g_grid[i++] = { c, -h, 0.0f, f };
        g_grid[i++] = { c,  h, 0.0f, f };
        // linea paralela al eje Z (x fijo)
        g_grid[i++] = { c, f, 0.0f, -h };
        g_grid[i++] = { c, f, 0.0f,  h };
    }
    g_gridVerts = i;
}

// ================= fuente bitmap =================
// atlas 128x128 = rejilla 16x16 de glifos 8x8 (chars 0..127)
static unsigned int __attribute__((aligned(16))) g_fontAtlas[128 * 128];

static void buildFontAtlas() {
    for (int i = 0; i < 128 * 128; ++i) g_fontAtlas[i] = 0; // transparente
    for (int c = 0; c < 128; ++c) {
        const int cx = (c % 16) * 8;
        const int cy = (c / 16) * 8;
        for (int row = 0; row < 8; ++row) {
            const unsigned char bits = (unsigned char)font8x8_basic[c][row];
            for (int col = 0; col < 8; ++col) {
                if (bits & (1 << col)) // bit 0 = pixel izquierdo
                    g_fontAtlas[(cy + row) * 128 + (cx + col)] = 0xFFFFFFFF;
            }
        }
    }
    sceKernelDcacheWritebackAll();
}

// vertice de sprite: coords de textura en TEXELS (0..128) y pantalla en pixeles
struct SpriteVertex {
    unsigned short u, v;
    short x, y, z;
};

// dibuja texto 2D en coords de pantalla (llamar dentro del frame GU).
// El color se aplica con sceGuColor (MODULATE): el vertice de 16 bits no
// lleva color propio.
static void drawText(int x, int y, float scale, unsigned int color, const char *text) {
    int len = 0;
    for (const char *p = text; *p; ++p) len++;
    if (len == 0) return;

    sceGuColor(color);

    SpriteVertex *v = (SpriteVertex *)sceGuGetMemory(sizeof(SpriteVertex) * 2 * len);
    const int cw = (int)(8 * scale), ch = (int)(8 * scale);
    int n = 0;
    int px = x;
    for (const char *p = text; *p; ++p) {
        unsigned char c = (unsigned char)*p;
        if (c >= 128) c = '?';
        const int cx = (c % 16) * 8;
        const int cy = (c / 16) * 8;
        v[n].u = cx;     v[n].v = cy;     v[n].x = px;      v[n].y = y;      v[n].z = 0; n++;
        v[n].u = cx + 8; v[n].v = cy + 8; v[n].x = px + cw; v[n].y = y + ch; v[n].z = 0; n++;
        px += cw;
    }
    sceGuDrawArray(GU_SPRITES,
                   GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D,
                   n, 0, v);
}

static void beginText() {
    sceGuEnable(GU_TEXTURE_2D);
    sceGuTexMode(GU_PSM_8888, 0, 0, 0);
    sceGuTexImage(0, 128, 128, 128, g_fontAtlas);
    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
    sceGuTexFilter(GU_NEAREST, GU_NEAREST);
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    sceGuDisable(GU_DEPTH_TEST);
}

static void endText() {
    sceGuEnable(GU_DEPTH_TEST);
    sceGuDisable(GU_BLEND);
    sceGuDisable(GU_TEXTURE_2D);
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
    sceGuDisable(GU_CULL_FACE);   // dibujamos lineas
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
    buildFontAtlas();
    initGu();

    SceCtrlData pad;
    float camYaw = 0.0f;

    int   fps = 0, frameAccum = 0;
    long long lastTick = sceKernelGetSystemTimeWide();
    char hud[64];

    while (!g_exit) {
        sceCtrlReadBufferPositive(&pad, 1);
        if (pad.Buttons & PSP_CTRL_START) g_exit = 1;

        camYaw += 0.006f; // orbita lenta

        // ---- FPS (medido cada segundo) ----
        frameAccum++;
        long long now = sceKernelGetSystemTimeWide();
        if (now - lastTick >= 1000000) {
            fps = frameAccum;
            frameAccum = 0;
            lastTick = now;
        }

        // ---- render ----
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
            ScePspFVector3 rot = { DEG2RAD(28.0f), camYaw, 0.0f }; // mira hacia abajo + orbita
            ScePspFVector3 tr  = { 0.0f, -3.0f, -18.0f };          // eleva y aleja la camara
            sceGumRotateXYZ(&rot);
            sceGumTranslate(&tr);
        }

        sceGumMatrixMode(GU_MODEL);
        sceGumLoadIdentity();

        sceGumDrawArray(GU_LINES,
                        GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                        g_gridVerts, 0, g_grid);

        // ---- HUD / texto (2D) ----
        beginText();
        snprintf(hud, sizeof(hud), "NOCTIS   motor 3D   FPS %d", fps);
        drawText(12, 12, 1.0f, RGBA(230, 230, 240, 255), hud);
        drawText(12, 24, 1.0f, RGBA(150, 160, 190, 255), "DISTRITO: Campanario");
        endText();

        sceGuFinish();
        sceGuSync(0, 0);
        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();
    }

    sceGuTerm();
    sceKernelExitGame();
    return 0;
}
