// ============================================================================
// skyworld.h -- MUNDO NUEVO (2026-10-15): megaestructura de PLATAFORMAS EN EL CIELO
//
// Referencia: TikTok de @sentientmass. Losas planas de hormigon colgadas sobre un vacio
// con bruma, figuras diminutas, y al fondo colosos: torres escalonadas, anillos-engranaje
// gigantes, vigas que cruzan el cielo. Gris-azul casi monocromo.
//
// MUNDO INFINITO POR CELDAS (SW_C x SW_C). Cada celda se deduce de un hash de (cx,cz):
// nada se guarda, asi que el mundo no tiene fin y no ocupa memoria por existir.
//  - Malla: solo las celdas a <= SW_R del jugador (5x5). Al cambiar de celda se liberan
//    las que quedaron lejos y se generan las nuevas DE A UNA POR FRAME (sin tirones al
//    pasar de seccion). Las del anillo exterior se dibujan solo con su nucleo (LOD).
//  - Niebla por HARDWARE (sceGuFog) que termina antes del anillo exterior: lo que aparece
//    o desaparece lo hace ya disuelto en la bruma, sin "pop".
//  - Colision: g_city se rehace con las 3x3 celdas alrededor del jugador al cambiar de
//    celda. Geometria y colision salen de la MISMA funcion (swEmitCell): no se desalinean.
//  - Fondo: colosos en un anillo lejano que SIGUE al jugador (como un skybox con volumen),
//    color horneado hacia la bruma. Se construye una vez.
//
// Se incluye despues de hud.h: usa LineVertex, addQuad, addLimb, brighten,
// gradQuad, g_city/g_cityCount/CityBldg, RGBA, LINE_FLAGS.
// ============================================================================

static const float SW_C      = 48.0f;    // lado de celda
static const int   SW_R      = 2;        // radio de celdas con malla (5x5)
static const int   SW_POOL   = 40;       // ranuras de malla (25 + histeresis)
static const int   SW_TV     = 1000;     // vertices de losas/puentes/escalones max por celda
static const int   SW_LV     = 1100;     // vertices de color max por celda
static const float SW_RISE   = 0.75f;    // alto de escalon (< 0.8, lo que blocked() deja subir)
static const float SW_TREAD  = 1.1f;     // huella de escalon
// La niebla termina ANTES de donde nacen las celdas nuevas (anillo 2: su contenido empieza a
// ~72-80u): asi aparecen ya disueltas en la bruma, no de golpe.
static const float SW_FOG_NEAR = 20.0f, SW_FOG_FAR = 86.0f;
static const unsigned int SW_HAZE = RGBA(142, 146, 152, 255);   // bruma GRIS de tormenta (antes 186,192,198)

// ---------------------------------------------------------------- hash / azar
static unsigned int swHash(int x, int z, unsigned int salt) {
    unsigned int h = (unsigned int)x * 73856093u ^ (unsigned int)z * 19349663u ^ salt * 83492791u;
    h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
    return h;
}
static float swRand(int x, int z, unsigned int salt) { return (float)(swHash(x, z, salt) & 0xFFFF) / 65535.0f; }

// ---------------------------------------------------------------- disposicion de una celda
// type 0 = vacio, 1 = plataforma caminable, 2 = coloso (torre que sale del abismo)
struct SwLayout { int type; float x, z, w, d, top; };

// CAMINO AL ELEVADOR: las celdas (0,0)..(0,SW_ELEV_CZ) son SIEMPRE losas unidas por puentes
// (se camina "hacia adelante" viendo el cielo); en la ultima esta el elevador (levels.h).
static const int SW_ELEV_CZ = -5;
static bool swOnPath(int cx, int cz) { return cx == 0 && cz <= 0 && cz >= SW_ELEV_CZ; }

static void swLayout(int cx, int cz, SwLayout &L) {
    const bool spawnArea = (cx >= -1 && cx <= 1 && cz >= -1 && cz <= 1) || swOnPath(cx, cz);
    const float r = swRand(cx, cz, 1);
    L.type = spawnArea ? 1 : (r < 0.78f ? 1 : 0);                 // sin celdas-torre: losa o cielo abierto
    L.w = 26.0f + 12.0f * swRand(cx, cz, 2);                       // losas MAS GRANDES (antes 18..32)
    L.d = 26.0f + 12.0f * swRand(cx, cz, 3);
    L.x = cx * SW_C + (swRand(cx, cz, 4) - 0.5f) * 6.0f;
    L.z = cz * SW_C + (swRand(cx, cz, 5) - 0.5f) * 6.0f;
    L.top = (float)(swHash(cx, cz, 6) % 3u) * 2.4f;          // 0 / 2.4 / 4.8: se sube por escalones
    if (swOnPath(cx, cz)) { L.x = 0.0f; L.top = 0.0f; }   // camino RECTO y PLANO hasta el elevador
    if (cx == 0 && cz == 0) { L.x = 0.0f; L.z = 0.0f; L.w = 36.0f; L.d = 36.0f; }
    if (cx == 0 && cz == SW_ELEV_CZ) { L.x = 0.0f; L.z = cz * SW_C; L.w = 36.0f; L.d = 36.0f; }
}

