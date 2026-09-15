#pragma once
// ============================================================================
// PROJECT NOCTIS - GARGOLAS (enemigo principal, directiva 19-21 / docs/DESIGN.md)
// ============================================================================
// Criaturas BIOMECANICAS talladas como esculturas goticas: piedra agrietada
// cosida con placas metalicas y una luz interior tenue (un ojo o una grieta).
// NO son copias de ningun diseno existente: la lectura es "escultura de cornisa
// que resulta estar viva".
//
// Se posan en cornisas, capiteles, muros y techo, INMOVILES. Mientras no las
// despiertes son parte de la arquitectura. Cuando te detectan se despliegan,
// se despegan de la piedra y VUELAN a por ti -- tambien cuando cambias la
// direccion de la gravedad y empiezas a subir: ese es el momento en que el
// sector deja de ser un decorado.
//
// ---------------------------------------------------------------------------
// DONDE SE INCLUYE
// ---------------------------------------------------------------------------
// En main.cpp, DESPUES de: struct LineVertex; RGBA(); brighten();
// addSolidBox(); addPyramid().  No depende de sector.h / gravity.h /
// objective.h: las constantes del mundo y el vector "arriba" de cada gravedad
// estan DUPLICADOS aqui a proposito (misma politica que objective.h), asi el
// orden de inclusion nunca puede romperlo.
//
// ---------------------------------------------------------------------------
// POR QUE SOLO addSolidBox / addPyramid
// ---------------------------------------------------------------------------
// Este buffer se dibuja en el pase CON back-face culling (el de g_bridges /
// g_objMark / g_spire). addSolidBox y addPyramid tienen el winding bueno.
// addLimb/addBall/addLoft (char_prims.h) tienen el winding OPUESTO: si se
// usaran aqui, la gargola DESAPARECERIA entera. Por eso el bicho es facetado,
// a placas -- que ademas es exactamente el lenguaje que queremos (escultura).
//
// ---------------------------------------------------------------------------
// PSP DURO
// ---------------------------------------------------------------------------
//   * Sin heap, sin STL, sin rand() de libc (LCG entero propio).
//   * C++17 + <math.h> y nada mas.
//   * IA por distancia: la gargola lejana NO piensa, solo cuenta.
//   * Presupuesto de dibujo: 1782 verts en el peor caso absoluto (las 7 vivas
//     y todas cerca), tope duro GARG_VMAX = 2000.
// ============================================================================

#include <math.h>

// ============================ MUNDO (copia local) ===========================
// Recinto cerrado de sector.h. Duplicado a proposito (ver cabecera).
static const float GG_PI    =  3.14159265f;
static const float GG_HPI   =  1.57079633f;
static const float GG_M0    =  9.0f;   // borde interior de las 4 masas
static const float GG_M1    = 45.0f;   // borde exterior de las 4 masas
static const float GG_MTOP  = 30.0f;   // altura de las masas (su techo es pisable)
static const float GG_HALF  = 54.0f;   // cara interior de los muros
static const float GG_CEIL  = 60.0f;   // techo del sector

// margenes de vuelo: la gargola no se mete en la piedra ni atraviesa el muro
static const float GG_CLR     = 1.60f;   // holgura al esquivar una masa
static const float GG_LIM_XZ  = 51.5f;   // limite de vuelo en XZ
static const float GG_LIM_LO  =  1.20f;  // no roza el suelo
static const float GG_LIM_HI  = 58.60f;  // no roza el techo

// ============================ PALETA ========================================
// Piedra + metal + luz interior. La luz es lo UNICO saturado: de noche, en un
// pasillo, lo que ves primero es un punto ambar que no estaba antes.
static const unsigned int GG_STONE = RGBA(108, 104,  96, 255);  // piedra
static const unsigned int GG_DARK  = RGBA( 58,  56,  52, 255);  // piedra en sombra / grieta
static const unsigned int GG_METAL = RGBA(134, 122,  96, 255);  // placa metalica embebida
static const unsigned int GG_EMBER = RGBA(255, 138,  54, 255);  // luz interior CALIDA
static const unsigned int GG_COLD  = RGBA(120, 220, 235, 255);  // luz interior FRIA (voladora)

// atenua la luz interior: k=0 -> casi piedra (estatua), k=1 -> encendida
static unsigned int gGlow(unsigned int c, float k) {
    if (k < 0.0f) k = 0.0f;
    if (k > 1.0f) k = 1.0f;
    return brighten(c, 0.16f + 0.84f * k);
}

// ============================================================================
// 1) MARCO LOCAL  (el truco que permite posarse en CUALQUIER superficie)
// ============================================================================
// Cada gargola se modela en su propio marco ortonormal:
//    u = ARRIBA del bicho  = normal de la superficie donde se posa
//    f = hacia donde MIRA  (perpendicular a u)
//    r = su derecha        = f x u
// Las POSICIONES de cada pieza se calculan EXACTAS en ese marco (por eso el
// ala, la cola o la garra apuntan bien con cualquier orientacion). Solo las
// MEDIDAS de la caja se reparten entre los ejes del mundo (addSolidBox es
// alineada a ejes): por eso las piezas largas van PARTIDAS en tramos cortos
// -- una cadena de placas chicas traza la direccion correcta siempre.
struct GFrame {
    float cx, cy, cz;      // punto de anclaje = entre las garras (los "pies")
    float rx, ry, rz;      // derecha
    float ux, uy, uz;      // arriba del bicho
    float fx, fy, fz;      // mirada
};

// local (derecha, arriba, adelante) -> mundo
static inline void gW(const GFrame &F, float pr, float pu, float pf,
                      float *x, float *y, float *z) {
    *x = F.cx + F.rx * pr + F.ux * pu + F.fx * pf;
    *y = F.cy + F.ry * pr + F.uy * pu + F.fy * pf;
    *z = F.cz + F.rz * pr + F.uz * pr * 0.0f + F.uz * pu + F.fz * pf;
}

// CAJA centrada en el punto local. sL = ancho, sU = alto, sF = fondo. 30 verts
static void gBox(LineVertex *buf, int &i, const GFrame &F,
                 float pr, float pu, float pf,
                 float sL, float sU, float sF, unsigned int col) {
    float wx, wy, wz;
    gW(F, pr, pu, pf, &wx, &wy, &wz);
    const float ex = fabsf(F.rx) * sL + fabsf(F.ux) * sU + fabsf(F.fx) * sF;
    const float ey = fabsf(F.ry) * sL + fabsf(F.uy) * sU + fabsf(F.fy) * sF;
    const float ez = fabsf(F.rz) * sL + fabsf(F.uz) * sU + fabsf(F.fz) * sF;
    addSolidBox(buf, i, wx, wy - ey * 0.5f, wz, ex, ez, ey, col);   // addSolidBox: base en Y
}

