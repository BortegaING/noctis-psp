// =====================================================================
// floor2.h -- PISO 2: jefe SERAFIN (OFANIM). Escalera blanca flotante en el cielo.
// Se incluye DESPUES de skyworld.h (usa LineVertex, RGBA, brighten, swBoxLit,
// swQuadC, addLimb, addBall, CityBldg). Todo se dibuja sin cull, color por vertice.
// =====================================================================
#pragma once

static const unsigned int F2_WHITE = RGBA(248, 246, 238, 255);
static const unsigned int F2_IVORY = RGBA(234, 226, 204, 255);
static const unsigned int F2_SHADE = RGBA(196, 196, 206, 255);   // base fria de los bloques
static const unsigned int F2_GOLD  = RGBA(232, 200, 112, 255);
static const unsigned int F2_GOLDD = RGBA(190, 152,  70, 255);
static const unsigned int F2_ROSE  = RGBA(226, 164, 134, 255);   // oro rosado / cobre claro
static const unsigned int F2_BLUE  = RGBA(104, 170, 238, 255);   // iris
static const unsigned int F2_PUPIL = RGBA( 12,  14,  26, 255);

static const int   F2_STEPS = 40;
static const float F2_RISE = 0.75f, F2_RUN = 3.0f, F2_WIDTH = 12.0f;
static const float F2_LAND0 = 14.0f, F2_LAND1 = 22.0f;
static const float F2_LAND1_Z = -(F2_LAND0 * 0.5f + F2_STEPS * F2_RUN + F2_LAND1 * 0.5f);
static const float F2_LAND1_TOP = (F2_STEPS + 1) * F2_RISE;

static void f2AddBox(CityBldg *boxes, int *n, int cap, float x, float z, float w, float d, float h, unsigned int col) {
    if (!boxes || !n || *n >= cap) return;
    boxes[*n].x = x; boxes[*n].z = z; boxes[*n].w = w; boxes[*n].d = d; boxes[*n].h = h;
    boxes[*n].color = col; (*n)++;
}

// Descanso: losa gruesa marfil + zocalo dorado debajo + inserto claro arriba.
static void f2Landing(LineVertex *buf, int &i, int cap, CityBldg *boxes, int *nBoxes, int boxCap,
                      float cz, float s, float top) {
    if (i + 90 <= cap) {
        swBoxLit(buf, i, 0, top - 2.2f, cz, s, s, 2.2f, F2_WHITE, F2_SHADE);                      // losa
        swBoxLit(buf, i, 0, top - 3.0f, cz, s * 0.82f, s * 0.82f, 0.9f, F2_GOLD, F2_GOLDD);       // zocalo
        swBoxLit(buf, i, 0, top - 0.02f, cz, s * 0.78f, s * 0.78f, 0.06f, F2_IVORY, F2_IVORY);    // inserto claro
    }
    f2AddBox(boxes, nBoxes, boxCap, 0, cz, s, s, top, F2_WHITE);
}

static int f2BuildStairs(LineVertex *buf, int cap, CityBldg *boxes, int *nBoxes, int boxCap) {
    int i = 0;
    if (nBoxes) *nBoxes = 0;
    // descanso inicial: techo y=0
    f2Landing(buf, i, cap, boxes, nBoxes, boxCap, 0.0f, F2_LAND0, 0.0f);
    for (int k = 0; k < F2_STEPS; k++) {
        float top = (k + 1) * F2_RISE;
        float zc = -(F2_LAND0 * 0.5f + F2_RUN * (k + 0.5f));
        bool alt = (k & 1) != 0;
        unsigned int cTop = alt ? F2_IVORY : F2_WHITE;
        unsigned int cBot = alt ? F2_SHADE : brighten(F2_SHADE, 1.08f);
        if (i + 60 > cap) break;
        swBoxLit(buf, i, 0, top - 1.6f, zc, F2_WIDTH, F2_RUN, 1.6f, cTop, cBot);          // peldano (losa)
        // nariz dorada fina en el borde frontal (+Z) del peldano
        swBoxLit(buf, i, 0, top - 0.02f, zc + F2_RUN * 0.5f - 0.18f, F2_WIDTH + 0.3f, 0.36f, 0.12f, F2_GOLD, F2_GOLDD);
        f2AddBox(boxes, nBoxes, boxCap, 0, zc, F2_WIDTH, F2_RUN, top, cTop);
    }
    f2Landing(buf, i, cap, boxes, nBoxes, boxCap, F2_LAND1_Z, F2_LAND1, F2_LAND1_TOP);
    return i;
}

static void f2LandingTop(float *x, float *y, float *z) {
    if (x) *x = 0.0f;
    if (y) *y = F2_LAND1_TOP;
    if (z) *z = F2_LAND1_Z;
}

