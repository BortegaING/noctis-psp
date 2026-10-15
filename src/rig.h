// ============================================================================
// rig.h -- HUNTER ARTICULADO (2026-10-15)
//
// Hasta ahora g_hero se dibujaba como UNA malla rigida: el cuerpo entero se mecia
// e inclinaba, pero piernas, brazos, cabeza y sable iban soldados -> "esta
// estatico". anim.h ya calculaba legL/kneeL/armL/headNod... y nadie los usaba.
//
// Ahora cada pieza de cand/hunter.h (rangos en g_hnMark) se dibuja con su propio
// giro alrededor de su articulacion, en coordenadas del modelo (metros * HN_S):
//   cadera -> muslo, rodilla -> cana+pie, hombro -> brazo, codo -> antebrazo
//   (+ el SABLE en la derecha), cuello -> cabeza, cintura -> faldon.
//
// Signos (el modelo mira a -Z):
//   rotX(+a) en una pieza que CUELGA de su pivote lleva su punta hacia -Z (adelante).
//   rotZ(+a) lleva hacia +X la punta de una pieza que cuelga.
// Se usa con la matriz del CUERPO ya cargada (posicion, rumbo, bob, inclinacion).
// ============================================================================

struct HunterRig {
    float legL, kneeL, legR, kneeR;    // cadera (+ adelante) / rodilla (+ = flexion, talon atras)
    float armL, elbowL, outL;          // hombro (+ adelante) / codo (+ flexion) / separacion lateral
    float armR, elbowR, outR;
    float head, headYaw;               // cabeceo (+ = mira abajo) / giro de cabeza
    float skirtX, skirtZ;              // vuelo del faldon (- = hacia atras) / vaiven lateral
};

static void rigPivot(float x, float y, float z, float rx, float rz) {
    ScePspFVector3 t = { x * HN_S, y * HN_S, z * HN_S };
    sceGumTranslate(&t);
    if (rx != 0.0f) sceGumRotateX(rx);
    if (rz != 0.0f) sceGumRotateZ(rz);
    ScePspFVector3 b = { -x * HN_S, -y * HN_S, -z * HN_S };
    sceGumTranslate(&b);
}
static void rigDraw(int a, int b) {
    if (b > a) sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, b - a, 0, g_hero + a);
}

static void drawHunterRig(const HunterRig &r) {
    const int *M = g_hnMark;
    // faldon: cuelga de la cintura y se lo lleva el aire
    sceGumPushMatrix(); rigPivot(0.0f, 1.14f, 0.0f, r.skirtX, r.skirtZ); rigDraw(M[4], M[5]); sceGumPopMatrix();
    rigDraw(M[5], M[6]);                                                  // torso (rigido con el cuerpo)
    // piernas: cadera y despues rodilla, encadenadas
    sceGumPushMatrix();
    rigPivot(-0.095f, 0.950f, 0.020f, r.legL, 0.0f);   rigDraw(M[0], M[1]);
    rigPivot(-0.112f, 0.470f, -0.030f, -r.kneeL, 0.0f); rigDraw(M[1], M[2]);
    sceGumPopMatrix();
    sceGumPushMatrix();
    rigPivot(0.095f, 0.950f, 0.038f, r.legR, 0.0f);    rigDraw(M[2], M[3]);
    rigPivot(0.125f, 0.478f, 0.058f, -r.kneeR, 0.0f);  rigDraw(M[3], M[4]);
    sceGumPopMatrix();
    // brazos: hombro (adelante/atras + abrir) y codo
    sceGumPushMatrix();
    rigPivot(-0.195f, 1.420f, -0.025f, r.armL, -r.outL); rigDraw(M[6], M[7]);
    rigPivot(-0.235f, 1.145f, -0.065f, r.elbowL, 0.0f);  rigDraw(M[7], M[8]);
    sceGumPopMatrix();
    sceGumPushMatrix();
    rigPivot(0.195f, 1.420f, -0.025f, r.armR, r.outR);   rigDraw(M[8], M[9]);
    rigPivot(0.255f, 1.140f, -0.060f, r.elbowR, 0.0f);   rigDraw(M[9], M[10]);   // + sable
    sceGumPopMatrix();
    // cabeza
    sceGumPushMatrix();
    {
        ScePspFVector3 t = { 0.0f, 1.575f * HN_S, -0.075f * HN_S }; sceGumTranslate(&t);
        if (r.headYaw != 0.0f) sceGumRotateY(r.headYaw);
        sceGumRotateX(-r.head);
        ScePspFVector3 b = { 0.0f, -1.575f * HN_S, 0.075f * HN_S }; sceGumTranslate(&b);
    }
    rigDraw(M[10], M[11]);
    sceGumPopMatrix();
}

