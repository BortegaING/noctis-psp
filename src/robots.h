#pragma once
// PROJECT NOCTIS - HABITANTES de la plaza: ROBOTS / ANDROIDES (la humanidad ya
// no existe, directiva 8-9,35). NO son todos militares: cada uno es un ROL con
// SILUETA propia y un ESTADO DE REPARACION distinto (pulido / oxidado / roto).
// Estetica de personaje anime-estilizado contra el mundo brutal (contraste).
//
// Se incluye en main.cpp DESPUES de char_prims.h (usa addLimb/addBall/addLoft
// para cuerpos organicos) y de los helpers addSolidBox/addPyramid/brighten/RGBA.
// El buffer es LineVertex (GU_TRIANGLES, flat). Culling OFF => winding libre.
//
// Espacio: coords CRUDAS de la plaza (las mismas que g_city y los NPC viejos, SIN
// WSCALE). Pies en y=0. El jugador nace en (0,0,0); los robots se colocan en un
// anillo r~14..27 para que el jugador SE LOS ENCUENTRE, EVITANDO las 4 catedrales:
//   GRAND  (0,-60, 52x44)  -> borde cercano z=-38
//   TWIN   (62, 6, 44x48)  -> borde cercano x= 40
//   BASILICA(-60,10,48x72) -> borde cercano x=-36
//   BELL   (10,60, 40x40)  -> borde cercano z= 40
// Todos los robots quedan holgados dentro de esos bordes y lejos del spawn.
//
// PSP DURO: determinista (sin rand/heap), C++17, <math.h>. Presupuesto <= ~1800
// verts en total. buildRobots() devuelve el total.

#include <math.h>

// ---- paleta: estados de reparacion (directiva) ----
static const unsigned int R_STEEL = RGBA(150, 158, 170, 255); // acero pulido
static const unsigned int R_PALE  = RGBA(202, 206, 212, 255); // metal palido pulido (mensajero)
static const unsigned int R_BRASS = RGBA(120,  96,  60, 255); // laton / bronce oxidado
static const unsigned int R_RUST  = RGBA(110,  70,  50, 255); // herrumbre
static const unsigned int R_TEAL  = RGBA( 70, 150, 150, 255); // acento teal (patina)
static const unsigned int R_DARK  = RGBA( 74,  78,  92, 255); // carcasa oscura
static const unsigned int R_EYEW  = RGBA(255, 180,  90, 255); // ojo calido (ambar)
static const unsigned int R_EYEC  = RGBA(120, 225, 235, 255); // ojo frio (cian)

// direccion HORIZONTAL hacia el centro de la plaza (spawn del jugador): los
// robots "miran" al jugador (cabeza/ojo/herramienta hacia adentro).
static inline void robotFacing(float cx, float cz, float &fx, float &fz) {
    float d = sqrtf(cx * cx + cz * cz);
    if (d < 1e-4f) { fx = 0.0f; fz = -1.0f; return; }
    fx = -cx / d; fz = -cz / d;
}

// ojo/visor: cubito luminoso (glow). 30 verts.
static void addRobotEye(LineVertex *buf, int &i, float ex, float ey, float ez,
                        float w, float h, unsigned int col) {
    addSolidBox(buf, i, ex, ey - h * 0.5f, ez, w, w * 0.5f, h, col);
}

