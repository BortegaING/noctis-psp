// sky.h - CIELO DE SOBRECAST (gris-pardo CALIDO) para PROJECT NOCTIS.
// -----------------------------------------------------------------------------
// Se dibuja 2D, SIN profundidad, ANTES del mundo, una vez por frame:
//     sceGuDisable(GU_DEPTH_TEST);  drawSky();  sceGuEnable(GU_DEPTH_TEST);
// Estado al entrar Y al salir: textura OFF, blend OFF, cull OFF, GU_SMOOTH y
// GU_DITHER ON (initGu los deja globales). Este archivo NO toca ni un estado:
// todo lo que pinta es OPACO. Helpers con nombres propios (Sky*) porque sky.h
// se incluye ANTES de que main.cpp defina GradVertex/gradQuad.
//
// ============================ QUE SE VE AHORA ============================
// El mundo (sector.h) es un RECINTO CERRADO: suelo, techo a y=60 y muros
// exteriores hasta el techo. Del cielo solo se asoma lo que cabe por los
// VENTANALES (franja abierta y=7..27 con pilares cada 19u) y por los huecos
// entre las agujas del telon (spirescape.h). NUNCA hay cielo a pantalla
// completa: es una CINTA, y encima medio tapada por siluetas.
//
// DONDE CAE ESA CINTA EN PANTALLA (geometria real de la camara de main.cpp):
//   lookAt: ojo = P - f*9 + arriba*4.5 ; centro = P + f*4 + arriba*1.8
//     -> la vista cae 2.7u cada 13u de avance = 11.75 grados de PICADA, y es
//        FIJA (camPitch esta (void)eado en 3ra persona): no depende de nada.
//   fovy 66 -> semi 33 (cot 1.54) -> y_pantalla = 136 - 209.4*tan(elev+11.75)
//     * elev 0 (linea del ojo = HORIZONTE) .................. y = 92   <<<<<
//     * TECHO del recinto (55.5u sobre el ojo): entraria en cuadro a partir de
//       d > 143u y el sector mide 112 -> el techo NO SE VE JAMAS. Arriba del
//       todo solo hay muro, aguja o cielo.
//     * DINTEL del ventanal (22.5u sobre el ojo): se sale por el borde
//       superior en cuanto d < 58u -> cerca de un muro el cielo llega a y=0.
//     * ALFEIZAR del ventanal (2.5u sobre el ojo): d=112 -> y=88 ; d=54 ->
//       y=82 ; d=20 -> y=64. El alfeizar esta POR ENCIMA del ojo, o sea que
//       por un ventanal es IMPOSIBLE mirar por debajo de la linea y=92.
//   => el cielo UTIL es la cinta y = 0..92. Todo lo de abajo lo tapan siempre
//      el antepecho (y=0..7), el suelo del sector y las bases de las agujas
//      (que arrancan en y=-30 justamente para eso).
//
// ============================ PRESUPUESTO ============================
// La auditoria midio 239.000 px/frame (2,1 pantallas) en la version con luna +
// halo + nubes con mezcla alfa + siluetas 2D bajando hasta y=272; casi todo
// quedaba tapado por el mundo. Ahora:
//   cinta opaca 480 x 112 ........... 53.760 px  (1 escritura, 0 lecturas)
//   2 velos de bruma opacos .........  5.088 px
//   TOTAL ........................... ~58.850 px = 0,45 pantallas  (-75%)
// Sin GU_BLEND en ningun punto: a 16 bits, mezclar cuesta leer+escribir.
//
// Determinista, sin rand, sin heap, C++17. 18 quads = 36 triangulos.
#ifndef NOCTIS_SKY_H
#define NOCTIS_SKY_H

#include <pspgu.h>
#include <math.h>

// vertice 2D con color por-vertice (mismo layout que GradVertex de main.cpp)
struct SkyVtx { unsigned int color; short x, y, z; };
#define SKY_FLAGS (GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D)

static const int SKY_HORIZON = 92;   // linea del ojo (ver cuentas de arriba)
static const int SKY_HEM     = 112;  // dobladillo: 20 px BAJO el horizonte y se
                                     // corta. Es puro seguro (salto, head-bob,
                                     // gravedad rara); lo normal es que ni se
                                     // llegue a ver. Pintar hasta y=272 costaba
                                     // 76.800 px de mas para nada.
