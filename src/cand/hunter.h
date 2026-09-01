#pragma once
// HUNTER candidate - hooded gothic stalker, hunched forward, sawtooth cleaver held point-down.
static int build_hunter(LineVertex *buf)
{
    int i = 0;

    // ---------------- dark gothic palette ----------------
    const unsigned int coat   = RGBA(20, 19, 25, 255);   // heavy near-black coat
    const unsigned int coatDk = cp_scale(coat, 0.65f);   // shaded folds / shadow side
    const unsigned int skirtDk= cp_scale(coat, 0.55f);   // deep hem shadow
    const unsigned int hood   = cp_scale(coat, 0.85f);   // hood / high collar (near-black)
    const unsigned int steel  = RGBA(110, 120, 140, 255);// cleaver blade
    const unsigned int red    = RGBA(140, 36, 40, 255);  // blood-red straps
    const unsigned int skin   = RGBA(120, 108, 102, 255);// gaunt, barely-lit skin
    const unsigned int pant   = RGBA(36, 32, 38, 255);   // pantalon (un pelo mas claro que el abrigo)
    const unsigned int pantDk = cp_scale(pant, 0.68f);   // lado en sombra
    const unsigned int boot   = RGBA(16, 15, 18, 255);   // bota, muy oscura

    // ================= LEGS (weight on the planted LEFT leg) =================
    // Piernas visibles (el abrigo llega a la rodilla, NO es falda).
    // Left leg: planted, near-vertical, foot stepped forward (-Z).
    addLimb(buf,i, -0.17f,1.70f, 0.00f, -0.19f,0.96f,-0.04f, 0.17f,0.12f, 8, pantDk, pant);  // thigh
    addLimb(buf,i, -0.19f,0.96f,-0.04f, -0.18f,0.12f, 0.00f, 0.135f,0.09f, 8, pantDk, pant); // calf
    addLimb(buf,i, -0.18f,0.12f, 0.00f, -0.18f,0.05f,-0.30f, 0.115f,0.06f, 6, boot, boot);   // boot
    // Right leg: trailing, knee bent, ankle pushed back (+Z) => off-balance stalk.
    addLimb(buf,i,  0.17f,1.70f, 0.04f,  0.22f,1.02f, 0.02f, 0.17f,0.12f, 8, pantDk, pant);  // thigh
    addLimb(buf,i,  0.22f,1.02f, 0.02f,  0.20f,0.18f, 0.16f, 0.135f,0.09f, 8, pantDk, pant); // calf
    addLimb(buf,i,  0.20f,0.18f, 0.16f,  0.20f,0.08f,-0.04f, 0.115f,0.06f, 6, boot, boot);   // boot

    // ================= PELVIS =================
    addLimb(buf,i, 0.00f,1.68f,0.02f, 0.00f,2.05f,0.00f, 0.30f,0.27f, 8, coatDk, coat);

    // ================= TORSO (manual bands, leaning -Z with height => HUNCH) =
    addLimb(buf,i, 0.00f,2.05f, 0.00f, 0.00f,2.35f,-0.04f, 0.28f,0.34f, 8, coatDk, coat);   // waist->lower chest
    addLimb(buf,i, 0.00f,2.35f,-0.04f, 0.00f,2.60f,-0.09f, 0.34f,0.34f, 8, coat,   coat);   // chest
    addLimb(buf,i, 0.00f,2.60f,-0.09f, 0.00f,2.80f,-0.15f, 0.34f,0.29f, 8, coat,   coatDk); // upper chest (hunched fwd)

    // ================= NECK + HIGH COLLAR =================
    addLimb(buf,i, 0.00f,2.80f,-0.15f, 0.00f,2.98f,-0.20f, 0.10f,0.09f, 6, coatDk, skin);   // gaunt neck, mostly hidden
    {
        const float cys[2] = { 2.70f, 2.98f };
        const float crs[2] = { 0.30f, 0.24f };
        addLoft(buf,i, 0.00f,-0.15f, cys, crs, 2, 8, coatDk, hood);  // collar standing around neck
    }

    // ================= HEAD + HOOD (lowered & forward, face in shadow) =======
    addBall(buf,i, 0.00f,3.24f,-0.22f, 0.19f,0.25f,0.21f, 4,6, skin);                // gaunt head under the hood
    addBall(buf,i, 0.00f,3.28f,-0.20f, 0.28f,0.30f,0.30f, 5,8, hood);                // hood mass OVER the head
    addLimb(buf,i, 0.00f,3.26f,-0.28f, 0.00f,3.06f,-0.50f, 0.16f,0.04f, 6, hood, hood);// forward hood peak (-Z)

    // ================= SHOULDERS (hunched, pushed -Z) =================
    addLimb(buf,i, -0.44f,2.78f,-0.15f, 0.44f,2.78f,-0.15f, 0.15f,0.15f, 7, coatDk, coat);

    // ================= ARMS =================
    // Left arm hangs, wrist slightly forward.
    addLimb(buf,i, -0.45f,2.74f,-0.14f, -0.52f,2.10f,-0.02f, 0.13f,0.09f, 7, coatDk, coat); // upper
    addLimb(buf,i, -0.52f,2.10f,-0.02f, -0.55f,1.52f,-0.06f, 0.09f,0.07f, 6, coatDk, coat); // forearm
    // Right arm lowered, holding the cleaver point-down and out.
    addLimb(buf,i,  0.45f,2.74f,-0.14f,  0.54f,2.08f,-0.02f, 0.13f,0.09f, 7, coatDk, coat); // upper
    addLimb(buf,i,  0.54f,2.08f,-0.02f,  0.60f,1.50f,-0.06f, 0.09f,0.07f, 6, coatDk, coat); // forearm

    // ================= RED CROSSED STRAPS (on the back, +Z, faces the camera) =
    addLimb(buf,i, -0.22f,2.55f,0.26f,  0.18f,1.75f,0.22f, 0.045f,0.045f, 3, red, red);
    addLimb(buf,i,  0.22f,2.55f,0.26f, -0.18f,1.75f,0.22f, 0.045f,0.045f, 3, red, red);

    // ================= COAT (largo hasta la RODILLA: deja ver las piernas) ====
    // Antes era un faldon hasta el piso; ahora es un abrigo acampanado que
    // termina sobre la rodilla, con las pantorrillas y botas a la vista.
    {
        const float sys[4] = { 1.15f, 1.45f, 1.75f, 2.05f };
        const float srs[4] = { 0.46f, 0.41f, 0.36f, 0.31f };
        addLoft(buf,i, 0.00f,-0.02f, sys, srs, 4, 10, skirtDk, coat);
    }
    // borde inferior irregular: jirones CORTOS colgando del dobladillo de rodilla
    addLimb(buf,i,  0.00f,1.16f, 0.44f,  0.05f,0.90f, 0.52f, 0.11f,0.0f, 4, skirtDk, coatDk); // atras centro
    addLimb(buf,i,  0.34f,1.16f, 0.30f,  0.40f,0.94f, 0.34f, 0.10f,0.0f, 4, skirtDk, coatDk); // atras der
    addLimb(buf,i, -0.34f,1.16f, 0.30f, -0.40f,0.94f, 0.34f, 0.10f,0.0f, 4, skirtDk, coatDk); // atras izq
    addLimb(buf,i,  0.44f,1.18f,-0.06f,  0.48f,0.98f,-0.06f, 0.09f,0.0f, 4, skirtDk, coatDk); // lado der
    addLimb(buf,i, -0.44f,1.18f,-0.06f, -0.48f,0.98f,-0.06f, 0.09f,0.0f, 4, skirtDk, coatDk); // lado izq

    // ================= CLEAVER (right hand, sawtooth, point-down) =============
    // ESPADA con FORMA: empunadura + guarda + hoja que se AFILA hacia la punta
    addLimb(buf,i, 0.60f,1.50f,-0.06f, 0.70f,1.28f,-0.11f, 0.05f,0.05f, 6, coatDk, coatDk); // empunadura
    addSolidBox(buf,i, 0.71f,1.24f,-0.11f, 0.30f,0.07f,0.09f, RGBA(120,100,60,255));         // guarda (cruz)
    addSolidBox(buf,i, 0.73f,0.80f,-0.115f, 0.30f,0.05f,0.46f, steel);                        // hoja ancha
    addSolidBox(buf,i, 0.74f,0.50f,-0.115f, 0.21f,0.05f,0.32f, steel);                        // se angosta
    addSolidBox(buf,i, 0.75f,0.28f,-0.115f, 0.12f,0.05f,0.24f, steel);                        // hacia la punta
    addSolidBox(buf,i, 0.755f,0.15f,-0.115f, 0.05f,0.05f,0.15f, brighten(steel,1.3f));        // PUNTA/filo
    // ================= BELT BUCKLE (tiny flat prop) =================
    addSolidBox(buf,i, 0.00f,1.66f,-0.30f, 0.14f,0.05f,0.08f, steel);

    return i;
}
