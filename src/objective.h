#pragma once
// ============================================================================
// PROJECT NOCTIS - objective.h
// EL BUCLE DE JUEGO: anclas (checkpoints), materiales (suben la EN maxima) y
// el FARO de la cima. Estado + logica pura + la geometria para dibujarlos.
//
// Es la pieza 4-5-6 del planteamiento de ESTADO.md ("Piezas a construir").
// La 1 (gravedad dirigida) y la 2 (caminar por paredes) YA funcionan:
// gravPickTarget / gravGroundAlong / gravBlocked en gravity.h.
//
// REGLAS DE LA CASA (respetadas): sin heap, sin rand, sin I/O, C++17,
// <math.h> como unico include, todo determinista y header-only.
//
// ---------------------------------------------------------------------------
// DEPENDENCIAS (que tiene que estar YA definido donde se incluya)
//   struct LineVertex, RGBA(), brighten(), addQuad(), addSolidBox(), addPyramid()
// Todo eso vive en main.cpp antes de la linea ~235, asi que este archivo se
// incluye DESPUES de addPyramid() (por ejemplo al lado de #include "gravity.h").
// NO depende de g_city ni de sector.h: las coordenadas del sector estan
// replicadas abajo como constantes (si se tocan SEC_* hay que actualizarlas).
//
// ---------------------------------------------------------------------------
// EL MAPA (sector.h), para entender las posiciones de este archivo
//   recinto 112x112: suelo y=0, TECHO y=60, cara interior de los muros |54|
//   4 masas de 36x36 y 30 de alto:  x,z en [9,45] (y sus espejos)
//       -> caras verticales en |9| (pasillo en cruz) y |45| (pasillo perimetral)
//       -> TECHO de la masa PISABLE en y=30 (la cornisa visual llega a 31.6)
//   20 columnas de 3.3x3.3 y 18.6 de alto en x=+-7 y z=+-7, cada 16
//       -> CAPITEL PISABLE en y=18.6 (arcos ojivales desde y=20.7: nada
//          que se dibuje encima de un capitel puede pasar de ~2.0 de alto)
//   muros: ventanales ABIERTOS (visualmente) entre y=7 y y=27; la colision es
//          el muro entero, asi que la cara interior se camina de 0 a 60.
//
// ---------------------------------------------------------------------------
// LOS TRES NUMEROS QUE HACEN QUE ESTO SEA UN JUEGO
//   1) SALTO = 7.56 de alto   (JUMP_VEL 0.55, GRAVITY 0.020 -> v^2/2g)
//      => TODO lo que este a mas de ~7.5 de una superficie pisable EXIGE
//         cambiar la gravedad. Por eso los capiteles (18.6) ya son "arriba".
//   2) ALCANCE DEL RAYO = 30 (GRAV_PICK_RANGE): solo podes cambiar la gravedad
//      hacia una cara que este a menos de 30. Es lo que ESCALONA la escalada:
//         suelo (ojo y=1.7)  -> techo y=60 esta a 58.3  -> IMPOSIBLE
//         capitel (y=20.3)   -> techo y=60 esta a 39.7  -> IMPOSIBLE
//         techo de masa (31.7) -> techo y=60 esta a 28.3 -> JUSTO ALCANZA
//      => la ruta obligada es suelo -> capitel -> techo de masa -> TECHO -> faro.
//   3) COSTE = 120 EN de 780 (6 cambios con la barra llena) y solo recarga con
//      los pies apoyados. Cada material sube el techo de la barra: mas cambios
//      encadenados = llegas mas lejos sin bajar a recargar.
//
// ---------------------------------------------------------------------------
// GOTCHA DEL MOTOR QUE CONDICIONA EL DISENO (no se puede arreglar desde aca):
// parado EXACTAMENTE sobre una cara, tu coordenada de pies COINCIDE con el
// borde de esa caja; si cambias la gravedad ahi mismo, gravBlocked() te ve
// dentro de la caja y quedas trabado. La maniobra correcta -y la que se pide
// al jugador- es SALTAR y cambiar la gravedad EN EL AIRE, con el cuerpo ya
// despegado de la superficie. Todas las rutas de abajo asumen eso.
// ============================================================================

#include <math.h>

