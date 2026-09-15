#pragma once
// PROJECT NOCTIS -- TELON DE FONDO: BOSQUE DENSISIMO DE AGUJAS GOTICAS EN NIEBLA.
// -----------------------------------------------------------------------------
// Que es: el mar de siluetas que se ve POR LAS ABERTURAS del recinto y al subir
// sobre las masas. El recinto util llega a r = 58 y sus muros suben a y = 60;
// TODO esto vive MAS ALLA, en r = 120..220, y solo existe para vender "la
// megaestructura sigue mucho mas alla de lo explorable".
//
// Referencia visual (objetivo no negociable): bosque DENSISIMO de agujas y
// campanarios que se pierde en bruma gris-parda calida; CIENTOS de siluetas
// casi negras SUPERPUESTAS en capas, cada capa mas tenue que la anterior, con
// puntitos ambar de ventana encendida.
//
// Como se consigue con 3528 vertices:
//
//   1) CINCO ANILLOS, NO TRES. 12 / 16 / 20 / 26 / 32 siluetas en radios
//      120..138 / 138..158 / 158..180 / 180..200 / 200..220. En cualquier
//      direccion hay CINCO profundidades apiladas: mires donde mires, detras de
//      una aguja hay otra, y detras otra. La sensacion de "cientos" la da la
//      SUPERPOSICION, no el detalle.
//
//   2) EL VERTICE SE GASTA DONDE SE VE. El coste por silueta cae con la
//      distancia: el anillo 0 se permite campanarios de 3 piezas (72 v) porque
//      ocupa media pantalla; el anillo 4 es UN SOLO CONO de 12 v porque a esa
//      distancia, y con 90% de niebla, una aguja ES un triangulo gris. Salen
//      106 siluetas (124 remates contando las gemelas) por menos vertices de los
//      que gastarian 50 torres detalladas.
//
//   3) SEIS SILUETAS DISTINTAS para que el horizonte sea IRREGULAR y no un
//      peine: campanario (cuerpo + sala de campanas + aguja), torre con
//      CONTRAFUERTES insinuados (basamento ensanchado + fuste esbelto + aguja),
//      aguja simple, BLOQUE MACIZO sin aguja (rompe el ritmo vertical), gemelas
//      (dos conos desiguales) y fantasma (un cono solo).
//
//   4) RACIMOS, NO REPARTO UNIFORME. La secuencia aurea reparte perfecto, y eso
//      se ve artificial. Aqui el angulo se DEFORMA: a2 = a + C*sin(m*a + p).
//      Como C*m < 1 el mapa sigue siendo monotono (no se cruzan siluetas) pero
//      la densidad local se multiplica por 1/(1 + C*m*cos(...)): sale ~3x mas
//      denso en los senos y ~0.6x en las crestas -> RACIMOS con CLAROS entre
//      medio, como una ciudad. Cada anillo usa su propio numero de lobulos
//      (3/3/4/5/5) y su propia fase, asi que el claro de un anillo cae sobre el
//      racimo del siguiente: NINGUNA direccion queda vacia, pero ninguna se ve
//      peinada.
//
//   5) SKYLINE ONDULADO. La altura no es solo ruido: se multiplica por
//      (1 - 0.26*cos(mismo lobulo)) -> donde hay RACIMO las torres son ALTAS
//      (nucleo catedralicio) y donde hay claro son bajas (arrabal), mas una
//      onda global de 1 lobulo que hace que un lado de la ciudad sea mas alto
//      que el otro. El perfil resulta asimetrico y reconocible.
//
//   6) NIEBLA EN DOS EJES.
//      - por DISTANCIA: fadeToVoid satura (t=1, HAZE puro) a partir de
//        dist=140, y este telon vive en 120..220: pasarle el radio crudo dejaria
//        TODO plano del mismo gris. spireFog() reescala 120..220 sobre el tramo
//        util 29.5..128 de fadeToVoid, asi que las capas caen en cinco bandas
//        SEPARADAS: 8% / 23% / 39% / 57% / 90% de niebla. Ese escalon entre
//        capas es lo unico que vende la distancia.
//      - por ALTURA: spireSky (replica de heightHaze, que se declara DESPUES de
//        este include y por eso no se puede llamar aqui). Bases oscuras contra
//        la banda luminosa del horizonte, puntas disueltas en el cielo.
//      El anillo 4 queda al 90% de HAZE = (55,65,81); la banda del horizonte que
//      pinta drawSky() es cGlow (126,118,104): aun al 90% la silueta sigue
//      siendo MAS OSCURA que el cielo, o sea que se lee. Por eso el tope es 90
//      y no 100.
//
//   7) AMBAR, MUY POCO. 8 ventanas encendidas, solo en los anillos 0 y 1 (los
//      unicos donde la niebla deja pasar color), a alturas distintas del fuste y
//      con la niebla al 40% para que no se agrisen. Son el unico acento calido:
//      si hubiera muchas leerian como estrellas, no como ventanas.
//
// GUARDIA DE RECINTO: ninguna esquina de ninguna caja puede entrar en r < 102
// (el recinto jugable llega a 58). Las anchuras se recortan con WMAX.
// Los fustes arrancan en y = -30 (bajo el horizonte) para que nunca se vea la
// "base flotando" al mirar desde arriba de una masa.
// Determinista: espiral aurea + hash entero. Sin rand, sin heap, C++17.

