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
// Cielo/niebla: gris CALIDO LUMINOSO (no negro). Clave anti-"vacio": el cielo
// debe ser MAS CLARO que la niebla para que las agujas lejanas se lean como
// SILUETAS OSCURAS contra la bruma (look Bloodborne/BLAME), no como fantasmas
// palidos flotando en negro. Antes: cielo 38 (mas oscuro que la niebla 66) -> vacio.
static const unsigned int CLEAR_COLOR = RGBA(104, 98, 87, 255);  // cielo brumoso: overcast calido, el horizonte "brilla"
static const unsigned int HAZE = RGBA(90, 84, 74, 255); // bruma calida: la geometria lejana se DISUELVE aqui (un pelo bajo el cielo)

// Sentido de "cara frontal" para el back-face culling por-pase (R1 rendimiento).
// El winding de cajas/piramides/piso es CONSISTENTE (probado), asi que exactamente
// UNO de GU_CW/GU_CCW es el correcto para TODOS los pases a la vez. Arranca en CW.
// >>> Si al capturar se ven las paredes/piso POR DENTRO (see-through), cambia esta
//     UNICA linea a GU_CCW y recompila: queda resuelto para todo. <<<
#define NOCTIS_FRONTFACE GU_CW
#define WSCALE 2.25f  // separa los edificios (menos juntos) y mas caen fuera de vista (mas FPS)
#define VIEWER_MODE 0     // 1 = visor de personaje; 0 = juego
#define HERO_SHOWCASE 1   // (dentro del visor) 1 = solo el HUNTER en primer plano
#define FLYCAM 0          // 1 = camara de vista elevada (SOLO para capturar el horizonte/abismo)

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

static TexVertex __attribute__((aligned(16))) g_solidWorld[36000]; // piedra texturizada (+ detalle)
static int g_solidVerts = 0;
static TexVertex __attribute__((aligned(16))) g_win[18000];        // ventanas (vidriera texturizada)
static int g_winVerts = 0;
static TexVertex __attribute__((aligned(16))) g_metal[4000];       // metal (baranda)
static int g_metalVerts = 0;
static LineVertex __attribute__((aligned(16))) g_env[3000];        // ambiente (braseros, estandartes)
static int g_envVerts = 0;
static LineVertex __attribute__((aligned(16))) g_void[2000];       // ruinas suspendidas del abismo
static int g_voidVerts = 0;
static LineVertex __attribute__((aligned(16))) g_farSil[2000];     // siluetas colosales del horizonte
static int g_farSilVerts = 0;
static LineVertex __attribute__((aligned(16))) g_vprops[2500];     // props del pueblo (faroles, rejas, tumbas)
static int g_vpropsVerts = 0;
static LineVertex __attribute__((aligned(16))) g_spire[5000];      // mar denso de agujas goticas (referencia)
static int g_spireVerts = 0;

// ===== CULLING por estructura + LOD (rendimiento PSP, directiva 36-37-52) =====
// El mundo se hornea segmentado: [piso][22 torres][60 agujas][cola: mirador/
// arcos/puente/cathedral]. Guardamos el rango de cada torre para dibujar solo
// las cercanas/al frente, y un nivel de LOD (saltar el detalle pesado lejos).
struct StructRange {
    int   sStart, sCount, sDetail;   // rango solido en g_solidWorld; inicio del detalle (para LOD)
    int   wStart, wCount;            // rango de ventanas en g_win
    float cx, cz;                    // centro (mundo) para el test de distancia
};
static StructRange g_srange[256];
static int g_srangeCount = 0;
static int g_floorCount   = 0;   // piso: [0, g_floorCount)
static int g_spireStart   = 0, g_spireEnd = 0;   // agujas: siempre
static int g_tailStart    = 0;   // cola (mirador..cathedral): [g_tailStart, g_solidVerts)
static int g_winTailStart = 0;   // ventanas de agujas+cathedral: siempre
// distancias (unidades de mundo). La niebla ya funde mas alla de ~98.
static const float DRAW_DIST = 58.0f;   // rango de mundo mas corto -> mas FPS
static const float LOD_DIST  = 26.0f;   // entre LOD_DIST y DRAW_DIST -> cuerpo sin detalle
static const float BEHIND_CULL = 24.0f; // dz por detras de la camara -> descartar

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

// ===== VISOR DE CANDIDATOS DE PERSONAJE (comparar 4 disenos, off-screen) =====
#include "char_prims.h"
#include "cand/aya.h"
#include "cand/hunter.h"
#include "cand/ff.h"
#include "cand/wraith.h"
#include "atmosphere.h"   // Vacio/Abismo (ruinas suspendidas) + siluetas colosales lejanas
#include "weapons_fx.h"   // FX/comportamiento distinto por arma a distancia (10)
#include "gravity.h"      // 6 direcciones de gravedad (mecanica firma, directiva 22)
#include "village_props.h" // props del pueblo (faroles calidos, rejas, tumbas) - BLAME!/Bloodborne
#include "spirescape.h"    // MAR DENSO de agujas goticas en bruma (el look de la referencia)
#if VIEWER_MODE
static LineVertex __attribute__((aligned(16))) g_candBuf[4][3300];
static int g_candV[4];
static const char *g_candName[4] = { "AYA", "HUNTER", "FF", "WRAITH" };
static const float g_candX[4]    = { -8.4f, -2.8f, 2.8f, 8.4f };
static void viewerBrighten(LineVertex *b, int n, float f) {
    for (int k = 0; k < n; ++k) {
        unsigned int c = b[k].color;
        int r=(int)((c&0xFF)*f), g=(int)(((c>>8)&0xFF)*f), bl=(int)(((c>>16)&0xFF)*f);
        if(r>255)r=255; if(g>255)g=255; if(bl>255)bl=255;
        b[k].color = RGBA(r, g, bl, 255);
    }
}
static void buildCandidates() {
    g_candV[0] = build_aya(g_candBuf[0]);
    g_candV[1] = build_hunter(g_candBuf[1]);
    g_candV[2] = build_ff(g_candBuf[2]);
    g_candV[3] = build_wraith(g_candBuf[3]);
    // aclarado SOLO en el visor: los trajes son casi negros y en el juego se
    // veran oscuros/atmosfericos, pero aqui hay que apreciar el diseno.
    for (int k = 0; k < 4; ++k) viewerBrighten(g_candBuf[k], g_candV[k], 1.75f);
}
#endif

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
    if (m < 7) return 0u;                         // ~70% APAGADAS (oscuro, claroscuro)
    if (m < 8) return RGBA(70, 92, 140, 255);     // pocas frias tenues
    return amber;                                 // ambar tenue
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

