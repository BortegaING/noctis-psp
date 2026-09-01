#pragma once
// PROJECT NOCTIS: generador de UN edificio gotico (silueta Bloodborne/MediEvil): cuerpo de piedra escalonado, techo de pizarra empinado, campanario descentrado con aguja, torretas de esquina y contrafuertes.
static void buildGothicBldg(TexVertex *buf, int &i, float cx, float cz,
                            float w, float d, float h, unsigned int baseColor, float dist,
                            int *detailStartOut) {
    // ---- Paleta: piedra fria + PIZARRA oscura azulada para el techo, ambar tenue en ventanas ----
    const unsigned int stone = fadeToVoid(brighten(baseColor, 3.0f), dist);   // legible (se ve el gotico) manteniendo mood oscuro
    const unsigned int slate = fadeToVoid(RGBA(48, 50, 70, 255), dist);   // pizarra oscura azulada (techo)
    const unsigned int win   = RGBA(150, 118, 66, 255);                    // ventana ojival ambar TENUE

    const bool cathedral = (h > 120.0f);   // landmark: mas agujas / mas alto
    const bool ruin      = (h < 40.0f);    // ruina: sin campanario, techo bajo y roto (asimetrico)

    // ---- CUERPO: 2 cajas, la superior mas angosta -> bodyTop ~ h*0.70 ----
    const float bodyTop = h * 0.70f;
    const float h1 = bodyTop * 0.58f;                       // planta baja (ancho pleno)
    const float h2 = bodyTop - h1;                          // cuerpo superior (retranqueado)
    const float w2 = w * 0.86f, d2 = d * 0.86f;
    addSolidBoxT(buf, i, cx, 0.0f, cz, w,  d,  h1, heightHaze(stone, h1 * 0.5f));              // 30
    addSolidBoxT(buf, i, cx, h1,   cz, w2, d2, h2, heightHaze(stone, h1 + h2 * 0.5f));         // 30

    // ---- TECHO EMPINADO de pizarra (la clave del gotico): alero + aguja piramidal ALTA ----
    const float eaveTop = bodyTop + h * 0.03f;
    // alero: anillo fino que vuela sobre el cuerpo y arranca el techo
    addSolidBoxT(buf, i, cx, bodyTop, cz, w2 * 1.10f, d2 * 1.10f, h * 0.03f,
                 heightHaze(brighten(slate, 0.9f), bodyTop));                                  // 30
    // techo principal: piramide ALTA y OSCURA -> ocupa ~40% de la silueta
    const float roofH = ruin ? (h * 0.15f) : (h * 0.46f);
    addPyramidT(buf, i, cx, eaveTop, cz, w2, d2, roofH, heightHaze(slate, bodyTop));           // 12
    // segunda piramide interior (mas fina y alta) -> aguja escalonada de pizarra
    if (!ruin)
        addPyramidT(buf, i, cx, eaveTop + roofH * 0.30f, cz, w2 * 0.60f, d2 * 0.60f,
                    roofH * 0.85f, heightHaze(brighten(slate, 1.12f), bodyTop));               // 12

    // ---- CAMPANARIO / AGUJA CENTRAL: torre delgada DESCENTRADA que perfora el techo ----
    if (!ruin) {
        const float bx = cx + w * 0.15f;                    // fuera de eje -> silueta asimetrica
        const float bz = cz - d * 0.11f;
        const float bw = w * 0.30f;
        const float belfryTop = h * (cathedral ? 1.32f : 1.04f);
        addSolidBoxT(buf, i, bx, bodyTop * 0.5f, bz, bw, bw, belfryTop - bodyTop * 0.5f,
                     heightHaze(stone, belfryTop * 0.5f));                                     // 30  fuste
        addSolidBoxT(buf, i, bx, belfryTop, bz, bw * 1.16f, bw * 1.16f, bw * 0.7f,
                     heightHaze(brighten(stone, 1.10f), belfryTop));                           // 30  belfry
        addPyramidT(buf, i, bx, belfryTop + bw * 0.7f, bz, bw * 1.16f, bw * 1.16f,
                    h * (cathedral ? 0.55f : 0.42f), heightHaze(slate, belfryTop));            // 12  aguja
        addWinRow(bx, bz, bw, bw, belfryTop + bw * 0.35f, 0, brighten(win, 1.15f));            // vano campanario
        addWinRow(bx, bz, bw, bw, belfryTop + bw * 0.35f, 2, brighten(win, 1.15f));

        // catedral: 2 agujas-aguja extra (needles) flanqueando el campanario
        if (cathedral) {
            addPyramidT(buf, i, cx - w * 0.30f, eaveTop, cz + d * 0.06f,
                        w * 0.14f, d * 0.14f, roofH * 1.05f, heightHaze(brighten(slate, 1.1f), bodyTop)); // 12
            addPyramidT(buf, i, cx + w * 0.02f, eaveTop, cz + d * 0.30f,
                        w * 0.13f, d * 0.13f, roofH * 0.90f, heightHaze(brighten(slate, 1.1f), bodyTop)); // 12
        }
    }

    // ---- TORRETAS / PINACULOS de esquina (cajita fina + aguja chica), ALTURAS VARIADAS ----
    const float tx = w * 0.5f - w * 0.05f;
    const float tz = d * 0.5f - d * 0.05f;
    const float tw = w * 0.13f;
    const float th = bodyTop * (ruin ? 0.42f : 0.55f);
    const unsigned int tc = heightHaze(stone, bodyTop);
    // NW, NE, SW siempre; SE solo si NO es ruina (ruina = una torreta caida -> asimetria)
    const float turH[4]  = { th * 1.15f, th * 0.80f, th * 0.95f, th * 1.25f };
    const float turX[4]  = { cx - tx, cx + tx, cx - tx, cx + tx };
    const float turZ[4]  = { cz - tz, cz - tz, cz + tz, cz + tz };
    const int   nTur     = ruin ? 3 : 4;
    for (int t = 0; t < nTur; ++t) {
        addSolidBoxT(buf, i, turX[t], bodyTop, turZ[t], tw, tw, turH[t], tc);                  // 30
        addPyramidT(buf, i, turX[t], bodyTop + turH[t], turZ[t], tw, tw, tw * 1.9f,
                    brighten(tc, 1.15f));                                                       // 12
    }

    // ---- CONTRAFUERTES / cornisas / pilastras / gargolas (marca LOD JUSTO antes) ----
    if (detailStartOut) *detailStartOut = i;
    addTowerDetail(buf, i, cx, cz, w, d, bodyTop, stone);   // ~510-654 verts (culleable por LOD)

    // ---- VENTANAS OJIVALES en las 4 caras (mas densas hacia arriba = nave de catedral) ----
    int rows = (int)(bodyTop / 5.0f); if (rows > 12) rows = 12; if (rows < 2) rows = 2;
    for (int r = 0; r < rows; ++r) {
        // sesgo cuadratico: filas mas juntas en el tercio superior
        float f = (float)(r + 1) / (float)(rows + 1);
        float y = (f * f) * bodyTop;
        if (y < 2.5f) y = 2.5f;
        addWinRow(cx, cz, w, d, y, 0, win);
        addWinRow(cx, cz, w, d, y, 1, win);
        addWinRow(cx, cz, w, d, y, 2, win);
        addWinRow(cx, cz, w, d, y, 3, win);
    }
    // banda alta luminosa (rosetones/lucernas) bajo el alero -> remate de catedral
    const float yTop = bodyTop * 0.88f;
    addWinRow(cx, cz, w, d, yTop, 0, brighten(win, 1.20f));
    addWinRow(cx, cz, w, d, yTop, 1, brighten(win, 1.20f));
}
