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

// --- colision: llena g_city con los volumenes solidos (cajas alineadas a ejes) ---
// Se reusa el sistema de colision que ya existe; los techos de las masas son pisables.
static void buildSectorCollision() {
    int n = 0;
    const unsigned int c = RGBA(64, 70, 82, 255);
    const float mc = (SEC_M0 + SEC_M1) * 0.5f;      // centro de masa = 27
    const float ms = (SEC_M1 - SEC_M0);             // lado = 36
    for (int q = 0; q < 4; ++q) {                   // 4 masas SEPARADAS entre si
        float sx = (q & 1) ? -1.0f : 1.0f, sz = (q & 2) ? -1.0f : 1.0f;
        g_city[n++] = { mc * sx, mc * sz, ms, ms, SEC_MASS_H, c };
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
    g_cityCount = n;                                   // 4 masas + 4 muros + 20 columnas = 28
}

// --- SUELO y TECHO (teselados finos: la camara nunca cruza un triangulo grande) ---
static void buildSectorFloorCeil(TexVertex *buf, int &i) {
    const float H = SEC_HALF + 4.0f;
    const int   N = 20;                             // celdas de ~5.8u
    const float C = (2.0f * H) / (float)N;
    const float uv = 1.0f / 10.0f;
    const unsigned int fcol = RGBA(240, 232, 220, 255);   // suelo claro (losas)
    const unsigned int ccol = RGBA(120, 126, 140, 255);   // techo mas oscuro (boveda)
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
    const unsigned int stone = brighten(RGBA(64, 70, 82, 255), 2.1f);
    const unsigned int dark  = brighten(RGBA(46, 50, 60, 255), 2.0f);
    const float mc = (SEC_M0 + SEC_M1) * 0.5f, ms = (SEC_M1 - SEC_M0);

    // 4 MASAS separadas: cuerpo + cornisa + remate escalonado (silueta gotica, no un ladrillo)
    for (int q = 0; q < 4; ++q) {
        float sx = (q & 1) ? -1.0f : 1.0f, sz = (q & 2) ? -1.0f : 1.0f;
        float x = mc * sx, z = mc * sz;
        addSolidBoxT(buf, i, x, 0.0f, z, ms, ms, SEC_MASS_H, stone);                       // cuerpo
        addSolidBoxT(buf, i, x, SEC_MASS_H, z, ms * 1.06f, ms * 1.06f, 1.6f, dark);        // cornisa
        addSolidBoxT(buf, i, x, SEC_MASS_H + 1.6f, z, ms * 0.55f, ms * 0.55f, 5.0f, stone);// remate
        // CONTRAFUERTES en las dos caras que dan a los pasillos (separados entre si)
        for (int k = -1; k <= 1; ++k) {
            float o = (float)k * ms * 0.30f;
            addSolidBoxT(buf, i, x - sx * (ms * 0.5f + 1.2f), 0.0f, z + o, 2.4f, 3.0f, SEC_MASS_H * 0.82f, dark);
            addSolidBoxT(buf, i, x + o, 0.0f, z - sz * (ms * 0.5f + 1.2f), 3.0f, 2.4f, SEC_MASS_H * 0.82f, dark);
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
