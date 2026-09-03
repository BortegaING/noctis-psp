#pragma once
// PROJECT NOCTIS -- "EL POZO" (THE SHAFT). Directiva maestra: "Estoy dentro de
// una megaestructura de BLAME!". El patio se reemplaza por el INTERIOR de un
// pozo vertical colosal: octogono de apotema 160, paredes de y=-700 (abismo)
// a y=+500 (se pierden en la bruma). El jugador esta en una repisa de la pared
// +Z (z ~ 98..140, y=0), en (0,0,118) mirando a -Z. Relieve industrial-gotico
// hecho en GEOMETRIA a escala colosal: contrafuertes/pilastras, cornisas-viga,
// tuberias, bahias rehundidas, portones/ventilaciones, bloques de maquinaria y
// unas pocas lamparas ambar. Fill barato: caras planas grandes, sin caras
// traseras ni tapas que el jugador no pueda ver.
//
// Se incluye DESPUES de TexVertex/TILE/HAZE/RGBA/brighten/fadeToVoid (main.cpp).
// Determinista (hash entero), sin rand, sin heap, C++17, <math.h>.
// Presupuesto: <= 4200 verts (ver nota al final; conteo exacto en el reporte).
//
// MARCO LOCAL DE PARED (s, y, p):
//   s = a lo largo de la pared, y = altura del mundo, p = RESALTE hacia el
//   centro del pozo (p=0 sobre el plano de la pared, p>0 sobresale).
//   Pared k (k=0..7): normal EXTERIOR n_k = (sin(45k), 0, cos(45k)):
//   k=0 -> +Z (la pared del jugador), k=2 -> +X, k=4 -> -Z, k=6 -> -X.
//   R_k = (n_z, 0, -n_x).  world(s,y,p) = C_k + R_k*s + Y*y - n_k*p, C_k = n_k*160.
//   (s crece hacia la IZQUIERDA del observador parado en el centro; no importa
//   para el winding, que se deriva por producto cruz.)
//
// WINDING (convencion): IDENTICO al de addSolidBoxT/addQuadT de main.cpp: en un
// quad a,b,c,d (tris a,b,c / a,c,d) la cara VISIBLE es la del lado de
// (c-b) x (b-a). Los paneles se emiten a=(s0,y0) b=(s1,y0) c=(s1,y1) d=(s0,y1)
// en el plano de la pared => normal visible = -n_k = HACIA EL ORIGEN (miran
// hacia adentro). Cada cara de caja sigue la misma regla (ver SK_* abajo), de
// modo que NOCTIS_FRONTFACE queda como esta.
//
// COLOR por vertice (por eso no se usa addQuadT/addSolidBoxT directamente):
//   base -> brighten(por cara) -> fadeToVoid(dist_XZ_al_jugador * SHAFT_FADE_K)
//        -> mezcla hacia HAZE por altura: t = clamp(|y|/500, 0, 0.85).
//   Las bandas verticales se cortan en los nudos |y|=425 e y=0 para que la
//   interpolacion lineal del hardware reproduzca esa curva exactamente.
//
// PSP y el near plane: sin GU_CLIP_PLANES el GE descarta un triangulo entero si
// un vertice queda detras de la camara. Por eso todo elemento largo en s se
// parte en piezas cuya cantidad depende de la distancia al jugador (pared +Z:
// 4 piezas de ~33u; paredes lejanas: 1). En y no hace falta (no cambia la
// profundidad).

#include <math.h>

static const float SHAFT_RIN       = 160.0f;   // apotema del octogono (centro -> pared)
static const float SHAFT_YBOT      = -700.0f;  // fondo (abismo)
static const float SHAFT_YTOP      =  500.0f;  // tope (bruma)
static const float SHAFT_SIDE      = 2.0f * 160.0f * 0.41421356f; // lado = 2*apotema*tan(22.5) = 132.55
static const float SHAFT_PLAYER_X  = 0.0f;
static const float SHAFT_PLAYER_Z  = 118.0f;
static const float SHAFT_FADE_K    = 0.30f;    // dist*K entra en fadeToVoid(26..98): pared +Z (42u) sin fade, pared -Z (278u) ~80% bruma
static const float SHAFT_HAZE_Y    = 500.0f;   // |y| al que la bruma por altura satura
static const float SHAFT_HAZE_MAX  = 0.85f;
static const float SHAFT_SEG_FRAC  = 0.55f;    // piezas en s: len / (dist*FRAC) (cap 4)

