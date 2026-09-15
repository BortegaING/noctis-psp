#pragma once
// ============================================================================
// PROJECT NOCTIS - anim.h
// Ciclo de caminata PROCEDURAL del HUNTER. Funcion PURA: no guarda estado, no
// toca el GU, no pide memoria, no usa rand. Solo <math.h>. C++17.
//
// Que resuelve: hoy el personaje se mueve como un bloque (un sinf en Y y un
// balanceo y nada mas). Aca sale una POSE COMPLETA por frame (bob doble,
// inclinacion por velocidad, balanceo lateral hacia el pie apoyado, contrafase
// de piernas y brazos, rodillas que flexionan en vuelo y se estiran al apoyar,
// y un pico de impacto por pisada para sacudir la camara).
//
// El cableado en main.cpp esta al FINAL del archivo (receta exacta, con el
// orden de sceGumRotateXYZ / sceGumTranslate y las trampas de signo).
// ============================================================================

#include <math.h>

// ----------------------------------------------------------------------------
// SISTEMA DE EJES (el del modelo de src/cand/hunter.h, tal como se construye):
//   pies en y = 0, altura total ~3.5, el personaje MIRA HACIA -Z.
//   +X = su DERECHA      -X = su IZQUIERDA
//   +Y = arriba (contra la gravedad local, despues del alineado por gravG)
//   -Z = adelante        +Z = atras (hacia la camara de 3ra persona)
// ----------------------------------------------------------------------------

struct HunterPose {
    float bobY, bobX;        // desplazamiento del centro de masa (unidades de mundo)
                             //   bobY: + = sube    bobX: + = se corre a su DERECHA (+X)
    float lean;              // inclinacion hacia ADELANTE segun velocidad (rad, + = adelante)
    float roll;              // balanceo lateral (rad, + = el torso cae hacia su DERECHA)
    float yawSway;           // giro leve del torso (rad, MISMO sentido que heroYaw: se suma directo)
    float legL, legR;        // angulo de cada pierna en la cadera (rad, + = adelante)
    float kneeL, kneeR;      // flexion de rodilla (rad, SIEMPRE >= 0, + = talon hacia atras/arriba)
    float armL, armR;        // angulo de cada brazo en el hombro (rad, + = adelante)
    float headNod;           // cabeceo (rad, + = barbilla al pecho / mira abajo)
    float stepImpact;        // 0..1, pico corto al apoyar CADA pie (2 por ciclo) -> shake de camara
};

// El modelo de cand/hunter.h mide ~3.5 de alto y la pose sale calibrada para un
// personaje de 1.8. Multiplica SOLO bobX/bobY por esto al cablear (los angulos
// no se escalan nunca).
static const float HUNTER_BOB_SCALE = 1.90f;   // 3.5 / 1.8

// ---------------------------- helpers privados ------------------------------
static inline float hp_clamp01(float v) { return (v < 0.0f) ? 0.0f : ((v > 1.0f) ? 1.0f : v); }
static inline float hp_max0(float v)    { return (v < 0.0f) ? 0.0f : v; }
static inline float hp_smooth01(float t){ t = hp_clamp01(t); return t * t * (3.0f - 2.0f * t); }

