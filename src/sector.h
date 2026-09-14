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
    g_cityCount = n;
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
    const float wt = 4.0f, wc = SEC_HALF + wt * 0.5f, wl = 2.0f * (SEC_HALF + wt);
    addSolidBoxT(buf, i,  0.0f, 0.0f,  wc, wl, wt, SEC_CEIL, stone);
    addSolidBoxT(buf, i,  0.0f, 0.0f, -wc, wl, wt, SEC_CEIL, stone);
    addSolidBoxT(buf, i,  wc,  0.0f,  0.0f, wt, wl, SEC_CEIL, stone);
    addSolidBoxT(buf, i, -wc,  0.0f,  0.0f, wt, wl, SEC_CEIL, stone);

    // ARCADA de los pasillos en cruz: columnas SEPARADAS + arcos ojivales entre ellas.
    // Van en los bordes del pasillo (|x|=9 y |z|=9), dejando el centro libre para caminar.
    const float colH = 16.0f, colR = 1.5f;
    for (int a = 0; a < 2; ++a) {                    // a=0 pasillo en Z, a=1 pasillo en X
        for (int s = -1; s <= 1; s += 2) {           // los dos lados del pasillo
            for (int k = -2; k <= 2; ++k) {          // 5 columnas por lado
                float t = (float)k * 16.0f;          // separadas 16u (no pegadas)
                float x = (a == 0) ? (SEC_M0 * s) : t;
                float z = (a == 0) ? t : (SEC_M0 * s);
                addSolidBoxT(buf, i, x, 0.0f,  z, colR * 2.2f, colR * 2.2f, 1.2f, dark);   // basa
                addSolidBoxT(buf, i, x, 1.2f,  z, colR * 1.6f, colR * 1.6f, colH, stone);  // fuste
                addSolidBoxT(buf, i, x, 1.2f + colH, z, colR * 2.4f, colR * 2.4f, 1.4f, dark); // capitel
                if (k < 2) {   // arco ojival hacia la columna siguiente
                    float ax = (a == 0) ? x : t + 8.0f, az = (a == 0) ? t + 8.0f : z;
                    addArchT(buf, i, ax, az, 8.0f, 1.2f + colH + 1.4f, 7.0f, colR * 1.5f, (a == 0) ? 1 : 0, stone);
                }
            }
        }
    }
}
