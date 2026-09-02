#pragma once
// ============================================================================
//  CATEDRAL GOTICA (Bloodborne / Yharnam) a escala BLAME! -> se vende por SILUETA.
//  El detalle fino (ojivas, vitrales, encofrado) lo PINTA la textura de fachada;
//  aqui SOLO la masa gruesa que hace que la mole NO se lea como bloque de oficina:
//    - cuerpo ALTO y ESTRECHO en 3 gradas que RETRANQUEAN al subir (verticalidad)
//    - TECHO de PIZARRA EMPINADO en piramide (apex >= ~0.42*h) -> jamas plano
//    - AGUJA CENTRAL: piramide-aguja finisima que domina el skyline
//    - 4 PINACULOS finos en las esquinas del techo -> "bosque de agujas"
//    - CONTRAFUERTES: costillas verticales que salen de la cara hacia la plaza
//    - landmark (h>120): TORRES GEMELAS a los extremos, cada una con su aguja
//                        -> frente-oeste de catedral
//  Los 5 "edificios" son en verdad MUROS del anillo (E/O = losas 24x122 largas,
//  S = 90x24, torres del porton 40x24): son ALARGADOS. La silueta se adapta al
//  eje LARGO (nave) vs eje CORTO (grosor). El jugador ve las caras internas + el
//  perfil del techo contra el cielo -> ahi es donde se gana el "gotico".
//
//  PRESUPUESTO PSP (addSolidBoxT=30, addPyramidT=12):
//    normal = 3 cuerpos(90)+techo(12)+aguja(12)+4 pinaculos(48)+4 contraf.(120) = 282
//    big    = normal(282) + 2 torres[caja+aguja](84)                            = 366
//  Ambos bajo el tope (~300 / ~380). Sin rand, sin heap, C++17, firma intacta.
// ============================================================================
static void buildGothicBldg(TexVertex *buf, int &i, float cx, float cz,
                            float w, float d, float h, unsigned int baseColor, float dist,
                            int *detailStartOut) {
    // muro: la fachada la PINTA la textura (MODULATE). Se preserva el ×2.7 que este
    // archivo ya aplicaba (main.cpp pasa b.color CRUDO ~RGBA(58,55,50)): sin el
    // brighten los muros quedarian casi negros. Solo cambia la GEOMETRIA, no el tono.
    const unsigned int stone = fadeToVoid(brighten(baseColor, 2.7f), dist);
    const unsigned int slate = fadeToVoid(RGBA(30, 28, 26, 255), dist);   // pizarra casi negra (techo/agujas)
    const bool  big  = (h > 120.0f);                                      // landmark/catedral
    const bool  longX = (w >= d);                                         // eje LARGO (nave) = X? (E/O -> false)

    // ---- variacion determinista por edificio (hash de cx,cz,h; NADA de rand) ----
    unsigned int hsh = (unsigned int)((int)cx * 374761393) + (unsigned int)((int)cz * 668265263)
                     + (unsigned int)((int)h * (int)2654435761u);
    hsh ^= hsh >> 15; hsh *= 2246822519u; hsh ^= hsh >> 13; hsh *= 3266489917u; hsh ^= hsh >> 16;
    const float v0 = (float)(hsh & 0xFFu)        * (1.0f / 255.0f);
    const float v1 = (float)((hsh >> 8)  & 0xFFu) * (1.0f / 255.0f);
    const float v2 = (float)((hsh >> 16) & 0xFFu) * (1.0f / 255.0f);

    // ===================== CUERPO: 3 gradas que retranquean =====================
    const float bodyTop = h * (0.80f + 0.05f * v1);      // tope del cuerpo (deja aire para techo+aguja)
    const float h1 = bodyTop * 0.55f;                    // grada baja (nave)
    const float h2 = bodyTop * 0.30f;                    // grada media (clerestorio)
    const float h3 = bodyTop - h1 - h2;                  // grada alta (~0.15*bodyTop)
    const float s2 = 0.90f - 0.04f * v2;                 // retranqueo grada 2 (~0.86..0.90)
    const float s3 = 0.78f - 0.04f * v0;                 // retranqueo grada 3 (~0.74..0.78)

    addSolidBoxT(buf, i, cx, 0.0f,      cz, w,      d,      h1, heightHaze(stone, h1 * 0.5f));            // 30
    addSolidBoxT(buf, i, cx, h1,        cz, w * s2, d * s2, h2, heightHaze(stone, h1 + h2 * 0.5f));       // 30
    addSolidBoxT(buf, i, cx, h1 + h2,   cz, w * s3, d * s3, h3, heightHaze(stone, h1 + h2 + h3 * 0.5f));  // 30

    const float topW = w * s3, topD = d * s3;            // huella de la grada superior (base del techo)

    // ============ TECHO de PIZARRA EMPINADO (la senal gotica #1) ============
    const float roofH = h * (0.42f + 0.06f * v0);        // apex >= ~0.42*h -> techo agudo, NUNCA plano
    addPyramidT(buf, i, cx, bodyTop, cz, topW, topD, roofH, heightHaze(slate, bodyTop));                 // 12

    // ============ TORRES GEMELAS (frente-oeste; solo landmark 'big') ============
    // Van a los EXTREMOS del eje largo -> dos torres que enmarcan la nave.
    if (big) {
        const float twT  = (longX ? d : w);              // grosor del muro (eje corto)
        const float twS  = twT * 0.72f;                  // huella cuadrada de la torre
        const float twH  = h * (0.90f + 0.06f * v1);     // mas alta que el cuerpo
        const float twSp = h * (0.28f + 0.06f * v2);     // aguja propia de cada torre
        if (longX) {
            const float txL = cx - (w * 0.5f - twS * 0.5f), txR = cx + (w * 0.5f - twS * 0.5f);
            addSolidBoxT(buf, i, txL, 0.0f, cz, twS, twS, twH, heightHaze(stone, twH * 0.5f));           // 30
            addPyramidT (buf, i, txL, twH,  cz, twS, twS, twSp, heightHaze(slate, twH));                 // 12
            addSolidBoxT(buf, i, txR, 0.0f, cz, twS, twS, twH, heightHaze(stone, twH * 0.5f));           // 30
            addPyramidT (buf, i, txR, twH,  cz, twS, twS, twSp, heightHaze(slate, twH));                 // 12
        } else {
            const float tzL = cz - (d * 0.5f - twS * 0.5f), tzR = cz + (d * 0.5f - twS * 0.5f);
            addSolidBoxT(buf, i, cx, 0.0f, tzL, twS, twS, twH, heightHaze(stone, twH * 0.5f));           // 30
            addPyramidT (buf, i, cx, twH,  tzL, twS, twS, twSp, heightHaze(slate, twH));                 // 12
            addSolidBoxT(buf, i, cx, 0.0f, tzR, twS, twS, twH, heightHaze(stone, twH * 0.5f));           // 30
            addPyramidT (buf, i, cx, twH,  tzR, twS, twS, twSp, heightHaze(slate, twH));                 // 12
        }
    }

    // LOD: la MASA nucleo (cuerpo + techo + torres) queda ANTES; a distancia main.cpp
    // puede dibujar solo [sStart, sDetail) y saltar el bosque fino de agujas de abajo.
    // El techo va en el nucleo -> jamas hay tope plano, ni siquiera lejos.
    if (detailStartOut) *detailStartOut = i;

    // ==================== AGUJA CENTRAL: la firma del skyline ====================
    const float thin    = (topW < topD ? topW : topD);   // dimension corta de la cubierta
    const float spF     = thin * 0.30f;                  // pie finisimo (aguja)
    const float spBaseY = bodyTop + roofH * 0.30f;       // brota desde el techo
    const float spH     = h * (0.55f + 0.12f * v1);      // altisima: se eleva MUY por encima del techo
    addPyramidT(buf, i, cx, spBaseY, cz, spF, spF, spH, heightHaze(slate, spBaseY));                     // 12

    // ==================== 4 PINACULOS en las esquinas del techo ====================
    const float pinF = thin * 0.20f;                     // pie chico
    const float pinH = h * (0.26f + 0.06f * v2);         // altos y finos
    const float pcx  = topW * 0.5f - pinF * 0.6f;
    const float pcz  = topD * 0.5f - pinF * 0.6f;
    addPyramidT(buf, i, cx - pcx, bodyTop, cz - pcz, pinF, pinF, pinH, heightHaze(slate, bodyTop));      // 12
    addPyramidT(buf, i, cx + pcx, bodyTop, cz - pcz, pinF, pinF, pinH, heightHaze(slate, bodyTop));      // 12
    addPyramidT(buf, i, cx - pcx, bodyTop, cz + pcz, pinF, pinF, pinH, heightHaze(slate, bodyTop));      // 12
    addPyramidT(buf, i, cx + pcx, bodyTop, cz + pcz, pinF, pinF, pinH, heightHaze(slate, bodyTop));      // 12

    // ==================== 4 CONTRAFUERTES (costillas hacia la plaza) ====================
    // Salen de la cara CORTA que mira al origen (donde esta el jugador) -> maxima lectura.
    const float off4[4] = { -0.34f, -0.13f, 0.13f, 0.34f };
    const float btH   = h * (0.44f + 0.10f * v0);        // costilla alta (hasta media nave)
    const float btLen = (longX ? w : d) * 0.10f;         // fina a lo largo de la nave
    const float btPro = (longX ? d : w) * 0.30f;         // sobresale desde la cara corta
    const unsigned int btCol = brighten(stone, 0.9f);
    if (longX) {                                          // nave en X -> cara en ±Z
        const float sz = (cz > 0.0f) ? -1.0f : 1.0f;     // hacia el origen (plaza)
        const float bz = cz + sz * (d * 0.5f + btPro * 0.5f);
        for (int k = 0; k < 4; ++k)
            addSolidBoxT(buf, i, cx + w * off4[k], 0.0f, bz, btLen, btPro, btH, heightHaze(btCol, btH * 0.5f)); // 30 c/u
    } else {                                              // nave en Z -> cara en ±X
        const float sx = (cx > 0.0f) ? -1.0f : 1.0f;
        const float bx = cx + sx * (w * 0.5f + btPro * 0.5f);
        for (int k = 0; k < 4; ++k)
            addSolidBoxT(buf, i, bx, 0.0f, cz + d * off4[k], btPro, btLen, btH, heightHaze(btCol, btH * 0.5f)); // 30 c/u
    }

    // ---------------------------------------------------------------------------
    // SILUETA construida: cuerpo alto/estrecho en 3 gradas retranqueadas + techo de
    // pizarra empinado (apex ~0.42-0.48*h, jamas plano) + aguja-aguja central que
    // domina + 4 pinaculos de esquina (bosque de agujas) + 4 contrafuertes hacia la
    // plaza; los landmark suman 2 torres gemelas con aguja (frente-oeste).
    // VERTS EXACTOS: normal = 282 (90+12+12+48+120); big = 366 (+84 torres). <=300/380.
    // detailStartOut = i, fijado JUSTO tras la masa nucleo (cuerpo+techo[+torres]) y
    // ANTES del racimo fino (aguja central + pinaculos + contrafuertes) para el LOD.
    // ---------------------------------------------------------------------------
}
