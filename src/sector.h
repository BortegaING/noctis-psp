#pragma once
// ===================== SECTOR CERRADO (planta de cruz gotica) =====================
// Reemplaza la plataforma gigante + monolitos lejanos. Es un recinto CHICO y CERRADO:
// suelo, TECHO y muros exteriores. Adentro, 4 MASAS SEPARADAS (no pegadas) dejan
// PASILLOS en cruz + un pasillo perimetral. Las paredes OCLUYEN: en un pasillo solo se
// pinta ese pasillo -> gran ahorro de fill (el cuello real de la PSP).
//
//   planta (vista desde arriba)          corte
//   +--------------------------+         techo  y=60 ------------------
//   |  ####    pasillo   ####  |         masas  y=30 ####      ####
//   |  ####      |       ####  |         suelo  y=0  ------------------
//   |  ----------+---------    |
//   |  ####      |       ####  |   masas 36x36 separadas por pasillos de 18
//   +--------------------------+   pasillo perimetral de 9
//
static const float SEC_HALF   = 54.0f;  // media anchura util (muro interior)
static const float SEC_CEIL   = 60.0f;  // TECHO = doble de la altura de una masa (30)
static const float SEC_MASS_H = 30.0f;  // altura de las masas ("edificios")
static const float SEC_M0     =  9.0f;  // borde interior de las masas -> pasillo en cruz de 18
static const float SEC_M1     = 45.0f;  // borde exterior -> pasillo perimetral de 9
// --- arcada: columnas EXENTAS (separadas de la masa, se puede pasar por detras) ---
static const float SEC_COL_OFF  =  7.0f;  // a 2u de la cara de la masa: no quedan pegadas
static const float SEC_COL_STEP = 16.0f;  // separacion entre columnas
static const float SEC_COL_W    =  3.3f;  // lado del fuste (colision)
static const float SEC_COL_H    = 18.6f;  // alto total con capitel
static const int   SEC_COL_SEG  =  5;     // el fuste va en TRAMOS: un quad de 16u de alto
                                          // cruza el plano de la camara al pasar cerca y el
                                          // hardware lo DESCARTA entero -> la columna "se borra".
// --- CORONACION de la masa (cuerpo + cornisa + remate): se dibuja Y se choca ---
// El techo REAL de una masa es la CORNISA (31.6), no el cuerpo (30): el que sube
// a una masa se para ahi. Las medidas viven aca para que el dibujo (buildSectorWalls)
// y la colision (buildSectorCollision) no puedan separarse.
static const float SEC_CORN_H   =  1.6f;                     // alto de la cornisa (30 -> 31.6)
static const float SEC_CORN_S   =  1.06f;                    // ensanche: vuela 1.08 al pasillo
static const float SEC_MASS_TOP = SEC_MASS_H + SEC_CORN_H;   // 31.6: superficie PISABLE
static const float SEC_CAP_S    =  0.55f;                    // lado del remate = 36 * 0.55 = 19.8
static const float SEC_CAP_H    =  5.0f;                     // alto del remate
static const float SEC_CAP_TOP  = SEC_MASS_TOP + SEC_CAP_H;  // 36.6: techo del remate
// --- CONTRAFUERTES de las dos caras interiores (3 por cara, 24 en total) ---
static const float SEC_BUT_DP   =  1.2f;                     // separacion del centro respecto a la cara
static const float SEC_BUT_W    =  2.4f;                     // vuelo hacia el pasillo
static const float SEC_BUT_D    =  3.0f;                     // ancho sobre la cara
static const float SEC_BUT_O    =  0.30f;                    // paso = 36 * 0.30 = 10.8
static const float SEC_BUT_H    = SEC_MASS_H * 0.82f;        // 24.6: alto

