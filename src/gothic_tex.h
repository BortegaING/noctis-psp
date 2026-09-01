#pragma once
// One repeating gothic-cathedral wall bay (lancet window + buttresses + cornice/plinth); tiles seamlessly, mid-tone for MODULATE.
static void genGothicFacade(unsigned int *t, int W) {
    const int cx    = W / 2;   // window horizontal center (64)
    const int hw    = 18;      // window half-width of the straight body
    const int yArch = 54;      // arch springline (straight sides begin here)
    const int yBot  = 110;     // window sill
    const int bhw   = 12;      // buttress half-width (the pier straddles the L/R seam)

    // deterministic integer hash -> 0..255 (no rand)
    auto H = [](int a, int b) -> int {
        unsigned int h = (unsigned int)(a * 374761393 + b * 668265263 + 0x9E3779B9u);
        h = (h ^ (h >> 13)) * 1274126177u;
        h ^= (h >> 16);
        return (int)(h & 0xFFu);
    };
    auto C = [](int v, int lo, int hi) -> int { return v < lo ? lo : (v > hi ? hi : v); };

    // pointed-arch (equilateral lancet) membership: two arcs struck from the
    // opposite springline points, radius = the span. Above the springline a
    // pixel is inside when it lies within BOTH circles -> a clean gothic point.
    auto inLancet = [&](int x, int y, int halfw, int bottom) -> bool {
        if (y >= yArch && y <= bottom && (x - cx <= halfw) && (cx - x <= halfw)) return true;
        if (y < yArch) {
            long r2 = (long)(2 * halfw) * (2 * halfw);
            long dR = (long)(x - (cx + halfw)) * (x - (cx + halfw)) + (long)(y - yArch) * (y - yArch);
            long dL = (long)(x - (cx - halfw)) * (x - (cx - halfw)) + (long)(y - yArch) * (y - yArch);
            if (dR <= r2 && dL <= r2) return true;
        }
        return false;
    };

    for (int y = 0; y < W; ++y) {
        for (int x = 0; x < W; ++x) {
            int r, g, b;

            // ---- 1) weathered ashlar stone background (cool, B >= G >= R) ----
            int fine  = (H(x, y) % 13) - 6;              // -6..+6 grain
            int block = (H(x >> 4, y >> 4) % 11) - 5;    // -5..+5 per-stone tint (tiles per 16px)
            int lum   = 90 + fine + block;
            int course = (y & 15);                       // 16px courses (divides W)
            int shift  = ((y >> 4) & 1) ? 16 : 0;        // running-bond alternating offset
            int vj     = ((x + shift) & 31);
            if (course == 0 || course == 15) lum -= 16;  // recessed horizontal mortar
            if (vj == 0)                     lum -= 14;  // recessed vertical joint
            if (course == 1)                 lum += 6;   // lit top edge of each block
            r = lum - 1; g = lum; b = lum + 3;           // faint cool cast

            // ---- 2) buttress / pilaster strips, split half-and-half over the seam ----
            int ed = (x <= W - 1 - x) ? x : (W - 1 - x); // distance to nearest vertical edge
            if (ed < bhw) {
                int pos = (x < cx) ? (bhw + x) : (x - (W - bhw)); // 0..2*bhw-1 across the joined pier
                int bl  = 116 - pos * 2;                  // directional relief (lit on the left)
                if (course == 0 || course == 15) bl -= 12; // carry the string course across
                if (pos == 0 || pos == 2 * bhw - 1) bl -= 18; // dark edge reveal both sides
                r = bl + 2; g = bl; b = bl - 1;           // pier stone, a touch warm where it is lit
            }

            // ---- 3) cornice band (top) and plinth band (bottom): stacked tiles = floors ----
            if (y <= 6) {
                int cl; switch (y) { case 0: cl = 98; break; case 1: cl = 122; break;
                    case 2: cl = 128; break; case 3: cl = 72; break; case 4: cl = 120; break;
                    case 5: cl = 112; break; default: cl = 66; }
                cl += (H(x, y) % 7) - 3;
                r = cl - 1; g = cl; b = cl + 3;
            } else if (y >= W - 7) {
                int yy = y - (W - 7);
                int cl; switch (yy) { case 0: cl = 68; break; case 1: cl = 118; break;
                    case 2: cl = 124; break; case 3: cl = 114; break; case 4: cl = 74; break;
                    case 5: cl = 100; break; default: cl = 92; }
                cl += (H(x, y) % 7) - 3;
                r = cl - 1; g = cl; b = cl + 3;
            }

            // ---- 4) dark stone frame / arch surround around the opening ----
            bool open = inLancet(x, y, hw, yBot);
            if (!open && inLancet(x, y, hw + 4, yBot + 4)) {
                int fl = 60 + (H(x, y) % 9) - 4;
                fl += (x < cx) ? 6 : -6;                  // lit left cheek, shaded right cheek
                r = C(fl + 3, 44, 120); g = C(fl, 44, 120); b = C(fl - 2, 44, 120);
            }

            // ---- 5) faint amber glass with a soft central glow + stone tracery ----
            if (open) {
                int dx = x - cx, dyc = y - 82;            // vertical center of the light
                int glow = 164 - (dx * dx + dyc * dyc) / 40;
                if (glow < 120) glow = 120;
                int tt = glow - 120;                      // 0..44 brighter toward center
                r = 118 + tt;                             // warm amber (R > G > B)
                g = 92 + (tt * 7) / 10;
                b = 56 + (tt * 3) / 10;
                int a = dx < 0 ? -dx : dx;
                bool mull = (a == 0 || a == 6 || a == 12); // vertical mullions
                bool tran = (y == 74 || y == 96);          // horizontal transoms
                if (mull || tran) { r = 54; g = 50; b = 46; } // thin dark stone bars
            }

            // ---- 6) little quatrefoil roundel / oculus above the arch ----
            {
                int rdx = x - cx, rdy = y - 13;
                int rd2 = rdx * rdx + rdy * rdy;
                if (rd2 <= 36) {                          // radius 6 stone disc
                    if (rd2 <= 9) { r = 150; g = 118; b = 70; }   // radius 3 glowing eye
                    else          { r = 58;  g = 54;  b = 52; }   // dark stone ring
                }
            }

            r = C(r, 22, 165); g = C(g, 22, 165); b = C(b, 22, 165);
            t[y * W + x] = RGBA(r, g, b, 255);
        }
    }
}