// puente de (cx,cz) hacia +X (axis 0) o +Z (axis 1). Lo "posee" la celda de menor coordenada.
static bool swBridge(int cx, int cz, int axis) {
    SwLayout a, b; swLayout(cx, cz, a); swLayout(cx + (axis == 0), cz + (axis == 1), b);
    if (a.type != 1 || b.type != 1) return false;
    const bool spawnLink = (cx >= -1 && cx <= 1 && cz >= -1 && cz <= 1 &&
                            cx + (axis == 0) <= 1 && cz + (axis == 1) <= 1)
                         || (axis == 1 && swOnPath(cx, cz) && swOnPath(cx, cz + 1));
    return spawnLink || (swHash(cx, cz, 7u + (unsigned int)axis) % 100u) < 72u;
}

// ---------------------------------------------------------------- salida (malla y/o colision)
struct SwOut {
    LineVertex *t; int tn, tCore;             // losas, puentes, escalones, pretiles, monolitos
    LineVertex *l; int ln, lCore, lBoxEnd;    // pilares, torres, losas de abajo | faroles | anillos
    CityBldg *box; int bn, bmax;              // colision (null = no se pide)
};

// ---------------------------------------------------------------- caja ILUMINADA
// Un SOL desde arriba-izquierda (como la referencia): cada cara tiene SU tono segun hacia
// donde mira, en vez del 1.15/0.82/0.60 simetrico de addSolidBox (frente = atras, izq = der:
// sin direccion de luz, se ve plano). Ademas cada costado va de su tono arriba a colBot
// abajo: oclusion ambiental en el pie de cada muro, o la bruma subiendo en los colosos.
// Sentido de las caras: el INVERSO de addSolidBox(T), que con NOCTIS_FRONTFACE y cull ON se
// veia desde ADENTRO ("veo dentro de las escaleras").
// cielo NUBLADO: luz difusa, caras mas parejas (antes 1.10 / 0.96 / 0.80 / 0.68 / 0.54)
static const float SW_SUN_TOP = 1.04f, SW_SUN_W = 0.92f, SW_SUN_N = 0.82f, SW_SUN_S = 0.74f, SW_SUN_E = 0.64f;
static void swQuadC(LineVertex *b, int &i, float ax, float ay, float az, unsigned int ca,
                    float bx, float by, float bz, unsigned int cb, float cx_, float cy_, float cz_, unsigned int cc,
                    float dx, float dy, float dz, unsigned int cd) {
    b[i++] = { ca, ax, ay, az }; b[i++] = { cb, bx, by, bz }; b[i++] = { cc, cx_, cy_, cz_ };
    b[i++] = { ca, ax, ay, az }; b[i++] = { cc, cx_, cy_, cz_ }; b[i++] = { cd, dx, dy, dz };
}
static void swBoxLit(LineVertex *b, int &i, float cx, float y0, float cz, float w, float d, float h,
                     unsigned int cTop, unsigned int cBot, bool withTop = true) {
    const float x0 = cx - w * 0.5f, x1 = cx + w * 0.5f, z0 = cz - d * 0.5f, z1 = cz + d * 0.5f, y1 = y0 + h;
    const unsigned int t  = brighten(cTop, SW_SUN_TOP);   // withTop=false: el techo lo ponen los paneles
    const unsigned int nT = brighten(cTop, SW_SUN_N), nB = brighten(cBot, SW_SUN_N);
    const unsigned int sT = brighten(cTop, SW_SUN_S), sB = brighten(cBot, SW_SUN_S);
    const unsigned int wT = brighten(cTop, SW_SUN_W), wB = brighten(cBot, SW_SUN_W);
    const unsigned int eT = brighten(cTop, SW_SUN_E), eB = brighten(cBot, SW_SUN_E);
    if (withTop) swQuadC(b, i, x0, y1, z0, t,  x0, y1, z1, t,  x1, y1, z1, t,  x1, y1, z0, t);    // techo
    swQuadC(b, i, x0, y0, z0, nB, x0, y1, z0, nT, x1, y1, z0, nT, x1, y0, z0, nB);   // -Z (norte)
    swQuadC(b, i, x1, y0, z1, sB, x1, y1, z1, sT, x0, y1, z1, sT, x0, y0, z1, sB);   // +Z (sur)
    swQuadC(b, i, x0, y0, z1, wB, x0, y1, z1, wT, x0, y1, z0, wT, x0, y0, z0, wB);   // -X (oeste)
    swQuadC(b, i, x1, y0, z0, eB, x1, y1, z0, eT, x1, y1, z1, eT, x1, y0, z1, eB);   // +X (este)
}
// caja comun: la base de cada muro un 28% mas oscura (contacto/oclusion)
static void swBoxL(LineVertex *b, int &i, float cx, float y0, float cz, float w, float d, float h, unsigned int col) {
    swBoxLit(b, i, cx, y0, cz, w, d, h, col, brighten(col, 0.72f));
}

