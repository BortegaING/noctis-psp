#pragma once
// PROJECT NOCTIS - Player character geometry, split into ANIMATABLE parts.
// Self-contained header: NO includes. Relies on types/functions already defined
// in main.cpp BEFORE this header is included:
//   struct LineVertex; RGBA(r,g,b,a); brighten(c,f);
//   addSolidBox(buf,i, cx,baseY,cz, w,d,h, col);     // 30 verts each
//   addPyramid(buf,i, cx,baseY,cz, w,d, apexH, col); // 12 verts each
//
// Each part is built in LOCAL space with its PIVOT at the origin (0,0,0) so the
// caller can rotate/translate it for walk/idle animation. Dark gothic warrior
// (Bloodborne-ish). Faces -z. Palette is near-black coat with blood-red accents.
//
// Vertex budget (function bodies): upper 474 + head 126 + leg 120 + arm 120
// + sword 102 = 942 verts total (under ~1000).

// -------------------------------------------------------------------------
// UPPER BODY: torso + chest plate + pauldrons + flared tattered coat + red
// straps. Pivot = hips at (0,0,0). Torso rises (local y 0..~1.18), the coat
// skirt hangs below the pivot (local y 0..-1.44). No head/arms/legs.
// 15 boxes + 2 pyramids = 474 verts.
static void buildChar_upper(LineVertex *buf, int &i)
{
    const unsigned int coat   = RGBA(38, 36, 46, 255);
    const unsigned int coatDk = brighten(coat, 0.75f);
    const unsigned int plate  = brighten(coat, 1.35f);
    const unsigned int red    = RGBA(150, 40, 45, 255);
    const unsigned int redDk  = RGBA(95, 28, 32, 255);

    // torso rising from the hips
    addSolidBox(buf, i,  0.00f,  0.00f, 0.00f, 0.82f, 0.56f, 0.50f, coat);  // abdomen
    addSolidBox(buf, i,  0.00f,  0.45f, 0.00f, 1.00f, 0.68f, 0.50f, plate); // chest plate
    addSolidBox(buf, i,  0.00f,  0.90f, 0.00f, 0.72f, 0.52f, 0.16f, plate); // clavicle bevel
    addSolidBox(buf, i,  0.00f,  1.00f, 0.06f, 0.62f, 0.50f, 0.18f, coatDk);// high collar

    // red belt + back straps (bandolier)
    addSolidBox(buf, i,  0.00f, -0.03f, 0.00f, 0.90f, 0.60f, 0.16f, red);   // belt
    addSolidBox(buf, i, -0.12f,  0.40f, 0.33f, 0.15f, 0.06f, 0.55f, red);   // strap L
    addSolidBox(buf, i,  0.14f,  0.35f, 0.32f, 0.14f, 0.06f, 0.50f, red);   // strap R

    // pauldrons (marked armored shoulders) with beveled spikes on top
    addSolidBox(buf, i, -0.64f,  0.66f, 0.00f, 0.52f, 0.64f, 0.36f, plate); // pauldron L
    addSolidBox(buf, i,  0.64f,  0.66f, 0.00f, 0.52f, 0.64f, 0.36f, plate); // pauldron R
    addPyramid (buf, i, -0.64f,  1.00f, 0.00f, 0.50f, 0.60f, 0.30f, plate); // spike L
    addPyramid (buf, i,  0.64f,  1.00f, 0.00f, 0.50f, 0.60f, 0.30f, plate); // spike R

    // coat / faldon hanging below the hips: bell-shaped in 3 layers
    addSolidBox(buf, i,  0.00f, -0.28f, 0.00f, 0.96f, 0.72f, 0.28f, coat);   // waist
    addSolidBox(buf, i,  0.00f, -0.52f, 0.00f, 1.24f, 0.88f, 0.28f, coatDk); // mid layer
    addSolidBox(buf, i,  0.00f, -0.72f, 0.00f, 1.50f, 1.02f, 0.24f, coat);   // wide hem

    // jirones (tattered strips hanging below the hem)
    addSolidBox(buf, i, -0.50f, -1.05f,  0.20f, 0.16f, 0.14f, 0.36f, redDk); // tatter L
    addSolidBox(buf, i,  0.46f, -1.10f,  0.28f, 0.16f, 0.14f, 0.40f, coatDk);// tatter R
    addSolidBox(buf, i,  0.00f, -1.05f, -0.40f, 0.18f, 0.14f, 0.34f, redDk); // tatter front
}

