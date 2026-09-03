#pragma once
// ============================================================================
// PROJECT NOCTIS -- "THE SHAFT": la REPISA (ledge.h)
//
// El jugador esta parado sobre una repisa de piedra/metal que sale del muro +Z
// de una megaestructura vertical (BLAME!) y cuelga sobre un VACIO sin fondo.
// Gotico-industrial frio: piedra clara arriba (es el piso que el jugador mira),
// acero oscuro en barandas/puntales/cadenas, un unico acento calido (el farol ambar).
//
// LIMITES CAMINABLES (clamp del jugador; ~1.2u dentro de la baranda, que va
// 0.25u adentro del borde). Superficie de marcha y = 0, el ojo va encima:
//     X min = -20.5     X max = +20.5
//     Z min = +99.5     Z max = +138.5
// (Z max deja 1.5u al muro z=140 para no meter el ojo en el zocalo). Spawn (0,0,+118)
// mirando -Z (al vacio). Los props se apilan pegados al muro (z >= 131, |x| >= 11)
// y NO bloquean; el corredor central queda libre.
//
// REGLA CRITICA (hardware fijo de PSP: un triangulo con un vertice detras del ojo
// se DESCARTA entero): TODO lo que el ojo puede tener al lado se emite en tramos
// cortos. El piso es una malla fina de quads (celdas 2.44 x 2.47 u, <= 2.5u) y
// los pasamanos, labio, costados de la losa y zocalo van segmentados por tramo.
//
// REQUIERE (main.cpp, antes del include): TexVertex, RGBA, TILE, HAZE, brighten,
// addQuadT, addSolidBoxT, addPyramidT.
// DIBUJAR con: GU_TFX_MODULATE + textura piedra/industrial, sceGuTexWrap(GU_REPEAT,
// GU_REPEAT) (el uv continuo mundo/6 es negativo en x<0) y GU_CULL_FACE OFF
// (winding mixto: rieles/puntales/cadenas son quads sueltos, no cajas cerradas).
// Determinista (hash entero, SIN rand, SIN heap). C++17, <math.h>.
// ============================================================================
#include <math.h>

// ---- marco de la repisa (unidades de mundo) ----
static const float LEDGE_X0 = -22.0f, LEDGE_X1 = 22.0f;   // ancho 44
static const float LEDGE_Z0 =  98.0f, LEDGE_Z1 = 140.0f;  // fondo 42: Z0 = borde al vacio, Z1 = muro
static const float LEDGE_TOP   = 0.0f;                    // superficie de marcha
static const float LEDGE_THICK = 4.0f;                    // losa: y in [-4, -0.05]
static const float LEDGE_RAIL_IN = 0.25f;                 // baranda 0.25u adentro del borde
static const float LEDGE_WALK_X0 = -20.5f, LEDGE_WALK_X1 = 20.5f;   // clamp X
static const float LEDGE_WALK_Z0 =  99.5f, LEDGE_WALK_Z1 = 138.5f;  // clamp Z
static const float LEDGE_SPAWN_X = 0.0f, LEDGE_SPAWN_Z = 118.0f;

// ---- teselado / conteo (todo lo que decide el numero de vertices) ----
static const int LEDGE_GRID_NX     = 18;  // 44/18 = 2.44u
static const int LEDGE_GRID_NZ     = 17;  // 42/17 = 2.47u
static const int LEDGE_SLAB_SEG    = 6;   // tramos por costado de la losa (7u)
static const int LEDGE_LIP_SEG     = 11;  // tramos del labio frontal (4u)
static const int LEDGE_POSTS_FRONT = 12;  // postes del frente (incluye ambas esquinas)
static const int LEDGE_POSTS_SIDE  = 10;  // postes por costado (sin la esquina)
static const int LEDGE_SPANS_FRONT = 11;  // tramos de pasamanos al frente
static const int LEDGE_SPANS_SIDE  = 11;  // tramos por costado (el ultimo muere en el muro)
static const int LEDGE_STRUTS      = 4;
static const int LEDGE_CHAINS      = 4;
static const int LEDGE_SILL_SEG    = 4;

