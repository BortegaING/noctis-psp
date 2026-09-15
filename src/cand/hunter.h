#pragma once
// ============================================================================
// HUNTER - cazador gotico (ref. Bloodborne) visto en 3RA PERSONA DESDE ATRAS.
// Camara ~9 unidades detras y ~4.5 arriba => lo que mas se lee es la SILUETA
// de espaldas: capucha, esclavina, faldon acampanado y el sable colgando.
//
// REGLA DE ESTE ARCHIVO: ni una sola caja apilada. Todo sale de las
// primitivas organicas de char_prims.h:
//    addLimb -> cono truncado entre 2 puntos (piernas, brazos, cuello,
//               jirones, quillones y la HOJA del sable)
//    addBall -> elipsoide (cabeza, capucha, hombros, manos, pomo)
//    addLoft -> anillos de radio variable (faldon que se ACAMPANA, torso con
//               cintura marcada, esclavina, cuello alto)
// addSolidBox se usa UNA sola vez, para una hebilla plana de 30 verts.
//
// ESCALA: el modelo se autoria en METROS HUMANOS (1.80 de alto, pies en y=0,
// mirando a -Z) y se emite en unidades de mundo a traves de HN_S. El mundo de
// NOCTIS corre a ~1.92 u/m (los otros candidatos miden 3.3-3.5 y main.cpp NO
// aplica escala al dibujar g_hero). Tocar SOLO HN_S para re-escalar todo.
// ============================================================================

static const float HN_S = 1.92f;   // 1.80 m * 1.92 = 3.456 unidades de mundo

// ---- wrappers de escala (autoria en metros -> emite en unidades de mundo) --
static void hn_limb(LineVertex *b, int &i,
                    float x0, float y0, float z0, float x1, float y1, float z1,
                    float r0, float r1, int sides, unsigned int cB, unsigned int cT)
{
    addLimb(b, i, x0*HN_S, y0*HN_S, z0*HN_S, x1*HN_S, y1*HN_S, z1*HN_S,
            r0*HN_S, r1*HN_S, sides, cB, cT);
}
static void hn_ball(LineVertex *b, int &i, float cx, float cy, float cz,
                    float rx, float ry, float rz, int stacks, int slices, unsigned int col)
{
    addBall(b, i, cx*HN_S, cy*HN_S, cz*HN_S, rx*HN_S, ry*HN_S, rz*HN_S, stacks, slices, col);
}
static void hn_loft(LineVertex *b, int &i, float cx, float cz,
                    const float *ys, const float *rs, int n, int sides,
                    unsigned int cBot, unsigned int cTop)
{
    float sy[12], sr[12];
    if (n > 12) n = 12;
    for (int k = 0; k < n; ++k) { sy[k] = ys[k]*HN_S; sr[k] = rs[k]*HN_S; }
    addLoft(b, i, cx*HN_S, cz*HN_S, sy, sr, n, sides, cBot, cTop);
}
static void hn_box(LineVertex *b, int &i, float cx, float baseY, float cz,
                   float w, float d, float h, unsigned int col)
{
    addSolidBox(b, i, cx*HN_S, baseY*HN_S, cz*HN_S, w*HN_S, d*HN_S, h*HN_S, col);
}