// ---------------- marco de pared ----------------
struct ShaftFrame { float nx, nz; float cx, cz; };

static ShaftFrame shaftFrame(float phiDeg, float rad) {
    const float a = phiDeg * 0.017453292f;
    ShaftFrame f;
    f.nx = sinf(a); f.nz = cosf(a);
    f.cx = f.nx * rad; f.cz = f.nz * rad;
    return f;
}
static ShaftFrame shaftWall(int k) { return shaftFrame(45.0f * (float)k, SHAFT_RIN); }

static inline void shaftWorld(const ShaftFrame& f, float s, float y, float p,
                              float& wx, float& wy, float& wz) {
    wx = f.cx + f.nz * s - f.nx * p;
    wy = y;
    wz = f.cz - f.nx * s - f.nz * p;
}

// distancia XZ de un punto local al jugador
static float shaftDist(const ShaftFrame& f, float s, float p) {
    float wx, wy, wz; shaftWorld(f, s, 0.0f, p, wx, wy, wz);
    const float dx = wx - SHAFT_PLAYER_X, dz = wz - SHAFT_PLAYER_Z;
    return sqrtf(dx * dx + dz * dz);
}

// cantidad de piezas en s para un tramo de largo len a distancia dist (1..4)
static int shaftSubCount(float len, float dist) {
    if (dist < 20.0f) dist = 20.0f;
    int n = (int)(len / (dist * SHAFT_SEG_FRAC)) + 1;
    if (n < 1) n = 1;
    if (n > 4) n = 4;
    return n;
}

// hash entero de 32 bits (overflow unsigned = definido)
static inline unsigned int shaftHash(unsigned int x) {
    x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16;
    return x;
}

// ---------------- color ----------------
static unsigned int shaftMixHaze(unsigned int base, float t) {
    int br = base & 0xFF, bg = (base >> 8) & 0xFF, bb = (base >> 16) & 0xFF;
    int cr = HAZE & 0xFF, cg = (HAZE >> 8) & 0xFF, cb = (HAZE >> 16) & 0xFF;
    int r  = br + (int)((cr - br) * t);
    int g  = bg + (int)((cg - bg) * t);
    int b2 = bb + (int)((cb - bb) * t);
    return RGBA(r, g, b2, 255);
}
static unsigned int shaftShade(unsigned int base, float wx, float wy, float wz, bool heightFog) {
    const float dx = wx - SHAFT_PLAYER_X, dz = wz - SHAFT_PLAYER_Z;
    unsigned int c = fadeToVoid(base, sqrtf(dx * dx + dz * dz) * SHAFT_FADE_K);
    if (heightFog) {
        float t = fabsf(wy) / SHAFT_HAZE_Y;
        if (t > SHAFT_HAZE_MAX) t = SHAFT_HAZE_MAX;
        c = shaftMixHaze(c, t);
    }
    return c;
}

// tipo de cara: fija el brillo (como addSolidBoxT: techo claro, lados oscuros)
// y documenta la normal visible esperada en el marco local (s,y,p):
//   SK_PANEL/SK_FRONT/SK_FLAT/SK_LAMP -> (0,0,+1) hacia el centro
//   SK_SIDE_LO -> (-1,0,0)   SK_SIDE_HI -> (+1,0,0)
//   SK_TOP -> (0,+1,0)       SK_BOTTOM -> (0,-1,0)
enum ShaftFaceKind { SK_PANEL = 0, SK_FRONT, SK_SIDE_LO, SK_SIDE_HI, SK_TOP, SK_BOTTOM, SK_FLAT, SK_LAMP };
static const float kShaftFaceMul[8] = { 0.78f, 0.86f, 0.60f, 0.60f, 1.15f, 0.50f, 1.0f, 1.0f };

#ifdef SHAFT_HOST_CHECK
static void shaftHostCheck(const ShaftFrame& f, int face, const TexVertex* v); // solo harness de PC
#endif

