#pragma once
// ============================================================================
// HUNTER - cazador gotico (ref. Bloodborne) visto en 3RA PERSONA DESDE ATRAS.
//
// VERSION 2 (2026-10-16, Benjamin: "mejora el diseno del personaje y mejora el diseno al
// momento de correr, se ve muy raro"; "usa los bordes o resolucion del serafin").
//   - El doble de resolucion: miembros de 8-10 lados (eran 5-6), cabeza 5x10, capucha 5x12,
//     torso y abrigo con anillos ELIPTICOS (un torso es mas ancho que profundo).
//   - El faldon ya no es una campana rigida: son CUATRO PANELES (dos laterales que siguen a
//     los muslos y dos colas traseras que flamean), con FORRO ROJO por dentro y jirones en el
//     borde. Corriendo, la campana entera giraba hacia atras como una tabla: eso era lo raro.
//   - Esclavina con jirones, capucha con punta caida y borde que atrapa la luz, cara palida con
//     bufanda-mascara, guantes y botas de cuero con su vuelta, cinturon con hebilla y bolsas.
//
// Todo sale de char_prims.h (addLimb/addBall) y gfx_prims.h (tubos, lofts elipticos, jirones,
// cajas biseladas). ESCALA: se autoria en METROS (1.80 de alto, pies en y=0, mirando a -Z) y se
// emite en unidades de mundo con HN_S. Tocar SOLO HN_S para re-escalar.
// ============================================================================

static const float HN_S = 1.92f;   // 1.80 m * 1.92 = 3.456 unidades de mundo

// ---- wrappers de escala (autoria en metros -> unidades de mundo) ----
static void hn_limb(LineVertex *b, int &i, float x0, float y0, float z0, float x1, float y1, float z1,
                    float r0, float r1, int sides, unsigned int cB, unsigned int cT) {
    addLimb(b, i, x0*HN_S, y0*HN_S, z0*HN_S, x1*HN_S, y1*HN_S, z1*HN_S, r0*HN_S, r1*HN_S, sides, cB, cT);
}
static void hn_tube(LineVertex *b, int &i, float x0, float y0, float z0, float x1, float y1, float z1,
                    float r0, float r1, int sides, unsigned int cB, unsigned int cT) {
    gpTube(b, i, x0*HN_S, y0*HN_S, z0*HN_S, x1*HN_S, y1*HN_S, z1*HN_S, r0*HN_S, r1*HN_S, sides, cB, cT);
}
static void hn_ball(LineVertex *b, int &i, float cx, float cy, float cz, float rx, float ry, float rz,
                    int stacks, int slices, unsigned int col) {
    addBall(b, i, cx*HN_S, cy*HN_S, cz*HN_S, rx*HN_S, ry*HN_S, rz*HN_S, stacks, slices, col);
}
static void hn_box(LineVertex *b, int &i, float cx, float y0, float cz, float w, float d, float h, float bv,
                   unsigned int cTop, unsigned int cBot) {
    gpBevelBox(b, i, cx*HN_S, y0*HN_S, cz*HN_S, w*HN_S, d*HN_S, h*HN_S, bv*HN_S, cTop, cBot);
}
// loft eliptico: hasta 8 anillos
static void hn_loftE(LineVertex *b, int &i, int n, const float *ys, const float *rx, const float *rz, const float *oz,
                     const unsigned int *col, int segs, float th0, float th1, float scaleR = 1.0f) {
    float Y[8], X[8], Z[8], O[8];
    if (n > 8) n = 8;
    for (int k = 0; k < n; ++k) { Y[k] = ys[k]*HN_S; X[k] = rx[k]*scaleR*HN_S; Z[k] = rz[k]*scaleR*HN_S; O[k] = oz[k]*HN_S; }
    gpLoftE(b, i, 0.0f, 0.0f, n, Y, X, Z, O, col, segs, th0, th1);
}
static void hn_tatters(LineVertex *b, int &i, float y, float rx, float rz, float oz, float th0, float th1,
                       int count, float len, unsigned int seed, unsigned int cTop, unsigned int cTip) {
    gpTatters(b, i, 0.0f, 0.0f, y*HN_S, rx*HN_S, rz*HN_S, oz*HN_S, th0, th1, count, len*HN_S, seed, cTop, cTip);
}

