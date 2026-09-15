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
static const int   SEC_COL_SEG  =  5;     // (historico: el fuste iba en tramos porque un quad
                                          // de 16u cruzaba el plano de camara y el hardware lo
                                          // descartaba entero. Ya se habilito GU_CLIP_PLANES.)
// --- CAPILLAS: lo que hay HOY en los bordes del pasillo, en lugar de las columnas ---
static const float SEC_CH_W    = 5.2f;               // frente, a lo largo del pasillo
static const float SEC_CH_D    = 3.4f;               // fondo, hacia la masa
static const float SEC_CH_TER  = SEC_COL_H;          // 18.6: CUBIERTA PISABLE. No tocar:
                                                     // el ancla A1 y los materiales M6 y M7
                                                     // del bucle de juego estan justo encima.
static const float SEC_CH_CORN = SEC_CH_TER - 1.8f;  // 16.8: arranque de la cornisa
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
    // CAPILLAS: solidas hasta su cubierta, que asi queda PISABLE. Eso no es un extra
    // estetico: el ancla A1 y los materiales M6 y M7 estan justo encima, a 18.6.
    // Una sola caja por capilla, con la huella de la cornisa (que es lo que mas vuela)
    // y el fondo de los contrafuertes. El pretil y los pinaculos NO llevan caja: son
    // relieve, y darsela convertiria la plataforma en una trampa de la que no se sale.
    for (int a = 0; a < 2; ++a)
        for (int s = -1; s <= 1; s += 2)
            for (int k = -1; k <= 1; k += 2) {
                const float t = (float)k * SEC_COL_STEP;
                const float x = (a == 0) ? (SEC_COL_OFF * (float)s) : t;
                const float z = (a == 0) ? t : (SEC_COL_OFF * (float)s);
                const float along = SEC_CH_W + 1.3f, lat = SEC_CH_D + 1.5f;
                g_city[n++] = { x, z, (a == 0) ? lat : along, (a == 0) ? along : lat,
                                SEC_CH_TER, c };
            }
    // CONTRAFUERTES: pilares de piedra de 24.6 de alto que vuelan 2.4 al pasillo
    // (ocupan de 6.6 a 9.0 en la coordenada perpendicular). Se dibujaban desde el
    // principio pero no tenian caja: se atravesaban caminando, y la camara tambien.
    // Entran los 24: los 16 de |o| = 10.8 y tambien los 8 del CENTRO de cada cara. El
    // central se dejaba fuera dando por hecho que lo tapaba la caja del portal ciego de
    // arch_detail.h, pero esa caja solo llega a y=10.5 (el tope real de las jambas):
    // de ahi a 24.6 el pilar quedaba fantasma. El portal es una puerta CIEGA, no un paso,
    // asi que macizo de suelo a cornisa es lo correcto.
    for (int q = 0; q < 4; ++q) {
        float sx = (q & 1) ? -1.0f : 1.0f, sz = (q & 2) ? -1.0f : 1.0f;
        float x = mc * sx, z = mc * sz;
        for (int k = -1; k <= 1; ++k) {             // los TRES, central incluido
            float o = (float)k * ms * SEC_BUT_O;
            g_city[n++] = { x - sx * (ms * 0.5f + SEC_BUT_DP), z + o,
                            SEC_BUT_W, SEC_BUT_D, SEC_BUT_H, c };
            g_city[n++] = { x + o, z - sz * (ms * 0.5f + SEC_BUT_DP),
                            SEC_BUT_D, SEC_BUT_W, SEC_BUT_H, c };
        }
    }
    g_cityCount = n;   // 4 masas + 4 remates + 4 muros + 8 capillas + 24 contrafuertes = 44
                       // (+38 de arch_detail.h = 82 de las 256 de g_city)
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