#include <math.h>

// hash entero de 32 bits (el overflow unsigned es comportamiento definido)
static inline unsigned int spireHash(unsigned int x) {
    x = x * 2654435761u; x ^= x >> 15;
    x = x * 2246822519u; x ^= x >> 13;
    x = x * 3266489917u; x ^= x >> 16;
    return x;
}

// hash -> [0,1)
static inline float spireF01(unsigned int h) {
    return (float)(h & 0xFFFFu) * (1.0f / 65536.0f);
}

// Distancia reescalada para fadeToVoid (a=20, b=140). El telon ocupa r=120..220,
// tramo que fadeToVoid crudo aplastaria contra el tope (r>=140 -> HAZE puro,
// todo del mismo color, sin profundidad). Estiramos 120..220 sobre 29.5..128:
//   r=120 -> t~0.08 (casi negro)   r=138 -> t~0.23
//   r=158 -> t~0.39                r=180 -> t~0.57
//   r=200 -> t~0.74                r=220 -> t~0.90 (fantasma en la bruma)
// Es la MISMA distancia real, solo estirada para que GRADUE en vez de saturar:
// cada anillo cae en su propia banda de gris y por eso se leen como capas.
static inline float spireFog(float r) {
    const float f = 29.5f + (r - 120.0f) * 0.99f;
    return (f < 0.0f) ? 0.0f : f;
}

// Niebla por ALTURA. Copia fiel de heightHaze() de main.cpp (que se define
// DESPUES de este include), recalibrada al rango de alturas del telon (hasta
// ~300) y con zona muerta bajo y=20 para que las BASES sigan oscuras: si las
// bases tambien se aclararan, la silueta perderia el contraste contra la banda
// luminosa del horizonte que pinta drawSky(). HAZE ya esta en scope.
static inline unsigned int spireSky(unsigned int base, float y) {
    float t = (y - 20.0f) * (1.0f / 300.0f);
    if (t < 0.0f)   t = 0.0f;
    if (t > 0.80f)  t = 0.80f;
    const int br = base & 0xFF, bg = (base >> 8) & 0xFF, bb = (base >> 16) & 0xFF;
    const int cr = HAZE & 0xFF, cg = (HAZE >> 8) & 0xFF, cb = (HAZE >> 16) & 0xFF;
    const int r  = br + (int)((cr - br) * t);
    const int g  = bg + (int)((cg - bg) * t);
    const int b2 = bb + (int)((cb - bb) * t);
    return RGBA(r, g, b2, 255);
}

// color final de un trozo de silueta: niebla por distancia + niebla por altura
static inline unsigned int spireMat(unsigned int base, float r, float ymid) {
    return spireSky(fadeToVoid(base, spireFog(r)), ymid);
}

// ============================= IMPOSTORES =====================================
// Todos reciben el APICE ABSOLUTO (yTop) y la base absoluta (y0 = -30).
// OJO: este pase se dibuja CON back-face culling; addSolidBox/addPyramid tienen
// el winding correcto ahi. NO usar addLimb (winding opuesto -> desapareceria).

