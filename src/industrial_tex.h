#pragma once
// MURO INDUSTRIAL-GOTICO DEL "POZO" (THE SHAFT) -- megaestructura estilo BLAME!: fria,
// industrial, ANTIGUA, con trazas goticas apenas.  Esta tesela se repite por paredes de
// cientos de unidades de alto (STEX=128 => TILE=6 unidades de mundo por repeticion), asi
// que debe leerse como ESTRUCTURA MONUMENTAL (paneles, tubos, remaches, vigas), no como
// una "paredcita".  Todo el detalle vive en la textura (el muro es solo masa).
//   - PANELES de hormigon/acero: 4 hiladas de alto DESIGUAL (34/58/16/20 px) con columnas
//     desiguales y desfasadas por hilada (rompe la "reja de oficina"), tono por panel,
//     paneles oxidados mas oscuros, costuras hondas con bisel iluminado arriba y sombra
//     abajo, y FILAS DE REMACHES (punto 3x3 con brillo arriba-izq y sombra abajo-der) a lo
//     largo de todas las costuras.
//   - TUBO GRUESO horizontal (r=5) con sombreado cilindrico brillo-medio-sombra, sombra
//     proyectada bajo el tubo, escamas de oxido y ABRAZADERAS cada 32 px; TUBO FINO (r=2)
//     abajo con sus abrazaderas desfasadas.
//   - VIGA / NERVIO vertical (contrafuerte industrial, viga en I de frente) en la costura
//     vertical: banda mas oscura con arista iluminada a la izquierda, ranura de sombra a la
//     derecha, garganta central y pernos cada 16 px.  Corre SIN cortarse -> escala BLAME.
//   - CONDUCTO de cables vertical (3 px, sombreado) con caja de empalme.
//   - Intemperie: regueros de mugre bajando desde cada costura y desde los tubos, manchas
//     de OXIDO (unico acento calido sobre la base fria), picaduras, FISURAS finas
//     asimetricas y unas ABOLLADURAS (dents) con rim iluminado.
//   - Traza GOTICA: un hueco OJIVAL recesado, estrecho, con jamba de acero y parteluz/
//     travesano, vidrio AZUL FRIO tenue encendido (RGBA ~110,160,210 con vignette) y
//     emplomado; y una LAMPARA/rejilla AMBAR diminuta con bisel oscuro (rara, unico
//     otro acento).  Banda de FRANJAS DE PELIGRO desconchada y desvaida (antigua, sutil).
// Determinista (hash entero, sin rand, sin heap; sin dependencia real de <math.h>).
// FUNCION EXACTA de (x mod W, y mod W): TESELA PERFECTA en ambos ejes (NOTA final).
// Brillo pensado para GU_TFX_MODULATE contra el vertice del muro:
//   panel medio ~95-150 | costuras/sombras/garganta de viga ~55-80 | brillos de tubo y
//   remache ~150-165 | vidrio frio encendido ~160-215 (canal b) | ambar ~190-200 (r).
//   Tono FRIO-neutro: b >= g >= r por poco margen (gris azulado, NO saturado); el oxido
//   es el unico calido.  El grueso queda MEDIO, no oscuro.
static void genIndustrial(unsigned int *t, int W) {
    // ---- geometria del modulo, derivada de W (exacta para potencia de 2; W=128 abajo) ----
    const int cx     = W >> 1;            // 64  eje del hueco ojival
    const int pipeY  = W * 5 / 32;        // 20  eje del TUBO GRUESO (y 15..25)
    const int pipeR  = W * 5 / 128;       // 5   radio del tubo grueso
    const int pipe2Y = W * 25 / 32;       // 100 eje del TUBO FINO (y 98..102)
    const int pipe2R = W / 64;            // 2   radio del tubo fino
    const int ribR   = W * 5 / 64;        // 10  semiancho de la VIGA en la costura vertical
    const int condX  = W * 25 / 32;       // 100 eje del CONDUCTO de cables vertical
    const int winW   = W / 16;            // 8   semiluz del hueco ojival
    const int winSpr = W * 7 / 16;        // 56  arranque del arco ojival
    const int winBot = W * 21 / 32;       // 84  alfeizar del hueco
    const int winTop = winSpr - (W * 13 / 128);   // 43 punta del arco (radio = 2*winW)
    const int winMid = (winSpr + winBot) / 2;     // 70 travesano
    const int gcy    = (winTop + winBot) / 2;     // 63 centro del vignette del vidrio
    const int ambX   = W * 43 / 64;       // 86  lampara ambar diminuta
    const int ambY   = W * 31 / 64;       // 62
    const int hz0    = W * 57 / 64;       // 114 banda de franjas de peligro (y 114..122)
    const int hz1    = W * 61 / 64;       // 122
    // hiladas de paneles: alto DESIGUAL (34 / 58 / 16 / 20 px) -> no lee como grilla
    const int rowTop[4] = { 0, W * 17 / 64, W * 23 / 32, W * 27 / 32 };      // 0, 34, 92, 108
    // columnas por hilada: desfase (rowOff) + costuras (colSeam) desiguales; -1 = fin
    const int rowOff[4]     = { 0, 0, 0, W / 4 };
    const int colSeam[4][4] = { { 0, W / 2, -1, -1 },                          // 64 | 64
                                { 0, W / 4, W * 3 / 4, -1 },                   // 32 | 64 (hueco) | 32
                                { 0, W * 5 / 16, W * 11 / 16, -1 },            // 40 | 48 | 40
                                { 0, W / 2, -1, -1 } };                        // 64 | 64 (desfasado 32)

    // ---- utilidades enteras ----
    auto H = [](int a, int b) -> int {                 // hash determinista -> 0..255
        unsigned int h = (unsigned int)a * 374761393u + (unsigned int)b * 668265263u + 0x9E3779B9u;
        h = (h ^ (h >> 13)) * 1274126177u; h ^= (h >> 16);
        return (int)(h & 0xFFu);
    };
    auto C  = [](int v, int lo, int hi) -> int { return v < lo ? lo : (v > hi ? hi : v); };
    auto AB = [](int v) -> int { return v < 0 ? -v : v; };
    auto MX = [](int a, int b) -> int { return a > b ? a : b; };
    auto D2 = [](int ax, int ay, int bx, int by) -> int {
        int dx = ax - bx, dy = ay - by; return dx*dx + dy*dy;
    };
    // reguero: envolvente vertical desde 'yL', sube en 'rise' y decae en 'run' (0 en ambos extremos)
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
    // REMACHE 3x3: brillo arriba-izq, sombra abajo-der (rx, ry = offset al centro)
    auto rivet = [](int rx, int ry) -> int {
        if (rx < -1 || rx > 1 || ry < -1 || ry > 1) return 0;
        int s = rx + ry;
        if (s < 0) return 26;
        if (s > 0) return -22;
        return (rx == 0) ? 10 : -4;
    };
    // hueco ojival con semiluz 'hw' (misma pareja de centros -> offset limpio del arco)
    auto winIn = [&](int px, int py, int hw) -> bool {
        int ex = hw - winW;                                           // margen extra
        if (AB(px - cx) <= hw && py >= winSpr && py <= winBot + ex) return true;   // cuerpo recto
        if (py < winSpr) {
            int rr = (winW + hw) * (winW + hw);
            if (D2(px, py, cx - winW, winSpr) <= rr && D2(px, py, cx + winW, winSpr) <= rr) return true;
        }
        return false;
    };

    for (int y = 0; y < W; ++y) {
        // hilada del panel (constante por fila)
        int row = 0; for (int k = 1; k < 4; ++k) if (y >= rowTop[k]) row = k;
        const int rowH = ((row < 3) ? rowTop[row + 1] : W) - rowTop[row];
        const int hj   = y - rowTop[row];                       // fila dentro del panel
        const int dyT  = (y < (W - y)) ? y : (W - y);           // distancia a la costura horizontal
        for (int x = 0; x < W; ++x) {
            // distancia CON SIGNO a la costura vertical (x=0/W): periodica, sella el modulo
            int xw = (x < cx) ? x : x - W;
            // columna del panel (con desfase por hilada, periodico via &(W-1))
            int xx = (x + rowOff[row]) & (W - 1);
            int col = 0, nc = 0;
            for (int k = 0; k < 4; ++k) { if (colSeam[row][k] < 0) break; nc = k + 1; if (xx >= colSeam[row][k]) col = k; }
            int colW = ((col < nc - 1) ? colSeam[row][col + 1] : W) - colSeam[row][col];
            int vj   = xx - colSeam[row][col];                  // columna dentro del panel

            // ---- 1) PANEL: tono por panel + grano (hormigon vs acero) + picaduras ----
            int pId   = H(row * 7 + col * 3 + 1, 91);           // identidad del panel 0..255
            int ptone = (pId % 15) - 7;                         // -7..+7
            bool conc = (pId & 1);                              // hormigon (grano grueso) vs acero (liso)
            int lum   = 122 + ptone + (conc ? (H(x, y) % 9 - 4) : (H(x, y) % 3 - 1));
            if (conc) lum += (H(x >> 2, y >> 2) % 7) - 3;       // moteado del hormigon
            if (pId < 22) lum -= 12;                            // panel oxidado / repuesto oscuro
            else if (pId > 240) lum += 8;                       // panel mas nuevo / claro
            { int n = H(x * 3 + 1, y * 5 + 2);
              if (n < 9) lum -= 10; else if (n > 249) lum += 7; } // picaduras y brillos puntuales
            // costuras hondas con bisel: arriba iluminado, abajo en sombra
            if      (hj == 0)        lum -= 46;                 // costura horizontal (~76)
            else if (hj == 1)        lum += 10;                 // bisel iluminado del panel
            else if (hj == rowH - 1) lum -= 14;                 // sombra antes de la costura
            if      (vj == 0)        lum -= 42;                 // costura vertical
            else if (vj == 1)        lum += 8;
            else if (vj == colW - 1) lum -= 12;
            // FILAS DE REMACHES a 3 px de cada costura, cada 8 px
            {
                int rv = 0;
                if (hj == 2 || hj == 3 || hj == 4)                rv = rivet((xx & 7) - 4, hj - 3);
                else if (hj >= rowH - 5 && hj <= rowH - 3)        rv = rivet((xx & 7) - 4, hj - (rowH - 4));
                if (rv == 0) {
                    if (vj >= 2 && vj <= 4)                       rv = rivet(vj - 3, (hj & 7) - 4);
                    else if (vj >= colW - 5 && vj <= colW - 3)    rv = rivet(vj - (colW - 4), (hj & 7) - 4);
                }
                lum += rv;
            }
            // ---- 1b) costura horizontal MAYOR en la costura de la tesela (reborde/ala) ----
            if      (dyT == 1) lum += 14;                       // labio iluminado del ala
            else if (dyT == 2) lum += 4;

            // ---- 2) ABOLLADURAS (dents): concavas, rim iluminado abajo-der ----
            {
                const int dc[3][3] = { { W * 3 / 16, W * 35 / 64, 4 },      // (24,70) r4
                                       { W * 13 / 16, W * 19 / 32, 3 },     // (104,76) r3
                                       { W * 15 / 32, W * 59 / 64, 3 } };   // (60,118) r3
                for (int k = 0; k < 3; ++k) {
                    int dx = x - dc[k][0], dy = y - dc[k][1], rr = dc[k][2];
                    int d2 = dx*dx + dy*dy;
                    if (d2 <= rr * rr) {
                        lum += (dx + dy) * 3;                   // sombra arriba-izq, luz abajo-der
                        if (d2 >= rr * rr - rr) lum += 5;       // rim
                    }
                }
            }

            // ---- 3) VIGA / NERVIO vertical en la costura (viga en I de frente, escala BLAME) ----
            if (AB(xw) <= ribR) {
                int d = AB(xw);
                lum = 104 + (H(x, y) % 3 - 1);                  // cara del ala, acero liso
                if      (d <= 1)         lum = 72;              // garganta central (alma)
                else if (d == 2)         lum = 120;             // arista de la garganta
                if      (xw == -ribR)    lum = 146;             // arista IZQ iluminada
                else if (xw == -ribR+1)  lum = 126;
                else if (xw == ribR)     lum = 60;              // ranura de sombra a la DER
                else if (xw == ribR-1)   lum = 78;
                if (d >= 5 && d <= 7) lum += rivet(d - 6, (y & 15) - 8);   // pernos cada 16 px
                if (((y >> 5) & 1) && d == 4) lum -= 6;         // veta sutil del laminado
            }

            // ---- 4) CONDUCTO de cables vertical (3 px, sombreado) + caja de empalme ----
            {
                int dxc = x - condX;
                if (dxc >= -1 && dxc <= 1) lum = 108 + (1 - AB(dxc + 1)) * 18 - (dxc == 1 ? 24 : 0);  // 126/108/84
                else if (dxc == 2)         lum -= 12;           // sombra proyectada
                else if (dxc == -2)        lum -= 4;
                // caja de empalme 7x9 (x 97..103, y 90..98)
                int by = y - (W * 45 / 64);                     // 90
                if (AB(dxc) <= 3 && by >= 0 && by <= 8) {
                    lum = 100;
                    if (by == 0 || dxc == -3) lum = 128;        // bordes iluminados
                    if (by == 8 || dxc == 3)  lum = 66;         // bordes en sombra
                    if (AB(dxc) <= 1 && by >= 3 && by <= 5) lum = 88;   // tapa recesada
                }
            }

            // ---- 5) TUBOS horizontales (sobre viga y conducto: van por delante) ----
            {
                int d = y - pipeY;                              // TUBO GRUESO
                if (AB(d) <= pipeR) {
                    lum = 100 + (pipeR - AB(d + 2)) * 12;       // brillo en d=-2 (160), sombra en d=+5 (76)
                    lum += H(x, y) % 3 - 1;
                    { int fl = H(x >> 2, 33); if (fl < 34 && d >= -1) lum -= (34 - fl) / 3; }   // escamas
                }
                else if (d > pipeR && d <= pipeR + 3) lum -= 20 - (d - pipeR) * 5;    // sombra proyectada
                else if (d < -pipeR && d >= -pipeR - 2) lum -= 6;                    // oclusion arriba
                int c = (x + 8) & 31;                           // ABRAZADERA cada 32 px (x 24..27 + ...)
                if (c <= 3 && AB(d) <= pipeR + 2) {
                    lum = 108;
                    if (c == 0) lum = 134; else if (c == 3) lum = 70;
                    if (AB(d) > pipeR) lum -= 12;               // oreja de la abrazadera
                    if (c == 1 && d == pipeR + 1) lum = 140;    // perno
                }
                d = y - pipe2Y;                                 // TUBO FINO
                if (AB(d) <= pipe2R) {
                    lum = 104 + (pipe2R - AB(d + 1)) * 18;      // 140 / 122 / 104 / 86
                    lum += H(x, y) % 3 - 1;
                }
                else if (d == pipe2R + 1) lum -= 14;
                c = (x + 24) & 31;                              // abrazaderas desfasadas
                if (c <= 2 && AB(d) <= pipe2R + 1) {
                    lum = 106;
                    if (c == 0) lum = 130; else if (c == 2) lum = 72;
                }
            }

            // ---- 6) FRANJAS DE PELIGRO desconchadas y desvaidas (antiguas, sutiles) ----
            int hz = 0;                                          // +1 franja clara (tibia), -1 oscura
            if (y >= hz0 && y <= hz1 && x >= W * 3 / 32 && x <= W * 13 / 32) {   // x 12..52
                int peel = H(x >> 1, (y >> 1) + 7);
                if (peel > 74) hz = (((x + y) >> 2) & 1) ? 1 : -1;
                if (hz < 0) lum -= 7;
            }

            // ---- 7) INTEMPERIE: regueros de mugre por columna + fisuras finas ----
            {
                int wet = 0;                                     // humedad por columna (rasgo vertical)
                int coarse = H(x >> 2, 51);
                int fine   = H(x, 77);
                if (coarse < 90) wet += (90 - coarse);           // 0..90
                if (fine   < 40) wet += (40 - fine) / 3;         // 0..13
                int venv = drip(y, 0, 4, W - 4);                 // desde el ala superior (todo el alto)
                for (int k = 1; k < 4; ++k) venv = MX(venv, drip(y, rowTop[k] + 1, 3, 30));   // desde cada costura
                venv = MX(venv, drip(y, pipeY + pipeR + 1, 2, 40));   // bajo el tubo grueso
                venv = MX(venv, drip(y, winBot + 4, 2, 18));          // bajo el alfeizar
                lum -= wet * venv / 300;                         // oscurece hasta ~22 en columnas mojadas
                int cd = crack(x, y, W * 9 / 64, W * 11 / 32, W * 3 / 16, W * 43 / 64);    // (18,44)-(24,86)
                cd = MX(cd, crack(x, y, W * 25 / 32, W * 19 / 64, W * 7 / 8, W * 15 / 32)); // (100,38)-(112,60)
                cd = MX(cd, crack(x, y, W * 35 / 64, W * 7 / 8, W * 23 / 32, W * 31 / 32)); // (70,112)-(92,124)
                lum -= cd;
            }

            // ---- 8) OXIDO: manchas calidas (parches + bajo tubos/abrazaderas + bajo remaches) ----
            int rust = 0;
            {
                int a = H(x >> 3, y >> 3);
                if (a < 34) rust += 34 - a;
                int b2 = H((((x + 4) & (W - 1)) >> 3) + 17, (((y + 4) & (W - 1)) >> 3) + 5);
                if (b2 < 28) rust += (28 - b2) / 2;
                if (rust > 0) {                                                 // borde carcomido, sin celdas cuadradas
                    int e = H(x >> 1, (y >> 1) + 9) % 7;                        // 0..6 mordida en bloques 2x2
                    rust = rust * (H(x, y + 9) % 5 + 2) / 6;
                    rust = (rust * (e + 2)) / 8;
                }
                int d = y - pipeY;                                              // chorreado bajo el tubo grueso
                if (d > pipeR && d < pipeR + 26 && H(x >> 1, 12) < 60) rust += (pipeR + 26 - d) * 2 / 3;
                d = y - pipe2Y;
                if (d > pipe2R && d < pipe2R + 10 && H(x >> 1, 21) < 50) rust += (pipe2R + 10 - d);
                if (AB(xw) <= ribR && (y & 15) > 8 && (y & 15) < 14 && AB(AB(xw) - 6) <= 1) rust += 6; // bajo pernos
                if (rust > 24) rust = 24;
            }

            // ---- color BASE frio-neutro (b >= g >= r, poco margen) ----
            int r = lum - 6, g = lum - 2, b = lum + 3;
            if (rust > 0) { r += rust; g += rust / 4; b -= rust / 2; lum -= rust / 4; }
            if (hz > 0)   { r += 9; g += 5; }                    // franja clara: amarillo desvaido

            // ---- 9) HUECO OJIVAL recesado (interior de la tesela) ----
            {
                bool glass = winIn(x, y, winW);
                bool frame = !glass && winIn(x, y, winW + 2);
                bool rec   = !glass && !frame && winIn(x, y, winW + 5);
                if (rec) {                                        // derrame recesado (sombra)
                    int v = lum - 18;
                    if (x < cx - winW - 2) v += 6;                // lado izq recibe algo de luz
                    r = v - 6; g = v - 2; b = v + 3;
                }
                if (frame) {                                      // jamba de acero
                    int v = 94 + (H(x, y) % 3 - 1);
                    if (x < cx) v += 12;                          // arista iluminada
                    if (y >= winBot + 1) v += 10;                 // alfeizar
                    r = v - 6; g = v - 2; b = v + 3;
                }
                if (glass) {
                    bool bar = (x == cx) || (y == winMid);        // parteluz + travesano
                    if (bar) { r = 74; g = 78; b = 84; }
                    else {                                        // vidrio AZUL FRIO tenue encendido
                        int gdx = x - cx, gdy = y - gcy;
                        int fall = (gdx*gdx*3 + gdy*gdy) / 7;   // vignette: nucleo (110,160,210), borde apagado
                        r = 110 - fall*3/4; if (r < 68)  r = 68;
                        g = 160 - fall;     if (g < 104) g = 104;
                        b = 210 - fall;     if (b < 152) b = 152;
                        if (((x + y) & 7) == 0) { r -= 14; g -= 16; b -= 14; }   // emplomado
                        if (((x - y) & 7) == 3) { r -= 6;  g -= 7;  b -= 6; }
                    }
                }
                // sombra bajo el alfeizar
                if (y == winBot + 6 && AB(x - cx) <= winW + 5) { r -= 8; g -= 8; b -= 8; }
            }

            // ---- 10) LAMPARA / rejilla AMBAR diminuta con bisel (rara; unico otro acento) ----
            {
                int dx = x - ambX, dy = y - ambY;
                if (AB(dx) <= 3 && AB(dy) <= 3) {
                    if (AB(dx) <= 1 && AB(dy) <= 1) {
                        int f = AB(dx) + AB(dy);                  // 0 nucleo, 1 borde
                        r = 200 - f * 24; g = 138 - f * 22; b = 58 - f * 6;
                    } else {
                        int v = (dx == -3 || dy == -3) ? 112 : 70; // bisel: arriba-izq lit, resto sombra
                        r = v - 4; g = v - 2; b = v + 2;
                        if (AB(dx) <= 2 && AB(dy) <= 2) { r += 12; g += 6; }  // halo tibio
                    }
                }
            }

            r = C(r, 44, 235); g = C(g, 42, 224); b = C(b, 40, 214);   // nada casi-negro, azul sin recorte
            t[y * W + x] = RGBA(r, g, b, 255);
        }
    }
}
// NOTA (verificacion):
// Elementos pintados: PANELES de hormigon/acero en 4 hiladas de alto desigual (34/58/16/20)
//   con columnas desiguales y desfasadas, tono por panel (paneles oxidados mas oscuros /
//   mas nuevos mas claros), grano grueso (hormigon) o liso (acero), picaduras, costuras
//   hondas con bisel arriba y sombra abajo, y FILAS DE REMACHES 3x3 (brillo arriba-izq,
//   sombra abajo-der) cada 8 px junto a TODAS las costuras; TUBO GRUESO (r=5) con sombreado
//   cilindrico, sombra proyectada, escamas y ABRAZADERAS con perno cada 32 px; TUBO FINO
//   (r=2) con abrazaderas desfasadas; VIGA en I vertical en la costura (arista izq
//   iluminada, ranura de sombra der, garganta central, pernos cada 16 px, corre sin
//   cortarse); CONDUCTO de cables vertical con caja de empalme; 3 ABOLLADURAS; FISURAS
//   finas asimetricas; regueros de mugre por columna desde ala/costuras/tubos/alfeizar;
//   manchas de OXIDO (parches + chorreado bajo tubos y pernos); banda de FRANJAS DE
//   PELIGRO desconchada; traza gotica: HUECO OJIVAL recesado con jamba de acero, parteluz,
//   travesano, vidrio AZUL FRIO encendido con vignette y emplomado; LAMPARA AMBAR diminuta.
// Brillos (MODULATE): panel medio ~100-140 (base 122 +-7 tono +-4 grano; regueros lo bajan
//   hasta ~95, sin ennegrecer); costuras horizontales ~76, verticales ~80, sombra de tubo
//   y garganta de viga ~60-80, ranura de viga 60, bisel de lampara 70; brillos: arista de
//   viga 146, brillo de tubo 160, remaches +26; vidrio (110,160,210) en el nucleo bajando a
//   (68,104,152) en el borde -> canal b ~152-210 (encendido frio); ambar (200,138,58).
//   Tono frio-neutro r=lum-6, g=lum-2, b=lum+3 (b >= g >= r, margen 9, no saturado); el
//   OXIDO (r+rust, g+rust/4, b-rust/2, tope 24) es el unico calido junto al ambar y la
//   franja desvaida (+9r,+5g).  El grueso queda medio (no oscuro).
// TESELA (ambos ejes, potencia de 2, GU_REPEAT):
//   - Hiladas: rowTop[] parte en y=0 y la ultima termina en W (hj=0 en y=0 = costura de la
//     tesela; hj=rowH-1 en y=W-1) -> costura horizontal continua al envolver.  Columnas:
//     xx=(x+rowOff)&(W-1) y colSeam[] con la primera en 0 -> periodicas en x mod W.
//   - Remaches: (xx&7), (hj&7) -> periodo 8 relativo a costuras interiores; pernos de viga
//     (y&15) periodo 16 (128 multiplo).  Abrazaderas (x+8)&31, (x+24)&31: periodo 32.
//   - Tubos y conducto son rasgos de fila/columna completos (funcion de y o de x) ->
//     continuos al envolver; la VIGA usa distancia CON SIGNO a la costura (xw) y el ala
//     superior distancia dyT -> continuos.
//   - Regueros: funcion de columna (H de x>>2 y x) x envolvente en y que vale 0 en y=0 y
//     en y=W-4..W-1 (drip(y,0,4,W-4)) -> sin salto en la costura horizontal.
//   - Oxido: celdas H(x>>3,y>>3) periodicas (128/8) y celda desfasada con ((x+4)&(W-1))>>3.
//     Franjas: banda interior (y 114..122, x 12..52).  Fisuras: segmentos INTERIORES
//     (x in [18,112], y in [38,124]).  Abolladuras, hueco ojival (x 51..77, y 38..90),
//     caja de empalme y lampara ambar: INTERIORES, nunca tocan x=0/W ni y=0/W.
//   => TESELA PERFECTA en ambos ejes.