// --- colision: llena g_city con los volumenes solidos (cajas alineadas a ejes) ---
// Se reusa el sistema de colision que ya existe; los techos de las masas son pisables.
static void buildSectorCollision() {
    int n = 0;
    const unsigned int c = RGBA(64, 70, 82, 255);
    const float mc = (SEC_M0 + SEC_M1) * 0.5f;      // centro de masa = 27
    const float ms = (SEC_M1 - SEC_M0);             // lado = 36
    for (int q = 0; q < 4; ++q) {                   // 4 masas SEPARADAS entre si
        float sx = (q & 1) ? -1.0f : 1.0f, sz = (q & 2) ? -1.0f : 1.0f;
        // CUERPO + CORNISA en una sola caja: sube hasta 31.6 (antes 30), que es donde
        // esta el techo que se ve. Con 30 el jugador se paraba con los pies metidos
        // 1.6 dentro de la cornisa. Se mantiene el lado del CUERPO (36): darle el lado
        // de la cornisa (38.16) cobraria su vuelo de 1.08 en TODA la altura y estrecharia
        // el pasillo perimetral de 9 a 7.92; el alero queda sin caja a proposito.
        g_city[n++] = { mc * sx, mc * sz, ms, ms, SEC_MASS_TOP, c };
        // REMATE (19.8 x 19.8, de 31.6 a 36.6): sin el, el bloque del centro del techo
        // es un fantasma que se cruza caminando. CityBldg siempre arranca en y=0, pero
        // su huella esta METIDA dentro de la masa -> abajo no roba ni un milimetro de
        // pasillo, y arriba deja un anillo pisable de 7u alrededor.
        g_city[n++] = { mc * sx, mc * sz, ms * SEC_CAP_S, ms * SEC_CAP_S, SEC_CAP_TOP, c };
    }
    const float wt = 4.0f, wc = SEC_HALF + wt * 0.5f;
    g_city[n++] = {  0.0f,  wc, 2.0f * (SEC_HALF + wt), wt, SEC_CEIL, c };  // muro +Z
    g_city[n++] = {  0.0f, -wc, 2.0f * (SEC_HALF + wt), wt, SEC_CEIL, c };  // muro -Z
    g_city[n++] = {  wc,  0.0f, wt, 2.0f * (SEC_HALF + wt), SEC_CEIL, c };  // muro +X
    g_city[n++] = { -wc,  0.0f, wt, 2.0f * (SEC_HALF + wt), SEC_CEIL, c };  // muro -X
    // COLUMNAS de la arcada: antes eran solo adorno y se atravesaban. Ahora son solidas
    // (y sus capiteles quedan pisables, util para la escalada con gravedad).
    for (int a = 0; a < 2; ++a)
        for (int s = -1; s <= 1; s += 2)
            for (int k = -2; k <= 2; ++k) {
                float t = (float)k * SEC_COL_STEP;
                float x = (a == 0) ? (SEC_COL_OFF * s) : t;
                float z = (a == 0) ? t : (SEC_COL_OFF * s);
                g_city[n++] = { x, z, SEC_COL_W, SEC_COL_W, SEC_COL_H, c };
            }
    // CONTRAFUERTES: pilares de piedra de 24.6 de alto que vuelan 2.4 al pasillo
    // (ocupan de 6.6 a 9.0 en la coordenada perpendicular). Se dibujaban desde el
    // principio pero no tenian caja: se atravesaban caminando, y la camara tambien.
    // Entran los 16 de |o| = 10.8. El del CENTRO de cada cara (o = 0) NO: cae dentro
    // del portal ciego que ya cierra arch_detail.h (|o| <= 3.9, vuelo 2.3). OJO: esa
    // caja solo llega a y=15, asi que el contrafuerte central sigue sin colision de
    // 15 a 24.6; subirla es cosa de arch_detail.h, no de este archivo.
    for (int q = 0; q < 4; ++q) {
        float sx = (q & 1) ? -1.0f : 1.0f, sz = (q & 2) ? -1.0f : 1.0f;
        float x = mc * sx, z = mc * sz;
        for (int k = -1; k <= 1; k += 2) {          // los dos de fuera; el central lo cubre el portal
            float o = (float)k * ms * SEC_BUT_O;
            g_city[n++] = { x - sx * (ms * 0.5f + SEC_BUT_DP), z + o,
                            SEC_BUT_W, SEC_BUT_D, SEC_BUT_H, c };
            g_city[n++] = { x + o, z - sz * (ms * 0.5f + SEC_BUT_DP),
                            SEC_BUT_D, SEC_BUT_W, SEC_BUT_H, c };
        }
    }
    g_cityCount = n;   // 4 masas + 4 remates + 4 muros + 20 columnas + 16 contrafuertes = 48
                       // (+35 de arch_detail.h = 83 de las 256 de g_city)
}