// ============================ CONSTANTES =====================================
static const int   OBJ_ANCHOR_N   = 5;      // anclas (checkpoints)
static const int   OBJ_MAT_N      = 10;     // materiales
static const int   OBJ_MARK_MAX   = 1200;   // tope de vertices de objBuildMarkers()

static const float OBJ_PICK_R     = 2.60f;  // radio para RECOGER un material
static const float OBJ_ANCHOR_R   = 3.20f;  // radio para ACTIVAR un ancla
static const float OBJ_GOAL_R     = 4.50f;  // radio del FARO (fin de partida)

static const float OBJ_EN_BASE    = 780.0f; // EN maxima de arranque (== EN_MAX de main.cpp)
static const float OBJ_EN_SWITCH  = 120.0f; // coste por cambio de gravedad
                                            // (DEBE coincidir con GRAV_EN_SWITCH de gravity.h)
static const float OBJ_EN_BONUS   =  60.0f; // +EN maxima por material recogido.
                                            // 10 materiales = +600 -> 1380 = 11 cambios
                                            // seguidos contra los 6 del arranque.

static const float OBJ_FALL_Y     =   1.60f;// "estas de vuelta en el suelo"
static const float OBJ_FALL_DROP  =  12.0f; // si el ancla activa esta mas alta que esto,
                                            // tocar el suelo CUENTA COMO CAIDA -> respawn.
                                            // (el recinto es cerrado: no hay vacio al que
                                            //  caerse, el castigo es perder la altura)
static const float OBJ_OUT_XZ     =  56.0f; // red de seguridad (muro interior = 54)

static const float OBJ_MAT_S      =   0.80f;// lado del cristal
static const float OBJ_MAT_LIFT   =   0.85f;// cuanto FLOTA el cristal sobre su superficie
static const float OBJ_MAT_BOB    =   0.18f;// amplitud del flote (lift+bob+0.95 <= 1.98:
                                            //  cabe bajo los arcos de los capiteles)

// posicion del FARO: colgado del TECHO en el CRUCE EXACTO de los dos pasillos.
// Ahi la vertical esta limpia (las columnas estan en +-7), asi que desde
// CUALQUIER punto del pasillo en cruz se lo ve mirando hacia arriba: esa es la
// promesa del diseno -"sabes adonde vas sin un solo icono en pantalla"-.
static const float OBJ_GOAL_X     =   0.0f;
static const float OBJ_GOAL_Y     =  60.0f; // pies del jugador sobre el TECHO
static const float OBJ_GOAL_Z     =   0.0f;

// ============================ TABLAS =========================================
// face = indice de gravedad (0..5, igual que gravity.h) con el que se PISA ese
// punto. Sirve para dos cosas: saber hacia donde "flota" el adorno y con que
// gravedad hay que reaparecer.
//   0 = -Y (suelo/techos de masa/capiteles)   1 = +Y (TECHO del sector)
//   2 = +X (se pisa una cara -X)              3 = -X (se pisa una cara +X)
//   4 = +Z (se pisa una cara -Z)              5 = -Z (se pisa una cara +Z)
struct ObjAnchor { float x, y, z; int face; float vis; };
struct ObjMat    { float x, y, z; int face; };

// ------------------------------- ANCLAS -------------------------------------
// vis = cuanto hay que SUBIR el adorno para que apoye en la superficie VISIBLE.
// En los techos de masa la colision esta en y=30 pero la cornisa dibujada llega
// a 31.6: sin este offset el pilar del ancla quedaria medio enterrado.
static const ObjAnchor kObjAnchors[OBJ_ANCHOR_N] = {
    //  x        y       z    face  vis
    {   0.0f,   0.0f,   0.0f,  0,  0.0f },  // A0 SUELO, cruce central. Arranque y checkpoint cero.
    {   7.0f,  18.6f,  16.0f,  0,  0.0f },  // A1 CAPITEL de la columna (+7, z=16).
    {  13.0f,  30.0f,  27.0f,  0,  1.6f },  // A2 TECHO de la masa NE, cerca del borde interior
                                            //    (x=13 esta fuera del remate, que ocupa 17.1..36.9).
    { -13.0f,  30.0f, -27.0f,  0,  1.6f },  // A3 TECHO de la masa SO: el otro lado del mapa,
                                            //    es el que agarra quien va a buscar M3/M4/M5.
    {   0.0f,  60.0f,   8.0f,  1,  0.0f },  // A4 TECHO DEL SECTOR, a 8 del faro. Se camina con
                                            //    gravedad +Y: por eso su pilar CUELGA hacia abajo.
};

