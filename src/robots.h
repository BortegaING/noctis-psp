#pragma once
// PROJECT NOCTIS - HABITANTES del sector: ROBOTS / ANDROIDES (la humanidad ya no
// existe, directiva 8-9,35). NO son todos militares: cada uno es un ROL con
// SILUETA propia y un ESTADO DE CONSERVACION distinto (pulido / oxidado /
// remendado / desvaido). Estetica de personaje estilizada contra el mundo
// brutal de piedra (contraste intencional).
//
// Se incluye en main.cpp DESPUES de char_prims.h (addLimb/addBall/addLoft: nada
// de cajas para los cuerpos) y de los helpers addSolidBox/addPyramid/brighten.
// El buffer es LineVertex (GU_TRIANGLES). Pies en y=0, coords CRUDAS (sin WSCALE).
//
// ---- DONDE VAN (ver sector.h) --------------------------------------------
// El mundo es un SECTOR CERRADO de pasillos goticos:
//   * 4 MASAS solidas de 36x36 con |x| y |z| ambos entre 9 y 45  -> PROHIBIDO.
//   * PASILLO EN CRUZ: |x| < 9 (a lo largo de Z) y |z| < 9 (a lo largo de X).
//   * COLUMNAS exentas en |x|=7 (z = 0,+-16,+-32) y |z|=7 (x = 0,+-16,+-32),
//     lado 3.3 (capitel 3.6) -> hueco libre ~+-1.8 alrededor de cada centro.
//   * CONTRAFUERTES que salen de las masas hacia el pasillo: ocupan la banda
//     |x| = 6.6..9.0 en z = +-16.2,+-27,+-37.8 (y su transpuesta).
// Por eso todos los robots van con |x| <= ~6.0 (o |z| <= ~6.0) -> arrimados al
// borde del pasillo, junto a la arcada, PERO en los huecos entre columna y
// contrafuerte. El jugador nace en (0,0,0): el centro queda libre.
//
// PSP DURO: determinista (sin rand/heap), C++17, <math.h>.
// Presupuesto: 1662 verts en total (buffer g_npc[2400]). buildRobots() lo devuelve.

#include <math.h>

// ---- paleta: estados de conservacion (directiva) ----
static const unsigned int R_STEEL = RGBA(150, 158, 170, 255); // acero
static const unsigned int R_PALE  = RGBA(202, 206, 212, 255); // metal palido PULIDO (mensajero)
static const unsigned int R_BRASS = RGBA(120,  96,  60, 255); // bronce OXIDADO (guardian)
static const unsigned int R_RUST  = RGBA(110,  70,  50, 255); // herrumbre (mecanico)
static const unsigned int R_TEAL  = RGBA( 70, 150, 150, 255); // patina / acento frio
static const unsigned int R_EYEW  = RGBA(255, 180,  90, 255); // ojo CALIDO (ambar)
static const unsigned int R_EYEC  = RGBA(120, 225, 235, 255); // ojo FRIO (cian)

// =====================================================================
// MARCO LOCAL DEL ROBOT
// Cada robot se modela en coordenadas propias: r = su derecha, f = hacia donde
// MIRA, y = arriba. Asi un robot del pasillo X y uno del pasillo Z quedan bien
// orientados (brazos al costado, visor al frente) con el mismo codigo.
// =====================================================================
struct RFrame { float cx, cz, fx, fz, rx, rz; };

static RFrame rFrame(float cx, float cz, float fx, float fz) {
    float d = sqrtf(fx * fx + fz * fz);
    if (d < 1e-4f) { fx = 0.0f; fz = -1.0f; d = 1.0f; }
    RFrame F;
    F.cx = cx; F.cz = cz;
    F.fx = fx / d; F.fz = fz / d;
    F.rx = -F.fz; F.rz = F.fx;          // derecha = forward girado 90 grados
    return F;
}
// mira al cruce del pasillo (donde nace el jugador)
static RFrame rFrameToCenter(float cx, float cz) { return rFrame(cx, cz, -cx, -cz); }

static inline float rWX(const RFrame &F, float r, float f) { return F.cx + F.rx * r + F.fx * f; }
static inline float rWZ(const RFrame &F, float r, float f) { return F.cz + F.rz * r + F.fz * f; }

