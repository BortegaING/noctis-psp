// ============================================================================
// boss_red.h -- PISO 5: "EL JINETE ROJO" (GUERRA). Modulo de jefe, prefijo br_.
//
// Contrato en boss_api.h (BossIO, helpers lv*/bx*). Se incluye desde levels.h DESPUES de
// boss_api.h, skyworld.h (swQuadC/swBoxLit/swBoxL/swRand), char_prims.h (addLimb/addBall/addLoft).
//
// ESCENARIO: cielo rojo carnoso (venas y manchas que laten), arena de piedra/carne rojiza de
//   radio ~42 (el doble, para correr y escapar) con borde organico (el suelo es una pila de franjas-caja), costillas de hueso en el
//   borde, tendones colgando, columnas de carne lejanas y niebla rojo oscuro.
// JINETE: caballo rojo (crin oscura, ojos brillantes, 4 patas articuladas que corren), jinete con
//   armadura, casco con cuernos, capa y una GRAN ESPADA. Escala RJ_S sobre el modelo (mira a -Z).
// PATRON:
//   PACE    trota alrededor tuyo. Si estas cerca: TAJO (onda roja en el piso: se SALTA).
//   ALIGN   corre al borde opuesto de la arena.
//   ROAR    BRAMA: se encabrita, linea en el piso hacia ti (se fija al final del aviso).
//   CHARGE  EMBISTE en linea recta por la linea fijada: 35 de danio si te toca (se esquiva
//           moviendote o saltando).
//   SKID    frena al otro lado. Si FALLO: SCREAM -> GRITA, tinte rojo, temblor fuerte y quedas
//           aturdido: 2 de danio cada 20 frames (3 ticks/s) hasta mover el analogo (BxMash).
//   Vida 110. Al morir cae, se deshace en ceniza roja y br_Done() = 1.
// Convenciones: rumbo rj_a -> direccion (sin a, cos a); yaw del modelo = rj_a + PI.
// ============================================================================
#pragma once

// ---------------------------------------------------------------- constantes
static const float RJ_PI   = 3.14159265f;
static const float RJ_TAU  = 6.2831853f;
static const float RJ_R    = 42.0f;        // radio nominal de la arena (x1.9: espacio para correr y escapar)
static const float RJ_K    = 42.0f / 22.0f; // factor de escala de la arena respecto al diseno base
static const float RJ_VMAX = 1.7f, RJ_VMAXE = 2.0f, RJ_DECEL = 0.13f;   // embestida: velocidad, enfurecido, frenado
static const int   RJ_HPMAX = 110;
static const float RJ_S    = 1.4f;         // escala del modelo (unidades del modelo -> mundo)
static const float RJ_PIV  = 1.8f;         // pivote del encabritado (z de los cascos traseros)
static const float RJ_MASHNEED = 8.0f;     // recorrido del analogo para soltarse del grito
static const int   RJ_MASHFRAMES = 420;    // tope de tiempo del aturdimiento
static const int   RJ_STR_N = 28;          // franjas maximas del suelo

// paleta
static const unsigned int RJ_HORSE  = RGBA(172, 26, 28, 255);
static const unsigned int RJ_HORSEL = RGBA(224, 60, 44, 255);
static const unsigned int RJ_HORSED = RGBA(104, 12, 20, 255);
static const unsigned int RJ_MANE   = RGBA(30, 4, 10, 255);
static const unsigned int RJ_MANET  = RGBA(156, 24, 14, 255);
static const unsigned int RJ_HOOF   = RGBA(26, 20, 24, 255);
static const unsigned int RJ_SOCK   = RGBA(64, 10, 16, 255);
static const unsigned int RJ_EYE    = RGBA(255, 246, 160, 255);
static const unsigned int RJ_IRON   = RGBA(94, 90, 104, 255);
static const unsigned int RJ_IRONL  = RGBA(142, 138, 154, 255);
static const unsigned int RJ_IROND  = RGBA(52, 48, 62, 255);
static const unsigned int RJ_GOLD   = RGBA(216, 166, 62, 255);
static const unsigned int RJ_CLOTH  = RGBA(148, 16, 24, 255);
static const unsigned int RJ_CLOTHD = RGBA(104, 10, 18, 255);
static const unsigned int RJ_BONE   = RGBA(234, 216, 188, 255);
static const unsigned int RJ_STEEL  = RGBA(230, 234, 244, 255);
static const unsigned int RJ_BLOOD  = RGBA(204, 36, 34, 255);

// ---------------------------------------------------------------- modelos (color por vertice)
static LineVertex __attribute__((aligned(16))) rj_mBody[7600];
static LineVertex __attribute__((aligned(16))) rj_mFUp[420];
static LineVertex __attribute__((aligned(16))) rj_mFLo[520];
static LineVertex __attribute__((aligned(16))) rj_mHUp[520];
static LineVertex __attribute__((aligned(16))) rj_mHLo[520];
static LineVertex __attribute__((aligned(16))) rj_mTail[1000];
static LineVertex __attribute__((aligned(16))) rj_mCape[400];
static LineVertex __attribute__((aligned(16))) rj_mArm[1400];
static int rj_nBody = 0, rj_nFUp = 0, rj_nFLo = 0, rj_nHUp = 0, rj_nHLo = 0, rj_nTail = 0, rj_nCape = 0, rj_nArm = 0;
static int rj_built = 0;
static int rj_capCur = 0;

// wrappers con tope de capacidad (si no cabe, se omite la pieza: nunca se escribe fuera)
static void rjL(LineVertex *b, int &i, float x0, float y0, float z0, float x1, float y1, float z1,
                float r0, float r1, int sides, unsigned int c0, unsigned int c1) {
    if (i + sides * 12 > rj_capCur) return;
    addLimb(b, i, x0, y0, z0, x1, y1, z1, r0, r1, sides, c0, c1);
}
static void rjB(LineVertex *b, int &i, float cx, float cy, float cz, float rx, float ry, float rz,
                int st, int sl, unsigned int c) {
    if (i + st * sl * 6 > rj_capCur) return;
    addBall(b, i, cx, cy, cz, rx, ry, rz, st, sl, c);
}
// hoja plana de espada: seccion en rombo (filos de acero, lomo rojo), punta en triangulo
static void rjBlade(LineVertex *b, int &i, float cx, float y0, float cz, float len, float w, float t,
                    unsigned int cEdge, unsigned int cRidge) {
    if (i + 60 > rj_capCur) return;
    const float ym = y0 + len * 0.80f, yt = y0 + len;
    const float bx[4] = { cx - w, cx, cx + w, cx };
    const float bz[4] = { cz, cz - t, cz, cz + t };
    const float mx[4] = { cx - w * 0.90f, cx, cx + w * 0.90f, cx };
    const float mz[4] = { cz, cz - t * 0.85f, cz, cz + t * 0.85f };
    const unsigned int cc[4] = { cEdge, cRidge, cEdge, cRidge };
    for (int k = 0; k < 4; ++k) {
        const int k1 = (k + 1) & 3;
        swQuadC(b, i, bx[k], y0, bz[k], cc[k], mx[k], ym, mz[k], cc[k],
                      mx[k1], ym, mz[k1], cc[k1], bx[k1], y0, bz[k1], cc[k1]);
        b[i++] = { cc[k], mx[k], ym, mz[k] };
        b[i++] = { cc[k1], mx[k1], ym, mz[k1] };
        b[i++] = { cEdge, cx, yt, cz };
    }
}