// quad con color POR VERTICE, coords locales (s,y,p), tris a,b,c / a,c,d
static void shaftQuad(TexVertex* buf, int& i, const ShaftFrame& f,
                      float as, float ay, float ap, float bs, float by, float bp,
                      float cs, float cy, float cp, float ds, float dy, float dp,
                      float ua, float va, float ub, float vb, float uc, float vc, float ud, float vd,
                      unsigned int base, int face) {
    const unsigned int b0 = brighten(base, kShaftFaceMul[face]);
    const bool fog = (face != SK_LAMP);
    float A[3], B[3], C[3], D[3];
    shaftWorld(f, as, ay, ap, A[0], A[1], A[2]);
    shaftWorld(f, bs, by, bp, B[0], B[1], B[2]);
    shaftWorld(f, cs, cy, cp, C[0], C[1], C[2]);
    shaftWorld(f, ds, dy, dp, D[0], D[1], D[2]);
    const unsigned int ca = shaftShade(b0, A[0], A[1], A[2], fog);
    const unsigned int cb = shaftShade(b0, B[0], B[1], B[2], fog);
    const unsigned int cc = shaftShade(b0, C[0], C[1], C[2], fog);
    const unsigned int cd = shaftShade(b0, D[0], D[1], D[2], fog);
    buf[i++] = { ua, va, ca, A[0], A[1], A[2] };
    buf[i++] = { ub, vb, cb, B[0], B[1], B[2] };
    buf[i++] = { uc, vc, cc, C[0], C[1], C[2] };
    buf[i++] = { ua, va, ca, A[0], A[1], A[2] };
    buf[i++] = { uc, vc, cc, C[0], C[1], C[2] };
    buf[i++] = { ud, vd, cd, D[0], D[1], D[2] };
#ifdef SHAFT_HOST_CHECK
    shaftHostCheck(f, face, &buf[i - 6]);
#endif
}

// bordes verticales: y0, nudos de la bruma (-425, 0, 425) que caigan dentro, y1
static int shaftYEdges(float y0, float y1, float* out) {
    static const float knots[3] = { -425.0f, 0.0f, 425.0f };
    int n = 0; out[n++] = y0;
    for (int j = 0; j < 3; ++j)
        if (knots[j] > y0 + 1.0f && knots[j] < y1 - 1.0f) out[n++] = knots[j];
    out[n++] = y1;
    return n;
}
// bordes en s: piezas segun distancia al jugador
static int shaftSEdges(const ShaftFrame& f, float s0, float s1, float p, float* out) {
    const int sub = shaftSubCount(s1 - s0, shaftDist(f, (s0 + s1) * 0.5f, p));
    for (int q = 0; q <= sub; ++q) out[q] = s0 + (s1 - s0) * (float)q / (float)sub;
    return sub + 1;
}

// ---- caras (todas con winding de addSolidBoxT) ----
// cara que mira al centro, en el plano p, rectangulo s0..s1 x y0..y1
static void shaftFaceFront(TexVertex* buf, int& i, const ShaftFrame& f,
                           float s0, float s1, float y0, float y1, float p,
                           unsigned int base, int face) {
    float ys[6]; const int ny = shaftYEdges(y0, y1, ys);
    float ss[6]; const int ns = shaftSEdges(f, s0, s1, p, ss);
    for (int a = 0; a + 1 < ny; ++a) for (int b = 0; b + 1 < ns; ++b) {
        const float ya = ys[a], yb = ys[a + 1], sa = ss[b], sb = ss[b + 1];
        shaftQuad(buf, i, f, sa, ya, p,  sb, ya, p,  sb, yb, p,  sa, yb, p,
                  sa / TILE, -ya / TILE,  sb / TILE, -ya / TILE,  sb / TILE, -yb / TILE,  sa / TILE, -yb / TILE,
                  base, face);
    }
}
// cara lateral en s (hi: normal +s, lo: normal -s), p0 (pared) .. p1 (resalte)
static void shaftFaceSide(TexVertex* buf, int& i, const ShaftFrame& f,
                          float s, float y0, float y1, float p0, float p1, bool hi, unsigned int base) {
    float ys[6]; const int ny = shaftYEdges(y0, y1, ys);
    for (int a = 0; a + 1 < ny; ++a) {
        const float ya = ys[a], yb = ys[a + 1];
        if (hi) shaftQuad(buf, i, f, s, ya, p1,  s, ya, p0,  s, yb, p0,  s, yb, p1,
                          p1 / TILE, -ya / TILE,  p0 / TILE, -ya / TILE,  p0 / TILE, -yb / TILE,  p1 / TILE, -yb / TILE,
                          base, SK_SIDE_HI);
        else    shaftQuad(buf, i, f, s, ya, p0,  s, ya, p1,  s, yb, p1,  s, yb, p0,
                          p0 / TILE, -ya / TILE,  p1 / TILE, -ya / TILE,  p1 / TILE, -yb / TILE,  p0 / TILE, -yb / TILE,
                          base, SK_SIDE_LO);
    }
}
// tapa horizontal en y (top: normal +y, bottom: normal -y)
static void shaftFaceCap(TexVertex* buf, int& i, const ShaftFrame& f,
                         float s0, float s1, float y, float p0, float p1, bool top, unsigned int base) {
    float ss[6]; const int ns = shaftSEdges(f, s0, s1, p1, ss);
    for (int b = 0; b + 1 < ns; ++b) {
        const float sa = ss[b], sb = ss[b + 1];
        if (top) shaftQuad(buf, i, f, sa, y, p1,  sb, y, p1,  sb, y, p0,  sa, y, p0,
                           sa / TILE, p1 / TILE,  sb / TILE, p1 / TILE,  sb / TILE, p0 / TILE,  sa / TILE, p0 / TILE,
                           base, SK_TOP);
        else     shaftQuad(buf, i, f, sa, y, p0,  sb, y, p0,  sb, y, p1,  sa, y, p1,
                           sa / TILE, p0 / TILE,  sb / TILE, p0 / TILE,  sb / TILE, p1 / TILE,  sa / TILE, p1 / TILE,
                           base, SK_BOTTOM);
    }
}