// cilindro conico entre dos puntos LOCALES (brazos, piernas, torso, baston). sides*12 verts
static void rLimb(LineVertex *buf, int &i, const RFrame &F,
                  float r0, float f0, float y0, float r1, float f1, float y1,
                  float rad0, float rad1, int sides, unsigned int cB, unsigned int cT) {
    addLimb(buf, i, rWX(F, r0, f0), y0, rWZ(F, r0, f0),
                    rWX(F, r1, f1), y1, rWZ(F, r1, f1), rad0, rad1, sides, cB, cT);
}
// elipsoide facetado en local; radL = a lo ancho, radY = alto, radF = a lo largo. st*sl*6 verts
static void rBall(LineVertex *buf, int &i, const RFrame &F, float r, float f, float y,
                  float radL, float radY, float radF, int st, int sl, unsigned int col) {
    float ex = fabsf(F.rx) * radL + fabsf(F.fx) * radF;
    float ez = fabsf(F.rz) * radL + fabsf(F.fz) * radF;
    addBall(buf, i, rWX(F, r, f), y, rWZ(F, r, f), ex, radY, ez, st, sl, col);
}
// caja alineada a ejes pero DIMENSIONADA segun la orientacion (placas, mochila). 30 verts
static void rBox(LineVertex *buf, int &i, const RFrame &F, float r, float f, float baseY,
                 float wL, float wF, float h, unsigned int col) {
    float w = fabsf(F.rx) * wL + fabsf(F.fx) * wF;
    float d = fabsf(F.rz) * wL + fabsf(F.fz) * wF;
    addSolidBox(buf, i, rWX(F, r, f), baseY, rWZ(F, r, f), w, d, h, col);
}
// piramide orientada (cresta, antena, punta de herramienta). 12 verts
static void rPyr(LineVertex *buf, int &i, const RFrame &F, float r, float f, float baseY,
                 float wL, float wF, float apexH, unsigned int col) {
    float w = fabsf(F.rx) * wL + fabsf(F.fx) * wF;
    float d = fabsf(F.rz) * wL + fabsf(F.fz) * wF;
    addPyramid(buf, i, rWX(F, r, f), baseY, rWZ(F, r, f), w, d, apexH, col);
}
// OJO luminoso: cubito brillante centrado en ey (calido o frio segun el robot). 30 verts
static void rEye(LineVertex *buf, int &i, const RFrame &F, float r, float f, float ey,
                 float w, float h, unsigned int col) {
    rBox(buf, i, F, r, f, ey - h * 0.5f, w, w * 0.55f, h, col);
}

// =====================================================================
// 1) MENSAJERO  -- 282 verts
//    Alto (3.6) y DELGADO, extremidades finas, cabeza-farol ovoide con antena.
//    Metal palido PULIDO: el androide en mejor estado, elegante. Lleva el
//    mensaje en alto con la mano derecha. Ojo calido.
// =====================================================================
static void buildMessenger(LineVertex *buf, int &i, const RFrame &F) {
    const unsigned int lo = brighten(R_STEEL, 0.78f);
    rLimb(buf, i, F, -0.17f, 0.00f, 0.00f, -0.13f, 0.02f, 1.76f, 0.13f, 0.09f, 3, lo, R_STEEL);       // pierna L      36
    rLimb(buf, i, F,  0.17f, 0.00f, 0.00f,  0.13f, 0.02f, 1.76f, 0.13f, 0.09f, 3, lo, R_STEEL);       // pierna R      36
    rLimb(buf, i, F,  0.00f, 0.00f, 1.70f,  0.00f, 0.03f, 3.02f, 0.27f, 0.16f, 4, R_STEEL, R_PALE);   // torso esbelto 48
    rLimb(buf, i, F, -0.23f, 0.02f, 2.92f, -0.31f, 0.10f, 1.92f, 0.075f, 0.05f, 3, R_PALE, R_STEEL);  // brazo L fino  36
    rLimb(buf, i, F,  0.23f, 0.02f, 2.92f,  0.26f, 0.34f, 2.12f, 0.075f, 0.05f, 3, R_PALE, R_STEEL);  // brazo R (ofrece) 36
    rBall(buf, i, F,  0.00f, 0.02f, 3.30f,  0.17f, 0.22f, 0.19f, 2, 4, R_PALE);                       // cabeza-farol  48
    rPyr (buf, i, F,  0.00f, 0.00f, 3.50f,  0.10f, 0.10f, 0.60f, brighten(R_PALE, 1.10f));            // antena        12
    rEye (buf, i, F,  0.00f, 0.17f, 3.30f,  0.10f, 0.10f, R_EYEW);                                    // ojo calido    30
}

