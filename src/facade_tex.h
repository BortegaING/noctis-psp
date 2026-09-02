#pragma once
// FACHADA BRUTALISTA -> REEMPLAZA TODA la geometria de ventanas (el usuario prohibio
// "40 objetos de ventanas": el detalle vive en la TEXTURA, no en la malla).
// Hormigon ENCOFRADO (lineas finas de tablilla + agujeros de amarre) con ventanas
// ojivales (lancet) ALTAS y ESTRECHAS pintadas en un ritmo regular de vanos: casi
// todas oscuras/recesadas, unas POCAS con brillo AMBAR (el unico acento saturado).
// Bandas de sombra de contrafuerte + costuras de MEGA-PANEL (escala BLAME) para
// verticalidad. Determinista (hash entero, sin rand) y funcion EXACTA de
// (x mod 32, y mod 64) -> tesela perfecta en ambos ejes bajo GU_REPEAT.
// Brillo MEDIO ~90-160: se dibuja con GU_TFX_MODULATE contra el vertice de piedra
// (baseColor x2.7), asi el producto se lee sin recorte; el ambar encendido sube mas.
// Calido/neutro en todo: r >= g >= b.
static void genFacade(unsigned int *t, int W) {
    const int BW      = 32;      // ancho de vano    -> 4 vanos en W=128
    const int FH      = 64;      // alto de piso     -> 2 pisos en W=128
    const int bcx     = BW / 2;  // centro horizontal de la ventana en el vano (16)
    const int hw      = 5;       // semiancho de la ventana (ESTRECHA -> lancet alta)
    const int ySpring = 22;      // arranque del arco (los lados rectos empiezan aqui)
    const int yBot    = 52;      // alfeizar (fondo del vano)

    // hash entero determinista -> 0..255 (mismo primitivo que genGround)
    auto H = [](int a, int b) -> int {
        unsigned int h = (unsigned int)(a * 374761393 + b * 668265263 + 0x9E3779B9u);
        h = (h ^ (h >> 13)) * 1274126177u;
        h ^= (h >> 16);
        return (int)(h & 0xFFu);
    };
    auto C   = [](int v, int lo, int hi) -> int { return v < lo ? lo : (v > hi ? hi : v); };
    auto AB  = [](int v) -> int { return v < 0 ? -v : v; };
    auto MIN = [](int a, int b) -> int { return a < b ? a : b; };

    // arco ojival (lancet) equilatero en coords locales de vano/piso: cuerpo recto bajo
    // el arranque + region interior a AMBOS arcos trazados desde los puntos opuestos
    // del arranque (radio = la luz) -> punta gotica limpia en vez de un tope redondo.
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
            int bx  = x & (BW - 1);      // 0..31 columna dentro del vano (periodico mod 32)
            int fy  = y & (FH - 1);      // 0..63 fila dentro del piso    (periodico mod 64)
            int bay = (x >> 5) & 3;      // indice de vano 0..3 (envuelto -> seamless)
            int flr = (y >> 6) & 1;      // indice de piso 0..1
            int r, g, b;

            // ---- 1) HORMIGON ENCOFRADO: base media + tablillas horizontales + grano ----
            int tnt = H(bay * 5 + 1, flr * 7 + 3) % 7 - 3;   // tinte calido por panel
            int lum = 112 + tnt + (H(x, y) % 7 - 3);         // hormigon MEDIO (para el MODULATE)
            int fb  = fy & 7;                                // tablillas del encofrado (8px)
            if (fb == 0)       lum -= 10;                    // junta de tablilla (sombra recesada)
            else if (fb == 1)  lum += 5;                     // labio iluminado bajo la junta
            if ((x & 15) == 0) lum -= 4;                     // costura vertical de tablero de encofrado
            r = lum; g = lum - 3; b = lum - 7;               // calido/neutro (r>=g>=b)

            // ---- 2) agujeros de amarre (cono oscuro) en rejilla 16x16, centrados en la tablilla ----
            {
                int tx = (x & 15) - 8, ty = ((y + 4) & 15) - 8;
                int td = tx * tx + ty * ty;
                if      (td <= 1) { r -= 34; g -= 32; b -= 28; }   // ojo del amarre
                else if (td <= 4) { r -= 14; g -= 13; b -= 11; }   // aro conico
            }

            // ---- 3) contrafuerte PINTADO: relieve direccional en cada junta de vano (verticalidad) ----
            int edge = MIN(bx, BW - bx);                     // dist al borde de vano mas cercano
            if (edge < 4) {
                int sp = (bx < 4) ? bx : bx - BW;            // -3..+3 cruzando el pilar unido en la junta
                int bl = 108 - (sp + 3) * 5;                 // iluminado a la izquierda, sombreado a la derecha
                if (AB(sp) == 3) bl -= 16;                   // ranura de sombra en ambos cantos
                r = bl + 2; g = bl - 1; b = bl - 6;          // pilar calido que "proyecta"
            }

            // ---- 4) costuras de MEGA-PANEL (escala BLAME): reja profunda cada 64px ----
            int mvx  = x & 63, mhy = y & 63;
            int seam = MIN(MIN(mvx, 64 - mvx), MIN(mhy, 64 - mhy));
            if      (seam == 0) { r -= 30; g -= 28; b -= 24; }  // fondo de la costura (mole ensamblada)
            else if (seam == 1) { r -= 16; g -= 15; b -= 12; }
            else if (seam == 2) { r += 6;  g += 5;  b += 4;  }  // chaflan iluminado del panel

            // ---- 5) VENTANAS PINTADAS: derrame recesado + vidrio (oscuro o ambar encendido) ----
            bool open  = inLancet(bx, fy, hw, yBot);
            bool frame = !open && inLancet(bx, fy, hw + 2, yBot + 2);
            if (frame) {                                     // jamba/derrame de piedra recesado
                int fr = 58 + (H(bx, fy) % 5 - 2);
                fr += (fy < ySpring) ? 6 : -8;               // dintel iluminado, alfeizar en sombra
                r = fr + 2; g = fr; b = fr - 4;
            }
            if (open) {
                int wsel = H(bay * 7 + 3, flr * 5 + 1) % 6;  // ritmo determinista de encendidas
                bool lit = (wsel == 0);                      // ~1 de 6 vanos con luz (pocas)
                if (lit) {
                    int gdx = bx - bcx, gdy = fy - 33;       // brillo centrado a media ventana
                    int dd   = gdx * gdx * 4 + gdy * gdy;    // elipse vertical (la ventana es alta)
                    int fall = dd / 6;
                    r = 235 - fall;            if (r < 150) r = 150;   // nucleo ambar caliente -> borde calido
                    g = 180 - fall * 11 / 10;  if (g < 108) g = 108;
                    b = 96  - fall * 13 / 10;  if (b < 56)  b = 56;
                    if (gdx == 0 || fy == 34 || fy == ySpring) { r -= 40; g -= 36; b -= 28; } // parteluz + travesanos
                    if (((bx + fy) & 7) == 0)                  { r -= 20; g -= 18; b -= 14; } // plomos tenues (vidriera)
                } else {
                    r = 44; g = 40; b = 35;                  // vidrio oscuro / recess profundo
                    if (bx == bcx - 1) { r += 16; g += 15; b += 12; } // reflejo fino vertical
                    if (bx == bcx)     { r = 32; g = 29; b = 26; }    // parteluz oscuro
                }
            }

            r = C(r, 24, 245); g = C(g, 22, 240); b = C(b, 20, 235);
            t[y * W + x] = RGBA(r, g, b, 255);
        }
    }
}
