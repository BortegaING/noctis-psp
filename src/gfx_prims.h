#pragma once
// =============================================================================
// gfx_prims.h -- primitivas de DETALLE (el nivel del Serafin) para todo el juego.
//
// Benjamin (2026-10-16): "usa los bordes o resolucion del serafin, esta muy bien detallado,
// usa ese mismo tipo de detalles dentro de todo el juego, se sigue viendo todo muy cuadrado".
// Lo que hace que el Serafin se vea fino es: curvas con MUCHOS lados (su ojo es una esfera de
// 7x12), sombreado por normal y degradados de color, y bordes que atrapan la luz (la nariz
// dorada de cada peldano). Aqui esta eso empaquetado:
//
//   gpBevelBox   caja con las aristas BISELADAS (90 v): el borde se ilumina distinto que la
//                cara y la caja deja de leerse como un cubo plano
//   gpArchSlab   losa con tope redondo u ojival (lapidas, nichos, puertas)  12*seg+30 v
//   gpDome       media esfera (montículos, cupulas, copas)                  stacks*slices*6 v
//   gpTube       cilindro conico SIN tapas (ramas, columnas)                sides*6 v
//   gpPrism      prisma/tronco de n caras PLANAS (obeliscos, postes)        sides*6 (+sides*3 con tapa)
//   gpRingY/XY   aro de seccion cuadrada (coronas, cruz celta, molduras)    seg*24 v
//   gpWing       ala de plumas en 3 capas (la del Serafin, orientable)      ~(cnt)*6 + 2 tubos
//   gpTuft       mechon de pasto de 3 hojas (9 v)
//
// Convenciones: LineVertex {color,x,y,z}, sin cull (el winding no importa), luz de cp_light
// (char_prims.h: arriba, algo a la izquierda y al frente -Z). Se incluye DESPUES de
// char_prims.h. Ninguna funcion controla la capacidad: el que llama usa gpFits().
// =============================================================================

#include <math.h>

static const float GP_PI = 3.14159265f, GP_TAU = 6.2831853f;

static inline int gpFits(int i, int need, int cap) { return i + need <= cap; }
static inline unsigned int gpShade(unsigned int c, float nx, float ny, float nz) {
    const float l = sqrtf(nx * nx + ny * ny + nz * nz) + 1e-6f;
    return cp_scale(c, cp_light(nx / l, ny / l, nz / l));
}
// quad con un color por esquina (2 triangulos)
static inline void gpQuad(LineVertex *b, int &i,
                          float ax, float ay, float az, unsigned int ca, float bx, float by, float bz, unsigned int cb,
                          float cx, float cy, float cz, unsigned int cc, float dx, float dy, float dz, unsigned int cd) {
    b[i++] = { ca, ax, ay, az }; b[i++] = { cb, bx, by, bz }; b[i++] = { cc, cx, cy, cz };
    b[i++] = { ca, ax, ay, az }; b[i++] = { cc, cx, cy, cz }; b[i++] = { cd, dx, dy, dz };
}
static inline void gpTri(LineVertex *b, int &i, float ax, float ay, float az, unsigned int ca,
                         float bx, float by, float bz, unsigned int cb, float cx, float cy, float cz, unsigned int cc) {
    b[i++] = { ca, ax, ay, az }; b[i++] = { cb, bx, by, bz }; b[i++] = { cc, cx, cy, cz };
}
// color de una altura dentro de un degradado vertical (musgo abajo, piedra limpia arriba)
static inline unsigned int gpGrad(unsigned int cBot, unsigned int cTop, float y, float y0, float y1) {
    return cp_lerp(cBot, cTop, (y1 - y0) > 1e-5f ? (y - y0) / (y1 - y0) : 1.0f);
}

