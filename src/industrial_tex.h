#pragma once
// SILLAR GOTICO MONUMENTAL -- LA PIEL PRINCIPAL DEL SECTOR (muros exteriores, las 4 masas
// y las columnas de la arcada).  Se ve de CERCA en los pasillos, asi que TODO el detalle
// vive en la TEXTURA: no hay geometria por bloque.  Referencia de arte: gotico oscuro
// monumental, piedra antigua y GASTADA en niebla gris-parda calida, siluetas casi negras y
// un unico acento saturado (el puntito AMBAR de la ventana encendida).  Demasiado grande,
// demasiado antiguo, demasiado silencioso: se combate la repeticion con VEJEZ ASIMETRICA.
//   - SILLERIA (ashlar): 9 HILADAS de ALTURA DISTINTA (14/12/18/14/12/16/14/14/14 = 128),
//     llagas de ANCHO DESIGUAL y DESPLAZADAS por hilada (4 llagas con jitter por hash),
//     tono por bloque, "spolia" (bloques ahollinados oscuros / piedra fresca clara),
//     juntas hondas de profundidad VARIABLE y ESQUINAS DESPORTILLADAS (arriba-izq / abajo-der).
//   - CORNISA en la costura horizontal (chaflan + cara + goteron) que se completa con la
//     TABLA DE CANECILLOS y la sombra de vuelo del otro lado de la costura -> banda continua.
//   - IMPOSTA a media altura (vuelo iluminado + sombra) y ARQUERIA CIEGA: fila de arcos
//     OJIVALES ciegos poco profundos, con baqueton, enjutas, abaco, plinto y COLUMNILLAS.
//   - VENTANA OJIVAL estrecha y PROFUNDA: derrame (splay) + jamba + alfeizar volado; el
//     vidrio lee OSCURO Y FRIO en la cabeza y el fondo del vano tiene un resplandor AMBAR
//     (parteluz, travesano y emplomado recortados contra el) -> UNICO acento calido.
//   - NICHO con figura tallada PALIDA bajo dosel volado, sobre peana.
//   - DESGASTE: chorreras verticales de agua bajo cornisa / imposta / plinto / alfeizar,
//     MUSGO humedo verdoso en juntas y bandas que escurren, FISURAS finas asimetricas,
//     picaduras.  Nada de esto es simetrico: el ojo no lee la tesela como un sello limpio.
// Determinista (hash entero, sin rand, sin heap; sin dependencia real de <math.h>).
// FUNCION EXACTA de (x mod W, y mod W) -> TESELA PERFECTA en ambos ejes (ver NOTA final).
// Brillo pensado para GU_TFX_MODULATE contra un color de vertice YA ACLARADO:
//   piedra media ~110-150 | juntas, llagas y recesos ~70-90 | fondo del nicho y cabeza
//   fria del vano ~58-75 | cornisa/imposta/alfeizar/figura iluminados ~140-155 | AMBAR
//   hasta ~208.  El GRUESO queda MEDIO (nunca bajo ~90): el pasillo no se va a negro.
//   Piedra CALIDA parda (r > g > b); el musgo es la unica nota verde.  OJO 16 BITS: la
//   textura se trunca a RGB565 (5/6/5 bits), asi que no hay degradados suaves largos:
//   contraste CLARO por escalones y ruido fino de +-4 (que ademas hace de dither).
static void genIndustrial(unsigned int *t, int W) {
    // ---- geometria del modulo (derivada de W; comentarios = valores exactos con W=128) ----
    const int NC = 9;                                        // hiladas de silleria
    const int courseY[9] = { 0, W*7/64, W*13/64, W*11/32, W*29/64,
                             W*35/64, W*43/64, W*25/32, W*57/64 };  // 0,14,26,44,58,70,86,100,114
    const int corY  = W - W*7/128;      // 121  CORNISA (ocupa hasta y=W-1, en la costura)
    const int corbB = W*5/128;          // 5    canecillos + sombra de vuelo (y 0..5)
    const int aTop  = W*21/128;         // 21   ARQUERIA CIEGA: abaco corrido
    const int aSpr  = W*19/64;          // 38   arranque de los arcos ciegos
    const int aBas  = W*51/128;         // 51   base de los arcos (plinto en 52/53)
    const int aRise = W*7/64;           // 14   flecha de la ojiva ciega (punta en y=24)
    const int aHW   = W*5/64;           // 10   semiluz del arco ciego (periodo 32 px)
    const int impY  = W*31/64;          // 62   IMPOSTA a media altura
    const int wcx   = W*5/16;           // 40   eje de la VENTANA (descentrada: asimetria)
    const int wHW   = W/32;             // 4    semiluz del vano (estrecho -> alto)
    const int wSpr  = W*21/32;          // 84   arranque del arco de la ventana
    const int wRise = W/8;              // 16   flecha -> punta en y=68
    const int wSill = W*7/8;            // 112  alfeizar
    const int wMid  = W*3/4;            // 96   travesano / foco del resplandor ambar
    const int nx    = W*27/32;          // 108  eje del NICHO
    const int nTop  = W*9/16;           // 72   arranque del nicho (dosel en 68..71)
    const int nBot  = W*13/16;          // 104  peana del nicho
    const int nHW   = W*7/128;          // 7    semiluz del nicho

    // ---- utilidades enteras ----
    auto H = [](int a, int b) -> int {                 // hash determinista -> 0..255
        unsigned int h = (unsigned int)a * 374761393u + (unsigned int)b * 668265263u + 0x9E3779B9u;
        h = (h ^ (h >> 13)) * 1274126177u; h ^= (h >> 16);
        return (int)(h & 0xFFu);
    };
    auto C  = [](int v, int lo, int hi) -> int { return v < lo ? lo : (v > hi ? hi : v); };
    auto AB = [](int v) -> int { return v < 0 ? -v : v; };
    auto MX = [](int a, int b) -> int { return a > b ? a : b; };
    // chorrera de agua: envolvente vertical desde una repisa 'yL', sube en 'rise' y decae en 'run'
    auto drip = [](int yy, int yL, int rise, int run) -> int {
        if (yy < yL || yy > yL + run) return 0;
        int d = yy - yL;
        int e = (d < rise) ? (d * 64 / rise) : (64 - (d - rise) * 64 / (run - rise));
        return e < 0 ? 0 : e;
    };
    // distancia^2 entera de un punto a un segmento -> FISURAS finas interiores
    auto SEG2 = [](int px, int py, int ax, int ay, int bx, int by) -> int {
        long long vx = bx - ax, vy = by - ay, wx = px - ax, wy = py - ay;
        long long c1 = vx*wx + vy*wy;
        if (c1 <= 0) return (int)(wx*wx + wy*wy);
        long long c2 = vx*vx + vy*vy;
        if (c2 <= c1) { long long ex = px - bx, ey = py - by; return (int)(ex*ex + ey*ey); }
        long long d2 = (wx*wx + wy*wy) - (c1*c1) / c2;
        return d2 < 0 ? 0 : (int)d2;
    };
    auto crack = [&](int px, int py, int ax, int ay, int bx, int by) -> int {   // 0/4/12/20
        int d2 = SEG2(px, py, ax, ay, bx, by);
        if (d2 <= 0) return 20;
        if (d2 <= 1) return 12;
        if (d2 <= 2) return 4;
        return 0;
    };
    // OJIVA parabolica: semiluz efectiva en la fila 'py' del anillo de semiancho 'hw'
    // (punta limpia, no semicirculo).  El anillo exterior (hw > hw0) sube y baja MAS ->
    // arcos CONCENTRICOS que no convergen en el mismo punto.  -1 = fuera del hueco.
    auto ogive = [](int py, int hw, int hw0, int ySpr, int rise, int yBot) -> int {
        int ex = hw - hw0;
        if (py > yBot + ex) return -1;
        if (py >= ySpr)     return hw;
        int d = ySpr - py, rs = rise + ex / 2;
        if (d > rs) return -1;
        int rr = rs * rs;
        return hw - (hw * d * d + rr / 2) / rr;
    };

    for (int y = 0; y < W; ++y) {
        // ---- hilada actual + LLAGAS de esta hilada (anchos desiguales, desplazadas) ----
        int course = 0;
        for (int k = 1; k < NC; ++k) if (y >= courseY[k]) course = k;
        const int ch = ((course < NC - 1) ? courseY[course + 1] : W) - courseY[course];
        const int hj = y - courseY[course];                   // fila dentro de la hilada
        int jx[4];                                            // 4 llagas, crecientes, todas < W
        {
            int off = (H(course * 3 + 7, 23) % 5) * (W / 32); // desfase por hilada: 0/4/8/12/16
            for (int k = 0; k < 4; ++k)
                jx[k] = k * (W / 4) + (H(course * 13 + k, 41) % 13) + off;
        }
        // ---- envolventes de las CHORRERAS (solo dependen de y; 0 en ambas costuras) ----
        const int dripC = drip(y, corbB + 1, 3, W - corbB - 8);  // larga, desde la cornisa
        const int dripI = drip(y, impY + 2, 2, W / 4);           // desde la imposta
        const int dripA = drip(y, aBas + 4, 2, W * 3 / 16);      // desde el plinto de la arqueria
        const int dripS = drip(y, wSill + 4, 2, W / 16);         // bajo el alfeizar

        for (int x = 0; x < W; ++x) {
            // ---- bloque de silleria: indice, ancho y posicion dentro del bloque ----
            int bj = -1;
            for (int k = 3; k >= 0; --k) if (x >= jx[k]) { bj = k; break; }
            int vj, bw;
            if (bj < 0)       { bj = 3; vj = x + W - jx[3]; bw = jx[0] + W - jx[3]; } // bloque que ENVUELVE
            else if (bj == 3) {         vj = x - jx[3];     bw = jx[0] + W - jx[3]; }
            else              {         vj = x - jx[bj];    bw = jx[bj + 1] - jx[bj]; }
            const int bId = H(course * 13 + 5, bj * 7 + 3);   // identidad del bloque 0..255

            // ---- 1) PIEDRA: tono por bloque + grano + moteado + picaduras ----
            int lum = 126 + (bId % 15) - 7;                   // piedra MEDIA (para el MODULATE)
            lum += (H(x, y) % 9) - 4;                         // grano fino (hace de dither en 565)
            lum += (H(x >> 2, y >> 2) % 7) - 3;               // moteado de la caliza
            if      (bId < 30)  lum -= 18;                    // spolia ahollinada (bloque oscuro)
            else if (bId > 228) lum += 14;                    // piedra fresca (bloque claro)
            { int n = H(x * 3 + 1, y * 5 + 2);
              if (n < 10) lum -= 14; else if (n > 248) lum += 9; }   // picaduras / cuarzo

            // ---- 2) JUNTAS: tendel y llaga hondos, de profundidad VARIABLE ----
            if      (hj == 0)      lum -= 36 + (H(course * 5 + 1, 200) % 10);   // tendel
            else if (hj == 1)      lum += 8;                                    // labio iluminado
            else if (hj == ch - 1) lum -= 12;                                   // sombra del canto
            if      (vj == 0)      lum -= 32 + (H(bj * 11 + 2, course * 17 + 9) % 9);  // llaga
            else if (vj == 1)      lum += 6;
            else if (vj == bw - 1) lum -= 10;
            // ESQUINAS DESPORTILLADAS: ~18% pierde la arista alta-izq, ~15% la baja-der
            if (hj <= 2 && vj <= 2 && H(bj * 3 + course * 7, 131) < 46) {
                if      (hj + vj <= 1) lum += 16;             // cara fresca expuesta (mas clara)
                else if (hj + vj == 2) lum += 6;
                else if (hj + vj == 3) lum -= 8;              // labio de sombra
            }
            if (hj >= ch - 3 && vj >= bw - 3 && H(bj * 5 + course * 3, 77) < 38) {
                int q = (ch - 1 - hj) + (bw - 1 - vj);
                if      (q <= 1) lum += 13;
                else if (q == 2) lum += 5;
            }

            // ---- 3) ARQUERIA CIEGA: arcos ojivales ciegos poco profundos (periodo 32) ----
            if (y >= aTop && y <= aBas + 3) {
                int u  = ((x + W / 16) & 31) - 16;            // arco centrado; columnilla en u=+-16
                int au = AB(u);
                if (y - aTop <= 1)  lum += 10;                // abaco corrido sobre los arcos
                else if (y > aBas)  lum += (y == aBas + 1) ? 8 : -8;    // plinto: nariz + sombra
                else if (u >= 13 || u <= -14) {               // COLUMNILLA entre arcos (fuste redondo)
                    const int shaft[6] = { 6, 15, 11, 1, -13, -5 };     // luz por la izquierda
                    lum += shaft[(u >= 13) ? (u - 13) : (u + 19)];
                } else {
                    int hwA = aHW;                            // jambas rectas bajo el arranque
                    if (y < aSpr) { int d = aSpr - y, rr = aRise * aRise;
                                    hwA = aHW - (aHW * d * d + rr / 2) / rr; }
                    if (y < aSpr - aRise) hwA = -1;           // por encima de la punta: macizo
                    if      (au <= hwA)     { lum -= 24; if (u < 0) lum += 9; }  // hueco ciego (luz rasante izq)
                    else if (au == hwA + 1) lum += 13;        // baqueton / moldura del arco
                    else                    lum += 3;         // enjuta
                }
            }

            // ---- 4) CORNISA en la costura + CANECILLOS y sombra de vuelo al otro lado ----
            if (y >= corY) {                                  // 121..127: banda volada
                int d = y - corY;
                if      (d == 0) lum = 88;                    // junta de retranqueo contra el muro
                else if (d == 1) lum = 152;                   // chaflan superior iluminado
                else if (d <= 4) lum = 138 - (d - 2) * 7;     // cara de la cornisa 138/131/124
                else             lum = 148 - (d - 5) * 6;     // goteron (arista inferior)
                lum += (H(x, y) % 7) - 3;
            } else if (y <= corbB) {                          // 0..5: soffit + TABLA DE CANECILLOS
                int cc = (x & 15) - 8, wt = (corbB - 1) - y;  // mensula que se estrecha al bajar
                if (wt >= 0 && AB(cc) <= wt) lum = 124 - y * 4 + ((cc < 0) ? 7 : -7);
                else                         lum = 78 + y * 5;   // sombra del vuelo (se abre al bajar)
                lum += (H(x, y) % 5) - 2;
            }

            // ---- 5) IMPOSTA a media altura (vuelo iluminado + sombra dura) ----
            if      (y == impY - 2) lum = 150;
            else if (y == impY - 1) lum = 136;
            else if (y == impY)     lum = 118;
            else if (y == impY + 1) lum = 76;
            else if (y == impY + 2) lum = 98;

            // ---- 6) DESGASTE: chorreras de agua por columna + FISURAS finas ----
            {
                int wet = 0;
                int coarse = H(x >> 2, 51);                   // bandas de ~4 px (mancha ancha)
                int fine   = H(x, 77);                        // reguero fino por columna
                if (coarse < 96) wet += (96 - coarse);        // 0..96
                if (fine   < 44) wet += (44 - fine) / 3;      // 0..14
                int venv = MX(MX(dripC, dripI), MX(dripA, dripS));
                lum -= wet * venv / 300;                      // hasta ~23 en columnas mojadas
                int cd = crack(x, y, W*7/64,  W*3/16,  W*5/64,  W*5/8);      // (14,24)-(10,80)
                cd = MX(cd, crack(x, y, W*23/32, W*9/64,  W*49/64, W*7/16)); // (92,18)-(98,56)
                cd = MX(cd, crack(x, y, W*15/32, W*27/32, W*9/16,  W*15/16));// (60,108)-(72,120)
                cd = MX(cd, crack(x, y, W*3/16,  W*23/32, W*7/32,  W*29/32));// (24,92)-(28,116)
                lum -= cd;
            }

            // ---- color de PIEDRA: calida/parda (r > g > b), nunca casi-negra ----
            lum = C(lum, 54, 176);
            int r = lum + 5, g = lum - 2, b = lum - 13;

            // ---- 7) MUSGO / humedad verdosa (juntas y bandas que escurren) ----
            {
                int moss = 0;
                if (hj <= 1 || vj == 0) { int m1 = H(x >> 1, y >> 1); if (m1 < 76) moss += (76 - m1) / 5; }
                int damp = 0;
                if (y > W * 27 / 32)           damp  = (y - W * 27 / 32) * 2;   // pie del pano (>108)
                if (y > impY && y < impY + 7)  damp += 9;                       // bajo la imposta
                if (y > aBas && y < aBas + 6)  damp += 7;                       // bajo el plinto
                if (damp > 0) { int m2 = H(x >> 2, 88); if (m2 < 150) moss += ((150 - m2) * damp) / 260; }
                if (moss > 0) {
                    int m = (moss > 17) ? 17 : moss;
                    r -= m + m / 3; g -= m * 2 / 5; b -= m;   // verde apagado: g cae MENOS que r y b
                }
            }

            // ---- 8) VENTANA OJIVAL estrecha y PROFUNDA (derrame + jamba + vano ambar) ----
            {
                int dxs = x - wcx, adx = AB(dxs);
                int hS = ogive(y, wHW + 6, wHW, wSpr, wRise, wSill);   // DERRAME (splay) hundido
                if (hS >= 0 && adx <= hS) {
                    int v = 98 - (hS - adx) * 2 + ((dxs < 0) ? 14 : 0);// se hunde hacia el vano
                    if (y > wSill) v = 128;                            // losa del alfeizar
                    r = v + 5; g = v - 2; b = v - 13;
                }
                int hF = ogive(y, wHW + 2, wHW, wSpr, wRise, wSill);   // JAMBA / baqueton de piedra
                if (hF >= 0 && adx <= hF) {
                    int v = 118 + ((dxs < 0) ? 16 : -12) + (H(x, y) % 5 - 2);
                    if (y >= wSill - 1) v = 142;                       // nariz del alfeizar
                    r = v + 5; g = v - 2; b = v - 13;
                }
                int hI = ogive(y, wHW, wHW, wSpr, wRise, wSill);       // VANO
                if (hI >= 0 && adx <= hI) {
                    if ((dxs == 0 && y > wSpr - 6) || y == wMid - 4) { r = 84; g = 79; b = 70; } // parteluz + travesano
                    else {
                        int gdy = y - (wMid + 6);
                        int a = 152 - (dxs * dxs * 9 + gdy * gdy * 2) / 5;   // resplandor AMBAR del fondo
                        if (a < 0) a = 0;                                    // cabeza: vidrio oscuro y FRIO
                        r = 56 + a;  g = 62 + a * 3 / 5;  b = 74 + a / 8;
                        if (((x + y) & 7) == 0) { r -= 16; g -= 14; b -= 10; }  // emplomado
                        if (((x - y) & 7) == 2) { r -= 7;  g -= 6;  b -= 4;  }
                    }
                }
            }

            // ---- 9) NICHO con figura tallada PALIDA (dosel volado + peana) ----
            {
                int dxn = x - nx, adn = AB(dxn);
                if (y >= nTop - 4 && y <= nBot + 3 && adn <= nHW + 3) {
                    if (y < nTop) {                                    // DOSEL
                        int v = (y == nTop - 4) ? 96 : ((y == nTop - 1) ? 74 : 144);
                        r = v + 5; g = v - 2; b = v - 13;
                    } else if (y > nBot) {                             // PEANA
                        int v = (y == nBot + 1) ? 146 : 92;
                        r = v + 5; g = v - 2; b = v - 13;
                    } else {
                        int hwN = nHW, dh = (nTop + nHW) - y;          // cabeza redondeada del nicho
                        if (dh > 0) { int rr = nHW * nHW; hwN = nHW - (nHW * dh * dh + rr / 2) / rr; }
                        if (adn <= hwN) {
                            bool body = (adn <= 2 && y >= nTop + nHW + 2 && y <= nBot - 2);
                            bool head = (adn <= 1 && y >= nTop + nHW - 2 && y <  nTop + nHW + 2);
                            if (body || head) {                        // figura de piedra PALIDA
                                int v = 148 - ((dxn > 0) ? 22 : 0) - (((y & 3) == 0) ? 7 : 0);
                                r = v + 4; g = v - 3; b = v - 14;
                            } else {                                   // fondo hundido del nicho
                                int v = 62 + ((dxn < 0) ? 10 : 0);
                                r = v + 3; g = v - 1; b = v - 8;
                            }
                        } else if (adn == hwN + 1) { r = 150; g = 143; b = 130; }  // arista del nicho
                    }
                }
            }

            r = C(r, 46, 236); g = C(g, 44, 226); b = C(b, 42, 214);   // nada casi-negro, ambar sin recorte
            t[y * W + x] = RGBA(r, g, b, 255);
        }
    }
}
// NOTA (verificacion):
// Pintado: SILLERIA de 9 hiladas de alto desigual con llagas desplazadas y de ancho
//   desigual, tono por bloque, spolia clara/oscura, juntas de profundidad variable y
//   esquinas desportilladas; CORNISA en la costura + canecillos y sombra de vuelo;
//   IMPOSTA a media altura; ARQUERIA CIEGA de arcos ojivales con baqueton, enjutas,
//   abaco, plinto y columnillas; VENTANA OJIVAL profunda (derrame+jamba+alfeizar) con el
//   vano oscuro y frio arriba y resplandor AMBAR abajo; NICHO con figura palida bajo
//   dosel; y desgaste asimetrico: chorreras, musgo en juntas bajas, 4 fisuras, picaduras.
// Brillo (MODULATE): piedra media ~110-150 (clamp de lum a 54..176 -> el grueso nunca cae
//   bajo ~90 por el desgaste); tendeles/llagas/hueco ciego ~72-92; nicho y cabeza fria del
//   vano ~58-75; cornisa/imposta/alfeizar/figura ~140-155; AMBAR hasta (208,153,93).
//   Piedra parda r>g>b, unico verde = musgo (g cae menos que r y b).  Ruido +-4 = dither
//   contra el bandeo de RGB565; todo el relieve es por escalones, sin degradados largos.
// TESELA (ambos ejes, potencia de 2, GU_REPEAT):
//   - Todo es funcion exacta de (x mod W, y mod W): hiladas courseY[] empiezan en 0 y la
//     ultima termina en W; las 4 llagas jx[] dependen solo de la hilada y el BLOQUE QUE
//     ENVUELVE se resuelve explicito (x < jx[0] -> vj = x + W - jx[3]) -> la llaga cruza la
//     costura vertical sin corte.  Canecillos (x&15) y arqueria ((x+8)&31): periodos 16 y
//     32, divisores de 128.  Cornisa (y>=121) + soffit/canecillos (y<=5) se COMPLETAN a
//     traves de la costura horizontal.  Chorreras: hash por columna x envolvente en y que
//     vale 0 en y=0..5 y en y>=122 -> sin salto en la costura.  Ventana (x 30..50,
//     y 62..118), nicho (x 98..118, y 68..107), arqueria (y 21..54) y las 4 fisuras
//     (x 10..98, y 18..120) son INTERIORES: nunca tocan x=0/W ni y=0/W.
//   => TESELA PERFECTA EN AMBOS EJES, sin costura visible.