// CAMPANARIO: cuerpo + sala de campanas (mas estrecha) + aguja.   30+30+12 = 72
static void spireBelfry(LineVertex *buf, int &i, float cx, float cz, float y0,
                        float W, float D, float yTop, float r, unsigned int base) {
    const float sp = yTop - y0;
    const float y1 = y0 + sp * 0.46f;
    const float y2 = y0 + sp * 0.76f;
    addSolidBox(buf, i, cx, y0, cz, W, D, y1 - y0,
                spireMat(base, r, (y0 + y1) * 0.5f));
    addSolidBox(buf, i, cx, y1, cz, W * 0.68f, D * 0.68f, y2 - y1,
                spireMat(brighten(base, 1.07f), r, (y1 + y2) * 0.5f));
    addPyramid (buf, i, cx, y2, cz, W * 0.68f, D * 0.68f, yTop - y2,
                spireMat(brighten(base, 1.13f), r, (y2 + yTop) * 0.5f));
}

// TORRE CON CONTRAFUERTES insinuados: basamento ensanchado + fuste esbelto +
// aguja. El escalon ancho-a-estrecho a 1/3 de altura es lo que lee como
// arbotante a esta distancia (un contrafuerte de verdad costaria 3 cajas mas).
//                                                                30+30+12 = 72
static void spireButtress(LineVertex *buf, int &i, float cx, float cz, float y0,
                          float W, float D, float yTop, float r, unsigned int base) {
    const float sp = yTop - y0;
    const float yb = y0 + sp * 0.30f;
    const float ys = y0 + sp * 0.72f;
    addSolidBox(buf, i, cx, y0, cz, W * 1.34f, D * 1.34f, yb - y0,
                spireMat(brighten(base, 0.90f), r, (y0 + yb) * 0.5f));
    addSolidBox(buf, i, cx, yb, cz, W, D, ys - yb,
                spireMat(base, r, (yb + ys) * 0.5f));
    addPyramid (buf, i, cx, ys, cz, W, D, yTop - ys,
                spireMat(brighten(base, 1.11f), r, (ys + yTop) * 0.5f));
}

// AGUJA: fuste + piramide. El caballo de batalla de los anillos medios. 30+12=42
static void spireNeedle(LineVertex *buf, int &i, float cx, float cz, float y0,
                        float W, float D, float yTop, float r, unsigned int base,
                        float fSpire) {
    const float ys = y0 + (yTop - y0) * fSpire;
    addSolidBox(buf, i, cx, y0, cz, W, D, ys - y0,
                spireMat(base, r, (y0 + ys) * 0.5f));
    addPyramid (buf, i, cx, ys, cz, W, D, yTop - ys,
                spireMat(brighten(base, 1.06f), r, (ys + yTop) * 0.5f));
}

// BLOQUE MACIZO con remate escalonado, SIN aguja. Uno de cada cuatro en el
// anillo cercano: es lo que impide que el horizonte sea un peine de triangulos.
//                                                                   30+30 = 60
static void spireBlock(LineVertex *buf, int &i, float cx, float cz, float y0,
                       float W, float D, float yTop, float r, unsigned int base) {
    const float y1 = y0 + (yTop - y0) * 0.82f;
    addSolidBox(buf, i, cx, y0, cz, W, D, y1 - y0,
                spireMat(base, r, (y0 + y1) * 0.5f));
    addSolidBox(buf, i, cx, y1, cz, W * 0.80f, D * 0.80f, yTop - y1,
                spireMat(brighten(base, 1.09f), r, (y1 + yTop) * 0.5f));
}

// GEMELAS: dos conos DESIGUALES separados en tangencial. Silueta inconfundible
// de fachada gotica por 24 v, y duplica las puntas del horizonte.   12+12 = 24
static void spireTwin(LineVertex *buf, int &i, float cx, float cz,
                      float tx, float tz, float y0,
                      float W, float D, float yTop, float r, unsigned int base,
                      float drop) {
    const float sep = W * 0.72f;    // se rozan sin fundirse: leen como DOS
    addPyramid(buf, i, cx - tx * sep, y0, cz - tz * sep, W, D, yTop - y0,
               spireMat(base, r, y0 + (yTop - y0) * 0.40f));
    addPyramid(buf, i, cx + tx * sep, y0, cz + tz * sep, W * 0.88f, D * 0.88f,
               (yTop - drop) - y0,
               spireMat(brighten(base, 1.07f), r, y0 + ((yTop - drop) - y0) * 0.40f));
}

