// ============================================================================
// rig.h -- HUNTER ARTICULADO (2026-10-15; v2 2026-10-16)
//
// Cada pieza de cand/hunter.h (rangos en g_hnMark) se dibuja con su propio giro alrededor de su
// articulacion, en coordenadas del modelo (metros * HN_S):
//   cadera -> muslo, rodilla -> cana+bota, hombro -> brazo, codo -> antebrazo (+ pistola o sable),
//   cuello -> cabeza, y el ABRIGO en 4 paneles: dos colas traseras que flamean y dos paneles
//   laterales que acompanan al muslo de su lado (v2: antes era UNA campana rigida que corriendo
//   giraba entera hacia atras como una tabla).
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
    float tailL, tailR, skirtZ;        // colas traseras: vuelo (- = hacia atras) / vaiven lateral
    float flapL, flapR;                // paneles laterales del abrigo (siguen al muslo)
};

// articulaciones (metros, las mismas coordenadas con que se arma hunter.h)
static const float HN_JHIP[3]  = { 0.095f, 0.960f,  0.000f };   // x se espeja por lado
static const float HN_JKNEE[3] = { 0.105f, 0.500f, -0.012f };
static const float HN_JSHL[3]  = { 0.200f, 1.430f, -0.012f };
static const float HN_JELB_L[3] = { -0.236f, 1.150f, -0.052f };
static const float HN_JELB_R[3] = {  0.250f, 1.145f, -0.052f };

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
    rigDraw(M[8], M[9]);                                                   // torso (rigido con el cuerpo)
    // piernas: cadera y despues rodilla, encadenadas
    sceGumPushMatrix();
    rigPivot(-HN_JHIP[0], HN_JHIP[1], HN_JHIP[2], r.legL, 0.0f);     rigDraw(M[0], M[1]);
    rigPivot(-HN_JKNEE[0], HN_JKNEE[1], HN_JKNEE[2], -r.kneeL, 0.0f); rigDraw(M[1], M[2]);
    sceGumPopMatrix();
    sceGumPushMatrix();
    rigPivot(HN_JHIP[0], HN_JHIP[1], HN_JHIP[2], r.legR, 0.0f);      rigDraw(M[2], M[3]);
    rigPivot(HN_JKNEE[0], HN_JKNEE[1], HN_JKNEE[2], -r.kneeR, 0.0f);  rigDraw(M[3], M[4]);
    sceGumPopMatrix();
    // abrigo: colas traseras (cuelgan de la cintura, atras) y paneles laterales (de la cadera)
    sceGumPushMatrix(); rigPivot(0.0f, 1.130f, 0.050f, r.tailL, r.skirtZ - 0.04f); rigDraw(M[4], M[5]); sceGumPopMatrix();
    sceGumPushMatrix(); rigPivot(0.0f, 1.130f, 0.050f, r.tailR, r.skirtZ + 0.04f); rigDraw(M[5], M[6]); sceGumPopMatrix();
    sceGumPushMatrix(); rigPivot(-0.170f, 1.120f, 0.000f, r.flapL, r.skirtZ); rigDraw(M[6], M[7]); sceGumPopMatrix();
    sceGumPushMatrix(); rigPivot( 0.170f, 1.120f, 0.000f, r.flapR, r.skirtZ); rigDraw(M[7], M[8]); sceGumPopMatrix();
    // brazos: hombro (adelante/atras + abrir) y codo
    sceGumPushMatrix();
    rigPivot(-HN_JSHL[0], HN_JSHL[1], HN_JSHL[2], r.armL, -r.outL);  rigDraw(M[9], M[10]);
    rigPivot(HN_JELB_L[0], HN_JELB_L[1], HN_JELB_L[2], r.elbowL, 0.0f); rigDraw(M[10], M[11]);
    sceGumPopMatrix();
    sceGumPushMatrix();
    rigPivot(HN_JSHL[0], HN_JSHL[1], HN_JSHL[2], r.armR, r.outR);    rigDraw(M[11], M[12]);
    rigPivot(HN_JELB_R[0], HN_JELB_R[1], HN_JELB_R[2], r.elbowR, 0.0f); rigDraw(M[12], M[13]);   // + sable
    sceGumPopMatrix();
    // cabeza
    sceGumPushMatrix();
    {
        ScePspFVector3 t = { 0.0f, 1.575f * HN_S, -0.060f * HN_S }; sceGumTranslate(&t);
        if (r.headYaw != 0.0f) sceGumRotateY(r.headYaw);
        sceGumRotateX(-r.head);
        ScePspFVector3 b = { 0.0f, -1.575f * HN_S, 0.060f * HN_S }; sceGumTranslate(&b);
    }
    rigDraw(M[13], M[14]);
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

static inline float rgMix(float a, float b, float k) { return a + (b - a) * k; }