// Vertices EXACTOS que emite buildLedge() (para dimensionar el buffer).
static const int LEDGE_VERTS =
      LEDGE_GRID_NX * LEDGE_GRID_NZ * 6                    // piso (malla fina)          1836
    + 2 * LEDGE_SLAB_SEG * 6                               // costados de la losa          72
    + LEDGE_LIP_SEG * 12 + 12                              // labio: chaflan+cara+2 tapas 144
    + (LEDGE_POSTS_FRONT + 2 * LEDGE_POSTS_SIDE) * 12      // postes (2 quads cruzados)   384
    + (LEDGE_SPANS_FRONT + 2 * LEDGE_SPANS_SIDE) * 18      // pasamanos 12 + riel medio 6 594
    + LEDGE_STRUTS * 24                                    // puntales (4 caras)           96
    + LEDGE_CHAINS * 12 + 30                               // cadenas + contrapeso         78
    + 156                                                  // farol + charco de luz       156
    + 72                                                   // pedestal de control          72
    + 30 + 30 + 72                                         // 2 cajas + barril            132
    + LEDGE_SILL_SEG * 12;                                 // zocalo contra el muro        48
                                                           // TOTAL                      3612

// ---- hash entero determinista (mismo mezclador que winPick/vpHash) ----
static unsigned int ledgeHash(unsigned int x) {
    x ^= 61u; x ^= (x >> 16); x *= 9u; x ^= (x >> 4); x *= 0x27d4eb2du; x ^= (x >> 15);
    return x;
}
static float ledgeJit(unsigned int s) {           // [-1, 1]
    return ((float)(ledgeHash(s) & 1023u) / 511.5f) - 1.0f;
}

// ---- mezcla RGB entre dos colores (alpha 255) ----
static unsigned int ledgeLerp(unsigned int a, unsigned int b, float t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    int ar = a & 0xFF, ag = (a >> 8) & 0xFF, ab = (a >> 16) & 0xFF;
    int br = b & 0xFF, bg = (b >> 8) & 0xFF, bb = (b >> 16) & 0xFF;
    return RGBA(ar + (int)((br - ar) * t), ag + (int)((bg - ag) * t), ab + (int)((bb - ab) * t), 255);
}
// niebla LOCAL por profundidad bajo la repisa (fadeToVoid mide distancia al origen y
// aqui estamos a z~118: se fundiria todo). Lo que cuelga se disuelve en la bruma HAZE.
static unsigned int ledgeDeep(unsigned int c, float y) {
    return ledgeLerp(c, HAZE, (-y) / 70.0f);
}

// quad con GRADIENTE: a,b llevan colAB; c,d llevan colCD (Gouraud gratis, 6 verts).
// Mismo orden de triangulos que addQuadT.
static void ledgeQuadG(TexVertex *buf, int &i,
                       float ax,float ay,float az, float bx,float by,float bz,
                       float cx2,float cy2,float cz2, float dx,float dy,float dz,
                       float u0,float v0,float u1,float v1,
                       unsigned int colAB, unsigned int colCD) {
    buf[i++] = {u0,v0,colAB,ax,ay,az}; buf[i++] = {u1,v0,colAB,bx,by,bz}; buf[i++] = {u1,v1,colCD,cx2,cy2,cz2};
    buf[i++] = {u0,v0,colAB,ax,ay,az}; buf[i++] = {u1,v1,colCD,cx2,cy2,cz2}; buf[i++] = {u0,v1,colCD,dx,dy,dz};
}

// poste/palo delgado = 2 quads verticales cruzados (12 verts; se lee como poste
// desde cualquier angulo y cuesta menos de la mitad que una caja).
static void ledgePost(TexVertex *buf, int &i, float x, float y0, float z,
                      float w, float h, unsigned int col) {
    const float hw = w * 0.5f, y1 = y0 + h, uw = w / TILE, uh = h / TILE;
    addQuadT(buf,i, x-hw,y0,z,  x+hw,y0,z,  x+hw,y1,z,  x-hw,y1,z,  0,uh,uw,0, col);                  // plano XY
    addQuadT(buf,i, x,y0,z-hw,  x,y0,z+hw,  x,y1,z+hw,  x,y1,z-hw,  0,uh,uw,0, brighten(col,0.80f)); // plano ZY
}
// idem con gradiente vertical (cadenas que se pierden en la bruma)
static void ledgePostG(TexVertex *buf, int &i, float x, float y0, float z,
                       float w, float h, unsigned int colBottom, unsigned int colTop) {
    const float hw = w * 0.5f, y1 = y0 + h, uw = w / TILE, uh = h / TILE;
    ledgeQuadG(buf,i, x-hw,y0,z,  x+hw,y0,z,  x+hw,y1,z,  x-hw,y1,z,  0,uh,uw,0, colBottom, colTop);
    ledgeQuadG(buf,i, x,y0,z-hw,  x,y0,z+hw,  x,y1,z+hw,  x,y1,z-hw,  0,uh,uw,0,
               brighten(colBottom,0.80f), brighten(colTop,0.80f));
}

