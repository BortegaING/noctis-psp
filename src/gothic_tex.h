#pragma once
// Repeating gothic-cathedral facade: 4 vertical BAYS x 2 FLOORS of pointed (lancet) windows
// glowing warm amber behind dark stone tracery, framed by projecting buttresses/pilasters and
// horizontal string-courses. Fully deterministic (integer hash, no rand) and an EXACT function
// of the in-tile coords (bx = x mod 32, fy = y mod 64), so it is perfectly periodic and tiles
// seamlessly in both axes under GU_REPEAT. Colors are tuned for GU_TFX_MODULATE against the
// ~(162,153,140) stone vertex: the wall is kept DARK and warm, the glass is painted VERY BRIGHT
// and warm, so after the modulate (texel * ~160/255) the windows read as lit gas lamps and the
// wall stays in shadow. Warm/neutral everywhere: r >= g >= b, never a cool blue cast.
static void genGothicFacade(unsigned int *t, int W) {
    const int BW      = 32;      // bay width    -> 4 bays across W=128 (128/32)
    const int FH      = 64;      // floor height -> 2 floors down W=128 (128/64)
    const int bcx     = BW / 2;  // window horizontal center inside a bay (16)
    const int hw      = 8;       // window half-width of the straight body
    const int Pw      = 4;       // buttress half-width (the pier straddles each bay seam)
    const int ySpring = 26;      // arch springline (the straight sides begin here)
    const int yBot    = 54;      // window sill (bottom of the opening)

    // deterministic integer hash -> 0..255 (no rand), same primitive as genGround
    auto H = [](int a, int b) -> int {
        unsigned int h = (unsigned int)a * 374761393u + (unsigned int)b * 668265263u + 0x9E3779B9u;
        h = (h ^ (h >> 13)) * 1274126177u;
        h ^= (h >> 16);
        return (int)(h & 0xFFu);
    };
    auto C  = [](int v, int lo, int hi) -> int { return v < lo ? lo : (v > hi ? hi : v); };
    auto AB = [](int v) -> int { return v < 0 ? -v : v; };

    // Equilateral pointed-arch (lancet) membership in bay/floor-local coords: the straight body
    // below the springline, plus the region inside BOTH arcs struck from the opposite springline
    // points (radius = the span) above it -> a clean gothic point instead of a round top.
    auto inLancet = [&](int bx, int fy, int halfw, int bottom) -> bool {
        int dx = bx - bcx;
        if (fy >= ySpring && fy <= bottom && dx >= -halfw && dx <= halfw) return true;
        if (fy < ySpring) {
            long r2 = (long)(2 * halfw) * (2 * halfw);
            long dR = (long)(bx - (bcx + halfw)) * (bx - (bcx + halfw)) + (long)(fy - ySpring) * (fy - ySpring);
            long dL = (long)(bx - (bcx - halfw)) * (bx - (bcx - halfw)) + (long)(fy - ySpring) * (fy - ySpring);
            if (dR <= r2 && dL <= r2) return true;
        }
        return false;
    };

    for (int y = 0; y < W; ++y) {
        for (int x = 0; x < W; ++x) {
            int bx = x & (BW - 1);   // 0..31 column inside the bay  (periodic mod 32)
            int fy = y & (FH - 1);   // 0..63 row inside the floor    (periodic mod 64)
            int r, g, b;

            // ---- 1) dark warm ashlar wall (running-bond blocks, faint mortar) ----
            int crs  = fy >> 3;                              // 8px courses (0..7)
            int roff = (crs & 1) ? 8 : 0;                    // running-bond stagger
            int blk  = ((bx + roff) & (BW - 1)) >> 4;        // 0..1 block id inside the bay
            int fine = (H(bx, fy) % 9) - 4;                  // -4..+4 grain (periodic -> seamless)
            int tint = (H(blk * 7 + 1, crs * 5 + 3) % 9) - 4;// -4..+4 per-block warm tint
            int lum  = 62 + fine + tint;                     // DARK stone base
            if ((fy & 7) == 0)           lum -= 12;          // recessed horizontal mortar
            if (((bx + roff) & 15) == 0) lum -= 10;          // recessed vertical joint
            if ((fy & 7) == 1)           lum += 5;           // lit top lip of each block
            r = lum;  g = lum - 3;  b = lum - 8;             // warm/neutral cast (r>=g>=b)

            // ---- 2) projecting buttress / pilaster over each bay seam (verticality) ----
            int edge = bx < (BW - bx) ? bx : (BW - bx);      // distance to nearest bay boundary
            if (edge < Pw) {
                int sp = (bx < Pw) ? bx : bx - BW;           // -3..+3 across the joined pier
                int bl = 92 - (sp + 3) * 4;                  // directional relief, lit on the left
                if (AB(sp) == 3) bl -= 14;                   // dark reveal grooves at both edges
                r = bl + 3;  g = bl;  b = bl - 4;            // lighter, warm projecting pier
            }

            // ---- 3) horizontal string-course that separates the stacked floors ----
            if (fy <= 4) {
                int cl; switch (fy) { case 0: cl = 44;  break;   // shadow reveal under the band
                    case 1: cl = 104; break; case 2: cl = 98; break;
                    case 3: cl = 80;  break; default: cl = 60; }  // lit fascia -> shaded underside
                cl += (H(bx, fy) % 5) - 2;
                r = cl + 3;  g = cl;  b = cl - 5;            // lit projecting molding
            }

            // ---- 4) dark stone frame / arch surround hugging the opening ----
            bool open = inLancet(bx, fy, hw, yBot);
            if (!open && inLancet(bx, fy, hw + 2, yBot + 2)) {
                int fr = 50 + (H(bx, fy) % 7) - 3;
                fr += (bx < bcx) ? 5 : -5;                   // lit left jamb, shaded right jamb
                r = fr + 2;  g = fr;  b = fr - 4;
            }

            // ---- 5) BRIGHT warm amber glass: central glow + dark gothic tracery ----
            if (open) {
                int gdx = bx - bcx, gdy = fy - 34;           // glow centered mid-window
                int dd   = gdx * gdx * 4 + gdy * gdy;        // vertical ellipse (the window is tall)
                int fall = dd / 6;
                r = 255 - fall;            if (r < 180) r = 180;  // hot amber core -> warm rim
                g = 228 - fall * 11 / 10;  if (g < 140) g = 140;
                b = 172 - fall * 13 / 10;  if (b < 92)  b = 92;
                if (((bx + fy) & 7) == 0 || ((bx - fy) & 7) == 0) { r -= 38; g -= 34; b -= 26; } // faint lead cames (stained-glass panes)
                int a = AB(bx - bcx);
                bool mull = (a == 0);                        // central vertical mullion (mainel)
                bool tran = (fy == ySpring || fy == 40);     // two horizontal transoms
                if (mull || tran) { r = 46; g = 42; b = 38; }// thin dark stone tracery bars
            }

            // ---- 6) tiny glowing oculus in the spandrel above each arch ----
            {
                int odx = bx - bcx, ody = fy - 8;
                int od2 = odx * odx + ody * ody;
                if (od2 <= 9) {
                    if (od2 <= 4) { r = 214; g = 176; b = 110; }  // small glowing rose-window eye
                    else          { r = 50;  g = 46;  b = 42;  }  // dark stone ring
                }
            }

            r = C(r, 8, 255); g = C(g, 8, 255); b = C(b, 8, 255);
            t[y * W + x] = RGBA(r, g, b, 255);
        }
    }
}