// ------------------------------ MATERIALES ----------------------------------
// POR QUE CADA UNO EXIGE CAMBIAR LA GRAVEDAD (salto = 7.5; nada de esto se
// alcanza saltando desde una superficie normal):
//
//  M0 (9, 22, 20) cara -X de la masa NE, pasillo en cruz.
//      Desde el capitel A1 (7, 18.6, 16): saltar y, en el aire, mirar la cara de
//      la masa (esta a 2 de distancia) -> gravedad +X -> caes sobre x=9 y CORRES
//      la pared hacia y=22. A 22 de altura y pegado a una pared: sin gravedad, no.
//  M1 (20, 26, 9) cara -Z de la masa NE, el otro brazo del pasillo (el de X).
//      Mismo gesto que M0 girado 90 grados: desde el suelo del pasillo en X,
//      saltar mirando la cara z=9 (esta a menos de 9) -> gravedad +Z -> caes
//      sobre z=9 y subis. y=26 es casi la cornisa: obliga a correr TODA la pared
//      sin soltar, y en el pasillo en X no hay capitel que sirva de descanso
//      a esa altura (los capiteles quedan a 18.6, siete metros mas abajo).
//  M2 (45, 24, -20) cara +X de la masa SE, pasillo PERIMETRAL (9 de ancho).
//      Desde el suelo del perimetro, mirar la masa -> gravedad -X -> caes sobre
//      x=45 y subis. El perimetro es un tubo estrecho: es el sitio mas comodo
//      para aprender a correr por pared.
//  M3 (-20, 27, -45) cara -Z de la masa SO, perimetral.
//      Mismo gesto con gravedad +Z. y=27 = casi el tope de la masa.
//  M4 (-45, 20, 20) cara -X de la masa NO, perimetral. Gravedad +X.
//  M5 (-9, 28, 20) cara +X de la masa NO, pasillo en cruz. Gravedad -X.
//      El mas alto de las caras (28 de 30): hay que encadenar. Lo normal es
//      venir corriendo de M0 (cara de enfrente, a 18): desde x=9 saltas, miras
//      la cara x=-9 (18 < 30 de alcance) y cruzas el pasillo DE PARED A PARED.
//  M6 (-7, 18.6, 16) CAPITEL de la columna (-7, 16).
//  M7 (16, 18.6, -7) CAPITEL de la columna (16, -7).
//      18.6 > 7.5: al capitel solo se sube pisando la cara de la columna
//      (gravedad +-X o +-Z segun el lado) y, cerca del tope, mirando el suelo
//      para volver a gravedad -Y: el motor te sube a la cara superior.
//  M8 (54, 42, 27) cara INTERIOR del muro +X, a 42 de altura.
//      Desde el TECHO de la masa NE (y=30): saltar hacia el muro (esta a 9 del
//      borde x=45), cambiar en el aire -> gravedad +X -> caes sobre x=54 y
//      SUBIS EL MURO. Va a 42 a proposito: por debajo de 27 el muro tiene los
//      ventanales abiertos y no hay piedra dibujada donde apoyarse.
//  M9 (0, 60, -34) pegado al TECHO del sector.
//      Solo existe para quien ya camina el techo con gravedad +Y, y esta lejos
//      del faro: obliga a un rodeo por el techo antes de cobrar la meta.
static const ObjMat kObjMats[OBJ_MAT_N] = {
    //  x        y       z    face
    {   9.0f,  22.0f,  20.0f,  2 },  // M0 cara -X masa NE   (cruz)
    {  20.0f,  26.0f,   9.0f,  4 },  // M1 cara -Z masa NE   (cruz)
    {  45.0f,  24.0f, -20.0f,  3 },  // M2 cara +X masa SE   (perimetral)
    { -20.0f,  27.0f, -45.0f,  4 },  // M3 cara -Z masa SO   (perimetral)
    { -45.0f,  20.0f,  20.0f,  2 },  // M4 cara -X masa NO   (perimetral)
    {  -9.0f,  28.0f,  20.0f,  3 },  // M5 cara +X masa NO   (cruz)
    {  -7.0f,  18.6f,  16.0f,  0 },  // M6 capitel (-7, 16)
    {  16.0f,  18.6f,  -7.0f,  0 },  // M7 capitel (16, -7)
    {  54.0f,  42.0f,  27.0f,  2 },  // M8 muro +X por encima de los ventanales
    {   0.0f,  60.0f, -34.0f,  1 },  // M9 bajo el TECHO
};