// -----------------------------------------------------------------------------
// gpBevelBox: caja apoyada en y0 con las 4 aristas verticales y las 4 de arriba BISELADAS
// (bv = ancho del bisel). La base no se bisela: se apoya en algo. 90 vertices.
// -----------------------------------------------------------------------------
static void gpBevelBox(LineVertex *b, int &i, float cx, float y0, float cz, float w, float d, float h, float bv,
                       unsigned int cTop, unsigned int cBot) {
    if (bv > w * 0.45f) bv = w * 0.45f; if (bv > d * 0.45f) bv = d * 0.45f; if (bv > h * 0.45f) bv = h * 0.45f;
    const float x0 = cx - w * 0.5f, x1 = cx + w * 0.5f, z0 = cz - d * 0.5f, z1 = cz + d * 0.5f, y1 = y0 + h;
    const float a0 = x0 + bv, a1 = x1 - bv, c0 = z0 + bv, c1 = z1 - bv, yb = y1 - bv;
    const unsigned int g0 = cBot, g1 = gpGrad(cBot, cTop, yb, y0, y1);
    const float R = 0.7071f, Q = 0.5774f;
    // tapa
    { const unsigned int c = gpShade(cTop, 0, 1, 0); gpQuad(b, i, a0, y1, c0, c, a1, y1, c0, c, a1, y1, c1, c, a0, y1, c1, c); }
    // 4 caras (de y0 a yb)
    gpQuad(b, i, a0, y0, z0, gpShade(g0, 0, 0, -1), a1, y0, z0, gpShade(g0, 0, 0, -1), a1, yb, z0, gpShade(g1, 0, 0, -1), a0, yb, z0, gpShade(g1, 0, 0, -1));
    gpQuad(b, i, a0, y0, z1, gpShade(g0, 0, 0, 1), a1, y0, z1, gpShade(g0, 0, 0, 1), a1, yb, z1, gpShade(g1, 0, 0, 1), a0, yb, z1, gpShade(g1, 0, 0, 1));
    gpQuad(b, i, x0, y0, c0, gpShade(g0, -1, 0, 0), x0, y0, c1, gpShade(g0, -1, 0, 0), x0, yb, c1, gpShade(g1, -1, 0, 0), x0, yb, c0, gpShade(g1, -1, 0, 0));
    gpQuad(b, i, x1, y0, c0, gpShade(g0, 1, 0, 0), x1, y0, c1, gpShade(g0, 1, 0, 0), x1, yb, c1, gpShade(g1, 1, 0, 0), x1, yb, c0, gpShade(g1, 1, 0, 0));
    // 4 biseles verticales
    gpQuad(b, i, a0, y0, z0, gpShade(g0, -R, 0, -R), x0, y0, c0, gpShade(g0, -R, 0, -R), x0, yb, c0, gpShade(g1, -R, 0, -R), a0, yb, z0, gpShade(g1, -R, 0, -R));
    gpQuad(b, i, a1, y0, z0, gpShade(g0, R, 0, -R), x1, y0, c0, gpShade(g0, R, 0, -R), x1, yb, c0, gpShade(g1, R, 0, -R), a1, yb, z0, gpShade(g1, R, 0, -R));
    gpQuad(b, i, a0, y0, z1, gpShade(g0, -R, 0, R), x0, y0, c1, gpShade(g0, -R, 0, R), x0, yb, c1, gpShade(g1, -R, 0, R), a0, yb, z1, gpShade(g1, -R, 0, R));
    gpQuad(b, i, a1, y0, z1, gpShade(g0, R, 0, R), x1, y0, c1, gpShade(g0, R, 0, R), x1, yb, c1, gpShade(g1, R, 0, R), a1, yb, z1, gpShade(g1, R, 0, R));
    // 4 biseles de arriba (de la cara a la tapa)
    { const unsigned int c = gpShade(cTop, 0, R, -R), e = gpShade(g1, 0, R, -R); gpQuad(b, i, a0, yb, z0, e, a1, yb, z0, e, a1, y1, c0, c, a0, y1, c0, c); }
    { const unsigned int c = gpShade(cTop, 0, R, R),  e = gpShade(g1, 0, R, R);  gpQuad(b, i, a0, yb, z1, e, a1, yb, z1, e, a1, y1, c1, c, a0, y1, c1, c); }
    { const unsigned int c = gpShade(cTop, -R, R, 0), e = gpShade(g1, -R, R, 0); gpQuad(b, i, x0, yb, c0, e, x0, yb, c1, e, a0, y1, c1, c, a0, y1, c0, c); }
    { const unsigned int c = gpShade(cTop, R, R, 0),  e = gpShade(g1, R, R, 0);  gpQuad(b, i, x1, yb, c0, e, x1, yb, c1, e, a1, y1, c1, c, a1, y1, c0, c); }
    // 4 esquinas
    gpTri(b, i, a0, yb, z0, gpShade(g1, -Q, Q, -Q), x0, yb, c0, gpShade(g1, -Q, Q, -Q), a0, y1, c0, gpShade(cTop, -Q, Q, -Q));
    gpTri(b, i, a1, yb, z0, gpShade(g1, Q, Q, -Q),  x1, yb, c0, gpShade(g1, Q, Q, -Q),  a1, y1, c0, gpShade(cTop, Q, Q, -Q));
    gpTri(b, i, a0, yb, z1, gpShade(g1, -Q, Q, Q),  x0, yb, c1, gpShade(g1, -Q, Q, Q),  a0, y1, c1, gpShade(cTop, -Q, Q, Q));
    gpTri(b, i, a1, yb, z1, gpShade(g1, Q, Q, Q),   x1, yb, c1, gpShade(g1, Q, Q, Q),   a1, y1, c1, gpShade(cTop, Q, Q, Q));
}

