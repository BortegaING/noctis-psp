#pragma once
// ============================================================================
// PROJECT NOCTIS - agent_detail.h
// Detalle arquitectonico / industrial para torres goticas (estilo BLAME! +
// gotico oscuro). Rompe el aspecto de "cajas de Roblox" agregando cornisas,
// molduras, pilastras/nervaduras, sillares de esquina (quoins), remates,
// tuberias/conductos y elementos que conectan torres (puentes y arcos ojivales).
//
// Header AUTOCONTENIDO: NO #include. Depende SOLO de simbolos ya definidos en
// main.cpp ANTES del #include de este archivo:
//     struct TexVertex { float u,v; unsigned int color; float x,y,z; };
//     #define RGBA(r,g,b,a) ...
//     static unsigned int brighten(unsigned int c, float f);
//     static void addSolidBoxT(buf,&i, cx,baseY,cz, w,d,h, col);   // 30 vert
//     static void addPyramidT (buf,&i, cx,baseY,cz, w,d, apexH, col); // 12 vert
//     static void addQuadT    (buf,&i, ...);                       //  6 vert
//
// Convenciones (verificadas): baseY = base (piso) de la pieza; la caja va de
// baseY a baseY+h; la piramide apunta a baseY+apexH. w = eje X, d = eje Z,
// h/apexH = eje Y. Todo ASCII, sin acentos. Solo geometria via los primitivos.
// ============================================================================

// -------- helper local (sin <cmath>) --------
static inline float nd_absf(float v) { return v < 0.0f ? -v : v; }