// ---- cuerpo: caballo (tronco, cuello, cabeza, crin), silla, jinete (sin el brazo de la espada)
static int rj_BuildBody(LineVertex *b, int cap) {
    int i = 0; rj_capCur = cap;
    // tronco: pecho y grupa redondos, lomo entre ambos
    rjL(b, i, 0, 2.85f, -1.7f, 0, 2.95f, 0.0f, 0.98f, 1.12f, 9, RJ_HORSE, RJ_HORSEL);
    rjL(b, i, 0, 2.95f, 0.0f, 0, 3.0f, 1.9f, 1.12f, 1.0f, 9, RJ_HORSEL, RJ_HORSE);
    rjB(b, i, 0, 2.9f, -1.75f, 1.0f, 1.12f, 1.0f, 5, 9, RJ_HORSE);
    rjB(b, i, 0, 3.0f, 2.0f, 1.02f, 1.1f, 1.0f, 5, 9, RJ_HORSE);
    for (int s = -1; s <= 1; s += 2) {                                  // hombros y ancas (musculo)
        rjB(b, i, s * 0.64f, 2.75f, -1.85f, 0.5f, 0.78f, 0.7f, 4, 7, RJ_HORSE);
        rjB(b, i, s * 0.7f, 3.0f, 2.0f, 0.6f, 0.86f, 0.8f, 4, 7, RJ_HORSE);
    }
    // peytral (peto del caballo) con boton dorado
    rjB(b, i, 0, 2.8f, -2.4f, 0.92f, 0.86f, 0.46f, 4, 8, RJ_IRON);
    rjB(b, i, 0, 2.95f, -2.78f, 0.26f, 0.26f, 0.12f, 3, 6, RJ_GOLD);
    // cuello y cabeza
    rjL(b, i, 0, 3.55f, -1.9f, 0, 5.05f, -2.95f, 0.72f, 0.42f, 8, RJ_HORSE, RJ_HORSEL);
    rjL(b, i, 0, 5.15f, -2.95f, 0, 4.5f, -4.35f, 0.44f, 0.27f, 8, RJ_HORSEL, RJ_HORSE);
    rjB(b, i, 0, 4.95f, -3.2f, 0.46f, 0.52f, 0.5f, 4, 8, RJ_HORSE);                // mejilla
    rjB(b, i, 0, 4.38f, -4.5f, 0.3f, 0.28f, 0.34f, 3, 7, RJ_HORSED);              // hocico
    rjL(b, i, 0, 4.75f, -3.6f, 0, 4.36f, -4.4f, 0.22f, 0.18f, 6, RJ_HORSED, RJ_HORSED);   // mandibula
    rjL(b, i, 0, 5.45f, -3.0f, 0, 4.78f, -4.4f, 0.11f, 0.09f, 6, RJ_GOLD, RJ_GOLD);       // testera dorada
    for (int s = -1; s <= 1; s += 2) {
        rjB(b, i, s * 0.4f, 5.02f, -3.45f, 0.15f, 0.14f, 0.22f, 3, 6, RJ_EYE);            // ojos que brillan
        rjB(b, i, s * 0.12f, 4.4f, -4.8f, 0.07f, 0.06f, 0.05f, 2, 4, RJ_HOOF);            // narices
        rjL(b, i, s * 0.2f, 5.45f, -2.85f, s * 0.3f, 6.1f, -2.7f, 0.14f, 0.03f, 5, RJ_HORSE, RJ_HORSED);   // orejas
    }
    // crin: llamas oscuras a lo largo del cuello, con la punta ardiente
    for (int k = 0; k < 10; ++k) {
        const float t = k / 9.0f, sd = (k & 1) ? 1.0f : -1.0f;
        const float bx = 0.0f, by = 4.25f + 1.2f * t, bz = -1.9f - 1.05f * t;
        rjL(b, i, bx, by, bz, bx + sd * 0.25f, by + 0.6f + 0.1f * sinf(k * 1.7f), bz + 0.6f, 0.27f, 0.03f, 5, RJ_MANE, RJ_MANET);
    }
    rjL(b, i, 0, 5.5f, -3.0f, 0, 5.75f, -3.5f, 0.16f, 0.02f, 5, RJ_MANE, RJ_MANET);        // flequillo
    // silla de montar
    rjB(b, i, 0, 4.02f, 0.2f, 0.88f, 0.22f, 1.08f, 3, 8, RJ_CLOTHD);
    rjB(b, i, 0, 3.9f, 0.2f, 0.96f, 0.12f, 1.16f, 3, 8, RJ_GOLD);
    // ---- jinete ----
    for (int s = -1; s <= 1; s += 2) {
        rjL(b, i, s * 0.5f, 4.2f, 0.2f, s * 1.18f, 3.55f, -0.3f, 0.32f, 0.24f, 6, RJ_IRON, RJ_IRON);   // muslo
        rjL(b, i, s * 1.18f, 3.55f, -0.3f, s * 1.3f, 2.55f, 0.05f, 0.24f, 0.19f, 6, RJ_IROND, RJ_IROND); // canilla
        rjB(b, i, s * 1.3f, 2.45f, -0.05f, 0.24f, 0.2f, 0.4f, 3, 6, RJ_HOOF);                          // bota
    }
    {   // torso: faldon rojo abajo, coraza arriba
        const float ys[5] = { 4.15f, 4.75f, 5.3f, 5.8f, 6.05f };
        const float rs[5] = { 0.64f, 0.72f, 0.9f, 0.92f, 0.52f };
        if (i + 4 * 8 * 12 <= cap) addLoft(b, i, 0.0f, 0.1f, ys, rs, 5, 8, RJ_CLOTH, RJ_IRON);
    }
    rjB(b, i, 0, 5.35f, -0.2f, 0.82f, 0.72f, 0.52f, 4, 8, RJ_IRONL);                  // peto
    rjB(b, i, 0, 5.0f, -0.62f, 0.2f, 0.2f, 0.1f, 3, 6, RJ_GOLD);                      // emblema
    rjB(b, i, -1.0f, 5.75f, 0.1f, 0.55f, 0.42f, 0.55f, 4, 8, RJ_IRON);                // hombrera izq.
    rjL(b, i, -1.1f, 5.95f, 0.1f, -1.3f, 6.6f, 0.05f, 0.2f, 0.02f, 5, RJ_GOLD, RJ_BONE);
    // brazo izquierdo con las riendas
    rjL(b, i, -1.0f, 5.65f, 0.15f, -1.1f, 4.95f, -0.45f, 0.3f, 0.25f, 6, RJ_IRON, RJ_IRON);
    rjL(b, i, -1.1f, 4.95f, -0.45f, -0.5f, 4.6f, -1.4f, 0.25f, 0.2f, 6, RJ_IRONL, RJ_IRON);
    rjB(b, i, -0.48f, 4.58f, -1.45f, 0.22f, 0.2f, 0.22f, 3, 6, RJ_CLOTHD);
    rjL(b, i, -0.5f, 4.6f, -1.4f, -0.3f, 4.46f, -4.3f, 0.05f, 0.05f, 4, RJ_GOLD, RJ_GOLD);
    // casco con cuernos, visera oscura, ojos rojos y cresta
    rjL(b, i, 0, 6.0f, 0.05f, 0, 5.8f, 0.1f, 0.5f, 0.72f, 8, RJ_IRON, RJ_IROND);       // gola
    rjB(b, i, 0, 6.55f, 0.05f, 0.52f, 0.6f, 0.58f, 5, 9, RJ_IRON);
    rjB(b, i, 0, 6.5f, -0.42f, 0.36f, 0.13f, 0.2f, 2, 6, RGBA(10, 4, 6, 255));
    for (int s = -1; s <= 1; s += 2) {
        rjB(b, i, s * 0.16f, 6.5f, -0.58f, 0.09f, 0.05f, 0.05f, 2, 4, RGBA(255, 84, 44, 255));
        rjL(b, i, s * 0.42f, 6.85f, -0.05f, s * 0.8f, 7.4f, -0.2f, 0.15f, 0.1f, 5, RJ_BONE, RJ_BONE);   // cuernos
        rjL(b, i, s * 0.8f, 7.4f, -0.2f, s * 1.0f, 8.0f, -0.5f, 0.1f, 0.01f, 5, RJ_BONE, RJ_BLOOD);
    }
    rjL(b, i, 0, 7.0f, 0.05f, 0, 7.55f, 0.6f, 0.13f, 0.1f, 5, RJ_CLOTH, RJ_BLOOD);      // cresta
    rjL(b, i, 0, 7.5f, 0.6f, 0, 7.35f, 1.5f, 0.1f, 0.01f, 5, RJ_BLOOD, RJ_CLOTHD);
    return i;
}

// ---- patas (origen en la articulacion de arriba, cuelgan hacia -Y)
static int rj_BuildLegs() {
    int i;
    i = 0; rj_capCur = 420;                                                         // antebrazo delantero
    rjB(rj_mFUp, i, 0, 0, 0, 0.42f, 0.42f, 0.4f, 3, 6, RJ_HORSE);
    rjL(rj_mFUp, i, 0, 0, 0, 0, -1.2f, 0, 0.38f, 0.24f, 7, RJ_HORSE, RJ_HORSE);
    rj_nFUp = i;
    i = 0; rj_capCur = 520;                                                         // rodilla -> casco
    rjB(rj_mFLo, i, 0, 0, 0, 0.25f, 0.22f, 0.25f, 3, 6, RJ_SOCK);
    rjL(rj_mFLo, i, 0, 0, 0, 0, -0.95f, 0, 0.2f, 0.15f, 6, RJ_SOCK, RJ_SOCK);
    rjB(rj_mFLo, i, 0, -0.95f, 0, 0.21f, 0.17f, 0.21f, 3, 6, RJ_SOCK);
    rjL(rj_mFLo, i, 0, -0.95f, 0, 0, -1.25f, 0.06f, 0.2f, 0.27f, 6, RJ_HOOF, RJ_HOOF);
    rj_nFLo = i;
    i = 0; rj_capCur = 520;                                                         // muslo trasero
    rjB(rj_mHUp, i, 0, -0.15f, 0.1f, 0.55f, 0.8f, 0.55f, 4, 7, RJ_HORSE);
    rjL(rj_mHUp, i, 0, 0, 0, 0, -1.15f, 0.35f, 0.5f, 0.24f, 7, RJ_HORSE, RJ_HORSE);
    rj_nHUp = i;
    i = 0; rj_capCur = 520;                                                         // corvejon -> casco
    rjB(rj_mHLo, i, 0, 0, 0, 0.24f, 0.22f, 0.26f, 3, 6, RJ_SOCK);
    rjL(rj_mHLo, i, 0, 0, 0, 0, -1.1f, -0.1f, 0.2f, 0.14f, 6, RJ_SOCK, RJ_SOCK);
    rjB(rj_mHLo, i, 0, -1.1f, -0.1f, 0.2f, 0.17f, 0.2f, 3, 6, RJ_SOCK);
    rjL(rj_mHLo, i, 0, -1.1f, -0.1f, 0, -1.4f, -0.04f, 0.2f, 0.27f, 6, RJ_HOOF, RJ_HOOF);
    rj_nHLo = i;
    return 0;
}
// ---- cola (origen en la base, cuelga hacia +Z/-Y)
static int rj_BuildTail(LineVertex *b, int cap) {
    int i = 0; rj_capCur = cap;
    rjL(b, i, 0, 0, 0, 0, -0.5f, 0.9f, 0.34f, 0.3f, 6, RJ_MANE, RJ_MANE);
    rjL(b, i, 0, -0.5f, 0.9f, 0, -1.6f, 1.4f, 0.3f, 0.2f, 6, RJ_MANE, RJ_MANET);
    rjL(b, i, 0, -1.6f, 1.4f, 0, -2.7f, 1.5f, 0.2f, 0.05f, 6, RJ_MANET, RJ_MANE);
    for (int s = -1; s <= 1; s += 2) {
        rjL(b, i, s * 0.1f, -0.4f, 0.8f, s * 0.55f, -1.5f, 1.5f, 0.2f, 0.05f, 5, RJ_MANE, RJ_MANET);
        rjL(b, i, s * 0.1f, -0.9f, 1.1f, s * 0.4f, -2.3f, 1.7f, 0.16f, 0.03f, 5, RJ_MANE, RJ_MANET);
    }
    return i;
}
// ---- capa desgarrada (origen en los hombros, cuelga en -Y)
static int rj_BuildCape(LineVertex *b, int cap) {
    int i = 0;
    const int N = 6;
    const unsigned int cT = RJ_CLOTH, cM = RGBA(116, 10, 18, 255), cB = RGBA(44, 2, 8, 255);
    for (int k = 0; k < N && i + 9 <= cap; ++k) {
        const float u0 = -1.0f + 2.0f * k / N, u1 = -1.0f + 2.0f * (k + 1) / N;
        const float w0 = sinf(k * 1.9f) * 0.18f, w1 = sinf((k + 1) * 1.9f) * 0.18f;
        const float yb = -3.6f - 0.4f * (float)((k * 5) % 3);
        swQuadC(b, i, u0 * 0.7f, 0, 0, cT, u1 * 0.7f, 0, 0, cT,
                      u1 * 1.05f, -1.9f, 0.45f + w1, cM, u0 * 1.05f, -1.9f, 0.45f + w0, cM);
        if (i + 3 > cap) break;
        b[i++] = { cM, u0 * 1.05f, -1.9f, 0.45f + w0 };
        b[i++] = { cM, u1 * 1.05f, -1.9f, 0.45f + w1 };
        b[i++] = { cB, (u0 + u1) * 0.55f, yb, 0.9f + w0 };
    }
    return i;
}
// ---- brazo derecho + GRAN ESPADA (origen en el hombro derecho)
static int rj_BuildArm(LineVertex *b, int cap) {
    int i = 0; rj_capCur = cap;
    rjB(b, i, 0.1f, 0.1f, 0, 0.56f, 0.44f, 0.5f, 4, 8, RJ_IRON);                        // hombrera
    rjL(b, i, 0.3f, 0.35f, 0, 0.5f, 1.0f, -0.05f, 0.2f, 0.02f, 5, RJ_GOLD, RJ_BONE);
    rjL(b, i, 0, 0, 0, 0.25f, -0.5f, -0.55f, 0.32f, 0.26f, 6, RJ_IRON, RJ_IRON);
    rjL(b, i, 0.25f, -0.5f, -0.55f, 0.3f, -0.1f, -1.15f, 0.27f, 0.22f, 6, RJ_IRONL, RJ_IRON);
    rjB(b, i, 0.3f, -0.08f, -1.2f, 0.26f, 0.24f, 0.26f, 3, 6, RJ_CLOTHD);                // mano
    rjL(b, i, 0.3f, -0.45f, -1.2f, 0.3f, 0.5f, -1.2f, 0.1f, 0.09f, 6, RJ_IROND, RJ_IROND); // empunadura
    rjB(b, i, 0.3f, -0.5f, -1.2f, 0.17f, 0.17f, 0.17f, 3, 6, RJ_GOLD);                   // pomo
    rjL(b, i, -0.55f, 0.55f, -1.2f, 1.15f, 0.55f, -1.2f, 0.11f, 0.11f, 6, RJ_GOLD, RJ_GOLD);   // guarda
    rjB(b, i, -0.55f, 0.55f, -1.2f, 0.16f, 0.16f, 0.16f, 3, 6, RJ_GOLD);
    rjB(b, i, 1.15f, 0.55f, -1.2f, 0.16f, 0.16f, 0.16f, 3, 6, RJ_GOLD);
    rjL(b, i, 0.3f, 0.5f, -1.2f, 0.3f, 0.75f, -1.2f, 0.2f, 0.16f, 6, RJ_IROND, RJ_IROND);  // talon de la hoja
    rjBlade(b, i, 0.3f, 0.7f, -1.2f, 7.2f, 0.46f, 0.12f, RJ_STEEL, RJ_BLOOD);
    return i;
}

