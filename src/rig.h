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
// v3 (2026-10-16, Benjamin: "se ve muy tosco y rigido"): el PECHO gira sobre la pelvis (hombros
// contra caderas al caminar, el cuerpo entero acompana el tajo, se dobla al acelerar y al caer),
// los PIES tienen tobillo (apoyan planos, despegan con la punta, entran de talon) y toda la pose
// pasa por un paso-bajo: apuntar, el tajo, saltar o empezar a correr ya no cambian de golpe.
//
// Signos (el modelo mira a -Z):
//   rotX(+a) en una pieza que CUELGA de su pivote lleva su punta hacia -Z (adelante); en el PIE
//   (que sale hacia adelante del tobillo) sube la punta.
//   rotZ(+a) lleva hacia +X la punta de una pieza que cuelga.
//   rotY(+a) lleva el hombro DERECHO hacia adelante.
// Se usa con la matriz del CUERPO ya cargada (posicion, rumbo, bob, inclinacion).
// ============================================================================

struct HunterRig {
    float legL, kneeL, legR, kneeR;    // cadera (+ adelante) / rodilla (+ = flexion, talon atras)
    float ankleL, ankleR;              // tobillo, relativo a la cana (+ = punta arriba)
    float armL, elbowL, outL;          // hombro (+ adelante) / codo (+ flexion) / separacion lateral
    float wristL;                      // muneca de la pistola (+ = la boca del canon baja)
    float armR, elbowR, outR;
    float head, headYaw;               // cabeceo (+ = mira abajo) / giro de cabeza
    float tailL, tailR, skirtZ;        // colas traseras: vuelo (- = hacia atras) / vaiven lateral
    float flapL, flapR;                // paneles laterales del abrigo (siguen al muslo)
    float chestYaw, chestBend, chestRoll;   // pecho sobre la pelvis: giro (rotY) / doblez (+ = adelante) / ladeo (rotZ)
};

