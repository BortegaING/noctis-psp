#pragma once
// PROJECT NOCTIS - "AYA": slim gothic operative (Aya Brea / The 3rd Birthday vibe).
// Built from tapered cylinders (addLimb), faceted ellipsoids (addBall) and lofts.
// NO cubes for the body: only 2 tiny flat boxes for the blade guard + belt buckle.
// Feet at y=0, centered x=0 z=0, faces -Z (camera behind). Height ~3.60.
// Pose: weight on the left leg, right knee bent forward, slight forward lean,
// sword lowered in the right hand pointing down-and-out. Types/helpers are already
// defined before this header is included, so it takes no includes.
static int build_aya(LineVertex *buf)
{
    int i = 0;

    // ---- dark palette, a few blood-red accents ----
    const unsigned int coat   = RGBA(26, 24, 32, 255);   // fitted near-black coat
    const unsigned int coatDk = cp_scale(coat, 0.70f);   // shaded folds / hem
    const unsigned int coatLt = brighten(coat, 1.45f);   // lit bust / shoulder
    const unsigned int steel  = RGBA(120, 138, 175, 255);// blade
    const unsigned int steelLt= brighten(steel, 1.20f);
    const unsigned int red    = RGBA(150, 40, 45, 255);  // straps / belt
    const unsigned int redDk  = cp_scale(red, 0.70f);
    const unsigned int skin   = RGBA(150, 132, 122, 255);// face / neck / hands
    const unsigned int skinDk = cp_scale(skin, 0.82f);
    const unsigned int hair   = RGBA(24, 20, 26, 255);   // shoulder-length layers
    const unsigned int hairLt = brighten(hair, 1.70f);
    const unsigned int boot   = RGBA(30, 26, 24, 255);   // dark leather boots

    // ================= LEGS (contrapposto) ================================
    // Left leg = weight leg (nearly straight). Right leg = bent knee, foot back.
    // thigh
    addLimb(buf,i, -0.17f,1.74f,-0.02f,  -0.19f,0.98f,-0.02f, 0.17f,0.12f, 6, coat, coat);
    addLimb(buf,i,  0.17f,1.74f,-0.02f,   0.21f,1.04f,-0.16f, 0.17f,0.12f, 6, coat, coat);
    // calf
    addLimb(buf,i, -0.19f,0.98f,-0.02f,  -0.18f,0.14f,-0.02f, 0.12f,0.08f, 6, coat, coatDk);
    addLimb(buf,i,  0.21f,1.04f,-0.16f,   0.20f,0.14f, 0.04f, 0.12f,0.08f, 6, coat, coatDk);
    // foot (points -Z)
    addLimb(buf,i, -0.18f,0.14f,-0.02f,  -0.18f,0.05f,-0.32f, 0.10f,0.06f, 5, boot, boot);
    addLimb(buf,i,  0.20f,0.14f, 0.04f,   0.20f,0.06f,-0.26f, 0.10f,0.06f, 5, boot, boot);

    // ================= PELVIS -> TORSO (hourglass, slight fwd lean) ========
    addLimb(buf,i, 0.00f,1.72f,-0.02f,  0.00f,2.05f,-0.03f, 0.26f,0.24f, 8, coatDk, coat);
    // three lofted bands: waist -> bust -> shoulder taper (cz grows -Z = lean)
    addLimb(buf,i, 0.00f,2.05f,-0.03f,  0.00f,2.32f,-0.05f, 0.24f,0.33f, 8, coat,   coat);
    addLimb(buf,i, 0.00f,2.32f,-0.05f,  0.00f,2.58f,-0.06f, 0.33f,0.35f, 8, coat,   coatLt);
    addLimb(buf,i, 0.00f,2.58f,-0.06f,  0.00f,2.82f,-0.08f, 0.35f,0.28f, 8, coatLt, coat);

    // ================= SHOULDERS + NECK + HEAD ============================
    addLimb(buf,i, -0.42f,2.80f,-0.06f,  0.42f,2.78f,-0.08f, 0.13f,0.13f, 6, coat, coat);
    addLimb(buf,i,  0.00f,2.82f,-0.08f,  0.00f,3.12f,-0.11f, 0.10f,0.09f, 6, skin, skin);
    // egg head, taller than wide, faces -Z
    addBall(buf,i, 0.00f,3.34f,-0.13f, 0.20f,0.26f,0.22f, 6,9, skin);

    // ================= ARMS (relaxed at sides) ===========================
    // left arm (free)
    addLimb(buf,i, -0.42f,2.76f,-0.05f, -0.50f,2.16f, 0.00f, 0.12f,0.09f, 6, coat, coat);
    addLimb(buf,i, -0.50f,2.16f, 0.00f, -0.52f,1.58f,-0.06f, 0.09f,0.07f, 6, coat, coatDk);
    addBall(buf,i, -0.53f,1.53f,-0.06f, 0.08f,0.09f,0.07f, 3,4, skin);
    // right arm (sword hand)
    addLimb(buf,i,  0.42f,2.76f,-0.06f,  0.50f,2.14f, 0.02f, 0.12f,0.09f, 6, coat, coat);
    addLimb(buf,i,  0.50f,2.14f, 0.02f,  0.54f,1.56f,-0.02f, 0.09f,0.07f, 6, coat, coatDk);
    addBall(buf,i,  0.55f,1.51f,-0.02f, 0.08f,0.09f,0.07f, 3,4, skin);

    // ================= LONG COAT, OPEN AT THE FRONT ======================
    // No closed front ring: a flaring back drape + two side tails + two front
    // lapels leave a -Z gap so the body/legs show through (open coat).
    addLimb(buf,i,  0.00f,2.00f, 0.14f,  0.00f,0.45f, 0.26f, 0.22f,0.34f, 8, coat, coatDk); // back drape
    addLimb(buf,i, -0.26f,1.98f, 0.08f, -0.40f,0.48f, 0.20f, 0.16f,0.26f, 6, coat, coatDk); // left tail
    addLimb(buf,i,  0.26f,1.98f, 0.08f,  0.40f,0.48f, 0.20f, 0.16f,0.26f, 6, coat, coatDk); // right tail
    addLimb(buf,i, -0.18f,2.55f,-0.16f, -0.22f,1.35f,-0.14f, 0.09f,0.12f, 5, coat, coatDk); // left lapel
    addLimb(buf,i,  0.18f,2.55f,-0.16f,  0.22f,1.35f,-0.14f, 0.09f,0.12f, 5, coat, coatDk); // right lapel

    // ================= TACTICAL STRAPS + BELT (red accents) ==============
    addLimb(buf,i, 0.00f,1.90f,-0.03f, 0.00f,1.99f,-0.03f, 0.29f,0.29f, 8, red, redDk);     // belt band
    addLimb(buf,i, -0.20f,2.60f,-0.24f, 0.16f,2.02f,-0.22f, 0.035f,0.035f, 4, red, redDk);  // chest strap
    addSolidBox(buf,i, 0.00f,1.90f,-0.30f, 0.10f,0.05f,0.09f, steel);                       // buckle (flat prop)

    // ================= HAIR: back mass + 8 layered strands ================
    addBall(buf,i, 0.00f,3.36f,0.03f, 0.23f,0.22f,0.20f, 5,7, hair);                        // soft back mass
    // bangs framing the face (sweep toward -Z over the forehead)
    addLimb(buf,i,  0.00f,3.50f,-0.18f, 0.00f,3.24f,-0.32f, 0.06f,0.03f, 4, hairLt, hair);
    addLimb(buf,i, -0.10f,3.48f,-0.17f,-0.15f,3.22f,-0.29f, 0.05f,0.02f, 4, hairLt, hair);
    addLimb(buf,i,  0.10f,3.48f,-0.17f, 0.15f,3.22f,-0.29f, 0.05f,0.02f, 4, hairLt, hair);
    // side layers down to the shoulders
    addLimb(buf,i, -0.19f,3.46f,-0.10f,-0.26f,2.86f, 0.00f, 0.06f,0.02f, 4, hair, hair);
    addLimb(buf,i,  0.19f,3.46f,-0.10f, 0.26f,2.86f, 0.00f, 0.06f,0.02f, 4, hair, hair);
    // back layers falling behind
    addLimb(buf,i, -0.12f,3.52f, 0.06f,-0.16f,2.84f, 0.16f, 0.06f,0.02f, 4, hair, hair);
    addLimb(buf,i,  0.12f,3.52f, 0.06f, 0.16f,2.84f, 0.16f, 0.06f,0.02f, 4, hair, hair);
    addLimb(buf,i,  0.00f,3.54f, 0.08f, 0.02f,2.82f, 0.18f, 0.07f,0.03f, 4, hairLt, hair);

    // ================= SLIM BLADE (lowered, pointing down-and-out) ========
    addLimb(buf,i, 0.55f,1.64f,-0.02f, 0.56f,1.42f,0.00f, 0.035f,0.035f, 5, coatDk, coatDk); // grip
    addSolidBox(buf,i, 0.56f,1.40f,0.00f, 0.22f,0.06f,0.04f, steel);                         // crossguard (flat prop)
    addLimb(buf,i, 0.57f,1.40f,0.00f, 0.74f,0.28f,-0.05f, 0.055f,0.012f, 5, steelLt, steel); // blade

    return i;
}