// --- SUELO y TECHO (teselados finos: la camara nunca cruza un triangulo grande) ---
static void buildSectorFloorCeil(TexVertex *buf, int &i) {
    const float H = SEC_HALF + 4.0f;
    const int   N = 20;                             // celdas de ~5.8u
    const float C = (2.0f * H) / (float)N;
    const float uv = 1.0f / 10.0f;
    const unsigned int fcol = RGBA(212, 194, 172, 255);   // suelo claro (losas)
    const unsigned int ccol = RGBA(104,  92,  76, 255);   // techo mas oscuro (boveda)
    for (int gz = 0; gz < N; ++gz) {
        float z0 = -H + C * gz, z1 = z0 + C;
        for (int gx = 0; gx < N; ++gx) {
            float x0 = -H + C * gx, x1 = x0 + C;
            addQuadT(buf, i, x0,0,z0, x1,0,z0, x1,0,z1, x0,0,z1,
                     x0*uv,z0*uv, x1*uv,z1*uv, fcol);                       // suelo
            addQuadT(buf, i, x0,SEC_CEIL,z1, x1,SEC_CEIL,z1, x1,SEC_CEIL,z0, x0,SEC_CEIL,z0,
                     x0*uv,z0*uv, x1*uv,z1*uv, ccol);                       // TECHO (limite)
        }
    }
}

// --- arco OJIVAL escalonado (5 cajas): el gesto gotico, barato ---
// axis 0 = el arco cruza en X (pilares a +-span), axis 1 = cruza en Z.
// SIN COLISION A PROPOSITO: el arco vuela de y=20.7 a 27.3 SOBRE el hueco entre dos
// columnas, y por ese hueco se pasa caminando. Como CityBldg no tiene baseY (siempre
// arranca en y=0), darle caja a un tramo del arco no seria un arco: seria un TAPON
// macizo del suelo a 27 que cerraria el paso entre columna y columna. Las "jambas"
// del arco son las propias columnas, que ya tienen su caja (SEC_COL_W x SEC_COL_H).
static void addArchT(TexVertex *buf, int &i, float cx, float cz, float span,
                     float baseY, float rise, float th, int axis, unsigned int col) {
    const float st = span / 3.0f;                   // 3 tramos por lado
    for (int s = 0; s < 3; ++s) {
        float off = span - st * (float)s * 0.5f - st * 0.5f;   // se acerca al centro
        float y   = baseY + rise * (0.30f + 0.26f * (float)s); // y sube
        float h   = rise * 0.30f;
        float w   = (axis == 0) ? st : th, d = (axis == 0) ? th : st;
        addSolidBoxT(buf, i, cx + ((axis == 0) ? off : 0.0f), y, cz + ((axis == 0) ? 0.0f : off), w, d, h, col);
        addSolidBoxT(buf, i, cx - ((axis == 0) ? off : 0.0f), y, cz - ((axis == 0) ? 0.0f : off), w, d, h, col);
    }
    addSolidBoxT(buf, i, cx, baseY + rise * 0.90f, cz, th * 1.3f, th * 1.3f, rise * 0.34f, brighten(col, 1.12f)); // clave
}