// FANTASMA: un cono solo, de la base al apice. Esbeltez ~14:1. Es TODO lo que
// queda de una aguja a 200 unidades con 75-90% de niebla, y cuesta 12 v.     12
static void spireGhost(LineVertex *buf, int &i, float cx, float cz, float y0,
                       float W, float D, float yTop, float r, unsigned int base) {
    addPyramid(buf, i, cx, y0, cz, W, D, yTop - y0,
               spireMat(base, r, y0 + (yTop - y0) * 0.40f));
}

static int buildSpirescape(LineVertex *buf) {
    int i = 0;

    const float GA    = 2.39996f;   // angulo aureo: minima discrepancia en 1D
    const float YBASE = -30.0f;     // los fustes nacen bajo el horizonte

    // pizarra CALIDA casi negra. Es deliberado que el material sea calido y la
    // niebla (HAZE) fria: lo cercano tira a pardo-negro, lo lejano a gris-azul.
    const unsigned int SLATE = warmTint(RGBA(26, 20, 16, 255));   // ~(31,20,13)
    // ambar de ventana. Se pre-aclara porque addSolidBox oscurece las caras
    // laterales (x0.82 / x0.60) y son justamente las que se ven de lejos.
    const unsigned int EMBER = brighten(RGBA(190, 112, 44, 255), 1.20f);

    // ---- tabla de anillos: cerca -> lejos ----
    const int   RN [5] = {  12,    16,    20,    26,    32    };  // siluetas
    const float RR0[5] = { 120.f, 138.f, 158.f, 180.f, 200.f };   // radio minimo
    const float RRS[5] = {  18.f,  20.f,  22.f,  20.f,  20.f };   // ancho del anillo
    const float RH0[5] = {  52.f,  60.f,  70.f,  80.f,  88.f };   // altura minima
    const float RHS[5] = {  92.f, 102.f, 112.f, 122.f, 130.f };   // rango de altura
    const float RCL[5] = { 0.22f, 0.20f, 0.16f, 0.12f, 0.10f };   // amplitud de racimo
    const int   RLO[5] = {   3,     3,     4,     5,     5    };  // lobulos (racimos/vuelta)
    const float RPH[5] = { 0.00f, 1.05f, 2.30f, 3.55f, 4.80f };   // fase de los racimos
    const float RSE[5] = { 0.60f, 0.90f, 1.95f, 3.10f, 4.45f };   // fase de la espiral
    const float RJI[5] = { 0.26f, 0.20f, 0.16f, 0.12f, 0.10f };   // jitter angular (rad)

    enum { K_BELFRY, K_BUTTRESS, K_NEEDLE, K_BLOCK, K_TWIN, K_GHOST };

    unsigned int seed = 0u;
    for (int q = 0; q < 5; ++q) {
        for (int j = 0; j < RN[q]; ++j, ++seed) {
            const unsigned int hA = spireHash(seed * 9u + 11u);   // jitter angular
            const unsigned int hR = spireHash(seed * 9u + 12u);   // radio
            const unsigned int hH = spireHash(seed * 9u + 13u);   // altura
            const unsigned int hW = spireHash(seed * 9u + 14u);   // ancho / fondo
            const unsigned int hC = spireHash(seed * 9u + 15u);   // tono

            // ---- RACIMOS: se deforma el angulo aureo. C*m < 1 -> monotono (no
            //      se cruzan), pero la densidad local va de ~0.6x (claro) a ~3x
            //      (racimo). Cada anillo con su lobulo y su fase, asi que el
            //      claro de uno cae sobre el racimo del de atras.
            const float a0 = (float)j * GA + RSE[q];
            const float lw = (float)RLO[q] * a0 + RPH[q];
            const float a  = a0 + RCL[q] * sinf(lw)
                           + (spireF01(hA) - 0.5f) * RJI[q];
            const float ca = cosf(a), sa = sinf(a);
            const float tx = -sa,     tz = ca;              // versor tangencial

            const float r  = RR0[q] + spireF01(hR) * RRS[q];
            const float cx = ca * r, cz = sa * r;

            // ---- SKYLINE: alto en el racimo (nucleo catedralicio), bajo en el
            //      claro (arrabal), mas una onda de 1 lobulo que inclina toda la
            //      ciudad hacia un lado. Perfil asimetrico, no un peine.
            float hf = 1.0f - 0.26f * cosf(lw);
            hf *= 0.96f + 0.14f * cosf(a0 + 0.90f);
            float yTop = (RH0[q] + spireF01(hH) * RHS[q]) * hf;

            // ---- que silueta toca. Las mezclas estan elegidas para que el
            //      anillo 0 tenga de todo (incluido el bloque sin aguja) y los
            //      lejanos sean casi solo conos: alli el detalle no llega.
            int kind;
            if (q == 0) {
                const int m = j & 3;
                kind = (m == 0) ? K_BELFRY : (m == 1) ? K_BUTTRESS
                     : (m == 2) ? K_NEEDLE : K_BLOCK;                  // 3/3/3/3
            } else if (q == 1) {
                const int m = j & 7;
                kind = (m == 0) ? K_BELFRY : (m == 4) ? K_BUTTRESS
                     : (j & 1)  ? K_NEEDLE : K_TWIN;                   // 2/2/8/4
            } else if (q == 2) {
                const int m = j % 5;
                kind = ((j % 10) == 0) ? K_BELFRY : (m == 4) ? K_GHOST
                     : (m == 3) ? K_TWIN : K_NEEDLE;                   // 2/4/4/10
            } else if (q == 3) {
                const int m = j % 6;
                kind = (m == 0 || m == 3) ? K_NEEDLE
                     : (m == 1 || m == 2) ? K_TWIN : K_GHOST;          // 9/9/8
            } else {
                kind = ((j & 7) == 0) ? K_TWIN : K_GHOST;              // 4/28
            }

            // ---- planta. La esbeltez crece con la altura pero SIN pasarse: a
            //      esta distancia una aguja de 1 pixel no existe.
            float W, D;
            if (kind == K_BELFRY) {
                W = 10.0f + yTop * 0.060f + (float)(hW % 4u);
                D = W * (0.82f + (float)((hW >> 5) % 4u) * 0.07f);
            } else if (kind == K_BUTTRESS) {
                W =  6.0f + yTop * 0.042f + (float)(hW % 3u);
                D = W * (0.86f + (float)((hW >> 5) % 3u) * 0.07f);
            } else if (kind == K_BLOCK) {
                yTop *= 0.60f;                              // achaparrado a proposito
                W = 11.0f + yTop * 0.085f + (float)(hW % 4u);
                D = W * (0.72f + (float)((hW >> 5) % 5u) * 0.06f);
            } else if (kind == K_NEEDLE) {
                W =  7.0f + yTop * 0.055f + (float)(hW % 4u);
                D = W * (0.78f + (float)((hW >> 5) % 5u) * 0.06f);
            } else if (kind == K_TWIN) {
                W =  5.0f + yTop * 0.038f + (float)(hW % 3u);
                D = W * (0.84f + (float)((hW >> 5) % 3u) * 0.08f);
            } else {
                W =  6.0f + yTop * 0.045f + (float)(hW % 4u);
                D = W * (0.80f + (float)((hW >> 5) % 4u) * 0.07f);
            }

            // ---- GUARDIA DE RECINTO. La caja NO esta rotada, asi que su esquina
            //      mas interior queda a r - 0.707*max(W,D). Con max(W,D)*bulk
            //      <= (r-102)*1.40 esa esquina nunca baja de r=102, y el recinto
            //      jugable (r<=58, muros incluidos) queda libre con 44 de margen.
            //      Solo llega a morder en el anillo 0 (WMAX=25.2 a r=120).
            const float WMAX = (r - 102.0f) * 1.40f;
            const float bulk = (kind == K_BUTTRESS) ? 1.34f : 1.0f;
            const float big  = ((W > D) ? W : D) * bulk;
            if (big > WMAX) { const float s = WMAX / big; W *= s; D *= s; }

            // leve variacion de tono por silueta (r>=g>=b conservado: nunca frio)
            const unsigned int base = brighten(SLATE, 0.90f + spireF01(hC) * 0.22f);

            switch (kind) {
                case K_BELFRY:
                    spireBelfry  (buf, i, cx, cz, YBASE, W, D, yTop, r, base);
                    break;
                case K_BUTTRESS:
                    spireButtress(buf, i, cx, cz, YBASE, W, D, yTop, r, base);
                    break;
                case K_NEEDLE:
                    spireNeedle  (buf, i, cx, cz, YBASE, W, D, yTop, r, base,
                                  0.58f + (float)((hW >> 9) % 5u) * 0.030f);
                    break;
                case K_BLOCK:
                    spireBlock   (buf, i, cx, cz, YBASE, W, D, yTop, r, base);
                    break;
                case K_TWIN:
                    spireTwin    (buf, i, cx, cz, tx, tz, YBASE, W, D, yTop, r, base,
                                  (yTop - YBASE) * (0.06f + spireF01(hW >> 3) * 0.18f));
                    break;
                default:
                    spireGhost   (buf, i, cx, cz, YBASE, W, D, yTop, r, base);
                    break;
            }

            // ---- VENTANA ENCENDIDA (8 en total, 30 v cada una) ----
            // Solo anillos 0 y 1: mas lejos la niebla ya se come el ambar y el
            // punto leeria como estrella. Se reparten por alturas distintas del
            // fuste (18%..48% del alto) para que se lean como VENTANAS de una
            // fachada y no como una guirnalda a la misma cota.
            const bool lit = (q == 0 && (j == 0 || j == 3 || j == 5 || j == 8 || j == 10))
                          || (q == 1 && (j == 1 || j == 7 || j == 13));
            if (lit) {
                const unsigned int hE = spireHash(seed * 9u + 16u);
                const float sz = 2.4f + spireF01(hE) * 1.8f;                // ~5-7 px
                const float wy = YBASE + (yTop - YBASE)
                               * (0.18f + spireF01(hE >> 8) * 0.30f);
                const float wd = ((W > D) ? W : D) * bulk * 0.5f + 1.0f;
                // pegada a la cara que MAS mira al centro del recinto (eje
                // dominante): el punto cae SOBRE la silueta, nunca flotando.
                float wx = cx, wz = cz;
                if (fabsf(ca) >= fabsf(sa)) wx = cx - ((ca >= 0.0f) ? wd : -wd);
                else                        wz = cz - ((sa >= 0.0f) ? wd : -wd);
                // niebla al 40%: la luz "atraviesa" la bruma. Sin niebla de
                // altura: una ventana no se disuelve, se apaga por distancia.
                addSolidBox(buf, i, wx, wy, wz, sz, sz, sz * 1.35f,
                            fadeToVoid(EMBER, spireFog(r) * 0.40f));
            }
        }
    }

    return i;   // 3528 vertices (limite 3600, buffer g_spire[5000])
}

