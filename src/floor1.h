// =====================================================================
// floor1.h - PISO 1: "campo simulado" (arbol grande + muros de cielo pintado)
// Se incluye en main.cpp DESPUES de skyworld.h y char_prims.h.
// Usa: LineVertex, RGBA(), brighten(), swBoxL/swBoxLit/swQuadC, addLimb, addBall, CityBldg.
// Todo se dibuja SIN cull. Sin texturas: color por vertice.
//   f1BuildFloor    ~2.6k verts (tira)
//   f1BuildWorld    ~9.2k verts (muros ~500, arbol ~3650, detalles ~5000)
//   f1BuildElevator ~560 verts
//   f1BuildEye      ~530 verts
// =====================================================================
#ifndef FLOOR1_H
#define FLOOR1_H

// ESCALA (2026-10-15, Benjamin: "el arbol y los ojos los siento muy cerca"): el campo era de
// 120x120 y se aparecia a 46 del arbol, con los ojos girando JUSTO encima. Ahora el campo es
// 280x280, se aparece a 100, el arbol es 1.4x y los ojos estan INCRUSTADOS en los muros.
static const float F1_HALF = 140.0f;    // semilado del campo (era 60)
static const float F1_WALL_T = 4.0f;    // grosor del muro (colision)
static const float F1_WALL_H = 150.0f;  // alto de muros (era 70): de lejos siguen tapando el horizonte
static const float F1_KS = F1_HALF / 60.0f;      // escala de lo pintado a lo largo del muro
static const float F1_KH = F1_WALL_H / 70.0f;    // escala de lo pintado en altura
static const float F1_TREE_S = 1.4f;             // arbol mas grande: sigue siendo el hito de lejos
static const float F1_EYE_S = 5.0f;              // ojos de los muros: 5x (radio 9), se leen a 40..250 de distancia

// ruido deterministico 0..1 a partir de dos enteros. Se multiplica SIN signo: antes era
// (unsigned)(a * 73856093) con int, que DESBORDA desde a = 30 (comportamiento indefinido). Con
// -O2 el compilador daba por hecho que gx < 30 y el bucle del suelo de 35x35 corria sin limite
// hasta llenar el buffer: el piso entero salia en blanco. (Para a < 30 da los mismos bits.)
static float f1Rnd(int a, int b) {
    unsigned int h = (unsigned int)a * 73856093u ^ (unsigned int)b * 19349663u ^ 0x9E3779B9u;
    h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
    return (float)(h & 0xFFFF) / 65535.0f;
}
// mezcla lineal de dos colores
static unsigned int f1Mix(unsigned int a, unsigned int b, float t) {
    if (t < 0) t = 0; if (t > 1) t = 1;
    int ar = a & 255, ag = (a >> 8) & 255, ab = (a >> 16) & 255;
    int br = b & 255, bg = (b >> 8) & 255, bb = (b >> 16) & 255;
    return RGBA((int)(ar + (br - ar) * t), (int)(ag + (bg - ag) * t), (int)(ab + (bb - ab) * t), 255);
}
// triangulo suelto
static void f1Tri(LineVertex *b, int &i, unsigned int c,
                  float ax, float ay, float az, float bx, float by, float bz, float cx, float cy, float cz) {
    b[i++] = { c, ax, ay, az }; b[i++] = { c, bx, by, bz }; b[i++] = { c, cx, cy, cz };
}
// disco (abanico) en plano XY a profundidad z, radio r, seg lados
static void f1Disc(LineVertex *b, int &i, float cx, float cy, float z, float r, int seg, unsigned int c) {
    for (int s = 0; s < seg; ++s) {
        float a0 = 6.2831853f * s / seg, a1 = 6.2831853f * (s + 1) / seg;
        f1Tri(b, i, c, cx, cy, z, cx + r * cosf(a0), cy + r * sinf(a0), z, cx + r * cosf(a1), cy + r * sinf(a1), z);
    }
}