// detalle arquitectonico anti-caja (agente): addTowerDetail/addBridge/addArchSpan
#include "agent_detail.h"

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
                       float w, float d, float h, unsigned int baseColor, float dist,
                       int *detailStartOut = 0) {
    const unsigned int stone = fadeToVoid(warmTint(brighten(baseColor, 1.45f)), dist);
    const unsigned int win   = RGBA(166, 108, 52, 255); // ambar TENUE (menos luz)
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
    // detalle arquitectonico (cornisas, pilastras, quoins, tuberias) anti-caja
    if (detailStartOut) *detailStartOut = i;   // marca para LOD (saltar detalle lejos)
    addTowerDetail(buf, i, cx, cz, w, d, bodyTop, stone);
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
    const unsigned int stone = fadeToVoid(warmTint(brighten(base, 1.4f)), dist);
    const float h1 = h * 0.68f;
    addSolidBoxT(buf, i, cx, 0.0f, cz, w, w, h1, heightHaze(stone, h1 * 0.5f));
    addSolidBoxT(buf, i, cx, h1, cz, w * 0.6f, w * 0.6f, h - h1, heightHaze(stone, h1 + (h - h1) * 0.5f));
    addPyramidT(buf, i, cx, h, cz, w * 0.6f, w * 0.6f, h * 0.42f, heightHaze(brighten(stone, 1.1f), h));
    int rows = (int)(h1 / 6.0f); if (rows > 7) rows = 7;
    for (int rrow = 0; rrow < rows; ++rrow) {
        float y = 4.0f + rrow * 6.0f;
        if (y > h1 - 2.0f) break;
        addWinRow(cx, cz, w, w, y, 0, RGBA(166, 108, 52, 255));
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

// ===== CIUDAD ABIERTA (grilla grande estilo Spider-Man 3 PSP) =====
struct CityBldg { float x, z, w, d, h; unsigned int color; };
static CityBldg g_city[256];
static int g_cityCount = 0;
#include "city.h"          // buildCity() llena g_city (16 edificios espaciados, plaza al centro)
#include "gothic_bldg.h"   // buildGothicBldg(): edificio gotico (mansiones/casonas)
#include "cathedral.h"     // buildCathedral(): catedral Yharnam ornamentada (landmarks, h>120)

// edificio SIMPLE de ciudad (pocos verts -> muchos edificios + culling = rinde)
static void buildCityBldg(TexVertex *buf, int &i, float cx, float cz,
                          float w, float d, float h, unsigned int baseColor, float dist,
                          int *detailStartOut) {
    const unsigned int stone = fadeToVoid(brighten(baseColor, 1.35f), dist);
    const unsigned int win   = RGBA(150, 118, 66, 255);          // ventana ambar tenue
    if (h > 70.0f) {                                             // rascacielos: 2 tramos + remate
        float h1 = h * 0.60f;
        addSolidBoxT(buf, i, cx, 0.0f, cz, w, d, h1, heightHaze(stone, h1 * 0.5f));
        addSolidBoxT(buf, i, cx, h1, cz, w * 0.74f, d * 0.74f, h - h1, heightHaze(stone, h1 + (h - h1) * 0.5f));
        addPyramidT(buf, i, cx, h, cz, w * 0.74f, d * 0.74f, h * 0.08f, heightHaze(brighten(stone, 1.12f), h));
    } else {                                                     // edificio: caja + parapeto
        addSolidBoxT(buf, i, cx, 0.0f, cz, w, d, h, heightHaze(stone, h * 0.5f));
        addSolidBoxT(buf, i, cx, h, cz, w * 1.04f, d * 1.04f, 1.3f, heightHaze(brighten(stone, 0.82f), h));
    }
    if (detailStartOut) *detailStartOut = i;                     // sin detalle pesado
    int rows = (int)((h - 5.0f) / 6.0f); if (rows > 12) rows = 12;
    for (int r = 0; r < rows; ++r) {
        float y = 4.0f + r * 6.0f; if (y > h - 3.0f) break;
        addWinRow(cx, cz, w, d, y, 0, win); addWinRow(cx, cz, w, d, y, 1, win);
        addWinRow(cx, cz, w, d, y, 2, win); addWinRow(cx, cz, w, d, y, 3, win);
    }
}

static void buildSolidWorld() {
    int i = 0;
    g_winVerts = 0;
    // suelo de piedra (plano texturizado grande) bajo todo
    {
        // PISO: adoquin (genGround, alto contraste) con color CLARO (el MODULATE
        // multiplica textura*color; textura ~90/255, asi que el color debe ser
        // claro para que se lea). uv ~ 12 unidades/repeticion -> losas ~1.5u.
        const float S = 120.0f;                 // suelo mas chico (jugador acotado a r~82) -> menos fill/minificacion (R5)
        const float uv = 2.0f * S / 12.0f;      // densidad de losa fija a 12u (indep. de S) -> losas visibles cerca
        addQuadT(g_solidWorld, i, -S,0.0f,-S,  S,0.0f,-S,  S,0.0f,S,  -S,0.0f,S,
                 0.0f,0.0f, uv,uv, RGBA(216, 198, 174, 255));   // adoquin gotico claro/calido (se ve, no gris plano)
    }
    g_floorCount = i;      // piso = [0, g_floorCount)  (siempre se dibuja)
    g_srangeCount = 0;
    for (int s = 0; s < g_cityCount && g_srangeCount < 256; ++s) {
        if (i > 33500) break;
        const CityBldg &b = g_city[s];
        float dd = sqrtf(b.x * b.x + b.z * b.z);
        StructRange &r = g_srange[g_srangeCount];
        r.sStart = i; r.wStart = g_winVerts; r.cx = b.x; r.cz = b.z; r.sDetail = i;
        buildGothicBldg(g_solidWorld, i, b.x, b.z, b.w, b.d, b.h, b.color, dd, &r.sDetail);
        r.sCount = i - r.sStart; r.wCount = g_winVerts - r.wStart;
        g_srangeCount++;
    }
    g_winTailStart = g_winVerts;   // ventanas de aqui en adelante (agujas+cathedral) = siempre
    // mar de agujas de fondo (espiral aurea): densidad que se pierde en neblina
    g_spireStart = i;
    for (int k = 0; k < 0; ++k) {   // agujas quitadas: la ciudad las reemplaza
        if (i > 34000) break;
        float ang = (float)k * 2.3999632f;
        float rad = 36.0f + (float)((k * 37) % 72);  // 36..107
        float cx = cosf(ang) * rad;
        float cz = sinf(ang) * rad;
        float hh = 60.0f + (float)((k * 53) % 175);  // 60..234 (se pierden en la bruma)
        float ww = 3.0f + (float)((k * 7) % 4);      // 3..6
        float dd = sqrtf(cx * cx + cz * cz);
        buildSpire(g_solidWorld, i, cx, cz, ww, hh, kStructures[k % kStructureCount].color, dd);
    }
    g_spireEnd = i;        // agujas = [g_spireStart, g_spireEnd)  (siempre)
    g_tailStart = i;       // cola (mirador/arcos/puente/cathedral) = [g_tailStart, g_solidVerts)  (siempre)
    // plataforma QUITADA: el jugador se para directo sobre el PISO grande (asi se
    // ve la textura de losas bajo el personaje, no la plataforma chica tapandola).
    // baranda de METAL del mirador -> buffer g_metal (textura de acero)
    int mi = 0;
    const unsigned int iron = RGBA(120, 124, 140, 255); // acero claro (para que se vea la textura)
    addRailing(g_metal, mi, -9.0f, -7.0f,  9.0f, -7.0f, iron); // frente
    addRailing(g_metal, mi, -9.0f, -7.0f, -9.0f,  1.0f, iron); // lado izq
    addRailing(g_metal, mi,  9.0f, -7.0f,  9.0f,  1.0f, iron); // lado der
    g_metalVerts = mi;
    // arcos goticos como portico del mirador (detalle del agente)
    {
        const unsigned int arc = warmTint(brighten(RGBA(40, 38, 48, 255), 1.4f));
        addArchSpan(g_solidWorld, i, -13.0f, -9.0f, 7.0f, 0.0f, arc);
        addArchSpan(g_solidWorld, i,  13.0f, -9.0f, 7.0f, 0.0f, arc);
        // un puente/pasarela alto cruzando el fondo del mirador
        addBridge(g_solidWorld, i, -30.0f, 26.0f, -34.0f, 30.0f, 26.0f, -34.0f, arc);
    }
    // The Cathedral: landmark del fondo (Benjamin la dejo, se ve bien)
    buildTower(g_solidWorld, i, 0.0f, -170.0f, 96.0f, 96.0f, 480.0f, RGBA(38, 34, 58, 255), 0.0f);
    g_solidVerts = i;
}

// altura del suelo bajo el jugador (0, o el techo de una estructura si esta
// por encima de el). Permite pararse en azoteas al caer sobre ellas.
static float groundHeight(float px, float pz, float py) {
    float g = 0.0f;   // suelo/plaza en y=0; los TECHOS de los edificios se pueden pisar
    for (int s = 0; s < g_cityCount; ++s) {
        const CityBldg &b = g_city[s];
        const float x0 = b.x - b.w * 0.5f, x1 = b.x + b.w * 0.5f;
        const float z0 = b.z - b.d * 0.5f, z1 = b.z + b.d * 0.5f;
        if (px >= x0 && px <= x1 && pz >= z0 && pz <= z1) {
            if (b.h > g && b.h <= py + 1.0f) g = b.h;
        }
    }
    return g;
}

// colision con muros: bloquea si el jugador (radio r) esta DENTRO de la huella
// de una estructura y por DEBAJO de su techo (no bloquea al estar encima).
static bool blocked(float px, float pz, float py) {
    const float r = 1.1f;
    if (px * px + pz * pz > 82.0f * 82.0f) return true;   // borde del pueblo/balcon: no se sale hacia el mar de agujas
    for (int s = 0; s < g_cityCount; ++s) {
        const CityBldg &b = g_city[s];
        if (py < b.h - 0.8f) {
            const float x0 = b.x - b.w * 0.5f - r, x1 = b.x + b.w * 0.5f + r;
            const float z0 = b.z - b.d * 0.5f - r, z1 = b.z + b.d * 0.5f + r;
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

// personaje por PIEZAS animables (agente): cada pieza con su pivot en el origen
static LineVertex __attribute__((aligned(16))) g_chUpper[700];
static LineVertex __attribute__((aligned(16))) g_chHead[240];
static LineVertex __attribute__((aligned(16))) g_chLeg[240];
static LineVertex __attribute__((aligned(16))) g_chArm[240];
static LineVertex __attribute__((aligned(16))) g_chSword[240];
static int g_chUpperV = 0, g_chHeadV = 0, g_chLegV = 0, g_chArmV = 0, g_chSwordV = 0;
// buildChar_upper / _head / _leg / _arm / _sword (partes con pivot en el origen)
#include "agent_character.h"

// PERSONAJE DEL JUEGO: el HUNTER (encapuchado gotico, elegido por Benjamin).
// Construido con las primitivas organicas (cilindros conicos + elipsoides +
// abrigo hasta la rodilla con piernas a la vista), NO cubos.
static LineVertex __attribute__((aligned(16))) g_hero[3200];
static int g_heroV = 0;
static void buildHero() {
    g_heroV = build_hunter(g_hero);
    // aclarado leve para que se lea en la escena sin perder lo tenebroso.
    for (int k = 0; k < g_heroV; ++k) {
        unsigned int c = g_hero[k].color;
        int r=(int)((c&0xFF)*1.28f), g=(int)(((c>>8)&0xFF)*1.28f), b=(int)(((c>>16)&0xFF)*1.28f);
        if(r>255)r=255; if(g>255)g=255; if(b>255)b=255;
        g_hero[k].color = RGBA(r, g, b, 255);
    }
}

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
static int g_npcKilled[64] = {0};   // robots derribados a tiros (no se dibujan)

// ===== ARMA / DISPAROS (L apunta, R dispara) =====
struct Shot { float x, y, z, vx, vy, vz; int life; unsigned int col; float size; int pierce; };
static Shot g_shots[24];
// chispa/estallido al derribar un robot (feedback de impacto)
struct Spark { float x, y, z; int life; };
static Spark g_sparks[12];

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
        if (g_npcKilled[n]) continue;   // derribado -> no se dibuja
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

// generadores de textura + ambiente del agente (mas detallados)
#include "agent_textures.h"
#include "gothic_tex.h"    // genGothicFacade: fachada gotica (ventanas/arcos EN la textura, no geometria)
#include "ground_tex.h"    // genGround: adoquin/losas gotico para el PISO de todo el mundo

// ---- SWIZZLE de texturas ----
// En la PSP REAL, una textura NO swizzled con filtrado se muestrea con muchos
// cache-miss -> rendimiento pesimo (el emulador no lo nota, por eso corria a 60
// en PPSSPP y a 2 FPS en hardware). Swizzlear reordena la textura al layout que
// la GE lee en bloques de 16x8 bytes -> gran salto de FPS en PSP real.
// width = ANCHO EN BYTES (para 8888 = px*4), height = filas. Se corre 1 vez.
static void swizzleTex(unsigned char *out, const unsigned char *in, int width, int height) {
    const int rowblocks = width / 16;
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            const int blockx = i / 16, blocky = j / 8;
            const int x = i - blockx * 16, y = j - blocky * 8;
            const int block = blockx + blocky * rowblocks;
            out[block * 128 + x + y * 16] = in[i + j * width];  // 128 = 16*8 bytes/bloque
        }
    }
}

// texturas del juego (rellenadas por los generadores del agente) + copia swizzled
#define WTEX 32
static unsigned int __attribute__((aligned(16))) g_winTex[WTEX * WTEX];
static unsigned int __attribute__((aligned(16))) g_winTexS[WTEX * WTEX];
static void buildWinTex() { genWindow(g_winTex, WTEX); swizzleTex((unsigned char*)g_winTexS, (const unsigned char*)g_winTex, WTEX * 4, WTEX); sceKernelDcacheWritebackAll(); }

#define MTEX 64
static unsigned int __attribute__((aligned(16))) g_metalTex[MTEX * MTEX];
static unsigned int __attribute__((aligned(16))) g_metalTexS[MTEX * MTEX];
static void buildMetalTex() { genMetal(g_metalTex, MTEX); swizzleTex((unsigned char*)g_metalTexS, (const unsigned char*)g_metalTex, MTEX * 4, MTEX); sceKernelDcacheWritebackAll(); }

#define STEX 128
static unsigned int __attribute__((aligned(16))) g_stoneTex[STEX * STEX];
static unsigned int __attribute__((aligned(16))) g_stoneTexS[STEX * STEX];
static void buildStoneTex() { genStone(g_stoneTex, STEX); swizzleTex((unsigned char*)g_stoneTexS, (const unsigned char*)g_stoneTex, STEX * 4, STEX); sceKernelDcacheWritebackAll(); }

// fachada gotica (ventanas ojivales en la TEXTURA): los edificios simples la usan
static unsigned int __attribute__((aligned(16))) g_facadeTex[STEX * STEX];
static unsigned int __attribute__((aligned(16))) g_facadeTexS[STEX * STEX];
static void buildFacadeTex() { genGothicFacade(g_facadeTex, STEX); swizzleTex((unsigned char*)g_facadeTexS, (const unsigned char*)g_facadeTex, STEX * 4, STEX); sceKernelDcacheWritebackAll(); }

// adoquin/losas para el PISO de todo el mundo
static unsigned int __attribute__((aligned(16))) g_groundTex[STEX * STEX];
static unsigned int __attribute__((aligned(16))) g_groundTexS[STEX * STEX];
static void buildGroundTex() { genGround(g_groundTex, STEX); swizzleTex((unsigned char*)g_groundTexS, (const unsigned char*)g_groundTex, STEX * 4, STEX); sceKernelDcacheWritebackAll(); }

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

// ambiente gotico: braseros con llama calida en el mirador (agente)
static void buildEnv() {
    int i = 0;
    addBrazier(g_env, i, -8.5f, -6.2f);   // solo 2 braseros al frente (menos luces)
    addBrazier(g_env, i,  8.5f, -6.2f);
    g_envVerts = i;
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
    const unsigned int top    = RGBA(44, 41, 36, 255);   // cielo brumoso alto (oscuro, calido)
    const unsigned int haze   = RGBA(72, 67, 60, 255);   // banda de bruma en el horizonte
    const unsigned int floorc = RGBA(24, 22, 20, 255);   // niebla baja entre las agujas (mas oscura)
    gradQuad(0, 150, top, haze);
    gradQuad(150, SCR_HEIGHT, haze, floorc);
}
// gradiente vertical (franjas laterales, para la vineta)
static void gradQuadV(int x0, int x1, unsigned int cL, unsigned int cR) {
    GradVertex *v = (GradVertex *)sceGuGetMemory(sizeof(GradVertex) * 6);
    v[0] = { cL, (short)x0, 0, 0 };
    v[1] = { cR, (short)x1, 0, 0 };
    v[2] = { cR, (short)x1, (short)SCR_HEIGHT, 0 };
    v[3] = { cL, (short)x0, 0, 0 };
    v[4] = { cR, (short)x1, (short)SCR_HEIGHT, 0 };
    v[5] = { cL, (short)x0, (short)SCR_HEIGHT, 0 };
    sceGuDrawArray(GU_TRIANGLES, GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 6, 0, v);
}
// vineta cinematografica: bordes oscuros que se funden hacia el centro
static void drawVignette() {
    const unsigned int e  = RGBA(0, 0, 0, 155);
    const unsigned int eB = RGBA(0, 0, 0, 200);
    const unsigned int t  = RGBA(0, 0, 0, 0);
    gradQuad(0, 70, e, t);                          // arriba
    gradQuad(SCR_HEIGHT - 80, SCR_HEIGHT, t, eB);   // abajo (mas oscuro)
    gradQuadV(0, 76, e, t);                         // izquierda
    gradQuadV(SCR_WIDTH - 76, SCR_WIDTH, t, e);     // derecha
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

#if VIEWER_MODE
// dibuja los 4 candidatos en fila sobre un turntable, sobre un piso oscuro.
static void drawCandidates() {
    static float t = 0.0f; t += 0.03f;
    const float yaw = 3.14159265f + 0.55f * sinf(t);  // frente a la camara, 3/4 suave

    sceGumMatrixMode(GU_PROJECTION);
    sceGumLoadIdentity();
    sceGumPerspective(55.0f, 16.0f / 9.0f, 0.5f, 1000.0f);
    sceGumMatrixMode(GU_VIEW);
    sceGumLoadIdentity();
    {
        ScePspFVector3 camOff = { 0.0f, -2.05f, -15.0f };
        sceGumTranslate(&camOff);
    }
    sceGumMatrixMode(GU_MODEL);

    // piso oscuro (para que no floten y se lea la silueta)
    {
        LineVertex *g = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 6);
        const unsigned int fc = RGBA(24, 22, 28, 255), fb = RGBA(9, 8, 12, 255);
        int gi = 0;
        g[gi++] = { fb, -15.0f, 0.0f, 3.5f };  g[gi++] = { fb, 15.0f, 0.0f, 3.5f };  g[gi++] = { fc, 15.0f, 0.0f, -6.0f };
        g[gi++] = { fb, -15.0f, 0.0f, 3.5f };  g[gi++] = { fc, 15.0f, 0.0f, -6.0f }; g[gi++] = { fc, -15.0f, 0.0f, -6.0f };
        sceGumLoadIdentity();
        sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, 6, 0, g);
    }

    for (int k = 0; k < 4; ++k) {
        sceGumLoadIdentity();
        ScePspFVector3 pos = { g_candX[k], 0.0f, 0.0f };
        sceGumTranslate(&pos);
        ScePspFVector3 rot = { 0.0f, yaw, 0.0f };
        sceGumRotateXYZ(&rot);
        sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g_candV[k], 0, g_candBuf[k]);
    }

    // etiquetas (2D)
    sceGuDisable(GU_DEPTH_TEST);
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    fontTexOn();
    drawText(196, 10, 1.0f, RGBA(215, 205, 230, 255), "CANDIDATOS DE PERSONAJE");
    const int lblX[4] = { 74, 150, 286, 356 };
    for (int k = 0; k < 4; ++k)
        drawText(lblX[k], 214, 1.0f, RGBA(240, 180, 120, 255), g_candName[k]);
    sceGuDisable(GU_TEXTURE_2D);
    sceGuDisable(GU_BLEND);
    sceGuEnable(GU_DEPTH_TEST);
}
#endif

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
    sceGuFrontFace(NOCTIS_FRONTFACE); // cara exterior = NOCTIS_FRONTFACE (winding probado consistente)
    sceGuDisable(GU_CULL_FACE);  // default OFF; el culling se ACTIVA por-pase en el lazo de render
                                 // (paredes/piso/agujas SI; ventanas/hero/braseros NO -> winding no probado).
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
    buildFacadeTex();
    buildGroundTex();
    buildMetalTex();
    buildCity();          // genera la ciudad (g_city) ANTES del mundo/colision
    buildSolidWorld();
    buildHero();
#if VIEWER_MODE
    buildCandidates();
#endif
    buildNpcs();
    // buildChains();   // cadenas QUITADAS (cosas entremedio que bajan FPS); g_chainVerts=0
    buildEnv();
    g_farSilVerts = buildFarSilhouettes(g_farSil);
    g_voidVerts   = buildVoidLayer(g_void);
    g_vpropsVerts = buildVillageProps(g_vprops);
    g_spireVerts  = buildSpirescape(g_spire);
    buildFontAtlas();
    initGu();

    SceCtrlData pad;
    float playerX = 0.0f, playerY = 0.0f, playerZ = 0.0f;
    float velY = 0.0f, velX = 0.0f, velZ = 0.0f;
    int   coyote = 0, prevJump = 0, jumpBuf = 0;
    float en = 780.0f;
    const float EN_MAX = 780.0f;
    // constantes de movilidad (diseno del agente)
    const float DEADZONE = 0.18f, RUN_SPEED = 0.22f, ACCEL_GND = 0.20f, ACCEL_AIR = 0.09f, STOP_FRIC = 0.22f, CAM_SPEED = 0.03f;
    const float GRAVITY = 0.020f, JUMP_VEL = 0.55f, SHORTHOP = 0.50f;
    const int   COYOTE_MAX = 6, JUMPBUF_MAX = 6;
    const float FLOAT_LIFT = 0.030f, FLOAT_GRAV = 0.006f, FLOAT_UPCAP = 0.12f, FLOAT_FALLCAP = -0.09f, EN_FLOAT = 6.0f, EN_REGEN = 5.0f;
    int   grounded = 1;
    float camYaw = 0.0f;    // camara FIJA (no rota); solo sigue la posicion del jugador
    float heroYaw = 0.0f;   // hacia donde encara el modelo (gira al avanzar)
    int   paused = 0, prevStart = 0;
    float walkPhase = 0.0f, idleT = 0.0f;   // animacion del personaje
    int   moving = 0;

    int fps = 0, frameAccum = 0;
    long long lastTick = sceKernelGetSystemTimeWide();
    char hud[80];

    int collected[64] = {0};
    int collectedCount = 0, pickTimer = 0, pickedType = 0;

    // ---- armas (L apunta, R dispara; D-pad izq/der cambia arma; O melee) ----
    int   aiming = 0, fireCD = 0, reloadCD = 0, muzzle = 0;
    unsigned int muzzleCol = RGBA(255, 235, 170, 255);
    int   curRanged = 0, curMelee = 0;
    int   ammoMag[16];
    for (int k = 0; k < kRangedCount && k < 16; ++k) ammoMag[k] = kRanged[k].magazine;
    int   prevDR = 0, prevDL = 0, prevDU = 0, prevDD = 0, prevCircle = 0;
    int   meleeCD = 0, meleeFx = 0;
    for (int s = 0; s < 24; ++s) g_shots[s].life = 0;
    for (int s = 0; s < 12; ++s) g_sparks[s].life = 0;
    const int   SHOT_LIFE = 55;
    // ---- gravedad (Triangulo cicla 6 direcciones) ----
    int   gravG = 0, prevTri = 0;
    float gvr = 0.0f, gvf = 0.0f, gvg = 0.0f;   // vel en el plano (right,fwd) + a lo largo de la gravedad
    static const char *kGravName[6] = { "ABAJO", "ARRIBA", "+X", "-X", "+Z", "-Z" };

    while (!g_exit) {
        sceCtrlReadBufferPositive(&pad, 1);
        // START = pausa (toggle con deteccion de flanco). Ya NO sale. HOME sale.
        int startNow = (pad.Buttons & PSP_CTRL_START) ? 1 : 0;
        if (startNow && !prevStart) paused = !paused;
        prevStart = startNow;

        if (!paused) {
            // ===== stick -> deseo en MUNDO FIJO (girar la camara NO cambia el control) =====
            float ax = ((int)pad.Lx - 128) / 128.0f;
            float ay = ((int)pad.Ly - 128) / 128.0f;
            float mag = sqrtf(ax * ax + ay * ay);
            float wishX = 0.0f, wishZ = 0.0f;
            if (mag > DEADZONE) {
                float k = (mag - DEADZONE) / (1.0f - DEADZONE); if (k > 1.0f) k = 1.0f;
                wishX = (ax / mag) * k;   // derecha = +X
                wishZ = (ay / mag) * k;   // arriba (ay<0) = adelante (-Z)
            }
            if (gravG == 0) {   // ===== MOVIMIENTO NORMAL (gravedad abajo, sin cambios) =====
            // ===== aceleracion / friccion (fluido) =====
            float targetVX = wishX * RUN_SPEED, targetVZ = wishZ * RUN_SPEED;
            float ctrl = grounded ? ACCEL_GND : ACCEL_AIR;
            velX += (targetVX - velX) * ctrl;
            velZ += (targetVZ - velZ) * ctrl;
            if (grounded && wishX == 0.0f && wishZ == 0.0f) { velX -= velX * STOP_FRIC; velZ -= velZ * STOP_FRIC; }
            // ===== colision por ejes separados (desliza por muros) =====
            float nx = playerX + velX;
            if (!blocked(nx, playerZ, playerY)) playerX = nx; else velX = 0.0f;
            float nz = playerZ + velZ;
            if (!blocked(playerX, nz, playerY)) playerZ = nz; else velZ = 0.0f;
            // ===== el PERSONAJE gira hacia donde avanza; la CAMARA NO rota =====
            // Antes la camara rotaba al moverse y desorientaba: "adelante" en el
            // stick terminaba moviendote de lado. Ahora la camara queda fija
            // (solo sigue la POSICION) y solo el modelo encara el avance.
            if (velX * velX + velZ * velZ > 0.004f) {
                float moveAng = atan2f(velX, -velZ);
                float dA = moveAng - heroYaw;
                while (dA >  3.14159265f) dA -= 6.28318531f;
                while (dA < -3.14159265f) dA += 6.28318531f;
                heroYaw += dA * 0.20f;   // el personaje encara la direccion de avance
            }
            }   // fin MOVIMIENTO NORMAL (gravG==0); el resto de gravedades cae abajo
            // ===== ARMAS: L apunta, R dispara; D-pad cambia; Circulo melee =====
            // cambio de arma a distancia (D-pad izq/der) y melee (D-pad arr/aba)
            int dR = (pad.Buttons & PSP_CTRL_RIGHT) ? 1 : 0, dL = (pad.Buttons & PSP_CTRL_LEFT) ? 1 : 0;
            int dU = (pad.Buttons & PSP_CTRL_UP) ? 1 : 0,    dD = (pad.Buttons & PSP_CTRL_DOWN) ? 1 : 0;
            if (dR && !prevDR) curRanged = (curRanged + 1) % kRangedCount;
            if (dL && !prevDL) curRanged = (curRanged - 1 + kRangedCount) % kRangedCount;
            if (dU && !prevDU) curMelee  = (curMelee + 1) % kMeleeCount;
            if (dD && !prevDD) curMelee  = (curMelee - 1 + kMeleeCount) % kMeleeCount;
            prevDR = dR; prevDL = dL; prevDU = dU; prevDD = dD;
            // Triangulo cicla la DIRECCION de gravedad (0=abajo .. 5=-Z)
            { int tri = (pad.Buttons & PSP_CTRL_TRIANGLE) ? 1 : 0;
              if (tri && !prevTri) { gravG = (gravG + 1) % 6; velX = velY = velZ = 0.0f; gvr = gvf = gvg = 0.0f; }
              prevTri = tri; }

            aiming = (pad.Buttons & PSP_CTRL_LTRIGGER) ? 1 : 0;
            if (aiming) {   // al apuntar, el personaje encara al frente (-Z)
                float dA = 0.0f - heroYaw;
                while (dA >  3.14159265f) dA -= 6.28318531f;
                while (dA < -3.14159265f) dA += 6.28318531f;
                heroYaw += dA * 0.35f;
            }
            if (fireCD > 0) fireCD--;
            if (reloadCD > 0) reloadCD--;
            if (muzzle > 0) muzzle--;
            if (meleeCD > 0) meleeCD--;
            if (meleeFx > 0) meleeFx--;
            // --- disparo a distancia (usa stats + FX del arma actual) ---
            if (aiming && (pad.Buttons & PSP_CTRL_RTRIGGER) && fireCD == 0 && reloadCD == 0) {
                WeaponFX wf = weaponFX(curRanged);
                int cost = kRanged[curRanged].energyCost;
                if (ammoMag[curRanged] > 0 && en >= (float)cost) {
                    for (int b = 0; b < wf.spread; ++b) {
                        float off = (wf.spread > 1) ? ((float)b - (wf.spread - 1) * 0.5f) * 0.11f : 0.0f;
                        float ya = heroYaw + off, fx = sinf(ya), fz = -cosf(ya);
                        float mx = playerX + fx * 0.7f, my = playerY + 1.5f, mz = playerZ + fz * 0.7f;
                        for (int s = 0; s < 24; ++s) if (g_shots[s].life <= 0) {
                            g_shots[s].x = mx; g_shots[s].y = my; g_shots[s].z = mz;
                            g_shots[s].vx = fx * wf.speed; g_shots[s].vy = 0.0f; g_shots[s].vz = fz * wf.speed;
                            g_shots[s].life = SHOT_LIFE; g_shots[s].col = wf.col; g_shots[s].size = wf.size; g_shots[s].pierce = wf.pierce;
                            break;
                        }
                    }
                    ammoMag[curRanged]--; en -= (float)cost; fireCD = wf.fireFrames;
                    muzzle = 4; muzzleCol = weaponMuzzle(curRanged);
                } else if (ammoMag[curRanged] == 0) {   // recarga automatica
                    ammoMag[curRanged] = kRanged[curRanged].magazine;
                    reloadCD = kRanged[curRanged].reloadMs / 16;
                }
            }
            // --- melee (Circulo): golpe en arco al frente ---
            if ((pad.Buttons & PSP_CTRL_CIRCLE) && !prevCircle && meleeCD == 0) {
                meleeCD = kMelee[curMelee].speedMs / 16; meleeFx = 8;
                float reach = kMelee[curMelee].reach * 0.20f;
                float fx = sinf(heroYaw), fz = -cosf(heroYaw);
                for (int n = 0; n < kNpcCount; ++n) {
                    if (g_npcKilled[n]) continue;
                    float rx = kNpcs[n].x - playerX, rz = kNpcs[n].z - playerZ;
                    float dd = sqrtf(rx*rx + rz*rz);
                    if (dd > reach + 0.7f) continue;
                    if (dd > 0.01f && (rx*fx + rz*fz) / dd < 0.30f) continue;   // solo al frente
                    g_npcKilled[n] = 1; buildNpcs();
                    for (int q = 0; q < 12; ++q) if (g_sparks[q].life <= 0) {
                        g_sparks[q].x = kNpcs[n].x; g_sparks[q].y = 1.0f; g_sparks[q].z = kNpcs[n].z; g_sparks[q].life = 14; break;
                    }
                }
            }
            prevCircle = (pad.Buttons & PSP_CTRL_CIRCLE) ? 1 : 0;
            if (gravG == 0) {
            // ===== salto: coyote time + buffer + salto variable =====
            if (grounded) coyote = COYOTE_MAX; else if (coyote > 0) coyote--;
            int jumpNow = (pad.Buttons & PSP_CTRL_CROSS) ? 1 : 0;
            if (jumpNow && !prevJump) jumpBuf = JUMPBUF_MAX; else if (jumpBuf > 0) jumpBuf--;
            if (jumpBuf > 0 && coyote > 0) { velY = JUMP_VEL; grounded = 0; coyote = 0; jumpBuf = 0; }
            if (!jumpNow && velY > 0.0f) velY *= SHORTHOP;
            prevJump = jumpNow;
            // ===== gravedad / PLANEO (CUADRADO gasta EN; L ahora apunta) =====
            if ((pad.Buttons & PSP_CTRL_SQUARE) && en > 0.0f) {
                velY += FLOAT_LIFT; if (velY > FLOAT_UPCAP)   velY = FLOAT_UPCAP;
                velY -= FLOAT_GRAV; if (velY < FLOAT_FALLCAP) velY = FLOAT_FALLCAP;
                en -= EN_FLOAT; grounded = 0;
            } else {
                velY -= GRAVITY;
            }
            // ===== integra vertical + suelo/azoteas =====
            playerY += velY;
            float gh = groundHeight(playerX, playerZ, playerY);
            if (playerY <= gh) { playerY = gh; velY = 0.0f; grounded = 1; } else grounded = 0;
            // ===== energia (EN) =====
            if (grounded && en < EN_MAX) en += EN_REGEN;
            if (en > EN_MAX) en = EN_MAX;
            if (en < 0.0f) en = 0.0f;
            // ===== animacion (segun velocidad real) =====
            moving = (velX * velX + velZ * velZ > 0.002f) ? 1 : 0;
            if (moving) walkPhase += 0.17f;   // ritmo de paso mas lento (acorde al RUN_SPEED bajo)
            idleT += 0.05f;
            } else {
                // ===== GRAVEDAD NO-ABAJO: caer/mover/saltar segun gravDir (v1 wall-walk) =====
                float rx, ry, rz, ffx, ffy, ffz, gx, gy, gz;
                gravBasis(gravG, &rx, &ry, &rz, &ffx, &ffy, &ffz);
                gravDirVec(gravG, &gx, &gy, &gz);
                float ctrlg = grounded ? ACCEL_GND : ACCEL_AIR;
                float wr = wishX * RUN_SPEED, wf = (-wishZ) * RUN_SPEED;   // wishZ: arriba(ay<0)=adelante
                gvr += (wr - gvr) * ctrlg;
                gvf += (wf - gvf) * ctrlg;
                if (grounded && wishX == 0.0f && wishZ == 0.0f) { gvr -= gvr * STOP_FRIC; gvf -= gvf * STOP_FRIC; }
                int jn = (pad.Buttons & PSP_CTRL_CROSS) ? 1 : 0;
                if (jn && !prevJump && grounded) { gvg = -JUMP_VEL; grounded = 0; }
                if (!jn && gvg < 0.0f) gvg *= SHORTHOP;
                prevJump = jn;
                if ((pad.Buttons & PSP_CTRL_SQUARE) && en > 0.0f) {       // planeo a lo largo de gravDir
                    gvg -= FLOAT_LIFT; if (gvg < -FLOAT_UPCAP) gvg = -FLOAT_UPCAP;
                    en -= EN_FLOAT; grounded = 0;
                } else {
                    gvg += GRAVITY;
                }
                float vX = rx * gvr + ffx * gvf + gx * gvg;
                float vY = ry * gvr + ffy * gvf + gy * gvg;
                float vZ = rz * gvr + ffz * gvf + gz * gvg;
                grounded = 0;
                float nX = playerX + vX;
                if (nX > -140.0f && nX < 140.0f && !blocked(nX, playerZ, playerY)) playerX = nX; else if (gx != 0.0f) { grounded = 1; gvg = 0.0f; }
                float nZ = playerZ + vZ;
                if (nZ > -140.0f && nZ < 140.0f && !blocked(playerX, nZ, playerY)) playerZ = nZ; else if (gz != 0.0f) { grounded = 1; gvg = 0.0f; }
                float nY = playerY + vY;
                if (nY < 0.0f) { nY = 0.0f; if (gy < 0.0f) { grounded = 1; gvg = 0.0f; } }
                if (!blocked(playerX, playerZ, nY)) playerY = nY; else if (gy != 0.0f) { grounded = 1; gvg = 0.0f; }
                if (grounded && en < EN_MAX) en += EN_REGEN;
                if (en > EN_MAX) en = EN_MAX; if (en < 0.0f) en = 0.0f;
                if (gvr * gvr + gvf * gvf > 0.004f) heroYaw = atan2f(gvr, gvf);   // encara el avance en el plano
                moving = (gvr * gvr + gvf * gvf > 0.002f) ? 1 : 0;
                idleT += 0.05f;
            }

            // ===== RED DE SEGURIDAD: si el jugador se fue al VACIO, reset al spawn =====
            // (evita "caer al vacio por siempre" al cambiar de gravedad sin superficie)
            if (playerY < -30.0f || playerY > 400.0f ||
                playerX * playerX + playerZ * playerZ > 130.0f * 130.0f) {
                playerX = 0.0f; playerY = 0.0f; playerZ = 0.0f;
                velX = velY = velZ = 0.0f; gvr = gvf = gvg = 0.0f;
                gravG = 0; grounded = 1;
            }

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

            // ===== balas: mover, chocar con muros, derribar robots =====
            for (int s = 0; s < 24; ++s) {
                if (g_shots[s].life <= 0) continue;
                g_shots[s].x += g_shots[s].vx;
                g_shots[s].y += g_shots[s].vy;
                g_shots[s].z += g_shots[s].vz;
                if (--g_shots[s].life <= 0) continue;
                if (blocked(g_shots[s].x, g_shots[s].z, g_shots[s].y)) { g_shots[s].life = 0; continue; }
                for (int n = 0; n < kNpcCount; ++n) {
                    if (g_npcKilled[n]) continue;
                    float dx = g_shots[s].x - kNpcs[n].x, dz = g_shots[s].z - kNpcs[n].z;
                    float hh = 1.5f + 0.18f * (float)(n % 3);
                    if (dx*dx + dz*dz < 0.5f*0.5f && g_shots[s].y > 0.0f && g_shots[s].y < hh + 0.6f) {
                        g_npcKilled[n] = 1; buildNpcs();
                        for (int q = 0; q < 12; ++q) if (g_sparks[q].life <= 0) {
                            g_sparks[q].x = kNpcs[n].x; g_sparks[q].y = g_shots[s].y; g_sparks[q].z = kNpcs[n].z;
                            g_sparks[q].life = 16; break;
                        }
                        if (!g_shots[s].pierce) { g_shots[s].life = 0; break; }   // pierce sigue de largo
                    }
                }
            }
            for (int q = 0; q < 12; ++q) if (g_sparks[q].life > 0) g_sparks[q].life--;
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

#if VIEWER_MODE
        drawCandidates();
#else
        sceGumMatrixMode(GU_PROJECTION);
        sceGumLoadIdentity();
        sceGumPerspective(72.0f, 16.0f / 9.0f, 0.8f, 300.0f); // R2: far 420->300 recorta el anillo externo de agujas + mejora Z (la bruma tapa el corte; la catedral z=-170 sigue entrando)

        sceGumMatrixMode(GU_VIEW);
        sceGumLoadIdentity();
        {
#if FLYCAM
            ScePspFVector3 rot    = { DEG2RAD(48.0f), 0.0f, 0.0f };   // mira el PISO del mundo (adoquin)
            ScePspFVector3 camOff = { 0.0f, -20.0f, -10.0f };
            ScePspFVector3 pOff   = { 0.0f, -2.0f, -34.0f };
#else
            float gpit, gyaw, grol; gravCamEuler(gravG, DEG2RAD(8.0f), &gpit, &gyaw, &grol);
            ScePspFVector3 rot    = { gpit, gyaw + camYaw, grol };   // reorienta segun gravedad
            ScePspFVector3 camOff = { 0.0f, -4.8f, -11.5f };         // cerca pero deja ver el mar de agujas al frente
            ScePspFVector3 pOff   = { -playerX, -playerY, -playerZ };
#endif
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
        sceGuTexMode(GU_PSM_8888, 0, 0, GU_TRUE);   // GU_TRUE = texturas SWIZZLED (PSP real)
        sceGuTexImage(0, STEX, STEX, STEX, g_groundTexS);   // PISO: adoquin gotico (genGround, alto contraste: juntas oscuras + losas)
        sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGB);
        sceGuTexFilter(GU_NEAREST, GU_NEAREST);   // 1 texel/pixel: gran ahorro de fill en PSP real
        sceGuTexWrap(GU_REPEAT, GU_REPEAT);
        // --- MUNDO SOLIDO con CULLING por estructura + LOD (solo lo cercano/al frente) ---
        sceGuEnable(GU_CULL_FACE);   // R1: back-face culling en pases PROBADOS (piso, edificios, cola/catedral) -> ~1/2 del fill
        sceGumDrawArray(GU_TRIANGLES, TEX_FLAGS, g_floorCount, 0, g_solidWorld);   // piso: siempre (piedra)
        sceGuTexImage(0, STEX, STEX, STEX, g_facadeTexS);   // EDIFICIOS: textura de FACHADA gotica (ventanas en la imagen)
        for (int s = 0; s < g_srangeCount; ++s) {
            const StructRange &r = g_srange[s];
            float ddx = r.cx - playerX, ddz = r.cz - playerZ;
            if (ddz > BEHIND_CULL) continue;                    // detras de la camara (fija mira -Z)
            float d2 = ddx * ddx + ddz * ddz;
            if (d2 > DRAW_DIST * DRAW_DIST) continue;           // demasiado lejos (la niebla ya lo tapa)
            { float axl = ddx < 0 ? -ddx : ddx; if (ddz < -1.0f && d2 > 625.0f && axl > -ddz * 2.0f) continue; } // muy a los lados (fuera del campo de vision)
            if (d2 > LOD_DIST * LOD_DIST)
                sceGumDrawArray(GU_TRIANGLES, TEX_FLAGS, r.sDetail - r.sStart, 0, g_solidWorld + r.sStart); // LOD: cuerpo sin detalle
            else
                sceGumDrawArray(GU_TRIANGLES, TEX_FLAGS, r.sCount, 0, g_solidWorld + r.sStart);             // completo
        }
        sceGuTexImage(0, STEX, STEX, STEX, g_stoneTexS);   // vuelve a PIEDRA para agujas/cola
        sceGumDrawArray(GU_TRIANGLES, TEX_FLAGS, g_spireEnd - g_spireStart, 0, g_solidWorld + g_spireStart); // agujas (fondo)
        sceGumDrawArray(GU_TRIANGLES, TEX_FLAGS, g_solidVerts - g_tailStart, 0, g_solidWorld + g_tailStart);  // cola + cathedral

        // ventanas: textura de vidriera (CLAMP). Solo torres CERCANAS + agujas/cathedral (siempre).
        sceGuDisable(GU_CULL_FACE);  // ventanas (addWinRow) tienen winding INCONSISTENTE -> NO cullear; metal tambien queda OFF
        sceGuTexImage(0, WTEX, WTEX, WTEX, g_winTexS);
        sceGuTexWrap(GU_CLAMP, GU_CLAMP);
        for (int s = 0; s < g_srangeCount; ++s) {
            const StructRange &r = g_srange[s];
            if (r.wCount <= 0) continue;
            float ddx = r.cx - playerX, ddz = r.cz - playerZ;
            if (ddz > BEHIND_CULL) continue;
            float d2 = ddx * ddx + ddz * ddz;
            if (d2 > LOD_DIST * LOD_DIST) continue;             // ventanas solo de torres cercanas
            sceGumDrawArray(GU_TRIANGLES, TEX_FLAGS, r.wCount, 0, g_win + r.wStart);
        }
        sceGumDrawArray(GU_TRIANGLES, TEX_FLAGS, g_winVerts - g_winTailStart, 0, g_win + g_winTailStart); // agujas + cathedral

        // metal: baranda con textura de acero (REPEAT)
        sceGuTexImage(0, MTEX, MTEX, MTEX, g_metalTexS);
        sceGuTexWrap(GU_REPEAT, GU_REPEAT);
        sceGumDrawArray(GU_TRIANGLES, TEX_FLAGS, g_metalVerts, 0, g_metal);
        sceGuDisable(GU_TEXTURE_2D);

        // atmosfera de fondo: siluetas colosales lejanas + ruinas suspendidas del abismo
        sceGuEnable(GU_CULL_FACE);   // R1: agujas + props + npc son cajas/piramides probadas -> cullables
        sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g_spireVerts, 0, g_spire);     // MAR DENSO de agujas (el look de la referencia)
        sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g_vpropsVerts, 0, g_vprops);   // props del pueblo
        // cables + robots + ambiente (braseros) sin textura
        sceGumDrawArray(GU_LINES, LINE_FLAGS, g_chainVerts, 0, g_chains);
        sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g_npcVerts, 0, g_npc);
        sceGuDisable(GU_CULL_FACE);  // braseros (g_env) + HUNTER + balas: winding NO verificado -> culling OFF (seguro)
        sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g_envVerts, 0, g_env);

        // ---- HUNTER (idle sutil: leve balanceo) ; encara heroYaw ----
        {
            // camina: rebote SUAVE (no salto) + leve balanceo al avanzar
            float bobY = moving ? (sinf(walkPhase) * 0.045f) : (sinf(idleT) * 0.03f);
            sceGumLoadIdentity();
            ScePspFVector3 pp = { playerX, playerY + bobY, playerZ };
            sceGumTranslate(&pp);
            ScePspFVector3 gm = { 0.0f, 0.0f, 0.0f };   // reorienta el modelo (pies hacia gravDir)
            if (gravG == 1) gm.z = 3.14159f; else if (gravG == 2) gm.z = -1.5708f; else if (gravG == 3) gm.z = 1.5708f;
            else if (gravG == 4) gm.x = 1.5708f; else if (gravG == 5) gm.x = -1.5708f;
            sceGumRotateXYZ(&gm);
            float sway = moving ? (sinf(walkPhase * 0.5f) * 0.03f) : 0.0f;
            ScePspFVector3 fr = { (moving ? 0.045f : 0.0f), heroYaw + sinf(idleT * 0.6f) * 0.02f, sway };
            sceGumRotateXYZ(&fr);
            sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g_heroV, 0, g_hero);
        }

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

        // ---- balas (tracer brillante) ----
        for (int s = 0; s < 24; ++s) {
            if (g_shots[s].life <= 0) continue;
            LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 30);
            int vi = 0;
            float bsz = g_shots[s].size;
            addSolidBox(v, vi, g_shots[s].x, g_shots[s].y - bsz * 0.5f, g_shots[s].z,
                        bsz, bsz, bsz, g_shots[s].col);
            sceGumLoadIdentity();
            sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, vi, 0, v);
        }
        // ---- fogonazo del canon ----
        if (muzzle > 0) {
            float fx = sinf(heroYaw), fz = -cosf(heroYaw);
            LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 30);
            int vi = 0;
            addSolidBox(v, vi, playerX + fx * 0.7f, playerY + 1.5f - 0.12f, playerZ + fz * 0.7f,
                        0.24f, 0.24f, 0.24f, muzzleCol);
            sceGumLoadIdentity();
            sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, vi, 0, v);
        }
        // ---- chispas de impacto (robot derribado) ----
        for (int q = 0; q < 12; ++q) {
            if (g_sparks[q].life <= 0) continue;
            float t = (float)g_sparks[q].life / 16.0f;       // 1..0
            float sz = 0.25f + (1.0f - t) * 0.9f;            // crece al estallar
            int br = (int)(120 + 135 * t);
            unsigned int col = RGBA(255, br, 60 + (int)(80 * t), 255);
            LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 30);
            int vi = 0;
            addSolidBox(v, vi, g_sparks[q].x, g_sparks[q].y - sz * 0.5f, g_sparks[q].z, sz, sz, sz, col);
            sceGumLoadIdentity();
            sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, vi, 0, v);
        }

        // ---- arco de melee (Circulo): destello frontal al golpear ----
        if (meleeFx > 0) {
            float fx = sinf(heroYaw), fz = -cosf(heroYaw);
            float reach = kMelee[curMelee].reach * 0.20f;
            LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 30);
            int vi = 0;
            addSolidBox(v, vi, playerX + fx * reach * 0.6f, playerY + 1.0f, playerZ + fz * reach * 0.6f,
                        reach * 1.2f, 0.28f, reach * 1.2f, RGBA(205, 225, 255, 255));
            sceGumLoadIdentity();
            sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, vi, 0, v);
        }

        // ---------- HUD (2D) ----------
        sceGuDisable(GU_DEPTH_TEST);
        sceGuEnable(GU_BLEND);
        sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
        sceGuDisable(GU_TEXTURE_2D);
        drawVignette();   // bordes oscuros cinematograficos (sobre el 3D, bajo el HUD)

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

        // mira (crosshair) al apuntar con L
        if (aiming) {
            unsigned int rc = RGBA(255, 90, 80, 235);
            drawRect(239, 127, 2, 7, rc);   // arriba
            drawRect(239, 139, 2, 7, rc);   // abajo
            drawRect(231, 135, 7, 2, rc);   // izq
            drawRect(243, 135, 7, 2, rc);   // der
            drawRect(239, 135, 2, 2, RGBA(255, 255, 255, 255)); // centro
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
        drawText(barX + barW + 4, 35, 1.0f, RGBA(220, 220, 230, 255), kGravName[gravG]);   // direccion de gravedad

        snprintf(hud, sizeof(hud), "DISTRITO: Campanario    FPS %d", fps);
        drawText(8, 58, 1.0f, RGBA(150, 160, 190, 255), hud);

        drawText(304, 226, 1.0f, RGBA(170, 205, 165, 255), kMelee[curMelee].name);    // melee (Circulo)
        drawText(304, 239, 1.0f, RGBA(222, 210, 188, 255), kRanged[curRanged].name);  // arma a distancia (R)
        if (reloadCD > 0) {
            drawText(304, 252, 1.0f, RGBA(235, 180, 90, 255), "RECARGANDO...");
        } else {
            snprintf(hud, sizeof(hud), "%d / %d", ammoMag[curRanged], kRanged[curRanged].magazine);
            drawText(304, 252, 1.0f, aiming ? RGBA(255, 120, 110, 255) : RGBA(150, 175, 215, 255), hud);
        }

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
                 "X salto  Cuad planeo  L/R arma  Dpad cambia  O melee  Tri GRAVEDAD");
        if (paused) {
            drawText(206, 104, 2.0f, RGBA(232, 222, 242, 255), "PAUSA");
            drawText(163, 130, 1.0f, RGBA(165, 175, 205, 255), "START continuar   HOME salir");
        }

        sceGuDisable(GU_TEXTURE_2D);
        sceGuDisable(GU_BLEND);
        sceGuEnable(GU_DEPTH_TEST);
#endif

        sceGuFinish();
        sceGuSync(0, 0);
        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();
    }

    sceGuTerm();
    sceKernelExitGame();
    return 0;
}