// ============================================================================
//                            MAPA DE FASES
// ----------------------------------------------------------------------------
// p = walkPhase, en RADIANES. main.cpp suma 0.17 por frame mientras camina:
//   2*PI = 1 CICLO COMPLETO = 2 PASOS = ~37 frames = ~0.62 s a 60 fps.
//
//   p = 0        -> PASE IZQUIERDO: la pierna IZQ va en VUELO pasando bajo el
//                   cuerpo hacia adelante; la DER esta plantada en midstance.
//                   CUERPO ARRIBA (pico 1 de 2 de bobY). kneeL en su maximo.
//   p = PI/2     -> APOYO IZQUIERDO (heel strike izq): legL = +max (adelante),
//                   legR = -max (atras, despegando). CUERPO ABAJO (absorbe).
//                   kneeL ~ 0 (pierna estirada al plantar). stepImpact = 1.
//                   El peso cae sobre el pie IZQ -> bobX hacia -X, roll a la izq.
//   p = PI       -> PASE DERECHO: la pierna DER en VUELO bajo el cuerpo; la IZQ
//                   plantada en midstance. CUERPO ARRIBA (pico 2 de 2).
//                   kneeR en su maximo.
//   p = 3*PI/2   -> APOYO DERECHO: legR = +max, legL = -max. CUERPO ABAJO.
//                   kneeR ~ 0. stepImpact = 1. Peso sobre el pie DER (+X).
//   p = 2*PI     -> identico a p = 0 (ciclo cerrado, continuo en valor y derivada).
//
// Reglas del ciclo, resumidas:
//   - piernas en CONTRAFASE exacta:      legR(p) = legL(p + PI)
//   - brazo opuesto a la pierna del MISMO lado: armL(p) = -legL(p)*k
//   - el cuerpo sube DOS veces por ciclo: bobY ~ cos(2p) (arriba en los pases,
//     abajo en los dos apoyos) - NO una sola vez como el sinf(walkPhase) actual
//   - rodilla: flexiona en VUELO, se estira justo antes de apoyar, y vuelve a
//     ceder un poco al recibir el peso (absorcion)
//   - inclinacion adelante proporcional a la velocidad
// ============================================================================

