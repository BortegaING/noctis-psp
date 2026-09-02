#pragma once
#include <math.h>
// PROJECT NOCTIS: BASILICA ANTIGUA HORIZONTAL (la del OESTE, footprint alargado en Z).
// No es una aguja vertical: es una MOLE larga y pesada, vieja, tumbada sobre el eje Z.
// La FIRMA es la SILUETA: el ritmo de una HILERA de ARBOTANTES a ambos flancos largos
// + la linea de techo continua a dos aguas + un abside facetado al fondo. Cada elemento
// es simple, pero repetido con cadencia da "alma" (no backrooms). El jugador la ve por su
// FLANCO ESTE largo (+X): ahi es donde el bosque de arbotantes y el clerestorio se leen.
//
// Convenciones motor (main.cpp): addSolidBoxT[30v] addPyramidT[12v] addQuadT[6v]
// addPinnacle[42v] addWinRow(->g_win, NO cuenta al presupuesto). Cull OFF en el pase
// solido -> los quads (rampas de arbotante, faldones, paneles del abside) se ven por
// ambas caras. Muros = brighten(baseColor,2.7f); pizarra = RGBA(30,28,26); niebla por
// altura heightHaze(col,y) y por distancia fadeToVoid(col,dist).
//
// Firma EXACTA (no tocar): eje largo = Z (d~72), corto = X (w~48), alto h~150.
static void buildCathedralBasilica(TexVertex* buf, int& i, float cx, float cz,
                                   float w, float d, float h, unsigned int baseColor,
                                   float dist, int* detailStartOut) {
    const float PI = 3.14159265f;

    // ---- PALETA: piedra calida legible (MODULATE 2.7x), pizarra marron oscura, ambar tenue ----
    const unsigned int stone = fadeToVoid(warmTint(brighten(baseColor, 2.7f)), dist);
    const unsigned int slate = fadeToVoid(RGBA(30, 28, 26, 255), dist);
    const unsigned int win   = RGBA(150, 112, 60, 255);   // vidriera ambar TENUE

    // ---- COTAS (eje largo = Z) ----
    const float z0 = cz - d * 0.5f, z1 = cz + d * 0.5f;   // extremos de la nave en Z
    const float naveW  = w * 0.40f;                       // nave central (moderada)
    const float aisleW = w * 0.27f;                       // naves laterales
    const float aisleOff = naveW * 0.5f + aisleW * 0.5f;  // X del eje de cada nave lateral
    const float hAisle = h * 0.30f;                       // naves laterales (bajas)
    const float hNave  = h * 0.48f;                       // muro de la nave hasta base del clerestorio
    const float hCler  = h * 0.15f;                       // banda del clerestorio (ventanas altas)
    const float clerW  = naveW * 0.86f;                   // clerestorio un pelo mas angosto (escalon)
    const float clerTop = hNave + hCler;                  // alero del clerestorio
    const float dRoof = d * 0.99f;
    const float zc0 = cz - dRoof * 0.5f, zc1 = cz + dRoof * 0.5f;
    const float ridgeY = clerTop + h * 0.10f;             // caballete (dos aguas) sobre el clerestorio
    const float exE = cx + clerW * 0.5f, exW = cx - clerW * 0.5f;  // aleros E/W del clerestorio

    // ================= NUCLEO (nave + clerestorio + naves laterales + techo) =================
    // Zocalo pesado continuo: base ancha que hace sentir la mole "vieja y asentada".
    addSolidBoxT(buf, i, cx, 0.0f, cz, w * 1.02f, d * 1.01f, h * 0.03f,
                 heightHaze(brighten(stone, 0.86f), h * 0.015f));                        // 30 zocalo
    // Nave central (recorre TODO el largo d en Z), alta hasta la base del clerestorio.
    addSolidBoxT(buf, i, cx, 0.0f, cz, naveW, d, hNave,
                 heightHaze(stone, hNave * 0.5f));                                       // 30 nave
    // Clerestorio: caja mas angosta y elevada sobre la nave = nivel de la banda de ventanas.
    addSolidBoxT(buf, i, cx, hNave, cz, clerW, dRoof, hCler,
                 heightHaze(brighten(stone, 1.05f), hNave + hCler * 0.5f));              // 30 clerestorio
    // Naves laterales: dos cajas bajas flanqueando la nave a lo largo de todo Z.
    addSolidBoxT(buf, i, cx - aisleOff, 0.0f, cz, aisleW, dRoof, hAisle,
                 heightHaze(stone, hAisle * 0.5f));                                      // 30 nave lat -X
    addSolidBoxT(buf, i, cx + aisleOff, 0.0f, cz, aisleW, dRoof, hAisle,
                 heightHaze(stone, hAisle * 0.5f));                                      // 30 nave lat +X
    // Cornisa/alero pesado del clerestorio (pizarra volada): remata la linea de techo larga.
    addSolidBoxT(buf, i, cx, clerTop, cz, clerW * 1.12f, dRoof * 1.02f, h * 0.014f,
                 heightHaze(brighten(slate, 0.95f), clerTop));                           // 30 cornisa

    // TECHO A DOS AGUAS continuo sobre el clerestorio: dos faldones largos (Z) + tapas de hastial.
    {
        const unsigned int sl = heightHaze(slate, clerTop);
        const float uz = dRoof / TILE, us = (clerW * 0.6f) / TILE;
        // faldon ESTE (mira al +X, el flanco que ve el jugador)
        addQuadT(buf, i, exE, clerTop, zc0, exE, clerTop, zc1, cx, ridgeY, zc1, cx, ridgeY, zc0,
                 0, us, uz, 0, brighten(sl, 0.94f));                                     // 6
        // faldon OESTE (-X)
        addQuadT(buf, i, exW, clerTop, zc1, exW, clerTop, zc0, cx, ridgeY, zc0, cx, ridgeY, zc1,
                 0, us, uz, 0, brighten(sl, 1.08f));                                     // 6
        // hastial NORTE (zc0) y SUR (zc1): triangulo (4o vertice == cumbre -> 2o tri degenerado)
        addQuadT(buf, i, exW, clerTop, zc0, exE, clerTop, zc0, cx, ridgeY, zc0, cx, ridgeY, zc0,
                 0, (hCler + h * 0.10f) / TILE, clerW / TILE, 0, heightHaze(stone, clerTop)); // 6
        addQuadT(buf, i, exE, clerTop, zc1, exW, clerTop, zc1, cx, ridgeY, zc1, cx, ridgeY, zc1,
                 0, (hCler + h * 0.10f) / TILE, clerW / TILE, 0, heightHaze(stone, clerTop)); // 6
    }
    // Faldones a un agua sobre las naves laterales: del muro de la nave (alto) al alero exterior (bajo).
    {
        const unsigned int sl = heightHaze(slate, hAisle);
        const float aHi = hAisle + h * 0.02f, aLo = hAisle * 0.96f;
        const float inX = naveW * 0.5f, outX = aisleOff + aisleW * 0.5f;
        const float uz = dRoof / TILE, us = (outX - inX) / TILE;
        // lateral -X
        addQuadT(buf, i, cx - inX, aHi, zc0, cx - inX, aHi, zc1, cx - outX, aLo, zc1, cx - outX, aLo, zc0,
                 0, us, uz, 0, brighten(sl, 1.06f));                                     // 6
        // lateral +X
        addQuadT(buf, i, cx + inX, aHi, zc1, cx + inX, aHi, zc0, cx + outX, aLo, zc0, cx + outX, aLo, zc1,
                 0, us, uz, 0, brighten(sl, 0.92f));                                     // 6
    }

    // >>> LOD: el detalle pesado (arbotantes + abside + torre + fachada) empieza aca <<<
    if (detailStartOut) *detailStartOut = i;

    // ================= DETALLE: TORRE DE CRUCERO baja (mole central) =================
    {
        const float ctW = naveW * 1.35f;
        const float ctBase = clerTop, ctH = h * 0.22f;
        addSolidBoxT(buf, i, cx, ctBase, cz, ctW, ctW, ctH,
                     heightHaze(brighten(stone, 1.02f), ctBase + ctH * 0.5f));           // 30 cuerpo
        addPyramidT(buf, i, cx, ctBase + ctH, cz, ctW, ctW, h * 0.10f,
                    heightHaze(brighten(slate, 1.10f), ctBase + ctH));                    // 12 remate chato
        const float p = ctW * 0.5f - 1.0f;                                               // 4 pinaculos de esquina
        for (int s = -1; s <= 1; s += 2)
            for (int t = -1; t <= 1; t += 2)
                addPyramidT(buf, i, cx + s * p, ctBase + ctH * 0.9f, cz + t * p,
                            1.4f, 1.4f, h * 0.06f,
                            heightHaze(brighten(slate, 1.12f), ctBase + ctH));            // 12 c/u -> 48
    }

    // ================= DETALLE: HILERA DE ARBOTANTES (LA FIRMA) =================
    // 6 por flanco largo. Cada uno = pier exterior (caja fina alta) + rampa ARQUEADA (quad)
    // que salta del muro del clerestorio a la cima del pier + pinaculo. Cadencia pareja en Z.
    {
        const int NB = 6;
        const float pierOff = w * 0.54f;              // X del pier: justo por fuera de la nave lateral
        const float pierH   = h * 0.40f;              // pier alto (recibe el arbotante)
        const float margin  = d * 0.06f;              // no pegados a los extremos
        const float span    = d - 2.0f * margin;
        const float innerY  = hNave * 0.98f;          // arranque alto sobre el clerestorio
        const float thz     = d * 0.010f;             // grosor del puntal en Z (fino)
        for (int s = -1; s <= 1; s += 2) {
            const float px = cx + s * pierOff;
            const float innerX = cx + s * (clerW * 0.5f);
            for (int m = 0; m < NB; ++m) {
                const float bz = z0 + margin + span * ((m + 0.5f) / NB);
                // pier exterior
                addSolidBoxT(buf, i, px, 0.0f, bz, w * 0.055f, d * 0.045f, pierH,
                             heightHaze(stone, pierH * 0.5f));                            // 30 pier
                // rampa/arco volador: quad inclinado del clerestorio (alto) al pier (bajo)
                const unsigned int ac = heightHaze(brighten(stone, 1.06f), innerY * 0.5f);
                addQuadT(buf, i, innerX, innerY, bz - thz, innerX, innerY, bz + thz,
                         px, pierH, bz + thz, px, pierH, bz - thz,
                         0, 0, (pierOff - clerW * 0.5f) / TILE, 1, ac);                   // 6 arco volador
                // pinaculo sobre el pier
                addPyramidT(buf, i, px, pierH, bz, w * 0.055f, d * 0.045f, h * 0.055f,
                            heightHaze(brighten(slate, 1.12f), pierH));                   // 12 pinaculo
            }
        }
        // 48 verts * 12 arbotantes = 576 (el gasto mayor; piers a proposito SIMPLES)
    }

    // ================= DETALLE: ABSIDE FACETADO al fondo (-Z) =================
    // Medio-poligono de 5 paneles de muro ANGULADOS (quads verticales) + techo conico (piramide).
    {
        const int NF = 5;
        const float apR = naveW * 0.5f;               // radio: los extremos casan con la nave
        const float apCz = z0;                        // centro del semicirculo en el borde de la nave
        const float apH  = hAisle * 1.08f;            // coro alto
        const unsigned int wc = heightHaze(stone, apH * 0.5f);
        for (int f = 0; f < NF; ++f) {
            const float a0 = PI * f / NF, a1 = PI * (f + 1) / NF;
            const float x0f = cx - apR * cosf(a0), zf0 = apCz - apR * sinf(a0);
            const float x1f = cx - apR * cosf(a1), zf1 = apCz - apR * sinf(a1);
            const float dx = x1f - x0f, dz = zf1 - zf0;
            const float chord = sqrtf(dx * dx + dz * dz);
            addQuadT(buf, i, x0f, 0.0f, zf0, x1f, 0.0f, zf1, x1f, apH, zf1, x0f, apH, zf0,
                     0, apH / TILE, chord / TILE, 0, wc);                                 // 6 c/u -> 30
        }
        // techo conico del abside (piramide baja sobre el centroide del semicirculo)
        addPyramidT(buf, i, cx, apH, apCz - apR * 0.45f, apR * 1.9f, apR * 1.15f, h * 0.14f,
                    heightHaze(brighten(slate, 1.06f), apH));                             // 12
    }

    // ================= DETALLE: FACHADA OESTE (entrada, +Z) =================
    // Bloque frontal macizo + hastial + portal ojival (marco/vano/arco) + recuadro del roseton.
    {
        const float wfW = w * 0.72f, wfD = d * 0.06f, wfH = hNave * 1.02f;
        const float wfCz = z1 + wfD * 0.30f;
        addSolidBoxT(buf, i, cx, 0.0f, wfCz, wfW, wfD, wfH,
                     heightHaze(stone, wfH * 0.5f));                                      // 30 bloque
        addPyramidT(buf, i, cx, wfH, wfCz, wfW, wfD, h * 0.10f,
                    heightHaze(slate, wfH));                                              // 12 hastial
        const float fz = z1 + wfD * 0.60f;
        const float pw = wfW * 0.22f;
        float ph = h * 0.16f;
        const unsigned int frame = heightHaze(brighten(stone, 1.25f), ph * 0.5f);
        addSolidBoxT(buf, i, cx, 0.0f, fz + 0.10f, pw * 1.30f, 0.5f, ph * 1.10f, frame);  // 30 marco
        addSolidBoxT(buf, i, cx, 0.0f, fz + 0.30f, pw, 0.6f, ph, RGBA(8, 9, 13, 255));    // 30 vano oscuro
        addPyramidT(buf, i, cx, ph, fz + 0.25f, pw * 1.20f, 0.6f, pw * 0.90f, frame);     // 12 arco apuntado
        const float roseY = ph * 1.25f + h * 0.05f;
        addSolidBoxT(buf, i, cx, roseY - w * 0.09f, fz + 0.05f, w * 0.20f, 0.4f, w * 0.18f,
                     heightHaze(brighten(stone, 1.28f), roseY));                          // 30 recuadro roseton
    }

    // ================= VENTANAS (van a g_win, NO cuentan al presupuesto de buf) =================
    // Banda de vidrieras del clerestorio en AMBOS flancos largos (varias filas).
    for (int r = 0; r < 3; ++r) {
        const float y = hNave + hCler * (0.30f + 0.24f * r);
        addWinRow(cx, cz, clerW, dRoof, y, 2, brighten(win, 1.15f));   // +X (flanco del jugador)
        addWinRow(cx, cz, clerW, dRoof, y, 3, brighten(win, 1.15f));   // -X
    }
    // Ventanas bajas de las naves laterales (cara exterior de cada una).
    addWinRow(cx + aisleOff, cz, aisleW, dRoof, hAisle * 0.55f, 2, win);
    addWinRow(cx - aisleOff, cz, aisleW, dRoof, hAisle * 0.55f, 3, win);
    // Lancetas del abside (-Z) y ROSETON de la fachada oeste (+Z), marcados.
    addWinRow(cx, z0, naveW * 0.7f, 1.0f, hAisle * 0.6f, 1, win);
    addWinRow(cx, z1, w * 0.42f, 0.6f, h * 0.16f * 1.25f + h * 0.05f, 0, brighten(win, 1.35f));
}
//
// -------------------------------------------------------------------------------------------
// NOTA (elementos / conteo de vertices en buf=g_solidWorld / detailStartOut):
//
// NUCLEO (antes de *detailStartOut) -- se dibuja siempre, incluso lejos:
//   zocalo 30 + nave 30 + clerestorio 30 + 2 naves laterales 60 + cornisa 30      = 180
//   techo dos aguas: 2 faldones (6+6) + 2 hastiales (6+6)                         =  24
//   2 faldones a un agua de las naves laterales (6+6)                             =  12
//   -> NUCLEO = 216 verts.  *detailStartOut = i queda fijado aqui (i == 216 local).
//
// DETALLE (despues de *detailStartOut) -- culleable por distancia (LOD):
//   torre de crucero: cuerpo 30 + remate 12 + 4 pinaculos (4*12=48)               =  90
//   HILERA DE ARBOTANTES (la firma): 12 * (pier 30 + arco 6 + pinaculo 12 = 48)   = 576
//   abside facetado: 5 paneles angulados (5*6=30) + techo conico 12               =  42
//   fachada oeste: bloque 30 + hastial 12 + marco 30 + vano 30 + arco 12 + roseton 30 = 144
//   -> DETALLE = 852 verts.
//
// TOTAL buf = 216 + 852 = 1068 verts (<= ~2000, holgado).
// Ventanas (addWinRow) -> buffer g_win aparte: ~8 filas, NO cuentan a este presupuesto.
// -------------------------------------------------------------------------------------------