static void rj_BuildModels() {
    if (rj_built) return;
    rj_nBody = rj_BuildBody(rj_mBody, 7600);
    rj_BuildLegs();
    rj_nTail = rj_BuildTail(rj_mTail, 1000);
    rj_nCape = rj_BuildCape(rj_mCape, 400);
    rj_nArm = rj_BuildArm(rj_mArm, 1400);
    sceKernelDcacheWritebackAll();
    rj_built = 1;
}

// ---------------------------------------------------------------- estado del jefe
enum { RJS_INTRO, RJS_PACE, RJS_ALIGN, RJS_ROAR, RJS_CHARGE, RJS_SKID, RJS_SCREAM, RJS_RECOVER,
       RJS_SLASH, RJS_DYING, RJS_DEAD };

static int   rj_s = RJS_INTRO, rj_st = 0, rj_t = 0;      // estado, frames en el estado, frames totales
static float rj_x = 0.0f, rj_z = -32.0f, rj_a = 0.0f;    // posicion y rumbo (dir = sin a, cos a)
static float rj_spd = 0.0f, rj_gait = 0.0f, rj_rear = 0.0f;
static float rj_dirX = 0.0f, rj_dirZ = 1.0f;             // direccion fijada de la embestida
static float rj_ax = 0.0f, rj_az = 0.0f;                 // punto de salida (borde opuesto)
static float rj_travel = 0.0f, rj_tBrake = 20.0f;
static float rj_ix = 0.0f, rj_iz = 0.0f;                 // punto de impacto del tajo
static float rj_sw = 0.35f, rj_swT = 0.35f;              // angulo del brazo de la espada (rotX)
static int   rj_hp = RJ_HPMAX, rj_hurt = 0, rj_done = 0, rj_dieT = 0;
static int   rj_paceT = 0, rj_slashCd = 0, rj_hitDone = 0, rj_enr = 0;
static float rj_orb = 1.0f;                              // sentido de la orbita
static float rj_pd = 30.0f;                              // distancia al jugador
static int   rj_stun = 0, rj_tick = 0, rj_tickFx = 0;
static BxMash rj_mash;
static float rj_shake = 0.0f;
static int   rj_tint = 0, rj_tintMax = 1;
static float rj_lastHp = 0.0f;
static int   rj_bgT = 0, rj_drawT = 0;
// pose actual (la guarda el dibujo para proyectar ojos en el HUD)
static float rj_pitchNow = 0.0f, rj_scNow = 1.0f, rj_yNow = 0.0f;

struct RjWave { float x, z, r; int on, hit; };
static RjWave rj_wv[3];
struct RjPart { float x, y, z, vx, vy, vz, gr, sz; int life, life0; unsigned int col; };
static RjPart rj_parts[64];
static unsigned int rj_seed = 2463534242u;
static float rj_rnd() { rj_seed = rj_seed * 1664525u + 1013904223u; return (float)((rj_seed >> 8) & 0xFFFF) / 65535.0f; }
static void rj_Spawn(float x, float y, float z, float vx, float vy, float vz, float gr, float sz, int life, unsigned int col) {
    for (int k = 0; k < 64; ++k) if (rj_parts[k].life <= 0) {
        rj_parts[k].x = x; rj_parts[k].y = y; rj_parts[k].z = z;
        rj_parts[k].vx = vx; rj_parts[k].vy = vy; rj_parts[k].vz = vz;
        rj_parts[k].gr = gr; rj_parts[k].sz = sz; rj_parts[k].life = life; rj_parts[k].life0 = life; rj_parts[k].col = col;
        return;
    }
}

// ---------------------------------------------------------------- arena
// borde organico: radio segun el angulo (angulo a: x = sin a, z = cos a)
static float rj_rad(float a) {
    return RJ_R + RJ_K * (1.5f * sinf(3.0f * a + 0.7f) + 0.9f * sinf(5.0f * a + 2.1f) + 0.5f * sinf(9.0f * a + 0.3f));
}
static bool rj_inside(float x, float z) { return sqrtf(x * x + z * z) < rj_rad(atan2f(x, z)); }

// color del suelo: carne rojiza o piedra, mas clara al centro, con variacion por baldosa
static unsigned int rj_floorCol(float x, float z, float var, int stone) {
    float r = sqrtf(x * x + z * z) / (RJ_R + 1.0f); if (r > 1.0f) r = 1.0f;
    const float k = 1.0f - r * r * 0.55f;
    const float n = 0.92f + 0.08f * sinf(x * 0.5f) * sinf(z * 0.45f + 1.3f);
    const float f = var * n * k;
    const float cr = stone ? 122.0f : 152.0f, cg = stone ? 90.0f : 44.0f, cb = stone ? 86.0f : 46.0f;
    int R = (int)(cr * f), G = (int)(cg * f), B = (int)(cb * f);
    if (R > 255) R = 255; if (G > 255) G = 255; if (B > 255) B = 255;
    return RGBA(R, G, B, 255);
}

struct RjStrip { float xl, xr, zc; };
static RjStrip rj_strips[RJ_STR_N];
static int rj_nStrips = 0;
static const int RJ_NV = 12, RJ_NP = 15;                   // venas del suelo: cuantas y puntos por vena
static float rj_vx[12][15], rj_vz[12][15];

// anillo plano en la geometria estatica
static void rj_RingGeo(LineVertex *g, int &i, int cap, float R, float w, float y, unsigned int col) {
    const int N = 32;
    if (i + N * 6 > cap) return;
    const float r0 = R - w * 0.5f, r1 = R + w * 0.5f;
    for (int s = 0; s < N; ++s) {
        const float a0 = RJ_TAU * s / N, a1 = RJ_TAU * (s + 1) / N;
        swQuadC(g, i, sinf(a0) * r0, y, cosf(a0) * r0, col, sinf(a0) * r1, y, cosf(a0) * r1, col,
                      sinf(a1) * r1, y, cosf(a1) * r1, col, sinf(a1) * r0, y, cosf(a1) * r0, col);
    }
}
// barra plana entre dos puntos del suelo (para el sigilo)
static void rj_BarGeo(LineVertex *g, int &i, int cap, float x0, float z0, float x1, float z1, float w, float y, unsigned int col) {
    if (i + 6 > cap) return;
    const float dx = x1 - x0, dz = z1 - z0, l = sqrtf(dx * dx + dz * dz) + 0.0001f;
    const float nx = -dz / l * w * 0.5f, nz = dx / l * w * 0.5f;
    swQuadC(g, i, x0 - nx, y, z0 - nz, col, x0 + nx, y, z0 + nz, col, x1 + nx, y, z1 + nz, col, x1 - nx, y, z1 - nz, col);
}

