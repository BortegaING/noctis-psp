// PROJECT NOCTIS - Motor + mundo navegable + gravedad + HUD (vertical slice).
// Distrito "Campanario": torres goticas en wireframe, jugador movible con el
// stick, salto/gravedad vertical, camara 3a persona, HUD estilo mockup
// (barras HP/EN/GRV + arma). Render: GU_LINES (3D) y GU_SPRITES (2D).

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <psppower.h>
#include <string.h>
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
static const unsigned int CLEAR_COLOR = RGBA( 44,  39,  33, 255);   // cielo FRIO nocturno (megaestructura BLAME): azul profundo -> contrasta con el calido de las catedrales
static const unsigned int HAZE = RGBA(122, 106,  86, 255); // bruma FRIA azul-gris: la geometria lejana se disuelve en frio -> el ambar/piedra calida saltan al frente

// Sentido de "cara frontal" para el back-face culling por-pase (R1 rendimiento).
// El winding de cajas/piramides/piso es CONSISTENTE (probado), asi que exactamente
// UNO de GU_CW/GU_CCW es el correcto para TODOS los pases a la vez. Arranca en CW.
// >>> Si al capturar se ven las paredes/piso POR DENTRO (see-through), cambia esta
//     UNICA linea a GU_CCW y recompila: queda resuelto para todo. <<<
#define NOCTIS_FRONTFACE GU_CCW
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
    const float a = 20.0f, b = 140.0f;  // niebla calibrada al SECTOR chico (antes 50..330, era del mundo grande)
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
static TexVertex __attribute__((aligned(16))) g_apron[3600];       // (legacy plaza) piso cercano fino; sin uso en EL POZO
static int g_apronCount = 0;
// ===== EL POZO =====
static int g_ledgeStart = 0, g_ledgeEnd = 0;   // balcon del jugador en g_solidWorld (textura industrial)
static int g_wallStart  = 0, g_wallEnd  = 0;   // muros colosales del pozo en g_solidWorld (textura industrial)
static LineVertex __attribute__((aligned(16))) g_bridges[3000];    // puentes/megavigas cruzando el pozo
static int g_bridgesVerts = 0;
static LineVertex __attribute__((aligned(16))) g_voidShaft[2800];  // vacio del pozo: estructuras que se pierden abajo/arriba
static int g_voidShaftVerts = 0;
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
    float rad;                       // radio del footprint (cull direccional conservador, 1ra persona)
};
static StructRange g_srange[256];
static int g_srangeCount = 0;
static int g_floorCount   = 0;   // piso: [0, g_floorCount)
static int g_spireStart   = 0, g_spireEnd = 0;   // agujas: siempre
static int g_tailStart    = 0;   // cola (mirador..cathedral): [g_tailStart, g_solidVerts)
static int g_winTailStart = 0;   // ventanas de agujas+cathedral: siempre
// distancias (unidades de mundo). La niebla ya funde mas alla de ~98.
static const float DRAW_DIST = 70.0f;   // los 5 muros ENCIERRAN al jugador (centros hasta ~58) -> no cullear la sala por distancia
static const float LOD_DIST  = 42.0f;   // plaza r46, landmarks ~60: al centro las 4 = solo nucleo (barato); al cruzar hacia una aparece su ornamento fino
static const float BEHIND_CULL = 24.0f; // (sin uso: reemplazado por el cull por vector de camara en 1ra persona)

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
#include "gargoyles.h"   // enemigos: 3 siluetas + maquina de estados con territorio
#include "savedata.h"    // 5 ranuras con checksum, escritura segura y recuperacion
#include "audio.h"       // atmosfera sonora procedural (viento, pisadas, campana, eco)
#include "objective.h"   // BUCLE DE JUEGO: anclas, materiales y faro (ruta de escalada)
#include "anim.h"        // ciclo de caminata procedural del hunter
#include "weapons_geo.h" // buildMeleeWeapon(): las 10 armas melee con forma propia (lista para cablear al cambio de arma)
#include "viewmodel.h"    // arma en 1ra persona (pistola de chispa) + spec de movimiento
#include "gravity.h"      // 6 direcciones de gravedad (mecanica firma, directiva 22)
#include "village_props.h" // props del pueblo (faroles calidos, rejas, tumbas) - BLAME!/Bloodborne
#include "spirescape.h"    // MAR DENSO de agujas goticas en bruma (el look de la referencia)
#include "sky.h"           // drawSky(): cielo dramatico (luna + gradiente + nubes + silueta del horizonte)
#include "bridges.h"       // EL POZO: buildBridges() megavigas/puentes cruzando el pozo a distintas alturas
#include "void_shaft.h"    // EL POZO: buildVoidShaft() abismo sin fondo (balcones/tuberias/luces que se pierden)
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
    float t = (y - 34.0f) / 230.0f; if (t < 0.0f) t = 0.0f; if (t > 0.94f) t = 0.94f;   // puntas de aguja se disuelven casi del todo en el horizonte (sin pop)
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
    const unsigned int win   = RGBA(196, 120, 46, 255); // ambar mas fuerte (pocas vidrieras encendidas = acento calido en la sombra)
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
// Fase 2 de gravity.h: colision y suelo GENERALIZADOS a las 6 gravedades. Va aqui
// porque necesita g_city/g_cityCount, declarados recien arriba.
#define NOCTIS_GRAVITY_WORLD
#include "gravity.h"
#include "city.h"          // buildCity() llena g_city (16 edificios espaciados, plaza al centro)
#include "gothic_bldg.h"   // buildGothicBldg(): edificio gotico (legacy, sin uso)
#include "cathedral.h"     // buildCathedral(): catedral Yharnam ornamentada (legacy)
#include "cathedral_grand.h"     // buildCathedralGrand():    landmark 0 (norte) - west-front + torres gemelas
#include "cathedral_twin.h"      // buildCathedralTwin():     landmark 1 (este)  - agujas gemelas caladas
#include "cathedral_basilica.h"  // buildCathedralBasilica(): landmark 2 (oeste) - nave larga + arbotantes
#include "cathedral_bell.h"      // buildCathedralBell():     landmark 3 (sur)   - campanario/reloj
#include "shaft_walls.h"         // EL POZO: buildShaftWalls() muros colosales industrial-goticos (octagono r~160, y -700..+500)
#include "ledge.h"               // EL POZO: buildLedge() balcon del jugador (top teselado fino + baranda + soportes)
#include "sector.h"              // SECTOR CERRADO de pasillos (planta de cruz gotica): el mundo actual
#include "vault.h"               // boveda gotica del techo (nervios, mensulas, clave)
#include "tracery.h"             // arcos ojivales, parteluz y oculo de los ventanales
#include "arch_detail.h"         // arcadas ciegas, contrafuertes, portales, hornacinas, balaustrada

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

// PISO cercano fino (celdas 2.5u): tapa el hueco que dejan las celdas grandes del piso
// bajo la camara (la celda que cruza el ojo se recorta -> hueco visible con celdas de 25u).
// Se hornea centrado en origen; en el render se TRASLADA al jugador (snap a 12u para que la
// textura calce con el piso grueso). uv = local/12.
static void buildApron() {
    int i = 0;
    const float HALF = 20.0f;                       // ±20: al borde de la plataforma (r72) el apron no se sale al vacio
    const int   N    = 14;                          // 14x14 celdas de 2.86u (3ra persona: el ojo va a 4.5, el primer piso visible a ~4.5u -> sigue sin cruzar la camara) (<2.6u -> el hueco cae fuera de pantalla)
    const float CELL = (2.0f * HALF) / (float)N;    // 2.5u
    const float invUV = 1.0f / 10.0f;                     // MISMA escala que buildSectorFloorCeil
    // MISMO color que el suelo de sector.h. Antes era (240,230,214) con la excusa de
    // que "REPLACE ignora el color", pero desde el reorden del pase el apron se dibuja
    // con MODULATE igual que el suelo: el color SI se aplicaba, y se veia un parche
    // cuadrado mas claro de 40x40 siguiendo al jugador, con borde duro y saltos de 12.
    const unsigned int fcol = RGBA(212, 194, 172, 255);
    for (int gz = 0; gz < N; ++gz) {
        float z0 = -HALF + CELL * (float)gz, z1 = z0 + CELL;
        float v0 = z0 * invUV, v1 = z1 * invUV;
        for (int gx = 0; gx < N; ++gx) {
            float x0 = -HALF + CELL * (float)gx, x1 = x0 + CELL;
            float u0 = x0 * invUV, u1 = x1 * invUV;
            addQuadT(g_apron, i, x0,0.0f,z0, x1,0.0f,z0, x1,0.0f,z1, x0,0.0f,z1, u0,v0, u1,v1, fcol);
        }
    }
    g_apronCount = i;
}