// Banda anular (seccion rectangular) en plano XY (eje +Z) o XZ (horiz = eje +Y).
// th = grosor radial, dep = grosor a lo largo del eje. 24 verts por segmento.
static void f2Band(LineVertex *buf, int &i, int cap, float cx, float cy, float cz,
                   float R, float th, float dep, int seg, bool horiz, unsigned int col) {
    if (i + seg * 24 > cap) return;
    const float TAU = 6.2831853f;
    float ri = R - th * 0.5f, ro = R + th * 0.5f, hd = dep * 0.5f;
    unsigned int cO = brighten(col, 0.95f), cI = brighten(col, 0.65f), cF = brighten(col, 1.12f), cB = brighten(col, 0.80f);
    for (int k = 0; k < seg; k++) {
        float a0 = TAU * k / seg, a1 = TAU * (k + 1) / seg;
        float c0 = cosf(a0), s0 = sinf(a0), c1 = cosf(a1), s1 = sinf(a1);
        // P(radio, desplazamiento en el eje, cos, sen)
        #define F2P(r, e, c, s) (horiz ? cx + (r) * (c) : cx + (r) * (c)), (horiz ? cy + (e) : cy + (r) * (s)), (horiz ? cz + (r) * (s) : cz + (e))
        swQuadC(buf, i, F2P(ro,-hd,c0,s0), cO, F2P(ro, hd,c0,s0), cO, F2P(ro, hd,c1,s1), cO, F2P(ro,-hd,c1,s1), cO);
        swQuadC(buf, i, F2P(ri,-hd,c0,s0), cI, F2P(ri, hd,c0,s0), cI, F2P(ri, hd,c1,s1), cI, F2P(ri,-hd,c1,s1), cI);
        swQuadC(buf, i, F2P(ri, hd,c0,s0), cF, F2P(ro, hd,c0,s0), cF, F2P(ro, hd,c1,s1), cF, F2P(ri, hd,c1,s1), cF);
        swQuadC(buf, i, F2P(ri,-hd,c0,s0), cB, F2P(ro,-hd,c0,s0), cB, F2P(ro,-hd,c1,s1), cB, F2P(ri,-hd,c1,s1), cB);
        #undef F2P
    }
}

// OJO central del ofanim: globo blanco + iris azul + pupila negra mirando a -Z.
static int f2BuildSeraphBody(LineVertex *buf, int cap) {
    int i = 0;
    const float R = 3.5f;
    if (i + 504 <= cap) addBall(buf, i, 0, 0, 0, R, R, R, 7, 12, F2_WHITE);                 // globo (504)
    // iris: elipsoide aplastado apoyado sobre la superficie del globo (-Z)
    if (i + 192 <= cap) addBall(buf, i, 0, 0, -3.1f, 1.7f, 1.7f, 0.58f, 4, 8, F2_BLUE);      // (192)
    if (i + 108 <= cap) addBall(buf, i, 0, 0, -3.55f, 0.72f, 0.72f, 0.28f, 3, 6, F2_PUPIL);  // (108)
    return i;
}

static int f2BuildSeraphWing(LineVertex *buf, int cap) {
    int i = 0;
    // espina: dos tramos que suben hacia la punta (+X, algo +Y)
    const float px[3] = { 0.0f, 4.2f, 8.6f }, py[3] = { 0.0f, 1.3f, 2.6f };
    if (i + 144 <= cap) {
        addLimb(buf, i, px[0], py[0], 0, px[1], py[1], 0, 0.30f, 0.24f, 6, F2_GOLD, F2_IVORY);
        addLimb(buf, i, px[1], py[1], 0, px[2], py[2], 0, 0.24f, 0.10f, 6, F2_IVORY, F2_WHITE);
    }
    // plumas en 3 capas, abanicadas hacia abajo/atras; 6 verts por pluma
    const int   cnt[3]  = { 14, 12, 10 };
    const float len0[3] = { 6.2f, 4.6f, 3.0f };
    const float wid[3]  = { 0.55f, 0.48f, 0.4f };
    const float zOff[3] = { -0.10f, 0.0f, 0.10f };
    const float t0[3] = { 0.04f, 0.10f, 0.18f }, t1[3] = { 0.98f, 0.92f, 0.80f };
    for (int r = 0; r < 3; r++) {
        for (int n = 0; n < cnt[r]; n++) {
            if (i + 6 > cap) return i;
            float u = (float)n / (float)(cnt[r] - 1);
            float t = t0[r] + (t1[r] - t0[r]) * u;
            float bx, by;
            if (t < 0.5f) { float q = t / 0.5f; bx = px[0] + (px[1] - px[0]) * q; by = py[0] + (py[1] - py[0]) * q; }
            else { float q = (t - 0.5f) / 0.5f; bx = px[1] + (px[2] - px[1]) * q; by = py[1] + (py[2] - py[1]) * q; }
            // angulo desde +X: cerca de la raiz caen casi verticales, hacia la punta se alinean con la espina
            float ang = -(1.45f - 1.25f * u);
            float L = len0[r] * (1.0f - 0.55f * u) + 0.6f;
            float dx = cosf(ang), dy = sinf(ang);
            float tx = bx + dx * L, ty = by + dy * L;
            float mx = bx + dx * L * 0.38f, my = by + dy * L * 0.38f;
            float nx = -dy, ny = dx;           // perpendicular en el plano XY
            float w = wid[r] * (1.0f - 0.35f * u);
            float z = zOff[r];
            float k = 0.86f + 0.1f * r;        // capas de atras mas oscuras
            unsigned int cB = brighten(((n & 1) ? F2_IVORY : F2_WHITE), k);
            unsigned int cT = brighten(F2_WHITE, 1.05f);
            unsigned int cL = brighten(cB, 1.06f), cR = brighten(cB, 0.84f);
            if (r == 0 && (n % 4) == 3) cT = F2_GOLD;   // algunas puntas doradas
            buf[i++] = { cB, bx, by, z };
            buf[i++] = { cL, mx + nx * w, my + ny * w, z };
            buf[i++] = { cT, tx, ty, z };
            buf[i++] = { cB, bx, by, z };
            buf[i++] = { cT, tx, ty, z };
            buf[i++] = { cR, mx - nx * w, my - ny * w, z };
        }
    }
    return i;
}

