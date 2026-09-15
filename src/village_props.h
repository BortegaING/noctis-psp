#pragma once
// ============ PROJECT NOCTIS - AMBIENTACION DEL SECTOR EN CRUZ (props) ============
// El mundo (sector.h) es un recinto CERRADO: 4 MASAS de 36x36 en los cuadrantes
// (9 < |x|,|z| < 45), un PASILLO EN CRUZ (franja |x|<9 a lo largo de Z y franja
// |z|<9 a lo largo de X), techo a y=60 y COLUMNAS EXENTAS en lat=+-7 cada 16u.
// Aqui NO se levanta arquitectura: se AMUEBLAN los pasillos pegados a sus bordes
// para darles ALMA (estatuas, sarcofagos, escombros, cadenas) y la UNICA LUZ
// CALIDA del juego (braseros). Todo lo demas es piedra gris FRIA.
//
//   corte de una crujia (mirando a lo largo)      planta de media crujia
//   techo y=60 ============================        masa  |brasero   estatua |  masa
//      |cadena|             |cadena|             lat 9 ->|  o 7.0   o 7.75  |
//      arco 18.6..27.3      arco 18.6..27.3              |   COLUMNA        |
//   masa |COL|  CENTRO LIBRE  |COL| masa         --------+-- centro libre --+-------
//   suelo y=0 =============================
//
// REGLAS DE COLOCACION (medidas LEIDAS de sector.h y arch_detail.h, no de memoria):
//   * |lat| <= 8.8 SIEMPRE -> nada entra jamas en una masa (las masas empiezan en 9.0).
//   * |lat| >= 4.65 SIEMPRE -> el centro del pasillo queda libre para caminar.
//   * columnas en lat=+-7, t = 16k (k=-2..2), huella 3.3 -> los props van entre
//     columnas o corridos 2.85 en t: no se tapa ninguna.
//   * CADA CARA DE MASA (|t| de 9 a 45) esta casi toda comida por relieve SOLIDO que
//     vuela al pasillo. Bandas de |t| que YA tienen caja en g_city, con su vuelo:
//        9.30..13.10  contrafuerte de arch_detail (AD_BT_O 15.8 +- 1.9)  2.5 -> lat 6.5
//       14.35..17.70  columna exenta (t=16) + contrafuerte de sector (10.8 +- 1.5) 2.4
//       23.10..30.90  PORTAL de arch_detail (AD_PT_HALF 3.9)             2.3 -> lat 6.7
//                     (dentro de esa banda va tambien el contrafuerte CENTRAL de
//                      sector.h, 25.50..28.50: no abre hueco nuevo, lo tapa el portal)
//       30.35..33.65  columna exenta (t=32)
//       36.30..39.30  contrafuerte de sector
//       40.90..44.70  contrafuerte de arch_detail
//     -> el UNICO hueco ancho pegado al muro es |t| 17.70..23.10 (5.4 de luz, centro
//        20.40 = VP_BAY) y SOLO CABE UN PROP. Hay 8 caras y 8 props de muro: uno cada.
//   * blocked() infla cada caja con el radio 1.1 del jugador -> todo prop deja >= 1.1
//     hasta el borde de la caja mas cercana, o se choca con un muro invisible.
//   * en ese hueco hay relieve que NO colisiona: la arcada ciega (|t| 17.95..23.05,
//     y 3.40..12.50), cuya columnilla vuela 0.42 -> la estatua se planta en lat 7.75
//     en vez de 8.10 (hombros a 0.73 de la pared) o se la comeria.
//   * escombros y columna rota NO caben en ningun hueco de cara: bajan a la BOCA del
//     cruce (|t| < 8.2), al pie del pilar de esquina, que es de donde se cayeron.
//   * arcos ojivales entre columnas: ocupan lat 5.54..8.46 entre y=18.6 y y=27.3
//     -> las cadenas cuelgan por DENTRO (lat 4.8..5.0) y fuera de esa banda.
// Determinista: hash entero + tablas fijas (SIN rand, SIN heap, C++17). 2196 verts.