// punto sobre la cara interior del muro 'side' (0=-Z,1=+Z,2=-X,3=+X): t a lo largo, y alto, k = hacia adentro
static void f1WallPt(int side, float t, float y, float k, float &x, float &yy, float &z) {
    yy = y;
    const float f = F1_HALF - k;
    if (side == 0) { x = t; z = -f; }
    else if (side == 1) { x = t; z = f; }
    else if (side == 2) { x = -f; z = t; }
    else { x = f; z = t; }
}
static void f1WallQuad(LineVertex *b, int &i, int side, float t0, float y0, unsigned int c0,
                       float t1, float y1, unsigned int c1, float k) {
    float x0, yy0, z0, x1, yy1, z1, x2, yy2, z2, x3, yy3, z3;
    f1WallPt(side, t0, y0, k, x0, yy0, z0); f1WallPt(side, t1, y0, k, x1, yy1, z1);
    f1WallPt(side, t1, y1, k, x2, yy2, z2); f1WallPt(side, t0, y1, k, x3, yy3, z3);
    swQuadC(b, i, x0, yy0, z0, c0, x1, yy1, z1, c0, x2, yy2, z2, c1, x3, yy3, z3, c1);
}
// nube sugerida: trapecio claro (base ancha, tope estrecho)
static void f1Cloud(LineVertex *b, int &i, int side, float t, float y, float w, float h, unsigned int col, float k) {
    float x[4], yy[4], z[4];
    f1WallPt(side, t - w * 0.5f, y,     k, x[0], yy[0], z[0]);
    f1WallPt(side, t + w * 0.5f, y,     k, x[1], yy[1], z[1]);
    f1WallPt(side, t + w * 0.30f, y + h, k, x[2], yy[2], z[2]);
    f1WallPt(side, t - w * 0.30f, y + h, k, x[3], yy[3], z[3]);
    unsigned int top = brighten(col, 1.06f);
    swQuadC(b, i, x[0], yy[0], z[0], col, x[1], yy[1], z[1], col, x[2], yy[2], z[2], top, x[3], yy[3], z[3], top);
}
// color del degradado de cielo segun altura normalizada e (0 = bruma, 1 = cenit)
static unsigned int f1SkyCol(float e) {
    const unsigned int SKY_TOP = RGBA(120, 176, 232, 255), SKY_MID = RGBA(170, 212, 244, 255);
    const unsigned int SKY_LOW = RGBA(228, 240, 248, 255), HAZE = RGBA(246, 248, 250, 255);
    if (e < 0.35f) return f1Mix(HAZE, SKY_LOW, e / 0.35f);
    if (e < 0.7f)  return f1Mix(SKY_LOW, SKY_MID, (e - 0.35f) / 0.35f);
    return f1Mix(SKY_MID, SKY_TOP, (e - 0.7f) / 0.3f);
}

// ---------------------------------------------------------------- suelo
// 35x35 paneles de 8u como UNA tira de triangulos (GU_TRIANGLE_STRIP): cada vertice se comparte
// con el panel vecino, 2.588 vertices en vez de 7.350. Filas unidas con triangulos degenerados.
// 8u NO es capricho: con paneles de 15 el GE descartaba los triangulos pegados a la camara
// (aunque recorte el plano cercano, un vertice a centimetros del ojo proyecta fuera de la
// banda de guarda) y se abria un AGUJERO en el pasto, a los pies del jugador.
static const int F1_FLOOR_N = 35;
static int f1BuildFloor(LineVertex *buf, int cap) {
    const unsigned int GR_A = RGBA(104, 134, 100, 255), GR_B = RGBA(128, 150, 118, 255);
    const int N = F1_FLOOR_N;
    const float S = 2.0f * F1_HALF / N;
    int i = 0;
    auto col = [&](int gx, int gz) {   // ruido suave + damero leve por vertice
        unsigned int c = f1Mix(GR_A, GR_B, f1Rnd(gx, gz));
        return ((gx + gz) & 1) ? brighten(c, 0.95f) : c;
    };
    for (int gz = 0; gz < N; ++gz) {
        if (i + 2 * (N + 1) + 2 > cap) break;
        const float z0 = -F1_HALF + gz * S, z1 = z0 + S;
        for (int gx = 0; gx <= N; ++gx) {
            const float x = -F1_HALF + gx * S;
            const LineVertex a = { col(gx, gz), x, 0.0f, z0 }, b = { col(gx, gz + 1), x, 0.0f, z1 };
            if (gx == 0 && gz > 0) buf[i++] = a;        // degenerado: entra a la fila nueva
            buf[i++] = a; buf[i++] = b;
            if (gx == N && gz < N - 1) buf[i++] = b;    // degenerado: sale de esta fila
        }
    }
    return i;
}