static void br_Enter(BossIO &io, LineVertex *geo, int geoCap, int *geoN, CityBldg *boxes, int *nBoxes, int boxCap,
                     float *sx, float *sy, float *sz) {
    rj_BuildModels();
    // ---- reinicio de estado ----
    rj_s = RJS_INTRO; rj_st = 0; rj_t = 0;
    rj_x = 0.0f; rj_z = -32.0f; rj_a = 0.0f; rj_spd = 0.0f; rj_gait = 0.0f; rj_rear = 0.0f;
    rj_dirX = 0.0f; rj_dirZ = 1.0f; rj_travel = 0.0f; rj_tBrake = 20.0f;
    rj_sw = rj_swT = 0.35f;
    rj_hp = RJ_HPMAX; rj_hurt = 0; rj_done = 0; rj_dieT = 0;
    rj_paceT = 150; rj_slashCd = 90; rj_hitDone = 0; rj_enr = 0; rj_orb = 1.0f; rj_pd = 30.0f;
    rj_stun = 0; rj_tick = 0; rj_tickFx = 0; bxMashStart(rj_mash, 0); rj_mash.frames = 0;
    rj_shake = 0.0f; rj_tint = 0; rj_tintMax = 1; rj_lastHp = io.hp;
    rj_pitchNow = 0.0f; rj_scNow = RJ_S; rj_yNow = 0.0f;
    for (int w = 0; w < 3; ++w) rj_wv[w].on = 0;
    for (int k = 0; k < 64; ++k) rj_parts[k].life = 0;
    io.camYaw = 0.0f;

    // ---- suelo: franjas-caja que siguen el borde organico (visual = colision) ----
    if (geoCap > 8500) geoCap = 8500;                     // presupuesto de vertices de la arena
    int i = 0, nb = 0;
    rj_nStrips = 0;
    const float SZ = 4.5f;                                // profundidad de cada franja
    for (int sIdx = -11; sIdx < 11 && rj_nStrips < RJ_STR_N; ++sIdx) {
        const float zc = ((float)sIdx + 0.5f) * SZ;
        if (!rj_inside(0.0f, zc)) continue;
        float xr = 0.0f, xl = 0.0f;
        while (xr < 70.0f && rj_inside(xr + 0.5f, zc)) xr += 0.5f;
        while (xl > -70.0f && rj_inside(xl - 0.5f, zc)) xl -= 0.5f;
        if (xr - xl < 4.0f) continue;
        rj_strips[rj_nStrips].xl = xl; rj_strips[rj_nStrips].xr = xr; rj_strips[rj_nStrips].zc = zc;
        ++rj_nStrips;
        if (boxes && nb < boxCap) { boxes[nb].x = (xl + xr) * 0.5f; boxes[nb].z = zc; boxes[nb].w = xr - xl; boxes[nb].d = SZ; boxes[nb].h = 0.0f; boxes[nb].color = RGBA(140, 40, 44, 255); ++nb; }
    }
    if (nBoxes) *nBoxes = nb;
    // lados de las franjas (carne que se oscurece hacia abajo) + junta oscura
    for (int s = 0; s < rj_nStrips; ++s) {
        const RjStrip &st = rj_strips[s];
        if (i + 36 > geoCap) break;
        swBoxLit(geo, i, (st.xl + st.xr) * 0.5f, -14.0f, st.zc, st.xr - st.xl, SZ, 13.88f, RGBA(122, 30, 34, 255), RGBA(20, 2, 8, 255), false);
        const unsigned int cg = RGBA(58, 8, 14, 255);
        swQuadC(geo, i, st.xl, -0.12f, st.zc - SZ * 0.5f, cg, st.xl, -0.12f, st.zc + SZ * 0.5f, cg,
                        st.xr, -0.12f, st.zc + SZ * 0.5f, cg, st.xr, -0.12f, st.zc - SZ * 0.5f, cg);
    }
    // baldosas de la cara superior: variacion de tono por baldosa y degradado radial por vertice
    for (int s = 0; s < rj_nStrips; ++s) {
        const RjStrip &st = rj_strips[s];
        int n = (int)((st.xr - st.xl) / 4.0f + 0.5f); if (n < 1) n = 1;
        const float cw = (st.xr - st.xl) / n, g = 0.1f;
        for (int c = 0; c < n; ++c) {
            if (i + 6 > geoCap) break;
            const float x0 = st.xl + c * cw + g, x1 = st.xl + (c + 1) * cw - g;
            const float z0 = st.zc - SZ * 0.5f + g, z1 = st.zc + SZ * 0.5f - g;
            const unsigned int h = (unsigned int)(s * 31 + c * 17 + 7) * 2654435761u;
            const float var = 0.84f + 0.04f * (float)((h >> 16) & 7);
            const int stone = (((h >> 20) % 5u) == 0u);
            swQuadC(geo, i, x0, 0.0f, z0, rj_floorCol(x0, z0, var, stone), x0, 0.0f, z1, rj_floorCol(x0, z1, var, stone),
                            x1, 0.0f, z1, rj_floorCol(x1, z1, var, stone), x1, 0.0f, z0, rj_floorCol(x1, z0, var, stone));
        }
    }
    // sigilo de guerra en el centro: tres anillos y una cruz en aspa
    rj_RingGeo(geo, i, geoCap, 10.0f, 0.8f, 0.09f, RGBA(176, 110, 40, 255));
    rj_RingGeo(geo, i, geoCap, 16.0f, 0.5f, 0.09f, RGBA(60, 6, 12, 255));
    rj_RingGeo(geo, i, geoCap, 27.0f, 0.7f, 0.09f, RGBA(120, 70, 34, 255));
    rj_BarGeo(geo, i, geoCap, -14.0f, -14.0f, 14.0f, 14.0f, 0.7f, 0.09f, RGBA(60, 6, 12, 255));
    rj_BarGeo(geo, i, geoCap, -14.0f, 14.0f, 14.0f, -14.0f, 0.7f, 0.09f, RGBA(60, 6, 12, 255));
    // costillas de hueso en el borde (curvadas hacia afuera)
    rj_capCur = geoCap;
    for (int k = 0; k < 22; ++k) {
        const float a = RJ_TAU * k / 22.0f + 0.15f * sinf(k * 2.3f);
        const float rr = rj_rad(a) + 1.0f, ox = sinf(a), oz = cosf(a);
        const float h = 4.0f + 3.2f * (float)((k * 7) % 5) / 4.0f;
        rjL(geo, i, ox * rr, -2.0f, oz * rr, ox * (rr + 2.6f), h, oz * (rr + 2.6f), 1.0f, 0.08f, 5, RGBA(120, 70, 62, 255), RJ_BONE);
    }
    // tendones colgando bajo la plataforma
    for (int k = 0; k < 8; ++k) {
        const float a = RJ_TAU * k / 8.0f + 0.4f;
        const float rr = rj_rad(a) * 0.92f, ox = sinf(a), oz = cosf(a), wig = 2.0f + 1.0f * sinf(k * 2.1f);
        rjL(geo, i, ox * rr, -3.0f, oz * rr, ox * rr * 0.95f + wig, -12.0f, oz * rr * 0.95f, 0.9f, 0.62f, 6, RGBA(170, 30, 34, 255), RGBA(120, 14, 22, 255));
        rjL(geo, i, ox * rr * 0.95f + wig, -12.0f, oz * rr * 0.95f, ox * rr * 0.9f - wig, -24.0f, oz * rr * 0.9f, 0.62f, 0.08f, 6, RGBA(120, 14, 22, 255), RGBA(56, 4, 12, 255));
    }
    // columnas de carne lejanas (profundidad) + rocas flotantes
    for (int k = 0; k < 8; ++k) {
        const float a = RJ_TAU * k / 8.0f + 0.3f * sinf(k * 1.9f);
        const float rr = 85.0f + (float)((k * 13) % 50), cx = sinf(a) * rr, cz = cosf(a) * rr;
        const float top = 40.0f + (float)((k * 11) % 50);
        rjL(geo, i, cx, -70.0f, cz, cx, top, cz, 8.0f, 4.2f, 7, RGBA(70, 8, 16, 255), RGBA(150, 32, 36, 255));
        rjB(geo, i, cx, top + 2.5f, cz, 5.8f, 5.2f, 5.8f, 3, 6, RGBA(176, 40, 40, 255));
    }
    for (int k = 0; k < 5; ++k) {
        const float a = RJ_TAU * k / 5.0f + 0.9f, rr = 62.0f + (float)((k * 17) % 34);
        rjB(geo, i, sinf(a) * rr, 12.0f + (float)((k * 9) % 28), cosf(a) * rr, 5.0f + 1.6f * (float)(k % 3), 4.0f + 1.5f * (float)(k % 2), 5.0f + 1.6f * (float)(k % 3), 4, 8, RGBA(120, 24, 28, 255));
    }
    if (geoN) *geoN = i;

    // ---- venas del suelo (se dibujan en rj_Draw pulsando): del centro al borde ----
    for (int k = 0; k < RJ_NV; ++k) {
        float th = RJ_TAU * k / RJ_NV + 0.25f * sinf(k * 1.7f);
        for (int j = 0; j < RJ_NP; ++j) {
            th += sinf(j * 0.8f + k * 1.9f) * 0.14f;
            float r = 4.0f + j * 2.9f, lim = rj_rad(th) - 1.5f;
            if (r > lim) r = lim;
            rj_vx[k][j] = sinf(th) * r; rj_vz[k][j] = cosf(th) * r;
        }
    }
    sceKernelDcacheWritebackAll();
    // jugador: de un lado de la arena, mirando al jinete (camara yaw 0 mira a -Z)
    *sx = 0.0f; *sy = 0.0f; *sz = 32.0f;
}

// ---------------------------------------------------------------- utilidades de IA
static float rj_angDiff(float from, float to) {
    float d = to - from;
    while (d > RJ_PI) d -= RJ_TAU;
    while (d < -RJ_PI) d += RJ_TAU;
    return d;
}
static void rj_turn(float target, float rate) {
    float d = rj_angDiff(rj_a, target);
    if (d > rate) d = rate;
    if (d < -rate) d = -rate;
    rj_a += d;
    while (rj_a > RJ_PI) rj_a -= RJ_TAU;
    while (rj_a < -RJ_PI) rj_a += RJ_TAU;
}
static void rj_Step(float dx, float dz, float spd) {
    rj_x += dx * spd; rj_z += dz * spd;
    rj_gait += spd * 0.2f + (spd > 0.05f ? 0.02f : 0.0f);
}
// distancia desde (x,z) en la direccion (dx,dz) hasta el circulo de radio R (raiz mayor)
static float rj_ChordEnd(float x, float z, float dx, float dz, float R) {
    const float b = x * dx + z * dz, c = x * x + z * z - R * R, disc = b * b - c;
    if (disc < 0.0f) return 40.0f;
    return -b + sqrtf(disc);
}
static void rj_Dust(float amount) {
    const float fx = sinf(rj_a), fz = cosf(rj_a);
    for (int s = -1; s <= 1; s += 2) {
        if (rj_rnd() > amount) continue;
        rj_Spawn(rj_x - fx * 2.5f + fz * s * 1.2f, 0.3f, rj_z - fz * 2.5f - fx * s * 1.2f,
                 (rj_rnd() - 0.5f) * 0.1f - fx * 0.05f, 0.05f + rj_rnd() * 0.07f, (rj_rnd() - 0.5f) * 0.1f - fz * 0.05f,
                 0.002f, 0.5f + rj_rnd() * 0.5f, 22 + (int)(rj_rnd() * 14.0f), rj_rnd() > 0.5f ? RGBA(150, 40, 30, 255) : RGBA(86, 22, 24, 255));
    }
}
static void rj_BeginDeath() {
    if (rj_s >= RJS_DYING) return;
    rj_s = RJS_DYING; rj_st = 0; rj_dieT = 0; rj_spd = 0.0f;
    rj_stun = 0; rj_mash.frames = 0; rj_tint = 0;
    for (int w = 0; w < 3; ++w) rj_wv[w].on = 0;
    rj_shake = 0.5f;
    lvMessage("EL JINETE ROJO CAE", 120);
}
// golpe de la embestida contra el jugador (una sola vez por embestida)
static void rj_ChargeHit(BossIO &io) {
    if (rj_hitDone) return;
    const float rx = io.px - rj_x, rz = io.pz - rj_z;
    const float along = rx * rj_dirX + rz * rj_dirZ, side = rx * rj_dirZ - rz * rj_dirX;
    if (fabsf(side) < 2.8f && along > -5.5f && along < 6.5f && io.py < 4.0f) {   // saltando lo esquivas
        rj_hitDone = 1;
        io.hp -= 35.0f;
        io.vy = 6.0f;
        io.px += (side >= 0.0f ? 1.0f : -1.0f) * rj_dirZ * 2.5f;
        io.pz -= (side >= 0.0f ? 1.0f : -1.0f) * rj_dirX * 2.5f;
        rj_tint = 40; rj_tintMax = 60; rj_shake = 0.6f; g_lvFlash = 8;
    }
}

