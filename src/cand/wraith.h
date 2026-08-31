#pragma once
// PROJECT NOCTIS - "WRAITH": gaunt gothic wraith-knight, faceless helm w/ red visor,
// tattered flaring coat, thin longsword point-down. All organic prims (no cubes).
static int build_wraith(LineVertex *buf)
{
    int i = 0;

    // ---- darkest palette (only the visor truly glows) ----
    const unsigned int coat     = RGBA(16, 15, 20, 255);   // near-black coat cloth
    const unsigned int coatDk   = cp_scale(coat, 0.6f);    // deep shade / folds
    const unsigned int armor    = RGBA(30, 30, 40, 255);   // cold plate
    const unsigned int armorLit = brighten(armor, 1.4f);   // lit plate edge
    const unsigned int helmCol  = RGBA(20, 19, 26, 255);   // tall thin skull-helm
    const unsigned int steel    = RGBA(120, 130, 150, 255);// blade
    const unsigned int glowRed  = RGBA(200, 40, 40, 255);  // the ONE bright thing
    const unsigned int dimRed   = RGBA(110, 26, 30, 255);  // faint red glow accents

    // ===================== COAT (signature tattered flare) =====================
    // near-black loft: tight at collar, blooming to a wide ragged skirt.
    {
        float cy[5] = { 0.35f, 0.90f, 1.45f, 1.90f, 2.20f };
        float cr[5] = { 0.72f, 0.60f, 0.48f, 0.40f, 0.30f };
        addLoft(buf, i, 0.0f, 0.0f, cy, cr, 5, 9, coatDk, coat);
    }
    // 8 jagged tatters hanging off the hem, alternating length = ragged edge.
    // (dx,dz) = 8 directions around the circle; a couple are red-tipped.
    addLimb(buf, i,  0.64f, 0.50f,  0.00f,  0.86f, 0.00f,  0.00f, 0.06f, 0.0f, 3, coatDk, coat);
    addLimb(buf, i,  0.45f, 0.50f,  0.45f,  0.58f, 0.15f,  0.58f, 0.06f, 0.0f, 3, coatDk, dimRed);
    addLimb(buf, i,  0.00f, 0.50f,  0.64f,  0.00f, 0.02f,  0.86f, 0.06f, 0.0f, 3, coatDk, coat);
    addLimb(buf, i, -0.45f, 0.50f,  0.45f, -0.58f, 0.14f,  0.58f, 0.06f, 0.0f, 3, coatDk, coat);
    addLimb(buf, i, -0.64f, 0.50f,  0.00f, -0.86f, 0.00f,  0.00f, 0.06f, 0.0f, 3, coatDk, coat);
    addLimb(buf, i, -0.45f, 0.50f, -0.45f, -0.58f, 0.15f, -0.58f, 0.06f, 0.0f, 3, coatDk, dimRed);
    addLimb(buf, i,  0.00f, 0.50f, -0.64f,  0.00f, 0.02f, -0.86f, 0.06f, 0.0f, 3, coatDk, coat);
    addLimb(buf, i,  0.45f, 0.50f, -0.45f,  0.58f, 0.14f, -0.58f, 0.06f, 0.0f, 3, coatDk, coat);

    // ============================ LEGS (elongated) =============================
    // hip -> knee -> ankle -> toe, tapering; feet point forward (-Z).
    // left
    addLimb(buf, i, -0.16f, 1.72f,  0.00f, -0.18f, 0.94f, -0.03f, 0.15f, 0.10f, 7, armor, coatDk);
    addLimb(buf, i, -0.18f, 0.94f, -0.03f, -0.17f, 0.10f,  0.00f, 0.10f, 0.06f, 7, coatDk, armor);
    addLimb(buf, i, -0.17f, 0.10f,  0.00f, -0.17f, 0.04f, -0.30f, 0.09f, 0.05f, 5, coatDk, coatDk);
    // right
    addLimb(buf, i,  0.16f, 1.72f,  0.00f,  0.18f, 0.94f, -0.03f, 0.15f, 0.10f, 7, armor, coatDk);
    addLimb(buf, i,  0.18f, 0.94f, -0.03f,  0.17f, 0.10f,  0.00f, 0.10f, 0.06f, 7, coatDk, armor);
    addLimb(buf, i,  0.17f, 0.10f,  0.00f,  0.17f, 0.04f, -0.30f, 0.09f, 0.05f, 5, coatDk, coatDk);

    // ============================ PELVIS + TORSO ===============================
    addLimb(buf, i, 0.0f, 1.70f, 0.0f, 0.0f, 2.10f, 0.0f, 0.27f, 0.24f, 9, coatDk, armor);
    {   // narrow armored ribcage
        float ty[4] = { 2.10f, 2.40f, 2.65f, 2.88f };
        float tr[4] = { 0.24f, 0.31f, 0.33f, 0.28f };
        addLoft(buf, i, 0.0f, 0.0f, ty, tr, 4, 9, armor, armorLit);
    }
    // thin horizontal rib plates (short wide rings hugging the torso)
    addLimb(buf, i, 0.0f, 2.28f, 0.01f, 0.0f, 2.31f, 0.01f, 0.33f, 0.33f, 6, armorLit, armorLit);
    addLimb(buf, i, 0.0f, 2.48f, 0.01f, 0.0f, 2.51f, 0.01f, 0.34f, 0.34f, 6, armorLit, armorLit);
    addLimb(buf, i, 0.0f, 2.70f, 0.01f, 0.0f, 2.73f, 0.01f, 0.30f, 0.30f, 6, armorLit, armorLit);

    // ============================ SHOULDERS + NECK =============================
    addLimb(buf, i, -0.42f, 2.88f, 0.0f, 0.42f, 2.88f, 0.0f, 0.13f, 0.13f, 8, armor, armor);
    addBall(buf, i, -0.44f, 2.84f, 0.0f, 0.15f, 0.11f, 0.17f, 4, 4, helmCol); // angular pauldron L
    addBall(buf, i,  0.44f, 2.84f, 0.0f, 0.15f, 0.11f, 0.17f, 4, 4, helmCol); // angular pauldron R
    addLimb(buf, i, 0.0f, 3.05f, 0.0f, 0.0f, 3.20f, -0.02f, 0.08f, 0.07f, 8, coatDk, armor); // thin neck (slight fwd tilt)

    // ============================ HELM (faceless) ==============================
    // tall thin elongated skull, tilted a touch forward (cz), near-black.
    addBall(buf, i, 0.0f, 3.45f, -0.03f, 0.16f, 0.30f, 0.20f, 5, 8, helmCol);
    // raised brow ridge: a thin arc across the front (-Z) face
    addLimb(buf, i, -0.14f, 3.40f, -0.16f, 0.14f, 3.40f, -0.16f, 0.03f, 0.03f, 6, armorLit, armorLit);
    // RED VISOR: single vertical slit on the front face -- the only bright thing
    addSolidBox(buf, i, 0.0f, 3.35f, -0.19f, 0.04f, 0.05f, 0.20f, glowRed);

    // ======================= ARMS (long, thin, hanging) ========================
    // shoulder -> elbow -> wrist; hands drop toward the sword in front.
    addLimb(buf, i, -0.44f, 2.86f, 0.0f, -0.48f, 2.10f, 0.02f, 0.10f, 0.08f, 6, armor, coatDk);
    addLimb(buf, i, -0.48f, 2.10f, 0.02f, -0.46f, 1.42f, -0.02f, 0.08f, 0.06f, 6, coatDk, armor);
    addLimb(buf, i,  0.44f, 2.86f, 0.0f,  0.48f, 2.10f, 0.02f, 0.10f, 0.08f, 6, armor, coatDk);
    addLimb(buf, i,  0.48f, 2.10f, 0.02f,  0.46f, 1.42f, -0.02f, 0.08f, 0.06f, 6, coatDk, armor);
    addBall(buf, i, -0.46f, 1.40f, -0.03f, 0.08f, 0.09f, 0.08f, 3, 4, helmCol); // loose hand L
    addBall(buf, i,  0.46f, 1.40f, -0.03f, 0.08f, 0.09f, 0.08f, 3, 4, helmCol); // loose hand R

    // ===================== LONGSWORD (vertical, point-down) ====================
    addLimb(buf, i, 0.10f, 1.50f, -0.05f, 0.10f, 1.70f, -0.05f, 0.05f, 0.05f, 6, coatDk, armor); // hilt
    addLimb(buf, i, -0.08f, 1.50f, -0.05f, 0.28f, 1.50f, -0.05f, 0.04f, 0.04f, 6, steel, steel); // crossguard
    // thin blade dropping from the guard to below the feet
    addSolidBox(buf, i, 0.10f, -0.10f, -0.050f, 0.06f, 0.04f, 1.60f, steel);
    // faint red fuller line down the blade
    addSolidBox(buf, i, 0.10f, -0.05f, -0.056f, 0.02f, 0.03f, 1.45f, dimRed);

    return i;
}
