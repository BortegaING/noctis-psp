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
static const unsigned int HAZE = RGBA(84, 80, 98, 255); // gris silueta (mas oscuro que el fondo)

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

// desvanece un color hacia el fondo (vacio) segun la distancia al centro del
// distrito. Niebla "horneada" fiable (no depende del fog por hardware).
static unsigned int fadeToVoid(unsigned int base, float dist) {
    const float a = 26.0f, b = 98.0f; // cerca..lejos: se funde en la neblina
    float t = (dist - a) / (b - a);
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    int br = base & 0xFF,  bg = (base >> 8) & 0xFF,  bb = (base >> 16) & 0xFF;
    int cr = HAZE & 0xFF,  cg = (HAZE >> 8) & 0xFF,  cb = (HAZE >> 16) & 0xFF;
    int r  = br + (int)((cr - br) * t);
    int g  = bg + (int)((cg - bg) * t);
    int b2 = bb + (int)((cb - bb) * t);
    return RGBA(r, g, b2, 255);
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

// --- caras solidas con sombreado falso por cara (sin luces) ---
static void addQuad(LineVertex *buf, int &i,
                    float ax, float ay, float az, float bx, float by, float bz,
                    float cx2, float cy2, float cz2, float dx, float dy, float dz,
                    unsigned int col) {
    buf[i++] = { col, ax, ay, az }; buf[i++] = { col, bx, by, bz }; buf[i++] = { col, cx2, cy2, cz2 };
    buf[i++] = { col, ax, ay, az }; buf[i++] = { col, cx2, cy2, cz2 }; buf[i++] = { col, dx, dy, dz };
}
static void addSolidBox(LineVertex *buf, int &i, float cx, float baseY, float cz,
                        float w, float d, float h, unsigned int col) {
    const float x0 = cx - w * 0.5f, x1 = cx + w * 0.5f;
    const float z0 = cz - d * 0.5f, z1 = cz + d * 0.5f;
    const float y0 = baseY, y1 = baseY + h;
    const unsigned int top = brighten(col, 1.15f);
    const unsigned int sa  = brighten(col, 0.82f);
    const unsigned int sb  = brighten(col, 0.60f);
    addQuad(buf, i, x0,y1,z0, x1,y1,z0, x1,y1,z1, x0,y1,z1, top); // techo
    addQuad(buf, i, x0,y0,z0, x1,y0,z0, x1,y1,z0, x0,y1,z0, sa);  // frente
    addQuad(buf, i, x1,y0,z1, x0,y0,z1, x0,y1,z1, x1,y1,z1, sa);  // atras
    addQuad(buf, i, x0,y0,z1, x0,y0,z0, x0,y1,z0, x0,y1,z1, sb);  // izquierda
    addQuad(buf, i, x1,y0,z0, x1,y0,z1, x1,y1,z1, x1,y1,z0, sb);  // derecha
}

static LineVertex __attribute__((aligned(16))) g_solidWorld[30000];
static int g_solidVerts = 0;

// piramide de 4 caras (aguja / chapitel gotico)
static void addPyramid(LineVertex *buf, int &i, float cx, float baseY, float cz,
                       float w, float d, float apexH, unsigned int col) {
    const float x0 = cx - w * 0.5f, x1 = cx + w * 0.5f;
    const float z0 = cz - d * 0.5f, z1 = cz + d * 0.5f;
    const float y0 = baseY, ay = baseY + apexH;
    const unsigned int a = brighten(col, 1.10f), b = brighten(col, 0.72f);
    buf[i++] = { a, x0, y0, z0 }; buf[i++] = { a, x1, y0, z0 }; buf[i++] = { a, cx, ay, cz };
    buf[i++] = { b, x1, y0, z0 }; buf[i++] = { b, x1, y0, z1 }; buf[i++] = { b, cx, ay, cz };
    buf[i++] = { a, x1, y0, z1 }; buf[i++] = { a, x0, y0, z1 }; buf[i++] = { a, cx, ay, cz };
    buf[i++] = { b, x0, y0, z1 }; buf[i++] = { b, x0, y0, z0 }; buf[i++] = { b, cx, ay, cz };
}

// decide el estado de una ventana por su posicion: 0 = apagada, o un color.
// Rompe la uniformidad: ~30% apagadas, algunas de luz fria, el resto ambar.
static unsigned int winPick(float x, float y, float z, unsigned int amber) {
    int seed = (int)(x * 97.0f) + (int)(y * 57.0f) + (int)(z * 131.0f);
    unsigned int u = (unsigned int)seed;
    u ^= 61u; u ^= (u >> 16); u *= 9u; u ^= (u >> 4); u *= 0x27d4eb2du; u ^= (u >> 15);
    int m = (int)(u % 10u);
    if (m < 3) return 0u;                        // apagada
    if (m < 5) return RGBA(120, 150, 215, 255);  // luz fria
    return amber;                                // ambar
}

// una fila de ventanas (quads emisivos) en una cara.
// face: 0=+z, 1=-z, 2=+x, 3=-x
static void addWinRow(LineVertex *buf, int &i, float cx, float cz,
                      float w, float d, float y, int face, unsigned int col) {
    const float wh = 0.62f, ww = 0.28f, e = 0.10f;
    if (face == 0 || face == 1) {
        const float z = (face == 0) ? (cz + d * 0.5f + e) : (cz - d * 0.5f - e);
        int cols = (int)(w / 2.4f); if (cols < 1) cols = 1; if (cols > 5) cols = 5;
        const float step = w / (cols + 1);
        for (int c = 1; c <= cols; ++c) {
            float x = cx - w * 0.5f + step * c;
            unsigned int wc = winPick(x, y, z, col); if (!wc) continue;
            addQuad(buf, i, x-ww,y-wh,z, x+ww,y-wh,z, x+ww,y+wh,z, x-ww,y+wh,z, wc);
        }
    } else {
        const float x = (face == 2) ? (cx + w * 0.5f + e) : (cx - w * 0.5f - e);
        int cols = (int)(d / 2.4f); if (cols < 1) cols = 1; if (cols > 5) cols = 5;
        const float step = d / (cols + 1);
        for (int c = 1; c <= cols; ++c) {
            float z = cz - d * 0.5f + step * c;
            unsigned int wc = winPick(x, y, z, col); if (!wc) continue;
            addQuad(buf, i, x,y-wh,z-ww, x,y-wh,z+ww, x,y+wh,z+ww, x,y+wh,z-ww, wc);
        }
    }
}

// pinaculo gotico: caja fina rematada en aguja (mini-torre de esquina)
static void addPinnacle(LineVertex *buf, int &i, float cx, float baseY, float cz,
                        float w, float h, unsigned int col) {
    addSolidBox(buf, i, cx, baseY, cz, w, w, h, col);
    addPyramid(buf, i, cx, baseY + h, cz, w, w, w * 1.7f, brighten(col, 1.12f));
}

// torre gotica detallada: cuerpo escalonado + aguja + pinaculos + contrafuertes
static void buildTower(LineVertex *buf, int &i, float cx, float cz,
                       float w, float d, float h, unsigned int baseColor, float dist) {
    const unsigned int stone = fadeToVoid(brighten(baseColor, 1.9f), dist);
    const unsigned int win   = RGBA(235, 165, 85, 255); // ambar (luces)
    float bodyTop;
    if (h > 45.0f) {
        // cuerpo en tres tramos que se angostan (perfil gotico escalonado)
        const float h1 = h * 0.50f, h2 = h * 0.28f, h3 = h - h1 - h2;
        addSolidBox(buf, i, cx, 0.0f,      cz, w,       d,       h1, stone);
        addSolidBox(buf, i, cx, h1,        cz, w*0.78f, d*0.78f, h2, stone);
        addSolidBox(buf, i, cx, h1 + h2,   cz, w*0.56f, d*0.56f, h3, stone);
        addPyramid(buf, i, cx, h, cz, w*0.56f, d*0.56f, h*0.34f, brighten(stone, 1.18f));
        // pinaculos en las esquinas del primer setback
        const float px = w*0.5f - 0.8f, pz = d*0.5f - 0.8f;
        const float ph = h * 0.13f;
        addPinnacle(buf, i, cx-px, h1, cz-pz, 1.4f, ph, stone);
        addPinnacle(buf, i, cx+px, h1, cz-pz, 1.4f, ph, stone);
        addPinnacle(buf, i, cx-px, h1, cz+pz, 1.4f, ph, stone);
        addPinnacle(buf, i, cx+px, h1, cz+pz, 1.4f, ph, stone);
        bodyTop = h1;
    } else {
        addSolidBox(buf, i, cx, 0.0f, cz, w, d, h, stone);
        if (h > 18.0f) addPyramid(buf, i, cx, h, cz, w, d, h * 0.40f, brighten(stone, 1.12f));
        bodyTop = h;
    }
    // contrafuertes en la base (4 lados) para darle forma
    if (w >= 6.0f && h > 30.0f) {
        const float bh = h * 0.30f, bw = 1.6f, bd = 2.6f;
        const unsigned int bc = brighten(stone, 0.82f);
        addSolidBox(buf, i, cx - w*0.5f - bd*0.35f, 0.0f, cz, bd, bw, bh, bc);
        addSolidBox(buf, i, cx + w*0.5f + bd*0.35f, 0.0f, cz, bd, bw, bh, bc);
        addSolidBox(buf, i, cx, 0.0f, cz - d*0.5f - bd*0.35f, bw, bd, bh, bc);
        addSolidBox(buf, i, cx, 0.0f, cz + d*0.5f + bd*0.35f, bw, bd, bh, bc);
    }
    // ventanas en las 4 caras del cuerpo bajo
    int rows = (int)((bodyTop - 4.0f) / 4.5f);
    if (rows > 11) rows = 11;
    for (int rrow = 0; rrow < rows; ++rrow) {
        float y = 3.5f + rrow * 4.5f;
        if (y > bodyTop - 2.0f) break;
        addWinRow(buf, i, cx, cz, w, d, y, 0, win);
        addWinRow(buf, i, cx, cz, w, d, y, 1, win);
        addWinRow(buf, i, cx, cz, w, d, y, 2, win);
        addWinRow(buf, i, cx, cz, w, d, y, 3, win);
    }
}

// aguja fina del "mar de agujas" de fondo (ligera, para densidad barata)
static void buildSpire(LineVertex *buf, int &i, float cx, float cz,
                       float w, float h, unsigned int base, float dist) {
    const unsigned int stone = fadeToVoid(brighten(base, 1.8f), dist);
    const float h1 = h * 0.68f;
    addSolidBox(buf, i, cx, 0.0f, cz, w, w, h1, stone);
    addSolidBox(buf, i, cx, h1, cz, w * 0.6f, w * 0.6f, h - h1, stone);
    addPyramid(buf, i, cx, h, cz, w * 0.6f, w * 0.6f, h * 0.42f, brighten(stone, 1.1f));
    int rows = (int)(h1 / 6.0f); if (rows > 7) rows = 7;
    for (int rrow = 0; rrow < rows; ++rrow) {
        float y = 4.0f + rrow * 6.0f;
        if (y > h1 - 2.0f) break;
        addWinRow(buf, i, cx, cz, w, w, y, 0, RGBA(235, 165, 85, 255));
    }
}

static void buildSolidWorld() {
    int i = 0;
    for (int s = 0; s < kStructureCount && s < 63; ++s) {
        if (i > 28000) break;
        const Structure &st = kStructures[s];
        float d = sqrtf(st.x * st.x + st.z * st.z);
        buildTower(g_solidWorld, i, st.x, st.z, st.w, st.d, st.h, st.color, d);
    }
    // mar de agujas de fondo (espiral aurea): densidad que se pierde en neblina
    for (int k = 0; k < 60; ++k) {
        if (i > 27000) break;
        float ang = (float)k * 2.3999632f;
        float rad = 36.0f + (float)((k * 37) % 72);  // 36..107
        float cx = cosf(ang) * rad;
        float cz = sinf(ang) * rad;
        float hh = 44.0f + (float)((k * 53) % 92);   // 44..135
        float ww = 3.0f + (float)((k * 7) % 4);      // 3..6
        float dd = sqrtf(cx * cx + cz * cz);
        buildSpire(g_solidWorld, i, cx, cz, ww, hh, kStructures[k % kStructureCount].color, dd);
    }
    // The Cathedral: detallada, silueta lejana con ventanas (secciones 26, 31)
    buildTower(g_solidWorld, i, 0.0f, -150.0f, 70.0f, 70.0f, 300.0f, RGBA(34, 32, 50, 255), 0.0f);
    g_solidVerts = i;
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

// colision con muros: bloquea si el jugador (radio r) esta DENTRO de la huella
// de una estructura y por DEBAJO de su techo (no bloquea al estar encima).
static bool blocked(float px, float pz, float py) {
    const float r = 1.1f;
    for (int s = 0; s < kStructureCount; ++s) {
        const Structure &st = kStructures[s];
        if (py < st.y + st.h - 0.8f) {
            const float x0 = st.x - st.w * 0.5f - r, x1 = st.x + st.w * 0.5f + r;
            const float z0 = st.z - st.d * 0.5f - r, z1 = st.z + st.d * 0.5f + r;
            if (px > x0 && px < x1 && pz > z0 && pz < z1) return true;
        }
    }
    return false;
}

// --- caja del jugador (local, se traslada con MODEL) ---
static LineVertex __attribute__((aligned(16))) g_playerBox[24];
static int g_playerBoxVerts = 0;

static void buildPlayerBox() {
    int i = 0;
    addBoxEdges(g_playerBox, i, 0.0f, 0.0f, 0.0f, 1.4f, 1.4f, 3.0f, RGBA(235, 130, 90, 255));
    g_playerBoxVerts = i;
}

// personaje humanoide solido (mira a -z en local): abrigo oscuro con detalle
// rojo, cabeza con pelo en puas y katana a la espalda (estilo de la referencia).
static LineVertex __attribute__((aligned(16))) g_playerModel[900];
static int g_playerModelVerts = 0;

static void buildPlayerModel() {
    int i = 0;
    const unsigned int coat  = RGBA(48, 46, 58, 255);
    const unsigned int red   = RGBA(158, 48, 54, 255);
    const unsigned int skin  = RGBA(150, 132, 122, 255);
    const unsigned int hair  = RGBA(28, 26, 34, 255);
    const unsigned int steel = RGBA(120, 138, 172, 255);
    // piernas
    addSolidBox(g_playerModel, i, -0.22f, 0.0f, 0.0f, 0.34f, 0.36f, 1.5f, coat);
    addSolidBox(g_playerModel, i,  0.22f, 0.0f, 0.0f, 0.34f, 0.36f, 1.5f, coat);
    // faldon del abrigo
    addSolidBox(g_playerModel, i, 0.0f, 1.25f, 0.0f, 0.98f, 0.62f, 0.55f, coat);
    // torso
    addSolidBox(g_playerModel, i, 0.0f, 1.75f, 0.0f, 0.82f, 0.50f, 0.95f, coat);
    // pecho rojo (frente = -z)
    addSolidBox(g_playerModel, i, 0.0f, 1.95f, -0.24f, 0.50f, 0.10f, 0.62f, red);
    // brazos
    addSolidBox(g_playerModel, i, -0.52f, 1.70f, 0.0f, 0.24f, 0.30f, 1.00f, coat);
    addSolidBox(g_playerModel, i,  0.52f, 1.70f, 0.0f, 0.24f, 0.30f, 1.00f, coat);
    // cuello + cabeza
    addSolidBox(g_playerModel, i, 0.0f, 2.68f, 0.0f, 0.20f, 0.20f, 0.16f, skin);
    addSolidBox(g_playerModel, i, 0.0f, 2.84f, 0.0f, 0.44f, 0.44f, 0.46f, skin);
    // pelo (bloque + puas)
    addSolidBox(g_playerModel, i, 0.0f, 3.22f, 0.06f, 0.52f, 0.50f, 0.16f, hair);
    addPyramid(g_playerModel, i, -0.12f, 3.34f, 0.02f, 0.18f, 0.18f, 0.30f, hair);
    addPyramid(g_playerModel, i,  0.12f, 3.34f, 0.10f, 0.16f, 0.16f, 0.26f, hair);
    // katana a la espalda (+z): hoja + mango
    addSolidBox(g_playerModel, i, 0.12f, 0.90f, 0.34f, 0.10f, 0.10f, 2.10f, steel);
    addSolidBox(g_playerModel, i, 0.12f, 3.00f, 0.34f, 0.14f, 0.14f, 0.50f, hair);
    g_playerModelVerts = i;
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

// --- cadenas colgantes entre torres (detalle iconico de la referencia) ---
static LineVertex __attribute__((aligned(16))) g_chains[1800];
static int g_chainVerts = 0;

static void addChain(LineVertex *buf, int &i, float ax, float ay, float az,
                     float bx, float by, float bz, float sag, unsigned int col) {
    const int SEG = 10;
    float px = ax, py = ay, pz = az;
    for (int s = 1; s <= SEG; ++s) {
        float t = (float)s / SEG;
        float x = ax + (bx - ax) * t;
        float z = az + (bz - az) * t;
        float y = ay + (by - ay) * t - sag * 4.0f * t * (1.0f - t); // pandeo
        buf[i++] = { col, px, py, pz };
        buf[i++] = { col, x, y, z };
        px = x; py = y; pz = z;
    }
}

static void buildChains() {
    int i = 0;
    const unsigned int col = RGBA(76, 72, 86, 255);
    const int offs[4] = { 2, 3, 5, 7 };
    for (int o = 0; o < 4; ++o) {
        for (int k = 0; k < kStructureCount; ++k) {
            if (i > 1700) break;
            const Structure &a = kStructures[k];
            const Structure &b = kStructures[(k + offs[o]) % kStructureCount];
            float dx = a.x - b.x, dz = a.z - b.z;
            float dist = sqrtf(dx * dx + dz * dz);
            if (dist > 8.0f && dist < 56.0f) {
                float ay = a.h * (0.52f + 0.09f * o);
                float by = b.h * (0.52f + 0.09f * o);
                addChain(g_chains, i, a.x, ay, a.z, b.x, by, b.z, 3.0f + dist * 0.06f, col);
            }
        }
    }
    g_chainVerts = i;
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

// fondo con gradiente vertical: neblina luminosa en el horizonte (profundidad)
struct GradVertex { unsigned int color; short x, y, z; };
static void gradQuad(int y0, int y1, unsigned int cTop, unsigned int cBot) {
    GradVertex *v = (GradVertex *)sceGuGetMemory(sizeof(GradVertex) * 6);
    v[0] = { cTop, 0, (short)y0, 0 };
    v[1] = { cTop, (short)SCR_WIDTH, (short)y0, 0 };
    v[2] = { cBot, (short)SCR_WIDTH, (short)y1, 0 };
    v[3] = { cTop, 0, (short)y0, 0 };
    v[4] = { cBot, (short)SCR_WIDTH, (short)y1, 0 };
    v[5] = { cBot, 0, (short)y1, 0 };
    sceGuDrawArray(GU_TRIANGLES, GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 6, 0, v);
}
static void drawBackdrop() {
    const unsigned int top    = RGBA(24, 22, 32, 255);
    const unsigned int haze   = RGBA(128, 120, 134, 255);
    const unsigned int floorc = RGBA(18, 17, 24, 255);
    gradQuad(0, 150, top, haze);
    gradQuad(150, SCR_HEIGHT, haze, floorc);
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
    buildSolidWorld();
    buildPlayerModel();
    buildNpcs();
    buildChains();
    buildFontAtlas();
    initGu();

    SceCtrlData pad;
    float playerX = 0.0f, playerY = 0.0f, playerZ = 0.0f;
    float velY = 0.0f;
    int   grounded = 1;
    float camYaw = 0.0f;
    int   paused = 0, prevStart = 0;

    int fps = 0, frameAccum = 0;
    long long lastTick = sceKernelGetSystemTimeWide();
    char hud[80];

    int collected[64] = {0};
    int collectedCount = 0, pickTimer = 0, pickedType = 0;

    while (!g_exit) {
        sceCtrlReadBufferPositive(&pad, 1);
        // START = pausa (toggle con deteccion de flanco). Ya NO sale. HOME sale.
        int startNow = (pad.Buttons & PSP_CTRL_START) ? 1 : 0;
        if (startNow && !prevStart) paused = !paused;
        prevStart = startNow;

        if (!paused) {
            // camara (D-pad izq/der)
            if (pad.Buttons & PSP_CTRL_LEFT)  camYaw -= 0.03f;
            if (pad.Buttons & PSP_CTRL_RIGHT) camYaw += 0.03f;

            // movimiento con colision por ejes separados (desliza por muros)
            int lx = (int)pad.Lx - 128, ly = (int)pad.Ly - 128;
            const int dead = 24;
            float mx = (lx > dead || lx < -dead) ? lx / 128.0f : 0.0f;
            float mz = (ly > dead || ly < -dead) ? ly / 128.0f : 0.0f;
            const float speed = 0.35f;
            float nx = playerX + mx * speed;
            if (!blocked(nx, playerZ, playerY)) playerX = nx;
            float nz = playerZ + mz * speed;
            if (!blocked(playerX, nz, playerY)) playerZ = nz;

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
        }

        // FPS
        frameAccum++;
        long long now = sceKernelGetSystemTimeWide();
        if (now - lastTick >= 1000000) { fps = frameAccum; frameAccum = 0; lastTick = now; }

        // ---------- render ----------
        sceGuStart(GU_DIRECT, g_list);
        sceGuClearColor(CLEAR_COLOR);
        sceGuClearDepth(0);
        sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

        // fondo brumoso (2D, sin profundidad): neblina en el horizonte
        sceGuDisable(GU_DEPTH_TEST);
        drawBackdrop();
        sceGuEnable(GU_DEPTH_TEST);

        sceGumMatrixMode(GU_PROJECTION);
        sceGumLoadIdentity();
        sceGumPerspective(75.0f, 16.0f / 9.0f, 0.5f, 1000.0f);

        sceGumMatrixMode(GU_VIEW);
        sceGumLoadIdentity();
        {
            ScePspFVector3 rot    = { DEG2RAD(15.0f), camYaw, 0.0f };
            ScePspFVector3 camOff = { 0.0f, -6.0f, -13.0f };
            ScePspFVector3 pOff   = { -playerX, -playerY, -playerZ };
            // orden correcto de camara orbital: offset (espacio camara) -> giro
            // -> centrar en el jugador. Asi el jugador NO se va al girar.
            sceGumTranslate(&camOff);
            sceGumRotateXYZ(&rot);
            sceGumTranslate(&pOff);
        }

        sceGumMatrixMode(GU_MODEL);
        sceGumLoadIdentity();
        sceGumDrawArray(GU_LINES, LINE_FLAGS, g_gridVerts, 0, g_grid);
        sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g_solidVerts, 0, g_solidWorld);
        sceGumDrawArray(GU_LINES, LINE_FLAGS, g_chainVerts, 0, g_chains);
        sceGumDrawArray(GU_LINES, LINE_FLAGS, g_npcVerts, 0, g_npc);

        sceGumLoadIdentity();
        {
            ScePspFVector3 pp   = { playerX, playerY, playerZ };
            ScePspFVector3 prot = { 0.0f, camYaw, 0.0f };
            sceGumTranslate(&pp);
            sceGumRotateXYZ(&prot);
        }
        sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g_playerModelVerts, 0, g_playerModel);

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

        if (paused) drawRect(150, 88, 180, 64, RGBA(10, 10, 16, 205)); // panel pausa

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
                 "Stick mover  Dpad camara  X saltar  START pausa");
        if (paused) {
            drawText(206, 104, 2.0f, RGBA(232, 222, 242, 255), "PAUSA");
            drawText(163, 130, 1.0f, RGBA(165, 175, 205, 255), "START continuar   HOME salir");
        }

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