// --- copias locales de la geometria de sector.h (ese header se incluye DESPUES) ---
static const float VP_CEIL     = 60.0f;   // SEC_CEIL     : techo del sector
static const float VP_STEP     = 16.0f;   // SEC_COL_STEP : separacion entre columnas
static const float VP_LAT_COL  =  7.0f;   // SEC_COL_OFF  : eje de las columnas exentas
static const float VP_LAT_WALL =  8.10f;  // arrimado a la cara de la masa (que esta en 9.0)
static const float VP_LAT_STAT =  7.75f;  // estatua: 0.50 de aire tras el pedestal y 0.73 tras
                                          // los hombros -> libra la columnilla de la arcada
                                          // ciega de arch_detail (vuela 0.42 desde y=3.40)
static const float VP_BAY      = 20.40f;  // centro del UNICO tramo libre de cada cara:
                                          // |t| 17.70 (contrafuerte + columna) .. 23.10 (portal)

// --- hash entero determinista (mismo mezclador que winPick): variacion sin rand ---
static unsigned int vpHash(unsigned int x) {
    x ^= 61u; x ^= (x >> 16); x *= 9u; x ^= (x >> 4); x *= 0x27d4eb2du; x ^= (x >> 15);
    return x;
}
static float vpRnd(unsigned int s) { return (float)(vpHash(s) & 1023u) * (1.0f / 1023.0f); }

// --- (eje, lado, avance, lateral) -> (x,z) ---------------------------------------
// a=0: el pasillo corre a lo largo de Z (lateral = x). a=1: corre en X (lateral = z).
static inline void vpMap(int a, float s, float t, float lat, float &x, float &z) {
    if (a == 0) { x = lat * s; z = t; } else { x = t; z = lat * s; }
}
// caja ORIENTADA al pasillo: wl = largo a lo LARGO del pasillo, wd = fondo (hacia la pared)
static inline void vpBoxA(LineVertex *buf, int &i, int a, float x, float y, float z,
                          float wl, float wd, float h, unsigned int col) {
    if (a == 0) addSolidBox(buf, i, x, y, z, wd, wl, h, col);
    else        addSolidBox(buf, i, x, y, z, wl, wd, h, col);
}
static inline void vpPyrA(LineVertex *buf, int &i, int a, float x, float y, float z,
                          float wl, float wd, float ah, unsigned int col) {
    if (a == 0) addPyramid(buf, i, x, y, z, wd, wl, ah, col);
    else        addPyramid(buf, i, x, y, z, wl, wd, ah, col);
}

// --- PIEDRA: gris FRIA (r<=g<=b) con variacion por hash + niebla horneada ---------
// f = valor de la pieza (basa oscura, remate claro). La distancia al centro funde el
// prop hacia la bruma igual que el resto del mundo -> profundidad dentro del pasillo.
static unsigned int vpStone(unsigned int seed, float x, float z, float f) {
    const unsigned int STONE = RGBA( 78,  70,  60, 255);          // piedra gris fria (base pedida)
    const float v = 0.90f + 0.20f * vpRnd(seed);               // variacion de sillar
    return fadeToVoid(brighten(STONE, 1.72f * f * v), sqrtf(x * x + z * z));
}
static const unsigned int VP_FLAME = RGBA(255, 168,  72, 255); // llama EMISIVA calida (unico acento)
static const unsigned int VP_CORE  = RGBA(255, 226, 168, 255); // corazon al rojo blanco
static const unsigned int VP_IRON  = RGBA( 78,  80,  92, 255); // hierro frio (cadenas)

// =================================================================================
// BRASERO DE PIE (84 verts): pie de piedra + pebetero + llama de 2 capas.
// Es la unica luz calida: la piedra del pebetero lleva warmTint (la lame el fuego).
// =================================================================================
static void vpBrazier(LineVertex *buf, int &i, float x, float z, unsigned int seed) {
    const unsigned int st  = vpStone(seed, x, z, 0.96f);
    const unsigned int lit = warmTint(vpStone(seed + 3u, x, z, 1.30f));
    const float fh = 1.30f + 0.50f * vpRnd(seed * 7u + 5u);         // ninguna llama igual
    addSolidBox(buf, i, x, 0.00f, z, 0.54f, 0.54f, 1.85f, st);      // 30 pie (fuste esbelto)
    addSolidBox(buf, i, x, 1.85f, z, 1.16f, 1.16f, 0.42f, lit);     // 30 pebetero
    addPyramid (buf, i, x, 2.27f, z, 0.98f, 0.98f, fh,         VP_FLAME); // 12 llama
    addPyramid (buf, i, x, 2.40f, z, 0.52f, 0.52f, fh * 0.70f, VP_CORE);  // 12 nucleo
}

