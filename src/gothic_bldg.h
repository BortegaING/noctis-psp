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

    // ---- CUERPO en CRUZ LATINA + muros en TALUD (deja de ser una caja recta) ----
    const float bodyTop = h * 0.70f;
    const float w2 = w * 0.86f, d2 = d * 0.86f;                 // cuerpo superior (lo usan el alero/techo de abajo)

    // (1) NAVE en TALUD: 3 tiers que se ANGOSTAN con la altura -> perfil escalonado, no rectangular
    const float h1 = bodyTop * 0.42f;                           // planta baja ancho pleno (la usa el portal)
    const float hm = bodyTop * 0.34f;                           // cuerpo medio
    const float ht = bodyTop - h1 - hm;                         // cuerpo alto (retranqueo) -> remata en w2,d2
    addSolidBoxT(buf, i, cx, 0.0f,    cz, w,       d,       h1, heightHaze(stone, h1 * 0.5f));               // 30
    addSolidBoxT(buf, i, cx, h1,      cz, w*0.93f, d*0.94f, hm, heightHaze(stone, h1 + hm * 0.5f));          // 30
    addSolidBoxT(buf, i, cx, h1 + hm, cz, w2,      d2,      ht, heightHaze(stone, h1 + hm + ht * 0.5f));     // 30

    // (2) TRANSEPTO / crucero: brazo perpendicular que sobresale a los lados -> planta en CRUZ LATINA (no un bloque)
    addSolidBoxT(buf, i, cx, 0.0f, cz - d * 0.05f, w * 1.26f, d * 0.44f, bodyTop * 0.40f,
                 heightHaze(stone, bodyTop * 0.20f));                                                        // 30

    // (3) ABSIDE poligonal: 3 gajos en ABANICO atras (-Z) formando un remate curvo (sin matrices, solo offset ~40deg)
    const float apZ0 = cz - d * 0.44f;                          // arranca del muro trasero
    const float apR  = d * 0.34f;
    const float apW  = w * 0.34f, apD = d * 0.30f, apH = bodyTop * 0.50f;
    const unsigned int apC = heightHaze(stone, apH * 0.5f);
    addSolidBoxT(buf, i, cx,               0.0f, apZ0 - apR,        apW,       apD, apH,        apC);        // 30 gajo centro
    addSolidBoxT(buf, i, cx - apR * 0.64f, 0.0f, apZ0 - apR*0.77f,  apW*0.82f, apD, apH*0.94f,  apC);        // 30 gajo izq (girado por posicion)
    addSolidBoxT(buf, i, cx + apR * 0.64f, 0.0f, apZ0 - apR*0.77f,  apW*0.82f, apD, apH*0.94f,  apC);        // 30 gajo der

    // (4) CONTRAFUERTES de esquina: 4 machones que SOBRESALEN en las esquinas (rompe la silueta cuadrada), alturas variadas
    const float cbx = w * 0.5f, cbz = d * 0.5f, cbw = w * 0.16f, cbd = d * 0.16f, cbh = bodyTop * 0.64f;
    const unsigned int cbC = heightHaze(stone, cbh * 0.5f);
    addSolidBoxT(buf, i, cx - cbx, 0.0f, cz - cbz, cbw, cbd, cbh,         cbC);                              // 30 NW
    addSolidBoxT(buf, i, cx + cbx, 0.0f, cz - cbz, cbw, cbd, cbh * 0.86f, cbC);                              // 30 NE
    addSolidBoxT(buf, i, cx - cbx, 0.0f, cz + cbz, cbw, cbd, cbh * 0.94f, cbC);                              // 30 SW
    addSolidBoxT(buf, i, cx + cbx, 0.0f, cz + cbz, cbw, cbd, cbh * 0.78f, cbC);                              // 30 SE

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

    // ---- FACHADA a nivel de SUELO (lo que se ve de cerca): PORTAL ojival + lancets ----
    {
        const float pw  = w * 0.34f;
        const float ph  = (h1 < 15.0f) ? (h1 * 0.75f) : 12.0f;
        const float pzf = cz + d * 0.5f;                          // cara que da a la plaza (+Z)
        const unsigned int frame = heightHaze(brighten(stone, 1.3f), ph * 0.5f);
        const unsigned int dark  = RGBA(8, 9, 14, 255);           // hueco oscuro (recesado)
        addSolidBoxT(buf, i, cx, 0.0f, pzf + 0.12f, pw * 1.32f, 0.5f, ph * 1.10f, frame);        // 30 jamba/marco
        addSolidBoxT(buf, i, cx, 0.0f, pzf + 0.32f, pw,         0.6f, ph,         dark);         // 30 vano de la puerta
        addPyramidT (buf, i, cx, ph,   pzf + 0.28f, pw * 1.18f, 0.6f, pw * 0.95f, frame);        // 12 ARCO apuntado
        addSolidBoxT(buf, i, cx - pw * 1.00f, ph * 0.20f, pzf + 0.16f, pw * 0.26f, 0.4f, ph * 1.15f, brighten(win, 0.85f)); // 30 lancet izq
        addSolidBoxT(buf, i, cx + pw * 1.00f, ph * 0.20f, pzf + 0.16f, pw * 0.26f, 0.4f, ph * 1.15f, brighten(win, 0.85f)); // 30 lancet der
    }

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