// gpBox: la misma caja SIN biselar (30 v, sin la base): para las versiones de lejos, donde el bisel
// ya no se distingue y cuesta el triple.
static void gpBox(LineVertex *b, int &i, float cx, float y0, float cz, float w, float d, float h,
                  unsigned int cTop, unsigned int cBot) {
    const float x0 = cx - w * 0.5f, x1 = cx + w * 0.5f, z0 = cz - d * 0.5f, z1 = cz + d * 0.5f, y1 = y0 + h;
    const unsigned int t = gpShade(cTop, 0, 1, 0);
    gpQuad(b, i, x0, y1, z0, t, x1, y1, z0, t, x1, y1, z1, t, x0, y1, z1, t);
    gpQuad(b, i, x0, y0, z0, gpShade(cBot, 0, 0, -1), x1, y0, z0, gpShade(cBot, 0, 0, -1), x1, y1, z0, gpShade(cTop, 0, 0, -1), x0, y1, z0, gpShade(cTop, 0, 0, -1));
    gpQuad(b, i, x0, y0, z1, gpShade(cBot, 0, 0, 1), x1, y0, z1, gpShade(cBot, 0, 0, 1), x1, y1, z1, gpShade(cTop, 0, 0, 1), x0, y1, z1, gpShade(cTop, 0, 0, 1));
    gpQuad(b, i, x0, y0, z0, gpShade(cBot, -1, 0, 0), x0, y0, z1, gpShade(cBot, -1, 0, 0), x0, y1, z1, gpShade(cTop, -1, 0, 0), x0, y1, z0, gpShade(cTop, -1, 0, 0));
    gpQuad(b, i, x1, y0, z0, gpShade(cBot, 1, 0, 0), x1, y0, z1, gpShade(cBot, 1, 0, 0), x1, y1, z1, gpShade(cTop, 1, 0, 0), x1, y1, z0, gpShade(cTop, 1, 0, 0));
}