// caja pegada a la pared (sin cara trasera): mascara de caras a emitir
enum { SB_FRONT = 1, SB_LO = 2, SB_HI = 4, SB_TOP = 8, SB_BOT = 16, SB_ALL = 31 };
static void shaftBox(TexVertex* buf, int& i, const ShaftFrame& f,
                     float s0, float s1, float y0, float y1, float p0, float p1,
                     unsigned int base, unsigned int faces) {
    if (faces & SB_FRONT) shaftFaceFront(buf, i, f, s0, s1, y0, y1, p1, base, SK_FRONT);
    if (faces & SB_LO)    shaftFaceSide(buf, i, f, s0, y0, y1, p0, p1, false, base);
    if (faces & SB_HI)    shaftFaceSide(buf, i, f, s1, y0, y1, p0, p1, true, base);
    if (faces & SB_TOP)   shaftFaceCap(buf, i, f, s0, s1, y1, p0, p1, true, base);
    if (faces & SB_BOT)   shaftFaceCap(buf, i, f, s0, s1, y0, p0, p1, false, base);
}
// caja cuya tapa visible depende de si esta por encima o por debajo del ojo (y=0)
static unsigned int shaftCapFor(float y0, float y1) {
    if (y1 <= 0.0f) return (unsigned int)SB_TOP;   // debajo del ojo: se ve el techo
    if (y0 >= 0.0f) return (unsigned int)SB_BOT;   // encima del ojo: se ve la base
    return 0u;                                     // cruza y=0: ninguna tapa (caja alta)
}

// ---------------- layout ----------------
// Cornisas-viga: {y, alto, resalte}. -441 remata los contrafuertes por debajo
// (terminan en -425) y +425 los remata por arriba.
static const float kShaftGirder[4][3] = {
    { -441.0f, 16.0f, 18.0f },
    { -120.0f, 12.0f, 14.0f },
    {  150.0f, 12.0f, 14.0f },
    {  425.0f, 16.0f, 18.0f },
};
// zonas entre cornisas para las bahias (y0, y1)
static const float kShaftZone[4][2] = {
    { SHAFT_YBOT, -441.0f },
    { -425.0f,    -120.0f },
    { -108.0f,     150.0f },
    {  162.0f,     425.0f },
};
static const float SHAFT_RIB_Y0 = -425.0f, SHAFT_RIB_Y1 = 425.0f; // contrafuertes: 850u de alto
static const float SHAFT_RIB_HW = 4.0f, SHAFT_RIB_P = 10.0f;      // ancho 8, resalte 10
static const float SHAFT_PIPE_HW = 3.5f, SHAFT_PIPE_P0 = 16.0f, SHAFT_PIPE_P1 = 23.0f; // pasan por delante de las cornisas