// =================================================================================
// CAPILLA GOTICA EXENTA (702 verts). Sustituye a las columnas de la arcada.
//
// Para que lea como EDIFICIO y no como pilar decorado: basamento que sobresale,
// cuerpo por hiladas, contrafuertes en los costados, hornacina ojival hundida en el
// frente con gablete encima, cornisa volada y pinaculos en las esquinas.
//
// LA ALTURA DE LA CUBIERTA NO SE PUEDE TOCAR. La terraza queda a SEC_COL_H (18.6)
// porque el bucle de juego la usa como PLATAFORMA: el ancla A1 esta en (7, 18.6, 16)
// y los materiales M6 y M7 en (-7, 18.6, 16) y (16, 18.6, -7). Los tres caen en
// |t| = SEC_COL_STEP, que es justo donde van las ocho capillas. Si alguien baja o
// sube esta cubierta, esas tres cosas quedan en el aire y el juego deja de poder
// terminarse. Por eso los pinaculos van en las ESQUINAS: el centro de la terraza
// tiene que quedar libre para pararse y saltar.
//
// a = eje del pasillo (0 = corre en Z, lateral = X; 1 = corre en X, lateral = Z).
// s = lado del pasillo. "dLat" positivo mueve HACIA el centro del pasillo.
// =================================================================================
static inline void chPos(int a, float s, float cx, float cz, float dLat, float dAlong,
                         float &ox, float &oz) {
    if (a == 0) { ox = cx - s * dLat; oz = cz + dAlong; }
    else        { ox = cx + dAlong;   oz = cz - s * dLat; }
}
static inline void chBox(TexVertex *buf, int &i, int a, float cx, float y, float cz,
                         float along, float lat, float h, unsigned int col) {
    if (a == 0) addSolidBoxT(buf, i, cx, y, cz, lat, along, h, col);
    else        addSolidBoxT(buf, i, cx, y, cz, along, lat, h, col);
}
static inline void chPyr(TexVertex *buf, int &i, int a, float cx, float y, float cz,
                         float along, float lat, float ah, unsigned int col) {
    if (a == 0) addPyramidT(buf, i, cx, y, cz, lat, along, ah, col);
    else        addPyramidT(buf, i, cx, y, cz, along, lat, ah, col);
}
static void buildChapel(TexVertex *buf, int &i, int a, float s, float cx, float cz,
                        unsigned int stone, unsigned int dark)
{
    const float W = SEC_CH_W, D = SEC_CH_D, TER = SEC_CH_TER, CORN = SEC_CH_CORN;
    float px, pz, qx, qz;

    chPos(a, s, cx, cz, 0.0f, 0.0f, px, pz);
    chBox(buf, i, a, px, 0.0f, pz, W + 0.9f, D + 0.9f, 1.1f, dark);                 // basamento
    const float course = (CORN - 1.1f) / 3.0f;                                       // 3 hiladas
    for (int g = 0; g < 3; ++g)
        chBox(buf, i, a, px, 1.1f + course * (float)g, pz, W, D, course,
              (g & 1) ? brighten(stone, 0.93f) : stone);

    // CONTRAFUERTES en los dos costados, en dos tramos con retranqueo (el de arriba
    // vuela menos): es lo que da el perfil escalonado de una capilla de verdad.
    for (int e = -1; e <= 1; e += 2) {
        chPos(a, s, cx, cz, 0.0f, (float)e * (W * 0.5f - 0.30f), qx, qz);
        chBox(buf, i, a, qx,  1.1f, qz, 1.2f, D + 1.5f,  9.0f, dark);
        chBox(buf, i, a, qx, 10.1f, qz, 1.0f, D + 0.9f,  6.7f, brighten(dark, 1.10f));
    }

    chBox(buf, i, a, px, CORN,        pz, W + 1.3f, D + 1.3f, 0.9f, dark);          // cornisa
    chBox(buf, i, a, px, CORN + 0.9f, pz, W + 1.3f, D + 1.3f, 0.9f, stone);         // terraza -> 18.6

    // PRETIL del borde. A proposito SIN caja de colision: desde aqui se salta, y un
    // pretil solido de 1.0 convertiria la plataforma en una trampa.
    for (int e = -1; e <= 1; e += 2) {
        chPos(a, s, cx, cz, 0.0f, (float)e * (W * 0.5f + 0.45f), qx, qz);
        chBox(buf, i, a, qx, TER, qz, 0.4f, D + 1.3f, 1.0f, dark);
        chPos(a, s, cx, cz, (float)e * (D * 0.5f + 0.45f), 0.0f, qx, qz);
        chBox(buf, i, a, qx, TER, qz, W + 1.3f, 0.4f, 1.0f, dark);
    }

    // HORNACINA OJIVAL. El pano oscuro se lee HUNDIDO porque las jambas sobresalen
    // por delante de el: mas barato que vaciar el muro y se lee igual de lejos.
    chPos(a, s, cx, cz, D * 0.5f + 0.05f, 0.0f, px, pz);
    chBox(buf, i, a, px, 1.1f, pz, 2.3f, 0.10f, 8.6f, brighten(dark, 0.62f));
    chPos(a, s, cx, cz, D * 0.5f + 0.16f, 0.0f, px, pz);
    chPyr(buf, i, a, px, 9.7f, pz, 2.3f, 0.32f, 2.2f, brighten(dark, 0.62f));       // cabeza ojival
    for (int e = -1; e <= 1; e += 2) {
        chPos(a, s, cx, cz, D * 0.5f + 0.22f, (float)e * 1.55f, qx, qz);
        chBox(buf, i, a, qx, 1.1f, qz, 0.8f, 0.45f, 9.2f, brighten(stone, 1.08f));  // jambas
    }
    chPos(a, s, cx, cz, D * 0.5f + 0.18f, 0.0f, px, pz);
    chPyr(buf, i, a, px, 12.1f, pz, 4.2f, 0.36f, 3.1f, brighten(stone, 1.14f));     // gablete

    // PINACULOS en las cuatro esquinas: rematan la silueta y dejan el centro de la
    // terraza libre, que es por donde pasa la ruta de escalada.
    for (int e = -1; e <= 1; e += 2)
        for (int f = -1; f <= 1; f += 2) {
            chPos(a, s, cx, cz, (float)f * (D * 0.5f + 0.30f), (float)e * (W * 0.5f + 0.30f), qx, qz);
            chBox(buf, i, a, qx, TER + 1.0f, qz, 0.9f, 0.9f, 2.2f, stone);
            chPyr(buf, i, a, qx, TER + 3.2f, qz, 0.9f, 0.9f, 2.4f, brighten(stone, 1.20f));
        }
}