// -----------------------------------------------------------------------------
// gpArchSlab: losa vertical (cara al frente = -Z) de ancho w, alto total h y grosor t, apoyada
// en y0, con el tope REDONDO (pointed = 0) u OJIVAL (pointed = 1, arco gotico). seg = lados del
// arco (8..14). Degradado cBot -> cTop en la altura. Vertices: 12*seg + 30.
// -----------------------------------------------------------------------------
static void gpArchSlab(LineVertex *b, int &i, float cx, float y0, float cz, float w, float h, float t, int seg,
                       int pointed, unsigned int cTop, unsigned int cBot) {
    if (seg < 2) seg = 2; if (seg > 24) seg = 24;
    const float r = w * 0.5f;
    float px[32], py[32]; int n = 0;
    px[n] = -r; py[n++] = 0.0f;
    px[n] = r;  py[n++] = 0.0f;
    if (!pointed) {
        const float hs = h - r;
        for (int k = 0; k <= seg; ++k) { const float a = GP_PI * k / seg; px[n] = r * cosf(a); py[n++] = hs + r * sinf(a); }
    } else {
        // dos arcos de radio R = 1.4w con centros corridos: se juntan en una punta en x = 0
        const float R = w * 1.4f, ca = (R - r) / R, amax = acosf(ca);
        const float apex = R * sinf(amax), hs = h - apex;
        const int hseg = seg / 2;
        for (int k = 0; k <= hseg; ++k) { const float a = amax * k / hseg; px[n] = r - R + R * cosf(a); py[n++] = hs + R * sinf(a); }
        for (int k = hseg - 1; k >= 0; --k) { const float a = amax * k / hseg; px[n] = -(r - R + R * cosf(a)); py[n++] = hs + R * sinf(a); }
    }
    const float zf = cz - t * 0.5f, zb = cz + t * 0.5f, ym = h * 0.45f;
    for (int k = 0; k < n; ++k) {                                   // caras de adelante y atras (abanico)
        const int j = (k + 1) % n;
        const unsigned int cm = gpGrad(cBot, cTop, ym, 0.0f, h), ck = gpGrad(cBot, cTop, py[k], 0.0f, h), cj = gpGrad(cBot, cTop, py[j], 0.0f, h);
        gpTri(b, i, cx, y0 + ym, zf, gpShade(cm, 0, 0, -1), cx + px[k], y0 + py[k], zf, gpShade(ck, 0, 0, -1), cx + px[j], y0 + py[j], zf, gpShade(cj, 0, 0, -1));
        gpTri(b, i, cx, y0 + ym, zb, gpShade(cm, 0, 0, 1),  cx + px[k], y0 + py[k], zb, gpShade(ck, 0, 0, 1),  cx + px[j], y0 + py[j], zb, gpShade(cj, 0, 0, 1));
    }
    for (int k = 1; k < n; ++k) {                                   // canto (sin la base)
        const int j = (k + 1) % n;
        float ex = px[j] - px[k], ey = py[j] - py[k]; const float l = sqrtf(ex * ex + ey * ey) + 1e-6f;
        const float nx = ey / l, ny = -ex / l;                       // normal hacia afuera (contorno antihorario)
        const unsigned int ck = gpShade(gpGrad(cBot, cTop, py[k], 0.0f, h), nx, ny, 0), cj = gpShade(gpGrad(cBot, cTop, py[j], 0.0f, h), nx, ny, 0);
        gpQuad(b, i, cx + px[k], y0 + py[k], zf, ck, cx + px[j], y0 + py[j], zf, cj, cx + px[j], y0 + py[j], zb, cj, cx + px[k], y0 + py[k], zb, ck);
    }
}

// -----------------------------------------------------------------------------
// gpDome: media esfera (solo la mitad de arriba) con degradado de la base al tope.
// -----------------------------------------------------------------------------
static void gpDome(LineVertex *b, int &i, float cx, float cy, float cz, float rx, float ry, float rz,
                   int stacks, int slices, unsigned int cTop, unsigned int cBase) {
    for (int st = 0; st < stacks; ++st) {
        const float t0 = 0.5f * GP_PI * st / stacks, t1 = 0.5f * GP_PI * (st + 1) / stacks;
        const float y0 = sinf(t0), y1 = sinf(t1), r0 = cosf(t0), r1 = cosf(t1);
        const unsigned int k0 = cp_lerp(cBase, cTop, y0), k1 = cp_lerp(cBase, cTop, y1);
        for (int sl = 0; sl < slices; ++sl) {
            const float a0 = GP_TAU * sl / slices, a1 = GP_TAU * (sl + 1) / slices;
            const float c0 = cosf(a0), s0 = sinf(a0), c1 = cosf(a1), s1 = sinf(a1);
            gpQuad(b, i, cx + rx * r0 * c0, cy + ry * y0, cz + rz * r0 * s0, gpShade(k0, r0 * c0, y0, r0 * s0),
                         cx + rx * r0 * c1, cy + ry * y0, cz + rz * r0 * s1, gpShade(k0, r0 * c1, y0, r0 * s1),
                         cx + rx * r1 * c1, cy + ry * y1, cz + rz * r1 * s1, gpShade(k1, r1 * c1, y1, r1 * s1),
                         cx + rx * r1 * c0, cy + ry * y1, cz + rz * r1 * s0, gpShade(k1, r1 * c0, y1, r1 * s0));
        }
    }
}