// Arma la pose final a partir del ciclo de anim.h + lo que esta haciendo el jugador.
//   spd01   velocidad 0..1 (normalizada a la velocidad REAL de caminata)
//   landK   1 al aterrizar fuerte, decae solo (lo maneja main)
//   meleeT  1 al empezar el tajo, 0 al terminar
// 0..1: cuanto esta CORRIENDO (main lo sube al mantener el nub 3 s y lo baja suave al soltar)
static float g_hunterRunK = 0.0f;
static HunterRig hunterRig(HunterPose &p, float walkPhase, float idleT, float spd01, int grounded,
                           int floating, int aiming, float meleeT, float landK) {
    HunterRig r;
    r.legL = p.legL; r.kneeL = p.kneeL; r.legR = p.legR; r.kneeR = p.kneeR;
    // el brazo del SABLE bracea menos (con el braceo entero la hoja barria hacia adelante)
    // (y va un poco atras: la hoja ya apunta adelante-abajo en el modelo; si el brazo tambien iba
    // adelante, al caminar el sable quedaba casi horizontal)
    r.armL = p.armL; r.armR = p.armR * 0.45f - 0.22f;
    // codo: siempre un poco doblado (brazo vivo, no palo) y mas cuando el brazo va adelante
    r.elbowL = 0.22f + 0.45f * (p.armL > 0.0f ? p.armL : 0.0f);
    r.elbowR = 0.18f + 0.20f * (r.armR > 0.0f ? r.armR : 0.0f);
    r.outL = 0.10f; r.outR = 0.16f;                       // brazos despegados del abrigo (el sable, mas)
    r.head = p.headNod; r.headYaw = 0.0f;
    // abrigo caminando: las colas se van un poco atras con la velocidad y flamean con cada paso
    // (cada una a su ritmo); los paneles laterales acompanan a medias al muslo de su lado
    const float sk = -0.14f * spd01 + 0.02f * sinf(idleT * 0.8f);
    r.tailL = sk - 0.05f * spd01 * sinf(walkPhase * 2.0f + 0.6f);
    r.tailR = sk - 0.05f * spd01 * sinf(walkPhase * 2.0f + 2.1f);
    r.skirtZ = 1.2f * p.roll + 0.03f * spd01 * sinf(walkPhase);
    r.flapL = 0.55f * r.legL; r.flapR = 0.55f * r.legR;

    if (g_hunterRunK > 0.0f && grounded && !floating) {   // CORRER: su propio ciclo (anim.h), mezclado
        const float k = g_hunterRunK;
        const HunterRunPose q = hunterRunPose(walkPhase, idleT);
        r.legL = rgMix(r.legL, q.legL, k); r.legR = rgMix(r.legR, q.legR, k);
        r.kneeL = rgMix(r.kneeL, q.kneeL, k); r.kneeR = rgMix(r.kneeR, q.kneeR, k);
        r.armL = rgMix(r.armL, q.armL, k); r.armR = rgMix(r.armR, q.armR, k);
        r.elbowL = rgMix(r.elbowL, q.elbowL, k); r.elbowR = rgMix(r.elbowR, q.elbowR, k);
        r.outL = rgMix(r.outL, q.outL, k); r.outR = rgMix(r.outR, q.outR, k);
        r.head = rgMix(r.head, q.head, k);
        r.tailL = rgMix(r.tailL, q.tailL, k); r.tailR = rgMix(r.tailR, q.tailR, k);
        r.flapL = rgMix(r.flapL, q.flapL, k); r.flapR = rgMix(r.flapR, q.flapR, k);
        r.skirtZ = rgMix(r.skirtZ, q.skirtZ, k);
        p.bobY = rgMix(p.bobY, q.bobY, k); p.bobX = rgMix(p.bobX, 0.0f, k);
        p.lean = rgMix(p.lean, q.lean, k); p.roll = rgMix(p.roll, q.roll, k);
        p.yawSway = rgMix(p.yawSway, q.yawSway, k);
    }
    if (!grounded && !floating) { r.tailL += 0.30f; r.tailR += 0.26f; }   // cayendo: el aire levanta las colas
    if (floating) {
        // LEVITACION: brazos abiertos, piernas colgando juntas, todo flota lento
        const float f = sinf(idleT * 1.3f);
        r.armL = 0.15f + 0.05f * f; r.armR = 0.15f - 0.05f * f;
        r.outL = 0.95f + 0.08f * f; r.outR = 0.95f - 0.08f * f;
        r.elbowL = r.elbowR = 0.30f;
        r.legL = 0.10f + 0.04f * f; r.legR = -0.04f - 0.04f * f;
        r.kneeL = 0.30f; r.kneeR = 0.45f;
        r.tailL = -0.08f + 0.10f * f; r.tailR = -0.06f + 0.09f * f; r.skirtZ = 0.06f * sinf(idleT * 0.9f);
        r.flapL = 0.05f; r.flapR = -0.02f;
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
        r.tailL += 0.15f * landK; r.tailR += 0.15f * landK;
        r.flapL += 0.20f * landK; r.flapR += 0.20f * landK;
        p.bobY -= 0.16f * landK;                           // baja el centro de masa
        p.lean += 0.22f * landK;
    }
    return r;
}
