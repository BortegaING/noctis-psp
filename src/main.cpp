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
#define RGBA(r, g, b, a) ((unsigned int)(((a) << 24) | ((b) << 16) | ((g) << 8) | (r))) // 0xAABBGGRR

static unsigned int __attribute__((aligned(16))) g_list[262144];
static const unsigned int CLEAR_COLOR = RGBA(16, 14, 20, 255);
static const unsigned int HAZE = RGBA(98, 86, 78, 255); // gris-marron silueta (calido)
#define WSCALE 1.45f  // separa el distrito para abrir la vista (mas skyline/agujas)

#define LINE_FLAGS (GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D)

// ================= geometria (lineas 3D) =================
struct LineVertex { unsigned int color; float x, y, z; };

// vertice texturizado (piedra): u,v + color + posicion
struct TexVertex { float u, v; unsigned int color; float x, y, z; };
#define TEX_FLAGS (GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D)
#define TILE 6.0f   // unidades de mundo por repeticion de la textura de piedra

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

// sesga el color hacia calido (marron/sepia) para acercarse a la referencia
static unsigned int warmTint(unsigned int c) {
    int r = (int)((c & 0xFF) * 1.20f);
    int g = (int)(((c >> 8) & 0xFF) * 1.02f);
    int b = (int)(((c >> 16) & 0xFF) * 0.82f);
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

static TexVertex __attribute__((aligned(16))) g_solidWorld[22000]; // piedra texturizada
static int g_solidVerts = 0;
static TexVertex __attribute__((aligned(16))) g_win[18000];        // ventanas (vidriera texturizada)
static int g_winVerts = 0;

// piramide de 4 caras (aguja) sin textura -- para personaje/robots (LineVertex)
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

// ---- helpers TEXTURIZADOS (piedra) para el mundo solido ----
static void addQuadT(TexVertex *buf, int &i,
                     float ax,float ay,float az, float bx,float by,float bz,
                     float cx2,float cy2,float cz2, float dx,float dy,float dz,
                     float u0,float v0,float u1,float v1, unsigned int col) {
    buf[i++] = {u0,v0,col,ax,ay,az}; buf[i++] = {u1,v0,col,bx,by,bz}; buf[i++] = {u1,v1,col,cx2,cy2,cz2};
    buf[i++] = {u0,v0,col,ax,ay,az}; buf[i++] = {u1,v1,col,cx2,cy2,cz2}; buf[i++] = {u0,v1,col,dx,dy,dz};
}
static void addSolidBoxT(TexVertex *buf, int &i, float cx, float baseY, float cz,
                         float w, float d, float h, unsigned int col) {
    const float x0 = cx-w*0.5f, x1 = cx+w*0.5f, z0 = cz-d*0.5f, z1 = cz+d*0.5f, y0 = baseY, y1 = baseY+h;
    const unsigned int top = brighten(col,1.15f), sa = brighten(col,0.82f), sb = brighten(col,0.60f);
    const float uw = w/TILE, ud = d/TILE, uh = h/TILE;
    addQuadT(buf,i, x0,y1,z0, x1,y1,z0, x1,y1,z1, x0,y1,z1, 0,0,uw,ud, top); // techo
    addQuadT(buf,i, x0,y0,z0, x1,y0,z0, x1,y1,z0, x0,y1,z0, 0,uh,uw,0, sa);  // frente
    addQuadT(buf,i, x1,y0,z1, x0,y0,z1, x0,y1,z1, x1,y1,z1, 0,uh,uw,0, sa);  // atras
    addQuadT(buf,i, x0,y0,z1, x0,y0,z0, x0,y1,z0, x0,y1,z1, 0,uh,ud,0, sb);  // izq
    addQuadT(buf,i, x1,y0,z0, x1,y0,z1, x1,y1,z1, x1,y1,z0, 0,uh,ud,0, sb);  // der
}
static void addPyramidT(TexVertex *buf, int &i, float cx, float baseY, float cz,
                        float w, float d, float apexH, unsigned int col) {
    const float x0 = cx-w*0.5f, x1 = cx+w*0.5f, z0 = cz-d*0.5f, z1 = cz+d*0.5f, y0 = baseY, ay = baseY+apexH;
    const unsigned int a = brighten(col,1.10f), b = brighten(col,0.72f);
    const float uw = w/TILE, uh = apexH/TILE;
    buf[i++]={0,uh,a,x0,y0,z0}; buf[i++]={uw,uh,a,x1,y0,z0}; buf[i++]={uw*0.5f,0,a,cx,ay,cz};
    buf[i++]={0,uh,b,x1,y0,z0}; buf[i++]={uw,uh,b,x1,y0,z1}; buf[i++]={uw*0.5f,0,b,cx,ay,cz};
    buf[i++]={0,uh,a,x1,y0,z1}; buf[i++]={uw,uh,a,x0,y0,z1}; buf[i++]={uw*0.5f,0,a,cx,ay,cz};
    buf[i++]={0,uh,b,x0,y0,z1}; buf[i++]={uw,uh,b,x0,y0,z0}; buf[i++]={uw*0.5f,0,b,cx,ay,cz};
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

// una fila de ventanas (quads emisivos) -> buffer g_win (sin textura).
// face: 0=+z, 1=-z, 2=+x, 3=-x
static void addWinRow(float cx, float cz, float w, float d, float y, int face, unsigned int col) {
    if (g_winVerts > 17800) return;
    const float wh = 0.62f, ww = 0.28f, e = 0.10f;
    if (face == 0 || face == 1) {
        const float z = (face == 0) ? (cz + d * 0.5f + e) : (cz - d * 0.5f - e);
        int cols = (int)(w / 2.4f); if (cols < 1) cols = 1; if (cols > 5) cols = 5;
        const float step = w / (cols + 1);
        for (int c = 1; c <= cols; ++c) {
            float x = cx - w * 0.5f + step * c;
            unsigned int wc = winPick(x, y, z, col); if (!wc) continue;
            addQuadT(g_win, g_winVerts, x-ww,y-wh,z, x+ww,y-wh,z, x+ww,y+wh,z, x-ww,y+wh,z, 0,0,1,1, wc);
        }
    } else {
        const float x = (face == 2) ? (cx + w * 0.5f + e) : (cx - w * 0.5f - e);
        int cols = (int)(d / 2.4f); if (cols < 1) cols = 1; if (cols > 5) cols = 5;
        const float step = d / (cols + 1);
        for (int c = 1; c <= cols; ++c) {
            float z = cz - d * 0.5f + step * c;
            unsigned int wc = winPick(x, y, z, col); if (!wc) continue;
            addQuadT(g_win, g_winVerts, x,y-wh,z-ww, x,y-wh,z+ww, x,y+wh,z+ww, x,y+wh,z-ww, 0,0,1,1, wc);
        }
    }
}

// pinaculo gotico texturizado: caja fina rematada en aguja
static void addPinnacle(TexVertex *buf, int &i, float cx, float baseY, float cz,
                        float w, float h, unsigned int col) {
    addSolidBoxT(buf, i, cx, baseY, cz, w, w, h, col);
    addPyramidT(buf, i, cx, baseY + h, cz, w, w, w * 1.7f, brighten(col, 1.12f));
}

// niebla por ALTURA: las estructuras se disuelven en la bruma del cielo al subir
static unsigned int heightHaze(unsigned int base, float y) {
    float t = y / 190.0f; if (t < 0.0f) t = 0.0f; if (t > 0.82f) t = 0.82f;
    int br = base & 0xFF,  bg = (base >> 8) & 0xFF,  bb = (base >> 16) & 0xFF;
    int cr = HAZE & 0xFF,  cg = (HAZE >> 8) & 0xFF,  cb = (HAZE >> 16) & 0xFF;
    int r  = br + (int)((cr - br) * t);
    int g  = bg + (int)((cg - bg) * t);
    int b2 = bb + (int)((cb - bb) * t);
    return RGBA(r, g, b2, 255);
}

// torre gotica detallada texturizada: cuerpo escalonado + aguja + pinaculos + contrafuertes
static void buildTower(TexVertex *buf, int &i, float cx, float cz,
                       float w, float d, float h, unsigned int baseColor, float dist) {
    const unsigned int stone = fadeToVoid(warmTint(brighten(baseColor, 1.9f)), dist);
    const unsigned int win   = RGBA(235, 165, 85, 255); // ambar (luces)
    float bodyTop;
    if (h > 45.0f) {
        const float h1 = h * 0.50f, h2 = h * 0.28f, h3 = h - h1 - h2;
        const unsigned int s1 = heightHaze(stone, h1 * 0.5f);
        const unsigned int s2 = heightHaze(stone, h1 + h2 * 0.5f);
        const unsigned int s3 = heightHaze(stone, h1 + h2 + h3 * 0.5f);
        addSolidBoxT(buf, i, cx, 0.0f,    cz, w,       d,       h1, s1);
        addSolidBoxT(buf, i, cx, h1,      cz, w*0.78f, d*0.78f, h2, s2);
        addSolidBoxT(buf, i, cx, h1 + h2, cz, w*0.56f, d*0.56f, h3, s3);
        addPyramidT(buf, i, cx, h, cz, w*0.56f, d*0.56f, h*0.34f, heightHaze(brighten(stone, 1.18f), h));
        const float px = w*0.5f - 0.8f, pz = d*0.5f - 0.8f;
        const float ph = h * 0.13f;
        const unsigned int sp = heightHaze(stone, h1);
        addPinnacle(buf, i, cx-px, h1, cz-pz, 1.4f, ph, sp);
        addPinnacle(buf, i, cx+px, h1, cz-pz, 1.4f, ph, sp);
        addPinnacle(buf, i, cx-px, h1, cz+pz, 1.4f, ph, sp);
        addPinnacle(buf, i, cx+px, h1, cz+pz, 1.4f, ph, sp);
        bodyTop = h1;
    } else {
        addSolidBoxT(buf, i, cx, 0.0f, cz, w, d, h, heightHaze(stone, h * 0.5f));
        if (h > 18.0f) addPyramidT(buf, i, cx, h, cz, w, d, h * 0.40f, heightHaze(brighten(stone, 1.12f), h));
        bodyTop = h;
    }
    if (w >= 6.0f && h > 30.0f) {
        const float bh = h * 0.30f, bw = 1.6f, bd = 2.6f;
        const unsigned int bc = brighten(stone, 0.82f);
        addSolidBoxT(buf, i, cx - w*0.5f - bd*0.35f, 0.0f, cz, bd, bw, bh, bc);
        addSolidBoxT(buf, i, cx + w*0.5f + bd*0.35f, 0.0f, cz, bd, bw, bh, bc);
        addSolidBoxT(buf, i, cx, 0.0f, cz - d*0.5f - bd*0.35f, bw, bd, bh, bc);
        addSolidBoxT(buf, i, cx, 0.0f, cz + d*0.5f + bd*0.35f, bw, bd, bh, bc);
    }
    int rows = (int)((bodyTop - 4.0f) / 4.5f);
    if (rows > 11) rows = 11;
    for (int rrow = 0; rrow < rows; ++rrow) {
        float y = 3.5f + rrow * 4.5f;
        if (y > bodyTop - 2.0f) break;
        addWinRow(cx, cz, w, d, y, 0, win);
        addWinRow(cx, cz, w, d, y, 1, win);
        addWinRow(cx, cz, w, d, y, 2, win);
        addWinRow(cx, cz, w, d, y, 3, win);
    }
}

// aguja fina del "mar de agujas" de fondo (texturizada)
static void buildSpire(TexVertex *buf, int &i, float cx, float cz,
                       float w, float h, unsigned int base, float dist) {
    const unsigned int stone = fadeToVoid(warmTint(brighten(base, 1.8f)), dist);
    const float h1 = h * 0.68f;
    addSolidBoxT(buf, i, cx, 0.0f, cz, w, w, h1, heightHaze(stone, h1 * 0.5f));
    addSolidBoxT(buf, i, cx, h1, cz, w * 0.6f, w * 0.6f, h - h1, heightHaze(stone, h1 + (h - h1) * 0.5f));
    addPyramidT(buf, i, cx, h, cz, w * 0.6f, w * 0.6f, h * 0.42f, heightHaze(brighten(stone, 1.1f), h));
    int rows = (int)(h1 / 6.0f); if (rows > 7) rows = 7;
    for (int rrow = 0; rrow < rows; ++rrow) {
        float y = 4.0f + rrow * 6.0f;
        if (y > h1 - 2.0f) break;
        addWinRow(cx, cz, w, w, y, 0, RGBA(235, 165, 85, 255));
    }
}

// baranda de hierro entre dos puntos: balaustres + postes con remate + pasamanos
static void addRailing(TexVertex *buf, int &i, float x0, float z0,
                       float x1, float z1, unsigned int col) {
    const float dx = x1 - x0, dz = z1 - z0;
    const float len = sqrtf(dx * dx + dz * dz);
    int n = (int)(len / 1.3f); if (n < 1) n = 1;
    for (int k = 0; k <= n; ++k) {
        float t = (float)k / n;
        float x = x0 + dx * t, z = z0 + dz * t;
        addSolidBoxT(buf, i, x, 0.0f, z, 0.16f, 0.16f, 1.4f, col);       // balaustre
        if (k % 4 == 0) {                                               // poste + remate
            addSolidBoxT(buf, i, x, 0.0f, z, 0.32f, 0.32f, 1.7f, col);
            addPyramidT(buf, i, x, 1.7f, z, 0.32f, 0.32f, 0.55f, brighten(col, 1.25f));
        }
    }
    const float mx = (x0 + x1) * 0.5f, mz = (z0 + z1) * 0.5f;
    if (fabsf(dx) > fabsf(dz)) addSolidBoxT(buf, i, mx, 1.28f, mz, len, 0.22f, 0.2f, col);
    else                       addSolidBoxT(buf, i, mx, 1.28f, mz, 0.22f, len, 0.2f, col);
}

static void buildSolidWorld() {
    int i = 0;
    g_winVerts = 0;
    // suelo de piedra (plano texturizado grande) bajo todo
    {
        const float S = 135.0f, uv = 2.0f * S / TILE;
        addQuadT(g_solidWorld, i, -S,0.0f,-S,  S,0.0f,-S,  S,0.0f,S,  -S,0.0f,S,
                 0.0f,0.0f, uv,uv, warmTint(RGBA(50, 48, 54, 255)));
    }
    for (int s = 0; s < kStructureCount && s < 63; ++s) {
        if (i > 20500) break;
        const Structure &st = kStructures[s];
        float sx = st.x * WSCALE, sz = st.z * WSCALE;
        float d = sqrtf(sx * sx + sz * sz);
        buildTower(g_solidWorld, i, sx, sz, st.w, st.d, st.h, st.color, d);
    }
    // mar de agujas de fondo (espiral aurea): densidad que se pierde en neblina
    for (int k = 0; k < 60; ++k) {
        if (i > 20000) break;
        float ang = (float)k * 2.3999632f;
        float rad = 36.0f + (float)((k * 37) % 72);  // 36..107
        float cx = cosf(ang) * rad;
        float cz = sinf(ang) * rad;
        float hh = 60.0f + (float)((k * 53) % 175);  // 60..234 (se pierden en la bruma)
        float ww = 3.0f + (float)((k * 7) % 4);      // 3..6
        float dd = sqrtf(cx * cx + cz * cz);
        buildSpire(g_solidWorld, i, cx, cz, ww, hh, kStructures[k % kStructureCount].color, dd);
    }
    // plataforma de piedra del mirador (suelo solido bajo el spawn)
    addSolidBoxT(g_solidWorld, i, 0.0f, -0.5f, -2.5f, 22.0f, 13.0f, 0.55f,
                 warmTint(RGBA(74, 70, 76, 255)));
    // baranda del mirador de spawn (primer plano, como la referencia)
    const unsigned int iron = RGBA(40, 38, 46, 255);
    addRailing(g_solidWorld, i, -9.0f, -7.0f,  9.0f, -7.0f, iron); // frente
    addRailing(g_solidWorld, i, -9.0f, -7.0f, -9.0f,  1.0f, iron); // lado izq
    addRailing(g_solidWorld, i,  9.0f, -7.0f,  9.0f,  1.0f, iron); // lado der
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
        const float cx = st.x * WSCALE, cz = st.z * WSCALE;
        const float x0 = cx - st.w * 0.5f, x1 = cx + st.w * 0.5f;
        const float z0 = cz - st.d * 0.5f, z1 = cz + st.d * 0.5f;
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
            const float cx = st.x * WSCALE, cz = st.z * WSCALE;
            const float x0 = cx - st.w * 0.5f - r, x1 = cx + st.w * 0.5f + r;
            const float z0 = cz - st.d * 0.5f - r, z1 = cz + st.d * 0.5f + r;
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

// modelo detallado del personaje (agente): buildPlayerModel(buf, i)
#include "agent_character.h"

// --- NPCs roboticos (cuerpo + cabeza, wireframe) ---
struct Npc { float x, z; };
static const Npc kNpcs[] = {
    { 6.0f, 4.0f }, { -5.0f, 7.0f }, { 8.0f, -3.0f }, { -7.0f, -6.0f },
    { 3.5f, 9.0f }, { -9.0f, 2.5f }, { 2.0f, 6.5f }, { -3.0f, 4.0f },
    { 5.5f, -5.0f }, { -5.5f, -3.0f }
};
static const int kNpcCount = (int)(sizeof(kNpcs) / sizeof(kNpcs[0]));
static LineVertex __attribute__((aligned(16))) g_npc[1800];
static int g_npcVerts = 0;

// robots solidos variados (oxidado/acero/oscuro/teal), con ojo luminoso
static void buildNpcs() {
    int i = 0;
    static const unsigned int pal[4] = {
        RGBA(150, 92, 58, 255),   // oxidado
        RGBA(120, 132, 152, 255), // acero
        RGBA(74, 78, 92, 255),    // oscuro
        RGBA(84, 168, 160, 255),  // teal
    };
    for (int n = 0; n < kNpcCount; ++n) {
        const unsigned int col = pal[n % 4];
        const float hh = 1.5f + 0.18f * (float)(n % 3);
        const float x = kNpcs[n].x, z = kNpcs[n].z;
        addSolidBox(g_npc, i, x, 0.0f, z, 0.70f, 0.55f, hh, col);                           // cuerpo
        addSolidBox(g_npc, i, x - 0.18f, 0.0f, z, 0.22f, 0.28f, hh * 0.55f, brighten(col, 0.8f)); // pierna
        addSolidBox(g_npc, i, x + 0.18f, 0.0f, z, 0.22f, 0.28f, hh * 0.55f, brighten(col, 0.8f)); // pierna
        addSolidBox(g_npc, i, x, hh, z, 0.50f, 0.50f, 0.50f, brighten(col, 1.12f));          // cabeza
        addSolidBox(g_npc, i, x, hh + 0.16f, z - 0.26f, 0.20f, 0.06f, 0.1f, RGBA(240, 180, 90, 255)); // ojo
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

// textura de ventana (vidriera): panel brillante + parteluz en cruz + marco
#define WTEX 32
static unsigned int __attribute__((aligned(16))) g_winTex[WTEX * WTEX];
static void buildWinTex() {
    for (int y = 0; y < WTEX; ++y)
        for (int x = 0; x < WTEX; ++x) {
            int b;
            int mx = x - WTEX / 2, my = y - WTEX / 2;
            bool frame = (x < 3 || x >= WTEX - 3 || y < 3 || y >= WTEX - 3);
            bool cross = (mx > -2 && mx < 1) || (my > -2 && my < 1);
            if (frame || cross) b = 55;                     // marco / parteluz oscuro
            else { b = 205 + (WTEX - y) * 2; if (b > 255) b = 255; } // panel con brillo
            g_winTex[y * WTEX + x] = RGBA(b, b, b, 255);
        }
    sceKernelDcacheWritebackAll();
}

// textura de metal (acero): estriado vertical + bandas y remaches
#define MTEX 64
static unsigned int __attribute__((aligned(16))) g_metalTex[MTEX * MTEX];
static void buildMetalTex() {
    for (int y = 0; y < MTEX; ++y)
        for (int x = 0; x < MTEX; ++x) {
            int b = 155;
            b += ((x * 5) % 7) - 3;                 // estriado vertical
            if ((y % 16) < 2) b -= 55;              // banda horizontal oscura
            int rx = x % 16, ry = y % 16;
            if (rx < 3 && ry < 3) b += 45;          // remache
            unsigned int h = (unsigned int)(x * 71 + y * 113);
            h ^= h >> 6; h *= 9u; h ^= h >> 4;
            b += (int)(h % 20u) - 10;               // ruido
            if (b < 40) b = 40; if (b > 255) b = 255;
            g_metalTex[y * MTEX + x] = RGBA((int)(b * 0.9f), (int)(b * 0.93f), b, 255); // frio
        }
    sceKernelDcacheWritebackAll();
}

// textura de piedra procedural: patron de ladrillos + grima (para MODULATE)
#define STEX 128
static unsigned int __attribute__((aligned(16))) g_stoneTex[STEX * STEX];
static void buildStoneTex() {
    for (int y = 0; y < STEX; ++y)
        for (int x = 0; x < STEX; ++x) {
            int v = 208;
            unsigned int hsh = (unsigned int)(x * 131 + y * 197);
            hsh ^= hsh >> 7; hsh *= 9u; hsh ^= hsh >> 4; hsh *= 0x27d4eb2du; hsh ^= hsh >> 15;
            v += (int)(hsh % 44u) - 26;                 // grima (ruido)
            int row = y >> 4;
            int xoff = (row & 1) ? 16 : 0;
            if ((y & 15) < 2) v = 118;                  // mortero horizontal
            else if (((x + xoff) & 31) < 2) v = 118;    // mortero vertical
            if (v < 45) v = 45; if (v > 255) v = 255;
            int r = v, g = (int)(v * 0.95f), b = (int)(v * 0.87f); // gris calido
            g_stoneTex[y * STEX + x] = RGBA(r, g, b, 255);
        }
    sceKernelDcacheWritebackAll();
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
            float ax = a.x * WSCALE, az = a.z * WSCALE, bx = b.x * WSCALE, bz = b.z * WSCALE;
            float dx = ax - bx, dz = az - bz;
            float dist = sqrtf(dx * dx + dz * dz);
            if (dist > 10.0f && dist < 80.0f) {
                float ay = a.h * (0.52f + 0.09f * o);
                float by = b.h * (0.52f + 0.09f * o);
                addChain(g_chains, i, ax, ay, az, bx, by, bz, 3.0f + dist * 0.05f, col);
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
    const unsigned int top    = RGBA(32, 27, 26, 255);
    const unsigned int haze   = RGBA(138, 122, 106, 255);
    const unsigned int floorc = RGBA(22, 19, 19, 255);
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

    buildWinTex();
    buildStoneTex();
    buildSolidWorld();
    buildPlayerModel(g_playerModel, g_playerModelVerts);
    buildNpcs();
    buildChains();
    buildFontAtlas();
    initGu();

    SceCtrlData pad;
    float playerX = 0.0f, playerY = 0.0f, playerZ = 0.0f;
    float velY = 0.0f;
    float en = 780.0f;
    const float EN_MAX = 780.0f;
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

            // salto + gravedad + FLOTAR (L = control gravitacional, gasta EN)
            if (grounded && (pad.Buttons & PSP_CTRL_CROSS)) { velY = 0.55f; grounded = 0; }
            if ((pad.Buttons & PSP_CTRL_LTRIGGER) && en > 0.0f) {
                velY += 0.05f;                 // sube/flota
                if (velY > 0.28f) velY = 0.28f;
                velY *= 0.86f;                 // caida amortiguada
                en -= 7.0f;
                grounded = 0;
            } else {
                velY -= 0.02f;                 // gravedad normal
            }
            playerY += velY;
            float gh = groundHeight(playerX, playerZ, playerY);
            if (playerY <= gh) { playerY = gh; velY = 0.0f; grounded = 1; }
            if (grounded && en < EN_MAX) en += 5.0f;
            if (en > EN_MAX) en = EN_MAX;
            if (en < 0.0f) en = 0.0f;

            // recoleccion de recursos por proximidad
            for (int r = 0; r < kResourceCount && r < 64; ++r) {
                if (collected[r]) continue;
                float dx = kResources[r].x * WSCALE - playerX, dz = kResources[r].z * WSCALE - playerZ;
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

        // piedra texturizada (torres, agujas, muros, suelo, plataforma)
        sceGuEnable(GU_TEXTURE_2D);
        sceGuTexMode(GU_PSM_8888, 0, 0, 0);
        sceGuTexImage(0, STEX, STEX, STEX, g_stoneTex);
        sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGB);
        sceGuTexFilter(GU_LINEAR, GU_LINEAR);
        sceGuTexWrap(GU_REPEAT, GU_REPEAT);
        sceGumDrawArray(GU_TRIANGLES, TEX_FLAGS, g_solidVerts, 0, g_solidWorld);

        // ventanas: textura de vidriera (CLAMP: cada ventana = 1 textura)
        sceGuTexImage(0, WTEX, WTEX, WTEX, g_winTex);
        sceGuTexWrap(GU_CLAMP, GU_CLAMP);
        sceGumDrawArray(GU_TRIANGLES, TEX_FLAGS, g_winVerts, 0, g_win);
        sceGuDisable(GU_TEXTURE_2D);

        // cables + robots (sin textura)
        sceGumDrawArray(GU_LINES, LINE_FLAGS, g_chainVerts, 0, g_chains);
        sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g_npcVerts, 0, g_npc);

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
            float dx = kResources[r].x * WSCALE - playerX, dz = kResources[r].z * WSCALE - playerZ;
            float d = sqrtf(dx * dx + dz * dz);
            float t = (d < 14.0f) ? (1.0f - d / 14.0f) : 0.0f; // 0 lejos .. 1 cerca
            int br = 60 + (int)(190 * t);
            unsigned int col = RGBA(br, 90 + (int)(95 * t), 140 + (int)(95 * t), 255);
            LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 40);
            int vi = 0;
            addSolidBox(v, vi, kResources[r].x * WSCALE, kResources[r].y + 0.2f, kResources[r].z * WSCALE,
                        0.7f, 0.7f, 0.7f, col); // gema solida que brilla al acercarse
            sceGumLoadIdentity();
            sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, vi, 0, v);
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
        drawRect(barX, 24, (int)(barW * en / EN_MAX), 6, RGBA(65, 140, 220, 255)); // EN
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
        snprintf(hud, sizeof(hud), "%d", (int)en);
        drawText(barX + barW + 4, 23, 1.0f, RGBA(220, 220, 230, 255), hud);
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
                 "Stick mover  Dpad cam  X saltar  L flotar  START pausa");
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
