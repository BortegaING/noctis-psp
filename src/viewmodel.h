#pragma once
// PROJECT NOCTIS - VIEWMODEL (arma en primera persona, VIEW SPACE).
// Se incluye en main.cpp DESPUES de: struct LineVertex; RGBA(); brighten();
// addSolidBox(); addPyramid();  y DESPUES de #include "char_prims.h"
// (usa addLimb / addBall de char_prims.h para las formas redondeadas).
// Culling OFF (main.cpp:1270) => el winding no importa, todo se ve.
//
// =====================================================================
//  ARMA: PISTOLA DE CAZADOR estilo pedernal (flintlock) - Bloodborne/Dracula.
//  Antigua, elegante, legible en low-poly. 11 PARTES DISTINTAS (no un bloque):
//    canon octagonal | boca acampanada (laton) | recamara/frame |
//    empunadura de madera curva (2 tramos) | pomo de laton |
//    martillo (cuerpo) | espolon del martillo | guardamonte en V (2 tramos) |
//    gatillo.  Paleta: gunmetal oscuro + laton envejecido + madera oscura.
//
// =====================================================================
//  ###  ESPEC DE SENSACION DE MOVIMIENTO (para main.cpp)  ###
//  Objetivo: reemplazar el andar "tosco". Todo a 60 fps, factores de lerp
//  POR FRAME (exponential smoothing). "clamp(v,a,b)" satura; "lerp(a,b,t)=a+(b-a)*t".
//
//  1) VELOCIDAD (suavizado de aceleracion)
//     const float WALK_SPEED = 0.090f;   // u/frame  (~5.4 u/s)
//     const float RUN_SPEED  = 0.150f;   // u/frame  (~9.0 u/s) con boton correr
//     const float ACCEL      = 0.16f;    // lerp hacia la velocidad deseada
//     const float STOP_FRIC  = 0.20f;    // lerp hacia 0 cuando no hay input
//     inputDir = normalize(stick) * (mag>DEADZONE?curva:0);  DEADZONE=0.16
//        target = inputDir * (running?RUN_SPEED:WALK_SPEED);
//        vel    = vel + (target - vel) * ACCEL;              // == lerp(vel,target,ACCEL)
//        if (sinInput) vel -= vel * STOP_FRIC;               // frena sin tiron
//        if (|vel| < 1e-4f) vel = 0;                         // mata deriva flotante
//     (Colision por ejes separados, como ya hace main.cpp: desliza por muros.)
//
//  2) MIRADA (nub analogico, suavizada)  -- pitch/yaw en radianes
//     const float TURN_YAW_MAX   = 0.045f;  // rad/frame a tope (~155 deg/s)
//     const float TURN_PITCH_MAX = 0.032f;  // rad/frame a tope (~110 deg/s)
//     const float LOOK_SMOOTH    = 0.25f;   // lerp del rate (quita el jitter del nub)
//     const float PITCH_CLAMP    = 1.30f;   // +-74 deg (nunca gimbal)
//        lx = deadzone((Rx-128)/128, 0.16);  ly = deadzone((Ry-128)/128, 0.16);
//        rawYaw   = sign(lx)*powf(fabsf(lx),1.5f) * TURN_YAW_MAX;    // curva de respuesta
//        rawPitch = sign(ly)*powf(fabsf(ly),1.5f) * TURN_PITCH_MAX;
//        yawRateS   += (rawYaw   - yawRateS)   * LOOK_SMOOTH;        // rate suavizado
//        pitchRateS += (rawPitch - pitchRateS) * LOOK_SMOOTH;
//        yaw += yawRateS;  pitch = clamp(pitch + pitchRateS, -PITCH_CLAMP, PITCH_CLAMP);
//
//  3) HEAD-BOB (atado a la velocidad horizontal) y ACOPLE del arma
//     const float BOB_STEP  = 11.0f;   // fase por unidad recorrida (cadencia de paso)
//     const float BOB_AMP_Y = 0.028f;  // amplitud vertical del bob
//     const float BOB_AMP_X = 0.018f;  // vaiven horizontal (medio ciclo => figura-8)
//        spd = sqrtf(velX*velX + velZ*velZ);   t = clamp(spd/WALK_SPEED, 0, 1);
//        bobPhase += spd * BOB_STEP;           // avanza con la DISTANCIA, no el tiempo
//        bobY = sinf(bobPhase)       * BOB_AMP_Y * t;
//        bobX = cosf(bobPhase*0.5f)  * BOB_AMP_X * t;
//     ACOPLE al viewmodel (lo que se pasa a buildViewmodel):
//        // velRight = componente de la velocidad hacia +X de la vista (strafe)
//        swayTarget = clamp(-yawRateS*1.4f - velRight*0.9f + bobX*0.6f, -0.05f, 0.05f);
//        bobTarget  = clamp( bobY - pitchRateS*0.20f,                   -0.03f, 0.03f);
//        swaySmooth += (swayTarget - swaySmooth) * 0.15f;   // 4) EASING anti-jitter
//        bobSmooth  += (bobTarget  - bobSmooth ) * 0.18f;
//        buildViewmodel(g_vm, swaySmooth, bobSmooth);       // <- llamada por frame
//     El arma LAGGEA la mirada (giras izq -> el canon queda a la der) y flota con
//     el bob; al frenar, swaySmooth/bobSmooth decaen a 0 solos (sin tiron).
// =====================================================================