// ----------------------------------------------------------------------------
// 1) addTowerDetail  ->  ~510 vert base / ~654 max (torres altas)  (budget 700)
//    base: 2 cornisas + 4 pilastras + 4 quoins + 2 tuberias + plinto/socle +
//          4 pinaculos de esquina + aguja central + 2 gargolas.
//    h>=6: 2 nichos ciegos ojivales (caras +Z/-Z).
//    h>=12: 2 contrafuertes escalonados (caras +X/-X) -> lectura de catedral.
//    Agrega relieve a una torre de cuerpo (cx,cz), base y=0, tamano w x d x h.
// ----------------------------------------------------------------------------
static void addTowerDetail(TexVertex *buf, int &i,
                           float cx, float cz, float w, float d, float h,
                           unsigned int stone)
{
    const float hw = w * 0.5f;
    const float hd = d * 0.5f;
    const float pr = 0.30f;                    // saliente de nervaduras
    const unsigned int cLight = brighten(stone, 1.12f);
    const unsigned int cMid   = brighten(stone, 1.02f);
    const unsigned int cDark  = brighten(stone, 0.90f);

    // --- CORNISAS / MOLDURAS horizontales (cajas finas mas anchas) ---
    float corH = h * 0.06f; if (corH < 0.4f) corH = 0.4f;
    const float over = 0.35f;                  // vuelo de la cornisa
    // remate / cornisa en la cima del cuerpo
    addSolidBoxT(buf, i, cx, h - corH, cz, w + over * 2.0f, d + over * 2.0f, corH, cLight);
    // moldura intermedia (banda que corta la altura)
    addSolidBoxT(buf, i, cx, h * 0.55f, cz, w + over * 1.4f, d + over * 1.4f, corH * 0.8f, cMid);

    // --- PILASTRAS verticales (una centrada por cara, hasta bajo la cornisa) ---
    float pilW = w * 0.16f; if (pilW < 0.4f) pilW = 0.4f;   // ancho tangencial en X
    float pilD = d * 0.16f; if (pilD < 0.4f) pilD = 0.4f;   // ancho tangencial en Z
    const float pilH = h - corH;
    addSolidBoxT(buf, i, cx, 0.0f, cz + hd + pr * 0.5f, pilW, pr, pilH, cLight); // cara +Z
    addSolidBoxT(buf, i, cx, 0.0f, cz - hd - pr * 0.5f, pilW, pr, pilH, cLight); // cara -Z
    addSolidBoxT(buf, i, cx + hw + pr * 0.5f, 0.0f, cz, pr, pilD, pilH, cLight); // cara +X
    addSolidBoxT(buf, i, cx - hw - pr * 0.5f, 0.0f, cz, pr, pilD, pilH, cLight); // cara -X

    // --- QUOINS de esquina (sillares) en las 4 aristas verticales, alternados ---
    const float qS   = 0.55f;                  // tamano del sillar
    const float qOff = 0.10f;                  // saliente sobre la arista
    const float sx[4] = {  hw,  hw, -hw, -hw };
    const float sz[4] = {  hd, -hd,  hd, -hd };
    for (int c = 0; c < 4; ++c) {
        const unsigned int qc = (c & 1) ? cLight : cDark;   // color alternado
        addSolidBoxT(buf, i,
                     cx + sx[c] + (sx[c] > 0.0f ? qOff : -qOff),
                     0.0f,
                     cz + sz[c] + (sz[c] > 0.0f ? qOff : -qOff),
                     qS, qS, h - corH, qc);
    }

    // --- TUBERIAS / conductos verticales (industrial) pegados a la cara -X ---
    const float tD   = 0.35f;                  // seccion del conducto
    const float tOut = tD * 0.4f;              // cuanto sobresale de la cara
    addSolidBoxT(buf, i, cx - hw - tOut, 0.0f, cz + hd * 0.45f, tD, tD, h * 0.95f, cDark);
    addSolidBoxT(buf, i, cx - hw - tOut, 0.0f, cz - hd * 0.45f, tD, tD, h * 0.80f, cDark);

    // --- BASE / SOCLE: plinto escalonado que ancla la torre al suelo (+30) ---
    float socH = corH * 1.2f;
    addSolidBoxT(buf, i, cx, 0.0f, cz, w + over * 2.4f, d + over * 2.4f, socH, cDark);

    // --- CORONACION: pinaculos de esquina + aguja central (silueta de catedral) ---
    // La cornisa de cima termina en y = h; los remates nacen desde ahi (+60).
    const float pinW = qS * 1.5f;
    const float pinH = 0.9f + h * 0.10f;               // mas afilado en torres altas
    for (int c = 0; c < 4; ++c) {
        addPyramidT(buf, i,
                    cx + sx[c] + (sx[c] > 0.0f ? qOff : -qOff),
                    h,
                    cz + sz[c] + (sz[c] > 0.0f ? qOff : -qOff),
                    pinW, pinW, pinH, cLight);         // 4 pinaculos = 48
    }
    // aguja / flecha central: crece con la altura de la torre
    addPyramidT(buf, i, cx, h, cz, w * 0.5f, d * 0.5f, 1.4f + h * 0.14f,
                brighten(stone, 1.16f));               // 12

    // --- GARGOLAS / canos vertedores bajo la cornisa (relieve amenazante, +60) ---
    const unsigned int cDeep = brighten(stone, 0.82f); // sombra profunda gotica
    const float gY = h - corH * 1.6f;
    addSolidBoxT(buf, i, cx + hw * 0.4f, gY, cz + hd + 0.45f, 0.35f, 0.9f, 0.35f, cDeep); // saliente +Z
    addSolidBoxT(buf, i, cx - hw * 0.4f, gY, cz - hd - 0.45f, 0.35f, 0.9f, 0.35f, cDeep); // saliente -Z

    // --- NICHOS CIEGOS OJIVALES en caras +Z / -Z: panel oscuro + capucha en punta ---
    if (h >= 6.0f) {                                    // +84
        const float niW = w * 0.30f;                   // ancho del nicho (X)
        const float niH = h * 0.34f;                   // alto del panel
        const float niY = h * 0.30f;                   // arranque del panel
        const float niHoodH = w * 0.18f;               // altura de la punta ojival
        addSolidBoxT(buf, i, cx, niY, cz + hd + 0.07f, niW, 0.14f, niH, cDeep);
        addPyramidT (buf, i, cx, niY + niH, cz + hd + 0.07f, niW * 1.1f, 0.30f, niHoodH, cMid);
        addSolidBoxT(buf, i, cx, niY, cz - hd - 0.07f, niW, 0.14f, niH, cDeep);
        addPyramidT (buf, i, cx, niY + niH, cz - hd - 0.07f, niW * 1.1f, 0.30f, niHoodH, cMid);
    }

    // --- CONTRAFUERTES ESCALONADOS al pie de pilastras +X / -X (solo torres altas) ---
    if (h >= 12.0f) {                                   // +60
        const float btH = h * 0.24f;                   // alto del pie del contrafuerte
        const float btW = pilD * 1.9f;                 // extension tangencial (Z)
        addSolidBoxT(buf, i, cx + hw + pr * 0.9f, 0.0f, cz, pr * 2.2f, btW, btH, cMid); // +X
        addSolidBoxT(buf, i, cx - hw - pr * 0.9f, 0.0f, cz, pr * 2.2f, btW, btH, cMid); // -X
    }
}