// PUA: piramide con la base en el punto local. addPyramid solo sabe apuntar a
// +Y del MUNDO, asi que se usa unicamente para cuernos / crestas / espolones,
// donde "hacia arriba del mundo" siempre se lee bien. 12 verts
static void gPyr(LineVertex *buf, int &i, const GFrame &F,
                 float pr, float pu, float pf,
                 float sL, float sU, float sF, float h, unsigned int col) {
    float wx, wy, wz;
    gW(F, pr, pu, pf, &wx, &wy, &wz);
    const float ex = fabsf(F.rx) * sL + fabsf(F.ux) * sU + fabsf(F.fx) * sF;
    const float ez = fabsf(F.rz) * sL + fabsf(F.uz) * sU + fabsf(F.fz) * sF;
    addPyramid(buf, i, wx, wy, wz, ex, ez, h, col);
}

// ============================================================================
// 2) LOS TRES MODELOS
// ============================================================================
// Coste FIJO por variante (hay que poder presupuestar sin dibujar):
//    POSADA   7 cajas + 4 puas = 210 + 48 = 258
//    VOLADORA 8 cajas + 1 pua  = 240 + 12 = 252
//    PESADA   8 cajas + 1 pua  = 240 + 12 = 252
static const int GG_VERTS[3]  = { 258, 252, 252 };
static const int GG_FAR_VERTS = 60;     // silueta lejana: 2 cajas

// ---------------------------------------------------------------------------
// VARIANTE 0 - POSADA / VIGIA      (258 verts)
// ---------------------------------------------------------------------------
// SILUETA: un BULTO ENCOGIDO. Ancas anchas, torso hundido hacia adelante,
// cabeza metida entre dos jorobas (las alas plegadas de canto contra el lomo),
// dos cuernos y una cresta dentada. Las garras muerden el borde de la cornisa.
// Es la que NO PARECE VIVA: con el ojo apagado es un remate de piedra mas.
// Al abrirse (open 0->1) se incorpora y las jorobas se TUMBAN en dos alas.
static void gargPerched(LineVertex *buf, int &i, const GFrame &F,
                        float phase, float open, float eye) {
    const float rise = 0.55f * open;                          // se incorpora
    const float sw   = 0.04f * sinf(phase * 0.9f) * open;     // vaiven al volar
    // ancas: la masa baja que la ancla a la cornisa (piedra agrietada)
    gBox(buf, i, F,  0.00f, 0.58f + rise * 0.35f, -0.12f, 1.48f, 1.16f, 1.34f, GG_DARK);
    // torso encorvado
    gBox(buf, i, F,  0.00f, 1.42f + rise,  0.12f + sw, 1.16f, 1.28f, 1.02f, GG_STONE);
    // ALAS: plegadas = placa alta de canto; abiertas = plano lateral tumbado
    {
        const float wR  = 0.78f + 1.10f * open;                       // se separan del lomo
        const float wU  = 1.58f + rise + 0.40f * open + 0.26f * sinf(phase) * open;
        const float wSL = 0.32f + 1.42f * open;                       // ancho lateral
        const float wSU = 1.74f - 1.24f * open;                       // alto (se tumba)
        const float wSF = 0.74f + 0.36f * open;
        gBox(buf, i, F, -wR, wU, -0.16f, wSL, wSU, wSF, GG_METAL);
        gBox(buf, i, F,  wR, wU, -0.16f, wSL, wSU, wSF, GG_METAL);
    }
    // cabeza hundida, hocico hacia adelante
    gBox(buf, i, F,  0.00f, 2.30f + rise, 0.50f + sw, 0.80f, 0.64f, 0.98f, GG_STONE);
    // OJO: ranura luminosa. Apagada mientras duerme -> indistinguible de piedra.
    gBox(buf, i, F,  0.00f, 2.40f + rise, 0.94f + sw, 0.54f, 0.14f, 0.18f, gGlow(GG_EMBER, eye));
    // garras metalicas mordiendo el borde
    gBox(buf, i, F,  0.00f, 0.13f, 0.60f, 1.34f, 0.26f, 0.60f, GG_METAL);
    // cuernos + cresta dorsal dentada
    gPyr(buf, i, F, -0.30f, 2.58f + rise,  0.16f, 0.26f, 0.26f, 0.26f, 0.72f, GG_DARK);
    gPyr(buf, i, F,  0.30f, 2.58f + rise,  0.16f, 0.26f, 0.26f, 0.26f, 0.72f, GG_DARK);
    gPyr(buf, i, F,  0.00f, 1.98f + rise, -0.42f, 0.30f, 0.30f, 0.34f, 0.64f, GG_DARK);
    gPyr(buf, i, F,  0.00f, 1.36f + rise, -0.54f, 0.26f, 0.26f, 0.30f, 0.54f, GG_DARK);
}

// ---------------------------------------------------------------------------
// VARIANTE 1 - VOLADORA            (252 verts)
// ---------------------------------------------------------------------------
// SILUETA: una CRUZ. Cuerpo delgado y largo, craneo alargado con un visor
// FRIO, cola de 1.8 que la desequilibra hacia atras y, sobre todo, 6 unidades
// de envergadura en cuatro placas: la interna y la externa de cada lado baten
// con distinta amplitud, asi que el ala se DOBLA en vez de girar rigida.
// animPhase es el aleteo; con open=0 (posada en un capitel) las pliega.
static void gargFlyer(LineVertex *buf, int &i, const GFrame &F,
                      float phase, float open, float eye) {
    const float sp   = 0.25f + 0.75f * open;        // envergadura (plegada <-> abierta)
    const float beat = sinf(phase) * open;          // aleteo: solo si esta volando
    // cuerpo delgado, alargado en el eje de la mirada
    gBox(buf, i, F,  0.00f, 0.95f,  0.05f, 0.72f, 0.70f, 2.05f, GG_STONE);
    // craneo alargado
    gBox(buf, i, F,  0.00f, 1.06f,  1.34f, 0.54f, 0.48f, 0.88f, GG_DARK);
    // GRIETA LUMINOSA que le cruza la cara (la luz interior, aqui fria)
    gBox(buf, i, F,  0.00f, 1.10f,  1.74f, 0.46f, 0.13f, 0.16f, gGlow(GG_COLD, eye));
    // ala interna (bate poco)
    {
        const float aU = 1.12f + 0.46f * beat;
        gBox(buf, i, F, -1.02f * sp, aU, 0.18f, 0.25f + 1.45f * sp, 0.17f, 1.08f, GG_METAL);
        gBox(buf, i, F,  1.02f * sp, aU, 0.18f, 0.25f + 1.45f * sp, 0.17f, 1.08f, GG_METAL);
    }
    // ala externa (bate el doble -> el ala se dobla)
    {
        const float bU = 1.12f + 1.02f * beat;
        gBox(buf, i, F, -2.30f * sp, bU, -0.06f, 0.20f + 1.40f * sp, 0.14f, 0.84f, GG_STONE);
        gBox(buf, i, F,  2.30f * sp, bU, -0.06f, 0.20f + 1.40f * sp, 0.14f, 0.84f, GG_STONE);
    }
    // cola larga (contrapeso visual: es lo que la hace leer como "voladora")
    gBox(buf, i, F,  0.00f, 0.86f - 0.10f * beat, -1.85f, 0.30f, 0.30f, 1.80f, GG_DARK);
    // cresta metalica
    gPyr(buf, i, F,  0.00f, 1.26f, 1.06f, 0.22f, 0.22f, 0.44f, 0.58f, GG_METAL);
}

