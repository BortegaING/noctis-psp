#pragma once
// ============================================================================
// PROJECT NOCTIS -- LA GRAN CATEDRAL (pieza central del distrito, landmark unico)
// ----------------------------------------------------------------------------
// NO es una caja texturizada: es una MEGA-CATEDRAL gotica con ALMA e identidad.
// Escala BLAME! + oscuridad Notre-Dame/Colonia/Yharnam, pero ORIGINAL. Lo que la
// vende es la SILUETA:
//   - WEST-FRONT (fachada oeste) mirando a +Z (la plaza): DOS torres macizas a
//     izq/der que suben toda la altura, cada una rematada en aguja + pinaculos.
//   - Entre las torres: HASTIAL central con ROSETON recesado (disco facetado) y
//     PORTAL de triple arco ojival en la base.
//   - Hacia -Z: NAVE larga y alta con techo a dos aguas MUY empinado (2 vertientes
//     addQuadT que se juntan en el caballete).
//   - AGUJA DEL CRUCERO (fleche): el punto MAS ALTO, delgada, sobre un cimborrio.
//   - CONTRAFUERTES VOLADORES a lo largo de ambos flancos de la nave (pilar
//     exterior + arco addQuadT que baja del muro alto al pilar) -> la firma gotica.
//   - PINACULOS (piramides finas) en las torres y en los pilares de los arbotantes.
//
// Se pinta con la textura de fachada en GU_TFX_MODULATE: la PIEDRA se aclara aca
// con brighten(baseColor,2.7f); techos/agujas son pizarra casi-negra calida
// heightHaze(RGBA(30,28,26,255),y). Todo se envuelve en fadeToVoid(col,dist) para
// que la distancia lo funda en la bruma. Determinista (sin rand), sin heap, C++17.
//
// LOD: *detailStartOut = i queda fijado tras la MASA NUCLEO (2 torres + fachada +
// nave + techo). Lo fino (fleche del crucero + arbotantes + pinaculos) va DESPUES,
// para que el detalle pueda descartarse a la distancia.
// ============================================================================
static void buildCathedralGrand(TexVertex* buf, int& i, float cx, float cz,
                                float w, float d, float h, unsigned int baseColor, float dist,
                                int* detailStartOut) {
    // ---- PALETA -------------------------------------------------------------
    // piedra: legible pero tenebrosa (brighten 2.7 sobre baseColor + tinte calido
    // para no romper el look del distrito); pizarra: casi-negra calida (agujas y
    // techos); vidriera: ambar TENUE. Todo horneado con la niebla de distancia.
    const unsigned int stone = fadeToVoid(warmTint(brighten(baseColor, 2.7f)), dist);
    const unsigned int slate = fadeToVoid(RGBA(30, 28, 26, 255), dist);
    const unsigned int win   = RGBA(150, 118, 66, 255);   // ambar tenue (vidriera)
    const unsigned int rose  = RGBA(176, 120, 60, 255);   // roseton un pelo mas vivo

    // ---- MARCO DEL FOOTPRINT ------------------------------------------------
    const float hw = w * 0.5f, hd = d * 0.5f;
    const float frontZ = cz + hd;                 // cara +Z: da a la plaza (west-front)
    const float backZ  = cz - hd;                 // fondo -Z: donde se alarga la nave

    // ======================================================================
    // (A) MASA NUCLEO -- siempre presente (silueta base)
    // ======================================================================

    // ---- (A1) DOS TORRES DEL FRENTE (+Z) ------------------------------------
    // escalonadas: fuste base -> cuerpo -> campanario -> aguja. Suben casi toda
    // la altura. Una aguja un pelo mas alta que la otra -> ASIMETRIA con alma.
    const float tw   = w * 0.30f;                 // lado de la torre (maciza)
    const float toX  = hw - tw * 0.5f;            // torres pegadas a las esquinas del frente
    const float toZ  = frontZ - tw * 0.5f;        // su cara +Z coincide con el frente
    const float tb1  = h * 0.42f;                 // fuste base
    const float tb2  = h * 0.30f;                 // cuerpo (mas angosto)
    const float belH = tw * 0.85f;               // campanario
    for (int s = -1; s <= 1; s += 2) {
        const float x = cx + s * toX;
        const float spireH = h * 0.20f * (s > 0 ? 1.06f : 0.94f);   // asimetria de agujas
        const float bodyTop = tb1 + tb2 + belH;
        addSolidBoxT(buf, i, x, 0.0f,      toZ, tw,        tw,        tb1,  heightHaze(stone, tb1 * 0.5f));               // 30 fuste
        addSolidBoxT(buf, i, x, tb1,       toZ, tw * 0.9f, tw * 0.9f, tb2,  heightHaze(stone, tb1 + tb2 * 0.5f));        // 30 cuerpo
        addSolidBoxT(buf, i, x, tb1 + tb2, toZ, tw * 0.82f,tw * 0.82f,belH, heightHaze(brighten(stone, 1.06f), bodyTop));// 30 campanario
        addPyramidT (buf, i, x, bodyTop,   toZ, tw * 0.70f,tw * 0.70f,spireH, heightHaze(brighten(slate, 1.10f), bodyTop)); // 12 aguja
        // lumbreras del campanario (a g_win, buffer aparte)
        addWinRow(x, toZ, tw * 0.82f, tw * 0.82f, tb1 + tb2 + belH * 0.5f, 0, brighten(win, 1.20f));
    }

    // ---- (A2) HASTIAL CENTRAL (west-front) + ROSETON + TRIPLE PORTAL ---------
    const float facW = w * 0.42f;                 // ancho del pano central (topa las torres)
    const float facD = w * 0.08f;                 // espesor del muro-fachada
    const float facZ = frontZ - facD * 0.5f;      // su cara +Z casi al ras del frente
    const float facH = h * 0.52f;                 // muro hasta el arranque del hastial
    const float faceZ = frontZ + 0.15f;           // plano donde se pintan roseton/portal (proud)
    // muro del hastial
    addSolidBoxT(buf, i, cx, 0.0f, facZ, facW, facD, facH, heightHaze(stone, facH * 0.5f));            // 30 muro
    // hastial: piramide ancha y baja (dos aguas del frente) + finial (cruz-aguja)
    addPyramidT(buf, i, cx, facH, facZ, facW, facD, h * 0.15f, heightHaze(slate, facH));               // 12 hastial
    addPyramidT(buf, i, cx, facH + h * 0.15f, facZ, facW * 0.10f, facD, h * 0.05f,
                heightHaze(brighten(slate, 1.14f), facH + h * 0.15f));                                 // 12 finial

    // ROSETON: marco proud + disco oscuro recesado + 4 facetas ambar (disco facetado)
    const float roseR = facW * 0.22f;             // radio del roseton
    const float roseY = facH * 0.62f;             // altura del centro
    const unsigned int frame = heightHaze(brighten(stone, 1.28f), roseY);
    const unsigned int dark  = RGBA(9, 9, 13, 255);
    addSolidBoxT(buf, i, cx, roseY - roseR * 1.10f, faceZ - 0.30f,
                 roseR * 2.20f, facD * 0.5f, roseR * 2.20f, frame);                                    // 30 marco cuadrado
    addSolidBoxT(buf, i, cx, roseY - roseR * 0.85f, faceZ - facD * 0.45f,
                 roseR * 1.70f, facD * 0.30f, roseR * 1.70f, dark);                                    // 30 vano recesado oscuro
    {   // 4 facetas triangulares (addQuadT como triangulo: 4o vertice = 3o) -> rosa de
        // vidrio. Winding volteado para que la cara FRONTAL mire a +Z (la plaza): el
        // producto (b-a)x(c-a) debe apuntar hacia ADENTRO (-Z), igual que las cajas.
        const float zc = faceZ;
        const float U = roseY + roseR, D = roseY - roseR;                 // arriba/abajo
        const float L = cx - roseR,    R = cx + roseR;                    // izq/der
        addQuadT(buf, i, cx, roseY, zc,  cx, U, zc,  R, roseY, zc,  R, roseY, zc, 0,0,1,1, brighten(rose, 1.10f)); // 6
        addQuadT(buf, i, cx, roseY, zc,  L, roseY, zc,  cx, U, zc,  cx, U, zc, 0,0,1,1, rose);                     // 6
        addQuadT(buf, i, cx, roseY, zc,  cx, D, zc,  L, roseY, zc,  L, roseY, zc, 0,0,1,1, brighten(rose, 0.86f)); // 6
        addQuadT(buf, i, cx, roseY, zc,  R, roseY, zc,  cx, D, zc,  cx, D, zc, 0,0,1,1, brighten(rose, 0.96f)); // 6
    }

    // TRIPLE PORTAL ojival en la base (3 vanos oscuros + arco apuntado c/u)
    float portalH = h * 0.11f; if (portalH > 24.0f) portalH = 24.0f;
    const float vw = facW * 0.22f;                                       // ancho de cada vano
    for (int p = -1; p <= 1; ++p) {
        const float px = cx + (float)p * facW * 0.30f;
        addSolidBoxT(buf, i, px, 0.0f, faceZ - 0.20f, vw, facD * 0.4f, portalH, dark);                 // 30 vano oscuro
        addPyramidT (buf, i, px, portalH, faceZ - 0.24f, vw * 1.14f, facD * 0.4f, vw * 0.95f, frame);  // 12 arco apuntado
    }
    // cornisa/string-course horizontal sobre los portales (banda gotica)
    addSolidBoxT(buf, i, cx, portalH * 1.12f, facZ, facW * 1.02f, facD * 1.1f, portalH * 0.10f,
                 heightHaze(brighten(stone, 0.92f), portalH));                                          // 30 cornisa
    // NARTEX: porche bajo proyectado hacia la plaza frente al portal central
    const float porchD = w * 0.10f;
    addSolidBoxT(buf, i, cx, 0.0f, frontZ + porchD * 0.5f, facW * 0.50f, porchD, portalH * 1.05f,
                 heightHaze(stone, portalH * 0.5f));                                                    // 30 nartex

    // ---- (A3) NAVE LARGA (-Z) + TECHO A DOS AGUAS EMPINADO ------------------
    const float naveW    = w * 0.52f;
    const float naveFZ   = facZ - facD * 0.5f;                // arranca detras del hastial
    const float naveBZ   = backZ;
    const float naveZ    = (naveFZ + naveBZ) * 0.5f;
    const float naveDep  = naveFZ - naveBZ;
    const float naveH    = h * 0.60f;
    const float roofRise = h * 0.16f;
    addSolidBoxT(buf, i, cx, 0.0f, naveZ, naveW, naveDep, naveH, heightHaze(stone, naveH * 0.5f));      // 30 nave
    addSolidBoxT(buf, i, cx, naveH, naveZ, naveW * 1.06f, naveDep * 1.02f, naveH * 0.02f + 0.6f,
                 heightHaze(brighten(stone, 0.9f), naveH));                                             // 30 alero/cornisa
    {   // dos vertientes que se juntan en el caballete (x=cx, y=naveH+roofRise)
        const float ex = naveW * 0.5f, rY = naveH + roofRise;
        const unsigned int rs = heightHaze(slate, naveH + roofRise * 0.5f);
        // vertiente -X : alero(-X) sube al caballete
        addQuadT(buf, i, cx - ex, naveH, naveFZ,  cx - ex, naveH, naveBZ,
                 cx, rY, naveBZ,  cx, rY, naveFZ, 0,0, naveDep / TILE, roofRise / TILE, rs);            // 6
        // vertiente +X
        addQuadT(buf, i, cx, rY, naveFZ,  cx, rY, naveBZ,
                 cx + ex, naveH, naveBZ,  cx + ex, naveH, naveFZ, 0,0, naveDep / TILE, roofRise / TILE,
                 brighten(rs, 1.08f));                                                                  // 6
        // hastial trasero (triangulo: cierra el caballete atras; addQuadT c==d)
        addQuadT(buf, i, cx - ex, naveH, naveBZ,  cx + ex, naveH, naveBZ,
                 cx, rY, naveBZ,  cx, rY, naveBZ, 0,0,1,1, heightHaze(stone, naveH));                   // 6
    }

    // >>> FIN DE LA MASA NUCLEO: marca de LOD (2 torres + fachada + nave + techo)
    if (detailStartOut) *detailStartOut = i;

    // ======================================================================
    // (B) DETALLE FINO -- descartar a la distancia (LOD)
    // ======================================================================

    // ---- (B1) AGUJA DEL CRUCERO (fleche): el punto MAS ALTO -----------------
    // cimborrio (linterna) sobre el caballete + aguja larga y fina + 4 espiguetas.
    const float ridgeY  = naveH + roofRise;
    const float crossZ  = naveZ + naveDep * 0.18f;            // hacia el frente (donde iria el crucero)
    const float lanW    = naveW * 0.20f;
    const float lanH    = h * 0.045f;
    addSolidBoxT(buf, i, cx, ridgeY, crossZ, lanW, lanW, lanH,
                 heightHaze(brighten(stone, 1.04f), ridgeY));                                           // 30 linterna
    addPyramidT (buf, i, cx, ridgeY + lanH, crossZ, lanW * 0.72f, lanW * 0.72f, h * 0.28f,
                 heightHaze(brighten(slate, 1.16f), ridgeY + lanH + h * 0.14f));                        // 12 fleche (mas alta)
    for (int c = 0; c < 4; ++c) {                                                                       // 4 espiguetas
        const float ex = (c & 1) ? lanW * 0.5f : -lanW * 0.5f;
        const float ez = (c & 2) ? lanW * 0.5f : -lanW * 0.5f;
        addPyramidT(buf, i, cx + ex, ridgeY + lanH * 0.4f, crossZ + ez, lanW * 0.22f, lanW * 0.22f,
                    h * 0.06f, heightHaze(brighten(slate, 1.12f), ridgeY));                             // 12 x4 = 48
    }

    // ---- (B2) CONTRAFUERTES VOLADORES -- LA FIRMA GOTICA --------------------
    // 4 por flanco (+X / -X): pilar exterior + PINACULO + arco volador (addQuadT
    // que baja desde alto en el muro de la nave hasta la cima del pilar).
    const float pierX = naveW * 0.5f + w * 0.14f;             // pilar fuera de la nave, dentro del footprint
    const float pierW = w * 0.06f, pierDp = d * 0.07f;
    const float pierH = naveH * 0.62f;
    const float wallY = naveH * 0.86f;                        // punto alto del muro donde nace el arco
    for (int s = -1; s <= 1; s += 2) {
        const float sx = cx + s * pierX;
        const float wx = cx + s * (naveW * 0.5f);             // pie del arco en el muro de la nave
        for (int b = 0; b < 4; ++b) {
            const float bz = naveBZ + naveDep * (0.16f + 0.22f * (float)b);   // repartidos por la nave
            addSolidBoxT(buf, i, sx, 0.0f, bz, pierW, pierDp, pierH,
                         heightHaze(brighten(stone, 0.9f), pierH * 0.5f));                              // 30 pilar
            addPyramidT (buf, i, sx, pierH, bz, pierW, pierDp, h * 0.10f,
                         heightHaze(brighten(slate, 1.10f), pierH));                                    // 12 pinaculo del pilar
            // ARCO VOLADOR: rampa del muro alto (wx,wallY) a la cima del pilar (sx,pierH)
            const float zA = bz - pierDp * 0.5f, zB = bz + pierDp * 0.5f;
            const unsigned int ac = heightHaze(brighten(stone, 1.02f), (wallY + pierH) * 0.5f);
            addQuadT(buf, i, wx, wallY, zA,  sx, pierH, zA,  sx, pierH, zB,  wx, wallY, zB,
                     0,0, pierX / TILE, 0.4f, ac);                                                      // 6 arco (rampa)
        }
    }

    // ---- (B3) PINACULOS en las CIMAS de las TORRES -------------------------
    // 4 por torre (una en cada esquina del campanario). addPinnacle = caja+aguja.
    const float pinBase = tb1 + tb2 + belH;                  // cima del campanario (arranque de la aguja)
    const float pex = tw * 0.42f;
    const unsigned int pinCol = heightHaze(stone, pinBase);
    for (int s = -1; s <= 1; s += 2) {
        const float x = cx + s * toX;
        addPinnacle(buf, i, x - pex, pinBase, toZ - pex, tw * 0.16f, h * 0.10f, pinCol);               // 42
        addPinnacle(buf, i, x + pex, pinBase, toZ - pex, tw * 0.16f, h * 0.10f, pinCol);               // 42
        addPinnacle(buf, i, x - pex, pinBase, toZ + pex, tw * 0.16f, h * 0.10f, pinCol);               // 42
        addPinnacle(buf, i, x + pex, pinBase, toZ + pex, tw * 0.16f, h * 0.10f, pinCol);               // 42
    }

    // ---- (B4) GALERIAS OJIVALES (ventanas -> buffer g_win, no cuenta en buf) -
    // filas de lancetas en la nave (clerestorio) + roseton iluminado, mas densas
    // arriba. El winPick interno apaga ~70% -> claroscuro, no una grilla plana.
    int rows = (int)(naveH / 6.5f); if (rows > 10) rows = 10; if (rows < 3) rows = 3;
    for (int r = 0; r < rows; ++r) {
        const float f = (float)(r + 1) / (float)(rows + 1);
        float y = (f * f) * naveH; if (y < 3.0f) y = 3.0f;   // sesgo cuadratico: mas juntas arriba
        addWinRow(cx, naveZ, naveW, naveDep, y, 2, win);     // flanco +X
        addWinRow(cx, naveZ, naveW, naveDep, y, 3, win);     // flanco -X
    }
    addWinRow(cx, facZ, facW, facD, roseY, 0, brighten(rose, 1.25f));   // brillo del roseton (cara +Z)

    // ========================================================================
    // NOTA DE CONSTRUCCION (conteo exacto, box=30 / piramide=12 / quad=6):
    //   SILUETA que la vende: 2 torres del frente con aguja+4 pinaculos c/u,
    //   hastial con roseton facetado + triple portal ojival + nartex, nave larga
    //   con techo a dos aguas empinado, FLECHE del crucero (el punto mas alto) y
    //   8 arbotantes (arco volador addQuadT) con pinaculo -> lectura gotica clara.
    //
    //   MASA NUCLEO (hasta *detailStartOut):
    //     torres   2*(30+30+30+12)               = 204
    //     fachada  muro30 +hastial12 +finial12 +marco30 +vano30 +4facetas24
    //              +3*(vano30+arco12)=126 +cornisa30 +nartex30            = 324
    //     nave     caja30 +alero30 +2vertientes12 +hastial-atras6         = 78
    //     ----------------------------------------------------- NUCLEO   = 606
    //     >>> *detailStartOut = i queda AQUI (606 verts en el bloque).
    //
    //   DETALLE FINO (LOD, tras *detailStartOut):
    //     crucero   linterna30 +fleche12 +4espiguetas48                  = 90
    //     arbotantes 8*(pilar30 +pinaculo12 +arco6)=8*48                 = 384
    //     pinaculos torres 8*addPinnacle(42)                             = 336
    //     ----------------------------------------------------- DETALLE  = 810
    //
    //   TOTAL en buf = 606 + 810 = 1416 verts  (<= 2200, margen ~784).
    //   Las ventanas (addWinRow) van al buffer g_win aparte y NO cuentan aca.
    // ========================================================================
}