// ============================ ESTADO =========================================
// Todo en estaticas: el bucle solo lee y escribe por estas funciones.
static int g_objActive     = 0;   // ancla activa (indice en kObjAnchors)
static int g_objAnchorSeen = 1;   // bitmask de anclas ya tocadas (A0 arranca puesta)
static int g_objMatTaken   = 0;   // bitmask de materiales recogidos
static int g_objMatCount   = 0;   // popcount del anterior (para no recontarlo)
static int g_objGoal       = 0;   // 1 = faro alcanzado

static void objReset()
{
    g_objActive = 0; g_objAnchorSeen = 1;
    g_objMatTaken = 0; g_objMatCount = 0; g_objGoal = 0;
}

// ============================ 1) ANCLAS ======================================
// Devuelve el indice del ancla RECIEN activada, o -1. El checkpoint solo AVANZA
// (nunca retrocede al volver a pisar una mas baja): perder altura ya es castigo
// suficiente, no hace falta ademas perder el checkpoint.
static int objAnchorTouch(float px, float py, float pz)
{
    for (int a = 0; a < OBJ_ANCHOR_N; ++a) {
        if (g_objAnchorSeen & (1 << a)) continue;
        const float dx = px - kObjAnchors[a].x;
        const float dy = py - kObjAnchors[a].y;
        const float dz = pz - kObjAnchors[a].z;
        if (dx * dx + dy * dy + dz * dz > OBJ_ANCHOR_R * OBJ_ANCHOR_R) continue;
        g_objAnchorSeen |= (1 << a);
        if (a > g_objActive) g_objActive = a;
        return a;
    }
    return -1;
}

static int objActiveAnchor() { return g_objActive; }

// Posicion de reaparicion = PIES sobre el ancla activa (ya verificado contra
// gravBlocked: ninguna de las 5 queda dentro de una caja).
static void objRespawn(float *x, float *y, float *z)
{
    const ObjAnchor &a = kObjAnchors[g_objActive];
    *x = a.x; *y = a.y; *z = a.z;
}

// IMPRESCINDIBLE: el ancla del TECHO se pisa con gravedad +Y. Si main.cpp
// reaparece ahi sin poner gravG = 1, el jugador se cae 60 unidades al instante.
static int objRespawnGrav() { return kObjAnchors[g_objActive].face; }

// "Te caiste": 1 = hay que reaparecer. El recinto es CERRADO (suelo y techo),
// asi que caer al vacio no existe; lo que cuenta como caida es VOLVER AL SUELO
// con la gravedad normal despues de haber ganado altura.
static int objFallCheck(float px, float py, float pz, int gravG, int grounded)
{
    if (py < -2.0f || py > 62.0f) return 1;                       // fuera del recinto
    if (px < -OBJ_OUT_XZ || px > OBJ_OUT_XZ) return 1;
    if (pz < -OBJ_OUT_XZ || pz > OBJ_OUT_XZ) return 1;
    if (gravG == 0 && grounded && py <= OBJ_FALL_Y &&
        kObjAnchors[g_objActive].y > OBJ_FALL_DROP) return 1;     // perdiste la escalada
    return 0;
}

// ============================ 2) MATERIALES ==================================
// Devuelve el indice recogido (y lo marca), o -1.
// El test es contra los PIES del jugador y los puntos de la tabla estan EN la
// superficie, no flotando: corriendo por una pared, tus pies van exactamente
// sobre la cara (gravGroundAlong devuelve la coordenada de la cara), asi que
// pasar al lado del material alcanza. Lo que flota es solo el dibujo.
static int objMaterialTouch(float px, float py, float pz)
{
    for (int m = 0; m < OBJ_MAT_N; ++m) {
        if (g_objMatTaken & (1 << m)) continue;
        const float dx = px - kObjMats[m].x;
        const float dy = py - kObjMats[m].y;
        const float dz = pz - kObjMats[m].z;
        if (dx * dx + dy * dy + dz * dz > OBJ_PICK_R * OBJ_PICK_R) continue;
        g_objMatTaken |= (1 << m);
        g_objMatCount++;
        return m;
    }
    return -1;
}

