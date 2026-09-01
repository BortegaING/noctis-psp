#pragma once
// Dark wet gothic cobblestone/flagstone plaza floor: seamless wrapped-Voronoi stones, dark mortar joints, grime patches, hairline cracks + wet flecks; mid-tone for GU_REPEAT + MODULATE.
static void genGround(unsigned int *t, int W) {
    const int CS = 16;            // cobble cell size (~16px stones -> reads as cobbles when tiled far)
    const int NC = W / CS;        // 8 cells across; hashing wraps mod NC so the field tiles seamlessly

    // deterministic integer hash -> 0..255 (no rand)
    auto H = [](int a, int b) -> int {
        unsigned int h = (unsigned int)(a * 374761393 + b * 668265263 + 0x9E3779B9u);
        h = (h ^ (h >> 13)) * 1274126177u;
        h ^= (h >> 16);
        return (int)(h & 0xFFu);
    };
    auto C = [](int v, int lo, int hi) -> int { return v < lo ? lo : (v > hi ? hi : v); };

    for (int y = 0; y < W; ++y) {
        for (int x = 0; x < W; ++x) {
            int pcx = x >> 4, pcy = y >> 4;   // pixel's home cell (CS = 16)

            // ---- 1) seamless Voronoi: nearest (F1) + 2nd nearest (F2) jittered site ----
            // Site jitter/offset are hashed on the WRAPPED cell id (& (NC-1)) but placed at the
            // UNWRAPPED cell base, so the whole field is exactly periodic over W -> tiles both ways.
            long f1 = 1L << 30, f2 = 1L << 30;
            int nWX = 0, nWY = 0, nfx = 0, nfy = 0;   // nearest cell id + site (for tone/cracks)
            for (int cj = -2; cj <= 2; ++cj) {
                for (int ci = -2; ci <= 2; ++ci) {
                    int cellX = pcx + ci, cellY = pcy + cj;
                    int wx = cellX & (NC - 1);        // NC is a power of two -> also wraps negatives
                    int wy = cellY & (NC - 1);
                    int jx  = 3 + H(wx, wy) % 10;             // 3..12 in-cell jitter
                    int jy  = 3 + H(wx + 53, wy + 29) % 10;   // 3..12
                    int off = (cellY & 1) ? (CS / 2) : 0;     // running-bond stagger (NC even -> periodic)
                    int fx  = cellX * CS + jx + off;
                    int fy  = cellY * CS + jy;
                    long dx = x - fx, dy = y - fy;
                    long d2 = dx * dx + dy * dy;
                    if (d2 < f1)      { f2 = f1; f1 = d2; nWX = wx; nWY = wy; nfx = fx; nfy = fy; }
                    else if (d2 < f2) { f2 = d2; }
                }
            }
            long e = f2 - f1;                         // squared-dist edge metric: ~0 at a joint

            // ---- 2) per-stone tone (integer hash of the nearest cell) ----
            int th = H(nWX * 7 + 3, nWY * 5 + 1);
            int L  = 86 + (th % 34) - 12;             // ~74..107 cold-neutral stone
            if ((th & 7) == 0) L += 20;              // a few worn/polished lighter tops

            // ---- 3) rounded stones + dark mortar joints (ramp mortar->stone by edge metric) ----
            int Lo = L;
            if (e < 100) {
                int m = (int)e;                       // 0..99
                Lo = 40 + (L - 40) * m / 100;         // deep joint at e~0, rounded rim as e grows
            }

            // ---- 4) coarse grime / wet patches (seamless: 32px grid divides W) ----
            int gp    = H(((x >> 5) & 3) * 13 + 7, ((y >> 5) & 3) * 11 + 5) % 16;
            int grime = gp - 11;                      // mostly darker (-11..+4)
            Lo += grime;

            // ---- 5) hairline cracks across ~1/16 of the stones (interior only) ----
            if ((th & 15) == 5 && e > 40) {
                int dxc = x - nfx, dyc = y - nfy;
                int line; switch (H(nWX + 1, nWY + 2) & 3) {
                    case 0:  line = dyc;        break; // horizontal
                    case 1:  line = dxc;        break; // vertical
                    case 2:  line = dxc - dyc;  break; // diagonal
                    default: line = dxc + dyc;  break; // anti-diagonal
                }
                if (line > -1 && line < 1) Lo -= 20;  // faint thin dark fissure
            }

            // ---- 6) rare wet-sheen specular fleck on stone tops (for the wet look) ----
            if (e > 130 && (H(x * 3 + 1, y * 3 + 7) & 1023) > 1018) Lo += 34;

            // ---- 7) pack: warm-neutral wet grey (r>g>b, base ~96,92,86); grime adds a green damp ----
            int r = Lo, g = Lo - 4, b = Lo - 10;
            if (grime < -4) { g += 3; b -= 1; }       // faint mossy/damp green in the darker patches
            r = C(r, 28, 150); g = C(g, 28, 150); b = C(b, 28, 150);
            t[y * W + x] = RGBA(r, g, b, 255);
        }
    }
}