// tramo de pasamanos a lo largo de X (frente, z fijo): cara superior + cara interior.
// innerZ = +1 si el area de marcha esta hacia +z (frente), -1 si hacia -z.
static void ledgeRailX(TexVertex *buf, int &i, float x0, float x1, float y, float z,
                       float hw, float t, unsigned int col, float innerZ) {
    const float zi = z + innerZ * hw, u0 = x0 / TILE, u1 = x1 / TILE;
    addQuadT(buf,i, x0,y+t,z-hw, x1,y+t,z-hw, x1,y+t,z+hw, x0,y+t,z+hw, u0,0,u1,hw*2/TILE, brighten(col,1.15f)); // arriba
    addQuadT(buf,i, x0,y,zi,     x1,y,zi,     x1,y+t,zi,   x0,y+t,zi,   u0,t/TILE,u1,0,    brighten(col,0.85f)); // interior
}
// tramo de pasamanos a lo largo de Z (costados, x fijo). innerX = +1 si la marcha esta hacia +x.
static void ledgeRailZ(TexVertex *buf, int &i, float z0, float z1, float y, float x,
                       float hw, float t, unsigned int col, float innerX) {
    const float xi = x + innerX * hw, u0 = z0 / TILE, u1 = z1 / TILE;
    addQuadT(buf,i, x-hw,y+t,z0, x-hw,y+t,z1, x+hw,y+t,z1, x+hw,y+t,z0, u0,0,u1,hw*2/TILE, brighten(col,1.15f)); // arriba
    addQuadT(buf,i, xi,y,z0,     xi,y,z1,     xi,y+t,z1,   xi,y+t,z0,   u0,t/TILE,u1,0,    brighten(col,0.85f)); // interior
}
// riel medio: solo la cara interior (6 verts; desde el ojo, arriba, no se ve el canto)
static void ledgeMidRailX(TexVertex *buf, int &i, float x0, float x1, float y, float z,
                          float hw, float t, unsigned int col, float innerZ) {
    const float zi = z + innerZ * hw;
    addQuadT(buf,i, x0,y,zi, x1,y,zi, x1,y+t,zi, x0,y+t,zi, x0/TILE,t/TILE,x1/TILE,0, brighten(col,0.85f));
}
static void ledgeMidRailZ(TexVertex *buf, int &i, float z0, float z1, float y, float x,
                          float hw, float t, unsigned int col, float innerX) {
    const float xi = x + innerX * hw;
    addQuadT(buf,i, xi,y,z0, xi,y,z1, xi,y+t,z1, xi,y+t,z0, z0/TILE,t/TILE,z1/TILE,0, brighten(col,0.85f));
}

