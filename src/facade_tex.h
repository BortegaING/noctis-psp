#pragma once
// FACHADA GOTICA DE CATEDRAL -- ANTIGUA, DESGASTADA, CON ALMA (Yharnam / Bloodborne +
// escala BLAME).  TODO el detalle vive en la TEXTURA (el muro es solo masa/silueta, sin
// geometria por ventana).  El modulo de W*W (STEX=128 => ~6 m de mundo por tesela, TILE=6)
// es UN VANO monumental, NO una reja de ventanitas iguales de oficina.  Se combate el
// "grid de oficina / backrooms" con VARIACION y VEJEZ, no con mas simetria:
//   - SILLERIA (ashlar) a soga con TONO por bloque (piedras algo distintas, no uniformes),
//     juntas de profundidad variable (algunas gastadas, otras hondas) y "spolia": bloques
//     sueltos mas oscuros (hollin) o mas claros (piedra fresca).
//   - CURTIDO / INTEMPERIE: derrames verticales de agua bajando desde cornisa/imposta/
//     alfeizar (regueros por columnas humedas), MUSGO/humedad verdosa en juntas y en la
//     base, FISURAS finas (grietas asimetricas) y ESQUINAS DESCANTILLADAS (chips) -> edad.
//   - PILAR/CONTRAFUERTE macizo en la costura (junta MEGA-PANEL escala BLAME, corre
//     vertical SIN cortarse por las cornisas -> lee "mas grande que un piso"), tambien
//     manchado por los regueros.
//   - Un ARCO OJIVAL envolvente recesado (el vano) con TRACERIA de piedra: DOS LANCETAS
//     altas y estrechas (arco apuntado, parteluz, travesano, CUATRIFOLIO en la cabeza).
//   - Un ROSETON: rueda circular con cubo + 8 radios + aro de foils, foco de la composicion.
//   - ARQUERIA CIEGA (fila de arquitos apuntados ciegos) como zocalo/banda en la base.
//   - Cornisa + TABLA DE CANECILLOS (corbel table) arriba, imposta al arranque y alfeizar
//     abajo (string-courses horizontales).
//   - DOS HORNACINAS con figura palida en el contrafuerte (estatuas entre vanos).
//   - Casi todo el vidrio OSCURO/FRIO; UNA lanceta encendida AMBAR (vidriera, unico acento
//     saturado) + un par de petalos tibios del roseton.  El curtido es ASIMETRICO
//     (regueros/grietas no espejados, lanceta ambar solo a la derecha) -> aunque la tesela
//     se repite, el ojo no la lee como un sello limpio.
// Determinista (hash entero, sin rand, sin heap; sin dependencia real de <math.h>).
// FUNCION EXACTA de (x mod W, y mod W): todo rasgo periodico -> TESELA PERFECTA en ambos
// ejes bajo GU_REPEAT (ver NOTA final).
// Brillo pensado para GU_TFX_MODULATE contra el vertice de piedra (baseColor ~x2.7):
//   piedra media ~95-150 | regueros/musgo bajan a ~95 | derrames/parteluces/sombra de
//   contrafuerte ~55-85 | vidrio oscuro/frio ~50-60 | AMBAR encendido ~180-232.
//   Piedra calida/neutra r>=g>=b (el musgo acerca g a r en las juntas, sin pasarlo).
static void genFacade(unsigned int *t, int W) {
    // ---- geometria del modulo, derivada de W (exacta para potencia de 2; W=128 abajo) ----
    const int cx    = W >> 1;          // 64  centro del vano
    const int half  = W * 3 / 8;       // 48  semiluz del arco envolvente
    const int L     = cx - half;       // 16  jamba izquierda del vano
    const int R     = cx + half;       // 112 jamba derecha
    const int ySpr  = W * 7 / 16;      // 56  arranque (springing) del arco mayor
    const int yPk   = W / 8;           // 16  punta del arco mayor
    const int ySill = W * 13 / 16;     // 104 alfeizar (fondo de las lancetas)
    const int pierR = W * 7 / 64;      // 14  semiancho del contrafuerte en la costura
    const int lOff  = W * 5 / 32;      // 20  separacion de la lanceta al centro
    const int lx    = cx - lOff;       // 44  eje lanceta izquierda
    const int rx    = cx + lOff;       // 84  eje lanceta derecha
    const int lw    = W * 7 / 64;      // 14  semiancho de lanceta (estrecha -> alta)
    const int lyS   = W * 9 / 16;      // 72  arranque de la lanceta
    const int lPk   = W * 27 / 64;     // 54  punta de la lanceta (rise = 18)
    const int roseCy= W * 19 / 64;     // 38  centro Y del roseton
    const int roseR = W * 7 / 64;      // 14  radio del roseton
    const int ym    = (lyS + ySill)/2; // 88  travesano de las lancetas
    const int rBay2 = (cx-L)*(cx-L) + (ySpr-yPk)*(ySpr-yPk);   // radio^2 arco mayor (3904)
    const int rLan2 = lw*lw + (lyS-lPk)*(lyS-lPk);             // radio^2 arco lanceta (520)
    const int A0    = ySill + 4;       // 108 borde superior de la ARQUERIA CIEGA (zocalo)
    const int A1    = ySill + 18;      // 122 borde inferior (interior, no toca la costura)

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
    // reguero de agua: envolvente vertical desde una repisa 'yL', sube en 'rise' y decae en 'run'
    auto drip = [](int yy, int yL, int rise, int run) -> int {
        if (yy < yL || yy > yL + run) return 0;
        int d = yy - yL;
        int e = (d < rise) ? (d * 64 / rise) : (64 - (d - rise) * 64 / (run - rise));
        return e < 0 ? 0 : e;
    };
    // distancia^2 (entera, aprox) de un punto a un segmento -> para FISURAS finas interiores
    auto SEG2 = [](int px, int py, int ax, int ay, int bx, int by) -> int {
        long long vx = bx - ax, vy = by - ay, wx = px - ax, wy = py - ay;
        long long c1 = vx*wx + vy*wy;
        if (c1 <= 0) return (int)(wx*wx + wy*wy);
        long long c2 = vx*vx + vy*vy;
        if (c2 <= c1) { long long ex = px - bx, ey = py - by; return (int)(ex*ex + ey*ey); }
        long long d2 = (wx*wx + wy*wy) - (c1*c1) / c2;
        return d2 < 0 ? 0 : (int)d2;
    };
    auto crack = [&](int px, int py, int ax, int ay, int bx, int by) -> int {   // 0/4/12/22
        int d2 = SEG2(px, py, ax, ay, bx, by);
        if (d2 <= 0) return 22;
        if (d2 <= 1) return 12;
        if (d2 <= 2) return 4;
        return 0;
    };

    for (int y = 0; y < W; ++y) {
        for (int x = 0; x < W; ++x) {
            // distancia CON SIGNO a la costura vertical (x=0/W): periodica, sella el modulo
            int xw = (x < cx) ? x : x - W;              // -.. a la izquierda de la costura, + a la derecha
            int dyT = (y < (W - y)) ? y : (W - y);      // distancia a la costura horizontal (y=0/W)

            // ---- membresias de vano/lancetas/roseton (todo INTERIOR -> nunca cruza costura) ----
            int rdx = x - cx, rdy = y - roseCy, rd2 = rdx*rdx + rdy*rdy;
            bool rose = (rd2 <= roseR*roseR);
            auto lanIn = [&](int c) -> bool {
                if (AB(x - c) <= lw && y >= lyS && y <= ySill) return true;      // cuerpo recto
                if (y < lyS && D2(x,y,c-lw,lyS) <= rLan2 && D2(x,y,c+lw,lyS) <= rLan2) return true; // cabeza apuntada
                return false;
            };
            bool lanL = lanIn(lx), lanR = lanIn(rx);
            bool glass = lanL || lanR || rose;
            bool inbay = false;                          // arco envolvente (recess del vano)
            if (x >= L && x <= R) {
                if (y >= ySpr && y <= ySill) inbay = true;
                else if (y < ySpr && y >= yPk && D2(x,y,L,ySpr) <= rBay2 && D2(x,y,R,ySpr) <= rBay2) inbay = true;
            }

            // ---- 1) SILLERIA (ashlar) a soga: TONO POR BLOQUE + grano + juntas variables ----
            int course = y >> 4;                          // hilada cada 16px (128/16=8 -> tesela)
            int off    = (course & 1) ? (W >> 3) : 0;     // soga: media pieza alterna por hilada
            int col    = ((x + off) & (W - 1)) >> 5;      // columna de bloque 0..3 (periodica: &127)
            int bId    = H(col * 7 + 3, course * 13 + 5); // identidad del bloque (0..255)
            int btone  = (bId % 17) - 8;                  // tono por bloque -8..+8 (rompe la uniformidad)
            int lum    = 120 + btone + (H(x, y) % 5 - 2); // piedra MEDIA (para el MODULATE) + grano fino
            if      (bId < 26)  lum -= 16;                // spolia oscura (bloque ahollinado / repuesto)
            else if (bId > 236) lum += 12;               // spolia clara (piedra fresca)
            int hj = y & 15;                              // fila dentro de la hilada
            int vj = (x + off) & 31;                      // llaga cada 32px (128/32=4 -> tesela)
            if      (hj == 0) lum -= 11 + (H(course * 5 + 1, 200) % 7);  // tendel: profundidad variable 11..17
            else if (hj == 1) lum += 5;                   // labio iluminado bajo el tendel
            if      (vj == 0) lum -= 8 + (H(col * 11 + 2, course * 17 + 9) % 7);  // llaga: profundidad variable 8..14
            else if (vj == 1) lum += 3;
            // ESQUINA DESCANTILLADA (chip): ~15% de bloques con arista rota (piedra fresca + labio)
            if (hj <= 2 && vj <= 2 && H(col * 3 + course * 7, 131) < 40) {
                if      (hj + vj <= 1) lum += 12;         // caras frescas expuestas (mas claras)
                else if (hj + vj == 2) lum += 4;
                else if (hj + vj == 3) lum -= 6;          // pequeno labio de sombra
            }

            // ---- 2) STRING-COURSES horizontales: cornisa+canecillos (arriba), imposta, alfeizar ----
            if      (dyT == 1) lum += 16;                // vuelo iluminado de la cornisa (en la costura)
            else if (dyT == 0) lum -= 14;                // sombra bajo el vuelo (la junta de la costura)
            else if (dyT <= 4) lum += 5;                 // cara de la cornisa
            if (y >= 7 && y <= 10) {                      // tabla de CANECILLOS (mensulas cada 16px)
                int cc = (x & 15) - 8, wtri = 10 - y;
                if (AB(cc) <= wtri) lum -= 10; else lum += 3;
            }
            if      (y == ySpr - 2) lum += 10;           // imposta (linea de arranque) volada
            else if (y == ySpr)     lum -= 6;
            if      (y >= ySill + 1 && y <= ySill + 2) lum += 12;   // nariz del alfeizar iluminada
            else if (y == ySill + 3)                   lum -= 6;

            // ---- 2b) ARQUERIA CIEGA (zocalo de arquitos apuntados) en la base, fuera del vidrio ----
            if (!glass && y >= A0 && y <= A1) {
                int u  = (x & 15) - 8;                    // -8..7 (arco centrado en x&15==8, periodo 16)
                int au = AB(u);                           // 0..8
                int byb = y - A0;                         // 0..14 dentro de la banda
                if (byb <= 1)          lum += 7;          // abaco/imposta sobre la arqueria (repisa lit)
                else if (byb >= 13) {                     // plinto/base
                    if (byb == 13) lum += 5; else lum -= 6;
                } else if (au >= 7) {                     // COLONNETTE (columnilla entre arcos)
                    if (au == 8) lum -= 6; else lum += 6; // fuste con nucleo de sombra -> lee redondo
                } else {                                  // campo del ARCO
                    int oh = (byb <= 2) ? 0 : (byb - 2 < 6 ? byb - 2 : 6);  // semiluz: apice arriba, jambas rectas
                    if      (au <= oh)      { lum -= 16; if (au == 0) lum -= 2; }  // hueco recesado (sombra)
                    else if (au == oh + 1)  lum += 8;     // baqueton/molde del arco (lit)
                    else                    lum += 2;     // enjuta
                }
            }

            // ---- 3) RECESS del vano + traceria/parteluces/jambas de piedra (solo piedra del vano) ----
            if (inbay && !glass) {
                lum -= 14;                               // panel recesado (el vano hunde)
                bool nearG = (AB(x-lx) <= lw+2 && y >= lyS && y <= ySill) ||
                             (AB(x-rx) <= lw+2 && y >= lyS && y <= ySill) ||
                             (rd2 <= (roseR+2)*(roseR+2));
                if (nearG) lum -= 8;                      // derrame profundo pegado al vidrio
                if (x < L + 4 || x > R - 4) lum -= 8;     // jamba lateral del arco mayor
                if (AB(x - cx) <= 6 && y > ySpr - 2 && y < ySill + 2) {   // PARTELUZ central mayor (mega-mullion BLAME)
                    int d = AB(x - cx);
                    if (d <= 1) lum += 22; else if (d >= 5) lum -= 8;
                }
            }

            // ---- 4) CONTRAFUERTE en la costura: junta MEGA-PANEL vertical (escala BLAME) ----
            // Se aplica DESPUES de las cornisas -> la junta central corre SIN cortarse
            // (lee como una mole ensamblada mas alta que un piso).
            if (AB(xw) <= pierR) {
                int d = AB(xw);
                if      (d <= 1) lum = 80;               // fondo de la MEGA-junta (colosal, no negra)
                else if (d <= 3) lum = 128;              // arris iluminada flanqueando la junta
                else             lum = 122 - (d - 3) * 3;// cara del contrafuerte hacia la sombra
                if (d >= pierR - 1) lum -= 20;           // ranura de sombra contra el muro
            }

            // ---- 4b) INTEMPERIE sobre la piedra final (mancha tambien cornisas y contrafuerte) ----
            // Regueros verticales de agua desde repisas (columnas humedas), + FISURAS finas.
            {
                int wet = 0;                              // humedad por columna (rasgo vertical -> costura segura)
                int coarse = H(x >> 2, 51);               // bandas anchas ~4px (mancha)
                int fine   = H(x, 77);                    // reguero fino por columna
                if (coarse < 96) wet += (96 - coarse);    // 0..96
                if (fine   < 44) wet += (44 - fine) / 3;  // 0..14
                int venv = drip(y, 4, 3, 108);            // reguero largo desde la cornisa (todo el muro alto)
                venv = MX(venv, drip(y, ySpr - 2, 2, 40));// desde la imposta
                venv = MX(venv, drip(y, ySill + 2, 2, 20));// desde el alfeizar
                lum -= wet * venv / 300;                  // oscurece hasta ~22 en columnas mojadas (regueros)
                // FISURAS finas asimetricas (segmentos INTERIORES: no tocan ninguna costura)
                int cd = crack(x, y, 12, 40, 9, 90);      // sobre la jamba/contrafuerte izq
                cd = MX(cd, crack(x, y, 98, 20, 104, 58));// enjuta derecha alta
                cd = MX(cd, crack(x, y, 40, 109, 46, 126));// muro bajo / plinto
                lum -= cd;
            }

            // ---- color de PIEDRA (calido/neutro r>=g>=b) ----
            int r = lum + 2, g = lum - 3, b = lum - 9;

            // ---- 4c) MUSGO / humedad verdosa (juntas + pie del muro): oscurece con tinte verde ----
            {
                int moss = 0;
                int damp = 0;
                if (y > ySill) damp = (y - ySill);        // 0..23 sube hacia el pie del muro
                if (y > A1)    damp += (y - A1) * 3;       // refuerzo fuerte en el zocalo (y123..127)
                if (damp > 0) { int mm = H(x >> 2, 88); if (mm < 150) moss += ((150 - mm) * damp) / 260; } // patchy
                if (hj == 0 || vj == 0) { int mm2 = H(x >> 1, y >> 2); if (mm2 < 70) moss += (70 - mm2) / 6; } // juntas
                if (moss > 0) {
                    int m = moss > 16 ? 16 : moss;
                    r -= m; g -= m * 2 / 5; b -= m + m / 3;  // musgo: g cae MENOS (verde), b cae mas (no azulea)
                }
            }

            // ---- 5) HORNACINAS con estatua en el contrafuerte (figuras entre vanos) ----
            for (int k = 0; k < 2; ++k) {
                int ny = k ? (W * 11 / 16) : (W * 5 / 16);   // 88 y 40
                if (AB(xw) <= 6 && y >= ny - 11 && y <= ny + 9) {
                    bool inside = true;
                    if (y < ny - 5 && AB(xw) > 6 - ((ny - 5) - y)) inside = false;  // cabeza redondeada del nicho
                    if (inside) {
                        bool body = (AB(xw) <= 2 && y >= ny - 6 && y <= ny + 7);    // cuerpo estrecho
                        bool head = (AB(xw) <= 1 && y >= ny - 9 && y <  ny - 6);    // cabeza
                        if (body || head) { r = 138; g = 130; b = 118; }           // figura palida (piedra tallada)
                        else              { r = 62;  g = 58;  b = 52;  }           // recess oscuro del nicho
                    }
                }
            }

            // ---- 6) VIDRIO: traceria fina (piedra) sobre vidrio oscuro/frio o AMBAR encendido ----
            if (glass) {
                bool bar = false;                                        // barra de traceria = piedra
                if (lanL && AB(x - lx) <= 1) bar = true;                 // parteluz lanceta izq
                if (lanR && AB(x - rx) <= 1) bar = true;                 // parteluz lanceta der
                if ((lanL || lanR) && AB(y - ym) <= 1) bar = true;       // travesano
                if (lanL && y < lyS) { int q = D2(x,y,lx,lyS-8); if (q >= 9 && q <= 25) bar = true; } // cuatrifolio izq
                if (lanR && y < lyS) { int q = D2(x,y,rx,lyS-8); if (q >= 9 && q <= 25) bar = true; } // cuatrifolio der
                if (rose) {                                              // roseton: cubo + radios + aro
                    if (rd2 <= 9) bar = true;                                        // cubo central
                    if (AB(rdx) <= 1 || AB(rdy) <= 1 || AB(AB(rdx)-AB(rdy)) <= 1) bar = true; // 8 radios
                    int ir = roseR / 2;                                              // aro de foils interior
                    if (AB(rd2 - ir*ir) <= ir) bar = true;
                    if (rd2 >= (roseR-1)*(roseR-1)) bar = true;                       // aro exterior
                }
                if (bar) {                                               // piedra de la traceria (lee clara)
                    r = 86; g = 82; b = 74;
                } else if (lanR) {                                       // UNICA lanceta ENCENDIDA (ambar)
                    int gdx = x - rx, gdy = y - ym;
                    int dd = gdx*gdx*3 + gdy*gdy, fall = dd / 7;
                    r = 232 - fall; if (r < 150) r = 150;                // nucleo caliente -> borde tibio
                    g = 176 - fall*11/10; if (g < 104) g = 104;
                    b = 92  - fall*12/10; if (b < 52)  b = 52;
                    if (((x + y) & 7) == 0) { r -= 18; g -= 16; b -= 12; }// plomos de la vidriera
                } else {                                                // vidrio OSCURO/FRIO (mayoria)
                    int gl = 52 + (H(x, y) % 5 - 2);
                    r = gl - 2; g = gl; b = gl + 6;                      // levemente frio (vano apagado)
                    if (rose && rdx == -2) { r += 8; g += 9; b += 12; }  // reflejo fino en el roseton
                    if (rose && rdx > 2 && rdy < -2) { r += 46; g += 26; b += 2; } // un petalo tibio (acento)
                    if (rose && rdx < -3 && rdy > 3)  { r += 38; g += 22; b += 2; } // segundo petalo tibio
                }
            }

            r = C(r, 44, 235); g = C(g, 42, 224); b = C(b, 40, 214);   // nada casi-negro, ambar sin recorte
            t[y * W + x] = RGBA(r, g, b, 255);
        }
    }
}
// NOTA (verificacion):
// Elementos goticos pintados: contrafuerte macizo con MEGA-junta vertical (escala BLAME,
//   corre sin cortarse por las cornisas); arco ojival envolvente recesado; DOS lancetas
//   apuntadas con parteluz, travesano y cuatrifolio; ROSETON (cubo + 8 radios + aro de
//   foils); cornisa + tabla de canecillos, imposta y alfeizar (string-courses); ARQUERIA
//   CIEGA (fila de arquitos apuntados) como zocalo en la base; DOS hornacinas con figura
//   palida (estatuas entre vanos).  VEJEZ: silleria con tono por bloque + spolia (bloques
//   ahollinados / frescos) + juntas de profundidad variable; regueros verticales de agua
//   desde cornisa/imposta/alfeizar; musgo/humedad verdosa en juntas y base; fisuras finas
//   asimetricas; esquinas descantilladas.  El curtido es ASIMETRICO (regueros, grietas y
//   lanceta ambar no espejados) para que la tesela repetida no se lea como un sello limpio.
// Brillos (para MODULATE ~x2.7): piedra media ~95-150 (regueros/musgo la bajan a ~95, sin
//   ennegrecer); derrames/parteluces/sombra de contrafuerte/MEGA-junta/recess ~55-85;
//   vidrio oscuro/frio ~50-60; AMBAR encendido ~180-232; piedra calida/neutra r>=g>=b
//   (excepcion buscada: el MUSGO de juntas y pie del muro deja g>r -> tinte verde damp,
//   unico verde de la piedra).  El grueso queda medio (no oscura).
// TESELA (ambos ejes, potencia de 2, GU_REPEAT):
//   - Silleria: hiladas 16px y llagas 32px (128 multiplo de ambos); col/curso via
//     ((x+off)&127)>>5 -> hash de bloque, spolia y chips PERIODICOS en x mod W.
//   - Canecillos periodo 16px; ARQUERIA CIEGA periodo 16px (arco centrado en x&15==8) y
//     confinada en y a [108,122] -> interior, no toca y=0/W.
//   - Cornisa/imposta/alfeizar y contrafuerte usan distancia CON SIGNO a las costuras
//     (dyT, xw) -> continuos al envolver; el nicho igual (xw).
//   - Regueros: funcion de columna (H de x>>2 y x) modulada por y -> rasgo VERTICAL, la
//     costura vertical solo junta dos columnas independientes (sin estructura horizontal
//     que empatar).  Fisuras: segmentos INTERIORES (x in [9,104], y in [20,126]) que nunca
//     tocan una costura.  Musgo: H(x>>1,y>>2) periodico + banda de base interior.
//   - Vano, lancetas y roseton son INTERIORES (x in [16,112], y in [16,104]): nunca tocan
//     x=0/W ni y=0/W.  => TESELA PERFECTA en ambos ejes.