// ---------------------------------------------------------------------------
// VARIANTE 2 - PESADA              (252 verts)
// ---------------------------------------------------------------------------
// SILUETA: un LADRILLO CON GARRAS. Lomo macizo de 2.3 de fondo, cabeza ancha y
// BAJA pegada al suelo, dos garras enormes que se adelantan al cuerpo (es como
// se arrastra por la pared) y un espolon dorsal. La luz interior no es un ojo:
// es una GRIETA que le parte la mandibula. Lenta, pero embiste fuerte y aguanta.
static void gargHeavy(LineVertex *buf, int &i, const GFrame &F,
                      float phase, float open, float eye) {
    const float sw = 0.22f * sinf(phase * 0.7f) * open;   // vaiven pesado de la cabeza
    const float li = 0.18f * open;                        // se levanta al despegarse
    // lomo macizo
    gBox(buf, i, F,  0.00f, 0.95f + li, -0.15f, 1.92f, 1.42f, 2.30f, GG_STONE);
    // cabeza ancha y baja, adelantada
    gBox(buf, i, F,  0.00f, 0.86f + li,  1.42f + sw, 1.34f, 0.88f, 1.10f, GG_DARK);
    // GRIETA LUMINOSA de la mandibula
    gBox(buf, i, F,  0.00f, 0.74f + li,  1.92f + sw, 1.00f, 0.18f, 0.18f, gGlow(GG_EMBER, eye));
    // hombros metalicos
    gBox(buf, i, F, -1.18f, 0.62f + li,  0.82f, 0.74f, 1.16f, 0.76f, GG_METAL);
    gBox(buf, i, F,  1.18f, 0.62f + li,  0.82f, 0.74f, 1.16f, 0.76f, GG_METAL);
    // GARRAS grandes, por delante de la cabeza (van alternadas: se arrastra)
    gBox(buf, i, F, -1.44f, 0.14f + li,  1.48f + sw * 0.5f, 1.10f, 0.28f, 1.22f, GG_METAL);
    gBox(buf, i, F,  1.44f, 0.14f + li,  1.48f - sw * 0.5f, 1.10f, 0.28f, 1.22f, GG_METAL);
    // cuartos traseros
    gBox(buf, i, F,  0.00f, 0.78f + li, -1.80f, 1.28f, 1.02f, 1.10f, GG_DARK);
    // espolon dorsal
    gPyr(buf, i, F,  0.00f, 1.62f + li, -0.32f, 0.52f, 0.52f, 0.86f, 0.92f, GG_DARK);
}

// ---------------------------------------------------------------------------
// LOD lejano (60 verts): a mas de ~72 unidades la niebla ya se la come. Solo
// queda el bulto y el punto de luz -- que es lo unico que se ve de verdad.
// ---------------------------------------------------------------------------
static void gargFarBlob(LineVertex *buf, int &i, const GFrame &F, int variant, float eye) {
    const float s = (variant == 2) ? 1.35f : 1.00f;
    gBox(buf, i, F, 0.0f, 1.10f * s, 0.0f, 1.50f * s, 1.60f * s, 2.10f * s, GG_STONE);
    gBox(buf, i, F, 0.0f, 1.30f * s, 1.05f * s, 0.55f, 0.24f, 0.24f,
         gGlow((variant == 1) ? GG_COLD : GG_EMBER, eye));
}

// despacho del modelo (mismo coste que GG_VERTS[variant])
static void gargBuildOne(LineVertex *buf, int &i, int variant, float phase,
                         const GFrame &F, float open, float eye) {
    if      (variant == 1) gargFlyer  (buf, i, F, phase, open, eye);
    else if (variant == 2) gargHeavy  (buf, i, F, phase, open, eye);
    else                   gargPerched(buf, i, F, phase, open, eye);
}

// ---------------------------------------------------------------------------
// gargBuild: una gargola SUELTA en el origen, de pie, mirando a -Z, desplegada
// y con la luz encendida. Es la entrada para el visor de personajes / debug;
// el juego usa gargBuildAll(). Devuelve GG_VERTS[variant].
// ---------------------------------------------------------------------------
static int gargBuild(LineVertex *buf, int variant, float animPhase) {
    GFrame F;
    F.cx = 0.0f; F.cy = 0.0f; F.cz = 0.0f;
    F.rx = 1.0f; F.ry = 0.0f; F.rz = 0.0f;      // derecha = +X
    F.ux = 0.0f; F.uy = 1.0f; F.uz = 0.0f;      // arriba  = +Y
    F.fx = 0.0f; F.fy = 0.0f; F.fz =-1.0f;      // mira    = -Z   (r = f x u)
    int i = 0;
    gargBuildOne(buf, i, variant, animPhase, F, 1.0f, 1.0f);
    return i;
}

// ============================================================================
// 3) LA COLONIA: estado, nidos y constantes de comportamiento
// ============================================================================
struct Garg { float x, y, z, vx, vy, vz; int state, variant, hp, timer; };

enum {
    GARG_PERCHED = 0,   // POSADA:   inmovil en su nido; es una estatua
    GARG_ALERT   = 1,   // ALERTA:   te detecto, se despliega y gira hacia ti
    GARG_CHASE   = 2,   // PERSIGUE: vuela hacia ti esquivando las masas
    GARG_ATTACK  = 3,   // ATACA:    embestida corta + retirada
    GARG_HURT    = 4,   // HERIDA:   retroceso y aturdimiento
    GARG_DEAD    = 5    // MUERTA:   ni piensa ni se dibuja
};

