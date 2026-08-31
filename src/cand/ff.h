#pragma once
// PROJECT NOCTIS - candidato "FF": heroe alto y dinamico estilo Final Fantasy PSP
// (Cloud / Crisis Core). Pelo puntiagudo dramatico, UNA hombrera sobredimensionada
// a la izquierda, mandoble buster punta-abajo en la mano derecha. NADA de cubos:
// torso, miembros, cuello, cabeza y pelo son cilindros conicos / elipsoides / loft.
// Pies en y=0, centrado x=0 z=0, mira hacia -Z. Culling off => winding no importa.
// Presupuesto medido: 3180 verts (limite 3200). addSolidBox: 4 usos (<=5).
static int build_ff(LineVertex *buf)
{
    int i = 0;

    // ---- paleta (ropa oscura, acero frio, acentos rojos) ----
    const unsigned int cloth   = RGBA(28, 28, 36, 255);      // tela top oscuro
    const unsigned int clothDk = cp_scale(cloth, 0.70f);     // sombra de la tela
    const unsigned int steel   = RGBA(150, 165, 195, 255);   // acero (hombrera/pomo)
    const unsigned int steelHi = brighten(steel, 1.20f);     // filo/reborde brillante
    const unsigned int blade   = RGBA(175, 185, 205, 255);   // filo de la hoja
    const unsigned int red     = RGBA(150, 44, 48, 255);     // acentos (cinturon/correa)
    const unsigned int skin    = RGBA(150, 134, 122, 255);   // cara / brazos desnudos
    const unsigned int hair    = RGBA(150, 140, 90, 255);    // rubio ceniza
    const unsigned int leather = RGBA(38, 30, 26, 255);      // botas / guantes / empuñadura

    // =================== PIERNAS (contrapposto: peso en la izq) ===================
    // Pierna izquierda: PLANTADA y recta (soporta el peso).
    addLimb(buf, i, -0.17f, 1.74f,  0.00f, -0.19f, 0.98f, -0.02f, 0.18f, 0.12f, 7, clothDk, cloth); // muslo
    addLimb(buf, i, -0.19f, 0.98f, -0.02f, -0.18f, 0.12f,  0.00f, 0.12f, 0.08f, 7, cloth,   cloth); // pantorrilla
    addLimb(buf, i, -0.18f, 0.12f,  0.00f, -0.18f, 0.06f, -0.32f, 0.11f, 0.06f, 7, leather, leather); // bota/pie
    // Pierna derecha: RELAJADA, rodilla flexionada y pie adelantado (dinamismo).
    addLimb(buf, i,  0.18f, 1.72f,  0.02f,  0.22f, 1.02f, -0.10f, 0.18f, 0.12f, 7, clothDk, cloth); // muslo
    addLimb(buf, i,  0.22f, 1.02f, -0.10f,  0.21f, 0.14f, -0.06f, 0.12f, 0.08f, 7, cloth,   cloth); // pantorrilla
    addLimb(buf, i,  0.21f, 0.14f, -0.06f,  0.21f, 0.06f, -0.42f, 0.11f, 0.06f, 7, leather, leather); // bota/pie

    // =================== PELVIS + CINTURON ===================
    addLimb(buf, i, 0.00f, 1.72f, 0.00f, 0.00f, 2.10f, 0.00f, 0.31f, 0.27f, 8, clothDk, cloth); // pelvis
    addLimb(buf, i, 0.00f, 1.70f, 0.00f, 0.00f, 1.80f, 0.00f, 0.33f, 0.33f, 8, red, red);       // cinturon (banda roja)
    addSolidBox(buf, i, 0.00f, 1.70f, -0.30f, 0.12f, 0.05f, 0.08f, steel);                       // hebilla frontal

    // =================== TORSO (top ceñido sin mangas: loft de anillos) ===================
    {
        const float ys[4] = { 2.10f, 2.35f, 2.60f, 2.86f };
        const float rs[4] = { 0.27f, 0.35f, 0.37f, 0.31f };
        addLoft(buf, i, 0.00f, 0.00f, ys, rs, 4, 8, clothDk, cloth);
    }

    // =================== HOMBROS (con leve torsion para dinamismo) ===================
    // Hombro izq un poco al frente (-z), der un poco atras (+z).
    addLimb(buf, i, -0.46f, 2.86f, -0.03f, 0.46f, 2.84f, 0.05f, 0.15f, 0.15f, 8, cloth, cloth);
    addBall(buf, i, -0.47f, 2.84f, -0.02f, 0.15f, 0.15f, 0.15f, 3, 5, skin);  // deltoides (brazos desnudos)
    addBall(buf, i,  0.47f, 2.84f,  0.05f, 0.15f, 0.15f, 0.15f, 3, 5, skin);

    // =================== CUELLO + CABEZA ===================
    addLimb(buf, i, 0.00f, 3.02f, 0.01f, 0.00f, 3.16f, 0.02f, 0.10f, 0.09f, 6, skin, skin);
    addBall(buf, i, 0.00f, 3.40f, 0.02f, 0.20f, 0.25f, 0.22f, 5, 8, skin);

    // =================== PELO (firma): 12 puas conicas r1=0 abaniendo ARRIBA/ATRAS ===
    // Front (4, mas gruesas, sides4): caen hacia adelante-arriba, dramaticas.
    addLimb(buf, i, -0.05f, 3.58f, -0.08f, -0.10f, 4.05f, 0.02f, 0.06f, 0.0f, 4, cp_scale(hair,0.78f), cp_scale(hair,1.12f));
    addLimb(buf, i,  0.06f, 3.58f, -0.06f,  0.14f, 4.00f, 0.06f, 0.06f, 0.0f, 4, cp_scale(hair,0.82f), cp_scale(hair,1.15f));
    addLimb(buf, i, -0.14f, 3.55f,  0.00f, -0.26f, 3.92f, 0.10f, 0.06f, 0.0f, 4, cp_scale(hair,0.72f), cp_scale(hair,1.05f));
    addLimb(buf, i,  0.15f, 3.55f,  0.00f,  0.28f, 3.95f, 0.08f, 0.06f, 0.0f, 4, cp_scale(hair,0.75f), cp_scale(hair,1.10f)); // asimetrica (mas larga)
    // Back/side (8, sides3): barren hacia atras (+z), en capas.
    addLimb(buf, i,  0.00f, 3.60f, 0.02f,  0.02f, 4.08f, 0.18f, 0.055f, 0.0f, 3, cp_scale(hair,0.80f), cp_scale(hair,1.13f)); // central alta
    addLimb(buf, i, -0.10f, 3.56f, 0.08f, -0.16f, 3.90f, 0.30f, 0.05f,  0.0f, 3, cp_scale(hair,0.70f), cp_scale(hair,1.00f));
    addLimb(buf, i,  0.10f, 3.56f, 0.08f,  0.18f, 3.94f, 0.28f, 0.05f,  0.0f, 3, cp_scale(hair,0.74f), cp_scale(hair,1.04f));
    addLimb(buf, i, -0.18f, 3.50f, 0.10f, -0.30f, 3.78f, 0.34f, 0.05f,  0.0f, 3, cp_scale(hair,0.68f), cp_scale(hair,0.98f));
    addLimb(buf, i,  0.18f, 3.50f, 0.10f,  0.30f, 3.82f, 0.32f, 0.05f,  0.0f, 3, cp_scale(hair,0.72f), cp_scale(hair,1.02f));
    addLimb(buf, i,  0.00f, 3.52f, 0.14f,  0.06f, 3.86f, 0.40f, 0.05f,  0.0f, 3, cp_scale(hair,0.66f), cp_scale(hair,0.96f));
    addLimb(buf, i, -0.08f, 3.54f, 0.12f, -0.10f, 3.98f, 0.24f, 0.05f,  0.0f, 3, cp_scale(hair,0.78f), cp_scale(hair,1.08f));
    addLimb(buf, i,  0.08f, 3.54f, 0.12f,  0.12f, 4.02f, 0.22f, 0.05f,  0.0f, 3, cp_scale(hair,0.76f), cp_scale(hair,1.10f));

    // =================== HOMBRERA IZQUIERDA (firma): guarda sobredimensionada ======
    addBall(buf, i, -0.52f, 2.92f, 0.00f, 0.28f, 0.22f, 0.30f, 4, 7, steel);                 // placa principal
    addLimb(buf, i, -0.78f, 2.98f, 0.02f, -0.52f, 2.66f, 0.28f, 0.10f, 0.03f, 5, steelHi, steel); // labio curvo (frente)
    addLimb(buf, i, -0.74f, 2.86f, -0.20f, -0.50f, 2.68f, -0.02f, 0.09f, 0.03f, 5, steelHi, steel); // labio (atras)
    addSolidBox(buf, i, -0.52f, 2.86f, -0.02f, 0.10f, 0.06f, 0.06f, red);                     // correa/hebilla roja
    // Correa (baldric) cruzando el pecho desde el hombro DER (desnudo/con tira) a la cadera izq.
    addLimb(buf, i, 0.42f, 2.78f, -0.18f, -0.16f, 2.10f, -0.20f, 0.045f, 0.04f, 4, red, cp_scale(red,0.7f));

    // =================== BRAZOS (desnudos; guante de cuero en la mano) ===================
    // Derecho (empuña el mandoble): baja al costado.
    addLimb(buf, i, 0.47f, 2.84f, 0.05f, 0.54f, 2.18f, 0.06f, 0.13f, 0.10f, 6, skin, skin);   // brazo
    addLimb(buf, i, 0.54f, 2.18f, 0.06f, 0.58f, 1.56f, -0.02f, 0.10f, 0.08f, 6, skin, skin);  // antebrazo
    addBall(buf, i, 0.59f, 1.50f, -0.02f, 0.10f, 0.10f, 0.10f, 3, 5, leather);                // guante/puño
    // Izquierdo: relajado, ligeramente hacia atras (+z).
    addLimb(buf, i, -0.47f, 2.82f, -0.02f, -0.53f, 2.16f, 0.10f, 0.13f, 0.10f, 6, skin, skin); // brazo
    addLimb(buf, i, -0.53f, 2.16f, 0.10f, -0.51f, 1.52f, 0.16f, 0.10f, 0.08f, 6, skin, skin);  // antebrazo
    addBall(buf, i, -0.50f, 1.46f, 0.16f, 0.10f, 0.10f, 0.10f, 3, 5, leather);                 // guante/puño

    // =================== MANDOBLE BUSTER (punta abajo al costado derecho) ============
    addLimb(buf, i, 0.60f, 1.66f, -0.02f, 0.60f, 1.40f, -0.02f, 0.05f, 0.055f, 6, leather, leather); // empuñadura envuelta
    addBall(buf, i, 0.60f, 1.70f, -0.02f, 0.06f, 0.07f, 0.06f, 3, 4, steel);                          // pomo
    addLimb(buf, i, 0.44f, 1.40f, -0.05f, 0.78f, 1.40f, 0.01f, 0.05f, 0.035f, 5, steelHi, steel);     // cruz/guarda
    // Hoja: losa PLANA ancha (fina en z, ancha en x, larga en y) hasta y=-0.20.
    addSolidBox(buf, i, 0.61f, -0.20f, 0.00f, 0.26f, 0.06f, 1.58f, steel);   // hoja
    addSolidBox(buf, i, 0.72f, -0.16f, 0.00f, 0.05f, 0.065f, 1.48f, blade);  // filo exterior (mas claro)

    return i;
}
