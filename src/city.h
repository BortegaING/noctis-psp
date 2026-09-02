#pragma once
// ============================================================================
//  NOCTIS — PATIO / COURTYARD (first-person)
//  Brutalist concrete-and-stone monoliths fused with a BLAME! megastructure,
//  ringing a single central plaza. FEW, MASSIVE, monolithic forms.
//
//  struct CityBldg { float x, z, w, d, h; unsigned int color; };  // AABB wall
//  footprint centered at (x,z), spanning x±w/2, z±d/2, height h. blocked() in
//  main.cpp treats each as a solid impassable wall (player radius r=1.1).
//
//  (a) PLAY-AREA BOUNDS (set main.cpp's movement clamp to these — tight and
//      fully enclosed by the footprints below; no void is reachable):
//          x in [-38.0, 38.0]      z in [-38.0, 38.0]
//      Ring inner faces sit at ±41, so the blocked() threshold is ±39.9 —
//      these bounds keep ~1.9 units of clearance from every wall.
//
//  (b) GATE OPENINGS (framed vistas — visible gap in the silhouette, but the
//      clamp above keeps the player inside so the gap is never walkable out):
//          NORTH GATE: world-space x in [-5.0, +5.0] at the north wall
//                      (z ~= -53), opening width = 10.0 units, flanked by two
//                      tall gothic towers. This is the only opening in the ring.
//
//  Ring layout (inner face / wall depth 24, corners overlap ~4u so there is no
//  diagonal slip): EAST & WEST slabs run the full Z span (z -61..61) and cover
//  all four corners; SOUTH slab bridges between them (x -45..45); the NORTH
//  side is split into two towers leaving the 10u gate.
// ============================================================================
static void buildCity() {
    int n = 0;
    // EAST wall  (x 41..65, z -61..61) — long slab, covers NE & SE corners
    g_city[n++] = {  53.0f,   0.0f, 24.0f, 122.0f, 130.0f, RGBA(58, 55, 50, 255) };
    // WEST wall  (x -65..-41, z -61..61) — long slab, covers NW & SW corners
    g_city[n++] = { -53.0f,   0.0f, 24.0f, 122.0f, 120.0f, RGBA(56, 53, 48, 255) };
    // SOUTH wall (x -45..45, z 41..65) — bridges east<->west, overlaps corners
    g_city[n++] = {   0.0f,  53.0f, 90.0f,  24.0f, 100.0f, RGBA(60, 57, 52, 255) };
    // NORTH-WEST tower (x -45..-5, z -65..-41) — left jamb of the gate
    g_city[n++] = { -25.0f, -53.0f, 40.0f,  24.0f, 200.0f, RGBA(54, 51, 46, 255) };
    // NORTH-EAST tower (x 5..45,  z -65..-41) — right jamb of the gate
    g_city[n++] = {  25.0f, -53.0f, 40.0f,  24.0f, 190.0f, RGBA(57, 54, 49, 255) };
    g_cityCount = n;
}