// puntal inclinado: prisma de seccion rectangular entre P0 (bajo el frente) y P1
// (dentro del muro +Z). 4 caras con gradiente near->far (24 verts). hw = semiancho
// en X, ht = semiespesor perpendicular al eje (en el plano YZ).
static void ledgeStrut(TexVertex *buf, int &i, float x,
                       float y0, float z0, float y1, float z1,
                       float hw, float ht, unsigned int colNear, unsigned int colFar) {
    const float dy = y1 - y0, dz = z1 - z0;
    const float len = sqrtf(dy * dy + dz * dz);
    const float uy = dy / len, uz = dz / len;        // eje
    const float ny = uz, nz = -uy;                   // normal en YZ: apunta "arriba/adelante" (+y)
    const float ay0 = y0 + ny * ht, az0 = z0 + nz * ht, by0 = y0 - ny * ht, bz0 = z0 - nz * ht;
    const float ay1 = y1 + ny * ht, az1 = z1 + nz * ht, by1 = y1 - ny * ht, bz1 = z1 - nz * ht;
    const float xl = x - hw, xr = x + hw;
    const float uw = hw * 2.0f / TILE, ul = len / TILE, ut = ht * 2.0f / TILE;
    // cara superior (la que se ve al asomarse por la baranda)
    ledgeQuadG(buf,i, xl,ay0,az0, xr,ay0,az0, xr,ay1,az1, xl,ay1,az1, 0,0,uw,ul,
               brighten(colNear,1.10f), brighten(colFar,1.10f));
    // cara inferior
    ledgeQuadG(buf,i, xr,by0,bz0, xl,by0,bz0, xl,by1,bz1, xr,by1,bz1, 0,0,uw,ul,
               brighten(colNear,0.60f), brighten(colFar,0.60f));
    // costado izquierdo (x = xl)
    ledgeQuadG(buf,i, xl,by0,bz0, xl,ay0,az0, xl,ay1,az1, xl,by1,bz1, 0,0,ut,ul,
               brighten(colNear,0.78f), brighten(colFar,0.78f));
    // costado derecho (x = xr)
    ledgeQuadG(buf,i, xr,ay0,az0, xr,by0,bz0, xr,by1,bz1, xr,ay1,az1, 0,0,ut,ul,
               brighten(colNear,0.78f), brighten(colFar,0.78f));
}

// barril octogonal: 8 caras laterales sombreadas por orientacion + tapa en abanico (72 verts)
static void ledgeBarrel(TexVertex *buf, int &i, float cx, float cz, float r, float h, unsigned int col) {
    const float PI2 = 6.28318531f, step = PI2 / 8.0f;
    const float useg = (PI2 * r / 8.0f) / TILE, uh = h / TILE;
    const unsigned int top = brighten(col, 1.15f);
    for (int k = 0; k < 8; ++k) {
        const float a0 = k * step, a1 = (k + 1) * step, am = a0 + step * 0.5f;
        const float x0 = cx + r * cosf(a0), z0 = cz + r * sinf(a0);
        const float x1 = cx + r * cosf(a1), z1 = cz + r * sinf(a1);
        const float f = 0.62f + 0.34f * (0.5f + 0.5f * cosf(am - 0.6f)); // sombreado por orientacion (luz desde +x, el corredor)
        addQuadT(buf,i, x0,0,z0, x1,0,z1, x1,h,z1, x0,h,z0, k*useg,uh,(k+1)*useg,0, brighten(col, f));
        // tapa: triangulo del abanico (centro, k, k+1) -- 3 verts
        buf[i++] = {cx/TILE, cz/TILE, top, cx, h, cz};
        buf[i++] = {x0/TILE, z0/TILE, top, x0, h, z0};
        buf[i++] = {x1/TILE, z1/TILE, top, x1, h, z1};
    }
}