// -------------------------------------------------------------------------
// HEAD: neck + skull + spiky black hair. Pivot = base of the neck at (0,0,0),
// rising (local y 0..~0.8). Seen from the back the skull is hair-colored; the
// face plate is skin toward -z. 3 boxes + 3 pyramids = 126 verts.
static void buildChar_head(LineVertex *buf, int &i)
{
    const unsigned int skin = RGBA(150, 132, 122, 255);
    const unsigned int hair = RGBA(24, 22, 30, 255);

    addSolidBox(buf, i, 0.00f, 0.00f,  0.02f, 0.26f, 0.26f, 0.20f, skin); // neck
    addSolidBox(buf, i, 0.00f, 0.16f,  0.04f, 0.50f, 0.52f, 0.42f, hair); // skull (back = hair)
    addSolidBox(buf, i, 0.00f, 0.22f, -0.20f, 0.42f, 0.14f, 0.30f, skin); // face plate (-z)

    addPyramid(buf, i, -0.14f, 0.50f, 0.12f, 0.24f, 0.24f, 0.30f, hair);  // hair spike L
    addPyramid(buf, i,  0.16f, 0.50f, 0.04f, 0.22f, 0.22f, 0.26f, hair);  // hair spike R
    addPyramid(buf, i,  0.00f, 0.48f, 0.22f, 0.24f, 0.22f, 0.34f, hair);  // hair spike back
}

// -------------------------------------------------------------------------
// ONE LEG: thigh + knee + shin + boot. Pivot = hip at (0,0,0), extends DOWN
// (local y 0..-1.35). Drawn twice (mirror x for L/R). 4 boxes = 120 verts.
static void buildChar_leg(LineVertex *buf, int &i)
{
    const unsigned int coat    = RGBA(38, 36, 46, 255);
    const unsigned int coatDk  = brighten(coat, 0.75f);
    const unsigned int leather = RGBA(30, 26, 24, 255);

    addSolidBox(buf, i, 0.00f, -0.60f,  0.00f, 0.36f, 0.42f, 0.60f, coat);   // thigh
    addSolidBox(buf, i, 0.00f, -0.74f,  0.00f, 0.30f, 0.38f, 0.16f, coatDk); // knee
    addSolidBox(buf, i, 0.00f, -1.15f,  0.00f, 0.30f, 0.36f, 0.45f, coat);   // shin
    addSolidBox(buf, i, 0.00f, -1.35f, -0.08f, 0.42f, 0.72f, 0.26f, leather);// boot (toe -z)
}

// -------------------------------------------------------------------------
// ONE ARM: shoulder/upper arm + elbow + forearm + glove. Pivot = shoulder at
// (0,0,0), extends DOWN (local y 0..-1.05). Drawn twice. 4 boxes = 120 verts.
static void buildChar_arm(LineVertex *buf, int &i)
{
    const unsigned int coat    = RGBA(38, 36, 46, 255);
    const unsigned int coatDk  = brighten(coat, 0.75f);
    const unsigned int leather = RGBA(30, 26, 24, 255);

    addSolidBox(buf, i, 0.00f, -0.50f,  0.00f, 0.30f, 0.36f, 0.50f, coat);   // upper arm
    addSolidBox(buf, i, 0.00f, -0.62f,  0.00f, 0.27f, 0.33f, 0.14f, coatDk); // elbow
    addSolidBox(buf, i, 0.00f, -0.92f,  0.00f, 0.27f, 0.31f, 0.34f, coat);   // forearm
    addSolidBox(buf, i, 0.00f, -1.05f, -0.02f, 0.26f, 0.34f, 0.16f, leather);// glove
}

// -------------------------------------------------------------------------
// KATANA: wrapped hilt + tsuba guard + long steel blade + point. Pivot at the
// grip (0,0,0). Hilt hangs below the pivot (y 0..-0.5) and the blade extends up
// (+y) so the caller can hang it on the back at any angle. 3 boxes + 1 pyramid
// = 102 verts.
static void buildChar_sword(LineVertex *buf, int &i)
{
    const unsigned int leather = RGBA(30, 26, 24, 255);
    const unsigned int brass   = RGBA(120, 100, 60, 255);
    const unsigned int steel   = RGBA(120, 138, 175, 255);

    addSolidBox(buf, i, 0.00f, -0.50f, 0.00f, 0.10f, 0.10f, 0.50f, leather); // wrapped hilt
    addSolidBox(buf, i, 0.00f,  0.00f, 0.00f, 0.30f, 0.26f, 0.07f, brass);   // tsuba guard
    addSolidBox(buf, i, 0.00f,  0.07f, 0.00f, 0.12f, 0.05f, 1.50f, steel);   // long thin blade
    addPyramid (buf, i, 0.00f,  1.57f, 0.00f, 0.12f, 0.05f, 0.40f, steel);   // blade point
}
