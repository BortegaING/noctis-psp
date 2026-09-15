#pragma once
// ENLOSADO GOTICO DE CATEDRAL: losas grandes de piedra en aparejo escalonado, junta de
// mortero oscura marcada, variacion de tono por losa, cenefa geometrica incisa en unas
// pocas, desgaste (centro pulido, bordes sucios), fisuras finas, una losa partida y
// manchas de humedad. Grano de arido fino en todo para que nada quede plano.
//
// La usa el SUELO y el TECHO del sector (ver sector.h) con GU_TFX_MODULATE: el pixel
// final es TEXTURA x COLOR DE VERTICE (suelo ~240 -> casi 1:1; techo ~120 -> la mitad).
// Por eso la textura va CLARA: media ~160, juntas ~110-128, NUNCA bajo ~100 (un piso
// oscuro se lee como "no hay piso"). Gris CALIDO neutro en todo: r >= g >= b.
//
// 16 BITS (RGB565): los degradados largos se bandean -> aqui todo va por escalones de
// contraste con el borde perturbado por hash, y el grano fino hace de dither.
//
// SEAMLESS: W es potencia de dos; TODA coordenada estructural y toda entrada de hash de
// baja frecuencia se envuelve mod W (mascaras & (W-1), celdas potencia de dos, NS par
// para que el aparejo escalonado sea periodico). El ruido blanco por pixel no correlaciona
// con el vecino, asi que tesela por construccion. Hash ENTERO: sin rand, sin heap.
static void genGround(unsigned int *t, int W) {
    const int SS = 32;                 // lado de losa en px -> 4x4 losas grandes en W=128
    const int NS = W / SS;             // 4 (PAR -> el escalonado cierra al envolver en Y)

    // hash entero determinista -> 0..255 (mismo primitivo que el resto del motor)
    auto H = [](int a, int b) -> int {
        unsigned int h = (unsigned int)a * 374761393u + (unsigned int)b * 668265263u + 0x9E3779B9u;
        h = (h ^ (h >> 13)) * 1274126177u;
        h ^= (h >> 16);
        return (int)(h & 0xFFu);
    };
    auto C   = [](int v, int lo, int hi) -> int { return v < lo ? lo : (v > hi ? hi : v); };
    auto MIN = [](int a, int b) -> int { return a < b ? a : b; };
    auto MAX = [](int a, int b) -> int { return a > b ? a : b; };
    auto AB  = [](int v) -> int { return v < 0 ? -v : v; };

    // ruido de valor TOROIDAL con celda de (1<<s) px e interpolacion entera: manchas
    // suaves (humedad/hollin) sin bloques y sin costura -> los indices de celda se
    // enmascaran con (W>>s)-1, que es potencia de dos, asi que x y x+W dan lo mismo.
    auto VN = [&](int px, int py, int s) -> int {
        const int m = (W >> s) - 1, F = (1 << s) - 1;
        int cx = px >> s, cy = py >> s, fx = px & F, fy = py & F;
        int a = H( cx      & m,  cy      & m), b = H((cx + 1) & m,  cy      & m);
        int c = H( cx      & m, (cy + 1) & m), d = H((cx + 1) & m, (cy + 1) & m);
        int u = a + (((b - a) * fx) >> s);
        int v = c + (((d - c) * fx) >> s);
        return u + (((v - u) * fy) >> s);      // 0..255
    };

    for (int y = 0; y < W; ++y) {
        for (int x = 0; x < W; ++x) {
            // ---- 1) rejilla de losas en APAREJO escalonado (running bond) ----
            int row = y / SS;                          // fila de losas 0..3
            int ox  = (row & 1) ? (SS / 2) : 0;        // medio paso en filas impares
            int lx  = (x - ox) & (SS - 1);             // col local (mod pot-2: envuelve negativos)
            int ly  = y & (SS - 1);                    // fila local
            int sc  = ((x - ox) & (W - 1)) / SS;       // id de losa X envuelto (seamless)
            int sr  = row & (NS - 1);                  // id de losa Y envuelto
            int sh  = H(sc * 7 + 3, sr * 5 + 1);       // identidad de la losa (tono/partida)
            int sh2 = H(sc * 11 + 5, sr * 13 + 2);     // segunda tirada (cenefa/fisura)

            // ---- 2) tono de la losa: cada una de su cantera (asi se lee el patron al caminar)
            int sv = sh % 12;                          // clase de losa (bien repartida en 4x4)
            int L  = 184 + (sh % 23) - 11;             // ~173..195 de base
            if      (sv == 9 || sv == 3) L += 13;      // losa clara, pulida por el paso
            else if (sv == 2)            L -= 38;      // losa de piedra oscura (la mas negra)
            else if (sv == 6 || sv == 0) L -= 21;      // losa media-oscura

            // ---- 3) DESGASTE: centro pulido/claro, orillas sucias. Anillos de Chebyshev
            //        (siguen la forma cuadrada de la losa) con el borde perturbado por hash
            //        -> escalones irregulares en vez de un degradado que se bandea en 565.
            int dx  = lx - SS / 2, dy = ly - SS / 2;
            int ax  = AB(dx), ay = AB(dy);
            int dch = MAX(ax, ay);                     // 0 (centro) .. 16 (junta)
            int wr  = dch + (H(x * 3 + 1, y * 5 + 7) % 7) - 3;
            if      (wr <=  5) L += 8;                 // corazon de la losa: brillo del uso
            else if (wr <=  9) L += 3;
            else if (wr >= 14) L -= 12;                // orilla: mugre acumulada
            else if (wr >= 12) L -= 6;

            // ---- 4) CENEFA: motivo geometrico inciso, NO en cada losa ----
            if (sh2 % 9 == 7) {                        // 3 losas de las 16 llevan cenefa
                if ((sh2 & 8) == 0) {                  // marco cuadrado doble
                    if      (dch == 11 || dch == 8) L -= 26;   // surco del cincel
                    else if (dch == 12 || dch == 9) L += 9;    // labio que pilla luz
                } else {                               // rombo inscrito + taco central
                    int dm = ax + ay;
                    if      (dm == 12 || dm == 9) L -= 26;
                    else if (dm == 13 || dm == 10) L += 9;
                    else if (dch <= 2)             L -= 15;    // tesela central oscura
                }
            }

            // ---- 5) JUNTAS de mortero: 2 px hundidos + canto iluminado. El tono de la
            //        junta NO hereda entero el de la losa (asi una losa oscura no abre un
            //        agujero negro): solo un cuarto de su desvio.
            int dvj = MIN(lx, SS - lx);                // dist a junta vertical (lx == 0)
            int dhj = MIN(ly, SS - ly);                // dist a junta horizontal (ly == 0)
            int gj  = MIN(dvj, dhj);
            int jn  = H(x + 17, y + 31) % 9 - 4;       // mortero irregular, nunca liso
            if      (gj == 0) L = 120 + ((L - 184) >> 2) + jn;   // fondo de junta (~112)
            else if (gj == 1) L = 138 + ((L - 184) >> 2) + jn;   // flanco de junta
            else if (gj == 2) L -= 6;                            // sombra al pie de la losa
            else if (gj == 3) L += 5;                            // canto de la losa

            // ---- 6) FISURA fina y LOSA PARTIDA (serpenteo por hash, nunca una recta) ----
            if (gj > 2) {
                int jt = (H(x >> 1, y >> 1) % 5) - 2;
                if (sh2 % 7 == 4 || sh2 % 7 == 5) {    // fisura de pelo: 4 losas de las 16
                    // el serpenteo se saca de la coordenada A LO LARGO de la grieta: asi hay
                    // exactamente un pixel por paso (linea continua y quebrada, no punteada).
                    int ln, jl;
                    switch ((sh >> 2) & 3) {
                        case 0:  ln = dy;      jl = (H( x        >> 1, sr + 41) % 5) - 2; break;  // horizontal
                        case 1:  ln = dx;      jl = (H(sc + 71,   y  >> 1)      % 5) - 2; break;  // vertical
                        case 2:  ln = dx - dy; jl = (H((x + y)    >> 2, sc + sr + 7) % 5) - 2; break;  // diagonal
                        default: ln = dx + dy; jl = (H((x - y)    >> 2, sr * 3 + 13) % 5) - 2; break;  // anti-diagonal
                    }
                    if (ln + jl == 0) L -= 28;
                }
                if (sh % 23 == 5) {                    // LOSA PARTIDA: exactamente 1 en la tesela
                    int ln = dx + dy + jt;
                    if      (ln > -2 && ln < 2) L -= 36;   // el corte
                    else if (ln >= 2)           L -= 10;   // mitad hundida (junta abierta)
                    else                        L += 6;    // mitad levantada, pilla luz
                }
            }

            // ---- 7) HUMEDAD: manchas amplias que cruzan varias losas (ruido toroidal).
            //        El borde va perturbado por hash y el salto es por ESCALONES: un
            //        degradado suave se bandearia feo en RGB565.
            int damp = VN(x, y, 5) + (H(x * 5 + 9, y * 7 + 3) % 9) - 4;   // celdas de 32 px
            int wet  = 0;
            if      (damp > 196) { wet = 2; L -= 17; } // nucleo de la mancha
            else if (damp > 172) { wet = 1; L -=  8; } // orla
            if (wet && gj <= 2)  L -= 6;               // el agua se queda en la junta
            int soot = VN(x + 53, y + 91, 4);          // veladura de hollin, celdas de 16
            L += (soot >> 5) - 4;                      // ~-4..+3

            // ---- 8) grano de arido fino: rompe lo plano y hace de DITHER contra el
            //        bandeado de RGB565 (R y B tienen paso 8) ----
            L += (H(x, y) % 13) - 6;

            // ---- 9) empaque en gris CALIDO: r >= g >= b SIEMPRE. La humedad DESCOLORA
            //        (acerca los canales) pero nunca invierte el orden -> jamas el gris
            //        azulado de la piedra fria. Los huecos r-g y g-b se dejan anchos (>=6)
            //        a proposito: en RGB565 el verde tiene 6 bits (paso 4) y el rojo 5
            //        (paso 8); con huecos chicos el redondeo levantaria el verde por encima
            //        del rojo y las zonas planas saldrian verdosas. El clamp inferior (106)
            //        es la garantia de que el piso nunca se lee como vacio.
            int r = L + 5 - wet, g = L - 3, b = L - 12 + 3 * wet;
            r = C(r, 106, 220); g = C(g, 102, 214); b = C(b, 98, 206);
            t[y * W + x] = RGBA(r, g, b, 255);
        }
    }
}