static void buildSolidWorld() {
    int i = 0;
    g_winVerts = 0;
    // ===== SECTOR CERRADO DE PASILLOS (ver sector.h) =====
    // Recinto chico con suelo, TECHO y muros; 4 masas separadas forman pasillos en cruz.
    // Las paredes OCLUYEN -> en un pasillo solo se pinta ese pasillo (el fill es el cuello).
    buildSectorCollision();             // llena g_city con los volumenes solidos (colision)
    // lo que sobresale al pasillo TAMBIEN frena al jugador y a la camara
    g_cityCount += archCollisionBoxes(g_city + g_cityCount, 256 - g_cityCount);
    g_floorCount = 0;
    g_ledgeStart = i;
    buildSectorFloorCeil(g_solidWorld, i);   // suelo + techo (textura de piedra, sin cull)
    g_ledgeEnd = i;
    g_wallStart = i;
    buildSectorWalls(g_solidWorld, i);       // masas + muros + arcada gotica (industrial, cull ON)
    buildVault(g_solidWorld, i);             // boveda del techo (mismo winding: va con cull ON)
    buildTracery(g_solidWorld, i);           // marco gotico de los 24 ventanales
    buildArchDetail(g_solidWorld, i);        // relieve gotico sobre las caras de las masas
    // El guardarrail va AQUI, no al final. Antes se clampeaba i despues de fijar
    // g_wallEnd: en caso de desborde g_solidVerts quedaba en 36000 pero g_wallEnd
    // conservaba el valor grande, y el draw de los muros leia fuera del array.
    if (i > 36000) i = 36000;     // red de seguridad: g_solidWorld[36000]
    g_wallEnd = i;
    g_srangeCount = 0;
    g_winTailStart = g_winVerts;
    g_spireStart = i; g_spireEnd = i;
    g_tailStart = i;
    g_metalVerts = 0;
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
    if (px * px + pz * pz > 200.0f * 200.0f) return true; // (era 82 = radio de la PLAZA vieja; el balcon esta en r~118 -> bloqueaba TODO movimiento)
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
static LineVertex __attribute__((aligned(16))) g_vm[640];   // viewmodel del arma (1ra persona)
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
#include "robots.h"   // buildRobots(): NPCs roboticos variados (mensajero/mecanico/guardian/dron/...)
#include "dialogue.h" // 84 lineas repartidas en 6 robots, cada uno con su caracter y su pedazo de historia
static LineVertex __attribute__((aligned(16))) g_npc[2400];
static int g_npcVerts = 0;
static int g_npcKilled[64] = {0};   // (robots = habitantes, no enemigos; el kill-system queda inerte)

// ===== ARMA / DISPAROS (L apunta, R dispara) =====
struct Shot { float x, y, z, vx, vy, vz; int life; unsigned int col; float size; int pierce; };
static Shot g_shots[24];
// chispa/estallido al derribar un robot (feedback de impacto)
struct Spark { float x, y, z; int life; };
static Spark g_sparks[12];

// NPCs roboticos variados y con alma (robots.h): mensajero, mecanico, guardian,
// dron, anciano, walker... con distinto estado de conservacion y ojo luminoso.
static void buildNpcs() {
    g_npcVerts = buildRobots(g_npc);
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
#include "gothic_tex.h"    // genGothicFacade: fachada gotica antigua (queda para referencia)
#include "facade_tex.h"    // genFacade: fachada gotica grande (catedrales)
#include "industrial_tex.h" // genIndustrial: muro industrial-gotico FRIO (EL POZO: balcon + muros)
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
// ---- RGBA8888 -> RGB565: la mitad de bytes leidos por pixel. En un escenario
// limitado por FILL (el cuello de la PSP) esto es de lo que mas rinde. El swizzle
// se hace DESPUES, sobre los datos ya de 16 bits (ancho en bytes = lado * 2).
static unsigned short __attribute__((aligned(16))) g_tmp16[128 * 128];
static void to5650(unsigned short *dst, const unsigned int *src, int n) {
    for (int k = 0; k < n; ++k) {
        unsigned int c = src[k];
        int r = c & 0xFF, g = (c >> 8) & 0xFF, b = (c >> 16) & 0xFF;
        dst[k] = (unsigned short)((r >> 3) | ((g >> 2) << 5) | ((b >> 3) << 11));
    }
}
#define STEX 128
// CUATRO TEXTURAS BORRADAS: ventana, metal, piedra y fachada. Se generaban, se
// convertian a 5650 y se swizzleaban en el arranque, y no las ataba NADIE: los
// unicos sceGuTexImage del programa son el atlas de fuente, g_indTexS y
// g_groundTexS. Eran 227 KB de .bss permanentes (sobrevivian incluso a -O2, porque
// SI se escribian: solo no se leian nunca) mas cuatro pasadas de ruido procedural
// en cada arranque. Los generadores siguen en agent_textures.h y facade_tex.h por
// si vuelven a hacer falta; lo que se va es la copia residente en memoria.
// EL POZO: textura INDUSTRIAL-gotica fria (paneles/tuberias/remaches) para balcon + muros del pozo
static unsigned int __attribute__((aligned(16))) g_indTex[STEX * STEX];
static unsigned short __attribute__((aligned(16))) g_indTexS[STEX * STEX];
static void buildIndTex() { genIndustrial(g_indTex, STEX); to5650(g_tmp16, g_indTex, STEX * STEX); swizzleTex((unsigned char*)g_indTexS, (const unsigned char*)g_tmp16, STEX * 2, STEX); sceKernelDcacheWritebackAll(); }

// adoquin/losas para el PISO de todo el mundo
static unsigned int __attribute__((aligned(16))) g_groundTex[STEX * STEX];
static unsigned short __attribute__((aligned(16))) g_groundTexS[STEX * STEX];
static void buildGroundTex() { genGround(g_groundTex, STEX); to5650(g_tmp16, g_groundTex, STEX * STEX); swizzleTex((unsigned char*)g_groundTexS, (const unsigned char*)g_tmp16, STEX * 2, STEX); sceKernelDcacheWritebackAll(); }

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
    const unsigned int e  = RGBA(0, 0, 0, 105);     // bordes mas suaves
    const unsigned int t  = RGBA(0, 0, 0, 0);
    gradQuad(0, 56, e, t);                           // arriba
    gradQuadV(0, 60, e, t);                          // izquierda
    gradQuadV(SCR_WIDTH - 60, SCR_WIDTH, t, e);      // derecha
    // SIN banda inferior: la vieja (alpha 200) aplastaba el PISO a ~0.22 -> se veia como VACIO.
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

#include "hud.h"         // HUD nuevo: minimapa del sector REAL (usa drawRect/drawText/fontTexOn)

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
    // 16 BITS: el fill es ancho de banda. A 8888 cada pixel escribe 4 bytes (y lee
    // otros 4 si hay mezcla); a 5650 escribe 2. Mismo truco que dio resultado en las
    // texturas. Ademas libera ~560 KB de VRAM.
    g_fbp0 = guGetStaticVramBuffer(BUF_WIDTH, SCR_HEIGHT, GU_PSM_5650);
    g_fbp1 = guGetStaticVramBuffer(BUF_WIDTH, SCR_HEIGHT, GU_PSM_5650);
    g_zbp  = guGetStaticVramBuffer(BUF_WIDTH, SCR_HEIGHT, GU_PSM_4444);

    sceGuInit();
    sceGuStart(GU_DIRECT, g_list);
    sceGuDrawBuffer(GU_PSM_5650, g_fbp0, BUF_WIDTH);
    sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, g_fbp1, BUF_WIDTH);
    sceGuDepthBuffer(g_zbp, BUF_WIDTH);
    sceGuOffset(2048 - (SCR_WIDTH / 2), 2048 - (SCR_HEIGHT / 2));
    sceGuViewport(2048, 2048, SCR_WIDTH, SCR_HEIGHT);
    sceGuDepthRange(65535, 0);
    sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuDepthFunc(GU_GEQUAL);
    // RECORTE POR PLANO CERCANO. Sin esto el GE no recorta: DESCARTA el triangulo
    // ENTERO en cuanto un vertice queda detras de la camara. Eso es lo que hacia
    // desaparecer paredes y suelo al caminar: las caras de las masas miden 36 y los
    // muros 116 unidades, y es la razon por la que medio motor esta teselado a
    // trozos de 6. Con esto encendido el hardware recorta de verdad.
    sceGuEnable(GU_CLIP_PLANES);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuFrontFace(NOCTIS_FRONTFACE); // cara exterior = NOCTIS_FRONTFACE (winding probado consistente)
    sceGuDisable(GU_CULL_FACE);  // default OFF; el culling se ACTIVA por-pase en el lazo de render
                                 // (paredes/piso/agujas SI; ventanas/hero/braseros NO -> winding no probado).
    sceGuDisable(GU_TEXTURE_2D);
    sceGuShadeModel(GU_SMOOTH);
    {   // dither: disimula el bandeado de los degradados a 16 bits. Coste cero.
        static const ScePspIMatrix4 kDither = { {-4,0,-3,1}, {2,-2,3,-1}, {-3,1,-4,0}, {3,-1,2,-2} };
        sceGuSetDither((ScePspIMatrix4*)&kDither);
        sceGuEnable(GU_DITHER);
    }
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
}