// articulaciones (metros, las mismas coordenadas con que se arma hunter.h)
static const float HN_JHIP[3]  = { 0.095f, 0.960f,  0.000f };   // x se espeja por lado
static const float HN_JKNEE[3] = { 0.105f, 0.500f, -0.012f };
static const float HN_JSHL[3]  = { 0.200f, 1.430f, -0.012f };
static const float HN_JELB_L[3] = { -0.236f, 1.150f, -0.052f };
static const float HN_JELB_R[3] = {  0.250f, 1.145f, -0.052f };
static const float HN_JANK[3]   = { 0.110f, 0.085f,  0.022f };   // x se espeja por lado
static const float HN_JCHEST[3] = { 0.000f, 1.220f, -0.010f };   // la cintura: aca se dobla y gira el pecho
static const float HN_JWRIST_L[3] = { -0.234f, 0.905f, -0.118f }; // muneca izquierda (la de la pistola)

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
    rigDraw(M[10], M[11]);                                                 // pelvis (rigida con el cuerpo)
    // piernas: cadera -> rodilla -> tobillo, encadenadas
    sceGumPushMatrix();
    rigPivot(-HN_JHIP[0], HN_JHIP[1], HN_JHIP[2], r.legL, 0.0f);     rigDraw(M[0], M[1]);
    rigPivot(-HN_JKNEE[0], HN_JKNEE[1], HN_JKNEE[2], -r.kneeL, 0.0f); rigDraw(M[1], M[2]);
    rigPivot(-HN_JANK[0], HN_JANK[1], HN_JANK[2], r.ankleL, 0.0f);    rigDraw(M[2], M[3]);
    sceGumPopMatrix();
    sceGumPushMatrix();
    rigPivot(HN_JHIP[0], HN_JHIP[1], HN_JHIP[2], r.legR, 0.0f);      rigDraw(M[3], M[4]);
    rigPivot(HN_JKNEE[0], HN_JKNEE[1], HN_JKNEE[2], -r.kneeR, 0.0f);  rigDraw(M[4], M[5]);
    rigPivot(HN_JANK[0], HN_JANK[1], HN_JANK[2], r.ankleR, 0.0f);     rigDraw(M[5], M[6]);
    sceGumPopMatrix();
    // abrigo: colas traseras (cuelgan de la cintura, atras) y paneles laterales (de la cadera)
    sceGumPushMatrix(); rigPivot(0.0f, 1.130f, 0.050f, r.tailL, r.skirtZ - 0.04f); rigDraw(M[6], M[7]); sceGumPopMatrix();
    sceGumPushMatrix(); rigPivot(0.0f, 1.130f, 0.050f, r.tailR, r.skirtZ + 0.04f); rigDraw(M[7], M[8]); sceGumPopMatrix();
    sceGumPushMatrix(); rigPivot(-0.170f, 1.120f, 0.000f, r.flapL, r.skirtZ); rigDraw(M[8], M[9]); sceGumPopMatrix();
    sceGumPushMatrix(); rigPivot( 0.170f, 1.120f, 0.000f, r.flapR, r.skirtZ); rigDraw(M[9], M[10]); sceGumPopMatrix();
    // PECHO: gira, se dobla y se ladea en la cintura; brazos y cabeza van montados en el
    sceGumPushMatrix();
    {
        ScePspFVector3 t = { 0.0f, HN_JCHEST[1] * HN_S, HN_JCHEST[2] * HN_S }; sceGumTranslate(&t);
        if (r.chestYaw != 0.0f) sceGumRotateY(r.chestYaw);
        if (r.chestBend != 0.0f) sceGumRotateX(-r.chestBend);
        if (r.chestRoll != 0.0f) sceGumRotateZ(r.chestRoll);
        ScePspFVector3 b = { 0.0f, -HN_JCHEST[1] * HN_S, -HN_JCHEST[2] * HN_S }; sceGumTranslate(&b);
    }
    rigDraw(M[11], M[12]);
    // brazos: hombro (adelante/atras + abrir) y codo
    sceGumPushMatrix();
    rigPivot(-HN_JSHL[0], HN_JSHL[1], HN_JSHL[2], r.armL, -r.outL);  rigDraw(M[12], M[13]);
    rigPivot(HN_JELB_L[0], HN_JELB_L[1], HN_JELB_L[2], r.elbowL, 0.0f); rigDraw(M[13], M[14]);
    rigPivot(HN_JWRIST_L[0], HN_JWRIST_L[1], HN_JWRIST_L[2], r.wristL, 0.0f); rigDraw(M[14], M[15]);   // mano + pistola
    sceGumPopMatrix();
    sceGumPushMatrix();
    rigPivot(HN_JSHL[0], HN_JSHL[1], HN_JSHL[2], r.armR, r.outR);    rigDraw(M[15], M[16]);
    rigPivot(HN_JELB_R[0], HN_JELB_R[1], HN_JELB_R[2], r.elbowR, 0.0f); rigDraw(M[16], M[17]);   // + sable
    sceGumPopMatrix();
    // cabeza
    sceGumPushMatrix();
    {
        ScePspFVector3 t = { 0.0f, 1.575f * HN_S, -0.060f * HN_S }; sceGumTranslate(&t);
        if (r.headYaw != 0.0f) sceGumRotateY(r.headYaw);
        sceGumRotateX(-r.head);
        ScePspFVector3 b = { 0.0f, -1.575f * HN_S, 0.060f * HN_S }; sceGumTranslate(&b);
    }
    rigDraw(M[17], M[18]);
    sceGumPopMatrix();
    sceGumPopMatrix();   // pecho
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
static int   g_hunterSlashType = 0;   // tajo del combo de sable (0 horizontal, 1 de vuelta, 2 media luna)
static float g_hunterTurn = 0.0f;     // cuanto giro el cuerpo este frame (rad; + = a la derecha). Lo pone main.