// ============================================================================
// PIEZAS ARTICULADAS (rig.h dibuja cada una con su giro): g_hnMark[k] = primer vertice de la pieza k
//   [0]muslo izq [1]cana de la bota izq [2]PIE izq [3]muslo der [4]cana der [5]PIE der
//   [6]cola trasera izq [7]cola trasera der [8]panel lateral izq [9]panel lateral der
//   [10]PELVIS (cintura, cinturon, bolsas) [11]PECHO (pecho, correas, esclavina, cuello)
//   [12]brazo izq [13]antebrazo izq [14]MANO+PISTOLA (gira en la muneca) [15]brazo der
//   [16]antebrazo+mano+SABLE der [17]cabeza/capucha [18]fin.
// v3 (2026-10-16, Benjamin: "se ve muy tosco y rigido"): el torso era UN bloque y el pie iba pegado
// a la cana. Ahora el pecho gira contra la pelvis (los hombros contra las caderas al caminar, el
// cuerpo entero en el tajo) y el pie tiene TOBILLO: apoya plano, despega con la punta. La mano de
// la pistola tiene MUNECA: al apuntar con el brazo estirado la pistola apuntaba al CIELO (iba pegada
// al antebrazo, que al levantarse la giraba 80 grados).
// Cada pieza va CONTIGUA en el buffer. Las articulaciones (pivotes) estan en rig.h (HN_J*).
// ============================================================================
static const int HN_NMARK = 19;
static int g_hnMark[HN_NMARK];
static const int HN_CAP = 12000;    // tamano de g_hero en main.cpp: build_hunter no debe pasarse