// =====================================================================
// 1) MENSAJERO  (STEEL/PALE) ~276v
//    Alto y delgado, miembros esbeltos, cabeza-farol + antena. Metal palido
//    pulido: el androide en mejor estado, elegante.
// =====================================================================
static void buildMessenger(LineVertex *buf, int &i, float cx, float cz) {
    float fx, fz; robotFacing(cx, cz, fx, fz);
    const unsigned int leg = brighten(R_STEEL, 0.85f);
    addSolidBox(buf, i, cx - 0.16f, 0.0f, cz, 0.18f, 0.20f, 1.70f, leg);              // pierna L (30)
    addSolidBox(buf, i, cx + 0.16f, 0.0f, cz, 0.18f, 0.20f, 1.70f, leg);              // pierna R (30)
    addLimb(buf, i, cx, 1.70f, cz, cx, 3.00f, cz, 0.26f, 0.16f, 4, R_STEEL, R_PALE);  // torso conico esbelto (48)
    addLimb(buf, i, cx - 0.22f, 2.92f, cz, cx - 0.30f, 2.00f, cz, 0.07f, 0.05f, 3, R_STEEL, R_PALE); // brazo L fino (36)
    addLimb(buf, i, cx + 0.22f, 2.92f, cz, cx + 0.30f, 2.00f, cz, 0.07f, 0.05f, 3, R_STEEL, R_PALE); // brazo R fino (36)
    addBall(buf, i, cx, 3.28f, cz, 0.17f, 0.21f, 0.17f, 2, 3, R_PALE);                // cabeza-farol (36)
    addSolidBox(buf, i, cx - 0.025f, 3.49f, cz, 0.05f, 0.05f, 0.72f, R_PALE);         // antena (30)
    addRobotEye(buf, i, cx + fx * 0.20f, 3.24f, cz + fz * 0.20f, 0.11f, 0.11f, R_EYEW); // ojo calido (30)
}

// =====================================================================
// 2) MECANICO  (RUST) ~282v
//    Encorvado, brazo izquierdo macizo, brazo-herramienta (taladro) derecho,
//    placas parchadas/oxidadas. El obrero desgastado.
// =====================================================================
static void buildMechanic(LineVertex *buf, int &i, float cx, float cz) {
    float fx, fz; robotFacing(cx, cz, fx, fz);
    const unsigned int leg = brighten(R_RUST, 0.85f);
    addSolidBox(buf, i, cx - 0.18f, 0.0f, cz, 0.22f, 0.24f, 1.30f, leg);              // pierna L (30)
    addSolidBox(buf, i, cx + 0.18f, 0.0f, cz, 0.22f, 0.24f, 1.30f, leg);              // pierna R (30)
    addSolidBox(buf, i, cx, 1.30f, cz, 0.80f, 0.60f, 0.90f, R_RUST);                  // torso ancho (30)
    addBall(buf, i, cx - fx * 0.15f, 2.05f, cz - fz * 0.15f, 0.42f, 0.35f, 0.42f, 2, 3, brighten(R_RUST, 0.9f)); // joroba oxidada en la espalda (36)
    addLimb(buf, i, cx - 0.45f, 2.10f, cz, cx - 0.56f, 1.15f, cz, 0.17f, 0.13f, 4, R_RUST, brighten(R_RUST, 1.12f)); // brazo macizo L (48)
    addLimb(buf, i, cx + 0.46f, 2.05f, cz, cx + 0.60f + fx * 0.2f, 1.45f, cz + fz * 0.2f, 0.12f, 0.08f, 3, R_STEEL, R_STEEL); // brazo-herramienta (36)
    addPyramid(buf, i, cx + 0.60f + fx * 0.2f, 1.20f, cz + fz * 0.2f, 0.16f, 0.16f, 0.40f, brighten(R_STEEL, 1.1f)); // punta/taladro (12)
    addSolidBox(buf, i, cx + fx * 0.22f, 2.02f, cz + fz * 0.22f, 0.36f, 0.36f, 0.36f, brighten(R_RUST, 1.15f)); // cabeza baja adelantada (30)
    addRobotEye(buf, i, cx + fx * 0.42f, 2.20f, cz + fz * 0.42f, 0.12f, 0.10f, R_EYEW); // ojo calido (30)
}

