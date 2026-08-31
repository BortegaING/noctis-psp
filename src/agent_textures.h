#pragma once
// PROJECT NOCTIS - procedural textures + gothic ambience geometry.
// AUTOCONTAINED header: no includes. Relies on symbols defined in main.cpp
// BEFORE this header is included:
//   #define RGBA(r,g,b,a)  -> packs 0xAABBGGRR
//   struct LineVertex { unsigned int color; float x,y,z; };
//   unsigned int brighten(unsigned int c, float f);
//   void addSolidBox(LineVertex*, int&, float,float,float, float,float,float, unsigned int);
//   void addPyramid (LineVertex*, int&, float,float,float, float,float,float, unsigned int);
// Style: BLAME! megastructure + dark gothic + warm sepia. Textures used with
// MODULATE (tex*color), so they average BRIGHT with darker detail.

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

// Stone: gothic ashlar / brick, mortar joints, hairline cracks, grime. Warm gray.
static void genStone(unsigned int *t, int W) {
    const int bw = 32, bh = 16;           // sillar (block) size
    for (int y = 0; y < W; ++y) {
        int row = y / bh;
        int off = (row & 1) ? (bw / 2) : 0; // running bond (half-block offset)
        for (int x = 0; x < W; ++x) {
            int gx = x + off;
            int bx = gx % bw;
            int by = y % bh;

            // warm gray base (R >= G >= B)
            int r = 192, g = 180, b = 164;

            // per-block tint variation so no two sillares look identical
            int bt = (int)(noctis_hash(gx / bw, row) & 15) - 7;
            r += bt; g += bt; b += bt;

            // mortar joints: recessed, cooler, darker
            int joint = (by < 2 || bx < 2);
            if (joint) {
                r -= 70; g -= 66; b -= 58;
            } else {
                // carved bevel: lit top-left, shaded bottom-right (3D sillar)
                if (by < 3 || bx < 4)             { r += 14; g += 14; b += 12; }
                if (by > bh - 3 || bx > bw - 4)   { r -= 20; g -= 20; b -= 18; }
            }

            // grime / general noise
            int n = (int)(noctis_hash(x, y) & 31) - 15;
            r += n; g += n; b += (n * 3) / 4;

            // wandering hairline cracks (broken diagonal families)
            int cw = (x + (int)(noctis_hash(y, 7) & 3)) - y;
            if ((cw % 57) == 0 && (noctis_hash(x, y * 5) & 7) < 5) {
                r -= 55; g -= 52; b -= 48;
            }
            // small dark pits
            if ((noctis_hash(x * 3, y) & 255) < 4) { r -= 30; g -= 28; b -= 26; }

            t[y * W + x] = RGBA(noctis_clampi(r), noctis_clampi(g), noctis_clampi(b), 255);
        }
    }
}

// Metal: iron/steel, vertical brushed striation, structural bands, rivets. Cold.
static void genMetal(unsigned int *t, int W) {
    for (int y = 0; y < W; ++y) {
        int band = y % 16;
        for (int x = 0; x < W; ++x) {
            // cool steel base (B >= G >= R)
            int r = 168, g = 176, b = 190;

            // vertical brushed striation (two frequencies)
            int s  = (int)(noctis_hash(x, 0) & 15) - 7;
            int s2 = (int)(noctis_hash(x * 7, 3) & 7) - 3;
            r += s + s2; g += s + s2; b += s + s2;

            // horizontal structural bands: groove every 16 rows, sheen below it
            if (band < 2) { r -= 45; g -= 45; b -= 40; }
            else          { int sh = 8 - band; if (sh < 0) sh = 0; r += sh; g += sh; b += sh; }

            // rivets on a 16x16 grid: bright head, dark seating ring
            int rx = (x % 16) - 8, ry = (y % 16) - 8;
            int rr = rx * rx + ry * ry;
            if      (rr <= 6)  { r += 35; g += 35; b += 35; }
            else if (rr <= 12) { r -= 25; g -= 25; b -= 22; }

            int n = (int)(noctis_hash(x, y) & 15) - 7;
            r += n; g += n; b += n;

            t[y * W + x] = RGBA(noctis_clampi(r), noctis_clampi(g), noctis_clampi(b), 255);
        }
    }
}