#define GARG_N 7                    // 3 POSADA + 2 VOLADORA + 2 PESADA
static const int GARG_VMAX = 2000;  // tope DURO de vertices dibujados

// --- por variante: 0 POSADA/VIGIA, 1 VOLADORA, 2 PESADA --------------------
// La vida sale de kEnemies["Gargoyles"] de game_data.h (hp 40): la voladora es
// mas fragil y la pesada aguanta bastante mas.
static const int   GG_HP [3] = {   40,    22,    64   };
static const float GG_DET[3] = { 30.0f, 44.0f, 26.0f };  // radio de deteccion
static const float GG_SPD[3] = { 0.190f, 0.260f, 0.130f }; // crucero (RUN_SPEED del jugador = 0.22)
static const float GG_LUN[3] = { 0.420f, 0.520f, 0.460f }; // embestida
static const float GG_RAD[3] = { 1.10f, 1.00f, 1.55f };    // radio de colision
static const float GG_TRN[3] = { 0.090f, 0.140f, 0.060f }; // agilidad de giro (lerp de velocidad)

// --- tiempos (frames a ~60 fps) --------------------------------------------
static const int   GG_T_ALERT = 26;   // despliegue antes de soltarse de la piedra
static const int   GG_T_ATK   = 28;   // embestida completa...
static const int   GG_T_BACK  = 16;   // ...de la que los primeros 12 son el impacto
static const int   GG_T_HURT  = 16;   // aturdimiento
static const int   GG_T_CD    = 30;   // enfriamiento base entre embestidas

// --- distancias -------------------------------------------------------------
static const float GG_FAR_THINK = 60.0f;  // mas alla: NO piensa, solo cuenta
static const float GG_GIVE_UP   = 96.0f;  // mas alla: vuelve al nido
static const float GG_ATK_R     =  7.2f;  // entra en embestida
static const float GG_HIGH      =  3.4f;  // apunta POR ENCIMA de ti (ataca desde lo alto)
static const float GG_KNOCK     =  0.42f; // retroceso al ser herida
static const float GG_LOD_FAR   = 72.0f;  // LOD de silueta
static const float GG_LOD_CULL  = 112.0f; // ni se dibuja (la niebla la tapa)

// el dano que corresponde a un toque (kEnemies["Gargoyles"].damage)
static const int   GARG_TOUCH_DMG = 6;

// ---------------------------------------------------------------------------
// NIDOS: donde nace y a donde vuelve cada gargola.
// face = indice de gravedad con el que se PISA esa superficie (mismo convenio
// que gravity.h / objective.h). El "arriba" del bicho es la normal de la cara:
//   0 = -Y (suelo, techos de masa, capiteles)   1 = +Y (TECHO del sector)
//   2 = +X (se pisa una cara -X)                3 = -X (se pisa una cara +X)
//   4 = +Z (se pisa una cara -Z)                5 = -Z (se pisa una cara +Z)
// pitch = cuanto tiene que dar la vuelta el cuerpo para quedar derecho al
// volar (0, 90 o 180 grados). La MIRADA del nido esta elegida para que ese
// giro salga hacia el lado bueno (ver gargFrame).
// ---------------------------------------------------------------------------
struct GargNest {
    float x, y, z;        // punto de apoyo (las garras)
    float fx, fy, fz;     // hacia donde mira posada
    int   face;           // cara sobre la que se posa
    int   variant;
};

static const GargNest kGargNests[GARG_N] = {
    // --- CORNISAS de las masas (y = 30 + 1.6 de cornisa = 31.6) --------------
    // Vigilan los dos brazos del pasillo en cruz desde 30 de altura: el jugador
    // las tiene ENCIMA todo el recorrido a pie y no las ve hasta que se mueven.
    {  10.5f, 31.6f,  20.0f,  -1.0f, 0.0f,  0.0f, 0, 0 },  // G0 masa NE, mira al pasillo en Z
    { -10.5f, 31.6f, -20.0f,   1.0f, 0.0f,  0.0f, 0, 0 },  // G1 masa SO, el espejo
    {  20.0f, 31.6f, -10.5f,   0.0f, 0.0f,  1.0f, 0, 0 },  // G2 masa SE, mira al pasillo en X
    // --- CAPITEL de la arcada (y = 1.2 + 16 + 1.4 = 18.6) -------------------
    // A la altura de la cara: es la primera que se cruza sin levantar la vista.
    {   7.0f, 18.6f, -16.0f,  -1.0f, 0.0f,  0.0f, 0, 1 },  // G3 capitel (7,-16)
    // --- PAREDES (se arrastran; su "arriba" es horizontal) ------------------
    // Cara -X de la masa NE (x=9), entre dos contrafuertes (z=16.2 y z=27).
    // Justo la pared que hay que CORRER para llegar al material M0 (9,22,20).
    {   9.0f, 14.0f,  21.0f,   0.0f,-1.0f,  0.0f, 2, 2 },  // G4 mirando hacia abajo
    // Cara interior del muro +X (x=54) por encima de los ventanales (y>27),
    // que es donde se sube a buscar M8 (54,42,27).
    {  54.0f, 34.0f, -30.0f,   0.0f,-1.0f,  0.0f, 2, 2 },  // G5 mirando hacia abajo
    // --- TECHO del sector (y = 60, cabeza abajo) ----------------------------
    // Cuelga a 20 del FARO: solo la encuentra quien ya camina el techo con
    // gravedad +Y, y llega justo cuando cree que ya gano.
    {   0.0f, 60.0f, -20.0f,   0.0f, 0.0f,  1.0f, 1, 1 },  // G6 mira hacia el faro
};

// --- estado vivo ------------------------------------------------------------
static Garg          g_garg[GARG_N];
static float         g_gargOpen[GARG_N];   // 0 = plegada (estatua) .. 1 = desplegada
static unsigned char g_gargHit [GARG_N];   // ya te golpeo en esta embestida
static unsigned char g_gargLos [GARG_N];   // ultimo resultado de linea de vista
static unsigned int  g_gargRng   = 0x1B3F27u;
static int           g_gargTick  = 0;      // contador global (reparte el pensamiento)
static int           g_gargGrav  = 0;      // gravedad del jugador el frame pasado
// ultima posicion conocida del jugador: la usa el LOD de gargBuildAll y el
// centro de masa para gargHitPlayer. La escribe gargUpdate.
static float g_gargPX = 0.0f, g_gargPY = 0.0f, g_gargPZ = 0.0f;
static float g_gargUX = 0.0f, g_gargUY = 1.0f, g_gargUZ = 0.0f;