static void swSolidT(SwOut &o, float x, float base, float z, float w, float d, float h, unsigned int col, bool collide) {
    if (o.t && o.tn + 30 <= SW_TV) swBoxL(o.t, o.tn, x, base, z, w, d, h, col);
    if (o.box && collide && o.bn < o.bmax) o.box[o.bn++] = { x, z, w, d, base + h, col };
}
static void swSolidL(SwOut &o, float x, float base, float z, float w, float d, float h, unsigned int col, bool collide) {
    if (o.l && o.ln + 30 <= SW_LV) swBoxL(o.l, o.ln, x, base, z, w, d, h, col);
    if (o.box && collide && o.bn < o.bmax) o.box[o.bn++] = { x, z, w, d, base + h, col };
}

// anillo vertical con dientes (engranaje). Eje horizontal (ax,az) unitario. Sin colision.
// Winding no auditado -> se dibuja SIN cull.
static void swRing(LineVertex *buf, int &i, int cap, float cx, float cy, float cz, float ax, float az,
                   float R, float th, float dd, int seg, float tooth, unsigned int col) {
    const float tx = -az, tz = ax;                    // tangente horizontal del plano del anillo
    const unsigned int cF = col, cB = brighten(col, 0.80f);
    for (int s = 0; s < seg; ++s) {
        if (i + 60 > cap) return;
        const float a0 = 6.2831853f * s / seg, a1 = 6.2831853f * (s + 1) / seg;
        const float c0 = cosf(a0), s0 = sinf(a0), c1 = cosf(a1), s1 = sinf(a1);
        const float Ro = (tooth > 0.0f && (s & 1)) ? R + th + tooth : R + th, Ri = R - th;
        // puntos: o/i = exterior/interior, f/b = cara delantera/trasera
        #define SWP(c, s_, rr, side) cx + (c) * (rr) * tx + ax * (side), cy + (s_) * (rr), cz + (c) * (rr) * tz + az * (side)
        const unsigned int cRim = brighten(col, 0.70f + 0.45f * (s0 > 0 ? s0 : 0));
        addQuad(buf, i, SWP(c0, s0, Ro,  dd), SWP(c1, s1, Ro,  dd), SWP(c1, s1, Ri,  dd), SWP(c0, s0, Ri,  dd), cF);
        addQuad(buf, i, SWP(c0, s0, Ro, -dd), SWP(c0, s0, Ri, -dd), SWP(c1, s1, Ri, -dd), SWP(c1, s1, Ro, -dd), cB);
        addQuad(buf, i, SWP(c0, s0, Ro,  dd), SWP(c0, s0, Ro, -dd), SWP(c1, s1, Ro, -dd), SWP(c1, s1, Ro,  dd), cRim);
        addQuad(buf, i, SWP(c0, s0, Ri,  dd), SWP(c1, s1, Ri,  dd), SWP(c1, s1, Ri, -dd), SWP(c0, s0, Ri, -dd), cB);
        if (Ro > R + th) {   // paredes laterales del diente
            const float Rb = R + th;
            addQuad(buf, i, SWP(c0, s0, Rb, dd), SWP(c0, s0, Ro, dd), SWP(c0, s0, Ro, -dd), SWP(c0, s0, Rb, -dd), cB);
            addQuad(buf, i, SWP(c1, s1, Rb, dd), SWP(c1, s1, Rb, -dd), SWP(c1, s1, Ro, -dd), SWP(c1, s1, Ro, dd), cB);
        }
        #undef SWP
    }
}