static int   objMaterialsTaken()  { return g_objMatCount; }
static int   objMaterialCount()   { return OBJ_MAT_N; }
static float objEnergyMaxBonus()  { return (float)g_objMatCount * OBJ_EN_BONUS; }
static float objEnergyMax()       { return OBJ_EN_BASE + objEnergyMaxBonus(); }

// ============================ 3) FARO ========================================
// 1 = el jugador esta en el faro. Solo se puede estar a menos de 4.5 de
// (0,60,0) caminando el TECHO con gravedad +Y: no hay forma de "rozarlo" desde
// abajo (lo mas alto pisable por debajo son los 30 de las masas).
static int objGoalReached(float px, float py, float pz)
{
    const float dx = px - OBJ_GOAL_X, dy = py - OBJ_GOAL_Y, dz = pz - OBJ_GOAL_Z;
    if (dx * dx + dy * dy + dz * dz > OBJ_GOAL_R * OBJ_GOAL_R) return 0;
    g_objGoal = 1;
    return 1;
}
static int objGoalDone() { return g_objGoal; }

// ============================ 4) GEOMETRIA ===================================
// OJO: este buffer se dibuja en el pase CON back-face culling (el de g_bridges /
// g_spire). Por eso SOLO se usan addSolidBox y addPyramid, que tienen el winding
// bueno. NADA de addLimb/addBall: su winding es el opuesto y desapareceria todo.
//
// Las cajas del motor no traen CARA INFERIOR. Para lo que se mira desde abajo
// (el ancla colgada del techo y el faro entero) hay que emitirla a mano: es lo
// que hace objBoxCap, reflejando el orden de vertices de la cara superior de
// addSolidBox (una reflexion invierte la orientacion, asi que el orden se da
// vuelta y la normal sigue apuntando hacia AFUERA).

// caja con tapa inferior -> 36 verts
static void objBoxCap(LineVertex *buf, int &i, float cx, float baseY, float cz,
                      float w, float d, float h, unsigned int col)
{
    addSolidBox(buf, i, cx, baseY, cz, w, d, h, col);                 // 30v
    const float x0 = cx - w * 0.5f, x1 = cx + w * 0.5f;
    const float z0 = cz - d * 0.5f, z1 = cz + d * 0.5f;
    addQuad(buf, i, x0, baseY, z1, x1, baseY, z1,
                    x1, baseY, z0, x0, baseY, z0, brighten(col, 0.55f)); // 6v
}

