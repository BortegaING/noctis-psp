#pragma once
// PROJECT NOCTIS - WEAPONS_GEO: MODELO PROPIO PARA LAS 10 ARMAS CUERPO A CUERPO.
// =====================================================================
// Se incluye en main.cpp DESPUES de: struct LineVertex; RGBA(); brighten();
// addSolidBox(); addPyramid();  y DESPUES de #include "char_prims.h"
// (usa addLimb / addBall / cp_scale / cp_lerp de char_prims.h).
// Culling OFF en el pase del personaje => el winding de addLimb no importa.
//
// FILOSOFIA: nada de "cuadrados grises". Cada hoja es una CADENA de cilindros
// conicos (addLimb con sides=4 => seccion de DIAMANTE, que es la seccion real
// de una hoja: puntas en +-X = filo y lomo, caras planas hacia +-Z). Encadenando
// 3-4 tramos con el eje desplazado se lee la CURVA (katana, guadana). Las cajas
// solo aparecen en detalles chicos (tsuba cuadrada, carcasa, dientes de sierra).
//
// ---------------------------------------------------------------------
//  CONTRATO DE ESPACIO LOCAL (para quien la cablee a la mano del personaje)
// ---------------------------------------------------------------------
//   ORIGEN (0,0,0) = centro del puno (donde cierra la mano).
//   -Y  = hacia la PUNTA (la hoja baja).      +Y = pomo / regaton (0.30..0.74).
//   -X  = FILO (borde cortante).              +X = LOMO (canto romo).
//   +-Z = CARAS PLANAS de la hoja (espesor); |z| <= 0.13 en todas.
//   Unidades = las del personaje (char_prims.h: humano ~3.4-3.8 de alto).
//   El largo sale de kMelee[idx].reach (decimetros * 0.12) y el grosor de
//   kMelee[idx].weight => un arma pesada SE VE maciza y una rapida SE VE fina.
//   Extension (y minimo / x minimo) por arma, ver la tabla al final del archivo.
//   Para ponerla en la mano: trasladar el origen al centro del puno y rotar
//   (no hace falta escalar: ya viene en unidades de personaje).
// =====================================================================

#include <math.h>
#include "game_data.h"   // kMelee[]: nombre, dano, speedMs, reach, weight

// Presupuesto por arma (la mas cara emite 408). Buffer que debe reservar quien
// la dibuje; si se juntan varias, multiplicar.
#define MELEE_MAX_VERTS 448

// ---------------------------------------------------------------------
// wg_chain: encadena tramos de hoja. pts = {x,y,z,r} * n (n>=2).
// El color se interpola de colB (primer punto) a colT (ultimo) a lo largo de
// la cadena => filo/punta clara, base oscura. Coste: (n-1) * sides * 12 verts.
// ---------------------------------------------------------------------
static void wg_chain(LineVertex *buf, int &i, const float *pts, int n, int sides,
                     unsigned int colB, unsigned int colT)
{
    if (n < 2) return;
    for (int k = 0; k + 1 < n; ++k) {
        const float *a = pts + k * 4;
        const float *b = pts + (k + 1) * 4;
        float t0 = (float)k / (float)(n - 1);
        float t1 = (float)(k + 1) / (float)(n - 1);
        addLimb(buf, i, a[0], a[1], a[2], b[0], b[1], b[2], a[3], b[3], sides,
                cp_lerp(colB, colT, t0), cp_lerp(colB, colT, t1));
    }
}