// ----------------------------------------------------------------------------
// 2) addBridge  ->  ~390 vert  (13 cajas)  (budget 450)
//    Pasarela gotica entre (x0,y0,z0) y (x1,y1,z1). Se orienta al eje horizontal
//    dominante (X o Z). Tablero + barandal + postes (ritmo de balaustres) +
//    coronamiento/coping sobre cada baranda + 2 mensulas de apoyo bajo el tablero.
// ----------------------------------------------------------------------------
static void addBridge(TexVertex *buf, int &i,
                      float x0, float y0, float z0,
                      float x1, float y1, float z1,
                      unsigned int stone)
{
    const float cx = (x0 + x1) * 0.5f;
    const float cy = (y0 + y1) * 0.5f;
    const float cz = (z0 + z1) * 0.5f;
    const float dxA = nd_absf(x1 - x0);
    const float dzA = nd_absf(z1 - z0);

    const float deckW = 1.4f;                  // ancho de la pasarela
    const float deckH = 0.35f;                 // espesor del tablero
    const float railW = 0.15f;                 // grosor del barandal
    const float railH = 0.70f;                 // alto del barandal
    const float postS = 0.22f;                 // seccion de poste
    const float postH = 0.85f;
    const unsigned int cDeck = brighten(stone, 0.98f);
    const unsigned int cRail = brighten(stone, 1.10f);
    const unsigned int cPost = brighten(stone, 0.90f);
    const int N = 3;                           // postes por lado

    if (dxA >= dzA) {
        // puente a lo largo de X
        const float len = dxA;
        addSolidBoxT(buf, i, cx, cy, cz, len, deckW, deckH, cDeck);        // tablero
        const float railTop = cy + deckH;
        const float zL = cz + deckW * 0.5f - railW * 0.5f;
        const float zR = cz - deckW * 0.5f + railW * 0.5f;
        addSolidBoxT(buf, i, cx, railTop, zL, len, railW, railH, cRail);   // baranda L
        addSolidBoxT(buf, i, cx, railTop, zR, len, railW, railH, cRail);   // baranda R
        const float xa = (x0 < x1) ? x0 : x1;
        for (int k = 0; k < N; ++k) {
            const float t  = (N == 1) ? 0.5f : (float)k / (float)(N - 1);
            const float px = xa + t * len;
            addSolidBoxT(buf, i, px, cy, zL, postS, postS, postH, cPost);
            addSolidBoxT(buf, i, px, cy, zR, postS, postS, postH, cPost);
        }
        // coronamiento (coping) sobre cada baranda -> remata el ritmo de balaustres
        const float capTop = railTop + railH;
        addSolidBoxT(buf, i, cx, capTop, zL, len, railW * 1.8f, deckH * 0.7f, cRail);
        addSolidBoxT(buf, i, cx, capTop, zR, len, railW * 1.8f, deckH * 0.7f, cRail);
        // mensulas / brackets de apoyo colgando bajo el tablero en los extremos
        const float brY = cy - deckH * 0.5f - 0.5f;
        addSolidBoxT(buf, i, xa,       brY, cz, postS * 1.6f, deckW * 0.9f, 0.6f, cPost);
        addSolidBoxT(buf, i, xa + len, brY, cz, postS * 1.6f, deckW * 0.9f, 0.6f, cPost);
    } else {
        // puente a lo largo de Z
        const float len = dzA;
        addSolidBoxT(buf, i, cx, cy, cz, deckW, len, deckH, cDeck);        // tablero
        const float railTop = cy + deckH;
        const float xL = cx + deckW * 0.5f - railW * 0.5f;
        const float xR = cx - deckW * 0.5f + railW * 0.5f;
        addSolidBoxT(buf, i, xL, railTop, cz, railW, len, railH, cRail);   // baranda L
        addSolidBoxT(buf, i, xR, railTop, cz, railW, len, railH, cRail);   // baranda R
        const float za = (z0 < z1) ? z0 : z1;
        for (int k = 0; k < N; ++k) {
            const float t  = (N == 1) ? 0.5f : (float)k / (float)(N - 1);
            const float pz = za + t * len;
            addSolidBoxT(buf, i, xL, cy, pz, postS, postS, postH, cPost);
            addSolidBoxT(buf, i, xR, cy, pz, postS, postS, postH, cPost);
        }
        // coronamiento (coping) sobre cada baranda -> remata el ritmo de balaustres
        const float capTop = railTop + railH;
        addSolidBoxT(buf, i, xL, capTop, cz, railW * 1.8f, len, deckH * 0.7f, cRail);
        addSolidBoxT(buf, i, xR, capTop, cz, railW * 1.8f, len, deckH * 0.7f, cRail);
        // mensulas / brackets de apoyo colgando bajo el tablero en los extremos
        const float brY = cy - deckH * 0.5f - 0.5f;
        addSolidBoxT(buf, i, cx, brY, za,       deckW * 0.9f, postS * 1.6f, 0.6f, cPost);
        addSolidBoxT(buf, i, cx, brY, za + len, deckW * 0.9f, postS * 1.6f, 0.6f, cPost);
    }
}