// ============================================================================
// buildLedge: emite la repisa completa en buf (LEDGE_VERTS vertices exactos).
// Dibujar con la textura de piedra/industrial en GU_TFX_MODULATE.
// ============================================================================
static void buildLedge(TexVertex *buf, int &i) {
    const int iStart = i;
    (void)iStart;

    // ---- paleta ----
    const unsigned int STONE = brighten(RGBA(120, 118, 112, 255), 1.6f);  // (192,188,179): piso claro
    const unsigned int STEEL = RGBA(60, 64, 72, 255);                     // acero frio
    const unsigned int LAMP  = RGBA(255, 180, 90, 255);                   // ambar calido (acento)
    const unsigned int RUST  = RGBA(112, 78, 56, 255);                    // barril oxidado
    const unsigned int SCREEN = RGBA(80, 150, 165, 255);                  // pantalla fria del pedestal

    const float X0 = LEDGE_X0, X1 = LEDGE_X1, Z0 = LEDGE_Z0, Z1 = LEDGE_Z1;
    const float TOP = LEDGE_TOP, SLAB_Y0 = TOP - LEDGE_THICK, SLAB_Y1 = TOP - 0.05f;

    // ------------------------------------------------------------------
    // 1) PISO: malla fina de quads (celdas <= 2.5u) -- NUNCA un quad grande.
    //    uv = mundo/TILE continuo; desgaste determinista +-7% por celda y las
    //    2 filas del borde al vacio un poco mas oscuras (humedad).
    // ------------------------------------------------------------------
    {
        const float cw = (X1 - X0) / LEDGE_GRID_NX, cd = (Z1 - Z0) / LEDGE_GRID_NZ;
        for (int iz = 0; iz < LEDGE_GRID_NZ; ++iz) {
            for (int ix = 0; ix < LEDGE_GRID_NX; ++ix) {
                const float x0 = X0 + ix * cw, x1 = x0 + cw;
                const float z0 = Z0 + iz * cd, z1 = z0 + cd;
                float f = 1.0f + 0.07f * ledgeJit((unsigned int)(ix * 131 + iz * 17 + 7));
                if (iz < 2) f *= 0.93f;
                addQuadT(buf,i, x0,TOP,z0, x1,TOP,z0, x1,TOP,z1, x0,TOP,z1,
                         x0/TILE, z0/TILE, x1/TILE, z1/TILE, brighten(STONE, f));
            }
        }
    }

    // ------------------------------------------------------------------
    // 2) LOSA bajo el piso: costados x=+-22 en tramos de 7u (la cara frontal la
    //    tapa el labio; el fondo no se ve nunca desde arriba).
    // ------------------------------------------------------------------
    {
        const unsigned int side = brighten(STONE, 0.60f);
        const float segD = (Z1 - Z0) / LEDGE_SLAB_SEG, uh = (SLAB_Y1 - SLAB_Y0) / TILE;
        for (int s = 0; s < LEDGE_SLAB_SEG; ++s) {
            const float z0 = Z0 + s * segD, z1 = z0 + segD;
            addQuadT(buf,i, X0,SLAB_Y0,z1, X0,SLAB_Y0,z0, X0,SLAB_Y1,z0, X0,SLAB_Y1,z1, z0/TILE,uh,z1/TILE,0, side); // izq
            addQuadT(buf,i, X1,SLAB_Y0,z0, X1,SLAB_Y0,z1, X1,SLAB_Y1,z1, X1,SLAB_Y1,z0, z0/TILE,uh,z1/TILE,0, side); // der
        }
    }

    // ------------------------------------------------------------------
    // 3) LABIO frontal chaflanado: chaflan (y 0 -> -1, z 98 -> 97.2) + cara
    //    vertical (y -1 -> -4 en z 97.2) en tramos de 4u + 2 tapas laterales.
    // ------------------------------------------------------------------
    {
        const float LIP_Z = Z0 - 0.8f, LIP_Y = TOP - 1.0f;
        const unsigned int cham = brighten(STONE, 0.95f), face = brighten(STONE, 0.70f), cap = brighten(STONE, 0.60f);
        const float segW = (X1 - X0) / LEDGE_LIP_SEG;
        for (int s = 0; s < LEDGE_LIP_SEG; ++s) {
            const float x0 = X0 + s * segW, x1 = x0 + segW;
            addQuadT(buf,i, x0,LIP_Y,LIP_Z, x1,LIP_Y,LIP_Z, x1,TOP,Z0, x0,TOP,Z0,
                     x0/TILE, 1.3f/TILE, x1/TILE, 0, cham);
            addQuadT(buf,i, x0,SLAB_Y0,LIP_Z, x1,SLAB_Y0,LIP_Z, x1,LIP_Y,LIP_Z, x0,LIP_Y,LIP_Z,
                     x0/TILE, (LIP_Y-SLAB_Y0)/TILE, x1/TILE, 0, face);
        }
        // tapas en x = +-22 (cierran el perfil del labio)
        addQuadT(buf,i, X0,SLAB_Y0,Z0, X0,SLAB_Y0,LIP_Z, X0,LIP_Y,LIP_Z, X0,TOP,Z0, 0,LEDGE_THICK/TILE,0.8f/TILE,0, cap);
        addQuadT(buf,i, X1,SLAB_Y0,LIP_Z, X1,SLAB_Y0,Z0, X1,TOP,Z0, X1,LIP_Y,LIP_Z, 0,LEDGE_THICK/TILE,0.8f/TILE,0, cap);
    }

    // ------------------------------------------------------------------
    // 4) BARANDA: frente (z = 98.25) y costados (x = +-21.75). Sin baranda atras.
    //    Postes 0.25 x 1.4 cada ~4u; pasamanos y=1.3 y riel medio y=0.7, ambos
    //    SEGMENTADOS por tramo entre postes (regla del ojo).
    // ------------------------------------------------------------------
    {
        const float RX0 = X0 + LEDGE_RAIL_IN, RX1 = X1 - LEDGE_RAIL_IN, RZ = Z0 + LEDGE_RAIL_IN;
        const float POST_W = 0.25f, POST_H = 1.4f;
        const float TOP_Y = 1.3f, MID_Y = 0.7f, RAIL_HW = 0.08f, RAIL_T = 0.12f;
        const unsigned int post = brighten(STEEL, 0.95f);

        // postes del frente (12, incluye esquinas)
        const float fStep = (RX1 - RX0) / (LEDGE_POSTS_FRONT - 1);
        for (int k = 0; k < LEDGE_POSTS_FRONT; ++k)
            ledgePost(buf,i, RX0 + k * fStep, TOP, RZ, POST_W, POST_H, post);
        // postes de los costados (10 por lado, z = 102.25 .. 138.25)
        for (int k = 1; k <= LEDGE_POSTS_SIDE; ++k) {
            const float z = RZ + k * 4.0f;
            ledgePost(buf,i, RX0, TOP, z, POST_W, POST_H, post);
            ledgePost(buf,i, RX1, TOP, z, POST_W, POST_H, post);
        }
        // pasamanos + riel medio del frente (11 tramos entre postes)
        for (int k = 0; k < LEDGE_SPANS_FRONT; ++k) {
            const float x0 = RX0 + k * fStep, x1 = x0 + fStep;
            ledgeRailX(buf,i, x0, x1, TOP_Y, RZ, RAIL_HW, RAIL_T, STEEL, +1.0f);
            ledgeMidRailX(buf,i, x0, x1, MID_Y, RZ, RAIL_HW, RAIL_T, STEEL, +1.0f);
        }
        // pasamanos + riel medio de los costados (11 tramos: 10 entre postes + 1 hasta el muro)
        for (int k = 0; k < LEDGE_SPANS_SIDE; ++k) {
            const float z0 = RZ + k * 4.0f;
            const float z1 = (k == LEDGE_SPANS_SIDE - 1) ? Z1 : z0 + 4.0f;
            ledgeRailZ(buf,i, z0, z1, TOP_Y, RX0, RAIL_HW, RAIL_T, STEEL, +1.0f);
            ledgeMidRailZ(buf,i, z0, z1, MID_Y, RX0, RAIL_HW, RAIL_T, STEEL, +1.0f);
            ledgeRailZ(buf,i, z0, z1, TOP_Y, RX1, RAIL_HW, RAIL_T, STEEL, -1.0f);
            ledgeMidRailZ(buf,i, z0, z1, MID_Y, RX1, RAIL_HW, RAIL_T, STEEL, -1.0f);
        }
    }

    // ------------------------------------------------------------------
    // 5) PUNTALES bajo la repisa: 4 prismas inclinados desde bajo el frente
    //    (y -2.5, z 100.5) hacia abajo y atras hasta el muro (y -34, z 152).
    //    Se disuelven en la bruma con la profundidad (gradiente por vertice).
    // ------------------------------------------------------------------
    {
        static const float sx[LEDGE_STRUTS] = { -15.0f, -5.0f, 5.0f, 15.0f };
        const unsigned int nearCol = brighten(STEEL, 0.85f);       // en sombra bajo la losa
        for (int k = 0; k < LEDGE_STRUTS; ++k) {
            const float y1 = -34.0f + 1.5f * ledgeJit((unsigned int)(k * 7 + 3));
            ledgeStrut(buf,i, sx[k], -2.5f, Z0 + 2.5f, y1, 152.0f, 1.1f, 0.9f, nearCol, ledgeDeep(nearCol, y1));
        }
    }

    // ------------------------------------------------------------------
    // 6) CADENAS colgando al vacio desde bajo el frente (2 quads cruzados con
    //    gradiente) + un contrapeso en la mas larga.
    // ------------------------------------------------------------------
    {
        static const float cx[LEDGE_CHAINS] = { -19.5f, -8.5f, 9.5f, 19.5f };
        static const float cz[LEDGE_CHAINS] = { 100.6f, 101.4f, 100.9f, 101.2f };
        static const float cl[LEDGE_CHAINS] = { 26.0f, 44.0f, 20.0f, 36.0f };
        const unsigned int chainTop = brighten(STEEL, 0.70f);
        const float anchorY = SLAB_Y0 + 0.5f;                       // empotrada 0.5u en la losa
        for (int k = 0; k < LEDGE_CHAINS; ++k) {
            const float yb = anchorY - cl[k];
            ledgePostG(buf,i, cx[k], yb, cz[k], 0.30f, cl[k], ledgeDeep(chainTop, yb), chainTop);
        }
        // contrapeso (bloque) al final de la cadena 1 (la de 44u)
        {
            const float yb = anchorY - cl[1];
            addSolidBoxT(buf,i, cx[1], yb - 1.6f, cz[1], 1.0f, 1.0f, 1.6f, ledgeDeep(brighten(STEEL, 0.8f), yb));
        }
    }

    // ------------------------------------------------------------------
    // 7) FAROL junto al muro (x -13.5, z 137): plinto + poste + brazo hacia el
    //    frente + LAMPARA emisiva ambar colgando + capuchon + charco de luz en el
    //    piso (abanico Gouraud: centro calido, borde = color del piso -> sin canto).
    // ------------------------------------------------------------------
    {
        const float lx = -13.5f, lz = 137.0f;
        const float poleH = 4.2f, armLen = 1.8f;
        const float headZ = lz - armLen + 0.2f;                    // lampara al final del brazo
        addSolidBoxT(buf,i, lx, TOP, lz, 1.2f, 1.2f, 0.35f, brighten(STEEL, 1.05f));                 // plinto
        addSolidBoxT(buf,i, lx, TOP + 0.35f, lz, 0.28f, 0.28f, poleH - 0.35f, STEEL);                // poste
        addSolidBoxT(buf,i, lx, TOP + poleH - 0.22f, lz - armLen * 0.5f, 0.22f, armLen, 0.22f, STEEL); // brazo
        addSolidBoxT(buf,i, lx, TOP + 3.1f, headZ, 0.8f, 0.8f, 0.8f, LAMP);                          // LAMPARA (emisiva)
        addPyramidT(buf,i, lx, TOP + 3.9f, headZ, 1.0f, 1.0f, 0.45f, brighten(STEEL, 0.9f));         // capuchon
        // charco de luz: 8 triangulos, radio 2.4 (aristas ~1.8u), y = +0.05 sobre el piso
        {
            const float py = TOP + 0.05f, r = 2.4f, step = 6.28318531f / 8.0f;
            const unsigned int cCenter = RGBA(255, 214, 150, 255);
            const unsigned int cRim = STONE;
            for (int k = 0; k < 8; ++k) {
                const float a0 = k * step, a1 = (k + 1) * step;
                const float x0 = lx + r * cosf(a0), z0 = headZ + r * sinf(a0);
                const float x1 = lx + r * cosf(a1), z1 = headZ + r * sinf(a1);
                buf[i++] = {lx/TILE, headZ/TILE, cCenter, lx, py, headZ};
                buf[i++] = {x0/TILE, z0/TILE,    cRim,    x0, py, z0};
                buf[i++] = {x1/TILE, z1/TILE,    cRim,    x1, py, z1};
            }
        }
    }

    // ------------------------------------------------------------------
    // 8) PEDESTAL de control (x 12.5, z 137): cuerpo + respaldo + panel inclinado
    //    + pantalla fria (72 verts).
    // ------------------------------------------------------------------
    {
        const float px = 12.5f, pz = 137.0f, bw = 1.5f, bd = 0.9f, bh = 1.0f;
        const float x0 = px - bw * 0.5f, x1 = px + bw * 0.5f;
        const float zf = pz - bd * 0.5f;                            // cara frontal del cuerpo (z 136.55)
        addSolidBoxT(buf,i, px, TOP, pz, bw, bd, bh, brighten(STEEL, 1.10f));                      // cuerpo
        addSolidBoxT(buf,i, px, TOP + bh, pz + 0.25f, bw, 0.4f, 0.3f, brighten(STEEL, 1.05f));     // respaldo
        // panel inclinado: de (y 1.0, z 136.55) a (y 1.3, z 137.05)
        const float py0 = TOP + bh, pz0 = zf, py1 = TOP + bh + 0.3f, pz1 = pz + 0.05f;
        addQuadT(buf,i, x0,py0,pz0, x1,py0,pz0, x1,py1,pz1, x0,py1,pz1, 0,0.1f,bw/TILE,0, brighten(STEEL, 1.0f));
        // pantalla: inset en el panel, desplazada 0.02 por la normal (0, 0.857, -0.514)
        const float sx0 = x0 + 0.25f, sx1 = x1 - 0.25f;
        const float qy0 = py0 + (py1 - py0) * 0.25f + 0.017f, qz0 = pz0 + (pz1 - pz0) * 0.25f - 0.010f;
        const float qy1 = py0 + (py1 - py0) * 0.75f + 0.017f, qz1 = pz0 + (pz1 - pz0) * 0.75f - 0.010f;
        addQuadT(buf,i, sx0,qy0,qz0, sx1,qy0,qz0, sx1,qy1,qz1, sx0,qy1,qz1, 0,0,0.2f,0.1f, SCREEN);
    }

    // ------------------------------------------------------------------
    // 9) CARGA junto al muro: 2 cajas metalicas apiladas (x ~18, z ~134.5) y un
    //    barril oxidado (x -18.5, z 133.2). Fuera del corredor central.
    // ------------------------------------------------------------------
    addSolidBoxT(buf,i, 18.2f, TOP, 134.6f, 1.7f, 1.7f, 1.6f, brighten(STEEL, 1.25f));   // caja grande
    addSolidBoxT(buf,i, 18.5f, TOP + 1.6f, 134.3f, 1.1f, 1.1f, 1.0f, brighten(STEEL, 1.10f)); // caja chica encima
    ledgeBarrel(buf,i, -18.5f, 133.2f, 0.62f, 1.3f, RUST);

    // ------------------------------------------------------------------
    // 10) ZOCALO contra el muro (z 139.4..140, alto 0.5): esconde la junta
    //     piso/muro. 4 tramos de 11u, cara superior + cara frontal.
    // ------------------------------------------------------------------
    {
        const float sz0 = Z1 - 0.6f, sh = 0.5f;
        const float segW = (X1 - X0) / LEDGE_SILL_SEG;
        const unsigned int sTop = brighten(STONE, 1.05f), sFace = brighten(STONE, 0.85f);
        for (int s = 0; s < LEDGE_SILL_SEG; ++s) {
            const float x0 = X0 + s * segW, x1 = x0 + segW;
            addQuadT(buf,i, x0,TOP,sz0, x1,TOP,sz0, x1,TOP+sh,sz0, x0,TOP+sh,sz0, x0/TILE,sh/TILE,x1/TILE,0, sFace);
            addQuadT(buf,i, x0,TOP+sh,sz0, x1,TOP+sh,sz0, x1,TOP+sh,Z1, x0,TOP+sh,Z1, x0/TILE,sz0/TILE,x1/TILE,Z1/TILE, sTop);
        }
    }
    // (i - iStart) == LEDGE_VERTS
}