// ---------------------------------------------------------------- logica por frame
static void br_Update(BossIO &io) {
    // el jugador murio y revivio: se suelta el aturdimiento y el jefe se reacomoda
    if (io.hp - rj_lastHp > io.hpMax * 0.5f) {
        rj_stun = 0; rj_mash.frames = 0;
        for (int w = 0; w < 3; ++w) rj_wv[w].on = 0;
        if (rj_s == RJS_ROAR || rj_s == RJS_CHARGE || rj_s == RJS_SKID || rj_s == RJS_SCREAM ||
            rj_s == RJS_SLASH || rj_s == RJS_ALIGN) { rj_s = RJS_RECOVER; rj_st = 0; }
    }
    { const float dx = io.px - rj_x, dz = io.pz - rj_z; rj_pd = sqrtf(dx * dx + dz * dz); }
    if (rj_shake > 0.0f) { rj_shake *= 0.955f; if (rj_shake < 0.01f) rj_shake = 0.0f; }
    if (rj_tint > 0) --rj_tint;
    if (rj_tickFx > 0) --rj_tickFx;
    if (rj_hurt > 0) --rj_hurt;

    if (g_lvTimeStop > 0) {            // tiempo detenido: solo el jugador puede soltarse
        if (rj_stun && bxMashUpdate(rj_mash, io.stickX, io.stickY, RJ_MASHNEED) != 0) rj_stun = 0;
        rj_lastHp = io.hp;
        return;
    }
    ++rj_t; ++rj_st;

    // ---- aturdimiento del grito: 3 ticks por segundo hasta mover el analogo ----
    if (rj_stun) {
        if (++rj_tick % 20 == 0) { io.hp -= 2.0f; rj_tickFx = 10; }
        const int r = bxMashUpdate(rj_mash, io.stickX, io.stickY, RJ_MASHNEED);
        if (r != 0) { rj_stun = 0; lvMessage(r > 0 ? "TE LIBERASTE" : "EL GRITO CEDE", 70); }
    }
    rj_enr = (rj_hp * 100 < RJ_HPMAX * 40) ? 1 : 0;      // bajo el 40%: mas rapido

    // ---- particulas (ceniza y polvo) ----
    for (int k = 0; k < 64; ++k) {
        RjPart &p = rj_parts[k]; if (p.life <= 0) continue;
        p.x += p.vx; p.y += p.vy; p.z += p.vz; p.vy -= p.gr; --p.life;
        if (p.y < 0.05f && p.gr > 0.0f) { p.y = 0.05f; p.vy = 0.0f; p.vx *= 0.8f; p.vz *= 0.8f; }
    }
    // ---- ondas del tajo ----
    for (int w = 0; w < 3; ++w) {
        RjWave &wv = rj_wv[w]; if (!wv.on) continue;
        wv.r += 0.6f;
        if (wv.r > 30.0f) { wv.on = 0; continue; }
        if (!wv.hit) {
            const float d = sqrtf((io.px - wv.x) * (io.px - wv.x) + (io.pz - wv.z) * (io.pz - wv.z));
            if (fabsf(d - wv.r) < 1.3f && io.py < 1.0f) {      // saltando la onda no te toca
                wv.hit = 1; io.hp -= 12.0f; rj_tint = 14; rj_tintMax = 40; g_lvFlash = 5;
            }
        }
    }

    float swRate = 0.10f;
    switch (rj_s) {
    case RJS_INTRO: {
        rj_turn(atan2f(io.px - rj_x, io.pz - rj_z), 0.05f);
        rj_rear += (((rj_st > 15 && rj_st < 100) ? 1.0f : 0.0f) - rj_rear) * 0.08f;
        rj_swT = 0.8f;
        if (rj_st == 10) lvMessage("EL SEGUNDO JINETE: GUERRA", 200);
        if (rj_st == 36) rj_shake = 0.30f;
        if (rj_st == 62) rj_shake = 0.20f;
        if (rj_st >= 150) { rj_s = RJS_PACE; rj_st = 0; }
        break;
    }
    case RJS_PACE: {
        rj_rear += (0.0f - rj_rear) * 0.1f;
        rj_swT = 0.35f;
        const float ang = atan2f(rj_x - io.px, rj_z - io.pz) + rj_orb * 0.55f;   // orbita alrededor tuyo
        float tx = io.px + sinf(ang) * 20.0f, tz = io.pz + cosf(ang) * 20.0f;
        const float tr = sqrtf(tx * tx + tz * tz);
        if (tr > 32.0f) { tx *= 32.0f / tr; tz *= 32.0f / tr; }
        const float mx = tx - rj_x, mz = tz - rj_z, md = sqrtf(mx * mx + mz * mz) + 0.001f;
        rj_turn(atan2f(mx, mz), 0.07f);
        rj_spd += ((md > 1.0f ? 0.62f : 0.0f) - rj_spd) * 0.08f;
        rj_Step(sinf(rj_a), cosf(rj_a), rj_spd);
        { const float rr = sqrtf(rj_x * rj_x + rj_z * rj_z); if (rr > 34.0f) { rj_x *= 34.0f / rr; rj_z *= 34.0f / rr; } }
        if (--rj_paceT <= 0) { rj_s = RJS_ALIGN; rj_st = 0; }
        else if (rj_slashCd > 0) --rj_slashCd;
        else if (rj_pd < 25.0f && rj_paceT > 30) { rj_s = RJS_SLASH; rj_st = 0; }
        break;
    }
    case RJS_ALIGN: {
        if (rj_st == 1) {                                  // borde opuesto a donde estas
            float pa = atan2f(io.px, io.pz);
            if (io.px * io.px + io.pz * io.pz < 64.0f) pa = rj_rnd() * RJ_TAU;
            rj_ax = -sinf(pa) * 32.0f; rj_az = -cosf(pa) * 32.0f;
        }
        rj_swT = 0.2f;
        const float mx = rj_ax - rj_x, mz = rj_az - rj_z, md = sqrtf(mx * mx + mz * mz) + 0.001f;
        rj_turn(atan2f(mx, mz), 0.12f);
        float want = 0.25f + md * 0.10f; if (want > 1.1f) want = 1.1f;
        rj_spd += (want - rj_spd) * 0.12f;
        rj_Step(sinf(rj_a), cosf(rj_a), rj_spd);
        if (md < 3.0f || rj_st > 260) { rj_s = RJS_ROAR; rj_st = 0; }
        break;
    }
    case RJS_ROAR: {
        rj_spd *= 0.85f;
        rj_rear += (((rj_st > 4 && rj_st < 50) ? 1.0f : 0.0f) - rj_rear) * 0.12f;
        rj_swT = -1.1f;
        if (rj_st < 52) {                                   // la linea te sigue...
            const float dx = io.px - rj_x, dz = io.pz - rj_z, d = sqrtf(dx * dx + dz * dz) + 0.001f;
            rj_dirX = dx / d; rj_dirZ = dz / d;
            rj_turn(atan2f(dx, dz), 0.2f);
        } else rj_turn(atan2f(rj_dirX, rj_dirZ), 0.3f);     // ...y se FIJA: ahora esquiva
        if (rj_st == 6) { lvMessage("¡EL JINETE BRAMA!", 80); rj_shake = 0.35f; }
        if (rj_st == 52) rj_shake = 0.2f;
        if (rj_st >= 82) {
            rj_a = atan2f(rj_dirX, rj_dirZ);
            rj_travel = 0.0f; rj_hitDone = 0; rj_spd = 0.5f;
            const float tEnd = rj_ChordEnd(rj_x, rj_z, rj_dirX, rj_dirZ, 32.0f);
            const float vm = rj_enr ? RJ_VMAXE : RJ_VMAX;
            rj_tBrake = tEnd - vm * vm / (2.0f * RJ_DECEL) - 0.5f; if (rj_tBrake < 12.0f) rj_tBrake = 12.0f;
            rj_s = RJS_CHARGE; rj_st = 0; rj_rear = 0.0f;
        }
        break;
    }
    case RJS_CHARGE: {
        rj_swT = -1.45f;
        rj_rear += (0.0f - rj_rear) * 0.2f;
        const float vmax = rj_enr ? RJ_VMAXE : RJ_VMAX;
        rj_spd += 0.08f; if (rj_spd > vmax) rj_spd = vmax;
        rj_a = atan2f(rj_dirX, rj_dirZ);
        rj_Step(rj_dirX, rj_dirZ, rj_spd);
        rj_travel += rj_spd;
        rj_ChargeHit(io);
        rj_Dust(0.6f);
        if (rj_travel >= rj_tBrake) { rj_s = RJS_SKID; rj_st = 0; }
        break;
    }
    case RJS_SKID: {
        rj_swT = -1.0f;
        rj_spd -= RJ_DECEL; if (rj_spd < 0.0f) rj_spd = 0.0f;
        rj_Step(rj_dirX, rj_dirZ, rj_spd);
        rj_ChargeHit(io);
        rj_Dust(1.0f);
        if (rj_spd <= 0.0f) {
            if (!rj_hitDone) {                               // FALLO: grita y te aturde
                rj_s = RJS_SCREAM; rj_st = 0;
                rj_shake = 0.85f; rj_tint = 150; rj_tintMax = 150;
                rj_stun = 1; rj_tick = 0; bxMashStart(rj_mash, RJ_MASHFRAMES);
                lvMessage("¡EL JINETE GRITA!", 90);
            } else { rj_s = RJS_RECOVER; rj_st = 0; }
        }
        break;
    }
    case RJS_SCREAM: {
        rj_spd = 0.0f;
        rj_rear += (((rj_st < 60) ? 1.0f : 0.0f) - rj_rear) * 0.15f;
        rj_swT = 0.1f;
        if (rj_st == 14 || rj_st == 28 || rj_st == 42) rj_shake += 0.3f;
        if (rj_st >= 70) { rj_s = RJS_RECOVER; rj_st = 0; }
        break;
    }
    case RJS_RECOVER: {
        rj_spd *= 0.8f;
        rj_rear += (0.0f - rj_rear) * 0.1f;
        rj_swT = 0.4f;
        rj_turn(atan2f(io.px - rj_x, io.pz - rj_z), 0.04f);
        if (rj_st >= (rj_hitDone ? 55 : 30) && !rj_stun) {
            rj_s = RJS_PACE; rj_st = 0;
            rj_paceT = rj_enr ? 130 : 190; rj_slashCd = 110; rj_orb = -rj_orb;
        }
        break;
    }
    case RJS_SLASH: {                                        // TAJO: levanta la espada y golpea el piso
        rj_spd *= 0.85f;
        rj_rear += (0.0f - rj_rear) * 0.1f;
        if (rj_st < 38) {
            rj_turn(atan2f(io.px - rj_x, io.pz - rj_z), 0.12f);
            rj_swT = 1.0f;
            rj_ix = rj_x + sinf(rj_a) * 11.0f; rj_iz = rj_z + cosf(rj_a) * 11.0f;
            { const float rr = sqrtf(rj_ix * rj_ix + rj_iz * rj_iz); if (rr > 38.0f) { rj_ix *= 38.0f / rr; rj_iz *= 38.0f / rr; } }
        } else if (rj_st < 70) { rj_swT = -2.2f; swRate = 0.40f; }
        else rj_swT = 0.35f;
        if (rj_st == 2) lvMessage("¡SALTA LA ONDA!", 70);
        if (rj_st == 48 || rj_st == 66) {                     // dos ondas: el golpe y su eco
            for (int w = 0; w < 3; ++w) if (!rj_wv[w].on) {
                rj_wv[w].on = 1; rj_wv[w].hit = 0; rj_wv[w].r = 1.5f; rj_wv[w].x = rj_ix; rj_wv[w].z = rj_iz; break;
            }
            if (rj_st == 48) {
                rj_shake = 0.3f;
                for (int q = 0; q < 8; ++q)
                    rj_Spawn(rj_ix, 0.4f, rj_iz, (rj_rnd() - 0.5f) * 0.4f, 0.1f + rj_rnd() * 0.12f, (rj_rnd() - 0.5f) * 0.4f,
                             0.01f, 0.5f + rj_rnd() * 0.6f, 24 + (int)(rj_rnd() * 12.0f), RGBA(220, 60, 30, 255));
            }
        }
        if (rj_st >= 100) { rj_s = RJS_PACE; rj_st = 0; rj_slashCd = rj_enr ? 170 : 220; }
        break;
    }
    case RJS_DYING: {
        ++rj_dieT;
        rj_spd = 0.0f; rj_rear *= 0.9f; rj_swT = 1.6f;
        if (rj_dieT >= 20 && rj_dieT < 150) {
            for (int n = 0; n < 2; ++n) {                     // el cuerpo se deshace en ceniza roja
                const float al = (rj_rnd() - 0.5f) * 10.0f, sd = (rj_rnd() - 0.5f) * 3.0f;
                const float fx = sinf(rj_a), fz = cosf(rj_a);
                const unsigned int cs[4] = { RGBA(150, 14, 16, 255), RGBA(230, 70, 24, 255), RGBA(92, 34, 38, 255), RGBA(40, 8, 12, 255) };
                rj_Spawn(rj_x + fx * al + fz * sd, 0.6f + rj_rnd() * 4.0f, rj_z + fz * al - fx * sd,
                         (rj_rnd() - 0.5f) * 0.12f, 0.03f + rj_rnd() * 0.07f, (rj_rnd() - 0.5f) * 0.12f,
                         0.0006f, 0.5f + rj_rnd() * 0.7f, 70 + (int)(rj_rnd() * 50.0f), cs[(int)(rj_rnd() * 3.99f)]);
            }
        }
        if (rj_dieT == 40) {
            rj_shake = 0.4f;
            for (int q = 0; q < 14; ++q)
                rj_Spawn(rj_x, 3.0f + rj_rnd() * 4.0f, rj_z, (rj_rnd() - 0.5f) * 0.35f, 0.05f + rj_rnd() * 0.1f, (rj_rnd() - 0.5f) * 0.35f,
                         0.002f, 0.7f + rj_rnd() * 0.7f, 60 + (int)(rj_rnd() * 40.0f), RGBA(210, 40, 24, 255));
        }
        if (rj_dieT >= 150) { rj_done = 1; rj_s = RJS_DEAD; rj_st = 0; }
        break;
    }
    default: break;
    }
    rj_sw += (rj_swT - rj_sw) * swRate;
    rj_lastHp = io.hp;
}

