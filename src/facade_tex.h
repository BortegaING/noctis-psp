#pragma once
// FACHADA GOTICA DE CATEDRAL (Yharnam / Bloodborne + escala BLAME).  TODO el detalle
// vive en la TEXTURA (el muro es solo masa/silueta, sin geometria por ventana).  El
// modulo de W*W (STEX=128 => ~6 m de mundo por tesela, TILE=6) es UN VANO monumental,
// NO una reja de ventanitas iguales de oficina:
//   - PILAR/CONTRAFUERTE macizo en la costura (junta MEGA-PANEL escala BLAME, corre
//     vertical SIN cortarse por las cornisas -> lee "mas grande que un piso").
//   - Un ARCO OJIVAL envolvente recesado (el vano) con TRACERIA de piedra que talla:
//   - DOS LANCETAS altas y estrechas (arco apuntado de dos arcos, parteluz central,
//     travesano, y un CUATRIFOLIO/oculo en la cabeza de cada una) bajo el arco mayor.
//   - Un ROSETON: rueda circular con 8 radios + aro de foils, foco de la composicion.
//   - Cornisa + TABLA DE CANECILLOS (corbel table) arriba, imposta en el arranque y
//     alfeizar abajo (string-courses horizontales); SILLERIA (ashlar) a soga como grano.
//   - DOS HORNACINAS con figura palida en el contrafuerte (estatuas entre vanos).
//   - Casi todo el vidrio OSCURO/FRIO; UNA lanceta encendida AMBAR (vidriera, unico
//     acento saturado) + un petalo tibio del roseton.
// Determinista (hash entero, sin rand, sin heap; solo <math.h> no hace falta aqui).
// FUNCION EXACTA de (x mod W, y mod W): todo rasgo periodico -> TESELA PERFECTA en
// ambos ejes bajo GU_REPEAT (ver NOTA final).
// Brillo pensado para GU_TFX_MODULATE contra el vertice de piedra (baseColor x2.7):
//   piedra media ~95-150 | derrames/parteluces/sombra de contrafuerte ~55-85 |
//   vidrio oscuro ~50-60 | AMBAR encendido ~150-232.  Piedra calida/neutra r>=g>=b.
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

    // ---- utilidades enteras ----
    auto H = [](int a, int b) -> int {                 // hash determinista -> 0..255
        unsigned int h = (unsigned int)a * 374761393u + (unsigned int)b * 668265263u + 0x9E3779B9u;
        h = (h ^ (h >> 13)) * 1274126177u; h ^= (h >> 16);
        return (int)(h & 0xFFu);
    };
    auto C  = [](int v, int lo, int hi) -> int { return v < lo ? lo : (v > hi ? hi : v); };
    auto AB = [](int v) -> int { return v < 0 ? -v : v; };
    auto D2 = [](int ax, int ay, int bx, int by) -> int {
        int dx = ax - bx, dy = ay - by; return dx*dx + dy*dy;
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

            // ---- 1) SILLERIA (ashlar) a soga: base media + juntas de tendel/llaga + grano ----
            int tnt  = (H(x >> 4, y >> 4) % 7) - 3;      // tinte calido por bloque
            int lum  = 120 + tnt + (H(x, y) % 7 - 3);    // piedra MEDIA (para el MODULATE)
            int hj   = y & 15;                           // hilada cada 16px (128/16=8 -> tesela)
            int off  = ((y >> 4) & 1) ? (W >> 3) : 0;    // soga: media pieza alterna por hilada
            int vj   = (x + off) & 31;                   // llaga cada 32px (128/32=4 -> tesela)
            if      (hj == 0) lum -= 13;                 // tendel (junta horizontal, sombra)
            else if (hj == 1) lum += 6;                  // labio iluminado bajo el tendel
            if      (vj == 0) lum -= 11;                 // llaga (junta vertical)
            else if (vj == 1) lum += 4;

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

            // ---- color de PIEDRA (calido/neutro r>=g>=b) ----
            int r = lum + 2, g = lum - 3, b = lum - 9;

            // ---- 5) HORNACINAS con estatua en el contrafuerte (figuras entre vanos) ----
            for (int k = 0; k < 2; ++k) {
                int ny = k ? (W * 11 / 16) : (W * 5 / 16);   // 88 y 40
                if (AB(xw) <= 6 && y >= ny - 11 && y <= ny + 9) {
                    bool inside = true;
                    if (y < ny - 5 && AB(xw) > 6 - ((ny - 5) - y)) inside = false;  // cabeza redondeada del nicho
                    if (inside) {
                        bool body = (AB(xw) <= 2 && y >= ny - 6 && y <= ny + 7);    // cuerpo estrecho
                        bool head = (AB(xw) <= 1 && y >= ny - 9 && y <  ny - 6);    // cabeza
                        if (body || head) { r = 126; g = 119; b = 108; }           // figura palida (piedra)
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
//   foils); cornisa + tabla de canecillos, imposta y alfeizar (string-courses); silleria
//   ashlar a soga como grano; DOS hornacinas con figura palida (estatuas entre vanos).
// Brillos (para MODULATE x2.7): piedra media ~95-150; derrames/parteluces/sombra de
//   contrafuerte/MEGA-junta ~55-85; vidrio oscuro/frio ~50-60; AMBAR encendido ~150-232;
//   piedra calida/neutra r>=g>=b.  El grueso de la textura queda medio (no oscura).
// TESELA: cada rasgo es funcion periodica de (x mod W, y mod W).  Silleria: hiladas 16px
//   y llagas 32px (128 multiplo de ambos).  Canecillos periodo 16px.  Cornisa/nicho/
//   contrafuerte usan distancia CON SIGNO a las costuras (xw, dyT) -> continuos al
//   envolver.  Vano, lancetas y roseton son INTERIORES (x in [16,112], y in [16,104]):
//   nunca tocan x=0/W ni y=0/W.  => TESELA PERFECTA en ambos ejes bajo GU_REPEAT.