// buffer propio, listo para que main.cpp no tenga que dimensionar nada
static LineVertex __attribute__((aligned(16))) g_gargVB[GARG_VMAX];
static int g_gargVerts = 0;

// ============================================================================
// 4) UTILIDADES BARATAS
// ============================================================================
// LCG entero de 32 bits: NADA de rand() de libc (no es determinista entre
// plataformas y arrastra la libc entera). Solo se usa para desincronizar
// enfriamientos y aleteos: dos gargolas nunca laten a la vez.
static inline unsigned int gargRand() {
    g_gargRng = g_gargRng * 1103515245u + 12345u;
    return (g_gargRng >> 16) & 0x7FFFu;
}

// "arriba" del jugador para una gravedad dada (= -gravedad). Copia local para
// no depender de gravity.h (ver cabecera).
static void gargUpVec(int g, float *ux, float *uy, float *uz) {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    switch (g) {
        case 0:  y =  1.0f; break;   // gravedad -Y -> arriba +Y
        case 1:  y = -1.0f; break;   // gravedad +Y -> arriba -Y (TECHO)
        case 2:  x = -1.0f; break;   // gravedad +X -> arriba -X
        case 3:  x =  1.0f; break;
        case 4:  z = -1.0f; break;   // gravedad +Z -> arriba -Z
        case 5:  z =  1.0f; break;
        default: y =  1.0f; break;
    }
    *ux = x; *uy = y; *uz = z;
}

// LINEA DE VISTA SIMPLE: 3 muestras del segmento contra las 4 masas. No mira
// columnas ni contrafuertes a proposito (son finos: que una gargola te vea por
// el hueco de una columna es correcto). ~36 operaciones, y solo se llama 1 de
// cada 8 o 16 frames por bicho.
static int gargLOS(float ax, float ay, float az, float bx, float by, float bz) {
    const float dx = bx - ax, dy = by - ay, dz = bz - az;
    for (int k = 1; k <= 3; ++k) {
        const float t = 0.25f * (float)k;
        const float y = ay + dy * t;
        if (y > GG_MTOP) continue;                   // por encima de las masas: pasa
        const float fx = fabsf(ax + dx * t);
        if (fx < GG_M0 || fx > GG_M1) continue;
        const float fz = fabsf(az + dz * t);
        if (fz < GG_M0 || fz > GG_M1) continue;
        return 0;                                    // la muestra cayo DENTRO de una masa
    }
    return 1;
}

// ESQUIVA: si el punto quedo dentro de una masa, lo saca por la salida mas
// corta (X, Z o por arriba) y mata la velocidad en ese eje para que no insista.
// Solo cuesta algo cuando de verdad choco.
static void gargAvoid(Garg &g) {
    // muros / suelo / techo del sector
    if (g.x >  GG_LIM_XZ) { g.x =  GG_LIM_XZ; if (g.vx > 0.0f) g.vx = 0.0f; }
    if (g.x < -GG_LIM_XZ) { g.x = -GG_LIM_XZ; if (g.vx < 0.0f) g.vx = 0.0f; }
    if (g.z >  GG_LIM_XZ) { g.z =  GG_LIM_XZ; if (g.vz > 0.0f) g.vz = 0.0f; }
    if (g.z < -GG_LIM_XZ) { g.z = -GG_LIM_XZ; if (g.vz < 0.0f) g.vz = 0.0f; }
    if (g.y <  GG_LIM_LO) { g.y =  GG_LIM_LO; if (g.vy < 0.0f) g.vy = 0.0f; }
    if (g.y >  GG_LIM_HI) { g.y =  GG_LIM_HI; if (g.vy > 0.0f) g.vy = 0.0f; }

    // las 4 masas: |x| y |z| ambos en [9,45] y por debajo de 30
    if (g.y > GG_MTOP + GG_CLR) return;
    const float ax = fabsf(g.x), az = fabsf(g.z);
    if (ax < GG_M0 - GG_CLR || ax > GG_M1 + GG_CLR) return;
    if (az < GG_M0 - GG_CLR || az > GG_M1 + GG_CLR) return;

    // tres salidas posibles; se toma la mas corta
    const float outXin  = ax - (GG_M0 - GG_CLR);       // hacia el pasillo en cruz
    const float outXout = (GG_M1 + GG_CLR) - ax;       // hacia el perimetral
    const float outZin  = az - (GG_M0 - GG_CLR);
    const float outZout = (GG_M1 + GG_CLR) - az;
    const float outUp   = (GG_MTOP + GG_CLR) - g.y;    // por encima del tejado
    float best = outUp; int mode = 0;
    if (outXin  < best) { best = outXin;  mode = 1; }
    if (outXout < best) { best = outXout; mode = 2; }
    if (outZin  < best) { best = outZin;  mode = 3; }
    if (outZout < best) { best = outZout; mode = 4; }
    const float sx = (g.x < 0.0f) ? -1.0f : 1.0f;
    const float sz = (g.z < 0.0f) ? -1.0f : 1.0f;
    switch (mode) {
        case 1: g.x = sx * (GG_M0 - GG_CLR); g.vx = 0.0f; break;
        case 2: g.x = sx * (GG_M1 + GG_CLR); g.vx = 0.0f; break;
        case 3: g.z = sz * (GG_M0 - GG_CLR); g.vz = 0.0f; break;
        case 4: g.z = sz * (GG_M1 + GG_CLR); g.vz = 0.0f; break;
        default: g.y = GG_MTOP + GG_CLR;     g.vy = 0.0f; break;
    }
}

// ============================================================================
// 5) gargInit
// ============================================================================
static void gargInit() {
    g_gargRng  = 0x1B3F27u;
    g_gargTick = 0;
    g_gargGrav = 0;
    for (int k = 0; k < GARG_N; ++k) {
        const GargNest &n = kGargNests[k];
        Garg &g = g_garg[k];
        g.x = n.x; g.y = n.y; g.z = n.z;
        g.vx = g.vy = g.vz = 0.0f;
        g.state   = GARG_PERCHED;
        g.variant = n.variant;
        g.hp      = GG_HP[n.variant];
        g.timer   = (int)(gargRand() & 31);   // el parpadeo de deteccion, desfasado
        g_gargOpen[k] = 0.0f;                 // todas nacen PLEGADAS: son estatuas
        g_gargHit [k] = 0;
        g_gargLos [k] = 0;
    }
}