// ============================================================================
static int build_hunter(LineVertex *buf)
{
    int i = 0;

    // ---------------- paleta ----------------
    const unsigned int coat    = RGBA( 20,  20,  26, 255);   // abrigo casi negro
    const unsigned int coatLit = cp_scale(coat, 1.55f);      // pliegues altos / hombros
    const unsigned int coatDk  = cp_scale(coat, 0.60f);      // sombra de pliegue
    const unsigned int hemDk   = cp_scale(coat, 0.42f);      // boca del faldon (oclusion)
    const unsigned int red     = RGBA(120,  30,  35, 255);   // correas / vivos rojos
    const unsigned int redDk   = cp_scale(red,  0.55f);
    const unsigned int skin    = RGBA(190, 170, 160, 255);   // piel palida
    const unsigned int skinDk  = cp_scale(skin, 0.70f);      // cara bajo la capucha
    const unsigned int hair    = RGBA( 26,  22,  28, 255);   // pelo oscuro
    const unsigned int hairDk  = cp_scale(hair, 0.60f);
    const unsigned int edge    = RGBA(196, 206, 226, 255);   // filo pulido
    const unsigned int spine   = RGBA( 92, 100, 118, 255);   // lomo de la hoja
    const unsigned int brass   = RGBA(152, 122,  62, 255);   // laton (guarda, pomo)
    const unsigned int brassDk = cp_scale(brass, 0.70f);
    const unsigned int grip    = RGBA( 44,  34,  32, 255);   // cuero de la empunadura
    const unsigned int gripLit = cp_scale(grip, 1.30f);
    const unsigned int boot    = RGBA( 24,  21,  24, 255);   // bota
    const unsigned int bootLit = cp_scale(boot, 1.45f);
    const unsigned int bootDk  = cp_scale(boot, 0.70f);
    const unsigned int belt    = cp_lerp(coat, red, 0.30f);  // cinturon tenido de rojo

    // =====================================================================
    // 1) PIERNAS - tres conos encadenados por pierna (muslo / cana / pie).
    //    Cada tramo AFINA y cambia de eje: rodilla y tobillo quedan
    //    articulados, nunca hay dos aristas paralelas seguidas.
    //    Izquierda plantada, derecha atrasada => postura de acecho.
    // =====================================================================
    hn_limb(buf,i, -0.095f,0.950f, 0.020f, -0.112f,0.470f,-0.030f, 0.105f,0.082f, 6, coatDk, coat);    // muslo (tapado por el abrigo)
    hn_limb(buf,i, -0.112f,0.470f,-0.030f, -0.108f,0.088f, 0.012f, 0.100f,0.058f, 6, bootLit, bootDk);  // cana de la bota (r0 ancho = vuelta)
    hn_limb(buf,i, -0.108f,0.088f, 0.012f, -0.108f,0.052f,-0.185f, 0.062f,0.042f, 6, bootDk, boot);    // pie
    hn_limb(buf,i,  0.095f,0.950f, 0.038f,  0.125f,0.478f, 0.058f, 0.105f,0.082f, 6, coatDk, coat);
    hn_limb(buf,i,  0.125f,0.478f, 0.058f,  0.118f,0.092f, 0.088f, 0.100f,0.058f, 6, bootLit, bootDk);
    hn_limb(buf,i,  0.118f,0.092f, 0.088f,  0.118f,0.052f,-0.098f, 0.062f,0.042f, 6, bootDk, boot);

    // =====================================================================
    // 2) FALDON DEL ABRIGO - loft de 4 anillos que se ENSANCHA hacia abajo
    //    (0.182 en la cintura -> 0.332 en el borde, a la altura de la rodilla).
    //    Es la pieza que mata la lectura de "cajas apiladas": una campana
    //    continua, con la boca oscurecida para que se lea el volumen.
    // =====================================================================
    {
        const float ys[4] = { 0.455f, 0.680f, 0.920f, 1.140f };
        const float rs[4] = { 0.332f, 0.286f, 0.230f, 0.182f };
        hn_loft(buf,i, 0.000f, 0.025f, ys, rs, 4, 8, hemDk, coat);
    }
    // jirones del dobladillo: conos que AFINAN A CERO => el borde no cierra en
    // un circulo perfecto ni en una arista recta.
    hn_limb(buf,i,  0.020f,0.478f, 0.330f,  0.048f,0.288f, 0.412f, 0.086f,0.0f, 6, coatDk, hemDk);
    hn_limb(buf,i, -0.238f,0.470f, 0.252f, -0.298f,0.322f, 0.310f, 0.076f,0.0f, 6, coatDk, hemDk);
    hn_limb(buf,i,  0.316f,0.474f, 0.048f,  0.378f,0.306f, 0.052f, 0.076f,0.0f, 6, coatDk, hemDk);
    hn_limb(buf,i, -0.214f,0.468f,-0.212f, -0.266f,0.344f,-0.278f, 0.070f,0.0f, 6, coatDk, hemDk);
    // vivos rojos bajando por los cantos traseros del abrigo
    hn_limb(buf,i, -0.148f,1.400f, 0.120f, -0.220f,0.520f, 0.262f, 0.016f,0.013f, 6, red, redDk);
    hn_limb(buf,i,  0.148f,1.400f, 0.120f,  0.220f,0.520f, 0.262f, 0.016f,0.013f, 6, red, redDk);

    // =====================================================================
    // 3) TORSO - segundo loft, corrido a -Z respecto del faldon: el tronco va
    //    INCLINADO hacia adelante (encorvado). Cintura 0.188 -> pecho 0.208 ->
    //    hombro 0.190: reloj de arena, no un prisma.
    // =====================================================================
    {
        const float ys[3] = { 1.115f, 1.310f, 1.468f };
        const float rs[3] = { 0.188f, 0.208f, 0.190f };
        hn_loft(buf,i, 0.000f,-0.030f, ys, rs, 3, 8, coat, coatLit);
    }
    // cinturon: anillo mas gordo que el torso, y de paso TAPA la junta
    // faldon/torso (los dos lofts estan desalineados en Z a proposito).
    hn_limb(buf,i, 0.000f,1.082f, 0.000f, 0.000f,1.176f,-0.010f, 0.206f,0.200f, 8, belt, belt);

    // =====================================================================
    // 4) ESCLAVINA (capa corta de hombros) - loft invertido: ancho abajo
    //    (0.296) y angosto arriba (0.215). Rompe el cilindro del torso y da el
    //    trapecio caracteristico del cazador.
    // =====================================================================
    {
        const float ys[2] = { 1.238f, 1.478f };
        const float rs[2] = { 0.296f, 0.215f };
        hn_loft(buf,i, 0.000f,-0.035f, ys, rs, 2, 8, cp_scale(coat,0.55f), coatLit);
    }
    hn_ball(buf,i, -0.172f,1.448f,-0.028f, 0.088f,0.078f,0.092f, 2,6, coatLit);   // hombro izq redondeado
    hn_ball(buf,i,  0.172f,1.448f,-0.028f, 0.088f,0.078f,0.092f, 2,6, coatLit);   // hombro der redondeado
    // correas rojas cruzadas en la ESPALDA (es lo que mira la camara)
    hn_limb(buf,i, -0.150f,1.428f, 0.105f,  0.152f,1.070f, 0.165f, 0.028f,0.026f, 6, red, redDk);
    hn_limb(buf,i,  0.150f,1.428f, 0.105f, -0.152f,1.070f, 0.165f, 0.028f,0.026f, 6, red, redDk);
    // unica caja del modelo: hebilla plana donde cruzan las correas
    hn_box(buf,i, 0.000f,1.225f, 0.140f, 0.070f,0.030f,0.048f, brass);

    // =====================================================================
    // 5) BRAZOS - hombro/codo/muneca con quiebre real y radios decrecientes;
    //    el r0 del antebrazo es mayor que el r1 del brazo => bocamanga.
    // =====================================================================
    hn_limb(buf,i, -0.195f,1.420f,-0.025f, -0.235f,1.145f,-0.065f, 0.072f,0.056f, 6, coat,   coatLit); // brazo izq
    hn_limb(buf,i, -0.235f,1.145f,-0.065f, -0.232f,0.885f,-0.130f, 0.060f,0.042f, 5, coatDk, coat);    // antebrazo izq
    hn_ball(buf,i, -0.232f,0.848f,-0.145f, 0.050f,0.056f,0.050f, 2,5, skin);                           // mano izq
    hn_limb(buf,i,  0.195f,1.420f,-0.025f,  0.255f,1.140f,-0.060f, 0.072f,0.056f, 6, coat,   coatLit); // brazo der
    hn_limb(buf,i,  0.255f,1.140f,-0.060f,  0.262f,0.878f,-0.158f, 0.060f,0.042f, 5, coatDk, coat);    // antebrazo der
    hn_ball(buf,i,  0.262f,0.843f,-0.178f, 0.052f,0.058f,0.052f, 2,5, skin);                           // mano der
    // dos dedos cerrados cruzando la empunadura: la mano AGARRA, no flota
    hn_limb(buf,i,  0.226f,0.872f,-0.170f,  0.300f,0.856f,-0.186f, 0.021f,0.017f, 6, skin, skinDk);
    hn_limb(buf,i,  0.232f,0.832f,-0.184f,  0.303f,0.818f,-0.200f, 0.019f,0.015f, 6, skin, skinDk);

    // =====================================================================
    // 6) CUELLO + CUELLO ALTO + CABEZA + CAPUCHA
    //    El cuello alto es un loft que se ABRE hacia arriba (0.150 -> 0.182);
    //    la capucha es un elipsoide corrido a +Z con una visera conica a -Z,
    //    asi la cara asoma por la abertura en vez de quedar tapada.
    // =====================================================================
    hn_limb(buf,i, 0.000f,1.400f,-0.055f, 0.000f,1.575f,-0.075f, 0.062f,0.055f, 6, coatDk, skinDk);
    {
        const float ys[2] = { 1.395f, 1.600f };
        const float rs[2] = { 0.150f, 0.182f };
        hn_loft(buf,i, 0.000f,-0.055f, ys, rs, 2, 8, coat, coatDk);
    }
    hn_ball(buf,i, 0.000f,1.672f,-0.078f, 0.092f,0.113f,0.098f, 3,6, skinDk);            // cabeza (cara en sombra)
    hn_limb(buf,i, -0.060f,1.565f, 0.085f, -0.085f,1.430f, 0.130f, 0.045f,0.018f, 6, hair, hairDk); // mechon izq
    hn_limb(buf,i,  0.060f,1.565f, 0.085f,  0.085f,1.430f, 0.130f, 0.045f,0.018f, 6, hair, hairDk); // mechon der
    hn_ball(buf,i, 0.000f,1.700f, 0.005f, 0.138f,0.148f,0.148f, 4,6, coat);              // masa de la capucha
    hn_limb(buf,i, 0.000f,1.715f,-0.085f, 0.000f,1.598f,-0.242f, 0.090f,0.012f, 6, coat, coatDk); // visera (pico a -Z)
    hn_limb(buf,i, 0.000f,1.700f, 0.105f, 0.020f,1.455f, 0.190f, 0.120f,0.050f, 6, coatDk, hemDk);  // tela cayendo por la espalda

    // =====================================================================
    // 7) SABLE - mano derecha, punta abajo y adelante. Nada de prismas grises:
    //    empunadura que ENGROSA hacia la mano, pomo esferico, guarda de dos
    //    quillones cruzados y una HOJA CURVA de 4 tramos que pasa de r=0.046
    //    junto a la guarda a r=0.004 en la punta (radio /11). El degradado
    //    colB->colT va de lomo (oscuro, junto a la guarda) a filo (acero claro,
    //    hacia la punta); ademas corre un nervio brillante por el canto de
    //    corte y el sombreado por normal de addLimb apaga solo las caras que
    //    miran al otro lado => lomo oscuro / filo encendido.
    // =====================================================================
    hn_ball(buf,i, 0.242f,0.934f,-0.120f, 0.036f,0.040f,0.036f, 2,5, brass);                            // pomo
    hn_limb(buf,i, 0.246f,0.923f,-0.128f, 0.279f,0.793f,-0.223f, 0.026f,0.034f, 6, grip,  gripLit);     // empunadura (ensancha a la mano)
    hn_limb(buf,i, 0.199f,0.798f,-0.220f, 0.369f,0.766f,-0.252f, 0.020f,0.012f, 6, brass, brassDk);     // quillon 1
    hn_limb(buf,i, 0.266f,0.810f,-0.318f, 0.302f,0.753f,-0.156f, 0.018f,0.011f, 6, brass, brassDk);     // quillon 2 (cruzado)
    {
        // hoja curva: la componente -Z crece tramo a tramo (-0.054, -0.070,
        // -0.096, -0.127) => barrido de sable, no una barra recta.
        const float bX[5] = {  0.284f,  0.294f,  0.302f,  0.306f,  0.308f };
        const float bY[5] = {  0.781f,  0.613f,  0.443f,  0.283f,  0.148f };
        const float bZ[5] = { -0.236f, -0.290f, -0.360f, -0.456f, -0.583f };
        const float bR[5] = {  0.046f,  0.039f,  0.031f,  0.020f,  0.004f };
        for (int s = 0; s < 4; ++s) {
            unsigned int c0 = cp_lerp(spine, edge, (float)s * 0.25f);
            unsigned int c1 = cp_lerp(spine, edge, (float)(s + 1) * 0.25f);
            hn_limb(buf,i, bX[s],bY[s],bZ[s], bX[s+1],bY[s+1],bZ[s+1], bR[s],bR[s+1], 6, c0, c1);
        }
        // nervio de FILO: hilo brillante pegado al canto de corte (adelante-abajo)
        hn_limb(buf,i, 0.285f,0.791f,-0.269f, 0.302f,0.453f,-0.392f, 0.013f,0.011f, 3, cp_lerp(spine,edge,0.5f), edge);
        hn_limb(buf,i, 0.302f,0.453f,-0.392f, 0.308f,0.150f,-0.590f, 0.011f,0.002f, 6, edge, edge);
    }

    return i;
}
