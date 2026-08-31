#pragma once
// PROJECT NOCTIS - Player character geometry (dark gothic warrior, Bloodborne-ish).
// Self-contained header: NO includes. Relies on types/functions already defined
// in main.cpp BEFORE this header is included:
//   struct LineVertex; RGBA(r,g,b,a); brighten(c,f);
//   addSolidBox(buf,i, cx,baseY,cz, w,d,h, col);   // 30 verts each
//   addPyramid(buf,i, cx,baseY,cz, w,d, apexH, col); // 12 verts each
//
// Character faces -z in local space (we see its BACK). Feet at y=0, centered on
// x=0,z=0, body height ~3.4 (hair spikes + raised greatsword rise a bit above).
//
// Vertex budget: 25 solid boxes (25*30=750) + 4 pyramids (4*12=48) = 798 verts.

static void buildPlayerModel(LineVertex *buf, int &i)
{
    // ---- palette ----
    const unsigned int coat    = RGBA(38, 36, 46, 255);   // near-black armored coat
    const unsigned int coatDk  = brighten(coat, 0.75f);   // shaded coat / gloves
    const unsigned int plate   = brighten(coat, 1.35f);   // armor plates (lighter)
    const unsigned int red     = RGBA(150, 40, 45, 255);  // blood-red straps/belt
    const unsigned int redDk   = RGBA(95, 28, 32, 255);   // dark tattered cloth
    const unsigned int skin    = RGBA(150, 132, 122, 255);// face / neck
    const unsigned int hair    = RGBA(24, 22, 30, 255);   // black spiky hair
    const unsigned int steel   = RGBA(120, 138, 175, 255);// katana blade
    const unsigned int leather = RGBA(30, 26, 24, 255);   // boots / hilt wrap
    const unsigned int brass   = RGBA(120, 100, 60, 255); // tsuba (guard)

    // ---- legs & boots (feet planted at y=0, slight stance) ----
    addSolidBox(buf, i, -0.36f, 0.00f, -0.04f, 0.44f, 0.72f, 0.36f, leather); // L boot
    addSolidBox(buf, i,  0.36f, 0.00f, -0.04f, 0.44f, 0.72f, 0.36f, leather); // R boot
    addSolidBox(buf, i, -0.32f, 0.34f,  0.02f, 0.36f, 0.40f, 1.10f, coat);    // L leg
    addSolidBox(buf, i,  0.32f, 0.34f,  0.02f, 0.36f, 0.40f, 1.10f, coat);    // R leg

    // ---- coat / faldon: bell-shaped skirt in 3 layers (wide hem -> waist) ----
    addSolidBox(buf, i,  0.00f, 0.95f, 0.00f, 1.50f, 1.00f, 0.55f, coat);   // lower flare
    addSolidBox(buf, i,  0.00f, 1.40f, 0.00f, 1.24f, 0.86f, 0.42f, coatDk); // mid
    addSolidBox(buf, i,  0.00f, 1.75f, 0.00f, 0.96f, 0.72f, 0.40f, coat);   // waist

    // ---- jirones (hanging tattered strips below the hem, at the back/side) ----
    addSolidBox(buf, i, -0.58f, 0.68f, 0.28f, 0.16f, 0.14f, 0.50f, redDk);  // L tatter (red)
    addSolidBox(buf, i,  0.52f, 0.62f, 0.34f, 0.16f, 0.14f, 0.56f, coatDk); // R tatter

    // ---- torso: red belt, broad chest plate, back strap (bandolier) ----
    addSolidBox(buf, i,  0.00f, 1.98f, 0.00f, 0.92f, 0.66f, 0.16f, red);   // waist belt
    addSolidBox(buf, i,  0.00f, 2.18f, 0.00f, 1.00f, 0.68f, 0.50f, plate); // chest plate
    addSolidBox(buf, i, -0.10f, 2.20f, 0.34f, 0.16f, 0.06f, 0.50f, red);   // back strap

    // ---- high gothic collar around the neck ----
    addSolidBox(buf, i,  0.00f, 2.62f, 0.06f, 0.66f, 0.56f, 0.30f, coatDk); // collar

    // ---- pauldrons (armored shoulders) ----
    addSolidBox(buf, i, -0.66f, 2.50f, 0.00f, 0.50f, 0.62f, 0.34f, plate); // L pauldron
    addSolidBox(buf, i,  0.66f, 2.50f, 0.00f, 0.50f, 0.62f, 0.34f, plate); // R pauldron

    // ---- arms hanging at the sides (upper arm + gauntleted forearm) ----
    addSolidBox(buf, i, -0.68f, 2.05f, 0.02f, 0.30f, 0.36f, 0.50f, coat);   // L upper
    addSolidBox(buf, i, -0.68f, 1.55f, 0.00f, 0.28f, 0.34f, 0.54f, coatDk); // L forearm
    addSolidBox(buf, i,  0.68f, 2.05f, 0.02f, 0.30f, 0.36f, 0.50f, coat);   // R upper
    addSolidBox(buf, i,  0.68f, 1.55f, 0.00f, 0.28f, 0.34f, 0.54f, coatDk); // R forearm

    // ---- neck + head (seen from back: skull is hair-colored, face plate is skin) ----
    addSolidBox(buf, i,  0.00f, 2.72f,  0.02f, 0.26f, 0.26f, 0.22f, skin); // neck
    addSolidBox(buf, i,  0.00f, 2.90f,  0.04f, 0.50f, 0.52f, 0.50f, hair); // head/skull
    addSolidBox(buf, i,  0.00f, 2.98f, -0.20f, 0.42f, 0.14f, 0.34f, skin); // face (front -z)

    // ---- greatsword / katana raised at the right side (mango + tsuba + long blade) ----
    addSolidBox(buf, i,  1.02f, 1.55f, 0.12f, 0.12f, 0.12f, 0.52f, leather); // hilt wrap
    addSolidBox(buf, i,  1.02f, 2.05f, 0.12f, 0.34f, 0.30f, 0.08f, brass);   // tsuba guard
    addSolidBox(buf, i,  1.02f, 2.13f, 0.12f, 0.14f, 0.06f, 1.42f, steel);   // long thin blade

    // ---- spiky black hair (3 small pyramids on top/back of skull) ----
    addPyramid(buf, i, -0.14f, 3.34f, 0.12f, 0.24f, 0.24f, 0.34f, hair); // spike L
    addPyramid(buf, i,  0.16f, 3.34f, 0.06f, 0.22f, 0.22f, 0.30f, hair); // spike R
    addPyramid(buf, i,  0.00f, 3.32f, 0.22f, 0.24f, 0.22f, 0.40f, hair); // spike back

    // ---- katana point ----
    addPyramid(buf, i,  1.02f, 3.55f, 0.12f, 0.14f, 0.06f, 0.42f, steel); // blade tip
}