// ============================================================================
// NOTA FINAL
// Elementos: piso en malla fina 18x17 (celdas 2.44x2.47u, uv continuo mundo/6,
// desgaste determinista), losa de 4u con costados segmentados, labio frontal
// chaflanado (chaflan + cara vertical + tapas), baranda de acero al frente y a
// los dos costados (32 postes, pasamanos y riel medio segmentados por tramo,
// sin baranda contra el muro), 4 puntales inclinados que se hunden en el muro +Z
// (z 152) disolviendose en la bruma, 4 cadenas colgando al vacio (20-44u) con
// contrapeso, farol de acero con lampara ambar emisiva y charco de luz Gouraud,
// pedestal de control con pantalla fria, 2 cajas apiladas, 1 barril octogonal
// oxidado y un zocalo contra el muro.
//
// Vertices: LEDGE_VERTS = 3612 (exacto, sumado por constante arriba).
//   La malla del piso sola, con celdas <= 2.5u, cuesta 1836 verts, y la regla del
//   ojo obliga a segmentar tambien pasamanos/labio/costados: por eso el conjunto
//   NO cabe en 2000. Para recortar: LEDGE_GRID_NX/NZ (no subir de 2.5u por celda),
//   LEDGE_POSTS_*/SPANS_* (espaciado 4u -> 5u ahorra ~120) o quitar el riel medio
//   (-198). Buffer sugerido: TexVertex g_ledge[LEDGE_VERTS] alineado a 16.
//
// Limites caminables (clamp): X [-20.5, +20.5]   Z [+99.5, +138.5]   y = 0.
// ============================================================================