// ---------------------------------------------------------------- blancos
static int br_AimTarget(float out[3]) {
    if (rj_s >= RJS_DYING) return 0;
    out[0] = rj_x + sinf(rj_a) * rj_spd * 6.0f;               // un poco por delante: va corriendo
    out[1] = 6.8f;
    out[2] = rj_z + cosf(rj_a) * rj_spd * 6.0f;
    return 1;
}
static bool br_ShotHit(float x, float y, float z, int dmg) {
    if (rj_s >= RJS_DYING) return false;
    if (y < -1.0f || y > 14.5f) return false;
    const float rx = x - rj_x, rz = z - rj_z, fx = sinf(rj_a), fz = cosf(rj_a);
    const float along = rx * fx + rz * fz, side = rx * fz - rz * fx;   // generoso: caballo + jinete
    if (fabsf(side) > 3.4f || along < -6.5f || along > 7.5f) return false;
    rj_hp -= (dmg < 1 ? 1 : dmg);
    rj_hurt = 6;
    if (rj_hp <= 0) { rj_hp = 0; rj_BeginDeath(); }
    return true;
}
static int   br_Blocks() { return rj_stun ? 1 : 0; }
static int   br_Done() { return rj_done; }
static float br_Shake() {
    float a = rj_shake;
    if (rj_s == RJS_CHARGE || rj_s == RJS_SKID) { float c = 1.0f - rj_pd / 60.0f; if (c < 0.0f) c = 0.0f; a += 0.18f * c; }
    if (rj_s == RJS_ROAR) a += 0.08f;
    if (rj_stun) a += 0.10f;
    if (rj_s == RJS_DYING) a += 0.15f;
    return a;
}
static unsigned int br_Fog(float *nearF, float *farF) {
    *nearF = 33.0f; *farF = 225.0f;
    return RGBA(122, 12, 20, 255);
}

// ---------------------------------------------------------------- cielo carnoso (2D)
// tramo de vena 2D: cinta con color por extremo (alfa incluido)
static void rj_Seg2D(float x0, float y0, float x1, float y1, float w, unsigned int c0, unsigned int c1) {
    const float dx = x1 - x0, dy = y1 - y0, l = sqrtf(dx * dx + dy * dy) + 0.0001f;
    const float nx = -dy / l * w * 0.5f, ny = dx / l * w * 0.5f;
    GradVertex *v = (GradVertex *)sceGuGetMemory(sizeof(GradVertex) * 6);
    v[0] = { c0, (short)(x0 - nx), (short)(y0 - ny), 0 };
    v[1] = { c0, (short)(x0 + nx), (short)(y0 + ny), 0 };
    v[2] = { c1, (short)(x1 + nx), (short)(y1 + ny), 0 };
    v[3] = { c0, (short)(x0 - nx), (short)(y0 - ny), 0 };
    v[4] = { c1, (short)(x1 + nx), (short)(y1 + ny), 0 };
    v[5] = { c1, (short)(x1 - nx), (short)(y1 - ny), 0 };
    sceGuDrawArray(GU_TRIANGLES, GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 6, 0, v);
}
static void br_Backdrop() {
    ++rj_bgT;
    const float t = (float)rj_bgT;
    sceGuDisable(GU_TEXTURE_2D);
    gradQuad(0, 96, RGBA(34, 2, 10, 255), RGBA(112, 8, 16, 255));
    gradQuad(96, 150, RGBA(112, 8, 16, 255), RGBA(178, 28, 26, 255));      // resplandor del horizonte
    gradQuad(150, SCR_HEIGHT, RGBA(178, 28, 26, 255), RGBA(58, 4, 10, 255));
    sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    // corazon del cielo: late en dos golpes
    {
        float b1 = sinf(t * 0.09f); b1 = b1 > 0.0f ? b1 * b1 * b1 : 0.0f;
        float b2 = sinf(t * 0.09f - 0.9f); b2 = b2 > 0.0f ? b2 * b2 * b2 : 0.0f;
        const float p = b1 + 0.6f * b2;
        lvFan2D(240, 118, 150 + (int)(34.0f * p), RGBA(255, 70, 50, 50 + (int)(70.0f * p)), RGBA(200, 20, 20, 0), 24);
    }
    // manchas de carne: claras y oscuras, a ritmos distintos
    for (int k = 0; k < 10; ++k) {
        const int cx = (int)((k * 97 + 40) % 520) - 20 + (int)(sinf(t * 0.01f + k) * 14.0f);
        const int cy = (int)((k * 53) % 220) + 20 + (int)(cosf(t * 0.013f + k * 1.7f) * 10.0f);
        const int r = 70 + (k * 37) % 80;
        const float pu = 0.5f + 0.5f * sinf(t * 0.05f + k * 1.3f);
        const int a = 36 + (int)(70.0f * pu);
        if (k & 1) lvFan2D(cx, cy, r, RGBA(42, 0, 6, a + 30), RGBA(60, 2, 10, 0), 14);
        else       lvFan2D(cx, cy, r, RGBA(214, 44, 44, a), RGBA(150, 10, 20, 0), 14);
    }
    // venas: tronco con una rama, tramos continuos; una onda de luz las recorre (laten)
    for (int k = 0; k < 6; ++k) {
        const float x0 = 30.0f + k * 78.0f;
        float tx[14], ty[14], bx[7], by[7], pu[14];
        for (int j = 0; j < 14; ++j) {
            ty[j] = j * 19.0f - 10.0f;
            tx[j] = x0 + sinf(j * 0.55f + k * 2.1f) * (18.0f + j) + j * (float)((k % 3) - 1) * 4.0f;
            const float p = sinf(j * 0.6f - t * 0.1f + k);
            pu[j] = p > 0.0f ? p : 0.0f;
        }
        bx[0] = tx[6]; by[0] = ty[6];
        for (int j = 1; j < 7; ++j) {
            bx[j] = bx[j - 1] + ((k & 1) ? 1.0f : -1.0f) * (10.0f + 2.0f * sinf((float)j + k));
            by[j] = by[j - 1] + 14.0f;
        }
        for (int j = 0; j < 13; ++j) {
            const float w = 2.0f + 3.0f * (13 - j) / 13.0f + 2.5f * pu[j];
            rj_Seg2D(tx[j], ty[j], tx[j + 1], ty[j + 1], w,
                     RGBA(70 + (int)(150.0f * pu[j]), 4 + (int)(24.0f * pu[j]), 12 + (int)(12.0f * pu[j]), 150 + (int)(90.0f * pu[j])),
                     RGBA(70 + (int)(150.0f * pu[j + 1]), 4 + (int)(24.0f * pu[j + 1]), 12 + (int)(12.0f * pu[j + 1]), 150 + (int)(90.0f * pu[j + 1])));
        }
        for (int j = 0; j < 6; ++j) {
            const float p0 = pu[(6 + j) < 14 ? 6 + j : 13], p1 = pu[(7 + j) < 14 ? 7 + j : 13];
            rj_Seg2D(bx[j], by[j], bx[j + 1], by[j + 1], 3.0f - 0.35f * j + 1.5f * p0,
                     RGBA(70 + (int)(150.0f * p0), 4 + (int)(24.0f * p0), 12 + (int)(12.0f * p0), 140 + (int)(90.0f * p0)),
                     RGBA(70 + (int)(150.0f * p1), 4 + (int)(24.0f * p1), 12 + (int)(12.0f * p1), 140 + (int)(90.0f * p1)));
        }
        for (int j = 1; j < 14; j += 3)                                  // nudos de carne
            lvFan2D((int)tx[j], (int)ty[j], 7 + (int)(5.0f * pu[j]), RGBA(150 + (int)(90.0f * pu[j]), 18, 24, 120), RGBA(60, 0, 8, 0), 8);
    }
    // lomas de carne oscura en el horizonte (bajo la arena)
    for (int k = 0; k < 6; ++k) {
        const int cx = 40 + k * 90 + (int)(sinf(t * 0.006f + k * 2.0f) * 10.0f);
        lvFan2D(cx, 276 - (k % 2) * 10, 110 + (k * 29) % 50, RGBA(40, 2, 8, 255), RGBA(70, 6, 14, 255), 14);
    }
    sceGuDisable(GU_BLEND);
}