// -----------------------------------------------------------------------------
// gpTube: cono truncado entre dos puntos, SIN tapas (para ramas encadenadas y columnas que
// tocan suelo y techo). sides*6 vertices. Sombreado suave por normal.
// -----------------------------------------------------------------------------
static void gpTube(LineVertex *b, int &i, float x0, float y0, float z0, float x1, float y1, float z1,
                   float r0, float r1, int sides, unsigned int cB, unsigned int cT) {
    if (sides < 3) sides = 3;
    float dx = x1 - x0, dy = y1 - y0, dz = z1 - z0; const float len = sqrtf(dx * dx + dy * dy + dz * dz); if (len < 1e-5f) return;
    dx /= len; dy /= len; dz /= len;
    float ux = 0, uy = 1, uz = 0; if (fabsf(dy) > 0.9f) { ux = 1; uy = 0; }
    float ax = uy * dz - uz * dy, ay = uz * dx - ux * dz, az = ux * dy - uy * dx; const float al = sqrtf(ax * ax + ay * ay + az * az);
    ax /= al; ay /= al; az /= al;
    const float bx = dy * az - dz * ay, by = dz * ax - dx * az, bz = dx * ay - dy * ax;
    for (int k = 0; k < sides; ++k) {
        const float a0 = GP_TAU * k / sides, a1 = GP_TAU * (k + 1) / sides;
        const float c0 = cosf(a0), s0 = sinf(a0), c1 = cosf(a1), s1 = sinf(a1);
        const float n0x = ax * c0 + bx * s0, n0y = ay * c0 + by * s0, n0z = az * c0 + bz * s0;
        const float n1x = ax * c1 + bx * s1, n1y = ay * c1 + by * s1, n1z = az * c1 + bz * s1;
        gpQuad(b, i, x0 + r0 * n0x, y0 + r0 * n0y, z0 + r0 * n0z, gpShade(cB, n0x, n0y, n0z),
                     x0 + r0 * n1x, y0 + r0 * n1y, z0 + r0 * n1z, gpShade(cB, n1x, n1y, n1z),
                     x1 + r1 * n1x, y1 + r1 * n1y, z1 + r1 * n1z, gpShade(cT, n1x, n1y, n1z),
                     x1 + r1 * n0x, y1 + r1 * n0y, z1 + r1 * n0z, gpShade(cT, n0x, n0y, n0z));
    }
}

// -----------------------------------------------------------------------------
// gpPrism: tronco de piramide de n caras PLANAS (cada cara con su normal: aristas nitidas),
// de y0 a y0+h, radio r0 abajo y r1 arriba, girado rot. cap = tapa de arriba.
// -----------------------------------------------------------------------------
static void gpPrism(LineVertex *b, int &i, float cx, float y0, float cz, float r0, float r1, float h, int sides,
                    float rot, unsigned int cBot, unsigned int cTop, int cap) {
    const float y1 = y0 + h, slope = (r0 - r1) / (h > 1e-5f ? h : 1.0f);
    for (int k = 0; k < sides; ++k) {
        const float a0 = rot + GP_TAU * k / sides, a1 = rot + GP_TAU * (k + 1) / sides, am = (a0 + a1) * 0.5f;
        const float nx = cosf(am), nz = sinf(am), ny = slope;
        gpQuad(b, i, cx + r0 * cosf(a0), y0, cz + r0 * sinf(a0), gpShade(cBot, nx, ny, nz),
                     cx + r0 * cosf(a1), y0, cz + r0 * sinf(a1), gpShade(cBot, nx, ny, nz),
                     cx + r1 * cosf(a1), y1, cz + r1 * sinf(a1), gpShade(cTop, nx, ny, nz),
                     cx + r1 * cosf(a0), y1, cz + r1 * sinf(a0), gpShade(cTop, nx, ny, nz));
        if (cap && r1 > 1e-4f) {
            const unsigned int c = gpShade(cTop, 0, 1, 0);
            gpTri(b, i, cx, y1, cz, c, cx + r1 * cosf(a0), y1, cz + r1 * sinf(a0), c, cx + r1 * cosf(a1), y1, cz + r1 * sinf(a1), c);
        }
    }
}