// =================================================================================
// ESTATUA ENCAPUCHADA SOBRE PEDESTAL (102 verts). Mira al pasillo: el fondo (hacia
// la pared) es menor que el ancho de hombros. La capucha se ladea distinto en cada una.
// =================================================================================
static void vpStatue(LineVertex *buf, int &i, int a, float x, float z, unsigned int seed) {
    const unsigned int ped = vpStone(seed,       x, z, 0.88f);
    const unsigned int rob = vpStone(seed + 17u, x, z, 1.00f);
    const unsigned int hd  = vpStone(seed + 31u, x, z, 1.12f);
    const float lean = (vpRnd(seed + 5u) - 0.5f) * 0.24f;           // cabeza ladeada
    const float lx = (a == 0) ? 0.0f : lean, lz = (a == 0) ? lean : 0.0f;
    vpBoxA(buf, i, a, x, 0.00f, z, 1.62f, 1.50f, 1.05f, ped);       // 30 pedestal
    vpBoxA(buf, i, a, x, 1.05f, z, 1.06f, 0.90f, 2.45f, rob);       // 30 tunica (angosta: figura)
    vpBoxA(buf, i, a, x, 3.50f, z, 1.34f, 1.04f, 0.55f, rob);       // 30 hombros/esclavina
    vpPyrA(buf, i, a, x + lx, 4.05f, z + lz, 1.02f, 0.84f, 1.30f, hd); // 12 CAPUCHA en punta
}

// =================================================================================
// SARCOFAGO / BANCO BAJO contra la pared (60 verts): cuerpo + tapa que vuela.
// =================================================================================
static void vpTomb(LineVertex *buf, int &i, int a, float x, float z, unsigned int seed) {
    vpBoxA(buf, i, a, x, 0.00f, z, 2.70f, 1.02f, 0.62f, vpStone(seed,       x, z, 0.92f)); // 30 cuerpo
    vpBoxA(buf, i, a, x, 0.62f, z, 2.94f, 1.20f, 0.24f, vpStone(seed + 23u, x, z, 1.06f)); // 30 tapa
}

// =================================================================================
// ESCOMBROS: dos sillares caidos (60 verts). El segundo rodo a lo largo del pasillo.
// =================================================================================
static void vpRubble(LineVertex *buf, int &i, int a, float x, float z, unsigned int seed) {
    const float o  = 0.95f + 0.55f * vpRnd(seed + 3u);                     // se alejo rodando
    const float sl = ((vpHash(seed) & 1u) ? -0.42f : 0.42f);               // y se corrio de lado
    const float dx = (a == 0) ? sl : o, dz = (a == 0) ? o : sl;
    vpBoxA(buf, i, a, x,      0.0f, z,      1.25f, 0.98f, 0.60f, vpStone(seed,      x, z, 0.84f)); // 30
    vpBoxA(buf, i, a, x + dx, 0.0f, z + dz, 0.80f, 0.70f, 0.38f, vpStone(seed + 9u, x, z, 0.78f)); // 30
}

// =================================================================================
// TROZO DE COLUMNA PARTIDA (90 verts): basa + fuste truncado + el tambor desprendido.
// =================================================================================
static void vpBrokenCol(LineVertex *buf, int &i, int a, float x, float z, unsigned int seed) {
    const float ht = 1.50f + 1.80f * vpRnd(seed + 11u);                    // rota a distinta altura
    const float dx = (a == 0) ? 0.30f : 1.35f, dz = (a == 0) ? 1.35f : 0.30f;
    addSolidBox(buf, i, x, 0.00f, z, 1.04f, 1.04f, 0.34f, vpStone(seed + 4u, x, z, 0.86f)); // 30 basa
    addSolidBox(buf, i, x, 0.34f, z, 0.76f, 0.76f, ht,    vpStone(seed,      x, z, 0.98f)); // 30 fuste
    vpBoxA(buf, i, a, x + dx, 0.0f, z + dz, 1.45f, 0.76f, 0.74f,
           vpStone(seed + 8u, x, z, 0.90f));                                                // 30 tambor caido
}