// speed01: velocidad horizontal normalizada 0..1 (0 = quieto, 1 = RUN_SPEED).
// idleT:   reloj que SIEMPRE avanza (0.05/frame) -> vida en reposo.
// grounded: 0 = en el aire (el ciclo se congela y sale una pose de salto).
static HunterPose hunterPose(float walkPhase, float idleT, float speed01, int grounded)
{
    const float PI_  = 3.14159265f;
    const float HALF = 1.57079633f;

    const float s    = hp_clamp01(speed01);
    const float g    = hp_smooth01(s);     // peso del ciclo de marcha (arranque suave)
    const float idle = 1.0f - g;           // peso de la pose VIVA de reposo

    const float p  = walkPhase;
    const float sp = sinf(p);              // +1 en apoyo IZQ (PI/2), -1 en apoyo DER (3PI/2)
    const float c2 = cosf(2.0f * p);       // +1 en los pases (0, PI), -1 en los dos apoyos

    // ---- amplitudes (calibradas para un personaje de 1.8 de alto) ----
    const float BOB_Y = 0.055f;            // 5.5 cm de sube-baja por paso
    const float BOB_X = 0.030f;            // 3 cm de vaiven lateral
    const float LEAN  = 0.20f;             // ~11.5 grados de inclinacion a full velocidad
    const float ROLL  = 0.055f;            // ~3.2 grados de balanceo
    const float YAWSW = 0.10f;             // ~5.7 grados de contragiro del torso
    const float LEG   = 0.62f;             // ~35 grados de apertura por pierna
    const float KNEE  = 0.85f;             // ~49 grados de flexion maxima en vuelo
    const float ARM   = 0.45f;             // ~26 grados de braceo
    const float NOD   = 0.07f;             // ~4 grados de cabeceo

    HunterPose o;

    // ---------------------------------------------------------------- bobY --
    // DOS subidas por ciclo: maximo en los pases (p = 0 y p = PI, cuerpo alto
    // sobre la pierna de apoyo), minimo en los dos apoyos (PI/2, 3PI/2, la
    // rodilla amortigua). Eso es lo que hoy falta: el sinf(walkPhase) actual
    // sube UNA sola vez por ciclo y por eso se ve como un flan.
    const float breath = 0.014f * sinf(idleT * 0.60f);   // respiracion (~3.5 s por ciclo)
    o.bobY = BOB_Y * c2 * g + breath;                    // la respiracion nunca se apaga

    // ---------------------------------------------------------------- bobX --
    // El centro de masa cae sobre el pie apoyado. Apoyo IZQ en p = PI/2 -> el
    // cuerpo se corre a su IZQUIERDA (-X) -> signo negativo sobre sin(p).
    o.bobX = -BOB_X * sp * g + 0.010f * idle * sinf(idleT * 0.37f + 1.1f);

    // ---------------------------------------------------------------- lean --
    // Proporcional a la velocidad REAL (s, no g: queremos que crezca lineal con
    // el stick) + un micro-cabeceo del torso 2 veces por ciclo (el tronco se
    // adelanta al apoyar y se endereza al pasar) + deriva de respiracion.
    o.lean = LEAN * s
           - 0.020f * g * c2
           + 0.010f * idle * sinf(idleT * 0.60f + 3.1f);

    // ---------------------------------------------------------------- roll --
    // Cae hacia el pie apoyado: apoyo IZQ (p = PI/2) -> roll hacia su IZQUIERDA
    // (negativo, porque + = derecha). Una sola oscilacion por ciclo.
    o.roll = -ROLL * sp * g + 0.012f * idle * sinf(idleT * 0.31f);

    // ------------------------------------------------------------- yawSway --
    // Contragiro de hombros contra la pelvis, una vez por ciclo, en fase con la
    // pierna adelantada. Se SUMA a heroYaw (mismo sentido, sin conversiones).
    o.yawSway = YAWSW * sp * g + 0.020f * idle * sinf(idleT * 0.23f);

    // -------------------------------------------------------------- piernas --
    // CONTRAFASE exacta. legL = +max en p = PI/2 (apoyo izq, pierna adelante),
    // -max en p = 3PI/2 (despegue izq, pierna atras). legR = legL desfasado PI.
    // En reposo queda un escalonado minimo (un pie apenas adelantado) que
    // respira, para que no parezca un maniqui con los pies juntos.
    const float idleStance = 0.035f + 0.015f * sinf(idleT * 0.30f);
    o.legL =  LEG * sp * g + idle * idleStance;
    o.legR = -LEG * sp * g - idle * idleStance;

    // -------------------------------------------------------------- rodillas --
    // VUELO de la pierna IZQ: p de 3PI/2 -> PI/2 pasando por 0, que es justo
    // donde cosf(p) > 0. Entonces max(0, cos(p)) ya da "1 mientras vuela, 0
    // mientras apoya" (stance ~50% del ciclo, como en la marcha real).
    // El +0.45 corre el pico a p ~ 5.83 (poco DESPUES del despegue, flexion
    // maxima en vuelo temprano) y deja la rodilla ESTIRADA justo antes del
    // apoyo en p = PI/2. La rodilla derecha es lo mismo desfasado PI.
    const float swingL = hp_max0(cosf(p + 0.45f));
    const float swingR = hp_max0(cosf(p + 0.45f + PI_));

    // ABSORCION: pequena flexion extra justo DESPUES de plantar el pie
    // (centrada en PI/2 + 0.35 para la izq, 3PI/2 + 0.35 para la der).
    // Cubo = campana angosta, un golpecito corto.
    float loadL = hp_max0(cosf(p - (HALF + 0.35f)));        loadL = loadL * loadL * loadL;
    float loadR = hp_max0(cosf(p - (HALF + 0.35f + PI_)));  loadR = loadR * loadR * loadR;

    // En reposo las rodillas quedan levemente flexionadas (postura de guardia),
    // nunca en hiperextension. Garantia del contrato: ambos terminos son >= 0.
    o.kneeL = (KNEE * swingL + 0.22f * loadL) * g + idle * (0.07f + 0.010f * sinf(idleT * 0.44f + 0.5f));
    o.kneeR = (KNEE * swingR + 0.22f * loadR) * g + idle * (0.07f + 0.010f * sinf(idleT * 0.44f + 2.6f));

    // --------------------------------------------------------------- brazos --
    // Cada brazo va OPUESTO a la pierna del MISMO lado: en p = PI/2 la pierna
    // IZQ esta adelante -> el brazo IZQ va ATRAS (armL = -legL normalizado).
    o.armL = -ARM * sp * g + idle * 0.025f * sinf(idleT * 0.50f);
    o.armR =  ARM * sp * g + idle * 0.025f * sinf(idleT * 0.50f + 0.9f);

    // -------------------------------------------------------------- headNod --
    // La cabeza tiende a estabilizarse: cabecea CONTRA el bob, 2 veces por
    // ciclo, con ~0.6 rad de retardo (la cabeza llega tarde al movimiento).
    o.headNod = -NOD * cosf(2.0f * p - 0.6f) * g
              + idle * 0.030f * sinf(idleT * 0.60f + 2.2f);

    // ----------------------------------------------------------- stepImpact --
    // -cos(2p) vale exactamente +1 en p = PI/2 y p = 3PI/2, o sea en las DOS
    // pisadas, y es negativo (recortado a 0) el resto. El cubo lo vuelve un
    // pico corto en vez de una onda ancha. 0 en reposo, 0 en el aire.
    float k = -c2; if (k < 0.0f) k = 0.0f;
    o.stepImpact = hp_clamp01(k * k * k * g);

    // ------------------------------------------------------------- EN EL AIRE --
    // Sin pies en el suelo no hay ciclo: se congela todo y sale una pose de
    // salto/caida (piernas recogidas, brazos abiertos para equilibrar), con un
    // flutter lento de idleT para que no quede como una estatua volando.
    if (!grounded) {
        const float f = sinf(idleT * 1.70f);
        o.bobY       =  0.020f * f;
        o.bobX       =  0.015f * sinf(idleT * 1.10f);
        o.lean       =  0.10f + 0.10f * s + 0.03f * f;   // se echa adelante al saltar corriendo
        o.roll       =  0.030f * sinf(idleT * 0.90f);
        o.yawSway    =  0.030f * sinf(idleT * 0.70f);
        o.legL       =  0.30f + 0.05f * f;               // pierna guia recogida adelante
        o.legR       = -0.22f - 0.05f * f;               // pierna de atras colgando
        o.kneeL      =  0.85f + 0.08f * f;               // rodillas dobladas (recogidas)
        o.kneeR      =  0.45f + 0.06f * f;
        o.armL       =  0.55f;                           // brazos arriba/afuera
        o.armR       = -0.35f;
        o.headNod    = -0.05f;                           // mira apenas arriba
        o.stepImpact =  0.0f;                            // el impacto lo dispara el aterrizaje
    }

    return o;
}