// ---------------------------------------------------------------- una celda
static const unsigned int SW_C_SLAB  = RGBA(164, 165, 166, 255);   // hormigon bajo cielo gris   // techo x1.15 = gris claro, no blanco quemado
static const unsigned int SW_C_MONO  = RGBA(150, 151, 150, 255);
static const unsigned int SW_C_PIL   = RGBA( 92,  97, 106, 255);
static const unsigned int SW_C_LOW   = RGBA(150, 156, 164, 255);
static const unsigned int SW_C_TOWER = RGBA(104, 110, 120, 255);
static const unsigned int SW_C_RING  = RGBA(120, 124, 132, 255);
static const unsigned int SW_C_LAMP  = RGBA(196, 236, 244, 255);

static void swEmitBridge(int cx, int cz, int axis, SwOut &o) {
    SwLayout a, b; swLayout(cx, cz, a); swLayout(cx + (axis == 0), cz + (axis == 1), b);
    const float lo = (a.top < b.top) ? a.top : b.top, hi = (a.top < b.top) ? b.top : a.top;
    const float W = 4.0f, TH = 1.6f;
    const int n = (int)ceilf((hi - lo) / SW_RISE - 0.01f);
    const bool upB = (b.top > a.top);                 // la losa alta es b (+eje)?
    // Sin superficies COPLANARES (con z de 16 bits parpadeaban): el puente queda 0.04 bajo
    // la losa donde se mete, y los escalones son 0.12 mas angostos que el puente.
    const float BT = lo - 0.04f, SW = W - 0.12f;
    const unsigned int cStep = brighten(SW_C_SLAB, 0.94f);       // un pelo mas oscuros: se leen los peldanos
    if (axis == 0) {
        const float x0 = a.x + a.w * 0.5f - 1.0f, x1 = b.x - b.w * 0.5f + 1.0f, zc = (a.z + b.z) * 0.5f;
        swSolidT(o, (x0 + x1) * 0.5f, BT - TH, zc, x1 - x0, W, TH, SW_C_SLAB, true);
        for (int k = 1; k <= n; ++k) {
            const float top = lo + (hi - lo) * k / n;
            const float sx = upB ? (b.x - b.w * 0.5f) - (n - k + 0.5f) * SW_TREAD
                                 : (a.x + a.w * 0.5f) + (n - k + 0.5f) * SW_TREAD;
            swSolidT(o, sx, BT - TH, zc, SW_TREAD, SW, top - (BT - TH), (k & 1) ? cStep : SW_C_SLAB, true);
        }
    } else {
        const float z0 = a.z + a.d * 0.5f - 1.0f, z1 = b.z - b.d * 0.5f + 1.0f, xc = (a.x + b.x) * 0.5f;
        swSolidT(o, xc, BT - TH, (z0 + z1) * 0.5f, W, z1 - z0, TH, SW_C_SLAB, true);
        for (int k = 1; k <= n; ++k) {
            const float top = lo + (hi - lo) * k / n;
            const float sz = upB ? (b.z - b.d * 0.5f) - (n - k + 0.5f) * SW_TREAD
                                 : (a.z + a.d * 0.5f) + (n - k + 0.5f) * SW_TREAD;
            swSolidT(o, xc, BT - TH, sz, SW, SW_TREAD, top - (BT - TH), (k & 1) ? cStep : SW_C_SLAB, true);
        }
    }
}

