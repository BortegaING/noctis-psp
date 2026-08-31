#pragma once
// PROJECT NOCTIS - Player character (single fixed pose, detailed silhouette).
// Self-contained header: NO includes. Uses types/functions already defined in
// main.cpp BEFORE this header is included:
//   struct LineVertex; RGBA(r,g,b,a); brighten(c,f);
//   addSolidBox(buf,i, cx,baseY,cz, w,d,h, col);     // 30 verts each
//   addPyramid(buf,i, cx,baseY,cz, w,d, apexH, col); // 12 verts each
//
// Dark gothic warrior (Bloodborne-ish) seen from the BACK (faces -z), standing,
// SLIM and athletic. Limbs are split into tapering segments so contours read as
// smooth stepped curves instead of blocky cubes. Feet at y=0, centered x=0,z=0,
// total height ~3.6 (hair spikes rise a touch above). Long sword held pointing
// down at the right side.
//
// Budget: 51 solid boxes (51*30=1530) + 5 pyramids (5*12=60) = 1590 verts.

static void buildPlayerModel(LineVertex *buf, int &i)
{
    // ---- dark palette ----
    const unsigned int coat    = RGBA(30, 28, 36, 255);   // near-black coat/armor
    const unsigned int coatDk  = brighten(coat, 0.70f);   // shaded folds / cloth
    const unsigned int plate   = brighten(coat, 1.40f);   // lit armor plate
    const unsigned int red     = RGBA(150, 40, 45, 255);  // blood-red straps/trim
    const unsigned int redDk   = RGBA(95, 28, 32, 255);   // dark red tatters
    const unsigned int skin    = RGBA(150, 132, 122, 255);// neck / hands / face
    const unsigned int hair    = RGBA(20, 18, 26, 255);   // black spiky hair
    const unsigned int steel   = RGBA(120, 138, 175, 255);// blade
    const unsigned int leather = RGBA(30, 26, 24, 255);   // boots / gloves / hilt
    const unsigned int brass   = RGBA(120, 100, 60, 255); // tsuba guard

    // ================= LEGS (tapered: boot -> calf -> thigh) =================
    // left leg
    addSolidBox(buf, i, -0.24f, 0.00f, -0.08f, 0.28f, 0.50f, 0.22f, leather); // boot
    addSolidBox(buf, i, -0.24f, 0.22f,  0.00f, 0.24f, 0.28f, 0.40f, coat);    // lower calf
    addSolidBox(buf, i, -0.24f, 0.60f,  0.00f, 0.26f, 0.30f, 0.42f, coatDk);  // upper calf/knee
    addSolidBox(buf, i, -0.24f, 1.00f,  0.00f, 0.28f, 0.32f, 0.40f, coat);    // lower thigh
    addSolidBox(buf, i, -0.24f, 1.38f,  0.00f, 0.32f, 0.34f, 0.36f, coatDk);  // upper thigh
    // right leg
    addSolidBox(buf, i,  0.24f, 0.00f, -0.08f, 0.28f, 0.50f, 0.22f, leather); // boot
    addSolidBox(buf, i,  0.24f, 0.22f,  0.00f, 0.24f, 0.28f, 0.40f, coat);    // lower calf
    addSolidBox(buf, i,  0.24f, 0.60f,  0.00f, 0.26f, 0.30f, 0.42f, coatDk);  // upper calf/knee
    addSolidBox(buf, i,  0.24f, 1.00f,  0.00f, 0.28f, 0.32f, 0.40f, coat);    // lower thigh
    addSolidBox(buf, i,  0.24f, 1.38f,  0.00f, 0.32f, 0.34f, 0.36f, coatDk);  // upper thigh

    // ================= PELVIS + TORSO (narrow waist -> broad chest) =========
    addSolidBox(buf, i, 0.00f, 1.68f, 0.00f, 0.68f, 0.42f, 0.22f, coatDk); // pelvis
    addSolidBox(buf, i, 0.00f, 1.88f, 0.00f, 0.56f, 0.36f, 0.26f, coat);   // narrow waist
    addSolidBox(buf, i, 0.00f, 2.12f, 0.00f, 0.66f, 0.40f, 0.30f, coat);   // lower chest
    addSolidBox(buf, i, 0.00f, 2.40f, 0.00f, 0.80f, 0.44f, 0.30f, plate);  // chest plate
    addSolidBox(buf, i, 0.00f, 2.66f, 0.00f, 0.88f, 0.46f, 0.14f, plate);  // shoulder yoke

    // ================= NECK + HIGH COLLAR + HEAD ===========================
    addSolidBox(buf, i, 0.00f, 2.78f,  0.02f, 0.20f, 0.22f, 0.26f, skin);   // thin neck
    addSolidBox(buf, i, 0.00f, 2.72f,  0.09f, 0.50f, 0.40f, 0.28f, coatDk); // gothic collar
    addSolidBox(buf, i, 0.00f, 3.02f,  0.04f, 0.42f, 0.46f, 0.24f, hair);   // lower skull
    addSolidBox(buf, i, 0.00f, 3.24f,  0.04f, 0.44f, 0.46f, 0.24f, hair);   // crown
    addSolidBox(buf, i, 0.00f, 3.08f, -0.20f, 0.34f, 0.12f, 0.28f, skin);   // face plate (-z)

    // ---- spiky black hair (5 slim pyramids, offset to look tilted) ----
    addPyramid(buf, i, -0.14f, 3.42f,  0.14f, 0.16f, 0.16f, 0.28f, hair);
    addPyramid(buf, i,  0.14f, 3.44f,  0.10f, 0.14f, 0.14f, 0.24f, hair);
    addPyramid(buf, i,  0.00f, 3.42f,  0.22f, 0.16f, 0.15f, 0.30f, hair);
    addPyramid(buf, i, -0.04f, 3.46f, -0.02f, 0.13f, 0.13f, 0.22f, hair);
    addPyramid(buf, i,  0.10f, 3.44f,  0.20f, 0.12f, 0.12f, 0.20f, hair);

    // ================= ARMS (pauldron -> upper -> forearm -> glove) ========
    // left arm
    addSolidBox(buf, i, -0.46f, 2.50f,  0.00f, 0.30f, 0.42f, 0.30f, plate);   // pauldron
    addSolidBox(buf, i, -0.44f, 2.12f,  0.02f, 0.22f, 0.28f, 0.42f, coat);    // upper arm
    addSolidBox(buf, i, -0.44f, 1.98f,  0.00f, 0.20f, 0.24f, 0.16f, coatDk);  // elbow
    addSolidBox(buf, i, -0.44f, 1.66f,  0.00f, 0.19f, 0.22f, 0.34f, coat);    // forearm
    addSolidBox(buf, i, -0.44f, 1.50f, -0.02f, 0.20f, 0.26f, 0.18f, leather); // glove
    // right arm (hand grips the sword)
    addSolidBox(buf, i,  0.46f, 2.50f,  0.00f, 0.30f, 0.42f, 0.30f, plate);   // pauldron
    addSolidBox(buf, i,  0.44f, 2.12f,  0.02f, 0.22f, 0.28f, 0.42f, coat);    // upper arm
    addSolidBox(buf, i,  0.44f, 1.98f,  0.00f, 0.20f, 0.24f, 0.16f, coatDk);  // elbow
    addSolidBox(buf, i,  0.44f, 1.66f,  0.00f, 0.19f, 0.22f, 0.34f, coat);    // forearm
    addSolidBox(buf, i,  0.46f, 1.48f,  0.00f, 0.20f, 0.26f, 0.18f, leather); // glove

    // ================= RED DETAILS (belt, back straps, coat trim) ==========
    addSolidBox(buf, i,  0.00f, 1.84f,  0.00f, 0.62f, 0.40f, 0.10f, red); // belt
    addSolidBox(buf, i, -0.12f, 2.08f,  0.25f, 0.11f, 0.05f, 0.62f, red); // back strap L
    addSolidBox(buf, i,  0.12f, 2.08f,  0.25f, 0.11f, 0.05f, 0.62f, red); // back strap R
    addSolidBox(buf, i, -0.10f, 1.30f, -0.20f, 0.05f, 0.06f, 0.62f, red); // coat front trim L
    addSolidBox(buf, i,  0.10f, 1.30f, -0.20f, 0.05f, 0.06f, 0.62f, red); // coat front trim R

    // ================= COAT PANELS + JIRONES (falling cloth) ===============
    addSolidBox(buf, i,  0.00f, 1.00f,  0.30f, 0.50f, 0.10f, 0.85f, coat);   // back center panel
    addSolidBox(buf, i, -0.28f, 0.95f,  0.26f, 0.22f, 0.10f, 0.88f, coatDk); // back left panel
    addSolidBox(buf, i,  0.28f, 1.00f,  0.26f, 0.22f, 0.10f, 0.82f, coatDk); // back right panel
    addSolidBox(buf, i, -0.40f, 1.05f,  0.05f, 0.12f, 0.34f, 0.80f, coat);   // side left panel
    addSolidBox(buf, i,  0.40f, 1.10f,  0.05f, 0.12f, 0.34f, 0.75f, coat);   // side right panel
    addSolidBox(buf, i, -0.18f, 1.15f, -0.20f, 0.16f, 0.10f, 0.68f, coatDk); // front left flap
    addSolidBox(buf, i,  0.18f, 1.20f, -0.20f, 0.16f, 0.10f, 0.62f, coatDk); // front right flap
    addSolidBox(buf, i, -0.34f, 0.68f,  0.28f, 0.10f, 0.08f, 0.40f, coatDk); // tatter
    addSolidBox(buf, i,  0.32f, 0.72f,  0.30f, 0.10f, 0.08f, 0.35f, redDk);  // tatter (red)
    addSolidBox(buf, i,  0.00f, 0.70f,  0.34f, 0.12f, 0.08f, 0.44f, coat);   // tatter

    // ================= LONG SWORD (down at the right side) =================
    // grip in hand, guard, then a blade stepped down-and-outward to fake a tilt
    addSolidBox(buf, i, 0.50f, 1.42f, 0.06f, 0.09f, 0.09f, 0.28f, leather); // wrapped grip
    addSolidBox(buf, i, 0.52f, 1.36f, 0.07f, 0.26f, 0.22f, 0.06f, brass);   // tsuba guard
    addSolidBox(buf, i, 0.56f, 1.02f, 0.09f, 0.11f, 0.05f, 0.36f, steel);   // blade 1
    addSolidBox(buf, i, 0.61f, 0.68f, 0.11f, 0.10f, 0.05f, 0.36f, steel);   // blade 2
    addSolidBox(buf, i, 0.66f, 0.36f, 0.13f, 0.09f, 0.05f, 0.34f, steel);   // blade 3
    addSolidBox(buf, i, 0.70f, 0.12f, 0.15f, 0.07f, 0.05f, 0.26f, steel);   // point
}