// ============================ NOTA DE VERIFICACION ============================
// Anillo 0  r 120..138  12 siluetas: 3 campanario(72) + 3 contrafuerte(72)
//                                  + 3 aguja(42) + 3 bloque(60)         =  738 v
// Anillo 1  r 138..158  16 siluetas: 2 campanario + 2 contrafuerte
//                                  + 8 aguja + 4 gemelas(24)            =  720 v
// Anillo 2  r 158..180  20 siluetas: 2 campanario + 10 aguja
//                                  + 4 gemelas + 4 fantasma(12)         =  708 v
// Anillo 3  r 180..200  26 siluetas: 9 aguja + 9 gemelas + 8 fantasma   =  690 v
// Anillo 4  r 200..220  32 siluetas: 4 gemelas + 28 fantasma            =  432 v
// Ventanas ambar                      8 cajas(30)                       =  240 v
//                                                               TOTAL   = 3528 v
// 106 siluetas / 124 remates verticales. Alturas ~32..300 (el bloque va x0.60).
// Niebla por capa: 8% / 23% / 39% / 57% / 90%. Esquina mas interior: r >= 102.
// Alcance: el punto mas lejano (r=220 visto desde la esquina opuesta del
// recinto, ~76) queda a ~296 < 320 = plano lejano de la camara. Entra entero.