// -----------------------------------------------------------------------------
// Aros de seccion cuadrada (molduras, coronas, la rueda de una cruz celta). 24 v por segmento.
//   gpRingY: horizontal (eje Y), a la altura y.   gpRingXY: vertical, en el plano XY (eje Z).
// -----------------------------------------------------------------------------
static void gpRingAxis(LineVertex *b, int &i, float cx, float cy, float cz, float R, float th, float dep, int seg,
                       int vertical, unsigned int col) {
    const float ri = R - th * 0.5f, ro = R + th * 0.5f, hd = dep * 0.5f;
    for (int k = 0; k < seg; ++k) {
        const float a0 = GP_TAU * k / seg, a1 = GP_TAU * (k + 1) / seg;
        const float c0 = cosf(a0), s0 = sinf(a0), c1 = cosf(a1), s1 = sinf(a1);
        // P(radio, eje, c, s): vertical -> (r c, r s, e); horizontal -> (r c, e, r s)
        #define GPR(r, e, c, s) (cx + (r) * (c)), (vertical ? cy + (r) * (s) : cy + (e)), (vertical ? cz + (e) : cz + (r) * (s))
        const float onx0 = c0, ony0 = vertical ? s0 : 0.0f, onz0 = vertical ? 0.0f : s0;
        const float onx1 = c1, ony1 = vertical ? s1 : 0.0f, onz1 = vertical ? 0.0f : s1;
        const float ay = vertical ? 0.0f : 1.0f, az = vertical ? 1.0f : 0.0f;   // el eje: la cara e = +hd lo mira
        gpQuad(b, i, GPR(ro, -hd, c0, s0), gpShade(col, onx0, ony0, onz0), GPR(ro, hd, c0, s0), gpShade(col, onx0, ony0, onz0),
                     GPR(ro, hd, c1, s1), gpShade(col, onx1, ony1, onz1), GPR(ro, -hd, c1, s1), gpShade(col, onx1, ony1, onz1));
        gpQuad(b, i, GPR(ri, -hd, c0, s0), gpShade(col, -onx0, -ony0, -onz0), GPR(ri, hd, c0, s0), gpShade(col, -onx0, -ony0, -onz0),
                     GPR(ri, hd, c1, s1), gpShade(col, -onx1, -ony1, -onz1), GPR(ri, -hd, c1, s1), gpShade(col, -onx1, -ony1, -onz1));
        const unsigned int cp = gpShade(col, 0.0f, ay, az), cn = gpShade(col, 0.0f, -ay, -az);
        gpQuad(b, i, GPR(ri, hd, c0, s0), cp, GPR(ro, hd, c0, s0), cp, GPR(ro, hd, c1, s1), cp, GPR(ri, hd, c1, s1), cp);
        gpQuad(b, i, GPR(ri, -hd, c0, s0), cn, GPR(ro, -hd, c0, s0), cn, GPR(ro, -hd, c1, s1), cn, GPR(ri, -hd, c1, s1), cn);
        #undef GPR
    }
}
static inline void gpRingY(LineVertex *b, int &i, float cx, float y, float cz, float R, float th, float dep, int seg, unsigned int col) {
    gpRingAxis(b, i, cx, y, cz, R, th, dep, seg, 0, col);
}
static inline void gpRingXY(LineVertex *b, int &i, float cx, float cy, float cz, float R, float th, float dep, int seg, unsigned int col) {
    gpRingAxis(b, i, cx, cy, cz, R, th, dep, seg, 1, col);
}

