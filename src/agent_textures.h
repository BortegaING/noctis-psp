#pragma once
// PROJECT NOCTIS - procedural textures + gothic ambience geometry.
// AUTOCONTAINED header: no includes. Relies on symbols defined in main.cpp
// BEFORE this header is included:
//   #define RGBA(r,g,b,a)  -> packs 0xAABBGGRR
//   struct LineVertex { unsigned int color; float x,y,z; };
//   unsigned int brighten(unsigned int c, float f);
//   void addSolidBox(LineVertex*, int&, float,float,float, float,float,float, unsigned int);
//   void addPyramid (LineVertex*, int&, float,float,float, float,float,float, unsigned int);
// Style: BLAME! megastructure + dark gothic (Bloodborne / 3rd Birthday mood).
// Textures are drawn with MODULATE (final = tex * vertex color, and the vertex
// color is often pre-darkened by fog / height haze). So the PATTERNS keep their
// detail in the MID tones (~60-170) and build contrast by VALUE, never by going
// near-black. Stone/metal/roof stay cool & desaturated; the world's warm braziers
// do the warming. Stone & metal & roof REPEAT (seamless); the window CLAMPs.

// ---- tiny internal helpers (no stdlib) -------------------------------------
static inline int noctis_clampi(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }

// integer hash -> pseudo random 0..2^32, stable per (x,y)
static inline unsigned int noctis_hash(int x, int y) {
    unsigned int h = (unsigned int)(x * 374761393) + (unsigned int)(y * 668265263);
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return h;
}

// ============================================================================
// PART 1 - PROCEDURAL TEXTURE GENERATORS (fill unsigned int* t, RGBA 0xAABBGGRR)
// ============================================================================

// Stone: weathered gothic ashlar. Block courses + recessed mortar with soft
// ambient occlusion into the joints, carved bevel, downward grime streaks,
// hairline cracks/pits and faint moss/verdigris pooling low & in the joints.
// Cool DESATURATED grey (B >= G >= R) so it never reads brown; tiles seamlessly.
static void genStone(unsigned int *t, int W) {
    const int bw = 32, bh = 16;           // sillar (block) size (divides W)
    for (int y = 0; y < W; ++y) {
        int row = y / bh;
        int off = (row & 1) ? (bw / 2) : 0; // running bond (half-block offset)
        for (int x = 0; x < W; ++x) {
            int gx = x + off;
            int bx = gx % bw;
            int by = y % bh;

            // cool desaturated grey-stone base (B >= G >= R)
            int r = 150, g = 154, b = 158;

            // per-block value variation (equal on channels => stays neutral grey)
            int bt = (int)(noctis_hash(gx / bw, row) & 15) - 7;
            r += bt; g += bt; b += bt;

            int joint = (by < 2 || bx < 2);
            if (joint) {
                // recessed mortar: darker and a touch cooler
                r -= 66; g -= 62; b -= 56;
            } else {
                // soft ambient occlusion: darken as we approach a joint edge
                int ao = 0;
                if (by < 6)              ao = 6 - by;
                if (bx < 7 && 7 - bx > ao) ao = 7 - bx;
                r -= ao * 4; g -= ao * 4; b -= ao * 3;

                // carved bevel: lit top-left, shaded bottom-right (3D sillar)
                if (by < 3 || bx < 4)           { r += 12; g += 12; b += 12; }
                if (by > bh - 3 || bx > bw - 4) { r -= 16; g -= 16; b -= 16; }
            }

            // fine stone grain / grime
            int n = (int)(noctis_hash(x, y) & 31) - 15;
            r += n; g += n; b += n;

            // vertical grime streaks bleeding DOWN from each course ledge.
            // Column choice is per-x (tiles across); intensity ramps with the
            // in-course row 'by' and resets each course (tiles vertically).
            unsigned int sh = noctis_hash(x, 111);
            if ((sh & 63) < 7) {
                int strength = 2 + (int)((sh >> 8) & 3);   // 2..5
                int d = (strength * by) / bh;              // 0 at top -> darker low
                r -= d; g -= d; b -= (d * 3) / 4;          // grime stays cool
            }

            // faint moss / verdigris on a few whole blocks, pooling low & at joints
            if ((int)(noctis_hash(gx / bw + 31, row * 2 + 1) & 15) < 3) {
                int prox   = (by > bh / 2) ? (by - bh / 2) : 0;
                int nearJ  = (bx < 5 || by < 4) ? 3 : 0;
                int mm     = prox + nearJ;
                if (mm > 0) {
                    mm += (int)(noctis_hash(x, y) & 3);
                    g += mm; b += (mm * 3) / 4; r -= mm / 2; // green-cyan tint
                }
            }

            // wandering hairline cracks (broken diagonal families)
            int cw = (x + (int)(noctis_hash(y, 7) & 3)) - y;
            if ((cw % 57) == 0 && (noctis_hash(x, y * 5) & 7) < 5) {
                r -= 52; g -= 50; b -= 46;
            }
            // small dark pits / spalled chips
            if ((noctis_hash(x * 3, y) & 255) < 4) { r -= 28; g -= 26; b -= 24; }

            t[y * W + x] = RGBA(noctis_clampi(r), noctis_clampi(g), noctis_clampi(b), 255);
        }
    }
}