// PIRAMIDE QUE APUNTA HACIA ABAJO. Llamar a addPyramid con apexH NEGATIVO pone el
// apice al otro lado de la base, y eso invierte el winding de las cuatro caras: con
// cull ON (que es como se dibuja g_vprops) el contrapeso se transformaba y se
// descartaba entero, asi que las cadenas colgaban del techo y terminaban en nada.
// Aqui la base se recorre al reves, que es justo lo que compensa la inversion.
static void vpPyramidDown(LineVertex *buf, int &i, float cx, float baseY, float cz,
                          float w, float d, float drop, unsigned int col) {
    const float x0 = cx - w * 0.5f, x1 = cx + w * 0.5f;
    const float z0 = cz - d * 0.5f, z1 = cz + d * 0.5f;
    const float y0 = baseY, ay = baseY - drop;
    const unsigned int a = brighten(col, 1.10f), b = brighten(col, 0.72f);
    buf[i++] = { a, x1, y0, z0 }; buf[i++] = { a, x0, y0, z0 }; buf[i++] = { a, cx, ay, cz };
    buf[i++] = { b, x1, y0, z1 }; buf[i++] = { b, x1, y0, z0 }; buf[i++] = { b, cx, ay, cz };
    buf[i++] = { a, x0, y0, z1 }; buf[i++] = { a, x1, y0, z1 }; buf[i++] = { a, cx, ay, cz };
    buf[i++] = { b, x0, y0, z0 }; buf[i++] = { b, x0, y0, z1 }; buf[i++] = { b, cx, ay, cz };
}

// =================================================================================
// CADENA DEL TECHO (42 verts): caja fina de y=VP_CEIL hasta yBot + contrapeso en
// punta (piramide que mira ABAJO, con su propio winding). Vende los 60u de altura.
// =================================================================================
static void vpChain(LineVertex *buf, int &i, float x, float z, float yBot, unsigned int seed) {
    const unsigned int fe = fadeToVoid(brighten(VP_IRON, 0.92f + 0.16f * vpRnd(seed)),
                                       sqrtf(x * x + z * z));
    addSolidBox(buf, i, x, yBot, z, 0.30f, 0.30f, VP_CEIL - yBot, fe);             // 30 tramo colgante
    vpPyramidDown(buf, i, x, yBot, z, 0.60f, 0.60f, 0.95f, brighten(fe, 0.85f));   // 12 contrapeso
}

// =================================================================================
// INCENSARIO COLGANTE (102 verts): la cadena baja enganchada al BORDE del cuenco
// (de ahi el offset), asi la llama sube libre por el centro sin atravesarla.
// =================================================================================
static void vpCenser(LineVertex *buf, int &i, int a, float x, float z, unsigned int seed) {
    const float ax = (a == 0) ? 0.0f : 0.46f, az = (a == 0) ? 0.46f : 0.0f;
    const unsigned int fe = fadeToVoid(VP_IRON, sqrtf(x * x + z * z));
    addSolidBox(buf, i, x + ax, 20.00f, z + az, 0.26f, 0.26f, VP_CEIL - 20.0f, fe);        // 30 tramo alto
    addSolidBox(buf, i, x + ax,  6.95f, z + az, 0.22f, 0.22f, 13.05f, brighten(fe, 0.9f)); // 30 tramo bajo
    addSolidBox(buf, i, x, 6.45f, z, 1.08f, 1.08f, 0.50f,
                warmTint(vpStone(seed, x, z, 1.28f)));                                     // 30 cuenco
    addPyramid (buf, i, x, 6.95f, z, 0.92f, 0.92f, 1.40f, VP_FLAME);                       // 12 llama
}