// --- estructura: masas separadas + muros + capillas en los pasillos ---
static void buildSectorWalls(TexVertex *buf, int &i) {
    const unsigned int stone = brighten(RGBA(69, 65, 58, 255), 2.1f);
    const unsigned int dark  = brighten(RGBA(46, 41, 36, 255), 2.0f);
    // PAREDES mas oscuras (Benjamin: "a las paredes colocale un color mas oscuro, no un
    // color tan piedra"). Son los paños grandes: el cuerpo de las masas y los muros
    // exteriores. Se baja el multiplicador de 2.1 a 1.35, o sea de (144,136,121) a
    // (93,88,78). Se mantiene el MISMO tono calido (r > g > b) a proposito: el problema
    // viejo de "todo se ve gris" venia de una piedra azulada, y volver a enfriarla lo
    // traeria de vuelta. Solo baja el valor, no cambia el color.
    // El relieve gotico, las capillas y las cornisas NO se tocan: al quedar mas claras
    // que el paño, la arquitectura resalta contra la pared en vez de fundirse con ella.
    const unsigned int wallStone = brighten(RGBA(69, 65, 58, 255), 1.35f);
    const float mc = (SEC_M0 + SEC_M1) * 0.5f, ms = (SEC_M1 - SEC_M0);

    // 4 MASAS separadas: cuerpo + cornisa + remate escalonado (silueta gotica, no un ladrillo)
    for (int q = 0; q < 4; ++q) {
        float sx = (q & 1) ? -1.0f : 1.0f, sz = (q & 2) ? -1.0f : 1.0f;
        float x = mc * sx, z = mc * sz;
        // las medidas salen de las constantes SEC_CORN_*/SEC_CAP_*/SEC_BUT_* de arriba:
        // buildSectorCollision usa LAS MISMAS, asi lo que se ve y lo que se choca no
        // se pueden separar (era justo el bug: cornisa, remate y contrafuertes sin caja).
        addSolidBoxT(buf, i, x, 0.0f, z, ms, ms, SEC_MASS_H, wallStone);                    // cuerpo
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
        addSolidBoxT(buf, i, cx, 0.0f,  cz, bw, bd, opY0, wallStone);                   // antifecho
        addSolidBoxT(buf, i, cx, opY1,  cz, bw, bd, SEC_CEIL - opY1, wallStone);        // muro alto
        for (int k = 0; k < nPier; ++k) {        // pilares entre ventanal y ventanal
            float t = -wl * 0.5f + bay * (float)k;
            if (t < -wl * 0.5f || t > wl * 0.5f) continue;
            addSolidBoxT(buf, i, alongX ? t : cx, opY0, alongX ? cz : t,
                         alongX ? pierW : wt, alongX ? wt : pierW, opY1 - opY0, dark);
        }
    }

    // CAPILLAS en los bordes del pasillo en cruz, donde antes habia 20 columnas con
    // arcos. Benjamin, probandolo en la consola: "borra columnas, hay muchas, y no
    // quiero que sean columnas, tienen que ser como iglesias o algo gotico".
    // Son OCHO, solo en |t| = SEC_COL_STEP, y esa posicion no es decorativa: el bucle de
    // juego usa su cubierta como plataforma (ver buildChapel).
    for (int a = 0; a < 2; ++a)                      // a=0 pasillo en Z, a=1 pasillo en X
        for (int s = -1; s <= 1; s += 2)             // los dos lados del pasillo
            for (int k = -1; k <= 1; k += 2) {       // solo t = -16 y +16
                const float t = (float)k * SEC_COL_STEP;
                const float x = (a == 0) ? (SEC_COL_OFF * (float)s) : t;
                const float z = (a == 0) ? t : (SEC_COL_OFF * (float)s);
                buildChapel(buf, i, a, (float)s, x, z, stone, dark);
            }
}