// =====================================================================
// 3) GUARDIAN  (BRONZE) ~288v
//    Pesado y ancho, torso acorazado, cabeza-casco con cresta, bronce oxidado
//    con patina teal. Imponente pero no "militar moderno": arcaico.
// =====================================================================
static void buildGuardian(LineVertex *buf, int &i, float cx, float cz) {
    float fx, fz; robotFacing(cx, cz, fx, fz);
    const unsigned int leg = brighten(R_BRASS, 0.82f);
    addSolidBox(buf, i, cx - 0.30f, 0.0f, cz, 0.32f, 0.34f, 1.40f, leg);              // pierna L gruesa (30)
    addSolidBox(buf, i, cx + 0.30f, 0.0f, cz, 0.32f, 0.34f, 1.40f, leg);              // pierna R gruesa (30)
    static const float gy[3] = { 1.40f, 2.20f, 2.90f };
    static const float gr[3] = { 0.60f, 0.66f, 0.44f };
    addLoft(buf, i, cx, cz, gy, gr, 3, 4, R_BRASS, brighten(R_BRASS, 1.10f));         // torso acorazado abombado (96)
    addSolidBox(buf, i, cx - 0.60f, 2.40f, cz, 0.36f, 0.42f, 0.34f, brighten(R_BRASS, 0.9f)); // hombrera L (30)
    addSolidBox(buf, i, cx + 0.60f, 2.40f, cz, 0.36f, 0.42f, 0.34f, brighten(R_BRASS, 0.9f)); // hombrera R (30)
    addSolidBox(buf, i, cx, 2.90f, cz, 0.44f, 0.44f, 0.44f, brighten(R_BRASS, 1.12f));// casco (30)
    addPyramid(buf, i, cx, 3.34f, cz, 0.44f, 0.44f, 0.36f, R_TEAL);                   // cresta (patina teal) (12)
    addRobotEye(buf, i, cx + fx * 0.24f, 3.06f, cz + fz * 0.24f, 0.30f, 0.07f, R_EYEC); // visor frio ancho (30)
}

// =====================================================================
// 4) DRON  (TEAL/STEEL) ~168v
//    Pequeno orbe/ovoide FLOTANTE (sin piernas), un solo ojo. Barato y ligero.
// =====================================================================
static void buildDrone(LineVertex *buf, int &i, float cx, float cz) {
    float fx, fz; robotFacing(cx, cz, fx, fz);
    addBall(buf, i, cx, 1.70f, cz, 0.42f, 0.34f, 0.42f, 3, 5, R_STEEL);               // cuerpo ovoide flotante (90)
    addBall(buf, i, cx, 1.35f, cz, 0.16f, 0.14f, 0.16f, 2, 4, R_TEAL);                // pod sensor inferior (48)
    addRobotEye(buf, i, cx + fx * 0.40f, 1.72f, cz + fz * 0.40f, 0.18f, 0.16f, R_EYEC); // ojo unico grande frio (30)
}

// =====================================================================
// 5) ANCIANO  (STEEL desvaido) ~288v
//    Figura togada/encapuchada: falda alta acampanada por LOFT, larga y digna,
//    con baston. Colores apagados: misterioso, el mas viejo de todos.
// =====================================================================
static void buildAncient(LineVertex *buf, int &i, float cx, float cz) {
    float fx, fz; robotFacing(cx, cz, fx, fz);
    const unsigned int robeB = brighten(R_STEEL, 0.66f); // desvaido abajo
    const unsigned int robeT = brighten(R_STEEL, 0.94f);
    static const float ay[4] = { 0.0f, 0.70f, 1.80f, 2.70f };
    static const float ar[4] = { 0.95f, 0.85f, 0.50f, 0.40f };
    addLoft(buf, i, cx, cz, ay, ar, 4, 4, robeB, robeT);                              // falda/toga acampanada (144)
    addLimb(buf, i, cx, 2.70f, cz, cx, 3.15f, cz, 0.34f, 0.22f, 4, robeT, brighten(R_STEEL, 0.85f)); // hombros/cogulla (48)
    addBall(buf, i, cx, 3.20f, cz, 0.20f, 0.24f, 0.20f, 2, 3, brighten(R_STEEL, 0.78f)); // cabeza encapuchada (36)
    addSolidBox(buf, i, cx + 0.52f, 0.0f, cz, 0.06f, 0.06f, 3.30f, R_BRASS);          // baston (30)
    addRobotEye(buf, i, cx + fx * 0.18f, 3.16f, cz + fz * 0.18f, 0.09f, 0.09f, R_EYEC); // ojo frio profundo (30)
}