// Window: gothic stained glass. Ogival (pointed) arch, dark mullion/tracery,
// very bright panels so MODULATE yields intense amber/blue.
static void genWindow(unsigned int *t, int W) {
    const int m       = 2;        // stone frame thickness
    const int cxk     = W / 2;    // center column
    const int springY = W / 2;    // springline: pointed above, straight below
    const int fullHalf = (W / 2) - m;
    for (int y = 0; y < W; ++y) {
        // allowed half-width of the opening at this row (0 at apex -> full at spring)
        int half;
        if (y < m)                 half = -1;
        else if (y < springY)      half = ((y - m) * fullHalf) / (springY - m);
        else                       half = fullHalf;

        for (int x = 0; x < W; ++x) {
            int dx = x - cxk;
            int inside = (half >= 0 && dx > -half && dx < half && y >= m && y < W - m);

            int r, g, b;
            if (inside) {
                // bright glass panel (near white for strong modulation)
                r = 246; g = 244; b = 240;

                // lead came grid -> individual glass pieces
                if ((x % 6) == 0 || (y % 6) == 0)    { r = 70; g = 66; b = 60; }
                // central mullion (parteluz)
                if (dx == 0 || dx == -1)             { r = 55; g = 50; b = 45; }
                // transom bar at the springline
                if (y == springY || y == springY - 1){ r = 55; g = 50; b = 45; }

                // per-piece brightness jitter (keeps modulated color rich, still bright)
                int pv = (int)(noctis_hash(x / 6, y / 6) & 15) - 4;
                r += pv; g += pv; b += pv;
            } else {
                // surrounding stone frame (warm, medium-dark so it does not glow)
                r = 96; g = 86; b = 72;
                int n = (int)(noctis_hash(x, y) & 15) - 7;
                r += n; g += n; b += n;
            }
            t[y * W + x] = RGBA(noctis_clampi(r), noctis_clampi(g), noctis_clampi(b), 255);
        }
    }
}

// Roof: dark slate tiles in overlapping courses (hileras).
static void genRoof(unsigned int *t, int W) {
    for (int y = 0; y < W; ++y) {
        int row = y / 8;
        int off = (row & 1) ? 8 : 0;
        int ty  = y % 8;
        for (int x = 0; x < W; ++x) {
            int tx = (x + off) % 16;

            // medium cool slate (dark overall color comes from the base col too)
            int r = 120, g = 124, b = 134;

            // per-tile shade variation
            int tv = (int)(noctis_hash((x + off) / 16, row) & 15) - 7;
            r += tv; g += tv; b += tv;

            if (ty == 0)  { r += 25; g += 25; b += 28; }   // top highlight edge
            if (ty >= 6)  { r -= 40; g -= 40; b -= 38; }   // overlap shadow
            if (tx == 0)  { r -= 45; g -= 45; b -= 42; }   // vertical gap

            int n = (int)(noctis_hash(x, y) & 15) - 7;
            r += n; g += n; b += n;

            t[y * W + x] = RGBA(noctis_clampi(r), noctis_clampi(g), noctis_clampi(b), 255);
        }
    }
}

// ============================================================================
// PART 2 - AMBIENCE GEOMETRY (append to buf at index i)
// ============================================================================

// Brazier / lantern: iron post + bowl + layered emissive flame.
static void addBrazier(LineVertex *buf, int &i, float x, float z) {
    addSolidBox(buf, i, x, 0.0f, z, 0.18f, 0.18f, 1.30f, RGBA(58, 58, 70, 255)); // post
    addSolidBox(buf, i, x, 1.30f, z, 0.55f, 0.55f, 0.22f, RGBA(96, 86, 74, 255)); // bowl
    addPyramid(buf, i, x, 1.50f, z, 0.50f, 0.50f, 0.75f, RGBA(240, 150, 60, 255)); // outer flame
    addPyramid(buf, i, x, 1.60f, z, 0.28f, 0.28f, 0.60f,
               brighten(RGBA(255, 210, 110, 255), 1.35f));                          // inner flame
}

// Banner: dark hanging cloth with a red border trim.
static void addBanner(LineVertex *buf, int &i, float x, float y, float z, float w, float h) {
    float base = y - h;
    addSolidBox(buf, i, x, base, z, w, 0.05f, h, RGBA(38, 28, 52, 255));           // cloth
    addSolidBox(buf, i, x, y - 0.12f, z, w + 0.02f, 0.06f, 0.12f, RGBA(150, 30, 30, 255)); // top trim
    addSolidBox(buf, i, x, base, z, w + 0.02f, 0.06f, 0.12f, RGBA(150, 30, 30, 255));      // bottom trim
}

// Arch: pointed gothic archway - two pillars + stepped lintel rising to a point.
static void addArch(LineVertex *buf, int &i, float x, float z, float w, float h) {
    const float pw = 0.25f, d = 0.30f;
    addSolidBox(buf, i, x - (w * 0.5f), 0.0f, z, pw, d, h, RGBA(120, 110, 96, 255)); // left pillar
    addSolidBox(buf, i, x + (w * 0.5f), 0.0f, z, pw, d, h, RGBA(120, 110, 96, 255)); // right pillar
    addSolidBox(buf, i, x, h,          z, w + pw,      d, 0.25f, RGBA(130, 120, 104, 255)); // lintel
    addSolidBox(buf, i, x, h + 0.25f,  z, w * 0.70f,   d, 0.22f, RGBA(126, 116, 100, 255)); // step
    addSolidBox(buf, i, x, h + 0.47f,  z, w * 0.42f,   d, 0.20f, RGBA(122, 112,  96, 255)); // step
    addPyramid(buf, i, x, h + 0.67f,   z, w * 0.42f,   d, 0.60f, RGBA(118, 108,  92, 255)); // apex point
}