int main(void) {
    setupCallbacks();
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    buildGroundTex();
    // buildCity() borrado: llenaba g_city con 8 monolitos y les ponia g_cityCount = 8,
    // y dos lineas mas abajo buildSolidWorld() llama a buildSectorCollision(), que
    // arranca en n = 0 y lo sobreescribe entero. Nunca existieron.
    buildIndTex();  // textura industrial del pozo (antes de renderizar)
    buildSolidWorld();
    buildApron();   // (legacy; no se dibuja en EL POZO)
    buildHero();
#if VIEWER_MODE
    buildCandidates();
#endif
    buildNpcs();
    gargInit();
    dlgInit();      // los 6 robots dejan de ser estatuas mudas
    // buildChains();   // cadenas QUITADAS (cosas entremedio que bajan FPS); g_chainVerts=0
    buildEnv();
    // buildFarSilhouettes() y buildVoidLayer() BORRADAS: generaban 3.228 vertices de
    // siluetas lejanas y capa de vacio, y seis lineas mas abajo se ponia a cero su
    // contador, asi que nunca llegaban a un draw. Se pagaba la generacion entera para
    // tirarla. Son del mundo de la PLAZA vieja: sus agujas y ruinas quedarian flotando
    // dentro del sector cerrado de hoy.
    g_bridgesVerts   = 0;   // puentes y pozo del mundo viejo: apagados por fill
    g_voidShaftVerts = 0;
    g_voidVerts = 0; g_farSilVerts = 0; g_chainVerts = 0;   // capas del mundo viejo (sin uso)
    g_vpropsVerts = buildVillageProps(g_vprops);
    g_spireVerts  = buildSpirescape(g_spire);
    buildFontAtlas();
    scePowerSetClockFrequency(333, 333, 166);   // MAXIMO de la PSP (por defecto corre a 222/111): +50% CPU y bus
    initGu();
    audioInit();          // si el canal de audio falla, todo queda en no-op y el juego sigue

    SceCtrlData pad; memset(&pad, 0, sizeof(pad));   // sin basura en el 1er frame (registraba un Triangulo fantasma -> gravedad arrancaba en -Z)
    float playerX = 0.0f, playerY = 0.0f, playerZ = 0.0f;   // BLAME: spawn al centro, monolitos lejos
    float velY = 0.0f, velX = 0.0f, velZ = 0.0f;
    int   coyote = 0, prevJump = 0, jumpBuf = 0;
    float en = 780.0f;
    const float HP_MAX = 100.0f;
    float hp = HP_MAX;      // VIDA REAL: antes la barra era decorativa y el numero estaba escrito a mano
    int   hurtFlash = 0;
    // DIALOGO: hablan solos al acercarte. Todos los botones estaban ya ocupados, y que
    // te hablen al pasar encaja mejor con el sitio que pedir un boton mas.
    int   dlgPrev = -1, dlgTimer = 0;
    const char *dlgLine = 0, *dlgWho = 0;
    float EN_MAX = 780.0f;   // sube con cada material recogido (objEnergyMax)
    // constantes de movilidad (diseno del agente)
    const float DEADZONE = 0.18f, RUN_SPEED = 0.22f, ACCEL_GND = 0.20f, ACCEL_AIR = 0.09f, STOP_FRIC = 0.22f, CAM_SPEED = 0.03f;
    const float GRAVITY = 0.020f, JUMP_VEL = 0.55f, SHORTHOP = 0.50f, FALL_CAP = 0.80f;
    const int   COYOTE_MAX = 6, JUMPBUF_MAX = 6;
    const float FLOAT_LIFT = 0.030f, FLOAT_GRAV = 0.006f, FLOAT_UPCAP = 0.12f, FLOAT_FALLCAP = -0.09f, EN_FLOAT = 6.0f, EN_REGEN = 5.0f;
    int   grounded = 1;
    float camYaw = 0.7f;    // mirada inicial: a un hueco NE (borde de plataforma + vacio + megaestructura + catedrales de reojo)
    float heroYaw = 0.0f;   // hacia donde encara el modelo (gira al avanzar)
    int   paused = 0, prevStart = 0;
    float walkPhase = 0.0f, idleT = 0.0f;   // animacion del personaje
    float speed01 = 0.0f;                   // velocidad normalizada 0..1 para la pose de caminata
    int   moving = 0;
    // ===== PRIMERA PERSONA: mirada + head-bob + sway del arma (constantes del agente de jugabilidad) =====
    float camPitch = 0.0f;                          // mirar arriba/abajo
    float yawRate = 0.0f, pitchRate = 0.0f;         // suavizado de la mirada (alimenta el sway del arma)
    float bobPhase = 0.0f, bobX = 0.0f, bobY = 0.0f;// head-bob segun velocidad
    float vmSway = 0.0f, vmBob = 0.0f;              // offsets suavizados del arma en pantalla
    const float TURN_MAX = 0.045f, LOOK_SMOOTH = 0.25f, PITCH_SPD = 0.030f, PITCH_CLAMP = 1.30f;
    const float FP_SPEED = 0.11f, FP_ACCEL = 0.16f, FP_STOP = 0.20f, EYE_H = 1.7f, COURT = 54.0f, STRAFE_SIGN = 1.0f;

    int fps = 0, frameAccum = 0;
    long long lastTick = sceKernelGetSystemTimeWide();
    char hud[80];   // ahora SI se usa: el texto del reparto del frame (mantener SELECT)
    // Reparto del frame en milisegundos, suavizado. Mantener SELECT para verlo.
    float msCpu = 0.0f, msGe = 0.0f, msWait = 0.0f;
    long long tFrame = sceKernelGetSystemTimeWide();

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
    // ===== PARTIDA GUARDADA: volcar el estado y continuar donde quedaste =====
    NoctisSave sv; saveClear(&sv);
    // tiempo jugado: el reloj desde que arranco esta sesion, mas lo que trajera la
    // partida que se cargo. El campo existia en el struct y nadie lo escribia nunca.
    const long long bootTick = sceKernelGetSystemTimeWide();
    uint32_t playBase = 0;
    auto snap = [&]() {
        sv.px = playerX; sv.py = playerY; sv.pz = playerZ;
        sv.camYaw = camYaw; sv.camPitch = camPitch; sv.gravG = gravG;
        sv.hp = hp; sv.hpMax = HP_MAX;   // la VIDA: el campo estaba y solo guardaba el 100 de fabrica
        sv.en = en; sv.enMax = EN_MAX;
        sv.objActive = objActiveAnchor(); sv.objAnchorSeen = objAnchorSeenMask();
        sv.objMatTaken = objMatTakenMask(); sv.objGoal = objGoalDone();
        sv.curRanged = curRanged; sv.curMelee = curMelee;
        for (int k = 0; k < 16 && k < kRangedCount; ++k) sv.ammoMag[k] = ammoMag[k];
        // INVENTARIO: las gemas recogidas caben en dos mascaras de 32 bits. Sin esto
        // reaparecian todas en cada arranque.
        sv.invResLo = 0; sv.invResHi = 0; sv.invCount = collectedCount;
        for (int r = 0; r < 64 && r < kResourceCount; ++r)
            if (collected[r]) { if (r < 32) sv.invResLo |= (1u << r); else sv.invResHi |= (1u << (r - 32)); }
        // CABECERA. Sin ella saveCounter valia 0 en las cinco ranuras, y saveLatestSlot
        // compara con >=: devolvia siempre la de indice mas alto, o sea la de
        // RECUPERACION, que es justo la partida MAS VIEJA.
        const long long nowUs = sceKernelGetSystemTimeWide();
        sv.playTimeSec = playBase + (uint32_t)((nowUs - bootTick) / 1000000LL);
        sv.stampSec    = (uint32_t)(nowUs / 1000000LL);
        sv.saveCounter++;
    };
    {   // carga: autoguardado y, si esta corrupto, cae solo a la ranura de recuperacion
        NoctisSave ld; saveClear(&ld);
        if (saveLoadAutoOrRecovery(&ld)) {
            // arrastra la cabecera y CUALQUIER campo que hoy no leemos: sin esto el
            // siguiente guardado los devolvia a los valores de fabrica.
            sv = ld;
            playBase = ld.playTimeSec;
            playerX = ld.px; playerY = ld.py; playerZ = ld.pz;
            camYaw = ld.camYaw; camPitch = ld.camPitch;
            // El checksum protege contra bits corruptos, no contra un archivo escrito
            // por otra build. Todo lo que luego indexa un array se sanea aqui.
            gravG = (ld.gravG < 0 || ld.gravG > 5) ? 0 : ld.gravG;          // kGravName[6]
            hp = (ld.hp > 0.0f && ld.hp <= ld.hpMax) ? ld.hp : HP_MAX;
            en = ld.en; EN_MAX = (ld.enMax > 0.0f) ? ld.enMax : objEnergyMax();
            if (en < 0.0f)    en = 0.0f;
            if (en > EN_MAX)  en = EN_MAX;
            objRestore(ld.objActive, ld.objAnchorSeen, ld.objMatTaken, ld.objGoal);
            curRanged = (ld.curRanged < 0 || ld.curRanged >= kRangedCount) ? 0 : ld.curRanged;
            curMelee  = (ld.curMelee  < 0 || ld.curMelee  >= kMeleeCount)  ? 0 : ld.curMelee;
            for (int k = 0; k < 16 && k < kRangedCount; ++k) ammoMag[k] = ld.ammoMag[k];
            collectedCount = 0;
            for (int r = 0; r < 64 && r < kResourceCount; ++r) {
                collected[r] = (r < 32) ? (int)((ld.invResLo >> r) & 1u)
                                        : (int)((ld.invResHi >> (r - 32)) & 1u);
                if (collected[r]) collectedCount++;
            }
        }
    }
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
            if (gravG == 0) {   // ===== PRIMERA PERSONA: nub gira+camina; D-pad strafe(izq/der)+mirar(arr/aba) =====
            // -- girar (yaw) con nub X, suavizado (curva cuadratica: preciso al centro) --
            float turnIn = 0.0f;
            if (ax > DEADZONE || ax < -DEADZONE) {
                float k = (ax < 0.0f ? -ax : ax); k = (k - DEADZONE) / (1.0f - DEADZONE); if (k > 1.0f) k = 1.0f;
                turnIn = (ax < 0.0f ? -(k * k) : (k * k));
            }
            yawRate += (turnIn * TURN_MAX - yawRate) * LOOK_SMOOTH;
            camYaw  += yawRate;
            // -- mirar arriba/abajo (pitch) con D-pad; vuelve al centro al soltar; clamp --
            int lookU = (pad.Buttons & PSP_CTRL_UP) ? 1 : 0, lookD = (pad.Buttons & PSP_CTRL_DOWN) ? 1 : 0;
            pitchRate += (((lookU ? PITCH_SPD : 0.0f) - (lookD ? PITCH_SPD : 0.0f)) - pitchRate) * LOOK_SMOOTH;
            camPitch  += pitchRate;
            if (!lookU && !lookD) camPitch += (-0.12f - camPitch) * 0.10f;   // reposo levemente HACIA ABAJO: se ve el borde del balcon y el abismo (vertigo)
            if (camPitch >  PITCH_CLAMP) camPitch =  PITCH_CLAMP;
            if (camPitch < -PITCH_CLAMP) camPitch = -PITCH_CLAMP;
            // -- avanzar/retroceder (nub Y) + strafe (D-pad izq/der), relativo a la MIRADA --
            float fwdIn = 0.0f;
            if (ay > DEADZONE || ay < -DEADZONE) {
                float k = (ay < 0.0f ? -ay : ay); k = (k - DEADZONE) / (1.0f - DEADZONE); if (k > 1.0f) k = 1.0f;
                fwdIn = (ay < 0.0f ? k : -k);   // nub arriba (ay<0) = adelante
            }
            float strafeIn = ((pad.Buttons & PSP_CTRL_RIGHT) ? 1.0f : 0.0f) - ((pad.Buttons & PSP_CTRL_LEFT) ? 1.0f : 0.0f);
            float fX = sinf(camYaw), fZ = -cosf(camYaw);     // adelante (horizontal)
            float rX = cosf(camYaw), rZ = sinf(camYaw);      // derecha
            float wvx = (fX * fwdIn + rX * strafeIn * STRAFE_SIGN) * FP_SPEED;
            float wvz = (fZ * fwdIn + rZ * strafeIn * STRAFE_SIGN) * FP_SPEED;
            velX += (wvx - velX) * FP_ACCEL;
            velZ += (wvz - velZ) * FP_ACCEL;
            if (fwdIn == 0.0f && strafeIn == 0.0f && grounded) { velX -= velX * FP_STOP; velZ -= velZ * FP_STOP; }
            // -- colision por ejes separados (desliza por muros) --
            float nx = playerX + velX; if (!blocked(nx, playerZ, playerY)) playerX = nx; else velX = 0.0f;
            float nz = playerZ + velZ; if (!blocked(playerX, nz, playerY)) playerZ = nz; else velZ = 0.0f;
            // RED DE SEGURIDAD, y ahora CUADRADA como la sala. Era circular (r=COURT) dentro
            // de un recinto CUADRADO de medio lado COURT: el circulo cortaba las cuatro
            // esquinas del pasillo perimetral con un muro invisible curvo, y al ser un
            // empujon incondicional aplicado DESPUES de blocked() metia al jugador dentro
            // de la huella inflada de las masas. Los 4 muros ya frenan en blocked(); esto
            // solo atrapa el caso de escaparse por un hueco.
            if (playerX >  COURT) playerX =  COURT;
            if (playerX < -COURT) playerX = -COURT;
            if (playerZ >  COURT) playerZ =  COURT;
            if (playerZ < -COURT) playerZ = -COURT;
            heroYaw = camYaw;   // disparo/melee usan heroYaw = hacia donde miras
            // -- head-bob por velocidad --
            float spd = sqrtf(velX * velX + velZ * velZ);
            if (spd > 0.008f) bobPhase += 0.30f + 0.9f * (spd / FP_SPEED);
            float bt = spd / FP_SPEED; if (bt > 1.0f) bt = 1.0f;
            bobY = sinf(bobPhase * 2.0f) * 0.028f * bt;
            bobX = sinf(bobPhase)        * 0.018f * bt;
            // -- sway/bob del arma (suavizado: el arma sigue con retraso la mirada y se asienta al parar) --
            float swayT = -yawRate * 1.4f - strafeIn * 0.9f + bobX * 0.6f;
            float bobT  =  bobY - pitchRate * 0.20f;
            vmSway += (swayT - vmSway) * 0.15f;
            vmBob  += (bobT  - vmBob ) * 0.18f;
            }   // fin PRIMERA PERSONA (gravG==0)
            // ===== ARMAS: L apunta, R dispara; D-pad cambia; Circulo melee =====
            // cambio de arma a distancia (D-pad izq/der) y melee (D-pad arr/aba)
            int dR = (pad.Buttons & PSP_CTRL_RIGHT) ? 1 : 0, dL = (pad.Buttons & PSP_CTRL_LEFT) ? 1 : 0;
            int dU = (pad.Buttons & PSP_CTRL_UP) ? 1 : 0,    dD = (pad.Buttons & PSP_CTRL_DOWN) ? 1 : 0;
            // D-pad reservado para strafe (izq/der) + mirar (arr/aba) en 1ra persona;
            // el cambio de arma se movera a un modificador (p.ej. mantener L) mas adelante.
            (void)dR; (void)dL; (void)dU; (void)dD;
            prevDR = dR; prevDL = dL; prevDU = dU; prevDD = dD;
            // Triangulo cicla la DIRECCION de gravedad (0=abajo .. 5=-Z)
            // GRAVEDAD (Triangulo) DESHABILITADA en 1ra persona por ahora: se reintroduce
            // con camara gravedad-consciente en una tanda dedicada. gravG queda en 0.
            { int tri = (pad.Buttons & PSP_CTRL_TRIANGLE) ? 1 : 0;
              if (tri && !prevTri && en >= GRAV_EN_MIN) {      // GRAVEDAD DIRIGIDA: a la cara que miras
                  const float cp2 = cosf(camPitch), sp2 = sinf(camPitch);
                  const int tg = gravPickTarget(playerX, playerY + EYE_H, playerZ,
                                                sinf(camYaw) * cp2, sp2, -cosf(camYaw) * cp2);
                  if (tg >= 0 && tg != gravG) {
                      gravG = tg; en -= GRAV_EN_SWITCH;        // cuesta energia: obliga a planear la ruta
                      dlgNotifyGravity();                     // los robots cercanos lo comentan
                      velX = velY = velZ = 0.0f; gvr = gvf = gvg = 0.0f; grounded = 0;
                  }
              }
              prevTri = tri; }   // GRAVEDAD: Triangulo cicla 6 direcciones

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
                {   // barrido de sable delante del jugador
                    const float mfx = sinf(heroYaw), mfz = -cosf(heroYaw);
                    gargDamage(playerX + mfx * reach * 0.6f, playerY + 1.0f, playerZ + mfz * reach * 0.6f,
                               reach * 0.8f, kMelee[curMelee].damage);
                }
                // (los robots ya NO son blancos: son los habitantes del sector)
            }
            prevCircle = (pad.Buttons & PSP_CTRL_CIRCLE) ? 1 : 0;   // flanco: un golpe por pulsacion, no uno por frame
            if (gravG == 0) {
            // ===== salto: coyote time + buffer + salto variable =====
            if (grounded) coyote = COYOTE_MAX; else if (coyote > 0) coyote--;
            int jumpNow = (pad.Buttons & PSP_CTRL_CROSS) ? 1 : 0;
            if (jumpNow && !prevJump) jumpBuf = JUMPBUF_MAX; else if (jumpBuf > 0) jumpBuf--;
            if (jumpBuf > 0 && coyote > 0) { velY = JUMP_VEL; grounded = 0; coyote = 0; jumpBuf = 0; }
            if (!jumpNow && velY > 0.0f) velY *= SHORTHOP;
            prevJump = jumpNow;
            // ===== gravedad / PLANEO (CUADRADO gasta EN; L apunta) =====
            if ((pad.Buttons & PSP_CTRL_SQUARE) && en > 0.0f) {
                velY += FLOAT_LIFT; if (velY > FLOAT_UPCAP)   velY = FLOAT_UPCAP;
                velY -= FLOAT_GRAV; if (velY < FLOAT_FALLCAP) velY = FLOAT_FALLCAP;
                en -= EN_FLOAT; grounded = 0;
            } else {
                velY -= GRAVITY;
            }
            // VELOCIDAD TERMINAL. Sin esto, cayendo desde el techo (60) se llega a ~1.55
            // por frame, y la colision se prueba UNA vez por frame: el jugador atraviesa
            // cualquier repisa mas fina que su paso. El pretil mas fino del mundo mide
            // 1.45, asi que el tope va por debajo de eso.
            if (velY < -FALL_CAP) velY = -FALL_CAP;
            // ===== integra vertical + suelo/azoteas =====
            playerY += velY;
            float gh = groundHeight(playerX, playerZ, playerY);
            if (playerY <= gh) { playerY = gh; velY = 0.0f; grounded = 1; } else grounded = 0;
            // ===== energia (EN) =====
            if (grounded && en < EN_MAX) en += EN_REGEN;
            if (en > EN_MAX) en = EN_MAX;
            if (en < 0.0f) en = 0.0f;
            // ===== animacion (segun velocidad real) =====
            speed01 = sqrtf(velX * velX + velZ * velZ) / RUN_SPEED;
            if (speed01 > 1.0f) speed01 = 1.0f;
            moving = (velX * velX + velZ * velZ > 0.002f) ? 1 : 0;
            if (moving) walkPhase += 0.17f;   // ritmo de paso acorde al RUN_SPEED bajo
            idleT += 0.05f;
            } else {
                // ===== GRAVEDAD NO-ABAJO: caer/mover/saltar segun gravDir (wall-walk) =====
                float rx, ry, rz, ffx, ffy, ffz, gx, gy, gz;
                gravBasis(gravG, &rx, &ry, &rz, &ffx, &ffy, &ffz);
                gravDirVec(gravG, &gx, &gy, &gz);
                float ctrlg = grounded ? ACCEL_GND : ACCEL_AIR;
                float wr = wishX * RUN_SPEED, wf2 = (-wishZ) * RUN_SPEED;   // wishZ: arriba(ay<0)=adelante
                gvr += (wr  - gvr) * ctrlg;
                gvf += (wf2 - gvf) * ctrlg;
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
                if (gvg > FALL_CAP) gvg = FALL_CAP;   // misma velocidad terminal, caminando por muros
                float vX = rx * gvr + ffx * gvf + gx * gvg;
                float vY = ry * gvr + ffy * gvf + gy * gvg;
                float vZ = rz * gvr + ffz * gvf + gz * gvg;
                grounded = 0;
                // colision GENERALIZADA (gravBlocked): los muros frenan igual caminando por una pared
                float nX = playerX + vX;
                if (!gravBlocked(gravG, nX, playerY, playerZ, 1.1f)) playerX = nX; else if (gx != 0.0f) { grounded = 1; gvg = 0.0f; }
                float nZ = playerZ + vZ;
                if (!gravBlocked(gravG, playerX, playerY, nZ, 1.1f)) playerZ = nZ; else if (gz != 0.0f) { grounded = 1; gvg = 0.0f; }
                float nY = playerY + vY;
                if (!gravBlocked(gravG, playerX, nY, playerZ, 1.1f)) playerY = nY; else if (gy != 0.0f) { grounded = 1; gvg = 0.0f; }
                // APOYO a lo largo del eje de la gravedad: la cara de la caja se vuelve el suelo
                {
                    const float f  = gravGroundAlong(gravG, playerX, playerY, playerZ);
                    const float sg = (gx + gy + gz);                 // signo del eje de gravedad
                    float *pa = (gx != 0.0f) ? &playerX : ((gy != 0.0f) ? &playerY : &playerZ);
                    if ((*pa - f) * sg >= 0.0f) { *pa = f; gvg = 0.0f; grounded = 1; }
                }
                if (grounded && en < EN_MAX) en += EN_REGEN;
                if (en > EN_MAX) en = EN_MAX; if (en < 0.0f) en = 0.0f;
                if (gvr * gvr + gvf * gvf > 0.004f) heroYaw = atan2f(gvr, gvf);   // encara el avance en el plano
                speed01 = sqrtf(gvr * gvr + gvf * gvf) / RUN_SPEED;
                if (speed01 > 1.0f) speed01 = 1.0f;
                moving = (gvr * gvr + gvf * gvf > 0.002f) ? 1 : 0;
                if (moving) walkPhase += 0.17f;   // las pisadas tambien suenan caminando por un muro
                idleT += 0.05f;
            }

            // ===== BUCLE DE JUEGO: gargolas, anclas, materiales, faro y caida =====
            gargUpdate(playerX, playerY, playerZ, gravG);
            {   // las gargolas que te alcanzan hacen dano; al morir, vuelves al ultimo ancla
                const int nHit = gargHitPlayer(playerX, playerY, playerZ, 1.0f);
                if (nHit > 0) { hp -= (float)(nHit * GARG_TOUCH_DMG); hurtFlash = 12; dlgNotifyHurt(); }
                if (hp <= 0.0f) {
                    objRespawn(&playerX, &playerY, &playerZ);
                    gravG = objRespawnGrav();
                    velX = velY = velZ = 0.0f; gvr = gvf = gvg = 0.0f;
                    grounded = 1; hp = HP_MAX;
                }
            }
            if (hurtFlash > 0) --hurtFlash;
            if (objAnchorTouch(playerX, playerY, playerZ) >= 0) { snap(); saveAuto(sv); }  // ancla NUEVA = autoguardado
            objMaterialTouch(playerX, playerY, playerZ);      // recoger sube la EN maxima
            EN_MAX = objEnergyMax();
            objGoalReached(playerX, playerY, playerZ);
            if (objFallCheck(playerX, playerY, playerZ, gravG, grounded)) {
                objRespawn(&playerX, &playerY, &playerZ);     // vuelve al ULTIMO ancla...
                gravG = objRespawnGrav();                     // ...con SU gravedad (si no, caes 60)
                velX = velY = velZ = 0.0f; gvr = gvf = gvg = 0.0f;
                grounded = 1;
            }

            // ===== DIALOGO: hablan al acercarte, una vez por acercamiento =====
            // dlgNearest mide distancia 3D real, no en el plano: caminando por una pared
            // 20 unidades por encima de un robot NO se le habla, que es lo correcto.
            // Hay que llamarla cada frame aunque no haya nadie cerca, porque es ella la
            // que hace caducar las reacciones de gravedad y de herida.
            {
                const int near = dlgNearest(playerX, playerY, playerZ, 4.0f);
                if (near >= 0 && near != dlgPrev) {
                    dlgLine = dlgSpeak(near, DLG_AUTO);   // DLG_AUTO elige segun el contexto
                    dlgWho  = dlgName(near);
                    dlgTimer = 280;
                }
                dlgPrev = near;
                if (dlgTimer > 0) --dlgTimer;
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

            // ===== balas: mover, chocar con muros, herir gargolas =====
            for (int s = 0; s < 24; ++s) {
                if (g_shots[s].life <= 0) continue;
                g_shots[s].x += g_shots[s].vx;
                g_shots[s].y += g_shots[s].vy;
                g_shots[s].z += g_shots[s].vz;
                if (--g_shots[s].life <= 0) continue;
                if (blocked(g_shots[s].x, g_shots[s].z, g_shots[s].y)) { g_shots[s].life = 0; continue; }
                // UN impacto, no dano por frame: gargDamage devuelve a cuantas hirio.
                // Si acerto, la bala revienta y deja chispa (salvo que perfore).
                if (gargDamage(g_shots[s].x, g_shots[s].y, g_shots[s].z, 1.6f, 34) > 0) {
                    for (int q = 0; q < 12; ++q) if (g_sparks[q].life <= 0) {
                        g_sparks[q].x = g_shots[s].x; g_sparks[q].y = g_shots[s].y;
                        g_sparks[q].z = g_shots[s].z; g_sparks[q].life = 16; break;
                    }
                    if (!g_shots[s].pierce) g_shots[s].life = 0;
                }
                // los robots ya NO son blancos: son los habitantes del sector
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
        drawSky();   // cielo gotico dramatico (2D): luna, gradiente, nubes, silueta del horizonte
        sceGuEnable(GU_DEPTH_TEST);

#if VIEWER_MODE
        drawCandidates();
#else
        sceGumMatrixMode(GU_PROJECTION);
        sceGumLoadIdentity();
        sceGumPerspective(66.0f, 16.0f / 9.0f, 1.0f, 420.0f); // alcanza el telon de agujas (r 120..190) visto desde el lado opuesto del recinto

        sceGumMatrixMode(GU_VIEW);
        sceGumLoadIdentity();
        {
#if FLYCAM
            ScePspFVector3 rot    = { DEG2RAD(48.0f), 0.0f, 0.0f };   // mira el PISO del mundo (adoquin)
            ScePspFVector3 camOff = { 0.0f, -20.0f, -10.0f };
            ScePspFVector3 pOff   = { 0.0f, -2.0f, -34.0f };
            sceGumTranslate(&camOff);
            sceGumRotateXYZ(&rot);
            sceGumTranslate(&pOff);
#else
            // PRIMERA PERSONA (lookAt): el "adelante" del movimiento y de la camara son
            // el MISMO vector por construccion -> sin bugs de signo al girar. Ojo a la
            // altura de la cabeza + head-bob; pivota en el ojo (pitch natural).
            // 3RA PERSONA con GRAVEDAD: "arriba" = -gravedad; adelante en el plano de la gravedad.
            // Con gravedad normal el nub gira la vista (camYaw); con gravedad cambiada la vista
            // queda fija al plano (el nub mueve en el plano) -> siempre consistente con el movimiento.
            (void)bobX; (void)bobY; (void)EYE_H;
            float gx, gy, gz, rx, ry, rz, fx, fy, fz;
            gravDirVec(gravG, &gx, &gy, &gz); gravBasis(gravG, &rx, &ry, &rz, &fx, &fy, &fz);
            float s = (gravG == 0) ? sinf(camYaw) : 0.0f, c = (gravG == 0) ? cosf(camYaw) : 1.0f;
            float fX = fx * c + rx * s, fY = fy * c + ry * s, fZ = fz * c + rz * s;
            float uX = -gx, uY = -gy, uZ = -gz;
            // COLISION DE CAMARA: iba 9 unidades atras a ciegas y se metia DENTRO de los
            // muros y las columnas. Ahora avanza desde el hombro hacia atras y se corta en
            // el ultimo punto libre, con margen para no rozar la piedra.
            const float CAM_BACK = 9.0f, CAM_UP = 4.5f, CAM_R = 0.9f;
            float camBack = CAM_BACK;
            {
                const float ox = playerX + uX * CAM_UP, oy = playerY + uY * CAM_UP, oz = playerZ + uZ * CAM_UP;
                for (int stp = 1; stp <= 9; ++stp) {
                    const float d = CAM_BACK * (float)stp * (1.0f / 9.0f);
                    if (gravBlocked(gravG, ox - fX * d, oy - fY * d, oz - fZ * d, CAM_R)) {
                        camBack = CAM_BACK * (float)(stp - 1) * (1.0f / 9.0f);
                        break;
                    }
                }
                // El minimo hay que PROBARLO, no imponerlo. Subir a 2.2 a ciegas era el
                // caso mas frecuente del juego (de espaldas a un muro del pasillo) y metia
                // el ojo 1.1 DENTRO de la piedra: se veia a traves del muro hacia el vacio.
                if (camBack < 2.2f)
                    camBack = gravBlocked(gravG, ox - fX * 2.2f, oy - fY * 2.2f, oz - fZ * 2.2f, CAM_R)
                            ? 0.0f : 2.2f;
            }
            ScePspFVector3 eye = { playerX - fX * camBack + uX * CAM_UP, playerY - fY * camBack + uY * CAM_UP, playerZ - fZ * camBack + uZ * CAM_UP };
            // LA VISTA AHORA SIGUE AL CABECEO. Antes habia un (void)camPitch: la camara
            // tenia un angulo FIJO mientras el rayo de gravedad SI usaba camPitch, que el
            // D-pad mueve casi 74 grados arriba y abajo. O sea que "la gravedad va a la
            // cara que miras" era mentira: la imagen no se movia, el rayo si, y acababas
            // saltando a una cara que no estabas mirando.
            // Se inclina el objetivo, no el ojo: la camara no orbita ni se mete en sitios
            // raros al mirar al cenit, y el rayo y la imagen ya apuntan a lo mismo.
            const float cpv = cosf(camPitch), spv = sinf(camPitch);
            const float aX = fX * cpv + uX * spv, aY = fY * cpv + uY * spv, aZ = fZ * cpv + uZ * spv;
            ScePspFVector3 ctr = { playerX + aX * 4.0f + uX * 1.8f, playerY + aY * 4.0f + uY * 1.8f, playerZ + aZ * 4.0f + uZ * 1.8f };
            ScePspFVector3 up  = { uX, uY, uZ };
            sceGumLookAt(&eye, &ctr, &up);
#endif
        }

        sceGumMatrixMode(GU_MODEL);
        sceGumLoadIdentity();

        // vector ADELANTE de la camara en XZ (yaw=0 mira -Z). Cull direccional: descarta
        // estructuras cuyo footprint quedo detras de la mirada (funciona en cualquier giro).
        const float fwdX = sinf(camYaw);
        const float fwdZ = -cosf(camYaw);

        // ===== MUNDO SOLIDO =====
        // Orden: OCLUSORES primero. El z-buffer descarta la escritura de color de todo el
        // suelo y el techo que queda detras de una masa, en vez de pintarlo y taparlo. Y una
        // sola atadura de textura por grupo: cada sceGuTexImage invalida la cache de la GE.
        // (Se limpiaron 11 dibujados de 0 vertices y 4 texturas muertas que quedaban de los
        //  mundos anteriores: catedrales, agujas de fondo, ventanas y metal del mirador.)
        sceGuEnable(GU_TEXTURE_2D);
        sceGuTexMode(GU_PSM_5650, 0, 0, GU_TRUE);   // 16 bits + SWIZZLED: mitad de lectura por pixel
        sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGB);
        sceGuTexFilter(GU_NEAREST, GU_NEAREST);     // 1 texel/pixel
        sceGuTexWrap(GU_REPEAT, GU_REPEAT);

        // 1) MUROS, MASAS, ARCADA, BOVEDA y TRACERIA (los que tapan): cull ON
        sceGuEnable(GU_CULL_FACE);
        sceGuTexImage(0, STEX, STEX, STEX, g_indTexS);
        sceGumDrawArray(GU_TRIANGLES, TEX_FLAGS, g_wallEnd - g_wallStart, 0, g_solidWorld + g_wallStart);

        // 2) SUELO y TECHO: quads de UNA cara (winding propio) -> sin cull.
        //    El apron va PRIMERO porque esta 0.03 mas arriba: gana el z-test y el suelo
        //    grueso de debajo se descarta en vez de repintarse.
        sceGuDisable(GU_CULL_FACE);
        sceGuTexImage(0, STEX, STEX, STEX, g_groundTexS);
        {
            ScePspFVector3 ap = { floorf(playerX / 10.0f + 0.5f) * 10.0f, 0.03f,
                                  floorf(playerZ / 10.0f + 0.5f) * 10.0f };
            sceGumTranslate(&ap);
            sceGumDrawArray(GU_TRIANGLES, TEX_FLAGS, g_apronCount, 0, g_apron);
            sceGumLoadIdentity();
        }
        sceGumDrawArray(GU_TRIANGLES, TEX_FLAGS, g_ledgeEnd - g_ledgeStart, 0, g_solidWorld + g_ledgeStart);
        sceGuDisable(GU_TEXTURE_2D);

        // 3) SIN textura. Con cull: objetivo, gargolas, telon de agujas y props (cajas).
        sceGuEnable(GU_CULL_FACE);
        g_objMarkVerts = objBuildMarkers(g_objMark, idleT);
        sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g_objMarkVerts, 0, g_objMark);
        g_gargVerts = gargBuildAll(g_gargVB, idleT);
        sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g_gargVerts, 0, g_gargVB);
        sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g_spireVerts, 0, g_spire);
        sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g_vpropsVerts, 0, g_vprops);
        // Sin cull: robots y braseros usan addLimb/addBall, cuyo winding es el OPUESTO.
        sceGuDisable(GU_CULL_FACE);
        sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g_npcVerts, 0, g_npc);
        sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g_envVerts, 0, g_env);

        // ---- 3RA PERSONA: el HUNTER con ciclo de caminata (anim.h) ----
        // Orden importante: gravedad -> encare -> bob (ya en el cuerpo) -> inclinacion
        // pivotando en la CADERA. Corrige tres fallos del bloque viejo: inclinaba hacia
        // ATRAS, el pitch se aplicaba en el eje del MUNDO (corriendo de lado parecia
        // volcar) y el balanceo tardaba dos ciclos en cerrar.
        {
            HunterPose pose = hunterPose(walkPhase, idleT, speed01, grounded);   // no llamarla hp: ensombrecia la VIDA del jugador
            const float HIP = 1.70f;
            sceGumLoadIdentity();
            ScePspFVector3 pp = { playerX, playerY, playerZ };
            sceGumTranslate(&pp);
            ScePspFVector3 gm = { 0.0f, 0.0f, 0.0f };           // pies hacia la gravedad
            if (gravG == 1) gm.z = 3.14159f; else if (gravG == 2) gm.z = -1.5708f;
            else if (gravG == 3) gm.z = 1.5708f; else if (gravG == 4) gm.x = 1.5708f;
            else if (gravG == 5) gm.x = -1.5708f;
            sceGumRotateXYZ(&gm);
            ScePspFVector3 fy = { 0.0f, heroYaw + pose.yawSway, 0.0f };
            sceGumRotateXYZ(&fy);
            ScePspFVector3 bob = { pose.bobX * HUNTER_BOB_SCALE, pose.bobY * HUNTER_BOB_SCALE, 0.0f };
            sceGumTranslate(&bob);
            ScePspFVector3 up   = { 0.0f,  HIP, 0.0f }; sceGumTranslate(&up);
            ScePspFVector3 body = { hunterPitchRad(pose), 0.0f, hunterRollRad(pose) };
            sceGumRotateXYZ(&body);
            ScePspFVector3 dn   = { 0.0f, -HIP, 0.0f }; sceGumTranslate(&dn);
            sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g_heroV, 0, g_hero);
        }

        // recursos: brillan al acercarse (seccion 12)
        for (int r = 0; r < kResourceCount && r < 64; ++r) {
            if (collected[r]) continue;
            float dx = kResources[r].x * WSCALE - playerX, dz = kResources[r].z * WSCALE - playerZ;
            float d = sqrtf(dx * dx + dz * dz);
            if (d > 16.0f) continue;                          // gema lejos -> sin draw-call/matriz por gema
            if (dx * fwdX + dz * fwdZ < -1.5f) continue;      // gema detras de la mirada
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

        // hacia donde encara el heroe: lo usan el fogonazo del canon y el arco de melee
        const float fx = sinf(heroYaw), fz = -cosf(heroYaw);

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
            float reach = kMelee[curMelee].reach * 0.20f;
            LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 30);
            int vi = 0;
            addSolidBox(v, vi, playerX + fx * reach * 0.6f, playerY + 1.0f, playerZ + fz * reach * 0.6f,
                        reach * 1.2f, 0.28f, reach * 1.2f, RGBA(205, 225, 255, 255));
            sceGumLoadIdentity();
            sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, vi, 0, v);
        }

        // ---- viewmodel: APAGADO en 3ra persona ----
        // Costaba un borrado del buffer de profundidad a pantalla completa + 3 matrices
        // + generar 640 verts por CPU cada frame, todo para un dibujado "if (0)".
        // Cuando vuelva la 1ra persona, recuperar el bloque del historial de git.
        sceGuDisable(GU_TEXTURE_2D);

        // ---------- HUD (2D) ----------
        sceGuDisable(GU_DEPTH_TEST);
        sceGuEnable(GU_BLEND);
        sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
        sceGuDisable(GU_TEXTURE_2D);

        // ---- DESTELLO DE DANO. hurtFlash se ponia a 12 y se decrementaba, pero no lo
        // dibujaba nadie: una gargola te mordia, la vida bajaba y en pantalla no pasaba
        // absolutamente nada. Velo rojo que se apaga solo, mas fuerte cuanto menos vida
        // queda, para que el ultimo mordisco se note de verdad.
        if (hurtFlash > 0) {
            const float t  = (float)hurtFlash / 12.0f;
            const float lo = 1.0f - (hp / HP_MAX);                  // 0 con la vida llena
            int a = (int)(t * (54.0f + 90.0f * lo));
            if (a > 160) a = 160;
            LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 6);
            const unsigned int c = RGBA(150, 18, 22, a);
            v[0] = { c,   0.0f,   0.0f, 0.0f }; v[1] = { c, 480.0f,   0.0f, 0.0f };
            v[2] = { c, 480.0f, 272.0f, 0.0f }; v[3] = { c,   0.0f,   0.0f, 0.0f };
            v[4] = { c, 480.0f, 272.0f, 0.0f }; v[5] = { c,   0.0f, 272.0f, 0.0f };
            sceGumDrawArray(GU_TRIANGLES, GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 6, 0, v);
        }
        {
            HudState st{};
            st.hp = hp; st.hpMax = HP_MAX;
            st.en = en;     st.enMax = EN_MAX;           // EN_MAX sube con cada material
            // La barra GRV media un literal 1.0f: pintaba un medidor SIEMPRE lleno, la
            // misma trampa decorativa que el "1200" escrito a mano de la vida. Ahora mide
            // lo unico que de verdad limita la gravedad: si te queda un cambio. Llena
            // mientras puedas cambiar, y solo se vacia en los ultimos GRAV_EN_SWITCH,
            // que es justo el aviso util (el siguiente salto te deja sin mecanica).
            { float g1 = en / GRAV_EN_SWITCH; if (g1 > 1.0f) g1 = 1.0f; if (g1 < 0.0f) g1 = 0.0f;
              st.grv = g1; }
            st.gravName = kGravName[gravG];
            st.px = playerX; st.pz = playerZ; st.yaw = heroYaw;
            st.district = "CAMPANARIO";
            st.matTaken = objMaterialsTaken(); st.matTotal = objMaterialCount();
            st.pickTimer = pickTimer;
            st.pickName = (pickedType >= 0 && pickedType < kMaterialCount) ? kMaterials[pickedType].name : 0;
            st.weaponName = kRanged[curRanged].name;
            st.ammo = ammoMag[curRanged]; st.ammoMax = kRanged[curRanged].magazine;
            st.reloading = (reloadCD > 0);
            st.fps = fps;
            st.aiming = (aiming != 0);
            st.paused = (paused != 0);
            hudDraw(st);
        }
        // ---- DIALOGO de los robots: nombre y linea, con desvanecido al final ----
        if (dlgTimer > 0 && dlgLine) {
            int a = 255;
            if (dlgTimer < 60) a = dlgTimer * 4;       // se apaga en el ultimo segundo
            drawText(24, 196, 1.0f, RGBA(196, 170, 120, a), dlgWho);
            drawText(24, 208, 1.0f, RGBA(228, 222, 206, a), dlgLine);
        }

        // ---- REPARTO DEL FRAME (mantener SELECT). Sin sprintf: en PSP arrastra medio
        // stdio al binario, y aqui solo hacen falta tres numeros con un decimal.
        if (pad.Buttons & PSP_CTRL_SELECT) {
            int n = 0;
            auto put = [&](const char *lbl, float ms) {
                while (*lbl && n < 70) hud[n++] = *lbl++;
                int t = (int)(ms * 10.0f + 0.5f); if (t > 9999) t = 9999;
                if (t >= 1000) hud[n++] = (char)('0' + (t / 1000) % 10);
                if (t >= 100)  hud[n++] = (char)('0' + (t / 100)  % 10);
                hud[n++] = (char)('0' + (t / 10) % 10);
                hud[n++] = '.';
                hud[n++] = (char)('0' + t % 10);
                hud[n++] = ' ';
            };
            put("CPU ", msCpu); put("GE ", msGe); put("ESP ", msWait);
            hud[n] = 0;
            // Como leerlo: a 30 fps el presupuesto es 33.3 ms. Si manda ESP, sobra
            // tiempo. Si manda GE, el cuello es la GPU (relleno o vertices). Si manda
            // CPU, el cuello es el codigo y optimizar el dibujado no sirve de nada.
            drawText(8, 250, 1.0f, RGBA(235, 225, 190, 255), hud);
        }
        sceGuDisable(GU_TEXTURE_2D);
        sceGuDisable(GU_BLEND);
        sceGuEnable(GU_DEPTH_TEST);