// Metal: worn wrought iron / dark steel. Vertical brushed striation + broad
// brushed sheen passes, recessed structural bands, riveted grid, and warm
// orange-brown rust flecks clustered where water sits (grooves & rivet rings).
// Cool base (B >= G >= R); rust is the only warmth. Tiles seamlessly.
static void genMetal(unsigned int *t, int W) {
    for (int y = 0; y < W; ++y) {
        int band = y % 16;
        for (int x = 0; x < W; ++x) {
            // dark cool steel base (B >= G >= R)
            int r = 150, g = 158, b = 170;

            // vertical brushed striation (two frequencies)
            int s  = (int)(noctis_hash(x, 0) & 15) - 7;
            int s2 = (int)(noctis_hash(x * 7, 3) & 7) - 3;
            r += s + s2; g += s + s2; b += s + s2;

            // broad brushed sheen, period 32 (tiles): brighter mid-pass
            int sxp = x & 31; int tri = (sxp < 16 ? sxp : 31 - sxp); // 0..15
            int sheen = (tri - 8) / 2;                               // -4..+3
            r += sheen; g += sheen; b += sheen;

            // horizontal structural bands: recessed groove + sheen falling below
            if (band < 2) { r -= 42; g -= 42; b -= 38; }
            else          { int sb = 8 - band; if (sb < 0) sb = 0; r += sb; g += sb; b += sb; }

            // rivets on a 16x16 grid: bright head, dark seating ring
            int rx = (x % 16) - 8, ry = (y % 16) - 8;
            int rr = rx * rx + ry * ry;
            if      (rr <= 6)  { r += 34; g += 34; b += 34; }
            else if (rr <= 12) { r -= 24; g -= 24; b -= 22; }

            // rust: warm orange-brown flecks, clustered where water collects
            unsigned int rc = noctis_hash(x / 5, y / 5);
            int rustZone = ((rc & 15) < 4) || (band < 3) || (rr <= 14 && rr > 6);
            if (rustZone && (noctis_hash(x, y) & 7) < 3) {
                int amt = 10 + (int)(noctis_hash(x * 5, y) & 15); // 10..25
                r += amt; g += (amt * 2) / 5; b -= amt / 2;       // push to orange-brown
            }

            int n = (int)(noctis_hash(x, y) & 15) - 7;
            r += n; g += n; b += n;

            t[y * W + x] = RGBA(noctis_clampi(r), noctis_clampi(g), noctis_clampi(b), 255);
        }
    }
}