static const int SKY_STEP    = 8;    // escalones cortos: el framebuffer es 5650
                                     // (rojo/azul a 5 bits = salto de 8) y un
                                     // degradado largo de una sola tirada se
                                     // bandea. Con tramos de 8 px + el dither
                                     // 4x4 que initGu deja activo, el salto
                                     // queda repartido en ruido fino.

// ---- Rampa del cielo. SOBRECAST: gris-pardo calido, SIN sol y SIN estrellas.
// La clave de la referencia es que el HORIZONTE es lo mas luminoso y el cenit
// lo mas apagado: esa banda clara por detras es la que hace LEER las agujas
// como siluetas (spirescape.h cuenta con ella: deja las bases oscuras a
// proposito para recortarlas contra esto). Tono constante r:g:b ~ 1:0,92:0,81
// -> pardo calido en toda la rampa, nunca gris frio ni azul.
struct SkyStop { short y; unsigned char r, g, b; };
static const SkyStop kSkyStops[] = {
    {   0,  62,  56,  49 },   // cenit: carbon pardo, apagado
    {  26,  76,  68,  59 },
    {  48, 100,  90,  78 },   // empieza a levantar
    {  66, 130, 118, 102 },
    {  80, 158, 145, 126 },
    {  92, 182, 168, 148 },   // HORIZONTE: lo mas claro de la escena
    { 100, 148, 136, 118 },   // bajo el ojo cae rapido (bruma lejana)
    { 112,  92,  84,  73 },   // fin del dobladillo
};
#define SKY_NSTOPS ((int)(sizeof(kSkyStops) / sizeof(kSkyStops[0])))

// color de la rampa en una fila cualquiera (interpolacion lineal por tramos).
// Lo usan tanto la cinta como los velos: asi el borde de un velo puede tomar
// EXACTAMENTE el color del fondo y desvanecerse sin necesidad de alfa.
static unsigned int skyToneAt(int y) {
    if (y <= kSkyStops[0].y)
        return RGBA(kSkyStops[0].r, kSkyStops[0].g, kSkyStops[0].b, 255);
    for (int s = 1; s < SKY_NSTOPS; ++s) {
        if (y <= kSkyStops[s].y) {
            const SkyStop &a = kSkyStops[s - 1], &b = kSkyStops[s];
            const int span = (int)b.y - (int)a.y, t = y - (int)a.y;
            return RGBA((int)a.r + ((int)b.r - (int)a.r) * t / span,
                        (int)a.g + ((int)b.g - (int)a.g) * t / span,
                        (int)a.b + ((int)b.b - (int)a.b) * t / span, 255);
        }
    }
    const SkyStop &e = kSkyStops[SKY_NSTOPS - 1];
    return RGBA(e.r, e.g, e.b, 255);
}

// aclara/oscurece un tono MANTENIENDO el sesgo calido (g y b se mueven menos)
static inline unsigned int skyShift(unsigned int c, int d) {
    int r = (int)(c & 0xFF) + d;
    int g = (int)((c >> 8) & 0xFF) + (d * 15) / 16;
    int b = (int)((c >> 16) & 0xFF) + (d * 13) / 16;
    if (r < 0)   r = 0;
    if (r > 255) r = 255;
    if (g < 0)   g = 0;
    if (g > 255) g = 255;
    if (b < 0)   b = 0;
    if (b > 255) b = 255;
    return RGBA(r, g, b, 255);
}

// escribe 6 vertices (2 triangulos) de un quad con color por esquina.
// MISMO winding que gradQuad de main.cpp: TL,TR,BR / TL,BR,BL.
static inline void skyQuadTo(SkyVtx *v, int x0, int y0, int x1, int y1,
                             unsigned int c00, unsigned int c10,
                             unsigned int c11, unsigned int c01) {
    v[0] = { c00, (short)x0, (short)y0, 0 };
    v[1] = { c10, (short)x1, (short)y0, 0 };
    v[2] = { c11, (short)x1, (short)y1, 0 };
    v[3] = { c00, (short)x0, (short)y0, 0 };
    v[4] = { c11, (short)x1, (short)y1, 0 };
    v[5] = { c01, (short)x0, (short)y1, 0 };
}