// =====================================================================
// 2) MECANICO  -- 282 verts
//    ENCORVADO (torso inclinado hacia adelante), brazo izquierdo macizo y brazo
//    derecho convertido en HERRAMIENTA que se afila en punta caliente. Oxidado,
//    con una pierna de repuesto en acero que no combina (remiendo). Ojo calido.
// =====================================================================
static void buildMechanic(LineVertex *buf, int &i, const RFrame &F) {
    const unsigned int patch = brighten(R_STEEL, 0.92f);   // pieza NUEVA, no combina
    rLimb(buf, i, F, -0.23f, 0.00f, 0.00f, -0.19f, 0.00f, 1.18f, 0.18f, 0.14f, 3, brighten(R_RUST, 0.80f), R_RUST); // pierna L oxidada 36
    rLimb(buf, i, F,  0.23f, 0.00f, 0.00f,  0.19f, 0.00f, 1.18f, 0.18f, 0.14f, 3, brighten(patch, 0.80f), patch);   // pierna R remendada 36
    rLimb(buf, i, F,  0.00f, -0.06f, 1.12f, 0.00f, 0.42f, 2.14f, 0.42f, 0.32f, 4, R_RUST, brighten(R_RUST, 1.18f)); // torso encorvado 48
    rLimb(buf, i, F, -0.42f, 0.38f, 2.04f, -0.53f, 0.30f, 1.02f, 0.17f, 0.13f, 4, R_RUST, brighten(R_RUST, 1.10f)); // brazo macizo L  48
    rLimb(buf, i, F,  0.40f, 0.40f, 2.00f,  0.47f, 0.86f, 1.30f, 0.12f, 0.03f, 3, patch, brighten(R_TEAL, 1.10f));  // brazo-herramienta 36
    rPyr (buf, i, F,  0.47f, 0.86f, 1.20f,  0.13f, 0.13f, 0.24f, R_EYEW);                                           // punta caliente  12
    rBall(buf, i, F,  0.00f, 0.52f, 2.24f,  0.24f, 0.20f, 0.26f, 2, 3, brighten(R_RUST, 1.20f));                    // cabeza baja     36
    rEye (buf, i, F,  0.00f, 0.74f, 2.26f,  0.12f, 0.09f, R_EYEW);                                                  // ojo calido      30
}