// Window: gothic stained glass (CLAMP => one texture is one whole window).
// Ogival (pointed) opening, dark lead came/tracery, a rose window of radial
// spokes + concentric rings in the upper arch, and deep jewel panes (crimson,
// cobalt, amber, violet, emerald, rose) that read as lit but stay moody. The
// leading is dark-but-not-black so it survives MODULATE against a lit panel.
static void genWindow(unsigned int *t, int W) {
    const int m        = 2;        // stone frame thickness
    const int cxk      = W / 2;    // center column
    const int springY  = W / 2;    // springline: pointed above, straight below
    const int fullHalf = (W / 2) - m;

    // deep jewel palette (lit but moody): crimson, cobalt, amber, violet,
    // emerald, rose-magenta. Floors kept off black so MODULATE stays readable.
    static const int jr[6] = {188,  60, 224, 150,  70, 200};
    static const int jg[6] = { 56,  92, 168,  78, 172,  64};
    static const int jb[6] = { 72, 198,  70, 190, 120, 150};

    // rose window: centre high in the arch
    const int rcx = cxk;
    const int rcy = W * 3 / 10;
    const int R   = (W / 5) > 3 ? (W / 5) : 3;

    for (int y = 0; y < W; ++y) {
        // allowed half-width of the opening at this row (0 at apex -> full at spring)
        int half;
        if (y < m)            half = -1;
        else if (y < springY) half = ((y - m) * fullHalf) / (springY - m);
        else                  half = fullHalf;

        for (int x = 0; x < W; ++x) {
            int dx = x - cxk;
            int inside = (half >= 0 && dx > -half && dx < half && y >= m && y < W - m);

            int r, g, b;
            if (inside) {
                int lead = 0;

                // default glass piece from the came grid
                int cellx = x / 6, celly = y / 6;
                int idx   = (int)(noctis_hash(cellx, celly) % 6u);

                // rose window overrides the grid in the upper arch
                int rdx = x - rcx, rdy = y - rcy;
                int rd2 = rdx * rdx + rdy * rdy;
                int inRose = (y < springY && rd2 <= R * R);
                if (inRose) {
                    int ax = rdx < 0 ? -rdx : rdx;
                    int ay = rdy < 0 ? -rdy : rdy;
                    int oct = 0;
                    if (rdx >= 0) oct |= 1;
                    if (rdy >= 0) oct |= 2;
                    if (ax > ay)  oct |= 4;
                    int rb = (rd2 * 3) / (R * R);          // ring band 0..2
                    idx = (oct + rb) % 6;
                    // rose leading: hub + 8 radial spokes + concentric rings
                    if (rd2 <= (R / 5 + 1) * (R / 5 + 1))               lead = 1;
                    if (ax < 1 || ay < 1)                               lead = 1;
                    if (ax - ay < 1 && ay - ax < 1)                     lead = 1;
                    int rnext = ((rb + 1) * (R * R)) / 3;
                    if (rd2 > rnext - 2 * R && rd2 < rnext + 2 * R)     lead = 1;
                }

                int rr = jr[idx], gg = jg[idx], bb = jb[idx];

                // backlit glow: brighter toward the centre of each glass piece
                int lx = (x % 6) - 3, ly = (y % 6) - 3;
                int glow = 5 - (lx * lx + ly * ly);        // ~ -13..+5
                if (glow < -6) glow = -6;
                rr += glow; gg += glow; bb += glow;

                // per-piece brightness jitter (keeps modulated colour lively)
                int pv = (int)(noctis_hash(cellx, celly * 3) & 7) - 3;
                rr += pv; gg += pv; bb += pv;

                // lead came / tracery on top of the glass
                if (!inRose && ((x % 6) == 0 || (y % 6) == 0)) lead = 1; // came grid
                if (dx == 0 || dx == -1)                       lead = 1; // central mullion
                if (y == springY || y == springY - 1)          lead = 1; // transom bar
                if (dx <= -half + 1 || dx >= half - 1)         lead = 1; // ogival outline

                if (lead) {
                    // dark cool leading (not black -> reads through MODULATE)
                    int ln = (int)(noctis_hash(x, y) & 3);
                    r = 48 + ln; g = 48 + ln; b = 56 + ln;
                } else {
                    r = rr; g = gg; b = bb;
                }
            } else {
                // surrounding stone frame: cool neutral grey (NOT brown)
                r = 92; g = 92; b = 98;
                int n = (int)(noctis_hash(x, y) & 15) - 7;
                r += n; g += n; b += n;
                // chamfer highlight right at the opening edge
                if (half >= 0 && (dx == -half || dx == half)) { r += 22; g += 22; b += 22; }
            }
            t[y * W + x] = RGBA(noctis_clampi(r), noctis_clampi(g), noctis_clampi(b), 255);
        }
    }
}

// Roof: dark wet slate/lead in overlapping courses (hileras). Barrel-shaded
// tiles (rounded), an exposed wet-sheen lip with specular sparkle, deep overlap
// shadows, vertical seam gaps and a little moss creeping from the shadows.
// Cool & dark but detail held in the mid tones; tiles seamlessly.
static void genRoof(unsigned int *t, int W) {
    for (int y = 0; y < W; ++y) {
        int row = y / 8;
        int off = (row & 1) ? 8 : 0;
        int ty  = y % 8;
        for (int x = 0; x < W; ++x) {
            int tx = (x + off) % 16;

            // dark wet slate (cool)
            int r = 116, g = 122, b = 132;

            // per-tile value variation
            int tv = (int)(noctis_hash((x + off) / 16, row) & 15) - 7;
            r += tv; g += tv; b += tv;

            // rounded tile: darker toward the vertical edges (barrel shading)
            int cx = tx - 8;
            int curve = (cx * cx) / 12;          // 0 centre .. ~5 edges
            r -= curve; g -= curve; b -= curve;

            // course structure
            if (ty == 0) { r += 22; g += 24; b += 30; }   // exposed wet lip highlight
            if (ty == 1) { r += 10; g += 11; b += 15; }   // sheen falloff
            if (ty >= 6) { r -= 36; g -= 36; b -= 34; }   // overlap shadow
            if (tx == 0) { r -= 40; g -= 40; b -= 38; }   // vertical seam gap

            // wet specular sparkle: sparse bright pinpoints on the tile face
            if (ty >= 1 && ty <= 4 && (noctis_hash(x, y) & 255) < 5) {
                r += 34; g += 36; b += 40;
            }

            // faint moss creeping up from the overlap shadow on a few tiles
            if (ty >= 5 && (int)(noctis_hash((x + off) / 16 + 9, row * 2) & 15) < 3) {
                int mm = (ty - 4) + (int)(noctis_hash(x, y) & 3);
                g += mm; b += mm / 2; r -= mm / 2;
            }

            int n = (int)(noctis_hash(x, y) & 15) - 7;
            r += n; g += n; b += n;

            t[y * W + x] = RGBA(noctis_clampi(r), noctis_clampi(g), noctis_clampi(b), 255);
        }
    }
}

