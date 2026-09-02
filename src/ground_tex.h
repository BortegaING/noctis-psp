#pragma once
// PISO BRUTALISTA: grandes losas de hormigon crudo humedo con juntas de dilatacion
// (rejilla ESCALONADA), vetas de mugre, brillo tenue de charco y fisuras finas.
// Seamless: todo el hashing/coords se envuelve mod W (W potencia de dos) -> tesela
// perfecta en ambos ejes bajo GU_REPEAT. Hash ENTERO (sin rand, sin floats-de-tiempo).
// El piso se dibuja con GU_TFX_REPLACE: lo que se genera aqui ES el brillo FINAL ->
// objetivo ~150-190 de media; NUNCA por debajo de ~120 o se lee como vacio.
// Gris calido neutro en todo: r >= g >= b, sin tinte azul frio.
static void genGround(unsigned int *t, int W) {
    const int SS = 32;                 // lado de losa en px -> 4x4 losas grandes en W=128 (escalonadas)
    const int NS = W / SS;             // 4 (potencia de dos -> las mascaras envuelven)

    // hash entero determinista -> 0..255 (mismo primitivo que el resto del motor)
    auto H = [](int a, int b) -> int {
        unsigned int h = (unsigned int)(a * 374761393 + b * 668265263 + 0x9E3779B9u);
        h = (h ^ (h >> 13)) * 1274126177u;
        h ^= (h >> 16);
        return (int)(h & 0xFFu);
    };
    auto C   = [](int v, int lo, int hi) -> int { return v < lo ? lo : (v > hi ? hi : v); };
    auto MIN = [](int a, int b) -> int { return a < b ? a : b; };

    for (int y = 0; y < W; ++y) {
        for (int x = 0; x < W; ++x) {
            int row = y / SS;                          // fila de losas 0..3
            int ox  = (row & 1) ? (SS / 2) : 0;        // aparejo escalonado (running-bond); NS par -> periodico
            int lx  = (x - ox) & (SS - 1);             // col local en la losa (mod pot-2 envuelve negativos)
            int ly  = y & (SS - 1);                    // fila local en la losa
            int sc  = ((x - ox) & (W - 1)) / SS;       // id de losa X envuelto (seamless)
            int sr  = row & (NS - 1);                  // id de losa Y envuelto

            // ---- 1) tono base por losa: hormigon gris calido medio-alto ----
            int slabH = H(sc * 7 + 3, sr * 5 + 1);
            int L = 172 + (slabH % 25) - 12;           // ~160..184 por losa (colada distinta)
            if ((slabH & 7) == 0) L += 8;              // alguna losa mas clara

            // ---- 2) grano fino de arido (ruido blanco = seamless por construccion) ----
            L += H(x, y) % 9 - 4;

            // ---- 3) juntas de dilatacion: surco a los bordes de losa + labio iluminado ----
            int dvj = MIN(lx, SS - lx);                // dist a junta vertical (lx==0)
            int dhj = MIN(ly, SS - ly);                // dist a junta horizontal (ly==0)
            int gj  = MIN(dvj, dhj);
            if      (gj == 0) L -= 42;                 // fondo del surco (oscuro pero > vacio)
            else if (gj == 1) L -= 26;
            else if (gj == 2) L -= 12;
            else if (gj == 3) L += 5;                  // chaflan iluminado junto al surco

            // ---- 4) vetas de mugre (bloques toroidales de baja frecuencia -> seamless) ----
            int bxg   = (x >> 3) & 15, byg = (y >> 4) & 7;
            int grime = H(bxg * 13 + 7, byg * 11 + 5) % 11 - 6;   // sobre todo mas oscuro (-6..+4)
            L += grime;

            // ---- 5) charcos: regiones amplias con lamina humeda + fleck especular raro ----
            int pud    = H((x >> 5) & 3, (y >> 5) & 3);
            int puddle = 0;
            if (pud > 196) {                           // ~1/4 de las zonas grandes son charco
                puddle = 1;
                L += 8;                                // agua algo mas clara (sheen)
                if (gj > 3 && (H(x, y) & 1023) > 1017) L += 30;  // destello especular puntual
            }

            // ---- 6) fisura fina ocasional cruzando el interior de la losa ----
            if ((slabH & 15) == 5 && gj > 4) {
                int dxc = lx - SS / 2, dyc = ly - SS / 2;
                int line; switch (H(sc + 1, sr + 2) & 3) {
                    case 0:  line = dyc;       break;  // horizontal
                    case 1:  line = dxc;       break;  // vertical
                    case 2:  line = dxc - dyc; break;  // diagonal
                    default: line = dxc + dyc; break;  // anti-diagonal
                }
                if (line > -1 && line < 1) L -= 22;    // hilo oscuro fino
            }

            // ---- 7) empaque gris calido humedo (r>=g>=b); mugre agrega verdin, charco enfria un pelo ----
            int r = L, g = L - 4, b = L - 9;
            if (grime < -3) { g += 2; b -= 1; }        // verdin/humedad en las zonas oscuras
            if (puddle)     { b += 3; }                // reflejo del charco un poco menos calido
            r = C(r, 120, 205); g = C(g, 116, 200); b = C(b, 110, 196);  // piso CLARO (REPLACE), nunca vacio
            t[y * W + x] = RGBA(r, g, b, 255);
        }
    }
}