// ============================================================================
// 6) gargUpdate - la maquina de estados (1 vez por frame, ANTES de dibujar)
// ============================================================================
// COSTE: una gargola lejana y dormida son ~8 operaciones (resta, cuadrado,
// comparacion, contador). Solo las que estan en juego integran velocidad, y son
// dos o tres como mucho. La linea de vista se reparte: cada bicho la calcula 1
// de cada 8 frames (posada) o 1 de cada 16 (persiguiendo).
static void gargUpdate(float px, float py, float pz, int gravG) {
    ++g_gargTick;

    float upx, upy, upz;
    gargUpVec(gravG, &upx, &upy, &upz);
    g_gargPX = px;  g_gargPY = py;  g_gargPZ = pz;
    g_gargUX = upx; g_gargUY = upy; g_gargUZ = upz;

    // CAMBIASTE LA GRAVEDAD: el sector entero lo nota. Las gargolas cercanas se
    // despiertan aunque no te vean -- por eso subir deja de ser gratis.
    const int gravFlip = (gravG != g_gargGrav);
    g_gargGrav = gravG;

    // centro del cuerpo del jugador (px,py,pz son los PIES; el cuerpo mide 3)
    const float cx = px + upx * 1.5f, cy = py + upy * 1.5f, cz = pz + upz * 1.5f;

    for (int k = 0; k < GARG_N; ++k) {
        Garg &g = g_garg[k];
        if (g.state == GARG_DEAD) continue;

        const int   v   = g.variant;
        const float dx  = cx - g.x, dy = cy - g.y, dz = cz - g.z;
        const float d2  = dx * dx + dy * dy + dz * dz;

        // ---- LEJOS Y TRANQUILA: no piensa, solo cuenta -----------------------
        // (las que ya estan en juego siguen simulando aunque te alejes: son 2 o 3)
        if (d2 > GG_FAR_THINK * GG_FAR_THINK && g.state <= GARG_ALERT) {
            ++g.timer;
            if (g_gargOpen[k] > 0.0f) g_gargOpen[k] -= 0.03f;   // se vuelve a plegar
            else                      g_gargOpen[k]  = 0.0f;
            continue;
        }

        // ---- despliegue suave (mueve el modelo y la luz interior) ------------
        {
            float tgt = (g.state == GARG_PERCHED) ? 0.0f : 1.0f;
            if (g.state == GARG_PERCHED) {                  // ...salvo que vuelva al nido
                const GargNest &n = kGargNests[k];
                const float ndx = n.x - g.x, ndy = n.y - g.y, ndz = n.z - g.z;
                if (ndx * ndx + ndy * ndy + ndz * ndz > 0.36f) tgt = 1.0f;
            }
            g_gargOpen[k] += (tgt - g_gargOpen[k]) * 0.14f;
        }

        switch (g.state) {

        // =====================================================================
        // POSADA: es una escultura. Lo unico que hace es volver a su sitio si
        // la corrieron, y mirar de reojo 1 de cada 8 frames.
        // =====================================================================
        case GARG_PERCHED: {
            const GargNest &n = kGargNests[k];
            const float ndx = n.x - g.x, ndy = n.y - g.y, ndz = n.z - g.z;
            const float nd2 = ndx * ndx + ndy * ndy + ndz * ndz;
            if (nd2 > 0.36f) {                              // regreso planeando
                const float inv = GG_SPD[v] * 0.9f / sqrtf(nd2);
                g.vx = ndx * inv; g.vy = ndy * inv; g.vz = ndz * inv;
                g.x += g.vx; g.y += g.vy; g.z += g.vz;
                gargAvoid(g);
            } else {                                        // posada de verdad
                g.x = n.x; g.y = n.y; g.z = n.z;
                g.vx = g.vy = g.vz = 0.0f;
            }
            ++g.timer;
            const float det = GG_DET[v];
            if (gravFlip && d2 < (det * 1.7f) * (det * 1.7f)) {   // el cambio de gravedad DESPIERTA
                g.state = GARG_ALERT; g.timer = GG_T_ALERT;
                break;
            }
            if ((g.timer & 7) != 0) break;                  // solo piensa 1 de cada 8 frames
            if (d2 > det * det) break;
            g_gargLos[k] = (unsigned char)gargLOS(g.x, g.y, g.z, cx, cy, cz);
            if (g_gargLos[k]) { g.state = GARG_ALERT; g.timer = GG_T_ALERT; }
            break;
        }

        // =====================================================================
        // ALERTA: se despliega en el sitio y gira hacia ti. Son ~26 frames de
        // aviso: la piedra cruje, el ojo se enciende y todavia puedes irte.
        // =====================================================================
        case GARG_ALERT: {
            g.vx *= 0.85f; g.vy *= 0.85f; g.vz *= 0.85f;
            if (--g.timer <= 0) {
                g.state = GARG_CHASE;
                g.timer = 8;                                  // enfriamiento corto inicial
            } else if (d2 > (GG_DET[v] * 2.2f) * (GG_DET[v] * 2.2f)) {
                g.state = GARG_PERCHED; g.timer = 0;          // te fuiste: se vuelve a dormir
            }
            break;
        }

        // =====================================================================
        // PERSIGUE: vuela hacia un punto POR ENCIMA de ti (en TU vertical, la
        // que marca tu gravedad) para caerte desde arriba. Si no tiene linea de
        // vista sube a volar por encima de las masas en vez de darse contra una.
        // =====================================================================
        case GARG_CHASE: {
            if (g.timer > 0) --g.timer;                       // enfriamiento de embestida

            if ((g_gargTick & 15) == (k & 15))                // LOS repartida
                g_gargLos[k] = (unsigned char)gargLOS(g.x, g.y, g.z, cx, cy, cz);

            float tx = cx + upx * GG_HIGH;
            float ty = cy + upy * GG_HIGH;
            float tz = cz + upz * GG_HIGH;
            if (!g_gargLos[k] && ty < GG_MTOP + 6.0f) ty = GG_MTOP + 6.0f;  // sobrevuela la masa

            const float ax = tx - g.x, ay = ty - g.y, az = tz - g.z;
            const float a2 = ax * ax + ay * ay + az * az;
            if (a2 > 1e-4f) {
                const float inv = GG_SPD[v] / sqrtf(a2);
                const float tn  = GG_TRN[v];
                g.vx += (ax * inv - g.vx) * tn;               // giro suave (inercia)
                g.vy += (ay * inv - g.vy) * tn;
                g.vz += (az * inv - g.vz) * tn;
            }
            g.x += g.vx; g.y += g.vy; g.z += g.vz;
            gargAvoid(g);

            if (d2 > GG_GIVE_UP * GG_GIVE_UP) {               // te perdio
                g.state = GARG_PERCHED; g.timer = 0;
            } else if (g.timer <= 0 && d2 < GG_ATK_R * GG_ATK_R && g_gargLos[k]) {
                g.state = GARG_ATTACK;                        // EMBISTE
                g.timer = GG_T_ATK;
                g_gargHit[k] = 0;
                const float inv = GG_LUN[v] / sqrtf(d2 > 1e-4f ? d2 : 1e-4f);
                g.vx = dx * inv; g.vy = dy * inv; g.vz = dz * inv;
            }
            break;
        }

        // =====================================================================
        // ATACA: 12 frames de embestida recta (es cuando golpea) y 16 de
        // retirada hacia atras. La retirada es lo que la hace legible: te da la
        // ventana para responder en vez de quedarse pegada encima.
        // =====================================================================
        case GARG_ATTACK: {
            if (g.timer > GG_T_BACK) {                        // ---- embestida ----
                g.vx *= 0.99f; g.vy *= 0.99f; g.vz *= 0.99f;
            } else {                                          // ---- retirada ----
                const float inv = -GG_LUN[v] * 0.55f / sqrtf(d2 > 1e-4f ? d2 : 1e-4f);
                g.vx += (dx * inv - g.vx) * 0.22f;
                g.vy += (dy * inv - g.vy) * 0.22f;
                g.vz += (dz * inv - g.vz) * 0.22f;
            }
            g.x += g.vx; g.y += g.vy; g.z += g.vz;
            gargAvoid(g);
            if (--g.timer <= 0) {
                g.state = GARG_CHASE;
                g.timer = GG_T_CD + (int)(gargRand() & 31);   // desfasa a las de al lado
            }
            break;
        }

        // =====================================================================
        // HERIDA: retroceso + aturdimiento. Sale SIEMPRE persiguiendo: herirla
        // no la ahuyenta, la enfada.
        // =====================================================================
        case GARG_HURT: {
            g.vx *= 0.88f; g.vy *= 0.88f; g.vz *= 0.88f;
            g.x += g.vx; g.y += g.vy; g.z += g.vz;
            gargAvoid(g);
            if (--g.timer <= 0) { g.state = GARG_CHASE; g.timer = 10; }
            break;
        }

        default: break;
        }
    }
}