// ---------------------------------------------------------------- mundo (sin el suelo)
static int f1BuildWorld(LineVertex *buf, int cap, CityBldg *boxes, int *nBoxes, int boxCap) {
    int i = 0, nb = 0;

    // ---- muros de cielo pintado
    const int BANDS = 8;
    const float TL = F1_HALF + F1_WALL_T;
    for (int side = 0; side < 4; ++side) {
        if (i + BANDS * 6 + 140 > cap) break;
        for (int bd = 0; bd < BANDS; ++bd) {
            float u0 = (float)bd / BANDS, u1 = (float)(bd + 1) / BANDS;
            // la bruma se concentra abajo
            f1WallQuad(buf, i, side, -TL, u0 * F1_WALL_H, f1SkyCol(powf(u0, 0.7f)),
                       TL, u1 * F1_WALL_H, f1SkyCol(powf(u1, 0.7f)), 0.0f);
        }
        // linea de horizonte: franja palida levemente azulada
        f1WallQuad(buf, i, side, -F1_HALF, 8.2f, RGBA(200, 222, 240, 255), F1_HALF, 9.0f, RGBA(236, 244, 250, 255), 0.04f);
        // nubes sugeridas: 5 grupos de 2-3 trapecios claros
        for (int c = 0; c < 5; ++c) {   // (posiciones del muro de 60, escaladas al muro actual)
            float t = (-52.0f + c * 26.0f + (f1Rnd(side, c) - 0.5f) * 10.0f) * F1_KS;
            float y = (18.0f + f1Rnd(c, side + 9) * 30.0f) * F1_KH;
            float w = (14.0f + f1Rnd(side + 3, c) * 10.0f) * F1_KS;
            unsigned int cc = RGBA(246, 250, 253, 255);
            f1Cloud(buf, i, side, t, y, w, 3.2f * F1_KH, cc, 0.08f);
            f1Cloud(buf, i, side, t + w * 0.22f, y + 1.8f * F1_KH, w * 0.55f, 2.6f * F1_KH, brighten(cc, 0.99f), 0.10f);
            if (c & 1) f1Cloud(buf, i, side, t - w * 0.3f, y - 1.6f * F1_KH, w * 0.7f, 2.0f * F1_KH, RGBA(236, 244, 251, 255), 0.06f);
        }
        // colision: una caja por muro
        if (boxes && nb < boxCap) {
            float cx = 0, cz = 0, w = 0, d = 0;
            const float mid = F1_HALF + F1_WALL_T * 0.5f;
            if (side == 0)      { cz = -mid; w = 2 * TL; d = F1_WALL_T; }
            else if (side == 1) { cz =  mid; w = 2 * TL; d = F1_WALL_T; }
            else if (side == 2) { cx = -mid; w = F1_WALL_T; d = 2 * TL; }
            else                { cx =  mid; w = F1_WALL_T; d = 2 * TL; }
            boxes[nb++] = { cx, cz, w, d, F1_WALL_H, RGBA(170, 212, 244, 255) };
        }
    }

    // ---- ARBOL GRANDE en (0,0). Se arma a tamano 1 y al final se escala entero (F1_TREE_S)
    const unsigned int BARK_B = RGBA(70, 52, 42, 255), BARK_T = RGBA(98, 76, 60, 255);
    const int treeI0 = i;
    if (i + 1900 <= cap) {
        // raices que se hunden en el suelo
        for (int r = 0; r < 6; ++r) {
            float a = 6.2831853f * r / 6 + f1Rnd(r, 1) * 0.5f, L = 4.6f + f1Rnd(r, 2) * 1.8f;
            addLimb(buf, i, cosf(a) * 0.6f, 2.4f, sinf(a) * 0.6f, cosf(a) * L, -0.15f, sinf(a) * L, 1.15f, 0.35f, 6, BARK_B, BARK_T);
        }
        // tronco: 4 tramos que afinan con leve torsion
        addLimb(buf, i,  0.0f,  0.0f, 0.0f,  0.1f,  5.0f, 0.0f, 2.5f, 1.9f, 8, BARK_B, BARK_T);
        addLimb(buf, i,  0.1f,  5.0f, 0.0f, -0.1f, 10.0f, 0.2f, 1.9f, 1.5f, 8, BARK_B, BARK_T);
        addLimb(buf, i, -0.1f, 10.0f, 0.2f,  0.2f, 15.0f, 0.1f, 1.5f, 1.2f, 7, BARK_B, BARK_T);
        addLimb(buf, i,  0.2f, 15.0f, 0.1f,  0.0f, 19.0f, 0.0f, 1.2f, 0.8f, 7, BARK_B, BARK_T);
        // ramas principales hacia la copa (2 tramos cada una)
        for (int r = 0; r < 7; ++r) {
            float a = 6.2831853f * r / 7 + 0.3f;
            float y0 = 11.0f + (r % 3) * 2.2f, L = 6.5f + f1Rnd(r, 5) * 2.5f;
            float mx = cosf(a) * L * 0.5f, mz = sinf(a) * L * 0.5f;
            float ex = cosf(a) * L, ez = sinf(a) * L, ey = y0 + 5.0f + f1Rnd(r, 6) * 2.0f;
            addLimb(buf, i, 0.0f, y0, 0.0f, mx, y0 + 2.6f, mz, 1.0f, 0.65f, 5, BARK_B, BARK_T);
            addLimb(buf, i, mx, y0 + 2.6f, mz, ex, ey, ez, 0.65f, 0.3f, 5, BARK_B, BARK_T);
        }
    }
    // copa: esferas facetadas en verdes oscuros con variacion (12 x 168 verts)
    if (i + 12 * 168 <= cap) {
        for (int k = 0; k < 8; ++k) {   // anillo exterior
            float a = 6.2831853f * k / 8 + 0.2f, R = 8.2f + f1Rnd(k, 11) * 1.6f;
            float cy = 17.5f + f1Rnd(k, 12) * 2.5f, rr = 5.6f + f1Rnd(k, 13) * 1.6f;
            unsigned int col = f1Mix(RGBA(42, 86, 54, 255), RGBA(70, 118, 66, 255), f1Rnd(k, 14));
            addBall(buf, i, cosf(a) * R, cy, sinf(a) * R, rr, rr * 0.82f, rr, 4, 7, col);
        }
        for (int k = 0; k < 3; ++k) {   // piso medio
            float a = 6.2831853f * k / 3 + 0.9f;
            addBall(buf, i, cosf(a) * 3.6f, 22.5f, sinf(a) * 3.6f, 5.6f, 4.6f, 5.6f, 4, 7,
                    f1Mix(RGBA(50, 100, 58, 255), RGBA(84, 134, 72, 255), f1Rnd(k, 21)));
        }
        addBall(buf, i, 0.0f, 25.0f, 0.0f, 5.4f, 3.8f, 5.4f, 4, 7, RGBA(92, 146, 78, 255));
    }
    for (int v = treeI0; v < i; ++v) { buf[v].x *= F1_TREE_S; buf[v].y *= F1_TREE_S; buf[v].z *= F1_TREE_S; }
    // colision del tronco
    if (boxes && nb < boxCap) boxes[nb++] = { 0.0f, 0.0f, 4.0f * F1_TREE_S, 4.0f * F1_TREE_S, 26.0f * F1_TREE_S, BARK_B };

    // ---- detalles: piedras, flores, hierba alta (solo si hay presupuesto). Repartidos por todo
    // el campo (antes solo hasta ~55 del arbol: con el campo grande, el pasto de la llegada quedaba pelado)
    const float DR = F1_HALF - 18.0f;   // alcance desde el arbol
    for (int r = 0; r < 10; ++r) {
        if (i + 90 > cap) break;
        float a = f1Rnd(r, 31) * 6.2831853f, d = 12.0f + f1Rnd(r, 32) * DR, s = 0.7f + f1Rnd(r, 33) * 0.9f;
        addBall(buf, i, cosf(a) * d, s * 0.25f, sinf(a) * d, s * 1.3f, s * 0.7f, s * 1.0f, 3, 5,
                f1Mix(RGBA(118, 120, 122, 255), RGBA(150, 148, 142, 255), f1Rnd(r, 34)));
    }
    static const unsigned int PET[4] = { RGBA(250, 244, 236, 255), RGBA(250, 226, 150, 255), RGBA(244, 190, 206, 255), RGBA(196, 210, 250, 255) };
    for (int r = 0; r < 24; ++r) {     // flores sueltas (tallo 36 + flor 48)
        if (i + 84 > cap) break;
        float a = f1Rnd(r, 41) * 6.2831853f, d = 11.0f + f1Rnd(r, 42) * DR;
        float x = cosf(a) * d, z = sinf(a) * d, h = 0.9f + f1Rnd(r, 43) * 0.5f;
        addLimb(buf, i, x, 0.0f, z, x, h, z, 0.07f, 0.05f, 3, RGBA(70, 108, 66, 255), RGBA(96, 140, 84, 255));
        addBall(buf, i, x, h + 0.1f, z, 0.32f, 0.2f, 0.32f, 2, 4, PET[r & 3]);   // 2x4: mide 0.3, mas caras no se ven
    }
    for (int r = 0; r < 30; ++r) {     // matas de hierba alta
        if (i + 72 > cap) break;
        float a = f1Rnd(r, 51) * 6.2831853f, d = 10.0f + f1Rnd(r, 52) * DR;
        float x = cosf(a) * d, z = sinf(a) * d;
        for (int b = 0; b < 2; ++b) {
            float lx = (f1Rnd(r, 53 + b) - 0.5f) * 0.9f, lz = (f1Rnd(r, 56 + b) - 0.5f) * 0.9f;
            addLimb(buf, i, x, 0.0f, z, x + lx, 1.5f + f1Rnd(r, 59 + b) * 0.8f, z + lz, 0.12f, 0.02f, 3,
                    RGBA(76, 116, 68, 255), RGBA(128, 164, 96, 255));
        }
    }
    *nBoxes = nb;
    return i;
}