// ----------------------------------------------------------------------------
// 3) addArchSpan  ->  ~384 vert  (13 cajas + 2 piramides)  (budget 400)
//    Arco ojival (gotico apuntado) a la altura y, centrado en (cx,cz), luz w.
//    El vano abre a lo largo de X (arco en el plano X-Y). 2 pilares, dovelas
//    escalonadas (voussoirs) que suben al centro, piramide que cierra la punta,
//    2 impostas/capiteles en el arranque, clave (keystone) resaltada y traceria
//    del timpano (mainel central + sub-punta ojival).
// ----------------------------------------------------------------------------
static void addArchSpan(TexVertex *buf, int &i,
                        float cx, float cz, float w, float y,
                        unsigned int stone)
{
    const float half = w * 0.5f;
    const float pilW = 0.55f;                  // seccion del pilar en X
    const float pilD = 0.55f;                  // seccion (Z), comun a todo el arco
    float pilH = w * 0.8f; if (pilH < 2.0f) pilH = 2.0f;
    const unsigned int cPil  = brighten(stone, 1.00f);
    const unsigned int cVou  = brighten(stone, 1.08f);
    const unsigned int cApex = brighten(stone, 1.14f);

    // pilares izquierdo / derecho
    addSolidBoxT(buf, i, cx - half, y, cz, pilW, pilD, pilH, cPil);
    addSolidBoxT(buf, i, cx + half, y, cz, pilW, pilD, pilH, cPil);

    // dovelas escalonadas subiendo hacia el centro (arco apuntado)
    const int   N = 3;                         // pasos por lado
    const float apexHalf = pilW * 0.5f;        // separacion final al eje
    const float dx = (half - apexHalf) / (float)N;   // avance al centro por paso
    const float dy = (w * 0.35f) / (float)N;         // subida por paso
    const float baseY = y + pilH;
    const float blk = pilW;                    // tamano del bloque de dovela
    for (int k = 1; k <= N; ++k) {
        const float off = half - dx * (float)k;
        const float by  = baseY + dy * (float)(k - 1);
        const float bh  = dy + blk * 0.4f;
        addSolidBoxT(buf, i, cx - off, by, cz, blk, pilD, bh, cVou); // izq
        addSolidBoxT(buf, i, cx + off, by, cz, blk, pilD, bh, cVou); // der
    }

    // punta ojival: piramide que cierra el vertice
    const float apexBaseY = baseY + dy * (float)N;
    const float apexW = apexHalf * 2.0f + blk;
    addPyramidT(buf, i, cx, apexBaseY, cz, apexW, pilD, dy * 1.6f + 0.6f, cApex);

    // --- IMPOSTAS / capiteles: bloque de arranque sobre cada pilar (linea de salmer, +60) ---
    const unsigned int cImp = brighten(stone, 1.04f);
    const float impH = pilW * 0.6f;
    addSolidBoxT(buf, i, cx - half, baseY - impH * 0.5f, cz, pilW * 1.7f, pilD * 1.6f, impH, cImp);
    addSolidBoxT(buf, i, cx + half, baseY - impH * 0.5f, cz, pilW * 1.7f, pilD * 1.6f, impH, cImp);

    // --- CLAVE (keystone): dovela central resaltada justo bajo la punta (+30) ---
    const float ksH = blk * 0.9f;
    addSolidBoxT(buf, i, cx, apexBaseY - ksH * 0.5f, cz, blk * 1.25f, pilD * 1.15f, ksH,
                 brighten(stone, 1.18f));

    // --- TRACERIA del timpano: mainel central + sub-punta ojival (+42) ---
    const float traH = apexBaseY - baseY;
    if (traH > 0.4f) {
        addSolidBoxT(buf, i, cx, baseY, cz, pilW * 0.5f, pilD * 0.7f, traH, cVou);        // mainel
        addPyramidT (buf, i, cx, baseY + traH, cz, pilW * 1.4f, pilD * 0.7f,
                     dy * 1.1f + 0.4f, cImp);                                              // sub-punta
    }
}