// ---- VELO DE BRUMA: tira horizontal OPACA, un pelo mas clara (o mas oscura)
// que el cielo en su eje y que se funde con el fondo en los 4 bordes porque
// las esquinas usan literalmente skyToneAt(). Da capas de niebla sobre el
// horizonte sin encender GU_BLEND ni pagar una sola lectura de framebuffer.
// 4 celdas = 2.688 px con halfW=168, halfH=4.
static void skyVeil(int cy, int halfH, int cx, int halfW, int lift) {
    const int y0 = cy - halfH, y1 = cy + halfH;
    const int xL = cx - halfW, xR = cx + halfW;
    const unsigned int cT = skyToneAt(y0);          // borde superior = fondo
    const unsigned int cM = skyToneAt(cy);          // bordes laterales = fondo
    const unsigned int cB = skyToneAt(y1);          // borde inferior = fondo
    const unsigned int cC = skyShift(cM, lift);     // eje del velo (lo unico nuevo)
    SkyVtx *v = (SkyVtx *)sceGuGetMemory(sizeof(SkyVtx) * 24);
    skyQuadTo(v +  0, xL, y0, cx, cy, cT, cT, cC, cM);   // sup-izq
    skyQuadTo(v +  6, cx, y0, xR, cy, cT, cT, cM, cC);   // sup-der
    skyQuadTo(v + 12, xL, cy, cx, y1, cM, cC, cB, cB);   // inf-izq
    skyQuadTo(v + 18, cx, cy, xR, y1, cC, cM, cB, cB);   // inf-der
    sceGuDrawArray(GU_TRIANGLES, SKY_FLAGS, 24, 0, v);
}

// ============================ CIELO COMPLETO ============================
static void drawSky() {
    // ---- 1) LA CINTA: de y=0 al dobladillo, en escalones de 8 px, todos
    //         opacos y sin solaparse -> cada pixel se escribe UNA vez. Una
    //         sola llamada de dibujo (14 quads seguidos en el display list).
    const int nBand = (SKY_HEM + SKY_STEP - 1) / SKY_STEP;
    SkyVtx *v = (SkyVtx *)sceGuGetMemory(sizeof(SkyVtx) * 6 * nBand);
    for (int k = 0; k < nBand; ++k) {
        int y0 = k * SKY_STEP, y1 = y0 + SKY_STEP;
        if (y1 > SKY_HEM) y1 = SKY_HEM;
        const unsigned int cA = skyToneAt(y0), cB = skyToneAt(y1);
        skyQuadTo(v + k * 6, 0, y0, SCR_WIDTH, y1, cA, cA, cB, cB);
    }
    sceGuDrawArray(GU_TRIANGLES, SKY_FLAGS, 6 * nBand, 0, v);

    // ---- 2) DOS VELOS DE BRUMA. Deriva lentisima (periodo ~50 s) para que el
    //         cielo respire: son lo unico del cielo que no es invariante en
    //         horizontal, asi que van anchos y flojos de contraste y no
    //         delatan que el fondo no gira con la camara.
    static float skyT = 0.0f; skyT += 0.004f;
    const int d1 = (int)(sinf(skyT) * 42.0f);
    const int d2 = (int)(sinf(skyT * 0.63f + 2.1f) * 34.0f);
    skyVeil(SKY_HORIZON - 18, 4, 236 + d1, 168, +16);  // claro: refuerza el brillo del horizonte
    skyVeil(SKY_HORIZON - 36, 4, 220 + d2, 150, -12);  // oscuro: repisa de niebla mas alta -> capas

    // Nada de luna, halo, nubes con alfa ni siluetas 2D: ver la nota del final.
}

// ------------------------------- NOTA -------------------------------------
// Dibuja una cinta de sobrecast pardo-calido de y=0 a y=112 (14 escalones
// opacos de 8 px, mas claros al acercarse al horizonte y=92) y 2 velos de
// bruma opacos que se funden con el fondo por color, sin alfa.
// Coste: ~58.850 px/frame = 0,45 pantallas, contra los ~239.000 (2,1
// pantallas) de la version con luna. Sin una sola lectura de framebuffer.
// Lo quitado no se echa de menos porque el recinto es CERRADO: la luna y su
// halo caian donde hoy hay muro o techo; las nubes alfa se veian en rendijas
// de 20-40 px y solo enturbiaban la banda que recorta las agujas; y la silueta
// 2D del horizonte era un duplicado peor del telon REAL de spirescape.h, que
// ya esta ahi delante en 3D. Lo unico que se asoma por los ventanales es la
// banda luminosa sobre el horizonte, y ahi es donde fue todo el detalle.
// --------------------------------------------------------------------------

#endif // NOCTIS_SKY_H