// =====================================================================
// 3) GUARDIAN  -- 378 verts
//    ANCHO y pesado (3.5 de alto, hombros a 1.4): torso acorazado abombado por
//    LOFT, hombreras, brazos gruesos, casco con cresta. Bronce OXIDADO con
//    patina teal: arcaico, no militar moderno. Visor frio ancho.
// =====================================================================
static void buildGuardian(LineVertex *buf, int &i, const RFrame &F) {
    const unsigned int dk = brighten(R_BRASS, 0.78f);
    static const float gy[3] = { 1.38f, 2.22f, 2.94f };   // cintura -> pecho -> cuello
    static const float gr[3] = { 0.50f, 0.66f, 0.42f };
    rLimb(buf, i, F, -0.34f, 0.00f, 0.00f, -0.30f, 0.00f, 1.44f, 0.27f, 0.22f, 3, dk, R_BRASS);   // pierna L gruesa 36
    rLimb(buf, i, F,  0.34f, 0.00f, 0.00f,  0.30f, 0.00f, 1.44f, 0.27f, 0.22f, 3, dk, R_BRASS);   // pierna R gruesa 36
    addLoft(buf, i, F.cx, F.cz, gy, gr, 3, 4, R_BRASS, brighten(R_BRASS, 1.15f));                 // torso acorazado 96
    rLimb(buf, i, F, -0.66f, 0.00f, 2.36f, -0.70f, 0.16f, 1.32f, 0.16f, 0.13f, 3, R_BRASS, dk);   // brazo L         36
    rLimb(buf, i, F,  0.66f, 0.00f, 2.36f,  0.70f, 0.16f, 1.32f, 0.16f, 0.13f, 3, R_BRASS, dk);   // brazo R         36
    rBox (buf, i, F, -0.62f, 0.00f, 2.30f,  0.36f, 0.44f, 0.36f, brighten(R_BRASS, 0.92f));       // hombrera L      30
    rBox (buf, i, F,  0.62f, 0.00f, 2.30f,  0.36f, 0.44f, 0.36f, brighten(R_BRASS, 0.92f));       // hombrera R      30
    rBall(buf, i, F,  0.00f, 0.02f, 3.04f,  0.28f, 0.26f, 0.30f, 2, 3, brighten(R_BRASS, 1.20f)); // casco           36
    rPyr (buf, i, F,  0.00f, 0.00f, 3.26f,  0.14f, 0.46f, 0.40f, R_TEAL);                         // cresta patinada 12
    rEye (buf, i, F,  0.00f, 0.26f, 3.06f,  0.34f, 0.08f, R_EYEC);                                // visor frio      30
}

// =====================================================================
// 4) DRON  -- 138 verts
//    Cuerpo OVOIDE que FLOTA: no tiene piernas, su punto mas bajo queda en
//    y~1.25. Un solo OJO grande frio adelante. Barato: es el que se ve de
//    lejos al fondo del pasillo.
// =====================================================================
static void buildDrone(LineVertex *buf, int &i, const RFrame &F) {
    rBall(buf, i, F, 0.00f, 0.00f, 1.78f, 0.40f, 0.30f, 0.48f, 3, 4, R_STEEL);   // ovoide flotante 72
    rBall(buf, i, F, 0.00f, 0.00f, 1.38f, 0.16f, 0.13f, 0.16f, 2, 3, R_TEAL);    // pod sensor      36
    rEye (buf, i, F, 0.00f, 0.44f, 1.80f, 0.18f, 0.16f, R_EYEC);                 // ojo unico frio  30
}

// =====================================================================
// 5) ANCIANO  -- 294 verts
//    Figura ENCAPUCHADA: tunica acampanada construida con LOFT (4 anillos),
//    cogulla conica, baston de bronce. Acero desvaido, digno y desgastado.
//    Ojo frio, chico y hundido bajo la capucha.
// =====================================================================
static void buildAncient(LineVertex *buf, int &i, const RFrame &F) {
    const unsigned int robeB = brighten(R_STEEL, 0.58f);   // desvaido abajo (polvo)
    const unsigned int robeT = brighten(R_STEEL, 0.92f);
    static const float ay[4] = { 0.00f, 0.72f, 1.80f, 2.62f };
    static const float ar[4] = { 0.85f, 0.76f, 0.46f, 0.38f };
    addLoft(buf, i, F.cx, F.cz, ay, ar, 4, 4, robeB, robeT);                                           // tunica    144
    rLimb(buf, i, F, 0.00f, 0.00f, 2.60f, 0.00f, 0.04f, 3.08f, 0.34f, 0.20f, 4, robeT, brighten(R_STEEL, 0.80f)); // cogulla 48
    rBall(buf, i, F, 0.00f, 0.06f, 3.16f, 0.19f, 0.23f, 0.21f, 2, 3, brighten(R_STEEL, 0.70f));        // cabeza     36
    rLimb(buf, i, F, 0.56f, 0.06f, 0.00f, 0.50f, 0.10f, 3.24f, 0.055f, 0.045f, 3, R_BRASS, brighten(R_BRASS, 1.20f)); // baston 36
    rEye (buf, i, F, 0.00f, 0.19f, 3.16f, 0.085f, 0.085f, R_EYEC);                                     // ojo frio   30
}