// =====================================================================
// 6) CAMINANTE  (DARK) ~222v
//    Cuadrupedo/insectoide portador: cuerpo bajo alargado sobre 4 patas.
//    Silueta totalmente distinta a los bipedos.
// =====================================================================
static void buildWalker(LineVertex *buf, int &i, float cx, float cz) {
    float fx, fz; robotFacing(cx, cz, fx, fz);
    addBall(buf, i, cx, 0.90f, cz, 0.55f, 0.30f, 0.78f, 2, 4, R_DARK);                // caparazon bajo (48)
    const unsigned int legc = brighten(R_DARK, 1.05f);
    addLimb(buf, i, cx - 0.40f, 0.85f, cz - 0.55f, cx - 0.64f, 0.0f, cz - 0.74f, 0.09f, 0.05f, 3, legc, R_DARK); // pata FL (36)
    addLimb(buf, i, cx + 0.40f, 0.85f, cz - 0.55f, cx + 0.64f, 0.0f, cz - 0.74f, 0.09f, 0.05f, 3, legc, R_DARK); // pata FR (36)
    addLimb(buf, i, cx - 0.40f, 0.85f, cz + 0.55f, cx - 0.64f, 0.0f, cz + 0.74f, 0.09f, 0.05f, 3, legc, R_DARK); // pata BL (36)
    addLimb(buf, i, cx + 0.40f, 0.85f, cz + 0.55f, cx + 0.64f, 0.0f, cz + 0.74f, 0.09f, 0.05f, 3, legc, R_DARK); // pata BR (36)
    addRobotEye(buf, i, cx + fx * 0.62f, 1.00f, cz + fz * 0.62f, 0.12f, 0.09f, R_EYEW); // sensor frontal (30)
}

// =====================================================================
// 7) TECNICO / EXPLORADOR  (STEEL + acentos TEAL) ~240v
//    Variante: mochila, un brazo con instrumento, tono acero con patina teal.
// =====================================================================
static void buildTechnician(LineVertex *buf, int &i, float cx, float cz) {
    float fx, fz; robotFacing(cx, cz, fx, fz);
    const unsigned int leg = brighten(R_STEEL, 0.85f);
    addSolidBox(buf, i, cx - 0.16f, 0.0f, cz, 0.20f, 0.22f, 1.50f, leg);              // pierna L (30)
    addSolidBox(buf, i, cx + 0.16f, 0.0f, cz, 0.20f, 0.22f, 1.50f, leg);              // pierna R (30)
    addLimb(buf, i, cx, 1.50f, cz, cx, 2.80f, cz, 0.24f, 0.20f, 4, R_STEEL, R_TEAL);  // torso (acento teal arriba) (48)
    addSolidBox(buf, i, cx - fx * 0.22f, 1.70f, cz - fz * 0.22f, 0.36f, 0.26f, 0.60f, brighten(R_STEEL, 0.82f)); // mochila a la espalda (30)
    addLimb(buf, i, cx + 0.26f, 2.72f, cz, cx + 0.34f + fx * 0.15f, 1.90f, cz + fz * 0.15f, 0.08f, 0.06f, 3, R_STEEL, R_TEAL); // brazo-instrumento (36)
    addBall(buf, i, cx, 2.84f, cz, 0.20f, 0.20f, 0.20f, 2, 3, brighten(R_STEEL, 1.10f)); // cabeza (36)
    addRobotEye(buf, i, cx + fx * 0.20f, 2.86f, cz + fz * 0.20f, 0.12f, 0.10f, R_EYEC); // ojo teal (30)
}

// =====================================================================
// buildRobots: coloca los 6-7 robots en el anillo de la plaza y devuelve el
// total de verts. Posiciones DETERMINISTAS (sin rand), en los huecos entre
// catedrales (N/E/O/S ocupados) y lejos del spawn (0,0).
// =====================================================================
static int buildRobots(LineVertex *buf) {
    int i = 0;
    buildMessenger (buf, i,  14.0f, -14.0f); // NE interior  (d~19.8)
    buildMechanic  (buf, i,  23.0f,   8.0f); // E interior   (d~24.4, TWIN empieza x=40)
    buildGuardian  (buf, i,   6.0f,  22.0f); // S interior   (d~22.8, BELL empieza z=40)
    buildDrone     (buf, i, -16.0f,  -8.0f); // O interior   (d~17.9, flota)
    buildAncient   (buf, i, -18.0f,  16.0f); // SO hueco      (d~24.1)
    buildWalker    (buf, i, -20.0f, -18.0f); // NO hueco      (d~26.9)
    buildTechnician(buf, i,  24.0f,  -6.0f); // ESE interior  (d~24.7)
    return i;
}