static int build_hunter(LineVertex *buf)
{
    int i = 0;
    g_hnMark[0] = 0;

    // ---------------- paleta ----------------
    const unsigned int coat    = RGBA( 30,  29,  36, 255);   // abrigo casi negro, un pelo azulado
    const unsigned int coatLit = cp_scale(coat, 1.70f);      // pliegues altos / hombros
    const unsigned int coatDk  = cp_scale(coat, 0.62f);
    const unsigned int hemDk   = cp_scale(coat, 0.45f);      // dobladillo (oclusion)
    const unsigned int lining  = RGBA(112,  26,  32, 255);   // forro rojo sangre
    const unsigned int liningDk= cp_scale(lining, 0.55f);
    const unsigned int red     = RGBA(132,  32,  38, 255);   // correas / vivos
    const unsigned int redDk   = cp_scale(red, 0.55f);
    const unsigned int pants   = RGBA( 38,  36,  42, 255);
    const unsigned int leather = RGBA( 52,  38,  30, 255);   // botas, guantes, cinturon
    const unsigned int leaLit  = cp_scale(leather, 1.45f);
    const unsigned int leaDk   = cp_scale(leather, 0.62f);
    const unsigned int skin    = RGBA(198, 178, 166, 255);
    const unsigned int skinDk  = cp_scale(skin, 0.62f);
    const unsigned int scarf   = RGBA( 24,  23,  28, 255);
    const unsigned int edge    = RGBA(206, 214, 232, 255);   // filo pulido
    const unsigned int spine   = RGBA( 96, 104, 122, 255);   // lomo de la hoja
    const unsigned int brass   = RGBA(164, 132,  66, 255);
    const unsigned int brassDk = cp_scale(brass, 0.65f);
    const unsigned int grip    = RGBA( 48,  36,  32, 255);
    const unsigned int gripLit = cp_scale(grip, 1.35f);

    // =====================================================================
    // 1) PIERNAS: muslo (pantalon) | cana de la bota con su vuelta + pie con suela.
    // =====================================================================
    for (int side = -1; side <= 1; side += 2) {
        const float s = (float)side;
        const int m0 = side < 0 ? 0 : 3;
        g_hnMark[m0] = i;
        // muslo: de la cadera a la rodilla, un poco hacia adelante
        hn_tube(buf,i, s*0.095f,0.960f, 0.000f, s*0.105f,0.500f,-0.012f, 0.088f,0.064f, 10, cp_scale(pants,0.8f), pants);
        hn_ball(buf,i, s*0.105f,0.500f,-0.014f, 0.064f,0.062f,0.066f, 2,8, pants);                   // rodilla
        g_hnMark[m0 + 1] = i;
        // cana de la bota: de la rodilla al tobillo, la vuelta de cuero arriba
        hn_tube(buf,i, s*0.105f,0.470f,-0.010f, s*0.110f,0.095f, 0.020f, 0.066f,0.056f, 10, leaLit, leather);
        hn_tube(buf,i, s*0.105f,0.505f,-0.012f, s*0.105f,0.430f,-0.008f, 0.080f,0.076f, 10, leaLit, leather);   // vuelta
        hn_ball(buf,i, s*0.110f,0.085f, 0.022f, 0.058f,0.052f,0.062f, 3,10, leather);                // tobillo / talon
        g_hnMark[m0 + 2] = i;
        // PIE (gira en el tobillo): empeine que afina hacia la punta + suela oscura que sobresale apenas
        hn_tube(buf,i, s*0.110f,0.075f, 0.040f, s*0.112f,0.050f,-0.165f, 0.054f,0.036f, 10, leather, leaLit);
        hn_box(buf,i, s*0.111f,0.000f,-0.058f, 0.105f,0.255f,0.026f, 0.010f, leaDk, cp_scale(leaDk,0.7f));
    }

    // =====================================================================
    // 2) FALDON: 4 paneles elipticos que se ACAMPANAN, cada uno con forro rojo y jirones.
    //    Arcos (th = 0 adelante, PI/2 derecha): adelante queda ABIERTO (se ven las piernas).
    // =====================================================================
    {
        const float ys[4] = { 1.140f, 0.950f, 0.720f, 0.440f };
        const float rx[4] = { 0.200f, 0.238f, 0.275f, 0.318f };
        const float rz[4] = { 0.150f, 0.192f, 0.232f, 0.272f };
        const float oz[4] = { 0.000f, 0.012f, 0.024f, 0.036f };
        const unsigned int cO[4] = { coat, coat, coatDk, hemDk };
        const unsigned int cL[4] = { liningDk, lining, liningDk, cp_scale(liningDk, 0.7f) };
        const float PI_ = 3.14159265f, TAU_ = 6.2831853f;
        // colas traseras (se separan en el centro de la espalda) y paneles laterales
        const float arcs[4][2] = { { PI_, TAU_ - 1.95f }, { 1.95f, PI_ }, { TAU_ - 1.95f, TAU_ - 0.52f }, { 0.52f, 1.95f } };
        for (int p = 0; p < 4; ++p) {
            g_hnMark[6 + p] = i;
            const float a0 = arcs[p][0], a1 = arcs[p][1];
            const float yl = (p < 2) ? 0.400f : 0.450f;            // las colas, un poco mas largas
            float ys2[4] = { ys[0], ys[1], ys[2], yl };
            hn_loftE(buf,i, 4, ys2, rx, rz, oz, cO, 6, a0, a1);           // tela
            hn_loftE(buf,i, 4, ys2, rx, rz, oz, cL, 4, a0, a1, 0.93f);    // forro por dentro
            hn_tatters(buf,i, yl, rx[3], rz[3], oz[3], a0, a1, 4, 0.11f, 17u + p * 5u, coatDk, hemDk);
            if (p >= 2) {   // vivo rojo por el canto delantero del panel lateral
                const float ae = (p == 2) ? a1 : a0;
                hn_tube(buf,i, sinf(ae)*rx[0],ys[0],-cosf(ae)*rz[0], sinf(ae)*rx[3],yl,oz[3]-cosf(ae)*rz[3], 0.012f,0.010f, 6, red, redDk);
            }
        }
    }

    // =====================================================================
    // 3) TORSO en DOS piezas: PELVIS (cintura, cinturon con hebilla y bolsas) y PECHO (pecho
    //    adelantado, hombros, correas rojas cruzadas en la espalda, esclavina con jirones, cuello
    //    alto). El pecho arranca DENTRO de la pelvis (1.20 < 1.29): al girar no se abre una
    //    rendija, y la costura queda bajo la esclavina.
    // =====================================================================
    g_hnMark[10] = i;
    {
        const float ys[3] = { 1.060f, 1.160f, 1.290f };
        const float rx[3] = { 0.172f, 0.166f, 0.188f };
        const float rz[3] = { 0.122f, 0.116f, 0.131f };
        const float oz[3] = { 0.000f, 0.000f,-0.011f };
        const unsigned int c[3] = { coatDk, coat, coat };
        hn_loftE(buf,i, 3, ys, rx, rz, oz, c, 12, 0.0f, 6.2831853f);
    }
    {   // cinturon (banda eliptica) + hebilla + 2 bolsas de cuero
        const float ys[2] = { 1.085f, 1.150f }, rx[2] = { 0.186f, 0.182f }, rz[2] = { 0.136f, 0.132f }, oz[2] = { 0.0f, 0.0f };
        const unsigned int c[2] = { leaDk, leather };
        hn_loftE(buf,i, 2, ys, rx, rz, oz, c, 14, 0.0f, 6.2831853f);
        hn_box(buf,i, 0.000f,1.090f,-0.138f, 0.060f,0.018f,0.055f, 0.008f, brass, brassDk);
        hn_box(buf,i, 0.150f,1.030f, 0.080f, 0.080f,0.050f,0.090f, 0.014f, leaLit, leaDk);
        hn_box(buf,i,-0.130f,1.035f, 0.100f, 0.070f,0.050f,0.080f, 0.014f, leaLit, leaDk);
    }
    g_hnMark[11] = i;
    {   // PECHO
        const float ys[5] = { 1.200f, 1.280f, 1.380f, 1.455f, 1.505f };
        const float rx[5] = { 0.160f, 0.184f, 0.202f, 0.192f, 0.150f };
        const float rz[5] = { 0.112f, 0.129f, 0.136f, 0.122f, 0.100f };
        const float oz[5] = { 0.000f,-0.010f,-0.016f,-0.010f, 0.000f };
        const unsigned int c[5] = { coat, coat, coatLit, coatLit, coat };
        hn_loftE(buf,i, 5, ys, rx, rz, oz, c, 12, 0.0f, 6.2831853f);
    }
    // correas rojas cruzadas en la ESPALDA (lo que mira la camara) + hebilla donde se cruzan
    hn_tube(buf,i, -0.150f,1.430f, 0.112f,  0.150f,1.110f, 0.132f, 0.020f,0.020f, 8, red, redDk);
    hn_tube(buf,i,  0.150f,1.430f, 0.112f, -0.150f,1.110f, 0.132f, 0.020f,0.020f, 8, red, redDk);
    hn_box(buf,i, 0.000f,1.240f, 0.134f, 0.060f,0.020f,0.050f, 0.008f, brass, brassDk);
    {   // ESCLAVINA (capa corta de hombros): ancha abajo, con jirones en el borde
        const float ys[4] = { 1.540f, 1.460f, 1.330f, 1.240f };
        const float rx[4] = { 0.140f, 0.252f, 0.305f, 0.322f };
        const float rz[4] = { 0.118f, 0.198f, 0.244f, 0.262f };
        const float oz[4] = { 0.000f, 0.004f, 0.012f, 0.020f };
        const unsigned int c[4] = { coatLit, coatLit, coat, coatDk };
        hn_loftE(buf,i, 4, ys, rx, rz, oz, c, 13, 0.0f, 6.2831853f);
        hn_tatters(buf,i, 1.240f, 0.322f, 0.262f, 0.020f, 0.0f, 6.2831853f, 13, 0.085f, 3u, coatDk, hemDk);
    }
    {   // cuello alto que se ABRE hacia arriba
        const float ys[2] = { 1.470f, 1.610f }, rx[2] = { 0.125f, 0.150f }, rz[2] = { 0.108f, 0.128f }, oz[2] = { -0.02f, -0.01f };
        const unsigned int c[2] = { coat, coatLit };
        hn_loftE(buf,i, 2, ys, rx, rz, oz, c, 12, 0.0f, 6.2831853f);
    }
    g_hnMark[12] = i;   // (los hombros los redondea la esclavina: bolas debajo no se veian)

    // =====================================================================
    // 4) BRAZOS: brazo | antebrazo con bocamanga, guante y mano que AGARRA.
    // =====================================================================
    // izquierdo: la PISTOLA DE PULSOS
    hn_tube(buf,i, -0.200f,1.430f,-0.012f, -0.236f,1.150f,-0.052f, 0.062f,0.052f, 8, coat, coatLit);
    hn_ball(buf,i, -0.236f,1.150f,-0.052f, 0.053f,0.053f,0.053f, 3,8, coat);
    g_hnMark[13] = i;
    hn_tube(buf,i, -0.236f,1.150f,-0.052f, -0.234f,0.920f,-0.112f, 0.052f,0.044f, 8, coatDk, coat);
    hn_tube(buf,i, -0.235f,0.975f,-0.098f, -0.234f,0.900f,-0.118f, 0.060f,0.056f, 8, leaLit, leather);   // bocamanga del guante
    g_hnMark[14] = i;   // MANO + PISTOLA (pivote en la muneca: rig.h HN_JWRIST_L)
    hn_ball(buf,i, -0.233f,0.862f,-0.140f, 0.046f,0.054f,0.048f, 3,8, leather);                          // mano enguantada
    {
        const unsigned int gun = RGBA(62, 66, 80, 255), gunLit = cp_scale(gun, 1.65f), gunDk = cp_scale(gun, 0.6f);
        const unsigned int glow = RGBA(130, 226, 255, 255);
        hn_limb(buf,i, -0.232f,0.868f,-0.110f, -0.232f,0.868f,-0.268f, 0.040f,0.034f, 8, gun, gunLit);    // cuerpo
        hn_limb(buf,i, -0.232f,0.848f,-0.160f, -0.232f,0.842f,-0.352f, 0.024f,0.021f, 8, gunDk, gun);     // canon
        hn_limb(buf,i, -0.232f,0.848f,-0.250f, -0.232f,0.848f,-0.290f, 0.032f,0.032f, 8, brass, brassDk); // anillo
        hn_limb(buf,i, -0.232f,0.856f,-0.112f, -0.238f,0.770f,-0.088f, 0.024f,0.020f, 6, grip, gripLit);  // culata
        hn_ball(buf,i, -0.232f,0.842f,-0.356f, 0.020f,0.020f,0.020f, 2,6, glow);                           // boca de luz
    }
    g_hnMark[15] = i;
    // derecho: el SABLE
    hn_tube(buf,i,  0.200f,1.430f,-0.012f,  0.250f,1.145f,-0.052f, 0.062f,0.052f, 8, coat, coatLit);
    hn_ball(buf,i,  0.250f,1.145f,-0.052f, 0.053f,0.053f,0.053f, 3,8, coat);
    g_hnMark[16] = i;
    hn_tube(buf,i,  0.250f,1.145f,-0.052f,  0.262f,0.905f,-0.150f, 0.052f,0.044f, 8, coatDk, coat);
    hn_tube(buf,i,  0.259f,0.960f,-0.128f,  0.262f,0.890f,-0.158f, 0.060f,0.056f, 8, leaLit, leather);
    hn_ball(buf,i,  0.262f,0.846f,-0.178f, 0.048f,0.056f,0.050f, 3,8, leather);
    hn_limb(buf,i,  0.226f,0.872f,-0.170f,  0.300f,0.856f,-0.186f, 0.020f,0.017f, 6, leather, leaDk);   // dedos cerrados
    hn_limb(buf,i,  0.232f,0.832f,-0.184f,  0.303f,0.818f,-0.200f, 0.019f,0.015f, 6, leather, leaDk);
    // sable: pomo, empunadura que engrosa, dos quillones cruzados y HOJA CURVA de 4 tramos
    hn_ball(buf,i, 0.242f,0.934f,-0.120f, 0.034f,0.038f,0.034f, 3,8, brass);
    hn_limb(buf,i, 0.246f,0.923f,-0.128f, 0.279f,0.793f,-0.223f, 0.025f,0.033f, 8, grip,  gripLit);
    hn_limb(buf,i, 0.199f,0.798f,-0.220f, 0.369f,0.766f,-0.252f, 0.019f,0.011f, 8, brass, brassDk);
    hn_limb(buf,i, 0.266f,0.810f,-0.318f, 0.302f,0.753f,-0.156f, 0.017f,0.010f, 8, brass, brassDk);
    {
        const float bX[5] = {  0.284f,  0.294f,  0.302f,  0.306f,  0.308f };
        const float bY[5] = {  0.781f,  0.613f,  0.443f,  0.283f,  0.148f };
        const float bZ[5] = { -0.236f, -0.290f, -0.360f, -0.456f, -0.583f };
        const float bR[5] = {  0.044f,  0.038f,  0.030f,  0.019f,  0.004f };
        for (int s = 0; s < 4; ++s)
            hn_limb(buf,i, bX[s],bY[s],bZ[s], bX[s+1],bY[s+1],bZ[s+1], bR[s],bR[s+1], 8,
                    cp_lerp(spine, edge, s * 0.25f), cp_lerp(spine, edge, (s + 1) * 0.25f));
        // nervio del FILO: sigue la MISMA curva de la hoja, pegado a su canto de adelante (recto, se
        // separaba de la hoja y en la punta se veia una horquilla)
        for (int s = 0; s < 4; ++s)
            hn_limb(buf,i, bX[s],bY[s]-0.004f,bZ[s]-bR[s]*0.75f, bX[s+1],bY[s+1]-0.004f,bZ[s+1]-bR[s+1]*0.75f,
                    0.011f*(1.0f-0.2f*s),0.011f*(1.0f-0.2f*(s+1))+0.001f, 4, cp_lerp(spine,edge,0.5f+0.12f*s), edge);
    }
    g_hnMark[17] = i;

    // =====================================================================
    // 5) CABEZA: cara palida con BUFANDA-MASCARA, capucha con borde que atrapa la luz, punta
    //    que cae hacia atras y la tela que baja por la espalda.
    // =====================================================================
    hn_tube(buf,i, 0.000f,1.470f,-0.030f, 0.000f,1.600f,-0.055f, 0.056f,0.052f, 8, skinDk, skinDk);   // cuello
    hn_ball(buf,i, 0.000f,1.680f,-0.072f, 0.086f,0.106f,0.094f, 4,8, skin);                           // cara
    hn_ball(buf,i, -0.034f,1.700f,-0.158f, 0.014f,0.010f,0.008f, 2,6, RGBA(24, 20, 22, 255));          // ojos (en sombra)
    hn_ball(buf,i,  0.034f,1.700f,-0.158f, 0.014f,0.010f,0.008f, 2,6, RGBA(24, 20, 22, 255));
    hn_ball(buf,i, 0.000f,1.632f,-0.082f, 0.096f,0.066f,0.102f, 3,8, scarf);                          // bufanda sobre boca y nariz
    // masa de la capucha: corrida hacia ATRAS para que la cara asome por delante
    hn_ball(buf,i, 0.000f,1.705f, 0.036f, 0.136f,0.152f,0.130f, 4,10, coat);
    // borde de la abertura: un aro vertical alrededor de la cara (atrapa la luz como la nariz dorada
    // de los peldanos del Serafin)
    gpRingXY(buf,i, 0.000f*HN_S,1.682f*HN_S,-0.104f*HN_S, 0.108f*HN_S, 0.030f*HN_S, 0.046f*HN_S, 10, coatLit);
    hn_limb(buf,i, 0.000f,1.786f,-0.040f, 0.000f,1.764f,-0.138f, 0.074f,0.034f, 8, coatLit, coat);   // ala de la capucha sobre los ojos
    hn_limb(buf,i, 0.000f,1.800f, 0.050f, 0.000f,1.760f, 0.230f, 0.070f,0.004f, 8, coat, coatDk);    // punta que cae atras
    hn_limb(buf,i, 0.000f,1.690f, 0.100f, 0.016f,1.470f, 0.180f, 0.112f,0.060f, 8, coatDk, hemDk);   // tela por la espalda
    g_hnMark[18] = i;

    return i;
}