// ============================================================================
// PART 2 - AMBIENCE GEOMETRY (append to buf at index i)
// ============================================================================

// Brazier / lantern: iron foot + post + bowl with glowing coals + layered flame.
static void addBrazier(LineVertex *buf, int &i, float x, float z) {
    addSolidBox(buf, i, x, 0.0f,  z, 0.34f, 0.34f, 0.08f, RGBA(46, 46, 56, 255));  // base foot
    addSolidBox(buf, i, x, 0.08f, z, 0.18f, 0.18f, 1.24f, RGBA(58, 58, 70, 255));  // post
    addSolidBox(buf, i, x, 1.30f, z, 0.55f, 0.55f, 0.22f, RGBA(96, 90, 82, 255));  // iron bowl
    addSolidBox(buf, i, x, 1.34f, z, 0.40f, 0.40f, 0.10f, RGBA(150, 70, 30, 255)); // glowing coals
    addPyramid (buf, i, x, 1.50f, z, 0.50f, 0.50f, 0.75f, RGBA(240, 150, 60, 255));// outer flame
    addPyramid (buf, i, x, 1.60f, z, 0.28f, 0.28f, 0.60f,
                brighten(RGBA(255, 210, 110, 255), 1.35f));                          // inner flame
}

// Banner: hanging pole + dark violet cloth with red trim and a gold emblem.
static void addBanner(LineVertex *buf, int &i, float x, float y, float z, float w, float h) {
    float base = y - h;
    addSolidBox(buf, i, x, y + 0.02f, z, w + 0.10f, 0.07f, 0.07f, RGBA(70, 62, 40, 255));  // hanging pole
    addSolidBox(buf, i, x, base, z, w, 0.05f, h, RGBA(38, 28, 52, 255));                    // cloth
    addSolidBox(buf, i, x, y - 0.12f, z, w + 0.02f, 0.06f, 0.12f, RGBA(150, 30, 30, 255));  // top trim
    addSolidBox(buf, i, x, base,      z, w + 0.02f, 0.06f, 0.12f, RGBA(150, 30, 30, 255));  // bottom trim
    addSolidBox(buf, i, x, base + h * 0.5f, z, w * 0.42f, 0.06f, w * 0.42f, RGBA(150, 120, 40, 255)); // gold emblem
}

// Arch: pointed gothic archway - pillars with capitals + stepped lintel to a point.
static void addArch(LineVertex *buf, int &i, float x, float z, float w, float h) {
    const float pw = 0.25f, d = 0.30f;
    float lx = x - (w * 0.5f), rx = x + (w * 0.5f);
    addSolidBox(buf, i, lx, 0.0f,      z, pw, d, h, RGBA(120, 112, 100, 255));                 // left pillar
    addSolidBox(buf, i, rx, 0.0f,      z, pw, d, h, RGBA(120, 112, 100, 255));                 // right pillar
    addSolidBox(buf, i, lx, h - 0.12f, z, pw + 0.12f, d + 0.06f, 0.16f, RGBA(128, 120, 106, 255)); // left capital
    addSolidBox(buf, i, rx, h - 0.12f, z, pw + 0.12f, d + 0.06f, 0.16f, RGBA(128, 120, 106, 255)); // right capital
    addSolidBox(buf, i, x, h,          z, w + pw,    d, 0.25f, RGBA(130, 122, 108, 255));      // lintel
    addSolidBox(buf, i, x, h + 0.25f,  z, w * 0.70f, d, 0.22f, RGBA(126, 118, 104, 255));      // step
    addSolidBox(buf, i, x, h + 0.47f,  z, w * 0.42f, d, 0.20f, RGBA(122, 114, 100, 255));      // step
    addPyramid (buf, i, x, h + 0.67f,  z, w * 0.42f, d, 0.60f, RGBA(118, 110,  96, 255));      // apex point
}