// ---------------------------------------------------------------- dibujo 3D
static void rj_Mat(float x, float y, float z, float yaw, float pitch, float roll, float sc) {
    sceGumLoadIdentity();
    ScePspFVector3 t = { x, y, z }; sceGumTranslate(&t);
    sceGumRotateY(yaw);
    if (roll != 0.0f) sceGumRotateZ(roll);
    ScePspFVector3 s = { sc, sc, sc }; sceGumScale(&s);
    if (pitch != 0.0f) {                                        // encabritado: gira sobre los cascos traseros
        ScePspFVector3 p1 = { 0.0f, 0.0f, RJ_PIV }; sceGumTranslate(&p1);
        sceGumRotateX(pitch);
        ScePspFVector3 p2 = { 0.0f, 0.0f, -RJ_PIV }; sceGumTranslate(&p2);
    }
}
static void rj_Leg(const LineVertex *up, int nUp, const LineVertex *lo, int nLo,
                   float hx, float hy, float hz, float ang, float flex, float kx, float ky, float kz) {
    sceGumPushMatrix();
    ScePspFVector3 t = { hx, hy, hz }; sceGumTranslate(&t);
    sceGumRotateX(ang);
    sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, nUp, 0, up);
    ScePspFVector3 k = { kx, ky, kz }; sceGumTranslate(&k);
    sceGumRotateX(-flex);
    sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, nLo, 0, lo);
    sceGumPopMatrix();
}
static float rj_ease(float v) { if (v < 0.0f) v = 0.0f; if (v > 1.0f) v = 1.0f; return v * v * (3.0f - 2.0f * v); }
static float rj_lerp(float a, float b, float t) { return a + (b - a) * t; }

// punto del modelo -> mundo (para proyectar ojos en el HUD)
static void rj_ModelToWorld(float mx, float my, float mz, float &wx, float &wy, float &wz) {
    const float zz = mz - RJ_PIV, cp = cosf(rj_pitchNow), sp = sinf(rj_pitchNow);
    const float y2 = my * cp - zz * sp, z2 = my * sp + zz * cp + RJ_PIV;
    const float yaw = rj_a + RJ_PI, cy = cosf(yaw), sy = sinf(yaw);
    wx = rj_x + (mx * cy + z2 * sy) * rj_scNow;
    wy = rj_yNow + y2 * rj_scNow;
    wz = rj_z + (-mx * sy + z2 * cy) * rj_scNow;
}

static void rj_DrawHorse() {
    float sc = RJ_S, roll = 0.0f, yoff = 0.0f, flail = 0.0f, e = 0.0f;
    if (rj_s == RJS_DYING) {
        e = rj_ease(rj_dieT / 38.0f);
        roll = 1.42f * e; yoff = 0.7f * e * RJ_S;
        sc = RJ_S * (1.0f - rj_ease((rj_dieT - 55.0f) / 95.0f));
        flail = 1.0f - e;
        if (sc < 0.02f) return;
    } else if (rj_s == RJS_DEAD) return;
    const float spR = (rj_spd > RJ_VMAX) ? 1.0f : rj_spd / RJ_VMAX;
    const float rear = rj_rear, pitch = rear * 0.72f + 0.03f * sinf(rj_gait * 2.0f) * spR;
    const float bob = (0.12f * fabsf(sinf(rj_gait)) * spR + 0.03f * sinf(rj_t * 0.07f)) * RJ_S;
    const float hurtS = (rj_hurt > 0) ? 1.03f : 1.0f;
    rj_pitchNow = pitch; rj_scNow = sc * hurtS; rj_yNow = bob + yoff;
    rj_Mat(rj_x, bob + yoff, rj_z, rj_a + RJ_PI, pitch, roll, sc * hurtS);
    const float t = (float)rj_t;

    sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, rj_nBody, 0, rj_mBody);
    // cola y capa: se tienden hacia atras con la velocidad
    sceGumPushMatrix();
    { ScePspFVector3 tr = { 0.0f, 3.55f, 2.75f }; sceGumTranslate(&tr); }
    sceGumRotateX(-(0.25f + 0.85f * spR + 0.08f * sinf(t * 0.2f)));
    sceGumRotateY(0.25f * sinf(t * 0.13f + rj_gait));
    sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, rj_nTail, 0, rj_mTail);
    sceGumPopMatrix();
    sceGumPushMatrix();
    { ScePspFVector3 tr = { 0.0f, 5.85f, 0.45f }; sceGumTranslate(&tr); }
    sceGumRotateX(-(0.6f + 0.8f * spR + 0.07f * sinf(t * 0.3f)));
    sceGumRotateY(0.1f * sinf(t * 0.17f));
    sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, rj_nCape, 0, rj_mCape);
    sceGumPopMatrix();
    // brazo de la espada
    sceGumPushMatrix();
    { ScePspFVector3 tr = { 0.95f, 5.65f, 0.1f }; sceGumTranslate(&tr); }
    sceGumRotateX(rj_sw);
    sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, rj_nArm, 0, rj_mArm);
    sceGumPopMatrix();
    // patas: delanteras (FL, FR) y traseras (HL, HR) con fases de galope
    const float off[4] = { 0.0f, 0.8f, 2.9f, 3.6f };
    const float A = 0.10f + 0.62f * spR, FK = 0.25f + 0.85f * spR;
    for (int k = 0; k < 4; ++k) {
        const float side = (k & 1) ? 1.0f : -1.0f;
        float ang = A * sinf(rj_gait + off[k]) + flail * 0.7f * sinf(t * 0.35f + off[k]) + (1.0f - flail) * (rj_s == RJS_DYING ? 0.4f : 0.0f);
        float flex = FK * fmaxf(0.0f, cosf(rj_gait + off[k])) + flail * 0.5f * (0.5f + 0.5f * sinf(t * 0.35f + off[k]));
        if (k < 2) {
            const float paw = 0.85f + 0.25f * sinf(t * 0.22f + side);
            ang = rj_lerp(ang, paw, rear); flex = rj_lerp(flex, 1.15f, rear);
            rj_Leg(rj_mFUp, rj_nFUp, rj_mFLo, rj_nFLo, side * 0.62f, 2.45f, -1.5f, ang, flex, 0.0f, -1.2f, 0.0f);
        } else {
            ang = ang * (1.0f - rear) - pitch;                          // los cascos traseros se quedan en el piso
            flex = flex * (1.0f - rear);
            rj_Leg(rj_mHUp, rj_nHUp, rj_mHLo, rj_nHLo, side * 0.65f, 2.55f, 1.75f, ang, flex, 0.0f, -1.15f, 0.35f);
        }
    }
    sceGumLoadIdentity();
}

// sombra elipsoidal bajo el jinete (alfa por vertice, sin escribir Z)
static void rj_DrawShadow() {
    if (rj_s >= RJS_DYING) return;
    const int N = 16;
    LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 3 * N);
    const float fx = sinf(rj_a), fz = cosf(rj_a), L = 5.8f, W = 2.4f;
    const unsigned int cIn = RGBA(10, 0, 4, 130), cOut = RGBA(10, 0, 4, 0);
    for (int s = 0; s < N; ++s) {
        const float a0 = RJ_TAU * s / N, a1 = RJ_TAU * (s + 1) / N;
        const float u0 = cosf(a0) * L, w0 = sinf(a0) * W, u1 = cosf(a1) * L, w1 = sinf(a1) * W;
        v[s * 3 + 0] = { cIn, rj_x, 0.07f, rj_z };
        v[s * 3 + 1] = { cOut, rj_x + fx * u0 + fz * w0, 0.07f, rj_z + fz * u0 - fx * w0 };
        v[s * 3 + 2] = { cOut, rj_x + fx * u1 + fz * w1, 0.07f, rj_z + fz * u1 - fx * w1 };
    }
    sceGumLoadIdentity();
    sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, 3 * N, 0, v);
}