// =================================================================================
// firma exacta pedida: recibe el buffer (g_vprops[2500]) y devuelve el n de verts.
// =================================================================================
static int buildVillageProps(LineVertex *buf) {
    int i = 0;

    // ---- 1. BRASEROS (12 x 84 = 1008) : 1 de cada 2 columnas -> k = -2, 0, +2 ----
    // Van sobre el EJE de la columna (lat 7.0) y corridos 2.85 en t: apoyados contra
    // el fuste sin taparlo (capitel 1.8 de medio ancho + 0.58 del pie = 2.38 < 2.85).
    // Los 4 del centro (k=0) alternan en molinete: enmarcan el spawn sin cerrarlo.
    for (int a = 0; a < 2; ++a)
        for (int sI = 0; sI < 2; ++sI) {
            const float s = sI ? -1.0f : 1.0f;
            for (int n = 0; n < 3; ++n) {
                const int   k  = (n - 1) * 2;                              // -2, 0, +2
                const float dt = (k == 0) ? (2.85f * s) : ((k < 0) ? -2.85f : 2.85f);
                float x, z; vpMap(a, s, (float)k * VP_STEP + dt, VP_LAT_COL, x, z);
                vpBrazier(buf, i, x, z, (unsigned int)(a * 53 + sI * 29 + n * 7 + 1));
            }
        }

    // ---- 2. ESTATUAS (4 x 102 = 408) : arrimadas a la cara de la masa, en molinete --
    // t=+-24 las metia ENTERAS en la caja del portal de arch_detail (|t| 23.10..30.90,
    // vuelo 2.3, 10.5 de alto): no se veian, y el jugador topaba con un bloque de 10.5
    // donde hay una figura de 5.35. Ahora van al centro del hueco (VP_BAY = 20.40): el
    // pedestal ocupa |t| 19.59..21.21, o sea 1.89 de aire a cada lado.
    for (int a = 0; a < 2; ++a)
        for (int sI = 0; sI < 2; ++sI) {
            const float s = sI ? -1.0f : 1.0f;
            float x, z; vpMap(a, s, VP_BAY * s, VP_LAT_STAT, x, z);
            vpStatue(buf, i, a, x, z, (unsigned int)(a * 97 + sI * 41 + 13));
        }

    // ---- 3. SARCOFAGOS / BANCOS (4 x 60 = 240) : molinete opuesto, en las otras 4 caras
    // t=-+23 dejaba la tapa en |t| 21.53..24.47 y el portal empieza en 23.10: 1.37 de sus
    // 2.94 (47%) enterrados. Centrada en VP_BAY ocupa |t| 18.93..21.87 -> 1.23 por lado.
    // La lat NO cambia: mide 0.86 de alto y pasa por debajo de la arcada (que abre en 3.40).
    for (int a = 0; a < 2; ++a)
        for (int sI = 0; sI < 2; ++sI) {
            const float s = sI ? -1.0f : 1.0f;
            float x, z; vpMap(a, s, -VP_BAY * s, VP_LAT_WALL + 0.05f, x, z);
            vpTomb(buf, i, a, x, z, (unsigned int)(a * 131 + sI * 67 + 7));
        }

    // ---- 4. ESCOMBROS (2 x 60 + 90 = 210) : en la BOCA del cruce, al pie de la esquina
    // Los tres estaban ENTERRADOS dentro de un contrafuerte de arch_detail (|t| 9.30..13.10,
    // vuelo 2.5, macizo de y=0 a 30.4): no se veia ninguno. En una cara no cabe ninguno (su
    // unico hueco, |t| 17.70..23.10, ya lo ocupa la estatua o el sarcofago), asi que bajan al
    // cruce, DELANTE del pilar de esquina -> a >= 1.2 de las dos cajas que lo forman.
    vpRubble   (buf, i, 0,  -7.30f,   4.60f,  3u);   // esquina -X/+Z, mirando al pasillo +Z
    vpRubble   (buf, i, 1,  -5.60f,   7.30f, 19u);   // misma esquina, mirando al pasillo -X
    vpBrokenCol(buf, i, 1,   4.70f,  -7.40f, 37u);   // boca +X, lado -Z (sigue entre las columnas 0 y 16)

    // ---- 5. CADENAS DEL TECHO (3 x 42 = 126) --------------------------------------
    // lat 4.8 = por DENTRO del arco (que empieza en 5.54) y fuera del centro pisable;
    // cortan a y>=16.5, muy por encima de la cabeza, y el t elegido esquiva la banda
    // lateral de los arcos de la crujia perpendicular (|t| > 8.2).
    vpChain(buf, i,   4.80f,  12.00f, 18.0f,  5u);
    vpChain(buf, i,  -4.80f, -40.00f, 16.5f, 11u);
    vpChain(buf, i, -24.00f,   4.80f, 19.5f, 23u);

    // ---- 6. INCENSARIOS COLGANTES (2 x 102 = 204) ---------------------------------
    // La otra fuente de luz calida: baja hasta y~6.5, casi a la altura de la mirada.
    vpCenser(buf, i, 0,  -5.00f,   9.00f, 71u);
    vpCenser(buf, i, 1,  25.00f,  -5.00f, 83u);

    return i;   // 2196 verts (limite 2200, buffer g_vprops[2500])
}