// =====================================================================
// 6) TECNICO  -- 288 verts
//    Estatura media, MOCHILA de instrumentos a la espalda y brazo derecho
//    levantado con un instrumento (acento teal). Acero con patina: en servicio,
//    ni pulido ni podrido. Ojo frio.
// =====================================================================
static void buildTechnician(LineVertex *buf, int &i, const RFrame &F) {
    const unsigned int lo = brighten(R_STEEL, 0.80f);
    rLimb(buf, i, F, -0.17f, 0.00f, 0.00f, -0.15f, 0.00f, 1.44f, 0.15f, 0.11f, 3, lo, R_STEEL);      // pierna L   36
    rLimb(buf, i, F,  0.17f, 0.00f, 0.00f,  0.15f, 0.00f, 1.44f, 0.15f, 0.11f, 3, lo, R_STEEL);      // pierna R   36
    rLimb(buf, i, F,  0.00f, 0.00f, 1.40f,  0.00f, 0.02f, 2.72f, 0.26f, 0.20f, 4, R_STEEL, R_TEAL);  // torso      48
    rBox (buf, i, F,  0.00f, -0.28f, 1.72f, 0.42f, 0.26f, 0.62f, brighten(R_STEEL, 0.80f));          // mochila    30
    rLimb(buf, i, F, -0.26f, 0.00f, 2.62f, -0.30f, -0.06f, 1.78f, 0.08f, 0.055f, 3, R_STEEL, lo);    // brazo L    36
    rLimb(buf, i, F,  0.26f, 0.00f, 2.62f,  0.33f, 0.34f, 1.94f, 0.08f, 0.055f, 3, R_STEEL, R_TEAL); // brazo-instrumento 36
    rBall(buf, i, F,  0.00f, 0.02f, 2.82f,  0.18f, 0.19f, 0.20f, 2, 3, brighten(R_STEEL, 1.12f));    // cabeza     36
    rEye (buf, i, F,  0.00f, 0.19f, 2.84f,  0.12f, 0.10f, R_EYEC);                                   // ojo frio   30
}

// =====================================================================
// buildRobots: 6 habitantes repartidos por los 4 brazos del pasillo en cruz,
// siempre en suelo libre (y=0) y arrimados al borde, en el HUECO entre columna
// y contrafuerte. Determinista (sin rand). Devuelve el total de verts = 1662.
//
//   robot        pos (x,z)        r      brazo del pasillo / hueco usado
//   MENSAJERO   (  5.8,  22.5)  23.2   +Z, lado E (cols z=16,32 / contraf. 16.2,27)
//   ANCIANO     ( -5.9,  11.5)  12.9   +Z, lado O, cerca del cruce
//   GUARDIAN    ( -5.5, -21.5)  22.2   -Z, lado O, de cara al cruce
//   DRON        (  5.2, -41.6)  41.9   -Z, fondo del pasillo, FLOTANDO
//   MECANICO    ( 20.5,  -5.6)  21.3   +X, lado N, reparando la pared
//   TECNICO     (-23.5,   5.6)  24.2   -X, lado S
// Ninguno cae en una masa (una de las dos coordenadas siempre < 6.2 en modulo,
// las masas empiezan en 9) ni pisa columna (centros en |7| con z/x multiplo de
// 16) ni contrafuerte (bandas 6.6..9.0 en +-16.2/+-27/+-37.8). Centro libre.
// =====================================================================
static int buildRobots(LineVertex *buf) {
    int i = 0;
    buildMessenger (buf, i, rFrameToCenter(   5.8f,  22.5f));                    // 282
    buildAncient   (buf, i, rFrameToCenter(  -5.9f,  11.5f));                    // 294
    buildGuardian  (buf, i, rFrameToCenter(  -5.5f, -21.5f));                    // 378
    buildDrone     (buf, i, rFrameToCenter(   5.2f, -41.6f));                    // 138
    buildMechanic  (buf, i, rFrame       (  20.5f,  -5.6f, -0.55f, -0.83f));     // 282 (mira la pared que repara)
    buildTechnician(buf, i, rFrameToCenter( -23.5f,   5.6f));                    // 288
    return i;   // 1662 <= 1700 (buffer g_npc[2400])
}