static void swEmitCell(int cx, int cz, SwOut &o) {
    SwLayout L; swLayout(cx, cz, L);
    // ===== T nucleo: losa + puentes propios =====
    if (L.type == 1) {
        // losa: costados iluminados + techo en PANELES de hormigon con tono levemente distinto
        // (las juntas del video). Teselar ademas reparte mejor la niebla por vertice.
        const int nx = (int)(L.w / 8.0f + 0.5f), nz = (int)(L.d / 8.0f + 0.5f);
        if (o.t && o.tn + 30 + 6 * nx * nz <= SW_TV) {
            swBoxLit(o.t, o.tn, L.x, L.top - 4.0f, L.z, L.w, L.d, 4.0f, SW_C_SLAB, brighten(SW_C_SLAB, 0.72f), false);
            const float x0 = L.x - L.w * 0.5f, z0 = L.z - L.d * 0.5f, pw = L.w / nx, pd = L.d / nz, y = L.top;
            for (int a = 0; a < nx; ++a)
                for (int c = 0; c < nz; ++c) {
                    const unsigned int t = brighten(SW_C_SLAB, SW_SUN_TOP * (0.95f + 0.07f * swRand(cx * 16 + a, cz * 16 + c, 99)));
                    const float px0 = x0 + a * pw, px1 = px0 + pw, pz0 = z0 + c * pd, pz1 = pz0 + pd;
                    swQuadC(o.t, o.tn, px0, y, pz0, t, px0, y, pz1, t, px1, y, pz1, t, px1, y, pz0, t);
                }
        }
        if (o.box && o.bn < o.bmax) o.box[o.bn++] = { L.x, L.z, L.w, L.d, L.top, SW_C_SLAB };
        if (swBridge(cx, cz, 0)) swEmitBridge(cx, cz, 0, o);
        if (swBridge(cx, cz, 1)) swEmitBridge(cx, cz, 1, o);
    }
    o.tCore = o.tn;
    // ===== T detalle: pretiles en bordes SIN puente, monolitos =====
    const bool elevCell = (cx == 0 && cz == SW_ELEV_CZ);   // la losa del elevador va despejada
    if (elevCell)   // muro alto al fondo: el elevador se apoya en el y no se sigue de largo al vacio
        swSolidT(o, L.x, L.top, L.z - L.d * 0.5f + 0.6f, L.w, 1.2f, 9.0f, SW_C_MONO, true);
    if (L.type == 1 && !elevCell) {
        const float hw = L.w * 0.5f, hd = L.d * 0.5f, PH = 1.1f, PT = 0.7f;
        if (!swBridge(cx, cz, 0)     && swRand(cx, cz, 10) < 0.5f) swSolidT(o, L.x + hw - PT * 0.5f, L.top, L.z, PT, L.d, PH, SW_C_SLAB, true);
        if (!swBridge(cx - 1, cz, 0) && swRand(cx, cz, 11) < 0.5f) swSolidT(o, L.x - hw + PT * 0.5f, L.top, L.z, PT, L.d, PH, SW_C_SLAB, true);
        if (!swBridge(cx, cz, 1)     && swRand(cx, cz, 12) < 0.5f) swSolidT(o, L.x, L.top, L.z + hd - PT * 0.5f, L.w, PT, PH, SW_C_SLAB, true);
        if (!swBridge(cx, cz - 1, 1) && swRand(cx, cz, 13) < 0.5f) swSolidT(o, L.x, L.top, L.z - hd + PT * 0.5f, L.w, PT, PH, SW_C_SLAB, true);
        const int nm = (int)(swHash(cx, cz, 14) % 3u);                     // 0..2 monolitos
        for (int m = 0; m < nm; ++m) {
            const float sx = ((swHash(cx, cz, 15 + m) & 1) ? 1.0f : -1.0f), sz = (m == 0) ? 1.0f : -1.0f;
            const float h = 5.0f + 9.0f * swRand(cx, cz, 17 + m);
            swSolidT(o, L.x + sx * L.w * 0.34f, L.top, L.z + sz * L.d * 0.34f, 2.4f, 2.4f, h, SW_C_MONO, true);
        }
    }
    // ===== L nucleo: (vacio) =====
    // Antes: pilares que bajaban 40..130u bajo cada losa, torres que subian del abismo y losas
    // sueltas abajo. Se leian como TORRES (Benjamin: "saca las torres"); ahora las losas flotan
    // sobre un mar de nubes (levels.h, lvBackdrop) como en un cielo real.
    o.lCore = o.ln;
    // ===== L detalle: farol frio en una esquina =====
    if (L.type == 1 && !elevCell && swRand(cx, cz, 50) < 0.45f) {
        const float sx = (swHash(cx, cz, 51) & 1) ? 1.0f : -1.0f, sz = (swHash(cx, cz, 52) & 1) ? 1.0f : -1.0f;
        const float x = L.x + sx * (L.w * 0.5f - 1.4f), z = L.z + sz * (L.d * 0.5f - 1.4f);
        swSolidL(o, x, L.top, z, 0.35f, 0.35f, 4.6f, SW_C_PIL, true);
        swSolidL(o, x, L.top + 4.6f, z, 0.9f, 0.9f, 0.6f, SW_C_LAMP, false);
    }
    o.lBoxEnd = o.ln;
    // ===== anillos (sin cull) =====
    if (o.l) {
        if (L.type == 1 && !elevCell && swRand(cx, cz, 60) < 0.20f) {
            const float R = 4.5f + 2.0f * swRand(cx, cz, 61);
            const bool alongX = (swHash(cx, cz, 62) & 1) != 0;
            swRing(o.l, o.ln, SW_LV, L.x - L.w * 0.2f, L.top + R + 0.7f, L.z + L.d * 0.2f,
                   alongX ? 1.0f : 0.0f, alongX ? 0.0f : 1.0f, R, 0.6f, 0.5f, 16, 0.0f, SW_C_RING);
        } else if (L.type == 2 && swRand(cx, cz, 63) < 0.5f) {
            const float R = 12.0f + 8.0f * swRand(cx, cz, 64);
            const float topY = 30.0f + 110.0f * swRand(cx, cz, 31);
            swRing(o.l, o.ln, SW_LV, cx * SW_C, topY + R * 0.6f, cz * SW_C, 1.0f, 0.0f, R, 1.4f, 1.2f, 20, 1.6f, SW_C_RING);
        }
    }
}

