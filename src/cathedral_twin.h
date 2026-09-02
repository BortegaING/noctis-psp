#pragma once
#include <math.h>
// PROJECT NOCTIS: CATEDRAL GEMELA -- la ELEGANTE VERTICAL (feel Colonia, ORIGINAL).
// Dos AGUJAS CALADAS que se disparan al cielo: verticalidad gotica EXTREMA. La
// SILUETA es la que vende: las agujas DOMINAN el horizonte, MUY por encima del
// cuerpo. No es una caja de backrooms -> tiene alma: cuerpo compacto y alto que
// estrecha en 3 tramos, torre-LINTERNA esbelta entre las agujas, contrafuertes de
// esquina ESCALONADOS rematados en pinaculo, gablete APUNTADO con roseton recesado
// y portal ojival, todo en la cara OESTE (-X, la que ve el jugador desde la plaza).
// Las agujas leen TIERED/OCTOGONAL (cajas que estrechan y ALTERNAN), no un cono liso.
static void buildCathedralTwin(TexVertex *buf, int &i, float cx, float cz,
                               float w, float d, float h, unsigned int baseColor,
                               float dist, int *detailStartOut) {
    // ---- PALETA: muro de piedra calida legible; agujas de pizarra casi negra
    //      (silueta contra la bruma); vidriera ambar tenue. Todo se funde por
    //      distancia (fadeToVoid) y por altura (heightHaze) hacia la neblina. ----
    const unsigned int stone = fadeToVoid(brighten(baseColor, 2.7f), dist);  // piedra del muro
    const unsigned int slate = fadeToVoid(RGBA(30, 28, 26, 255), dist);      // pizarra de las agujas
    const unsigned int win   = RGBA(150, 118, 66, 255);                      // vidriera ambar tenue
    const unsigned int dark  = RGBA(8, 9, 14, 255);                          // vano recesado oscuro

    // ---- GEOMETRIA MAESTRA ----
    const float bodyW   = w * 0.86f;
    const float bodyD   = d * 0.96f;
    const float bodyCx  = cx + w * 0.07f;   // cuerpo un pelo al ESTE -> las agujas quedan al frente
    const float bodyCz  = cz;
    const float frontX  = cx - w * 0.30f;   // linea de FACHADA (-X): de aqui nacen las agujas
    const float spireDZ = d * 0.30f;        // media separacion de las agujas (en Z)
    const float tw      = w * 0.25f;        // planta de la aguja en X
    const float td      = d * 0.22f;        // planta de la aguja en Z (alterna -> feel octogonal)
    const float bodyTop = h * 0.50f;        // el cuerpo llega a MEDIA altura; las agujas lo DOBLAN

    // ================= CUERPO: 3 tramos que ESTRECHAN (enfasis vertical) + techo empinado =================
    const float b1 = bodyTop * 0.50f;                       // planta baja (ancho pleno)
    const float b2 = bodyTop * 0.30f;                       // naves laterales
    const float b3 = bodyTop - b1 - b2;                     // clerigo (nave central)
    const float bw2 = bodyW * 0.86f, bd2 = bodyD * 0.94f;
    const float bw3 = bodyW * 0.70f, bd3 = bodyD * 0.90f;
    const float shoulderY = b1 + b2;                        // hombro del clerigo
    addSolidBoxT(buf, i, bodyCx, 0.0f,      bodyCz, bodyW, bodyD, b1, heightHaze(stone, b1 * 0.5f));            // 30
    addSolidBoxT(buf, i, bodyCx, b1,        bodyCz, bw2,   bd2,   b2, heightHaze(stone, b1 + b2 * 0.5f));       // 30
    addSolidBoxT(buf, i, bodyCx, shoulderY, bodyCz, bw3,   bd3,   b3, heightHaze(stone, shoulderY + b3 * 0.5f));// 30
    addPyramidT (buf, i, bodyCx, bodyTop,   bodyCz, bw3,   bd3,   h * 0.10f, heightHaze(slate, bodyTop));       // 12 techo empinado
    // subtotal cuerpo = 102

    // ================= LAS DOS AGUJAS (cores): fuste alto + stack calado + aguja finisima =================
    // Cada aguja = fuste (caja alta y esbelta) + 4 cajas que ESTRECHAN y ALTERNAN w<->d
    // (lectura octogonal/escalonada, NO cono liso) + piramide MUY alta y fina que corona.
    const float shaftTop = h * 0.50f;       // fuste
    const float tierH    = h * 0.07f;       // alto de cada tramo del stack
    for (int s = -1; s <= 1; s += 2) {
        const float sz = cz + (float)s * spireDZ;
        addSolidBoxT(buf, i, frontX, 0.0f, sz, tw, td, shaftTop, heightHaze(stone, shaftTop * 0.5f)); // 30 fuste
        float y = shaftTop;
        for (int t = 0; t < 4; ++t) {
            const float shrink = 0.90f - 0.06f * (float)t;        // 0.90, 0.84, 0.78, 0.72
            const float ww = tw * shrink, dd = td * shrink;
            const float aw = (t & 1) ? ww * 1.06f : ww * 0.94f;   // alterna X<->Z por tramo -> octogonal
            const float ad = (t & 1) ? dd * 0.94f : dd * 1.06f;
            addSolidBoxT(buf, i, frontX, y, sz, aw, ad, tierH, heightHaze(slate, y + tierH * 0.5f)); // 30 x4
            y += tierH;
        }
        addPyramidT(buf, i, frontX, y, sz, tw * 0.62f, td * 0.62f, h * 0.30f, heightHaze(slate, y)); // 12 aguja
    }
    // subtotal por aguja = 30 + 120 + 12 = 162 ; x2 = 324  (acumulado 426)

    // ================= LINTERNA entre las agujas (core): caja esbelta + campanil + agujita + gablete =================
    const float lanX    = cx - w * 0.14f;   // centrada entre las torres, un pelo al este de la fachada
    const float lanW    = w * 0.20f, lanD = d * 0.20f;
    const float lanBase = shoulderY;        // arranca sobre las naves
    const float lanTop  = h * 0.72f;        // MAS alta que el cuerpo, MENOR que las agujas
    addSolidBoxT(buf, i, lanX, lanBase, cz, lanW, lanD, lanTop - lanBase, heightHaze(stone, (lanBase + lanTop) * 0.5f)); // 30
    addSolidBoxT(buf, i, lanX, lanTop,  cz, lanW * 0.80f, lanD * 0.80f, h * 0.05f, heightHaze(stone, lanTop));           // 30 campanil
    addPyramidT (buf, i, lanX, lanTop + h * 0.05f, cz, lanW * 0.72f, lanD * 0.72f, h * 0.16f, heightHaze(slate, lanTop));// 12 aguja linterna
    // subtotal linterna = 72  (acumulado 498)

    // ===== LOD: el detalle ornamental (pinaculos + gablete + roseton + portal) se puede cullear de lejos =====
    if (detailStartOut) *detailStartOut = i;   // marca JUSTO antes de pinaculos + gablete

    // ---- CROCKETS: corona de pinaculos finos en el remate del cuerpo de cada aguja (bristle calado) ----
    const float crockY = shaftTop + 4.0f * tierH;   // base de la aguja finisima
    for (int s = -1; s <= 1; s += 2) {
        const float sz = cz + (float)s * spireDZ;
        for (int ox = -1; ox <= 1; ox += 2)
        for (int oz = -1; oz <= 1; oz += 2) {
            addPyramidT(buf, i, frontX + (float)ox * tw * 0.42f, crockY - h * 0.02f,
                        sz + (float)oz * td * 0.42f, tw * 0.16f, td * 0.16f, h * 0.09f,
                        heightHaze(slate, crockY)); // 12 x16
        }
    }
    // crockets = 96  (acumulado 594)

    // ---- CONTRAFUERTES DE ESQUINA del cuerpo (4): 2 escalones + pinaculo fino y alto ----
    const float bpx = bodyW * 0.5f, bpz = bodyD * 0.5f;
    for (int sx = -1; sx <= 1; sx += 2)
    for (int sz2 = -1; sz2 <= 1; sz2 += 2) {
        const float px = bodyCx + (float)sx * bpx;
        const float pz = bodyCz + (float)sz2 * bpz;
        addSolidBoxT(buf, i, px, 0.0f,           pz, w * 0.09f, d * 0.09f, bodyTop * 0.55f,
                     heightHaze(stone, bodyTop * 0.28f));                                    // 30 escalon bajo
        addSolidBoxT(buf, i, px, bodyTop * 0.55f, pz, w * 0.07f, d * 0.07f, bodyTop * 0.28f,
                     heightHaze(stone, bodyTop * 0.70f));                                    // 30 escalon alto (inset)
        addPyramidT (buf, i, px, bodyTop * 0.83f, pz, w * 0.07f, d * 0.07f, h * 0.12f,
                     heightHaze(slate, bodyTop * 0.83f));                                    // 12 pinaculo
    }
    // contrafuertes = 4 x 72 = 288  (acumulado 882)

    // ---- GABLETE APUNTADO + ROSETON recesado en la cara OESTE del clerigo ----
    {
        const float fx = bodyCx - bodyW * 0.5f;                  // cara -X del cuerpo
        addSolidBoxT(buf, i, fx + 0.15f, shoulderY, bodyCz, 0.5f, bodyD * 0.42f, h * 0.06f,
                     heightHaze(stone, shoulderY));                                          // 30 base del gablete
        addPyramidT (buf, i, fx + 0.10f, shoulderY + h * 0.06f, bodyCz, 0.6f, bodyD * 0.42f, h * 0.12f,
                     heightHaze(slate, shoulderY + h * 0.06f));                              // 12 pico apuntado
        const float roseY = b1 + h * 0.10f;
        const unsigned int frame = heightHaze(brighten(stone, 1.25f), roseY);
        addSolidBoxT(buf, i, fx + 0.12f, roseY - w * 0.090f, bodyCz, 0.5f, w * 0.18f, w * 0.18f, frame); // 30 marco roseton
        addSolidBoxT(buf, i, fx + 0.26f, roseY - w * 0.075f, bodyCz, 0.4f, w * 0.15f, w * 0.15f, dark);  // 30 oculo oscuro
        addWinRow(bodyCx, bodyCz, bodyW, bodyD, roseY, 3, brighten(win, 1.35f));   // roseton iluminado (cara -X, a g_win)
    }
    // gablete + roseton = 30 + 12 + 30 + 30 = 102  (acumulado 984)

    // ---- PORTAL OJIVAL entre las torres, al pie de la fachada (la gran puerta oeste) ----
    {
        const float fx = frontX - tw * 0.40f;                   // adelantado hacia -X, centrado en Z
        const float pw = w * 0.16f;
        float ph = h * 0.16f; if (ph > 24.0f) ph = 24.0f;
        const unsigned int frame = heightHaze(brighten(stone, 1.20f), ph * 0.5f);
        addSolidBoxT(buf, i, fx + 0.15f, 0.0f, cz, 0.6f, pw * 1.3f, ph * 1.1f, frame); // 30 marco/jamba
        addSolidBoxT(buf, i, fx + 0.35f, 0.0f, cz, 0.6f, pw,        ph,        dark);  // 30 vano de la puerta
        addPyramidT (buf, i, fx + 0.20f, ph,   cz, 0.7f, pw * 1.2f, pw * 1.10f, frame);// 12 arco apuntado
    }
    // portal = 72  (acumulado 1056)

    // ================================================================================
    // NOTA (conteo EXACTO de vertices en buf; addWinRow escribe en g_win aparte):
    //   CORE (antes de detailStart):
    //     cuerpo 3 cajas + techo ............................  90 + 12 =  102
    //     2 agujas (fuste 30 + 4 tramos 120 + aguja 12 = 162)........ =  324
    //     linterna (caja 30 + campanil 30 + aguja 12) ..............  =   72
    //     ----------------------------------------------------- CORE  =  498  -> *detailStartOut = 498
    //   DETALLE (ornamento culleable por LOD):
    //     crockets 2 agujas x 4 esq. x 12 ..........................  =   96
    //     4 contrafuertes de esquina (30 + 30 + 12 = 72) ...........  =  288
    //     gablete apuntado + roseton recesado (30+12+30+30) ........  =  102
    //     portal ojival (30 + 30 + 12) .............................  =   72
    //     --------------------------------------------------- DETALLE  =  558
    //   ------------------------------------------------------ TOTAL buf = 1056 verts  (<= ~1800 OK)
    //   ELEMENTOS: cuerpo escalonado + techo empinado | 2 agujas caladas octogonales
    //     + aguja finisima | linterna central | 8 crockets | 4 contrafuertes con
    //     pinaculo | gablete apuntado | roseton recesado + oculo | portal ojival.
    //   detailStartOut = 498 (i tras cuerpo + cores de agujas + linterna).
    // ================================================================================
}