#endif

        // ===== REPARTO DEL FRAME (L + SELECT lo muestra en el HUD) =====
        // Medir antes de optimizar. Hasta ahora el proyecto daba por hecho que el cuello
        // era el relleno, y una auditoria con los numeros delante dice que con -O2 el
        // relleno no llega al 20% del presupuesto de 30 fps. Esto parte el frame en
        // tres y lo zanja en la consola de verdad, en vez de discutirlo con aritmetica:
        //   CPU = juego + generar vertices a mano | GE = transformar + rellenar
        //   ESP = lo que se espera al vblank (si esto domina, sobra tiempo: vamos bien)
        // sceGuSync serializa, asi que el tiempo de GE que sale de aqui es real.
        const long long tCpu = sceKernelGetSystemTimeWide();
        sceGuFinish();
        sceGuSync(0, 0);
        const long long tGe = sceKernelGetSystemTimeWide();
        audioFrame(walkPhase, gravG);  // MISMA fase que la animacion: el sonido cae con el pie que apoya
        sceDisplayWaitVblankStart();
        const long long tEnd = sceKernelGetSystemTimeWide();
        // media movil simple: un valor por frame salta demasiado para leerlo en marcha
        msCpu = msCpu * 0.9f + (float)(tCpu - tFrame) * 0.001f * 0.1f;
        msGe  = msGe  * 0.9f + (float)(tGe  - tCpu  ) * 0.001f * 0.1f;
        msWait= msWait* 0.9f + (float)(tEnd - tGe   ) * 0.001f * 0.1f;
        tFrame = tEnd;
        sceGuSwapBuffers();
    }

    snap(); saveAuto(sv);   // guardado de emergencia al salir
    sceGuTerm();
    sceKernelExitGame();
    return 0;
}