// ---------------------------------------------------------------- pool de celdas con malla
struct SwCell { int cx, cz, used, tn, tCore, ln, lCore, lBoxEnd; };
static SwCell g_swCell[SW_POOL];
static LineVertex __attribute__((aligned(16))) g_swT[SW_POOL][SW_TV];
static LineVertex __attribute__((aligned(16))) g_swL[SW_POOL][SW_LV];
static int g_swPcx = 0x7FFFFFF, g_swPcz = 0x7FFFFFF;

static int swFind(int cx, int cz) {
    for (int s = 0; s < SW_POOL; ++s) if (g_swCell[s].used && g_swCell[s].cx == cx && g_swCell[s].cz == cz) return s;
    return -1;
}
static void swGenCell(int s, int cx, int cz) {
    SwOut o = { g_swT[s], 0, 0, g_swL[s], 0, 0, 0, 0, 0, 0 };
    swEmitCell(cx, cz, o);
    SwCell &c = g_swCell[s];
    c.cx = cx; c.cz = cz; c.used = 1;
    c.tn = o.tn; c.tCore = o.tCore; c.ln = o.ln; c.lCore = o.lCore; c.lBoxEnd = o.lBoxEnd;
    sceKernelDcacheWritebackRange(g_swT[s], sizeof(LineVertex) * o.tn);
    sceKernelDcacheWritebackRange(g_swL[s], sizeof(LineVertex) * o.ln);
}
static int swFreeSlot(int pcx, int pcz) {
    int far = -1, farD = -1;
    for (int s = 0; s < SW_POOL; ++s) {
        if (!g_swCell[s].used) return s;
        int dx = g_swCell[s].cx - pcx, dz = g_swCell[s].cz - pcz;
        if (dx < 0) dx = -dx; if (dz < 0) dz = -dz;
        const int d = dx > dz ? dx : dz;
        if (d > SW_R && d > farD) { farD = d; far = s; }
    }
    return far;   // -1 = todo ocupado por celdas cercanas (no pasa con SW_POOL >= 25)
}
static void swBuildCollision(int pcx, int pcz) {
    SwOut o = { 0, 0, 0, 0, 0, 0, 0, g_city, 0, 256 };
    for (int dz = -1; dz <= 1; ++dz)
        for (int dx = -1; dx <= 1; ++dx) swEmitCell(pcx + dx, pcz + dz, o);
    g_cityCount = o.bn;
}

// cada frame: colision al cambiar de celda + genera hasta 'budget' celdas que falten,
// de la mas cercana a la mas lejana.
static void swUpdate(float px, float pz, int budget) {
    const int pcx = (int)floorf(px / SW_C + 0.5f), pcz = (int)floorf(pz / SW_C + 0.5f);
    if (pcx != g_swPcx || pcz != g_swPcz) {
        g_swPcx = pcx; g_swPcz = pcz;
        swBuildCollision(pcx, pcz);
    }
    for (int ring = 0; ring <= SW_R; ++ring)
        for (int dz = -ring; dz <= ring; ++dz)
            for (int dx = -ring; dx <= ring; ++dx) {
                if ((dx < 0 ? -dx : dx) != ring && (dz < 0 ? -dz : dz) != ring) continue;
                if (swFind(pcx + dx, pcz + dz) >= 0) continue;
                if (budget <= 0) return;
                const int s = swFreeSlot(pcx, pcz);
                if (s < 0) return;
                swGenCell(s, pcx + dx, pcz + dz);
                --budget;
            }
}