// ---------------------------------------------------------------- elevador
// Huella 6x6 centrada en origen, alto 8, abierto hacia +Z. y=0 = suelo de la cabina.
// v2 (2026-10-16, "se sigue viendo todo muy cuadrado"): era una caja de chapa; ahora es una JAULA
// gotica de hierro: plataforma de piedra biselada con aro de laton, cuatro columnas octogonales,
// barrotes en tres lados, arco ojival sobre la entrada, techo piramidal con polea y las dos
// cadenas que suben hasta perderse (de algo cuelga). ~2300 v.
static int f1BuildElevator(LineVertex *buf, int cap) {
    int i = 0;
    if (cap < 2500) return 0;
    const unsigned int IRON = RGBA(58, 60, 68, 255), IRONL = RGBA(104, 108, 120, 255);
    const unsigned int STONE = RGBA(118, 116, 112, 255), STONED = RGBA(80, 78, 76, 255);
    const unsigned int BRASS = RGBA(176, 142, 72, 255), LIGHT = RGBA(255, 214, 120, 255);
    gpBevelBox(buf, i, 0.0f, -0.4f, 0.0f, 6.6f, 6.6f, 0.5f, 0.12f, STONE, STONED);                // plataforma
    for (int k = 0; k < 16; ++k) {                                                                // aro de laton en el piso (plano)
        const float a0 = GP_TAU * k / 16, a1 = GP_TAU * (k + 1) / 16;
        gpQuad(buf, i, cosf(a0) * 2.1f, 0.12f, sinf(a0) * 2.1f, BRASS, cosf(a1) * 2.1f, 0.12f, sinf(a1) * 2.1f, BRASS,
                       cosf(a1) * 2.3f, 0.12f, sinf(a1) * 2.3f, BRASS, cosf(a0) * 2.3f, 0.12f, sinf(a0) * 2.3f, BRASS);
    }
    const float cx[4] = { -2.9f, 2.9f, 2.9f, -2.9f }, cz[4] = { -2.9f, -2.9f, 2.9f, 2.9f };
    for (int k = 0; k < 4; ++k) {                                                                 // columnas
        gpBevelBox(buf, i, cx[k], 0.1f, cz[k], 0.62f, 0.62f, 0.32f, 0.06f, IRONL, IRON);
        gpPrism(buf, i, cx[k], 0.42f, cz[k], 0.22f, 0.18f, 7.2f, 8, 0.0f, IRON, IRONL, 0);
        gpTube(buf, i, cx[k], 0.95f, cz[k], cx[k], 1.05f, cz[k], 0.25f, 0.25f, 8, BRASS, BRASS);
        gpTube(buf, i, cx[k], 7.25f, cz[k], cx[k], 7.35f, cz[k], 0.23f, 0.23f, 8, BRASS, BRASS);
    }
    for (int w = 0; w < 3; ++w) {                                                                 // barrotes: atras, izq, der
        for (int k = 1; k <= 8; ++k) {
            const float t = -2.9f + 5.8f * k / 9.0f;
            const float x = (w == 0) ? t : (w == 1 ? -2.9f : 2.9f), z = (w == 0) ? -2.9f : t;
            gpTube(buf, i, x, 0.1f, z, x, 7.6f, z, 0.035f, 0.035f, 3, IRON, IRONL);
        }
        for (int r = 0; r < 3; ++r) {
            const float y = (r == 0) ? 1.0f : (r == 1 ? 3.2f : 7.3f);
            const float x0 = (w == 0) ? -2.9f : (w == 1 ? -2.9f : 2.9f), z0 = (w == 0) ? -2.9f : -2.9f;
            const float x1 = (w == 0) ? 2.9f : x0, z1 = (w == 0) ? -2.9f : 2.9f;
            gpTube(buf, i, x0, y, z0, x1, y, z1, r == 1 ? 0.07f : 0.05f, r == 1 ? 0.07f : 0.05f, 4, r == 1 ? BRASS : IRON, r == 1 ? BRASS : IRONL);
        }
    }
    {   // arco ojival de hierro sobre la entrada (+Z)
        const float R = 4.1f, r = 2.9f, ca = (R - r) / R, amax = acosf(ca), apex = R * sinf(amax), hs = 7.6f - apex;
        float px = 2.9f, py = hs;
        for (int k = 1; k <= 7; ++k) {
            const float a = amax * k / 7, x = r - R + R * cosf(a), y = hs + R * sinf(a);
            gpTube(buf, i, px, py, 2.9f, x, y, 2.9f, 0.09f, 0.09f, 4, IRONL, IRONL);
            gpTube(buf, i, -px, py, 2.9f, -x, y, 2.9f, 0.09f, 0.09f, 4, IRONL, IRONL);
            px = x; py = y;
        }
    }
    gpBevelBox(buf, i, 0.0f, 7.6f, 0.0f, 6.8f, 6.8f, 0.42f, 0.10f, IRONL, IRON);                   // techo
    gpPrism(buf, i, 0.0f, 8.02f, 0.0f, 3.7f, 0.35f, 1.7f, 4, 0.7854f, IRON, IRONL, 0);
    gpRingXY(buf, i, 0.0f, 10.2f, 0.0f, 0.55f, 0.12f, 0.16f, 8, BRASS);                          // polea
    gpTube(buf, i, 0.0f, 9.7f, 0.0f, 0.0f, 10.2f, 0.0f, 0.07f, 0.07f, 4, IRONL, IRONL);
    for (int s = -1; s <= 1; s += 2)                                                              // cadenas
        gpTube(buf, i, s * 0.55f, 10.2f, 0.0f, s * 0.55f, 46.0f, 0.0f, 0.05f, 0.05f, 4, IRON, IRON);
    addBall(buf, i, 0.0f, 7.25f, 2.95f, 0.24f, 0.24f, 0.16f, 3, 8, LIGHT);                          // luz sobre la entrada
    gpPrism(buf, i, 0.0f, 7.2f, 0.0f, 0.8f, 0.8f, 0.1f, 4, 0.7854f, RGBA(255, 246, 220, 255), RGBA(255, 246, 220, 255), 0);
    return i;
}