// ----------------------------------------------------------------------------
// Helpers OPCIONALES de cableado: devuelven el angulo YA con el signo correcto
// para meterlo tal cual en el ScePspFVector3 de sceGumRotateXYZ, dado que el
// modelo de cand/hunter.h MIRA HACIA -Z. Si algun dia el modelo se da vuelta,
// se cambia el signo aca y en ningun otro lado.
//   pitch: rotar +X inclina la coronilla hacia +Z (= hacia ATRAS del hunter),
//          asi que para inclinarse ADELANTE hay que pasar el lean NEGADO.
//   roll:  rotar +Z sube el lado +X (= el cuerpo cae a su IZQUIERDA), asi que
//          para caer a la DERECHA hay que pasar el roll NEGADO.
// El headNod se suma al pitch con un factor chico: sin esqueleto, la cabeza no
// se puede mover sola (ver receta), asi que se usa como micro-pitch del cuerpo.
// ----------------------------------------------------------------------------
static inline float hunterPitchRad(const HunterPose &o) { return -(o.lean + 0.35f * o.headNod); }
static inline float hunterRollRad (const HunterPose &o) { return -o.roll; }

// ============================================================================
//                RECETA EXACTA DE CABLEADO EN main.cpp
// ============================================================================
//
// 0) INCLUDE
//    anim.h no depende de nada del motor (solo <math.h>): se puede incluir
//    donde sea. Sugerido junto a los otros headers del personaje, p.ej. despues
//    de #include "cand/hunter.h" (linea ~232):
//
//        #include "anim.h"   // ciclo de caminata procedural del hunter
//
// 1) DATOS DE ENTRADA (ya existen todos en main.cpp)
//    En el bloque de animacion (linea ~1197, "===== animacion (segun velocidad
//    real) =====") calcular la velocidad normalizada. Con gravedad NORMAL las
//    velocidades planas son velX/velZ; con gravedad cambiada (wall-walk) son
//    gvr/gvf. Conviene una sola variable declarada junto a walkPhase/idleT:
//
//        float speed01 = 0.0f;                      // junto a: float walkPhase = 0.0f, idleT = 0.0f;
//        ...
//        // gravedad normal:
//        speed01 = sqrtf(velX*velX + velZ*velZ) / RUN_SPEED;   // RUN_SPEED = 0.22f
//        // rama de gravedad no-abajo:
//        speed01 = sqrtf(gvr*gvr + gvf*gvf) / RUN_SPEED;
//        if (speed01 > 1.0f) speed01 = 1.0f;
//
//    OJO: walkPhase solo avanza si moving; eso esta bien y no hay que tocarlo.
//    La pose ya se apaga sola cuando speed01 -> 0 (queda la de reposo viva).
//
// 2) REEMPLAZO DEL BLOQUE DE DIBUJO (main.cpp ~1427-1441, "3RA PERSONA: el
//    HUNTER encara heroYaw"). El bloque entero queda asi:
//
//        {
//            HunterPose hp = hunterPose(walkPhase, idleT, speed01, grounded);
//            const float HIP = 1.70f;   // altura de cadera del modelo de cand/hunter.h
//
//            sceGumLoadIdentity();
//
//            // (a) posicion en el mundo (SIN el bob: el bob va en el frame del cuerpo)
//            ScePspFVector3 pp = { playerX, playerY, playerZ };
//            sceGumTranslate(&pp);
//
//            // (b) alineado con la gravedad local: IGUAL QUE HOY, y SIEMPRE PRIMERO,
//            //     asi toda la pose queda relativa al "suelo" actual (pared/techo).
//            ScePspFVector3 gm = { 0.0f, 0.0f, 0.0f };
//            if (gravG == 1) gm.z = 3.14159f; else if (gravG == 2) gm.z = -1.5708f;
//            else if (gravG == 3) gm.z = 1.5708f; else if (gravG == 4) gm.x = 1.5708f;
//            else if (gravG == 5) gm.x = -1.5708f;
//            sceGumRotateXYZ(&gm);
//
//            // (c) hacia donde encara + contragiro del torso (yawSway se SUMA con el
//            //     MISMO signo con el que hoy se pasa heroYaw: no hay conversion)
//            ScePspFVector3 fy = { 0.0f, heroYaw + hp.yawSway, 0.0f };
//            sceGumRotateXYZ(&fy);
//
//            // (d) bob del centro de masa, YA EN EL FRAME DEL CUERPO:
//            //     X = su derecha, Y = arriba de la gravedad local. Por eso va
//            //     DESPUES del yaw y no sumado a playerY como hoy.
//            ScePspFVector3 bob = { hp.bobX * HUNTER_BOB_SCALE, hp.bobY * HUNTER_BOB_SCALE, 0.0f };
//            sceGumTranslate(&bob);
//
//            // (e) inclinacion + balanceo PIVOTANDO EN LA CADERA (si se pivota en los
//            //     pies, con lean 0.2 la cabeza se va ~0.7 unidades y se ve raro)
//            ScePspFVector3 up   = { 0.0f,  HIP, 0.0f };  sceGumTranslate(&up);
//            ScePspFVector3 body = { hunterPitchRad(hp), 0.0f, hunterRollRad(hp) };
//            sceGumRotateXYZ(&body);
//            ScePspFVector3 dn   = { 0.0f, -HIP, 0.0f };  sceGumTranslate(&dn);
//
//            sceGumDrawArray(GU_TRIANGLES, LINE_FLAGS, g_heroV, 0, g_hero);
//        }
//
// 3) POR QUE ESE ORDEN (y no otro)
//    sceGumRotateXYZ(v) hace M = M * Rx(v.x) * Ry(v.y) * Rz(v.z), y sceGumTranslate
//    tambien multiplica POR LA DERECHA. O sea: la ULTIMA llamada es la mas LOCAL
//    (la primera que se aplica al vertice) y la PRIMERA es la mas global.
//    Por eso: mundo -> gravedad -> yaw -> bob del cuerpo -> pitch/roll del cuerpo.
//    Si el pitch se pasa en la MISMA llamada que el yaw (como hoy), el Rx queda
//    a la IZQUIERDA del Ry y termina inclinando alrededor del eje X DEL MUNDO,
//    no del personaje: caminando hacia +X el "lean" se ve como un vuelco lateral.
//    Por eso el yaw va en una llamada y el pitch/roll en OTRA, despues.
//
// 4) BUGS DE SIGNO QUE YA ESTAN EN EL CODIGO ACTUAL (aprovechar y corregir)
//    - `fr.x = 0.045f` cuando moving: rotar +X lleva la coronilla hacia +Z, que
//      para este modelo (mira -Z) es HACIA ATRAS. Hoy el hunter corre echado
//      para atras. Por eso hunterPitchRad() devuelve el lean NEGADO.
//    - `fr.z = sinf(walkPhase*0.5f)*0.03f`: media frecuencia -> el balanceo tarda
//      DOS ciclos completos en cerrar y se desincroniza de los pasos. El roll de
//      esta pose va a sinf(walkPhase), una oscilacion por ciclo, en fase con el
//      pie que apoya.
//    - main.cpp ya tiene floats locales llamados bobX/bobY (head-bob de 1ra
//      persona, hoy anulados con (void)). NO renombrar nada: usar siempre
//      hp.bobX / hp.bobY para no pisarlos.
//
// 5) stepImpact -> SACUDIDA DE CAMARA (esto es lo que mas "vende" el paso)
//    En el armado de la vista (main.cpp ~1337-1343), el ojo se calcula con
//    eye = player - fwd*9 + up*4.5. Guardar la pose del frame (o recalcularla:
//    es pura y baratisima) y hundir el ojo en el pico de cada pisada:
//
//        float shake = hp.stepImpact * 0.10f;   // 10 cm, subir/bajar a gusto
//        ScePspFVector3 eye = { playerX - fX*9.0f + uX*(4.5f - shake),
//                               playerY - fY*9.0f + uY*(4.5f - shake),
//                               playerZ - fZ*9.0f + uZ*(4.5f - shake) };
//
//    Dos golpes por ciclo, sincronizados con los dos pies. Si se quiere sonido
//    de pisada, el disparador es el flanco: guardar el stepImpact del frame
//    anterior y disparar cuando pasa de < 0.35 a >= 0.35.
//
// 6) HONESTIDAD SOBRE legL/legR/kneeL/kneeR
//    g_hero es UNA SOLA MALLA dibujada con UNA sola sceGumDrawArray. Con una
//    sola matriz NO existe forma de mover las piernas por separado: cualquier
//    rotacion que apliques mueve tambien el torso y la cabeza. Dicho derecho:
//    **legL/legR/kneeL/kneeR NO van a tener ningun efecto visual mientras el
//    hunter se dibuje de una sola pasada**. Se calculan igual porque no cuestan
//    nada y porque son la mitad de las dos alternativas de abajo.
//
//    ALTERNATIVA A (la recomendada, ~15 lineas y queda un ciclo de paso REAL):
//    partir el draw en tres, sin tocar la geometria. En src/cand/hunter.h las
//    PRIMERAS 3 llamadas addLimb son la pierna IZQUIERDA (muslo, pantorrilla,
//    bota) y las 3 siguientes la pierna DERECHA; todo lo demas (pelvis, torso,
//    cabeza, arma) viene despues. O sea el buffer YA esta ordenado como
//    [pierna izq | pierna der | resto]. Basta con que build_hunter publique dos
//    marcas (int v0 = i despues de la izq, int v1 = i despues de la der) y en
//    el dibujo hacer TRES sceGumDrawArray con la misma cadena de matrices de
//    (2) y, encima de cada pierna, un pivote en la cadera:
//
//        // pierna izquierda: rota legL en la cadera, y kneeL se aproxima
//        // acortando/inclinando el tramo (o se ignora y queda pierna rigida,
//        // que ya se ve 10 veces mejor que hoy)
//        push: translate {0, HIP, 0} -> rotateXYZ { -legL, 0, 0 } -> translate {0,-HIP,0}
//              drawArray(..., legLCount, 0, g_hero + 0)
//        idem con -legR y g_hero + v0 (count v1-v0)
//        y el resto del cuerpo con g_hero + v1 sin rotacion de pierna.
//
//    (el signo -legL es el mismo criterio que el pitch: + adelante = -X-rot,
//     porque el modelo mira -Z). Con solo eso ya hay zancada de verdad; las
//    rodillas requieren partir ademas muslo/pantorrilla y son el paso siguiente.
//
//    ALTERNATIVA B (si NO se quiere tocar hunter.h para nada): simular la
//    zancada con la unica matriz disponible, convirtiendo la apertura de
//    piernas en un BALANCEO PENDULAR del cuerpo entero sobre el pie apoyado.
//    Es el truco clasico de los muñecos de una pieza y funciona sorprendentemente
//    bien a 9 unidades de camara. En el paso (e), en vez de solo el lean:
//
//        float stride = (hp.legL - hp.legR) * 0.18f;   // +-0.22 rad a full
//        ScePspFVector3 body = { hunterPitchRad(hp) - stride, 0.0f, hunterRollRad(hp) };
//
//    y pivotando en el TOBILLO (HIP = 0.15f en vez de 1.70f) para ese termino:
//    el cuerpo cae adelante mientras "rueda" sobre el pie y se endereza en cada
//    pisada. Sumado al bobY de dos picos, al bobX hacia el pie apoyado, al roll
//    en fase y al shake de camara del stepImpact, el ojo lee CAMINATA aunque la
//    malla sea un bloque. Lo que nunca va a leer es la tijera de las piernas:
//    para eso, alternativa A.
//
// 7) AJUSTE FINO (las tres perillas que importan, en este orden)
//    - LEG/ARM: si se ve exagerado, bajar ARM primero (el braceo es lo que mas
//      canta en una malla rigida).
//    - BOB_Y: si "flota", bajar a 0.040f; si se ve pesado, subir a 0.070f.
//    - LEAN: 0.20 es bastante agresivo (estilo Bloodborne corriendo). Para un
//      caminar tranquilo, 0.12.
//    El modelo lleva un cleaver en la mano derecha: si el braceo derecho se ve
//    mal, en el cableado usar armR * 0.4f (brazo del arma casi quieto).
//
// ============================================================================
// NOTA FINAL (que devuelve cada campo, en que unidades, y en que orden se cablea)
// ----------------------------------------------------------------------------
// bobY/bobX: UNIDADES DE MUNDO para un personaje de 1.8 -> multiplicar por
//   HUNTER_BOB_SCALE (1.90) para este modelo; bobY + = sube, bobX + = a su derecha.
// lean/roll/yawSway/legL/legR/kneeL/kneeR/armL/armR/headNod: RADIANES. lean + =
//   adelante, roll + = cae a su derecha, yawSway se suma DIRECTO a heroYaw,
//   piernas/brazos + = adelante, rodillas siempre >= 0, headNod + = mira abajo.
// stepImpact: adimensional 0..1, pico corto en cada una de las dos pisadas del
//   ciclo (p = PI/2 y p = 3PI/2); usarlo para hundir la camara y disparar audio.
// ORDEN DE CABLEADO (multiplicacion por la derecha: lo ultimo es lo mas local):
//   translate(playerX,Y,Z) -> rotateXYZ(gravedad) -> rotateXYZ(0, heroYaw+yawSway, 0)
//   -> translate(bobX*S, bobY*S, 0) -> [translate(+HIP) -> rotateXYZ(pitch,0,roll)
//   -> translate(-HIP)] -> drawArray. Pitch y roll SIEMPRE en una llamada aparte
//   y POSTERIOR al yaw, o se inclinan respecto de los ejes del mundo.
// ============================================================================