// ---------------------------------------------------------------- fondo: colosos
static LineVertex __attribute__((aligned(16))) g_swBgBox[6000];
static LineVertex __attribute__((aligned(16))) g_swBgRing[9000];
static int g_swBgBoxN = 0, g_swBgRingN = 0;

static unsigned int swFade(unsigned int base, float t) {    // hacia la bruma
    int br = base & 0xFF, bg = (base >> 8) & 0xFF, bb = (base >> 16) & 0xFF;
    int cr = SW_HAZE & 0xFF, cg = (SW_HAZE >> 8) & 0xFF, cb = (SW_HAZE >> 16) & 0xFF;
    return RGBA(br + (int)((cr - br) * t), bg + (int)((cg - bg) * t), bb + (int)((cb - bb) * t), 255);
}

// (los colosos usan swBoxLit con cBot = casi bruma: salen de la niebla, cima oscura)

// FONDO: solo ENGRANAJES colosales lejos (Benjamin: fuera las torres; estructuras grandes pero a lo
// lejos). Cada uno se arma en su espacio local (eje +Z) y se dibuja con su propia matriz: mira al
// centro y GIRA lento sobre su eje. Siguen al jugador como un skybox con volumen.
struct SwGear { int start, count; float x, y, z, yaw, spin; };
static SwGear g_swGear[5];
static void swBuildBackground() {
    int i = 0, j = 0;
    const unsigned int dark = RGBA(66, 72, 84, 255);
    for (int k = 0; k < 5; ++k) {
        const float ang = 6.2831853f * k / 5.0f + 0.4f + (swRand(k, 3, 89) - 0.5f) * 0.5f;
        const float r = 400.0f + 70.0f * swRand(k, 3, 86);
        const float R = 95.0f + 55.0f * swRand(k, 3, 87);
        const unsigned int c = swFade(dark, 0.56f + 0.12f * (r - 400.0f) / 70.0f);
        SwGear &g = g_swGear[k];
        g.x = sinf(ang) * r; g.z = -cosf(ang) * r; g.y = 30.0f + 100.0f * swRand(k, 3, 88);
        g.yaw = 3.14159265f - ang;                                    // su eje apunta al centro
        g.spin = (0.0015f + 0.0020f * swRand(k, 3, 90)) * ((k & 1) ? 1.0f : -1.0f);
        g.start = j;
        swRing(g_swBgRing, j, 9000, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, R, 0.10f * R, 0.075f * R, 44, 0.05f * R, c);
        addLimb(g_swBgRing, j, 0.0f, 0.0f, -0.2f * R, 0.0f, 0.0f, 0.2f * R, R * 0.14f, R * 0.14f, 8, brighten(c, 0.8f), c);
        for (int s = 0; s < 6; ++s) {
            const float a = 6.2831853f * s / 6.0f, ca = cosf(a), sa = sinf(a);
            addLimb(g_swBgRing, j, 0.0f, 0.0f, 0.0f, ca * (R - 0.06f * R), sa * (R - 0.06f * R), 0.0f,
                    0.045f * R, 0.030f * R, 6, c, brighten(c, 1.15f));
        }
        g.count = j - g.start;
    }    g_swBgBoxN = i; g_swBgRingN = j;
    sceKernelDcacheWritebackAll();
}

// ---------------------------------------------------------------- arranque / caidas
static float g_swSafeX = 0.0f, g_swSafeY = 0.0f, g_swSafeZ = 0.0f;

static void swInit(float *px, float *py, float *pz) {
    for (int s = 0; s < SW_POOL; ++s) g_swCell[s].used = 0;
    swBuildBackground();
    *px = 0.0f; *pz = 0.0f;
    SwLayout L; swLayout(0, 0, L);
    *py = L.top;
    g_swSafeX = *px; g_swSafeY = *py; g_swSafeZ = *pz;
    swUpdate(*px, *pz, 99);                      // todo de una al arrancar
}
// Recuerda el ultimo punto pisado; si caes 45 por debajo, vuelves al CENTRO de la losa de
// esa celda. Volver al punto exacto era volver al BORDE: con el stick adelante se caia otra
// vez en bucle (visto en el emulador).
static bool swFallRespawn(float &px, float &py, float &pz, int grounded) {
    if (grounded) { g_swSafeX = px; g_swSafeY = py; g_swSafeZ = pz; return false; }
    if (py > g_swSafeY - 45.0f) return false;
    const int cx = (int)floorf(g_swSafeX / SW_C + 0.5f), cz = (int)floorf(g_swSafeZ / SW_C + 0.5f);
    SwLayout L; swLayout(cx, cz, L);
    if (L.type == 1) { px = L.x; pz = L.z; py = L.top + 0.5f; }
    else             { px = g_swSafeX; pz = g_swSafeZ; py = g_swSafeY + 0.5f; }   // (pisaba un puente)
    return true;
}