// ============================================================================
// 7) gargBuildAll - geometria de las VIVAS (1 vez por frame, DESPUES de update)
// ============================================================================
// Arma el marco de cada gargola y la dibuja. La clave esta en `open`:
//   open = 0 -> marco EXACTO del nido (posada en su cara, sin trigonometria):
//               una estatua quieta, perfectamente pegada a la piedra.
//   open > 0 -> el cuerpo hace la voltereta que lo pone derecho (pitch alrededor
//               de su propia derecha) y la mirada pasa a apuntar a donde vuela.
// Devuelve el numero de vertices escritos (<= GARG_VMAX).
static int gargBuildAll(LineVertex *buf, float t) {
    int i = 0;
    for (int k = 0; k < GARG_N; ++k) {
        const Garg &g = g_garg[k];
        if (g.state == GARG_DEAD) continue;

        const int   v   = g.variant;
        const float ddx = g.x - g_gargPX, ddy = g.y - g_gargPY, ddz = g.z - g_gargPZ;
        const float dd2 = ddx * ddx + ddy * ddy + ddz * ddz;
        if (dd2 > GG_LOD_CULL * GG_LOD_CULL) continue;                 // la niebla la tapa

        const int  far  = (dd2 > GG_LOD_FAR * GG_LOD_FAR);
        const int  cost = far ? GG_FAR_VERTS : GG_VERTS[v];
        if (i + cost > GARG_VMAX) break;                               // tope duro

        // ---- marco del nido -------------------------------------------------
        const GargNest &n = kGargNests[k];
        float ux, uy, uz;
        gargUpVec(n.face, &ux, &uy, &uz);          // arriba = normal de la cara
        float fx = n.fx, fy = n.fy, fz = n.fz;     // mirada posada
        float rx = fy * uz - fz * uy;              // r = f x u
        float ry = fz * ux - fx * uz;
        float rz = fx * uy - fy * ux;

        const float open = g_gargOpen[k];

        // ---- al despegarse, el cuerpo se endereza ---------------------------
        // Gira u y f alrededor de r (r x u = -f, r x f = u), justo el angulo que
        // hace falta para que "arriba" acabe siendo +Y del mundo:
        //   nido en el suelo/cornisa/capitel (u=+Y) -> 0     (no hay giro: estatua exacta)
        //   nido en una pared              (u horizontal) -> 90 grados
        //   nido en el techo               (u=-Y)         -> 180 grados (voltereta)
        if (open > 0.001f) {
            const float thMax = (uy >  0.5f) ? 0.0f : ((uy < -0.5f) ? GG_PI : GG_HPI);
            if (thMax > 0.0f) {
                const float th = thMax * open, c = cosf(th), s = sinf(th);
                const float nux = ux * c - fx * s, nuy = uy * c - fy * s, nuz = uz * c - fz * s;
                const float nfx = fx * c + ux * s, nfy = fy * c + uy * s, nfz = fz * c + uz * s;
                ux = nux; uy = nuy; uz = nuz;
                fx = nfx; fy = nfy; fz = nfz;
            }
            // ---- y mira hacia donde va ---------------------------------------
            // Rumbo = su velocidad si vuela, o el jugador si esta frenando.
            float hx = g.vx, hy = g.vy, hz = g.vz;
            if (hx * hx + hy * hy + hz * hz < 1e-4f) {
                hx = g_gargPX - g.x; hy = g_gargPY - g.y; hz = g_gargPZ - g.z;
            }
            // mezcla con la mirada del nido (evita el tiron al soltarse) y se
            // ortogonaliza contra el nuevo "arriba"
            float mx = fx * (1.0f - open) + hx * open;
            float my = fy * (1.0f - open) + hy * open;
            float mz = fz * (1.0f - open) + hz * open;
            const float dot = mx * ux + my * uy + mz * uz;
            mx -= ux * dot; my -= uy * dot; mz -= uz * dot;
            const float m2 = mx * mx + my * my + mz * mz;
            if (m2 > 1e-5f) {
                const float inv = 1.0f / sqrtf(m2);
                fx = mx * inv; fy = my * inv; fz = mz * inv;
                rx = fy * uz - fz * uy;
                ry = fz * ux - fx * uz;
                rz = fx * uy - fy * ux;
            }
        }

        GFrame F;
        F.cx = g.x; F.cy = g.y; F.cz = g.z;
        F.rx = rx;  F.ry = ry;  F.rz = rz;
        F.ux = ux;  F.uy = uy;  F.uz = uz;
        F.fx = fx;  F.fy = fy;  F.fz = fz;

        // ---- luz interior ---------------------------------------------------
        // Dormida esta casi apagada (0.10): a simple vista es piedra. Se
        // enciende con el despliegue y DESTELLA cuando la hieres.
        float eye = 0.10f + 0.90f * open;
        if (g.state == GARG_HURT) eye = 1.0f;
        eye *= 0.84f + 0.16f * sinf(t * 3.1f + (float)k * 1.9f);

        if (far) {
            gargFarBlob(buf, i, F, v, eye);
        } else {
            // aleteo: ritmo propio por variante y desfase propio por bicho
            const float rate  = (v == 1) ? 7.4f : ((v == 2) ? 3.1f : 5.2f);
            const float phase = t * rate + (float)k * 1.73f;
            gargBuildOne(buf, i, v, phase, F, open, eye);
        }
    }
    return i;
}