// =====================================================================
// buildMeleeWeapon: emite el modelo del arma melee 'idx' (0..9) de kMelee[]
// en coordenadas LOCALES (ver contrato arriba). 'tint' = color base del metal:
// el filo se aclara y el lomo se oscurece a partir de el.
// Devuelve el numero de vertices escritos.
// =====================================================================
static int buildMeleeWeapon(LineVertex *buf, int idx, unsigned int tint)
{
    int i = 0;
    if (idx < 0) idx = 0;
    if (idx >= kMeleeCount) idx = kMeleeCount - 1;

    // ---- LA ESTADISTICA MANDA LA FORMA -------------------------------
    const float L = (float)kMelee[idx].reach  * 0.12f;          // largo util de la hoja/asta
    const float W = 0.045f + (float)kMelee[idx].weight * 0.008f; // media-anchura (peso => masa)

    // ---- paleta derivada de 'tint' -----------------------------------
    const unsigned int EDGE  = cp_scale(tint, 1.40f);            // filo (lo mas claro)
    const unsigned int STEEL = tint;                             // cuerpo de la hoja
    const unsigned int SPINE = cp_scale(tint, 0.58f);            // lomo / guarniciones
    const unsigned int DARKM = cp_scale(tint, 0.34f);            // sombra profunda del metal
    const unsigned int GRIP  = RGBA(34, 28, 32, 255);            // cuero/envoltura oscura
    const unsigned int WRAP  = RGBA(58, 44, 40, 255);            // madera/correa
    const unsigned int GOLD  = RGBA(172, 136, 64, 255);          // oro viejo (ancestral)
    const unsigned int GOLDD = cp_scale(GOLD, 0.60f);
    const unsigned int GLOW  = cp_lerp(tint, RGBA(150, 226, 255, 255), 0.85f); // nucleo energetico
    const unsigned int GLOWD = cp_scale(GLOW, 0.62f);
    const unsigned int GRAV  = cp_lerp(tint, RGBA(150,  86, 224, 255), 0.82f); // violeta gravitacional

    switch (idx) {

    // =================================================================
    // 0 - KATANA (dmg 28, 260ms, reach 12, peso 3)
    //     Fina y CURVA, tsuba REDONDA, tsuka larga envuelta. Silueta de sable.
    // =================================================================
    case 0: {
        addLimb(buf, i, 0.00f, 0.30f, 0.01f,  0.00f, -0.02f, 0.00f,
                0.046f, 0.044f, 6, WRAP, GRIP);                      // tsuka (72)
        addLimb(buf, i, 0.00f, 0.30f, 0.01f,  0.00f,  0.36f, 0.012f,
                0.052f, 0.040f, 4, DARKM, SPINE);                    // kashira/pomo (48)
        addLimb(buf, i, 0.00f, -0.06f, -0.045f, 0.00f, -0.06f, 0.045f,
                0.125f, 0.125f, 8, SPINE, STEEL);                    // TSUBA redonda (96)
        // hoja: 4 tramos, cada punto corrido hacia -X => el sori (curva) se lee
        const float b[] = {
            0.000f,        -0.06f,          0.0f, W * 1.15f,
           -L * 0.020f,    -0.06f - L*0.28f, 0.0f, W * 1.10f,
           -L * 0.060f,    -0.06f - L*0.56f, 0.0f, W * 0.98f,
           -L * 0.130f,    -0.06f - L*0.82f, 0.0f, W * 0.76f,
           -L * 0.230f,    -0.06f - L*1.00f, 0.0f, 0.005f,
        };
        wg_chain(buf, i, b, 5, 4, STEEL, EDGE);                      // (192)
        break;                                                        // = 408
    }

    // =================================================================
    // 1 - KATANA PESADA (dmg 48, 460ms, reach 13, peso 6)
    //     Misma familia pero MACIZA: hoja el doble de gruesa, curva mas suave,
    //     tsuba CUADRADA, empunadura mas larga y un lomo reforzado.
    // =================================================================
    case 1: {
        addLimb(buf, i, 0.00f, 0.40f, 0.01f,  0.00f, -0.02f, 0.00f,
                0.060f, 0.056f, 6, WRAP, GRIP);                      // tsuka larga (72)
        addLimb(buf, i, 0.00f, 0.40f, 0.01f,  0.00f,  0.47f, 0.012f,
                0.066f, 0.046f, 4, DARKM, SPINE);                    // pomo (48)
        addSolidBox(buf, i, 0.00f, -0.13f, 0.00f, 0.30f, 0.11f, 0.055f, SPINE); // tsuba cuadrada (30)
        const float b[] = {
            0.000f,        -0.10f,           0.0f, W * 1.12f,
           -L * 0.015f,    -0.10f - L*0.30f, 0.0f, W * 1.06f,
           -L * 0.045f,    -0.10f - L*0.58f, 0.0f, W * 0.96f,
           -L * 0.100f,    -0.10f - L*0.84f, 0.0f, W * 0.76f,
           -L * 0.180f,    -0.10f - L*1.00f, 0.0f, 0.006f,
        };
        wg_chain(buf, i, b, 5, 4, STEEL, EDGE);                      // hoja gruesa (192)
        // lomo reforzado: SIGUE la curva (si no, queda flotando al lado)
        addLimb(buf, i,  W * 0.70f, -0.16f, 0.0f,  -L * 0.085f + W * 0.42f, -0.10f - L*0.78f, 0.0f,
                0.034f, 0.024f, 4, DARKM, SPINE);                    // (48)
        break;                                                        // = 390
    }

    // =================================================================
    // 2 - ESPADA ENERGETICA (dmg 34, 220ms = LA MAS RAPIDA, reach 11, peso 2)
    //     Casi sin masa: mango corto, anillo emisor y una hoja de NUCLEO
    //     brillante con dos flancos de halo. Nada de metal ancho.
    // =================================================================
    case 2: {
        addLimb(buf, i, 0.00f, 0.24f, 0.00f,  0.00f, -0.05f, 0.00f,
                0.046f, 0.058f, 6, DARKM, SPINE);                    // mango/emisor (72)
        addLimb(buf, i, 0.00f, 0.24f, 0.00f,  0.00f,  0.31f, 0.00f,
                0.050f, 0.030f, 4, DARKM, STEEL);                    // tapa (48)
        addLimb(buf, i, 0.00f, -0.08f, -0.040f, 0.00f, -0.08f, 0.040f,
                0.085f, 0.085f, 6, STEEL, EDGE);                     // anillo emisor (72)
        addBall(buf, i, 0.00f, -0.13f, 0.00f, 0.070f, 0.060f, 0.070f, 2, 6, GLOW); // bloom (72)
        addLimb(buf, i, 0.00f, -0.10f, 0.00f,  0.00f, -0.10f - L, 0.00f,
                W * 0.90f, 0.012f, 4, GLOW, EDGE);                   // NUCLEO (48)
        addLimb(buf, i, -W * 0.85f, -0.14f, 0.0f, -W * 0.45f, -0.10f - L*0.94f, 0.0f,
                W * 0.50f, 0.008f, 4, GLOWD, GLOW);                  // halo filo (48)
        addLimb(buf, i,  W * 0.85f, -0.14f, 0.0f,  W * 0.45f, -0.10f - L*0.94f, 0.0f,
                W * 0.50f, 0.008f, 4, GLOWD, GLOW);                  // halo lomo (48)
        break;                                                        // = 408
    }

    // =================================================================
    // 3 - HOJA GRAVITACIONAL (dmg 52, 520ms, reach 15, peso 5)
    //     Hoja PARTIDA en 3 tramos que flotan separados, unidos por nodos
    //     violeta; orbe gravitacional en la guarda. Larga y pesada.
    // =================================================================
    case 3: {
        addLimb(buf, i, 0.00f, 0.28f, 0.00f,  0.00f, -0.06f, 0.00f,
                0.050f, 0.048f, 6, GRIP, DARKM);                     // mango (72)
        addLimb(buf, i, 0.00f, 0.28f, 0.00f,  0.00f,  0.35f, 0.00f,
                0.056f, 0.032f, 4, DARKM, GRAV);                     // pomo (48)
        addBall(buf, i, 0.00f, -0.10f, 0.00f, 0.100f, 0.085f, 0.100f, 2, 6, GRAV); // orbe (72)
        addLimb(buf, i, 0.00f, -0.22f, 0.0f, 0.00f, -0.22f - L*0.30f, 0.0f,
                W * 1.12f, W * 1.02f, 4, STEEL, EDGE);               // tramo 1 (48)
        addLimb(buf, i, 0.00f, -0.22f - L*0.34f, 0.0f, 0.00f, -0.22f - L*0.64f, 0.0f,
                W * 0.98f, W * 0.80f, 4, STEEL, EDGE);               // tramo 2 (48)
        addLimb(buf, i, 0.00f, -0.22f - L*0.68f, 0.0f, 0.00f, -0.22f - L*0.95f, 0.0f,
                W * 0.74f, 0.005f, 4, STEEL, EDGE);                  // tramo 3 / punta (48)
        addLimb(buf, i, 0.00f, -0.22f - L*0.32f, -0.05f, 0.00f, -0.22f - L*0.32f, 0.05f,
                0.032f, 0.032f, 3, GRAV, GRAV);                      // nodo de union 1 (36)
        addLimb(buf, i, 0.00f, -0.22f - L*0.66f, -0.05f, 0.00f, -0.22f - L*0.66f, 0.05f,
                0.028f, 0.028f, 3, GRAV, GRAV);                      // nodo de union 2 (36)
        break;                                                        // = 408
    }

    // =================================================================
    // 4 - ESPADA INDUSTRIAL (dmg 60, 640ms, reach 11 = CORTA, peso 8)
    //     Tosca y DENTADA: cuchilla ancha y corta (cleaver), guarda-bloque,
    //     contrapeso cuadrado y 5 dientes de sierra en el filo.
    // =================================================================
    case 4: {
        addLimb(buf, i, 0.00f, 0.34f, 0.00f,  0.00f, -0.06f, 0.00f,
                0.075f, 0.070f, 6, GRIP, WRAP);                      // barra de agarre (72)
        addSolidBox(buf, i, 0.00f,  0.34f, 0.00f, 0.13f, 0.13f, 0.10f, DARKM); // contrapeso (30)
        addSolidBox(buf, i, 0.00f, -0.20f, 0.00f, 0.30f, 0.14f, 0.07f, SPINE); // guarda-bloque (30)
        addLimb(buf, i, 0.00f, -0.14f, 0.0f, 0.00f, -0.14f - L*0.92f, 0.0f,
                W * 1.05f, W * 0.95f, 4, STEEL, SPINE);              // cuerpo del cleaver (48)
        addLimb(buf, i, -W * 0.85f, -0.16f, 0.0f, -W * 0.95f, -0.14f - L*0.92f, 0.0f,
                W * 0.55f, W * 0.50f, 4, EDGE, EDGE);                // FILO claro (48)
        addLimb(buf, i,  W * 0.80f, -0.16f, 0.0f,  W * 0.85f, -0.14f - L*0.90f, 0.0f,
                W * 0.50f, W * 0.45f, 4, DARKM, SPINE);              // lomo oscuro (48)
        addLimb(buf, i, 0.00f, -0.14f - L*0.92f, 0.0f, -W * 0.50f, -0.14f - L*1.06f, 0.0f,
                W * 0.95f, 0.020f, 4, STEEL, EDGE);                  // punta de cincel (48)
        for (int t = 0; t < 5; ++t) {                                 // dientes (5 * 12 = 60)
            float ty = -0.28f - L * 0.15f * (float)t;
            addPyramid(buf, i, -W * 1.45f, ty, 0.00f, 0.075f, 0.055f, 0.105f, SPINE);
        }
        break;                                                        // = 384
    }

    // =================================================================
    // 5 - LANZA (dmg 32, 380ms, reach 22 = EL MAYOR ALCANCE, peso 4)
    //     Asta larguisima y delgada (la punta queda a ~2.5 bajo la mano),
    //     regaton arriba, envoltura de agarre y PUNTA DE HOJA aleonada.
    // =================================================================
    case 5: {
        const float R = W * 0.52f;                                    // radio del asta
        addLimb(buf, i, 0.00f, 0.62f, 0.00f,  0.00f, 0.74f, 0.00f,
                R * 1.10f, R * 0.40f, 4, DARKM, SPINE);              // regaton (48)
        addLimb(buf, i, 0.00f, 0.62f, 0.00f,  0.00f, -0.02f, 0.00f,
                R, R * 1.05f, 6, WRAP, GRIP);                        // asta alta (72)
        addLimb(buf, i, 0.00f, 0.10f, 0.00f,  0.00f, -0.30f, 0.00f,
                R * 1.35f, R * 1.30f, 5, GRIP, DARKM);               // envoltura de agarre (60)
        addLimb(buf, i, 0.00f, -0.02f, 0.00f, 0.00f, -L * 0.66f, 0.00f,
                R * 1.05f, R * 0.95f, 6, WRAP, DARKM);               // asta baja (72)
        addLimb(buf, i, 0.00f, -L * 0.66f, 0.00f, 0.00f, -L * 0.71f, 0.00f,
                R * 1.55f, R * 1.25f, 5, SPINE, STEEL);              // cuello/socket (60)
        addLimb(buf, i, 0.00f, -L * 0.705f, 0.00f, 0.00f, -L * 0.78f, 0.00f,
                R * 1.20f, W * 1.05f, 4, STEEL, EDGE);               // hoja: ensancha (48)
        addLimb(buf, i, 0.00f, -L * 0.78f, 0.00f, 0.00f, -L * 0.955f, 0.00f,
                W * 1.05f, 0.004f, 4, EDGE, STEEL);                  // hoja: punta (48)
        break;                                                        // = 408
    }

    // =================================================================
    // 6 - GUADANA (dmg 50, 540ms, reach 19, peso 5)
    //     Mango largo con una leve quiebra + HOJA EN ANGULO que sale del pie
    //     del mango hacia -X y se curva hacia arriba (3 tramos encadenados).
    // =================================================================
    case 6: {
        const float R = W * 0.55f;
        addLimb(buf, i, 0.00f, 0.58f, 0.00f,  0.00f, 0.70f, 0.00f,
                R * 1.05f, R * 0.45f, 4, DARKM, SPINE);              // contrapeso (48)
        addLimb(buf, i, 0.00f, 0.58f, 0.02f,  0.00f, -0.50f, 0.00f,
                R, R, 6, WRAP, GRIP);                                // asta alta (72)
        addLimb(buf, i, 0.00f, -0.50f, 0.00f, -0.05f, -L * 0.71f, 0.02f,
                R, R * 0.92f, 6, GRIP, WRAP);                        // asta baja (quiebra) (72)
        addLimb(buf, i, -0.05f, -L * 0.71f, 0.02f, -0.07f, -L * 0.775f, 0.02f,
                R * 1.55f, R * 1.20f, 5, SPINE, STEEL);              // abrazadera (60)
        const float b[] = {
           -0.07f,        -L * 0.775f, 0.02f, W * 0.95f,
           -L * 0.200f,   -L * 0.815f, 0.03f, W * 0.78f,
           -L * 0.375f,   -L * 0.765f, 0.03f, W * 0.52f,
           -L * 0.492f,   -L * 0.632f, 0.03f, 0.004f,
        };
        wg_chain(buf, i, b, 4, 4, STEEL, EDGE);                      // hoja en angulo (144)
        break;                                                        // = 396
    }

    // =================================================================
    // 7 - ESPADA DE DOS MANOS (dmg 74, 820ms = LA MAS LENTA, reach 17, peso 9)
    //     ANCHA: tres cilindros paralelos (filo claro | vaceo oscuro | lomo)
    //     forman una hoja plana y maciza; GUARDA LARGA recta y pomo de bola.
    // =================================================================
    case 7: {
        addLimb(buf, i, 0.00f, 0.55f, 0.00f,  0.00f, -0.08f, 0.00f,
                0.048f, 0.052f, 6, WRAP, GRIP);                      // empunadura 2 manos (72)
        addBall(buf, i, 0.00f, 0.60f, 0.00f, 0.085f, 0.075f, 0.085f, 2, 6, SPINE); // pomo (72)
        addLimb(buf, i, -0.46f, -0.16f, 0.00f,  0.46f, -0.16f, 0.00f,
                0.042f, 0.042f, 5, SPINE, STEEL);                    // guarda larga (60)
        addLimb(buf, i, -W * 0.62f, -0.18f, 0.0f, -W * 0.55f, -0.18f - L*0.80f, 0.0f,
                W * 0.52f, W * 0.42f, 4, EDGE, EDGE);                // FILO (48)
        addLimb(buf, i,  W * 0.62f, -0.18f, 0.0f,  W * 0.55f, -0.18f - L*0.80f, 0.0f,
                W * 0.52f, W * 0.42f, 4, SPINE, SPINE);              // lomo (48)
        addLimb(buf, i,  0.00f, -0.18f, 0.0f, 0.00f, -0.18f - L*0.80f, 0.0f,
                W * 0.66f, W * 0.55f, 4, DARKM, STEEL);              // vaceo central (48)
        addLimb(buf, i,  0.00f, -0.18f - L*0.80f, 0.0f, 0.00f, -0.18f - L*1.02f, 0.0f,
                W * 0.95f, 0.005f, 4, STEEL, EDGE);                  // punta (48)
        break;                                                        // = 396
    }

    // =================================================================
    // 8 - HOJA EXPERIMENTAL (dmg 44, 340ms, reach 13, peso 3)
    //     Prototipo ASIMETRICO: hoja fina desviada hacia +X, carcasa tecnica
    //     con modulo lateral, riel secundario teal y travesano de union.
    // =================================================================
    case 8: {
        addLimb(buf, i, 0.00f, 0.26f, 0.00f,  0.00f, -0.04f, 0.00f,
                0.042f, 0.046f, 6, GRIP, DARKM);                     // mango (72)
        addSolidBox(buf, i,  0.00f, -0.22f, 0.00f, 0.16f, 0.12f, 0.12f, SPINE); // carcasa (30)
        addSolidBox(buf, i, -0.14f, -0.19f, 0.00f, 0.09f, 0.09f, 0.09f, DARKM); // modulo (30)
        addBall(buf, i, 0.00f, -0.08f, 0.02f, 0.050f, 0.045f, 0.050f, 2, 5, GLOW); // celda (60)
        const float b[] = {
            0.010f, -0.16f,            0.0f, W * 1.15f,
            0.040f, -0.16f - L*0.55f,  0.0f, W * 0.90f,
            0.090f, -0.16f - L*1.00f,  0.0f, 0.004f,
        };
        wg_chain(buf, i, b, 3, 4, STEEL, EDGE);                      // hoja desviada (96)
        addLimb(buf, i, -0.11f, -0.22f, 0.0f, -0.07f, -0.16f - L*0.62f, 0.0f,
                0.028f, 0.014f, 4, GLOWD, GLOW);                     // riel secundario (48)
        addLimb(buf, i, -0.10f, -0.16f - L*0.28f, 0.0f, 0.020f, -0.16f - L*0.30f, 0.0f,
                0.022f, 0.022f, 3, SPINE, STEEL);                    // travesano (36)
        break;                                                        // = 372
    }

    // =================================================================
    // 9 - ARMA ANCESTRAL (dmg 88 = EL MAYOR, 700ms, reach 16, peso 8)
    //     Reliquia ORNAMENTADA: pomo enjoyado, guarda de ALAS curvadas hacia
    //     arriba, hoja ancha de dos tramos y una incrustacion de oro en la cara.
    // =================================================================
    default: {
        addLimb(buf, i, 0.00f, 0.42f, 0.00f,  0.00f, -0.08f, 0.00f,
                0.054f, 0.050f, 6, WRAP, GOLDD);                     // empunadura (72)
        addBall(buf, i, 0.00f, 0.47f, 0.00f, 0.085f, 0.090f, 0.085f, 3, 5, GOLD); // pomo joya (90)
        addLimb(buf, i, -0.02f, -0.12f, 0.00f, -0.36f, 0.06f, 0.00f,
                0.050f, 0.014f, 4, GOLD, GOLDD);                     // ala izq (48)
        addLimb(buf, i,  0.02f, -0.12f, 0.00f,  0.36f, 0.06f, 0.00f,
                0.050f, 0.014f, 4, GOLD, GOLDD);                     // ala der (48)
        addLimb(buf, i, 0.00f, -0.16f, 0.00f, 0.00f, -0.16f - L*0.62f, 0.00f,
                W * 1.14f, W * 0.94f, 4, STEEL, STEEL);              // hoja tramo 1 (48)
        addLimb(buf, i, 0.00f, -0.16f - L*0.62f, 0.00f, 0.00f, -0.16f - L*1.00f, 0.00f,
                W * 0.94f, 0.005f, 4, STEEL, EDGE);                  // hoja tramo 2 (48)
        addLimb(buf, i, 0.00f, -0.24f, W * 0.72f, 0.00f, -0.16f - L*0.60f, W * 0.60f,
                0.020f, 0.016f, 4, GOLD, GOLDD);                     // incrustacion (48)
        break;                                                        // = 402
    }
    }

    return i;
}

// =====================================================================
//  TABLA DE REFERENCIA (extension aproximada en espacio local, unidades de
//  personaje; el eje +Y es el pomo, -Y la punta, -X el filo):
//    idx  arma                 y max   y min   x min   x max   verts
//     0   Katana                0.36   -1.50   -0.33   +0.13    408
//     1   Katana Pesada         0.47   -1.66   -0.28   +0.15    390
//     2   Espada Energetica     0.31   -1.42   -0.09   +0.09    408
//     3   Hoja Gravitacional    0.35   -1.93   -0.10   +0.10    408
//     4   Espada Industrial     0.44   -1.54   -0.21   +0.15    384
//     5   Lanza                 0.74   -2.52   -0.09   +0.09    408
//     6   Guadana               0.70   -1.86   -1.13   +0.06    396
//     7   Espada de Dos Manos   0.69   -2.26   -0.46   +0.46    396
//     8   Hoja Experimental     0.26   -1.72   -0.16   +0.16    372
//     9   Arma Ancestral        0.56   -2.08   -0.36   +0.36    402
//  Ninguna supera 448 verts (MELEE_MAX_VERTS) ni |z| = 0.13.
// =====================================================================