// MOVIMIENTO SECUNDARIO (2026-10-16, Benjamin: "mejora el movimiento del personaje, tiene que verse mas
// fluido"): el abrigo y la cabeza ya no siguen al cuerpo al instante. Resortes amortiguados: al FRENAR
// las colas siguen de largo y vuelven, al ARRANCAR se quedan atras, en una CURVA el abrigo se abre hacia
// afuera y la cabeza mira hacia donde dobla antes que el cuerpo (anticipacion).
static float s_coatA = 0.0f, s_coatV = 0.0f, s_swayA = 0.0f, s_swayV = 0.0f, s_headY = 0.0f, s_prevSpd = 0.0f;
static float s_accA = 0.0f, s_accV = 0.0f, s_prevEff = 0.0f;
static inline float rgClampF(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
static void hunterSecondary(HunterRig &r, HunterPose &p, float spd01, int grounded) {
    const float acc = spd01 - s_prevSpd; s_prevSpd = spd01;
    const float tgt = grounded ? rgClampF(-acc * 10.0f, -0.6f, 0.6f) : 0.0f;   // frenar -> adelante (+)
    s_coatV += (tgt - s_coatA) * 0.12f; s_coatV *= 0.80f; s_coatA += s_coatV;
    r.tailL += s_coatA; r.tailR += s_coatA * 0.9f; r.flapL += s_coatA * 0.45f; r.flapR += s_coatA * 0.45f;
    const float tz = rgClampF(-g_hunterTurn * 6.0f, -0.35f, 0.35f);           // la fuerza de la curva: afuera
    s_swayV += (tz - s_swayA) * 0.10f; s_swayV *= 0.80f; s_swayA += s_swayV;
    r.skirtZ += s_swayA;
    s_headY += (rgClampF(-g_hunterTurn * 7.0f, -0.45f, 0.45f) - s_headY) * 0.15f;   // mira hacia la curva
    r.chestYaw += 0.4f * s_headY; r.headYaw += 0.6f * s_headY;                     // (el pecho acompana la mitad)
    // ACELERAR / FRENAR: al arrancar (o al pasar a correr) el cuerpo se va adelante y el pecho mas;
    // al frenar se echa atras un poco. Resorte: se pasa apenas y vuelve.
    const float eff = spd01 * (1.0f + 0.75f * g_hunterRunK);
    const float ac = eff - s_prevEff; s_prevEff = eff;
    const float ta = grounded ? rgClampF(ac * 6.0f, -0.18f, 0.30f) : 0.0f;
    s_accV += (ta - s_accA) * 0.10f; s_accV *= 0.78f; s_accA += s_accV;
    p.lean += 0.30f * s_accA; r.chestBend += 0.55f * s_accA;
}
// TOBILLO de un pie. pr = fase PROPIA de esa pierna (0..2PI; el talon toca en ~PI/2). shinW / legW =
// angulo de la cana y del muslo EN EL MUNDO (+ = adelante). Apoyado: el pie queda PLANO (-shinW) y,
// con la pierna atras, despega el talon (la punta empuja). En el vuelo: punta abajo al despegar y
// arriba al llegar, asi entra de talon.
static inline float rgSmooth01(float t) { t = rgClampF(t, 0.0f, 1.0f); return t * t * (3.0f - 2.0f * t); }
static float rgAnkle(float pr, float shinW, float legW, float runK) {
    const float a0 = rgMix(1.50f, 1.45f, runK), a1 = rgMix(2.00f, 1.85f, runK);   // talon -> pie plano
    const float b0 = rgMix(3.95f, 3.30f, runK), b1 = rgMix(4.40f, 3.75f, runK);   // despega
    const float st = rgSmooth01((pr - a0) / (a1 - a0)) * (1.0f - rgSmooth01((pr - b0) / (b1 - b0)));
    const float push = legW < -0.25f ? 1.2f * (legW + 0.25f) : 0.0f;
    float u = pr - b0; if (u < 0.0f) u += 6.2831853f;
    u = rgClampF(u / (6.2831853f - b0 + a0), 0.0f, 1.0f);                         // 0 al despegar .. 1 al apoyar
    const float swing = rgMix(-0.30f, -0.50f, runK) + rgMix(0.45f, 0.62f, runK) * rgSmooth01(u * 1.4f);
    return st * (push - shinW) + (1.0f - st) * swing;
}
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
    r.ankleL = r.ankleR = 0.0f; r.chestYaw = r.chestBend = r.chestRoll = 0.0f; r.wristL = 0.0f;
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
    const float kRun = (g_hunterRunK > 0.0f && grounded && !floating) ? g_hunterRunK : 0.0f;
    const float gw = hp_smooth01(spd01);
    {   // PECHO: el cuerpo gira con la PELVIS (yawSway); los hombros van al REVES (x1.7 = netos 0.7
        // contra la pelvis) y la cabeza compensa casi todo para seguir mirando al frente. Se dobla un
        // poco en cada pisada (camina) o a mitad del apoyo (corre), y respira.
        const float twist = 1.7f * p.yawSway;
        r.chestYaw = twist; r.headYaw = -0.8f * (twist - p.yawSway);
        r.chestBend = rgMix(-0.035f * gw * cosf(2.0f * walkPhase), 0.05f * cosf(2.0f * (walkPhase - 2.45f)), kRun)
                    - 0.016f * sinf(idleT * 0.60f);
        r.chestRoll = 0.5f * p.roll;                       // los hombros se ladean la mitad que la cadera
    }
    if (!grounded && !floating) {                          // cayendo: el aire levanta las colas
        r.tailL += 0.30f; r.tailR += 0.26f;
        r.chestBend = 0.08f; r.chestYaw = 0.3f * p.yawSway; r.headYaw = 0.0f;
    }
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
        r.chestYaw = 0.05f * sinf(idleT * 0.7f); r.chestBend = -0.08f; r.chestRoll = 0.0f; r.headYaw = 0.0f;
    }
    if (aiming) {                                          // APUNTAR: la PISTOLA (mano izq) al frente
        // de perfil: el hombro de la pistola adelante (el pecho mira 0.2 a la derecha; el brazo se
        // abre esos 0.2 para seguir apuntando al frente) y la cabeza vuelve al blanco
        r.chestYaw = -0.20f + 0.3f * r.chestYaw; r.chestRoll *= 0.5f; r.headYaw = 0.18f;
        r.armL = 1.42f; r.elbowL = 0.05f; r.outL = 0.10f;
        r.armR = 0.30f; r.elbowR = 0.50f; r.outR = 0.22f;   // el sable queda abajo, listo
        r.head = -0.05f;
    }
    if (meleeT > 0.0f) {                                   // TAJO (combo de Zero): rapido al principio
        const float k = 1.0f - meleeT, e = 1.0f - (1.0f - k) * (1.0f - k) * (1.0f - k);
        if (g_hunterSlashType == 0)      { r.armR = 1.25f; r.outR = 1.30f - 2.10f * e; r.elbowR = 0.15f; }   // de derecha a izquierda
        else if (g_hunterSlashType == 1) { r.armR = 1.25f; r.outR = -0.80f + 2.10f * e; r.elbowR = 0.15f; }  // de vuelta
        else                             { r.armR = -2.60f + 3.80f * e; r.outR = 0.20f; r.elbowR = 0.25f; }   // por encima de la cabeza
        r.armL = 0.20f; r.outL = 0.35f;                       // el otro brazo se abre para equilibrar
        // el tajo sale del CUERPO: el pecho se carga hacia el lado de donde viene la hoja y se
        // descarga hacia el otro (en el de arriba se arquea y se cierra); la cabeza sigue al blanco
        if (g_hunterSlashType == 0)      { r.chestYaw = -0.45f + 0.95f * e; r.chestBend = 0.06f; }
        else if (g_hunterSlashType == 1) { r.chestYaw =  0.50f - 0.95f * e; r.chestBend = 0.06f; }
        else                             { r.chestYaw = 0.12f; r.chestBend = -0.22f + 0.62f * e; }
        r.headYaw = -0.6f * r.chestYaw; r.chestRoll *= 0.3f;
    }
    hunterSecondary(r, p, spd01, grounded);                // abrigo y cabeza con inercia; inclinacion al acelerar
    if (landK > 0.0f) {                                    // ATERRIZAJE: se agacha y amortigua
        r.legL += 0.35f * landK; r.legR += 0.35f * landK;
        r.kneeL += 0.95f * landK; r.kneeR += 0.95f * landK;
        r.armL += 0.25f * landK; r.armR += 0.25f * landK;
        r.outL += 0.25f * landK; r.outR += 0.25f * landK;
        r.tailL += 0.15f * landK; r.tailR += 0.15f * landK;
        r.flapL += 0.20f * landK; r.flapR += 0.20f * landK;
        p.bobY -= 0.16f * landK;                           // baja el centro de masa
        p.lean += 0.22f * landK;
        r.chestBend += 0.20f * landK;                      // y el pecho se cierra con el golpe
    }
    r.head -= 0.7f * r.chestBend;                          // la cabeza no se va con el pecho: sigue mirando al frente
    // MUNECA de la pistola: el canon gira con todo lo que hay encima (cuerpo, pecho, hombro, codo).
    // Apuntando se compensa ENTERO (el canon queda horizontal, al frente); si no, a medias (la mano
    // acompana el braceo sin que la pistola mire al cielo).
    {
        const float up = r.armL + r.elbowL + r.chestBend - (p.lean + 0.35f * p.headNod);
        r.wristL = aiming ? -up : -0.6f * up;
    }

    // ---- TOBILLOS (al final: dependen del angulo REAL de cada pierna en el mundo) ----
    {
        const float lean = p.lean + 0.35f * p.headNod;     // lo mismo que inclina hunterPitchRad
        const float wLL = r.legL - lean, wLR = r.legR - lean;
        const float flatL = -(wLL - r.kneeL), flatR = -(wLR - r.kneeR);
        if (floating) { r.ankleL = -0.40f; r.ankleR = -0.35f; }
        else if (!grounded) { r.ankleL = -0.18f; r.ankleR = -0.30f; }
        else {
            const float TAU_ = 6.2831853f;
            float prL = fmodf(walkPhase, TAU_), prR = fmodf(walkPhase + 3.14159265f, TAU_);
            if (prL < 0.0f) prL += TAU_; if (prR < 0.0f) prR += TAU_;
            const float aL = rgAnkle(prL, wLL - r.kneeL, wLL, kRun), aR = rgAnkle(prR, wLR - r.kneeR, wLR, kRun);
            const float m = rgMix(gw, 1.0f, kRun);         // quieto: los dos planos
            r.ankleL = rgMix(flatL, aL, m); r.ankleR = rgMix(flatR, aR, m);
            if (landK > 0.0f) { r.ankleL = rgMix(r.ankleL, flatL, landK); r.ankleR = rgMix(r.ankleR, flatR, landK); }
        }
        r.ankleL = rgClampF(r.ankleL, -0.90f, 0.90f); r.ankleR = rgClampF(r.ankleR, -0.90f, 0.90f);
    }

    // ---- SUAVIZADO: paso-bajo sobre la pose ENTERA (rig + cuerpo). Lo que antes cambiaba de un frame
    // al otro (apuntar, el tajo, despegar, aterrizar, pasar a correr) ahora se funde en 3-5 frames.
    // En el tajo va mas rapido para que el golpe no pierda filo. La primera llamada copia sin filtrar.
    {
        static HunterRig s_r; static float s_p[6]; static int s_init = 0;
        const float a = (meleeT > 0.0f) ? 0.80f : 0.55f + 0.15f * kRun;
        float pv[6] = { p.bobY, p.bobX, p.lean, p.roll, p.yawSway, p.headNod };
        float *cur = reinterpret_cast<float *>(&r), *prv = reinterpret_cast<float *>(&s_r);
        const int n = (int)(sizeof(HunterRig) / sizeof(float));
        if (!s_init) { s_r = r; for (int k = 0; k < 6; ++k) s_p[k] = pv[k]; s_init = 1; }
        for (int k = 0; k < n; ++k) { prv[k] += (cur[k] - prv[k]) * a; cur[k] = prv[k]; }
        for (int k = 0; k < 6; ++k) s_p[k] += (pv[k] - s_p[k]) * a;
        p.bobY = s_p[0]; p.bobX = s_p[1]; p.lean = s_p[2]; p.roll = s_p[3]; p.yawSway = s_p[4]; p.headNod = s_p[5];
    }
    return r;
}