#include <math.h>

// Presupuesto de vertices del viewmodel (buffer que main.cpp debe reservar).
#define VIEWMODEL_MAX_VERTS 512   // el modelo emite 498; deja holgura.

// Emite la pistola alrededor del ORIGEN en VIEW SPACE.
//   Ejes autor: +X derecha, +Y arriba, +Z HACIA LA ESCENA (adelante).
//   'sway' ~[-0.05,0.05] = yaw pequeno + deslizamiento horizontal.
//   'bob'  ~[-0.03,0.03] = desplazamiento vertical.
// Devuelve el numero de vertices escritos en 'buf'.
static int buildViewmodel(LineVertex *buf, float sway, float bob)
{
    int i = 0;

    // ---- paleta (antigua, elegante) ----
    const unsigned int GUNMETAL = RGBA(48, 46, 52, 255);
    const unsigned int GUNMETAL_HI = brighten(GUNMETAL, 1.15f);
    const unsigned int STEEL    = RGBA(78, 76, 84, 255);   // martillo/gatillo
    const unsigned int BRASS    = RGBA(150, 120, 60, 255); // laton envejecido
    const unsigned int BRASS_HI = RGBA(186, 150, 86, 255);
    const unsigned int WOOD     = RGBA(60, 42, 30, 255);   // nogal oscuro
    const unsigned int WOOD_HI  = RGBA(84, 60, 42, 255);

    // ---- pose: yaw por 'sway' alrededor del origen + slide + bob vertical ----
    const float cs = cosf(sway), sn = sinf(sway);
    const float offx = sway * 0.45f;   // deslizamiento horizontal sutil
    const float offy = bob;            // flotacion vertical

    // transforma un punto autor -> view space (lambdas: sin heap, deterministas)
    auto TF = [&](float x, float y, float z, float &ox, float &oy, float &oz) {
        ox = cs * x + sn * z + offx;
        oy = y + offy;
        oz = -sn * x + cs * z;
    };
    // cilindro conico (canon, empunadura, guardamonte, espolon)
    auto LIMB = [&](float x0, float y0, float z0, float x1, float y1, float z1,
                    float r0, float r1, int sides, unsigned int cB, unsigned int cT) {
        float a, b, c, d, e, f;
        TF(x0, y0, z0, a, b, c);
        TF(x1, y1, z1, d, e, f);
        addLimb(buf, i, a, b, c, d, e, f, r0, r1, sides, cB, cT);
    };
    // caja (recamara, martillo, gatillo). cx,cy,cz = CENTRO. Caras alineadas a
    // ejes; con sway pequeno el giro del centro basta (imperceptible en las caras).
    auto BOX = [&](float cx, float cy, float cz, float w, float d, float h, unsigned int col) {
        float ox, oy, oz;
        TF(cx, cy, cz, ox, oy, oz);
        addSolidBox(buf, i, ox, oy - h * 0.5f, oz, w, d, h, col);
    };
    // elipsoide (pomo)
    auto BALL = [&](float cx, float cy, float cz, float rx, float ry, float rz,
                    int st, int sl, unsigned int col) {
        float ox, oy, oz;
        TF(cx, cy, cz, ox, oy, oz);
        addBall(buf, i, ox, oy, oz, rx, ry, rz, st, sl, col);
    };

    // =================================================================
    //  Geometria. Todo en el cuadrante inferior-derecho (x+,y-), el canon
    //  se lanza ADELANTE (+Z) inclinado ~14 deg hacia el centro (-X) y ~9 deg
    //  arriba (+Y): la boca apunta al centro-frente de la pantalla.
    // =================================================================

    // (1) CANON OCTAGONAL - la firma del arma. 8 lados => octagono real. 96 v.
    LIMB(0.170f, -0.070f, 0.360f,   0.030f, 0.030f, 0.980f,
         0.036f, 0.028f, 8, GUNMETAL, GUNMETAL_HI);

    // (2) BOCA ACAMPANADA (laton) - corona flared en la punta, sobre el eje. 60 v.
    LIMB(0.030f, 0.030f, 0.980f,   0.0191f, 0.0378f, 1.0282f,
         0.030f, 0.042f, 5, BRASS, BRASS_HI);

    // (3) RECAMARA / FRAME (bloque central donde monta todo). 30 v.
    BOX(0.185f, -0.075f, 0.320f,   0.090f, 0.140f, 0.120f, GUNMETAL);

    // (4) EMPUNADURA DE MADERA, tramo bajo (hincha de palma). 60 v.
    LIMB(0.255f, -0.400f, 0.050f,   0.235f, -0.255f, 0.155f,
         0.055f, 0.048f, 5, WOOD, WOOD_HI);
    // (5) EMPUNADURA, tramo alto (dobla hacia el frame => curva). 48 v.
    LIMB(0.235f, -0.255f, 0.155f,   0.205f, -0.120f, 0.270f,
         0.048f, 0.040f, 4, WOOD_HI, WOOD);

    // (6) POMO de laton en el talon de la empunadura. 36 v.
    BALL(0.255f, -0.400f, 0.050f,   0.060f, 0.052f, 0.058f, 2, 3, BRASS);

    // (7) MARTILLO (cuerpo) sobre la recamara, lado del tirador. 30 v.
    BOX(0.205f, -0.005f, 0.285f,   0.030f, 0.055f, 0.060f, STEEL);
    // (8) ESPOLON del martillo (se enrosca arriba-atras hacia la camara). 36 v.
    LIMB(0.205f, 0.020f, 0.275f,   0.212f, 0.075f, 0.245f,
         0.016f, 0.010f, 3, STEEL, STEEL);

    // (9) GUARDAMONTE en V, tramo frontal (baja). 36 v.
    LIMB(0.185f, -0.140f, 0.375f,   0.200f, -0.215f, 0.340f,
         0.012f, 0.012f, 3, BRASS, BRASS);
    // (10) GUARDAMONTE, tramo trasero (sube). 36 v.
    LIMB(0.200f, -0.215f, 0.340f,   0.205f, -0.140f, 0.305f,
         0.012f, 0.012f, 3, BRASS, BRASS_HI);

    // (11) GATILLO colgando dentro del guardamonte. 30 v.
    BOX(0.196f, -0.175f, 0.335f,   0.012f, 0.028f, 0.045f, STEEL);

    return i;   // 498
}