// ============================================================================
// 8) COLISION CON EL JUGADOR
// ============================================================================
// Cuantas gargolas te han alcanzado ESTE frame. Solo cuenta durante los 12
// frames de embestida y solo UNA vez por embestida (g_gargHit), asi que una
// misma gargola no te vacia la vida por quedarse pegada encima.
// (px,py,pz) = los mismos que se le pasan a gargUpdate (pies del jugador);
// r = radio del jugador (~1.0 para la caja de 1.4x1.4).
// El dano por toque sugerido es GARG_TOUCH_DMG (= kEnemies["Gargoyles"].damage).
static int gargHitPlayer(float px, float py, float pz, float r) {
    const float cx = px + g_gargUX * 1.5f;      // centro del cuerpo, no los pies
    const float cy = py + g_gargUY * 1.5f;
    const float cz = pz + g_gargUZ * 1.5f;
    int hits = 0;
    for (int k = 0; k < GARG_N; ++k) {
        Garg &g = g_garg[k];
        if (g.state != GARG_ATTACK) continue;
        if (g.timer <= GG_T_BACK)   continue;   // ya se esta retirando: no golpea
        if (g_gargHit[k])           continue;   // este envite ya conecto
        const float dx = g.x - cx, dy = g.y - cy, dz = g.z - cz;
        const float rr = r + GG_RAD[g.variant];
        if (dx * dx + dy * dy + dz * dz < rr * rr) { g_gargHit[k] = 1; ++hits; }
    }
    return hits;
}

// ============================================================================
// 9) DANO (balas, sable, ataques gravitacionales)
// ============================================================================
// Esfera de dano en (x,y,z) de radio r. Una bala pasa r pequeno (~0.5); un
// mandoble, r grande (~3). La que sobrevive entra en HERIDA con retroceso y
// SALE persiguiendo; la que cae pasa a MUERTA y deja de existir (ni piensa ni
// se dibuja). No devuelve nada a proposito: quien quiera contar bajas puede
// mirar g_garg[k].state.
static void gargDamage(float x, float y, float z, float r, int dmg) {
    for (int k = 0; k < GARG_N; ++k) {
        Garg &g = g_garg[k];
        if (g.state == GARG_DEAD) continue;
        const float dx = g.x - x, dy = g.y - y, dz = g.z - z;
        const float d2 = dx * dx + dy * dy + dz * dz;
        const float rr = r + GG_RAD[g.variant];
        if (d2 > rr * rr) continue;

        g.hp -= dmg;
        if (g.hp <= 0) {
            g.state = GARG_DEAD;
            g.vx = g.vy = g.vz = 0.0f;
            continue;
        }
        // retroceso: la empuja ALEJANDOLA del impacto
        const float inv = GG_KNOCK / sqrtf(d2 > 1e-4f ? d2 : 1e-4f);
        g.vx = dx * inv; g.vy = dy * inv; g.vz = dz * inv;
        g.state = GARG_HURT;
        g.timer = GG_T_HURT;
        g_gargHit[k]  = 1;          // corta la embestida en curso
        g_gargLos[k]  = 1;          // si te dispara, te ha visto
        g_gargOpen[k] = 1.0f;       // y desde luego ya no es una estatua
    }
}

// ============================================================================
// NOTA DE CABLEADO (para main.cpp)
// ============================================================================
// SILUETAS. POSADA/VIGIA: bulto encogido, alas de canto contra el lomo, cuernos
//   y cresta; con el ojo apagado es un remate de cornisa. VOLADORA: una cruz de
//   6 de envergadura en 4 placas que baten desfasadas, cuerpo fino y cola de
//   1.8. PESADA: ladrillo de 2.3 de fondo, cabeza baja, dos garras enormes por
//   delante y una grieta ambar en la mandibula.
// ESTADOS. POSADA -(distancia + linea de vista, o cambio de gravedad cerca)->
//   ALERTA -(26 frames)-> PERSIGUE -(a menos de 7.2 y sin enfriamiento)->
//   ATACA -(28 frames: 12 de embestida + 16 de retirada)-> PERSIGUE.
//   gargDamage manda a HERIDA (16 frames) y de ahi vuelve a PERSIGUE, o a
//   MUERTA si hp<=0. PERSIGUE a mas de 96 vuelve a POSADA (planea a su nido).
// VERTICES. POSADA 258, VOLADORA 252, PESADA 252, silueta lejana 60. Con 7
//   gargolas el peor caso absoluto es 1782; tope duro GARG_VMAX = 2000.
// POR FRAME, EN ESTE ORDEN:
//   1) una sola vez al arrancar:  gargInit();
//   2) logica:                    gargUpdate(px, py, pz, gravG);
//   3) golpes recibidos:          int n = gargHitPlayer(px, py, pz, 1.0f);
//                                 if (n) vida -= n * GARG_TOUCH_DMG;
//   4) dano que repartes (cuando dispares/golpees): gargDamage(hx,hy,hz,r,dmg);
//   5) geometria:                 g_gargVerts = gargBuildAll(g_gargVB, idleT);
//   6) dibujo, en el pase CON culling (junto a g_objMark, ANTES del
//      sceGuDisable(GU_CULL_FACE) de los robots):
//      sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g_gargVerts, 0, g_gargVB);
// ============================================================================