// celdas (pared, hueco, zona) ocupadas por porton/maquinaria/tuberia vertical
static bool shaftBayBlocked(int k, int g, int z) {
    if (k == 0 && g == 1 && z == 2) return true;  // porton detras de la repisa
    if (k == 0 && g == 2)           return true;  // tuberia vertical
    if (k == 2 && g == 0)           return true;  // maquinaria + tuberia
    if (k == 3 && g == 1 && z == 1) return true;  // porton
    if (k == 4 && g == 1)           return true;  // tuberia vertical
    if (k == 5 && g == 2)           return true;  // tuberia vertical
    if (k == 6 && g == 1 && z == 2) return true;  // ventilacion
    if (k == 6 && g == 2 && z == 1) return true;  // maquinaria
    return false;
}

// tuberia vertical (frente + 2 lados) con un collar (anillo) opcional
static void shaftPipeV(TexVertex* buf, int& i, const ShaftFrame& f, float s, float y0, float y1,
                       float p0, float p1, unsigned int col) {
    shaftBox(buf, i, f, s - SHAFT_PIPE_HW, s + SHAFT_PIPE_HW, y0, y1, p0, p1, col, SB_FRONT | SB_LO | SB_HI);
}
static void shaftCollarV(TexVertex* buf, int& i, const ShaftFrame& f, float s, float y, float p0, float p1, unsigned int col) {
    shaftBox(buf, i, f, s - SHAFT_PIPE_HW - 2.0f, s + SHAFT_PIPE_HW + 2.0f, y - 4.0f, y + 4.0f, p0 - 1.0f, p1 + 2.0f,
             col, SB_FRONT | SB_LO | SB_HI | shaftCapFor(y - 4.0f, y + 4.0f));
}
// tuberia horizontal a lo largo de toda la pared (frente + tapa visible) con collar central
static void shaftPipeH(TexVertex* buf, int& i, const ShaftFrame& f, float y, unsigned int col) {
    const float hL = SHAFT_SIDE * 0.5f;
    shaftBox(buf, i, f, -hL, hL, y - SHAFT_PIPE_HW, y + SHAFT_PIPE_HW, SHAFT_PIPE_P0, SHAFT_PIPE_P1, col,
             SB_FRONT | shaftCapFor(y - SHAFT_PIPE_HW, y + SHAFT_PIPE_HW));
    shaftBox(buf, i, f, -5.0f, 5.0f, y - SHAFT_PIPE_HW - 2.0f, y + SHAFT_PIPE_HW + 2.0f, SHAFT_PIPE_P0 - 1.0f, SHAFT_PIPE_P1 + 2.0f,
             col, SB_FRONT | SB_LO | SB_HI | shaftCapFor(y - SHAFT_PIPE_HW - 2.0f, y + SHAFT_PIPE_HW + 2.0f));
}
// lampara ambar: quad pequeno pegado a una superficie (p = resalte de esa superficie + 0.4)
static void shaftLamp(TexVertex* buf, int& i, const ShaftFrame& f, float s, float y, float p, unsigned int col) {
    shaftQuad(buf, i, f, s - 2.0f, y - 3.0f, p,  s + 2.0f, y - 3.0f, p,  s + 2.0f, y + 3.0f, p,  s - 2.0f, y + 3.0f, p,
              0, 1, 1, 1, 1, 0, 0, 0, col, SK_LAMP);
}
// porton/ventilacion: rectangulo casi negro apenas por delante del panel + dintel
static void shaftGate(TexVertex* buf, int& i, const ShaftFrame& f, float hw, float y0, float y1,
                      unsigned int gateCol, unsigned int lintelCol) {
    shaftFaceFront(buf, i, f, -hw, hw, y0, y1, 0.8f, gateCol, SK_FLAT);
    shaftBox(buf, i, f, -hw - 10.0f, hw + 10.0f, y1, y1 + 12.0f, 0.0f, 12.0f, lintelCol,
             SB_FRONT | shaftCapFor(y1, y1 + 12.0f));
}

