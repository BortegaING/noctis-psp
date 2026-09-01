#pragma once
// PROJECT NOCTIS: catedral gotica ornamentada (Yharnam Grand Cathedral): nave alta escalonada, BOSQUE de agujas finas sobre el caballete, 2 torres gemelas, contrafuertes voladores, galerias ojivales, portal + roseton y techo empinado. Landmark de PRIMER PLANO (pocos en escena -> se permite detalle).
static void buildCathedral(TexVertex *buf, int &i, float cx, float cz,
                           float w, float d, float h, unsigned int baseColor, float dist,
                           int *detailStartOut) {
    // ---- PALETA: piedra gris-marron CALIDA (no azul); pizarra un poco mas oscura y calida; ventana ambar tenue ----
    const unsigned int stone = fadeToVoid(brighten(baseColor, 3.0f), dist);   // piedra legible, mood oscuro
    const unsigned int slate = fadeToVoid(RGBA(60, 48, 40, 255), dist);       // techo/pizarra oscuro CALIDO (marron), no azul
    const unsigned int win   = RGBA(150, 118, 66, 255);                       // vidriera ambar TENUE

    // ---- CUERPO: 3 cajas escalonadas -> seccion de basilica (naves laterales + clerigo central alto) ----
    const float bodyTop = h * 0.72f;
    const float h1 = bodyTop * 0.46f;                    // planta baja (ancho pleno)
    const float h2 = bodyTop * 0.30f;                    // naves laterales
    const float h3 = bodyTop - h1 - h2;                  // clerigo (nave central) que carga el techo
    const float w2 = w * 0.82f, d2 = d * 0.98f;
    const float w3 = w * 0.66f, d3 = d * 0.96f;
    const float shoulderY = h1 + h2;                     // hombro de las naves laterales
    addSolidBoxT(buf, i, cx, 0.0f,      cz, w,  d,  h1, heightHaze(stone, h1 * 0.5f));                 // 30
    addSolidBoxT(buf, i, cx, h1,        cz, w2, d2, h2, heightHaze(stone, h1 + h2 * 0.5f));           // 30
    addSolidBoxT(buf, i, cx, shoulderY, cz, w3, d3, h3, heightHaze(stone, shoulderY + h3 * 0.5f));    // 30

    // ---- TECHO EMPINADO de pizarra sobre el clerigo (alero volado + piramide larga + caballete interior) ----
    const float eaveH = h * 0.02f;
    const float roofH = h * 0.20f;                       // muy inclinado
    const float roofBase = bodyTop + eaveH;
    addSolidBoxT(buf, i, cx, bodyTop, cz, w3 * 1.12f, d3 * 1.06f, eaveH,
                 heightHaze(brighten(slate, 0.95f), bodyTop));                                        // 30 alero
    addPyramidT (buf, i, cx, roofBase, cz, w3, d3, roofH, heightHaze(slate, bodyTop));                // 12 techo
    addPyramidT (buf, i, cx, roofBase + roofH * 0.22f, cz, w3 * 0.58f, d3 * 0.94f, roofH * 0.72f,
                 heightHaze(brighten(slate, 1.08f), bodyTop));                                        // 12 caballete interior

    // ---- BOSQUE DE AGUJAS sobre el caballete (LA FIRMA): 8 agujas finas de alturas variadas, con jitter ASIMETRICO ----
    // majores: fuste (caja fina) + aguja (piramide). menores: solo aguja mas corta. Todas MUY delgadas.
    const float ridgeY = roofBase + roofH * 0.50f;       // nacen a media altura del techo (linea del caballete)
    for (int j = 0; j < 8; ++j) {
        const float t  = (j + 0.5f) / 8.0f;                          // 0..1 a lo largo de la nave (Z)
        const float nz = cz - d * 0.34f + t * d * 0.62f;             // repartidas por casi toda la nave
        const float nx = cx + ((j * 7) % 5 - 2) * (w * 0.028f);      // jitter lateral -> silueta erizada asimetrica
        const float needleH = h * (0.16f + 0.11f * ((j * 13) % 7) / 6.0f);   // ~0.16h..0.27h
        const float nw = w * 0.05f;                                  // fina como aguja
        if ((j & 1) == 0) {                                          // MAYOR: fuste + aguja
            addSolidBoxT(buf, i, nx, ridgeY, nz, nw, nw, needleH * 0.45f,
                         heightHaze(stone, ridgeY + needleH * 0.2f));                                 // 30
            addPyramidT (buf, i, nx, ridgeY + needleH * 0.45f, nz, nw, nw, needleH * 0.55f,
                         heightHaze(brighten(slate, 1.12f), ridgeY + needleH * 0.6f));                // 12
        } else {                                                     // MENOR: solo aguja
            addPyramidT (buf, i, nx, ridgeY, nz, nw * 0.8f, nw * 0.8f, needleH * 0.82f,
                         heightHaze(brighten(slate, 1.14f), ridgeY + needleH * 0.4f));                // 12
        }
    }
    // 4 agujas FLANCO sobre los hombros de las naves laterales -> mas bristle fuera del eje
    for (int k = 0; k < 4; ++k) {
        const float side = (k < 2) ? -1.0f : 1.0f;
        const float fx = cx + side * (w * 0.40f);
        const float fz = cz - d * 0.22f + (k % 2) * (d * 0.34f);
        const float fh = h * (0.13f + 0.06f * (k % 3));
        addPyramidT(buf, i, fx, shoulderY, fz, w * 0.045f, w * 0.045f, fh,
                    heightHaze(brighten(slate, 1.06f), shoulderY));                                   // 12
    }

    // ---- TORRES GEMELAS del frente (+Z, la que da a la plaza): fuste alto + campanario + aguja + 2 pinaculos ----
    const float tox = w * 0.34f;                          // separacion de las torres
    const float toz = cz + d * 0.40f;                     // adelantadas hacia la fachada
    const float tw  = w * 0.24f;
    const float towerBody = h * 0.86f;                    // sobresalen del cuerpo de la nave
    const float belfryH = tw * 0.9f;
    for (int s = -1; s <= 1; s += 2) {
        const float x = cx + s * tox;
        const float spireH = h * 0.34f * (s > 0 ? 1.0f : 1.14f);   // una aguja mas alta -> ASIMETRIA
        addSolidBoxT(buf, i, x, 0.0f, toz, tw, tw, towerBody,
                     heightHaze(stone, towerBody * 0.5f));                                            // 30 fuste
        addSolidBoxT(buf, i, x, towerBody, toz, tw * 1.14f, tw * 1.14f, belfryH,
                     heightHaze(brighten(stone, 1.08f), towerBody));                                  // 30 campanario
        addPyramidT (buf, i, x, towerBody + belfryH, toz, tw * 1.05f, tw * 1.05f, spireH,
                     heightHaze(brighten(slate, 1.12f), towerBody));                                  // 12 aguja principal
        addPyramidT (buf, i, x - tw * 0.5f, towerBody + belfryH * 0.2f, toz + tw * 0.5f,
                     tw * 0.28f, tw * 0.28f, spireH * 0.45f,
                     heightHaze(brighten(slate, 1.10f), towerBody));                                  // 12 pinaculo esq
        addPyramidT (buf, i, x + tw * 0.5f, towerBody + belfryH * 0.2f, toz + tw * 0.5f,
                     tw * 0.28f, tw * 0.28f, spireH * 0.50f,
                     heightHaze(brighten(slate, 1.10f), towerBody));                                  // 12 pinaculo esq
        addWinRow(x, toz, tw, tw, towerBody + belfryH * 0.5f, 0, brighten(win, 1.15f));   // lumbreras
        addWinRow(x, toz, tw, tw, towerBody + belfryH * 0.5f, 1, brighten(win, 1.15f));
    }

    // ---- CONTRAFUERTES VOLADORES (flying buttresses): pier exterior + pinaculo + puntal (fingido por posicion) ----
    // 2 por cada lado largo (+X / -X). El "arco volador" = caja fina horizontal del clerigo al pier.
    const float pierOff = w * 0.64f;                     // X del pier exterior (fuera del cuerpo)
    const float spanW   = pierOff - w3 * 0.5f;           // luz del arco (clerigo -> pier), positiva
    const float midOff  = (pierOff + w3 * 0.5f) * 0.5f;
    const float pierH   = bodyTop * 0.55f;
    for (int s = -1; s <= 1; s += 2) {
        for (int m = 0; m < 2; ++m) {
            const float bz = cz - d * 0.18f + m * (d * 0.30f);
            addSolidBoxT(buf, i, cx + s * pierOff, 0.0f, bz, w * 0.09f, d * 0.10f, pierH,
                         heightHaze(stone, pierH * 0.5f));                                            // 30 pier
            addPyramidT (buf, i, cx + s * pierOff, pierH, bz, w * 0.09f, d * 0.10f, h * 0.11f,
                         heightHaze(brighten(slate, 1.10f), pierH));                                  // 12 pinaculo del pier
            addSolidBoxT(buf, i, cx + s * midOff, shoulderY - h * 0.02f, bz, spanW, d * 0.06f, h * 0.03f,
                         heightHaze(brighten(stone, 1.06f), shoulderY));                              // 30 arco volador (puntal)
        }
    }

    // ---- PORTAL OJIVAL + ROSETON en la fachada +Z (bajo/entre las torres) ----
    {
        const float fz = cz + d * 0.5f;
        const float pw = w * 0.26f;
        float ph = h * 0.20f; if (ph > 16.0f) ph = 16.0f;
        const unsigned int frame = heightHaze(brighten(stone, 1.3f), ph * 0.5f);
        const unsigned int dark  = RGBA(8, 9, 14, 255);              // vano recesado oscuro
        addSolidBoxT(buf, i, cx, 0.0f, fz + 0.12f, pw * 1.34f, 0.5f, ph * 1.12f, frame);   // 30 marco/jamba
        addSolidBoxT(buf, i, cx, 0.0f, fz + 0.30f, pw, 0.6f, ph, dark);                    // 30 vano de la puerta
        addPyramidT (buf, i, cx, ph, fz + 0.26f, pw * 1.20f, 0.6f, pw * 1.00f, frame);     // 12 ARCO apuntado
        const float roseY = ph * 1.5f + h * 0.06f;
        addSolidBoxT(buf, i, cx, roseY - w * 0.10f, fz + 0.10f, w * 0.22f, 0.4f, w * 0.20f, frame); // 30 recuadro del roseton
        addWinRow(cx, cz, w, d, roseY, 0, brighten(win, 1.35f));    // roseton iluminado (marcado, cara +Z)
    }

    // ---- DETALLE PESADO (cornisas/pilastras/quoins/gargolas/contrafuertes/aguja) -> marca LOD JUSTO antes ----
    if (detailStartOut) *detailStartOut = i;
    addTowerDetail(buf, i, cx, cz, w, d, bodyTop, stone);   // ~600 verts (culleable por distancia)

    // ---- GALERIAS OJIVALES: filas densas de ventanas altas en las 4 caras, MAS DENSAS hacia arriba ----
    int rows = (int)(bodyTop / 4.0f); if (rows > 16) rows = 16; if (rows < 3) rows = 3;
    for (int r = 0; r < rows; ++r) {
        const float f = (float)(r + 1) / (float)(rows + 1);
        float y = (f * f) * bodyTop;                        // sesgo cuadratico -> mas juntas arriba
        if (y < 2.5f) y = 2.5f;
        addWinRow(cx, cz, w, d, y, 0, win);
        addWinRow(cx, cz, w, d, y, 1, win);
        addWinRow(cx, cz, w, d, y, 2, win);
        addWinRow(cx, cz, w, d, y, 3, win);
    }
    // banda alta luminosa del clerigo (triforio) bajo el alero
    const float yTri = bodyTop * 0.90f;
    addWinRow(cx, cz, w, d, yTri, 2, brighten(win, 1.20f));
    addWinRow(cx, cz, w, d, yTri, 3, brighten(win, 1.20f));
}