// ---------------------------------------------------------------- ojo enemigo
// Centrado en origen, radio 1.8, mira hacia -Z.
static int f1BuildEye(LineVertex *buf, int cap) {
    int i = 0;
    if (cap < 540) return 0;
    const float R = 1.8f;
    addBall(buf, i, 0, 0, 0, R, R, R, 4, 8, RGBA(244, 238, 232, 255));       // globo
    // borde del iris + iris rojo + pupila negra, sobre la superficie -Z
    const float rI = 0.95f, rP = 0.42f;
    float zI = -sqrtf(R * R - rI * rI) - 0.03f;
    float zP = -sqrtf(R * R - rP * rP) - 0.07f;
    f1Disc(buf, i, 0, 0, zI - 0.02f, rI + 0.12f, 10, RGBA(54, 8, 10, 255));
    f1Disc(buf, i, 0, 0, zI - 0.04f, rI, 10, RGBA(150, 14, 20, 255));
    f1Disc(buf, i, 0, 0, zP - 0.04f, rP, 10, RGBA(6, 4, 6, 255));
    // parpados oscuros (arriba y abajo)
    addBall(buf, i, 0, R * 0.78f, -0.35f, R * 0.95f, 0.38f, R * 0.95f, 2, 6, RGBA(60, 30, 40, 255));
    addBall(buf, i, 0, -R * 0.80f, -0.35f, R * 0.9f, 0.32f, R * 0.9f, 2, 6, RGBA(60, 30, 40, 255));
    // venitas
    addLimb(buf, i, -1.6f, 0.4f, -0.7f, -0.9f, 0.1f, -1.52f, 0.05f, 0.025f, 3, RGBA(176, 40, 44, 255), RGBA(190, 70, 70, 255));
    addLimb(buf, i, 1.6f, -0.3f, -0.7f, 1.0f, -0.1f, -1.5f, 0.05f, 0.025f, 3, RGBA(176, 40, 44, 255), RGBA(190, 70, 70, 255));
    return i;
}
// El MISMO ojo para lejos (~170 verts en vez de ~530): globo con menos caras, iris y pupila.
// Lejos los parpados y las venitas no llegan ni a un pixel; con 30 ojos son ~10k vertices
// menos por frame, que en la PSP real es lo que mas pesa de este piso.
static int f1BuildEyeFar(LineVertex *buf, int cap) {
    int i = 0;
    if (cap < 200) return 0;
    const float R = 1.8f, rI = 0.95f, rP = 0.42f;
    addBall(buf, i, 0, 0, 0, R, R, R, 3, 7, RGBA(236, 230, 224, 255));
    // con 7 caras el globo asoma mas adelante que la esfera ideal: los discos van por DELANTE
    const float zI = -R - 0.03f, zP = -R - 0.08f;
    f1Disc(buf, i, 0, 0, zI, rI + 0.12f, 8, RGBA(150, 14, 20, 255));
    f1Disc(buf, i, 0, 0, zP, rP, 8, RGBA(6, 4, 6, 255));
    return i;
}

#endif // FLOOR1_H