// -----------------------------------------------------------------------------
// gpWing: el ala de plumas del Serafin (3 capas abanicadas), orientable. Se construye en su
// plano local (X = a lo largo de la espina, Y = las plumas cuelgan hacia -Y, Z = grosor) y se
// lleva al mundo con la base (ex, ey, ez) y el origen o. s = escala (el ala del Serafin mide
// ~8.6 de espina; 0.3 da un ala de angel de ~2.6). cA/cB = plumas claras/oscuras, cTip = puntas.
// fold: 0 = abierta como la del Serafin (las plumas de la punta siguen a la espina); 1 = PLEGADA
// (todas cuelgan hacia -Y, como el ala de una estatua). fl = largo de las plumas respecto de la
// espina (1 = el Serafin; una estatua plegada tiene espina corta y plumas largas: 2..2.5).
// Vertices: 72 + (14+12+10)*6 = 288.
// -----------------------------------------------------------------------------
struct GpBasis { float o[3], ex[3], ey[3], ez[3]; };
static inline void gpXf(const GpBasis &B, float lx, float ly, float lz, float &x, float &y, float &z) {
    x = B.o[0] + B.ex[0] * lx + B.ey[0] * ly + B.ez[0] * lz;
    y = B.o[1] + B.ex[1] * lx + B.ey[1] * ly + B.ez[1] * lz;
    z = B.o[2] + B.ex[2] * lx + B.ey[2] * ly + B.ez[2] * lz;
}
static void gpWing(LineVertex *b, int &i, const GpBasis &B, float s, unsigned int cA, unsigned int cB, unsigned int cTip,
                   float fold = 0.0f, float fl = 1.0f, int step = 1) {   // step 2 = la mitad de las plumas (de lejos)
    const float px[3] = { 0.0f, 4.2f, 8.6f }, py[3] = { 0.0f, 1.3f, 2.6f };
    for (int k = 0; k < 2; ++k) {                                   // espina (hueso del ala)
        float x0, y0, z0, x1, y1, z1;
        gpXf(B, px[k] * s, py[k] * s, 0, x0, y0, z0); gpXf(B, px[k + 1] * s, py[k + 1] * s, 0, x1, y1, z1);
        gpTube(b, i, x0, y0, z0, x1, y1, z1, (k ? 0.24f : 0.32f) * s, (k ? 0.08f : 0.24f) * s, 6, cA, cA);
    }
    // normal del plano del ala (para sombrear las plumas)
    const float nx = B.ez[0], ny = B.ez[1], nz = B.ez[2];
    const int cnt[3] = { 14, 12, 10 };
    const float len0[3] = { 6.2f, 4.6f, 3.0f }, wid[3] = { 0.55f, 0.48f, 0.40f }, zOff[3] = { -0.10f, 0.0f, 0.10f };
    const float t0[3] = { 0.04f, 0.10f, 0.18f }, t1[3] = { 0.98f, 0.92f, 0.80f };
    for (int r = 0; r < 3; ++r)
        for (int n = 0; n < cnt[r]; n += step) {
            const float u = (float)n / (float)(cnt[r] - 1), t = t0[r] + (t1[r] - t0[r]) * u;
            float bx, by;
            if (t < 0.5f) { const float q = t / 0.5f; bx = px[0] + (px[1] - px[0]) * q; by = py[0] + (py[1] - py[0]) * q; }
            else { const float q = (t - 0.5f) / 0.5f; bx = px[1] + (px[2] - px[1]) * q; by = py[1] + (py[2] - py[1]) * q; }
            const float ang = -(1.45f - 1.25f * u * (1.0f - fold)), L = (len0[r] * (1.0f - 0.55f * u * (1.0f - 0.5f * fold)) + 0.6f) * fl;
            const float dx = cosf(ang), dy = sinf(ang), tx = bx + dx * L, ty = by + dy * L;
            const float mx = bx + dx * L * 0.38f, my = by + dy * L * 0.38f, qx = -dy, qy = dx;
            const float w = wid[r] * (1.0f - 0.35f * u) * (fl > 1.0f ? 0.4f + 0.6f * fl : 1.0f), z = zOff[r];
            const unsigned int base = cp_scale((n & 1) ? cB : cA, 0.86f + 0.10f * r);
            const unsigned int tip = (r == 0 && (n % 4) == 3) ? cTip : cp_scale(cA, 1.04f);
            float X[4], Y[4], Z[4];
            gpXf(B, bx * s, by * s, z * s, X[0], Y[0], Z[0]);
            gpXf(B, (mx + qx * w) * s, (my + qy * w) * s, z * s, X[1], Y[1], Z[1]);
            gpXf(B, tx * s, ty * s, z * s, X[2], Y[2], Z[2]);
            gpXf(B, (mx - qx * w) * s, (my - qy * w) * s, z * s, X[3], Y[3], Z[3]);
            const unsigned int cb = gpShade(base, nx, ny, nz), cl = gpShade(cp_scale(base, 1.06f), nx, ny, nz);
            const unsigned int cr = gpShade(cp_scale(base, 0.84f), nx, ny, nz), ct = gpShade(tip, nx, ny, nz);
            gpTri(b, i, X[0], Y[0], Z[0], cb, X[1], Y[1], Z[1], cl, X[2], Y[2], Z[2], ct);
            gpTri(b, i, X[0], Y[0], Z[0], cb, X[2], Y[2], Z[2], ct, X[3], Y[3], Z[3], cr);
        }
}