// --- estructura: masas separadas + muros + arcada de columnas en los pasillos ---
static void buildSectorWalls(TexVertex *buf, int &i) {
    const unsigned int stone = brighten(RGBA(69, 65, 58, 255), 2.1f);
    const unsigned int dark  = brighten(RGBA(46, 41, 36, 255), 2.0f);
    const float mc = (SEC_M0 + SEC_M1) * 0.5f, ms = (SEC_M1 - SEC_M0);

    // 4 MASAS separadas: cuerpo + cornisa + remate escalonado (silueta gotica, no un ladrillo)
    for (int q = 0; q < 4; ++q) {
        float sx = (q & 1) ? -1.0f : 1.0f, sz = (q & 2) ? -1.0f : 1.0f;
        float x = mc * sx, z = mc * sz;
        // las medidas salen de las constantes SEC_CORN_*/SEC_CAP_*/SEC_BUT_* de arriba:
        // buildSectorCollision usa LAS MISMAS, asi lo que se ve y lo que se choca no
        // se pueden separar (era justo el bug: cornisa, remate y contrafuertes sin caja).
        addSolidBoxT(buf, i, x, 0.0f, z, ms, ms, SEC_MASS_H, stone);                        // cuerpo
        addSolidBoxT(buf, i, x, SEC_MASS_H, z, ms * SEC_CORN_S, ms * SEC_CORN_S,
                     SEC_CORN_H, dark);                                                     // cornisa
        addSolidBoxT(buf, i, x, SEC_MASS_TOP, z, ms * SEC_CAP_S, ms * SEC_CAP_S,
                     SEC_CAP_H, stone);                                                     // remate
        // CONTRAFUERTES en las dos caras que dan a los pasillos (separados entre si)
        for (int k = -1; k <= 1; ++k) {
            float o = (float)k * ms * SEC_BUT_O;
            addSolidBoxT(buf, i, x - sx * (ms * 0.5f + SEC_BUT_DP), 0.0f, z + o,
                         SEC_BUT_W, SEC_BUT_D, SEC_BUT_H, dark);
            addSolidBoxT(buf, i, x + o, 0.0f, z - sz * (ms * 0.5f + SEC_BUT_DP),
                         SEC_BUT_D, SEC_BUT_W, SEC_BUT_H, dark);
        }
    }

    // MUROS exteriores hasta el techo (cierran el sector)
    // MUROS exteriores CON VENTANALES: antifecho solido abajo, muro macizo arriba y una
    // hilera de pilares en medio -> huecos altos por los que se ve el telon de agujas en
    // niebla (la postal de la referencia). La COLISION sigue siendo el muro entero, asi
    // que se ve hacia afuera pero no se puede salir ni caer.
    const float wt = 4.0f, wc = SEC_HALF + wt * 0.5f, wl = 2.0f * (SEC_HALF + wt);
    const float opY0 = 7.0f, opY1 = 27.0f;       // franja abierta (altura de la vista)
    const float pierW = 5.0f, bay = 19.0f;       // pilar y paso entre ventanales
    const int   nPier = (int)(wl / bay) + 1;
    for (int wI = 0; wI < 4; ++wI) {
        const bool alongX = (wI < 2);            // 0,1 = muros +Z/-Z ; 2,3 = muros +X/-X
        const float sgn   = (wI & 1) ? -1.0f : 1.0f;
        const float cx    = alongX ? 0.0f : wc * sgn, cz = alongX ? wc * sgn : 0.0f;
        const float bw    = alongX ? wl : wt,        bd = alongX ? wt : wl;
        addSolidBoxT(buf, i, cx, 0.0f,  cz, bw, bd, opY0, stone);                       // antifecho
        addSolidBoxT(buf, i, cx, opY1,  cz, bw, bd, SEC_CEIL - opY1, stone);            // muro alto
        for (int k = 0; k < nPier; ++k) {        // pilares entre ventanal y ventanal
            float t = -wl * 0.5f + bay * (float)k;
            if (t < -wl * 0.5f || t > wl * 0.5f) continue;
            addSolidBoxT(buf, i, alongX ? t : cx, opY0, alongX ? cz : t,
                         alongX ? pierW : wt, alongX ? wt : pierW, opY1 - opY0, dark);
        }
    }

    // ARCADA de los pasillos en cruz: columnas SEPARADAS + arcos ojivales entre ellas.
    // Van en los bordes del pasillo (|x|=9 y |z|=9), dejando el centro libre para caminar.
    const float colH = 16.0f, colR = 1.5f;
    const float segH = colH / (float)SEC_COL_SEG;    // tramos cortos (ver SEC_COL_SEG)
    for (int a = 0; a < 2; ++a) {                    // a=0 pasillo en Z, a=1 pasillo en X
        for (int s = -1; s <= 1; s += 2) {           // los dos lados del pasillo
            for (int k = -2; k <= 2; ++k) {          // 5 columnas por lado
                float t = (float)k * SEC_COL_STEP;
                float x = (a == 0) ? (SEC_COL_OFF * s) : t;
                float z = (a == 0) ? t : (SEC_COL_OFF * s);
                addSolidBoxT(buf, i, x, 0.0f, z, colR * 2.2f, colR * 2.2f, 1.2f, dark);    // basa
                for (int g = 0; g < SEC_COL_SEG; ++g)   // FUSTE EN TRAMOS (no un bloque de 16)
                    addSolidBoxT(buf, i, x, 1.2f + segH * (float)g, z,
                                 colR * 1.6f, colR * 1.6f, segH, (g & 1) ? stone : brighten(stone, 0.94f));
                addSolidBoxT(buf, i, x, 1.2f + colH, z, colR * 2.4f, colR * 2.4f, 1.4f, dark); // capitel
                if (k < 2) {   // arco ojival hacia la columna siguiente
                    float ax = (a == 0) ? x : t + SEC_COL_STEP * 0.5f;
                    float az = (a == 0) ? t + SEC_COL_STEP * 0.5f : z;
                    addArchT(buf, i, ax, az, SEC_COL_STEP * 0.5f, 1.2f + colH + 1.4f, 7.0f,
                             colR * 1.5f, (a == 0) ? 1 : 0, stone);
                }
            }
        }
    }
}