// venas del suelo: ribetes que laten del centro al borde
static void rj_DrawVeins() {
    const int tot = RJ_NV * (RJ_NP - 1) * 6;
    LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * tot); int i = 0;
    const float t = (float)rj_drawT;
    const unsigned int cLo = RGBA(64, 6, 14, 255), cHi = RGBA(218, 36, 32, 255);
    for (int k = 0; k < RJ_NV; ++k) {
        for (int j = 0; j < RJ_NP - 1; ++j) {
            const float x0 = rj_vx[k][j], z0 = rj_vz[k][j], x1 = rj_vx[k][j + 1], z1 = rj_vz[k][j + 1];
            const float dx = x1 - x0, dz = z1 - z0, l = sqrtf(dx * dx + dz * dz) + 0.0001f;
            const float nx = -dz / l, nz = dx / l;
            const float w0 = 1.0f - 0.05f * j, w1 = 1.0f - 0.05f * (j + 1);
            float p0 = 0.5f + 0.5f * sinf(j * 0.5f - t * 0.09f + k * 1.1f);
            float p1 = 0.5f + 0.5f * sinf((j + 1) * 0.5f - t * 0.09f + k * 1.1f);
            const unsigned int c0 = lvMix(cLo, cHi, p0 * p0), c1 = lvMix(cLo, cHi, p1 * p1);
            swQuadC(v, i, x0 - nx * w0, 0.15f, z0 - nz * w0, c0, x0 + nx * w0, 0.15f, z0 + nz * w0, c0,
                          x1 + nx * w1, 0.15f, z1 + nz * w1, c1, x1 - nx * w1, 0.15f, z1 - nz * w1, c1);
        }
    }
    sceGumLoadIdentity();
    sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, i, 0, v);
}

// ceniza y polvo: cubitos que se encogen
static void rj_DrawParts() {
    int n = 0;
    for (int k = 0; k < 64; ++k) if (rj_parts[k].life > 0) ++n;
    if (!n) return;
    LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 30 * n); int i = 0;
    for (int k = 0; k < 64; ++k) {
        const RjPart &p = rj_parts[k]; if (p.life <= 0) continue;
        const float s = p.sz * (0.35f + 0.65f * (float)p.life / (float)p.life0);
        swBoxL(v, i, p.x, p.y - s * 0.5f, p.z, s, s, s, p.col);
    }
    sceGumLoadIdentity();
    sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, i, 0, v);
}

// linea de aviso de la embestida: cinta ancha del ancho letal + chevrones + lvBeam al centro
static void rj_DrawLane() {
    const float ang = atan2f(rj_dirX, rj_dirZ);
    float len = rj_ChordEnd(rj_x, rj_z, rj_dirX, rj_dirZ, 36.0f); if (len < 6.0f) len = 6.0f;
    const bool locked = rj_st >= 52;
    const int pulse = (rj_t & 4) ? 1 : 0;
    const float fx = rj_dirX, fz = rj_dirZ, nx = -rj_dirZ, nz = rj_dirX, W = 2.8f;
    LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * (6 + 12 * 12)); int i = 0;
    const unsigned int c0 = locked ? RGBA(255, 120, 60, 40) : RGBA(255, 40, 30, 20);
    const unsigned int c1 = locked ? RGBA(255, 70, 40, 150 + 60 * pulse) : RGBA(255, 40, 30, 90);
    const float y = 0.13f;
    swQuadC(v, i, rj_x - nx * W, y, rj_z - nz * W, c0, rj_x + nx * W, y, rj_z + nz * W, c0,
                  rj_x + fx * len + nx * W, y, rj_z + fz * len + nz * W, c1, rj_x + fx * len - nx * W, y, rj_z + fz * len - nz * W, c1);
    const unsigned int cc = locked ? RGBA(255, 236, 200, 230) : RGBA(255, 150, 90, 170);
    float ph = (float)(rj_t % 14) * 0.5f;                              // los chevrones avanzan hacia ti
    for (int k = 0; k < 12; ++k) {
        const float s = 6.0f + ph + k * 7.0f;
        if (s + 1.4f > len) break;
        for (int sd = -1; sd <= 1; sd += 2) {
            const float ax = rj_x + fx * s + nx * sd * 1.8f, az = rj_z + fz * s + nz * sd * 1.8f;
            const float bx = rj_x + fx * (s + 1.6f), bz = rj_z + fz * (s + 1.6f);
            swQuadC(v, i, ax, y + 0.02f, az, cc, ax + fx * 0.5f, y + 0.02f, az + fz * 0.5f, cc,
                          bx + fx * 0.5f, y + 0.02f, bz + fz * 0.5f, cc, bx, y + 0.02f, bz, cc);
        }
    }
    sceGumLoadIdentity();
    sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, i, 0, v);
    lvBeam(rj_x, 0.16f, rj_z, ang, len, 0.7f, locked ? RGBA(255, 244, 210, 235) : RGBA(255, 80, 50, 150 + 60 * pulse));
}

static void br_Draw(float px, float py, float pz) {
    (void)px; (void)py; (void)pz;
    ++rj_drawT;
    sceGuDisable(GU_CULL_FACE);
    rj_DrawVeins();
    rj_DrawHorse();
    rj_DrawParts();
    // efectos translucidos (sin escribir Z)
    sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0); sceGuDepthMask(GU_TRUE);
    rj_DrawShadow();
    if (rj_s == RJS_ROAR) rj_DrawLane();
    if (rj_s == RJS_SLASH && rj_st < 48) {                           // aviso del tajo: punto de impacto y alcance
        const int a = 80 + ((rj_t & 8) ? 70 : 0);
        lvRing3D(rj_ix, 0.13f, rj_iz, 2.0f + 0.04f * rj_st, 0.6f, RGBA(255, 40, 30, a));
        lvRing3D(rj_ix, 0.12f, rj_iz, 29.0f, 0.45f, RGBA(255, 60, 40, 50 + ((rj_t & 8) ? 30 : 0)));
    }
    for (int w = 0; w < 3; ++w) if (rj_wv[w].on) {                   // onda roja que se salta
        lvRing3D(rj_wv[w].x, 0.12f, rj_wv[w].z, rj_wv[w].r, 1.4f, RGBA(230, 20, 24, 230));
        lvRing3D(rj_wv[w].x, 0.14f, rj_wv[w].z, rj_wv[w].r, 0.45f, RGBA(255, 190, 150, 255));
    }
    sceGuDepthMask(GU_FALSE); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0); sceGuDisable(GU_BLEND);
    sceGumLoadIdentity();
}

// ---------------------------------------------------------------- HUD
static void rj_EyeGlow() {
    if (rj_s >= RJS_DYING) return;
    const float k = (rj_s == RJS_ROAR || rj_s == RJS_SCREAM || rj_s == RJS_CHARGE) ? 1.7f : 1.0f;
    const float pulse = 0.85f + 0.15f * sinf(rj_t * 0.2f);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_FIX, 0, 0xFFFFFFFF);      // luz sumada
    for (int pass = 0; pass < 2; ++pass) {
        for (int s = -1; s <= 1; s += 2) {
            float wx, wy, wz; int sx, sy;
            if (pass == 0) rj_ModelToWorld(s * 0.4f, 5.02f, -3.5f, wx, wy, wz);     // ojos del caballo
            else           rj_ModelToWorld(s * 0.16f, 6.5f, -0.6f, wx, wy, wz);     // ojos del casco
            if (!lvProject(wx, wy, wz, sx, sy)) continue;
            const float dx = wx - g_lvEye[0], dy = wy - g_lvEye[1], dz = wz - g_lvEye[2];
            const float d = sqrtf(dx * dx + dy * dy + dz * dz) + 1.0f;
            int R = (int)(260.0f / d * k * (pass == 0 ? 1.0f : 0.6f));
            if (R < 4) R = 4; if (R > 40) R = 40;
            lvFan2D(sx, sy, R, pass == 0 ? RGBA(255, 214, 90, (int)(210.0f * pulse)) : RGBA(255, 70, 40, (int)(230.0f * pulse)),
                    RGBA(255, 60, 20, 0), 10);
        }
    }
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
}

static void br_Hud(BossIO &io) {
    (void)io;
    sceGuDisable(GU_DEPTH_TEST); sceGuDisable(GU_TEXTURE_2D);
    sceGuEnable(GU_BLEND); sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    rj_EyeGlow();
    // tintes rojos: grito, golpes, ticks del aturdimiento
    if (rj_tint > 0) bxTint(RGBA(200, 8, 8, rj_tint * 150 / (rj_tintMax > 0 ? rj_tintMax : 1)));
    if (rj_tickFx > 0) bxTint(RGBA(255, 20, 20, rj_tickFx * 7));
    if (rj_stun) bxTint(RGBA(130, 0, 0, 34 + (int)(14.0f * sinf(rj_t * 0.3f))));
    if (rj_s != RJS_DEAD) bxBossBar("JINETE ROJO", rj_hp, RJ_HPMAX);
    // avisos
    if (rj_s == RJS_ROAR && rj_st > 6) {
        const unsigned int c = (rj_t & 8) ? RGBA(255, 150, 70, 255) : RGBA(255, 70, 40, 255);
        fontTexOn();
        drawText(240 - textLen("¡ESQUIVA LA EMBESTIDA!") * 4, 52, 1.0f, c, "¡ESQUIVA LA EMBESTIDA!");
        drawText(240 - textLen("MUÉVETE O SALTA") * 4, 64, 1.0f, RGBA(255, 214, 180, 255), "MUÉVETE O SALTA");
        sceGuDisable(GU_TEXTURE_2D);
    }
    if (rj_s == RJS_SLASH && rj_st < 50) {
        fontTexOn();
        drawText(240 - textLen("¡SALTA LA ONDA!") * 4, 52, 1.0f, (rj_t & 8) ? RGBA(255, 150, 70, 255) : RGBA(255, 70, 40, 255), "¡SALTA LA ONDA!");
        sceGuDisable(GU_TEXTURE_2D);
    }
    if (rj_stun) {
        fontTexOn();
        drawText(240 - textLen("¡ATURDIDO!") * 4, 118, 1.0f, (rj_t & 8) ? RGBA(255, 90, 70, 255) : RGBA(255, 200, 190, 255), "¡ATURDIDO!");
        sceGuDisable(GU_TEXTURE_2D);
        bxMashHud(rj_mash, RJ_MASHNEED, "MUEVE EL ANÁLOGO");
    }
    sceGuDisable(GU_TEXTURE_2D); sceGuEnable(GU_DEPTH_TEST);
}