// "arriba" del jugador para una cara dada (= -gravedad). Se calcula aca y no
// con gravDirVec() para que objective.h no dependa del orden de inclusion.
static void objUpVec(int face, float *ux, float *uy, float *uz)
{
    float x = 0.0f, y = 0.0f, z = 0.0f;
    switch (face) {
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

// Pieza de un ancla: se apila a distancia d0 de la superficie. s=+1 se levanta
// desde el suelo, s=-1 CUELGA del techo (misma tabla, misma altura, invertido).
static void objStack(LineVertex *buf, int &i, float cx, float y0, float cz, float s,
                     float d0, float w, float h, unsigned int col, int cap)
{
    const float base = (s > 0.0f) ? (y0 + d0) : (y0 - d0 - h);
    if (cap) objBoxCap(buf, i, cx, base, cz, w, w, h, col);
    else     addSolidBox(buf, i, cx, base, cz, w, w, h, col);
}

// ----------------------------------------------------------------------------
// Construye TODOS los marcadores del bucle. Devuelve el numero de vertices.
// El buffer tiene que ser de al menos OBJ_MARK_MAX (1200) LineVertex.
// Peor caso real: 5 anclas (468) + 10 materiales sin recoger (480) + faro (192)
// = 1140 verts. A medida que se recogen materiales, BAJA.
// `time` es un reloj en segundos (vale cualquier acumulador del bucle): mueve el
// flote y el palpito. Todo es sinf/cosf: no hay estado propio de animacion.
// ----------------------------------------------------------------------------
static int objBuildMarkers(LineVertex *buf, float time)
{
    int i = 0;

    // ------------------------------ ANCLAS ----------------------------------
    // Pilar bajo (1.95 de alto: es el maximo que cabe bajo los arcos ojivales,
    // que arrancan a 20.7 sobre un capitel de 18.6) con una luz FRIA arriba.
    // La activa palpita y brilla mucho mas: desde lejos se lee cual esta viva.
    for (int a = 0; a < OBJ_ANCHOR_N; ++a) {
        if (i + 120 > OBJ_MARK_MAX) break;
        const ObjAnchor &k = kObjAnchors[a];
        const int   act  = (a == g_objActive);
        const int   hang = (k.face == 1);              // A4 cuelga del techo
        const float s    = hang ? -1.0f : 1.0f;
        const float y0   = k.y + s * k.vis;            // superficie VISIBLE
        const float pul  = 0.5f + 0.5f * sinf(time * 2.1f + (float)a * 1.7f);

        const unsigned int plinth = RGBA( 86,  96, 112, 255);   // piedra fria
        const unsigned int postc  = RGBA(110, 124, 146, 255);
        const unsigned int lampc  = act ? brighten(RGBA(168, 214, 255, 255), 0.85f + 0.45f * pul)
                                        : RGBA( 62,  98, 150, 255);   // apagada = azul sucio

        const float lw = act ? (0.95f + 0.16f * pul) : 0.80f;
        objStack(buf, i, k.x, y0, k.z, s, 0.00f, 1.50f, 0.50f, plinth, hang); // basa
        objStack(buf, i, k.x, y0, k.z, s, 0.50f, 0.55f, 1.00f, postc,  hang); // fuste
        objStack(buf, i, k.x, y0, k.z, s, 1.50f, lw,    0.45f, lampc,  hang); // luz
    }

    // ---------------------------- MATERIALES --------------------------------
    // Cristal que FLOTA sobre su superficie y "gira": como addSolidBox es
    // alineado a los ejes, la rotacion se finge intercambiando ancho y fondo con
    // el tiempo (cuesta cero y se lee como un cristal dando vueltas). El palpito
    // va en el brillo. Los recogidos no se dibujan.
    for (int m = 0; m < OBJ_MAT_N; ++m) {
        if (g_objMatTaken & (1 << m)) continue;
        if (i + 48 > OBJ_MARK_MAX) break;

        float ux, uy, uz; objUpVec(kObjMats[m].face, &ux, &uy, &uz);
        const float ph   = time * 1.9f + (float)m * 0.9f;
        const float lift = OBJ_MAT_LIFT + OBJ_MAT_BOB * sinf(ph);
        const float cx   = kObjMats[m].x + ux * lift;
        const float cy   = kObjMats[m].y + uy * lift;
        const float cz   = kObjMats[m].z + uz * lift;

        const float sp = time * 2.4f + (float)m;
        const float w  = OBJ_MAT_S * (0.55f + 0.45f * fabsf(cosf(sp)));
        const float d  = OBJ_MAT_S * (0.55f + 0.45f * fabsf(sinf(sp)));
        const float pu = 0.74f + 0.36f * sinf(ph * 1.4f);
        const unsigned int c = brighten(RGBA(150, 222, 236, 255), pu);   // hielo/cian

        objBoxCap (buf, i, cx, cy - OBJ_MAT_S * 0.5f, cz, w, d, OBJ_MAT_S, c);        // 36v
        addPyramid(buf, i, cx, cy + OBJ_MAT_S * 0.5f, cz, w, d, 0.55f, brighten(c, 1.25f)); // 12v
    }

    // ------------------------------- FARO -----------------------------------
    // Baliza CALIDA colgada del techo, en el eje del cruce. Cuelga 13.8 (de 60 a
    // 46.2): queda 16 por encima de los techos de las masas, asi que se ve desde
    // el suelo de los dos pasillos y desde arriba de cualquier masa. Es lo unico
    // calido y saturado del sector: no hace falta ningun icono de HUD.
    if (i + 200 <= OBJ_MARK_MAX) {
        const float bx = OBJ_GOAL_X, bz = OBJ_GOAL_Z;
        const float pul = 0.5f + 0.5f * sinf(time * 1.6f);
        // mastil en 3 tramos que se afinan (tramos cortos: un quad largo cerca de
        // la camara cruza el plano de vista y el hardware lo descarta ENTERO)
        objBoxCap(buf, i, bx, 56.0f, bz, 1.70f, 1.70f, 4.0f, RGBA( 92,  80,  66, 255));
        objBoxCap(buf, i, bx, 52.0f, bz, 1.35f, 1.35f, 4.0f, RGBA(104,  88,  70, 255));
        objBoxCap(buf, i, bx, 48.0f, bz, 1.05f, 1.05f, 4.0f, RGBA(116,  96,  74, 255));
        // collar que respira (lo que da la silueta reconocible desde 60 de lejos)
        const float rw = 6.20f + 1.10f * pul;
        objBoxCap(buf, i, bx, 47.90f, bz, rw, rw, 0.45f,
                  brighten(RGBA(255, 168,  86, 255), 0.75f + 0.35f * pul));
        // linterna: el nucleo brillante
        objBoxCap(buf, i, bx, 46.20f, bz, 4.00f, 4.00f, 1.80f,
                  brighten(RGBA(255, 206, 132, 255), 0.80f + 0.40f * pul));
        // llama que sube envolviendo el mastil (piramide hacia ARRIBA: una
        // piramide invertida tendria el winding dado vuelta y la cullearia)
        addPyramid(buf, i, bx, 48.35f, bz, 2.60f, 2.60f, 3.00f + 0.90f * pul,
                   brighten(RGBA(255, 228, 170, 255), 0.85f + 0.35f * pul));
    }

    return i;
}

// Buffer listo para usar (evita adivinar tamanos con sceGuGetMemory).
// Se reconstruye cada frame porque los marcadores estan animados.
static LineVertex __attribute__((aligned(16))) g_objMark[OBJ_MARK_MAX];
static int g_objMarkVerts = 0;

// ============================================================================
// NOTA DE CABLEADO (esto es lo que tiene que hacer main.cpp)
//
// API: objReset / objAnchorTouch / objActiveAnchor / objRespawn / objRespawnGrav /
//      objFallCheck / objMaterialTouch / objMaterialsTaken / objMaterialCount /
//      objEnergyMaxBonus / objEnergyMax / objGoalReached / objGoalDone /
//      objBuildMarkers (+ buffer g_objMark / g_objMarkVerts).
//
// DONDE ESTA CADA COSA Y POR QUE SE ALCANZA: 5 anclas -> suelo del cruce (0,0,0),
// capitel (7,18.6,16), techos de las masas NE y SO (y=30) y el TECHO del sector
// (0,60,8), que es el unico que se pisa con gravedad +Y. 10 materiales en caras
// verticales de las masas (|9| y |45|, a 20-28 de altura), 2 sobre capiteles,
// 1 en el muro +X a y=42 (por encima de los ventanales) y 1 pegado al techo.
// Todo esta a mas de 7.5 de cualquier superficie pisable, que es lo que sube un
// salto: sin cambiar la gravedad no se toca nada. El escalonado lo impone el
// alcance de 30 del rayo: desde el suelo no se "ve" el techo (58.3), desde un
// capitel tampoco (39.7), desde el techo de una masa SI (28.3).
// VERTICES: 468 (anclas) + 480 (10 materiales) + 192 (faro) = 1140 <= 1200.
//
// LLAMADAS POR FRAME, EN ESTE ORDEN:
//   1. (una vez, al empezar partida)      objReset();
//   2. despues de mover al jugador:       objAnchorTouch(playerX, playerY, playerZ);
//   3.                                    int mm = objMaterialTouch(playerX, playerY, playerZ);
//                                         // mm >= 0 -> aviso "MATERIAL  EN MAX +60"
//   4. tope de la barra de EN:            enMax = objEnergyMax();  // en vez del EN_MAX fijo
//                                         if (en > enMax) en = enMax;
//   5.                                    if (objGoalReached(playerX,playerY,playerZ)) { /* fin */ }
//   6. ULTIMO del bloque de fisica:       if (objFallCheck(playerX,playerY,playerZ,gravG,grounded)) {
//                                             objRespawn(&playerX,&playerY,&playerZ);
//                                             gravG = objRespawnGrav();          // <- NO olvidar
//                                             velX=velY=velZ=0; gvr=gvf=gvg=0; grounded=1;
//                                         }
//                                         // reemplaza la RED DE SEGURIDAD que hoy manda al balcon z=118
//   7. al dibujar, DENTRO del pase CON culling (junto a g_bridges/g_spire):
//                                         g_objMarkVerts = objBuildMarkers(g_objMark, tSec);
//                                         sceGumLoadIdentity();
//                                         sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g_objMarkVerts, 0, g_objMark);
// ============================================================================