// =====================================================================
//  NOTA FINAL
//  - Arma construida: PISTOLA DE CAZADOR de pedernal (flintlock), 11 partes:
//    canon octagonal, boca de laton, recamara, empunadura curva de madera,
//    pomo, martillo + espolon, guardamonte en V y gatillo.
//  - Vertices: 498 (buffer sugerido VIEWMODEL_MAX_VERTS = 512), GU_TRIANGLES.
//  - Ejes autor (main.cpp DEBE honrarlos al dibujar): +X derecha, +Y arriba,
//    +Z ADELANTE (hacia la escena); el modelo ocupa z ~ 0.05..1.03. Dibujar en
//    un pase propio: limpiar depth, perspectiva con near <= 0.05, VIEW = identidad
//    (arma pegada a la camara en el origen). OJO: el mundo mira -Z (GU estandar);
//    si el pase del viewmodel hereda ese eje, aplicar 180 deg en Y o escalar Z
//    por -1, o la boca apuntaria hacia atras. Culling OFF (winding indiferente).
//  - Movimiento: WALK 0.090 / RUN 0.150 u/f, ACCEL 0.16, mirada TURN_YAW 0.045
//    rad/f suavizada 0.25 (pitch clamp +-1.30), bob AMP_Y 0.028 atado a la
//    velocidad; sway/bob del arma = -yawRate*1.4 - strafe*0.9 (+bob), easing 0.15.
// =====================================================================