// ---------------------------------------------------------------- dibujo
static void swBackdrop() {   // cielo (2D): azul grisaceo arriba, blanco de bruma en el horizonte
    // gris plomo de tormenta: oscuro arriba, el horizonte apenas mas claro (va a llover)
    gradQuad(0, 120, RGBA(78, 83, 92, 255), RGBA(142, 146, 152, 255));
    gradQuad(120, SCR_HEIGHT, RGBA(142, 146, 152, 255), RGBA(118, 122, 130, 255));
}

static int g_swVtx = 0;   // vertices dibujados este frame (SELECT lo muestra en miles: KV)

static void swDraw(float px, float pz, float fwdX, float fwdZ) {
    g_swVtx = g_swBgBoxN + g_swBgRingN;
    // 1) fondo: sigue al jugador en XZ, sin niebla HW (color ya horneado)
    sceGuDisable(GU_TEXTURE_2D);
    sceGumLoadIdentity();
    { ScePspFVector3 t = { px, 0.0f, pz }; sceGumTranslate(&t); }
    sceGuEnable(GU_CULL_FACE);
    if (g_swBgBoxN > 0) sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g_swBgBoxN, 0, g_swBgBox);
    sceGuDisable(GU_CULL_FACE);
    static int s_frame = 0; ++s_frame;
    for (int k = 0; k < 5; ++k) {                          // engranajes: miran al centro y giran lento
        const SwGear &g = g_swGear[k];
        sceGumLoadIdentity();
        ScePspFVector3 t = { px + g.x, g.y, pz + g.z }; sceGumTranslate(&t);
        sceGumRotateY(g.yaw);
        sceGumRotateZ(g.spin * s_frame);
        sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g.count, 0, g_swBgRing + g.start);
    }
    sceGumLoadIdentity();

    // 2) celdas con niebla HW
    sceGuFog(SW_FOG_NEAR, SW_FOG_FAR, SW_HAZE);
    sceGuEnable(GU_FOG);
    // Hormigon LISO como en la referencia: sin textura (y la GE no lee texeles: menos relleno).
    // El sombreado por cara de la caja (techo claro, costados 0.82 / 0.60) da el volumen.
    sceGuDisable(GU_TEXTURE_2D);
    sceGuEnable(GU_CULL_FACE);
    int ringOf[SW_POOL];
    for (int s = 0; s < SW_POOL; ++s) {
        ringOf[s] = -1;
        const SwCell &c = g_swCell[s];
        if (!c.used) continue;
        int dx = c.cx - g_swPcx, dz = c.cz - g_swPcz;
        if (dx < 0) dx = -dx; if (dz < 0) dz = -dz;
        const int ring = dx > dz ? dx : dz;
        if (ring > SW_R) continue;
        // detras de la mirada: fuera (salvo las vecinas, que pueden asomar por los lados)
        const float wx = c.cx * SW_C - px, wz = c.cz * SW_C - pz;
        if (ring >= 1 && wx * fwdX + wz * fwdZ < -SW_C * 0.75f) continue;
        ringOf[s] = ring;
        // SIN LOD: el anillo exterior se dibujaba solo con su nucleo y pretiles/monolitos
        // "aparecian" al acercarse (se leia como el mundo a medio cargar). Ya cabe completo.
        const int n = c.tn;
        if (n > 0) { sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, n, 0, g_swT[s]); g_swVtx += n; }
    }
    for (int s = 0; s < SW_POOL; ++s) {
        if (ringOf[s] < 0) continue;
        const SwCell &c = g_swCell[s];
        const int n = c.lBoxEnd;
        if (n > 0) { sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, n, 0, g_swL[s]); g_swVtx += n; }
    }
    sceGuDisable(GU_CULL_FACE);
    for (int s = 0; s < SW_POOL; ++s) {
        if (ringOf[s] < 0) continue;
        const SwCell &c = g_swCell[s];
        if (c.ln > c.lBoxEnd) { sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, c.ln - c.lBoxEnd, 0, g_swL[s] + c.lBoxEnd); g_swVtx += c.ln - c.lBoxEnd; }
    }
    sceGuDisable(GU_FOG);
}