// SOMBRA de contacto bajo el personaje: disco oscuro que se DESVANECE hacia el borde
// (alfa por vertice) y se achica/aclara con la altura. Es lo que "apoya" al personaje en
// el suelo; sin ella flota aunque este parado. Sin escritura de Z (no tapa nada).
static void drawBlobShadow(float x, float gy, float z, float aboveGround) {
    float k = 1.0f - aboveGround / 14.0f;            // 1 en el suelo .. 0 a 14u de altura
    if (k <= 0.05f) return;
    if (k > 1.0f) k = 1.0f;
    const float R = 1.05f + 0.55f * k;
    const int A = (int)(150.0f * k);
    const int N = 14;
    LineVertex *v = (LineVertex *)sceGuGetMemory(sizeof(LineVertex) * 3 * N);
    const unsigned int cIn = RGBA(8, 10, 14, A), cOut = RGBA(8, 10, 14, 0);
    const float y = gy + 0.05f;
    for (int s = 0; s < N; ++s) {
        const float a0 = 6.2831853f * s / N, a1 = 6.2831853f * (s + 1) / N;
        v[s * 3 + 0] = { cIn,  x, y, z };
        v[s * 3 + 1] = { cOut, x + cosf(a0) * R, y, z + sinf(a0) * R };
        v[s * 3 + 2] = { cOut, x + cosf(a1) * R, y, z + sinf(a1) * R };
    }
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    sceGuDepthMask(GU_TRUE);                          // no escribe Z
    sceGumLoadIdentity();
    sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, 3 * N, 0, v);
    sceGuDepthMask(GU_FALSE);
    sceGuDisable(GU_BLEND);
}

// Arma la pose final a partir del ciclo de anim.h + lo que esta haciendo el jugador.
//   spd01   velocidad 0..1 (normalizada a la velocidad REAL de carrera)
//   landK   1 al aterrizar fuerte, decae solo (lo maneja main)
//   meleeT  1 al empezar el tajo, 0 al terminar
static HunterRig hunterRig(HunterPose &p, float walkPhase, float idleT, float spd01, int grounded,
                           int floating, int aiming, float meleeT, float landK) {
    HunterRig r;
    r.legL = p.legL; r.kneeL = p.kneeL; r.legR = p.legR; r.kneeR = p.kneeR;
    r.armL = p.armL; r.armR = p.armR;
    // codo: siempre un poco doblado (brazo vivo, no palo) y mas cuando el brazo va adelante
    r.elbowL = 0.22f + 0.45f * (p.armL > 0.0f ? p.armL : 0.0f);
    r.elbowR = 0.22f + 0.45f * (p.armR > 0.0f ? p.armR : 0.0f);
    r.outL = 0.10f; r.outR = 0.12f;                       // brazos despegados del abrigo
    r.head = p.headNod; r.headYaw = 0.0f;
    // faldon: se va hacia atras con la velocidad y flamea con cada paso
    r.skirtX = -0.22f * spd01 - 0.05f * spd01 * sinf(walkPhase * 2.0f) + 0.02f * sinf(idleT * 0.8f);
    r.skirtZ = 1.4f * p.roll + 0.04f * spd01 * sinf(walkPhase);

    if (!grounded && !floating) r.skirtX += 0.30f;        // cayendo: el aire levanta el faldon
    if (floating) {
        // LEVITACION: brazos abiertos, piernas colgando juntas, todo flota lento
        const float f = sinf(idleT * 1.3f);
        r.armL = 0.15f + 0.05f * f; r.armR = 0.15f - 0.05f * f;
        r.outL = 0.95f + 0.08f * f; r.outR = 0.95f - 0.08f * f;
        r.elbowL = r.elbowR = 0.30f;
        r.legL = 0.10f + 0.04f * f; r.legR = -0.04f - 0.04f * f;
        r.kneeL = 0.30f; r.kneeR = 0.45f;
        r.skirtX = -0.08f + 0.10f * f; r.skirtZ = 0.06f * sinf(idleT * 0.9f);
        r.head = 0.18f;                                    // mira hacia abajo, al vacio
    }
    if (aiming) {                                          // APUNTAR: la PISTOLA (mano izq) al frente
        r.armL = 1.42f; r.elbowL = 0.05f; r.outL = -0.10f;
        r.armR = 0.30f; r.elbowR = 0.50f; r.outR = 0.22f;   // el sable queda abajo, listo
        r.head = -0.05f;
    }
    if (meleeT > 0.0f) {                                   // TAJO: de arriba-adelante a abajo-atras
        r.armR = -0.75f + 2.70f * meleeT;
        r.outR = 0.55f * meleeT;
        r.elbowR = 0.35f * meleeT;
    }
    if (landK > 0.0f) {                                    // ATERRIZAJE: se agacha y amortigua
        r.legL += 0.35f * landK; r.legR += 0.35f * landK;
        r.kneeL += 0.95f * landK; r.kneeR += 0.95f * landK;
        r.armL += 0.25f * landK; r.armR += 0.25f * landK;
        r.outL += 0.25f * landK; r.outR += 0.25f * landK;
        r.skirtX += 0.15f * landK;
        p.bobY -= 0.16f * landK;                           // baja el centro de masa
        p.lean += 0.22f * landK;
    }
    return r;
}