// Ojito incrustado: cono hexagonal blanco + iris azul + pupila (48 verts).
// c = base sobre el aro; d = direccion hacia afuera (plano XY); t = tangente; re = radio.
static void f2EyeBump(LineVertex *buf, int &i, int cap, float cx, float cy, float cz,
                      float dx, float dy, float tx, float ty, float re) {
    if (i + 48 > cap) return;
    const float TAU = 6.2831853f;
    // P(h, u, v) = c + d*h + t*u + z*v
    #define F2E(h, u, v) cx + dx * (h) + tx * (u), cy + dy * (h) + ty * (u), cz + (v)
    float hA = re * 0.70f, hI = re * 0.33f + 0.01f * re, hP = re * 0.55f;
    for (int k = 0; k < 6; k++) {
        float a0 = TAU * k / 6, a1 = TAU * (k + 1) / 6;
        float u0 = cosf(a0), v0 = sinf(a0), u1 = cosf(a1), v1 = sinf(a1);
        float sh = 0.85f + 0.2f * u0;
        unsigned int cw = brighten(F2_WHITE, sh);
        buf[i++] = { cw, cx + dx * hA, cy + dy * hA, cz };                       // apice
        buf[i++] = { cw, F2E(0, u0 * re, v0 * re) };
        buf[i++] = { cw, F2E(0, u1 * re, v1 * re) };
        float r2 = re * 0.55f;                                                   // iris (disco hex sobre el cono)
        buf[i++] = { F2_BLUE, cx + dx * (hA + 0.02f * re), cy + dy * (hA + 0.02f * re), cz };
        buf[i++] = { F2_BLUE, F2E(hI, u0 * r2, v0 * r2) };
        buf[i++] = { F2_BLUE, F2E(hI, u1 * r2, v1 * r2) };
    }
    // pupila: 4 tris en abanico, un poco mas afuera
    float rp = re * 0.24f, hp = re * 0.58f;
    for (int k = 0; k < 4; k++) {
        float a0 = TAU * k / 4, a1 = TAU * (k + 1) / 4;
        buf[i++] = { F2_PUPIL, cx + dx * (hp + 0.05f * re), cy + dy * (hp + 0.05f * re), cz };
        buf[i++] = { F2_PUPIL, F2E(hP, cosf(a0) * rp, sinf(a0) * rp) };
        buf[i++] = { F2_PUPIL, F2E(hP, cosf(a1) * rp, sinf(a1) * rp) };
    }
    #undef F2E
}

// Aro oro rosado en plano XY (eje +Z) cubierto de ojitos mirando hacia afuera.
// Banda 12 seg = 288 verts + 12 ojitos * 48 = 576 -> ~864 verts.
static int f2BuildSeraphRing(LineVertex *buf, int cap, float R) {
    int i = 0;
    const float TAU = 6.2831853f;
    float th = R * 0.16f, dep = R * 0.14f;
    f2Band(buf, i, cap, 0, 0, 0, R, th, dep, 12, false, F2_ROSE);                 // 288
    float re = dep * 0.42f;
    const int NE = 12;
    for (int k = 0; k < NE; k++) {
        float a = TAU * (k + 0.5f) / NE, c = cosf(a), s = sinf(a);
        float rb = R + th * 0.5f;
        f2EyeBump(buf, i, cap, rb * c, rb * s, 0.0f, c, s, -s, c, re);                // 48 c/u
    }
    return i;
}