// -----------------------------------------------------------------------------
// gpLoftE: anillos ELIPTICOS apilados (torso, abrigo, esclavina): ys[] alturas, rx[]/rz[] semiejes,
// oz[] corrimiento en Z de cada anillo, col[] color de cada anillo. El arco va de th0 a th1 con
// th = 0 al FRENTE (-Z) y th = PI/2 a la DERECHA (+X); 0..TAU = cerrado. Los paneles de un
// abrigo son arcos parciales. Vertices: (n-1)*segs*6.
// -----------------------------------------------------------------------------
static void gpLoftE(LineVertex *b, int &i, float cx, float cz, int n, const float *ys, const float *rx, const float *rz,
                   const float *oz, const unsigned int *col, int segs, float th0, float th1) {
    for (int k = 0; k + 1 < n; ++k) {
        const float dy = ys[k + 1] - ys[k], dr = ((rx[k + 1] + rz[k + 1]) - (rx[k] + rz[k])) * 0.5f;
        const float ny = (fabsf(dy) > 1e-5f) ? -dr / dy : 0.0f;    // se acampana -> mira hacia abajo
        for (int s = 0; s < segs; ++s) {
            const float a0 = th0 + (th1 - th0) * s / segs, a1 = th0 + (th1 - th0) * (s + 1) / segs;
            float X[4], Y[4], Z[4]; unsigned int C[4];
            const int ring[4] = { k, k, k + 1, k + 1 }; const float ang[4] = { a0, a1, a1, a0 };
            for (int q = 0; q < 4; ++q) {
                const int r = ring[q]; const float sa = sinf(ang[q]), ca = cosf(ang[q]);
                X[q] = cx + sa * rx[r]; Y[q] = ys[r]; Z[q] = cz + oz[r] - ca * rz[r];
                C[q] = gpShade(col[r], sa / rx[r], ny * 6.0f, -ca / rz[r]);
            }
            gpQuad(b, i, X[0], Y[0], Z[0], C[0], X[1], Y[1], Z[1], C[1], X[2], Y[2], Z[2], C[2], X[3], Y[3], Z[3], C[3]);
        }
    }
}
// gpTatters: jirones (triangulos que cuelgan) en el borde de abajo de un arco eliptico: el dobladillo
// deja de ser una linea perfecta. count*3 vertices.
static void gpTatters(LineVertex *b, int &i, float cx, float cz, float y, float rx, float rz, float oz,
                      float th0, float th1, int count, float len, unsigned int seed, unsigned int cTop, unsigned int cTip) {
    for (int k = 0; k < count; ++k) {
        unsigned int q = (seed + k) * 2654435761u; q ^= q >> 15; q *= 0x5bd1e995u; q ^= q >> 13;
        const float u = (k + 0.5f) / count, a = th0 + (th1 - th0) * u, h = (th1 - th0) * 0.5f / count;
        const float L = len * (0.45f + 0.55f * ((q & 255) / 255.0f)), out = 1.0f + 0.10f * (((q >> 8) & 255) / 255.0f);
        const float xa = cx + sinf(a - h) * rx, za = cz + oz - cosf(a - h) * rz, xb = cx + sinf(a + h) * rx, zb = cz + oz - cosf(a + h) * rz;
        const float xt = cx + sinf(a) * rx * out, zt = cz + oz - cosf(a) * rz * out;
        const unsigned int c0 = gpShade(cTop, sinf(a), -0.3f, -cosf(a)), c1 = gpShade(cTip, sinf(a), -0.3f, -cosf(a));
        gpTri(b, i, xa, y, za, c0, xb, y, zb, c0, xt, y - L, zt, c1);
    }
}

// -----------------------------------------------------------------------------
// gpTuft: mechon de 3 hojas de pasto finas, cada una inclinada a su lado (9 v). seed varia.
// -----------------------------------------------------------------------------
static void gpTuft(LineVertex *b, int &i, float x, float y, float z, float h, unsigned int seed,
                   unsigned int cBase, unsigned int cTip) {
    for (int k = 0; k < 3; ++k) {
        unsigned int q = seed * 2654435761u + (unsigned int)k * 40503u; q ^= q >> 13; q *= 0x5bd1e995u; q ^= q >> 15;
        const float a = GP_TAU * ((q & 1023) / 1023.0f), lean = 0.18f + 0.30f * (((q >> 10) & 255) / 255.0f);
        const float hh = h * (0.65f + 0.45f * (((q >> 18) & 255) / 255.0f)), w = 0.05f + 0.03f * k;
        const float ca = cosf(a), sa = sinf(a);
        gpTri(b, i, x - sa * w, y, z + ca * w, cBase, x + sa * w, y, z - ca * w, cBase,
                    x + ca * lean * hh, y + hh, z + sa * lean * hh, cTip);
    }
}