// ================= EL POZO =================
static void buildShaftWalls(TexVertex* buf, int& i) {
    const unsigned int wall   = brighten(RGBA(70, 76, 88, 255), 1.8f);  // acero-piedra frio (126,136,158)
    const unsigned int rib    = brighten(wall, 1.04f);
    const unsigned int girder = brighten(wall, 0.94f);
    const unsigned int pipe   = brighten(RGBA(60, 58, 66, 255), 1.5f);  // metal mas oscuro, leve violeta
    const unsigned int mach   = brighten(RGBA(64, 66, 72, 255), 1.6f);
    const unsigned int recess = RGBA(30, 34, 42, 255);                   // bahia rehundida
    const unsigned int gate   = RGBA(16, 18, 24, 255);                   // porton: casi negro
    const unsigned int lamp   = RGBA(235, 170, 80, 255);                 // ambar calido
    const float L = SHAFT_SIDE, hL = L * 0.5f;
    const float gapS[3] = { -L / 3.0f, 0.0f, L / 3.0f };                 // centros de los 3 huecos entre contrafuertes
    const float ribS[2] = { -L / 6.0f, L / 6.0f };                       // contrafuertes interiores (+ pilastra de esquina)

    for (int k = 0; k < 8; ++k) {
        const ShaftFrame f = shaftWall(k);

        // 1) PANEL colosal de la pared (mira al centro), 4 bandas de bruma
        shaftFaceFront(buf, i, f, -hL, hL, SHAFT_YBOT, SHAFT_YTOP, 0.0f, wall, SK_PANEL);

        // 2) CONTRAFUERTES interiores: 850u de alto, 8 de ancho, resalte 10
        for (int r = 0; r < 2; ++r)
            shaftBox(buf, i, f, ribS[r] - SHAFT_RIB_HW, ribS[r] + SHAFT_RIB_HW, SHAFT_RIB_Y0, SHAFT_RIB_Y1,
                     0.0f, SHAFT_RIB_P, rib, SB_FRONT | SB_LO | SB_HI);

        // 3) CORNISAS-VIGA a 4 alturas (frente + la tapa que se ve desde y=0)
        for (int c = 0; c < 4; ++c) {
            const float y = kShaftGirder[c][0], h = kShaftGirder[c][1], p = kShaftGirder[c][2];
            shaftBox(buf, i, f, -hL, hL, y, y + h, 0.0f, p, girder, SB_FRONT | shaftCapFor(y, y + h));
        }

        // 4) BAHIAS rehundidas (oscuras) en huecos x zonas, eleccion por hash
        for (int g = 0; g < 3; ++g) for (int z = 0; z < 4; ++z) {
            if (shaftBayBlocked(k, g, z)) continue;
            const unsigned int h = shaftHash((unsigned int)(k * 131 + g * 17 + z * 7 + 977));
            if ((h % 100u) >= 55u) continue;
            const float m = 20.0f;
            const float y0 = kShaftZone[z][0] + m, y1 = kShaftZone[z][1] - m;
            const float hw = ((h >> 8) & 1u) ? 13.0f : 10.0f;
            shaftFaceFront(buf, i, f, gapS[g] - hw, gapS[g] + hw, y0, y1, 0.6f, recess, SK_FLAT);
            if (((h >> 12) % 10u) < 3u)  // alfeizar/viga bajo algunas bahias
                shaftBox(buf, i, f, gapS[g] - hw - 3.0f, gapS[g] + hw + 3.0f, y0 - 5.0f, y0, 0.0f, 6.0f,
                         girder, SB_FRONT | shaftCapFor(y0 - 5.0f, y0));
        }
    }

    // 5) PILASTRAS DE ESQUINA: caja en el marco del vertice (22.5 grados), resalte 26
    for (int k = 0; k < 8; ++k) {
        const ShaftFrame fc = shaftFrame(45.0f * (float)k + 22.5f, SHAFT_RIN / 0.92387953f); // vertice a 173.2
        shaftBox(buf, i, fc, -9.0f, 9.0f, SHAFT_RIB_Y0, SHAFT_RIB_Y1, 0.0f, 26.0f, rib, SB_FRONT | SB_LO | SB_HI);
    }

    // 6) TUBERIAS colosales
    {   // horizontales: anillo alto (y=+236) por k=1..3, anillo bajo (y=-293) por k=5..7
        for (int k = 1; k <= 3; ++k) shaftPipeH(buf, i, shaftWall(k),  236.0f, pipe);
        for (int k = 5; k <= 7; ++k) shaftPipeH(buf, i, shaftWall(k), -293.0f, pipe);
        // verticales completas (-700..500) con 2 collares
        const int   vk[3] = { 0, 4, 5 };
        const float vs[3] = { L / 3.0f, 0.0f, L / 3.0f };
        for (int q = 0; q < 3; ++q) {
            const ShaftFrame f = shaftWall(vk[q]);
            shaftPipeV(buf, i, f, vs[q], SHAFT_YBOT, SHAFT_YTOP, SHAFT_PIPE_P0, SHAFT_PIPE_P1, pipe);
            shaftCollarV(buf, i, f, vs[q], -300.0f, SHAFT_PIPE_P0, SHAFT_PIPE_P1, pipe);
            shaftCollarV(buf, i, f, vs[q],  100.0f, SHAFT_PIPE_P0, SHAFT_PIPE_P1, pipe);
        }
    }

    // 7) MAQUINARIA: bloque en +X (k=2, hueco 0) alimentado por una tuberia vertical
    {
        const ShaftFrame f = shaftWall(2);
        const float s = -L / 3.0f;
        shaftBox(buf, i, f, s - 16.0f, s + 16.0f, -40.0f, 50.0f, 0.0f, 28.0f, mach, SB_ALL);
        shaftBox(buf, i, f, s - 10.0f, s + 10.0f,  50.0f, 92.0f, 0.0f, 18.0f, mach, SB_FRONT | SB_LO | SB_HI | SB_TOP);
        shaftPipeV(buf, i, f, s, 92.0f, SHAFT_YTOP, 10.0f, 17.0f, pipe);          // sube desde el bloque
        shaftPipeV(buf, i, f, s, SHAFT_YBOT, -40.0f, 10.0f, 17.0f, pipe);         // baja al abismo
        shaftCollarV(buf, i, f, s, -260.0f, 10.0f, 17.0f, pipe);
        shaftLamp(buf, i, f, s, 20.0f, 28.4f, lamp);
    }
    // maquinaria en -X (k=6, hueco 2), bajo el ojo
    {
        const ShaftFrame f = shaftWall(6);
        const float s = L / 3.0f;
        shaftBox(buf, i, f, s - 16.0f, s + 16.0f, -250.0f, -170.0f, 0.0f, 24.0f, mach, SB_FRONT | SB_LO | SB_HI | SB_TOP);
        shaftBox(buf, i, f, s - 10.0f, s + 10.0f, -170.0f, -135.0f, 0.0f, 16.0f, mach, SB_FRONT | SB_LO | SB_HI | SB_TOP);
        shaftLamp(buf, i, f, s, -210.0f, 24.4f, lamp);
    }

    // 8) PORTONES / VENTILACION (3 aberturas colosales oscuras)
    {   // +Z: gran porton detras de la repisa del jugador, enmarcado por los contrafuertes de +-22
        const ShaftFrame f = shaftWall(0);
        shaftGate(buf, i, f, 17.0f, -40.0f, 128.0f, gate, girder);
        shaftLamp(buf, i, f, -L / 6.0f, 64.0f, SHAFT_RIB_P + 0.4f, lamp);   // sobre los contrafuertes que lo flanquean
        shaftLamp(buf, i, f,  L / 6.0f, 64.0f, SHAFT_RIB_P + 0.4f, lamp);
    }
    {   // k=3: porton hundido bajo el ojo
        const ShaftFrame f = shaftWall(3);
        shaftGate(buf, i, f, 16.0f, -400.0f, -140.0f, gate, girder);
        shaftLamp(buf, i, f, 0.0f, -134.0f, 12.4f, lamp);                    // sobre el dintel
    }
    {   // k=6: ventilacion con rejilla de lamas horizontales
        const ShaftFrame f = shaftWall(6);
        shaftGate(buf, i, f, 16.0f, -80.0f, 110.0f, gate, girder);
        for (int q = 0; q < 6; ++q) {
            const float y = -70.0f + 30.0f * (float)q;
            shaftFaceFront(buf, i, f, -16.0f, 16.0f, y, y + 7.0f, 1.6f, wall, SK_FRONT);
        }
        shaftLamp(buf, i, f, -L / 6.0f, 40.0f, SHAFT_RIB_P + 0.4f, lamp);
    }
}
