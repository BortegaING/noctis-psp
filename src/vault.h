#pragma once
#include <math.h>
#include "sector.h"
// ================== BOVEDA: detalle arquitectonico del TECHO (y = SEC_CEIL) ==================
// El techo del sector era un plano teselado y VACIO: mirar arriba no daba nada. Esto le cuelga
// NERVIOS de piedra por debajo, de modo que cada tramo de pasillo se lea como una BOVEDA DE
// CRUCERIA y el cruce central como el CRUCERO de una nave.
//
//   corte de un nervio (cruza el pasillo)          planta del CRUCE (|x|,|z| < 9)
//   y=60 ==============================                 \    |    /
//        |  |                      |  |                  \   |   /   nervios DIAGONALES
//        |  |____              ____|  |                   \  |  /
//        |       \____________/       |                 ----[C]----  C = CLAVE
//   y~54 |_/|                    |\_|                     /  |  \    arcos FAJONES en +-9
//         M                        M                     /   |   \
//        M = MENSULA (piramide invertida)               /    |    \
//
// COMO ENCAJA CON sector.h:
//   - los nervios transversales van alineados con las COLUMNAS (SEC_COL_STEP = 16), o sea en
//     t = -32,-16,+16,+32 de cada pasillo; t = 0 lo ocupa la cruceria del crucero.
//   - la luz del nervio es 21u = los 18 del pasillo + 1.5 a cada lado, para que la MENSULA
//     quede APOYADA sobre la masa (|x| >= 9) y el nervio no muera en el aire.
//   - los arcos fajones se plantan justo en el borde del crucero (SEC_M0 = 9).
//
// DOS TRAMPAS DEL MOTOR, y como se esquivan:
//   1) addSolidBoxT NO dibuja cara inferior (las cajas del mundo se miran de lado). Aqui TODO
//      se mira desde ABAJO -> addVaultBeamT dibuja INTRADOS + 4 costados y omite la tapa, que
//      queda al ras del techo y no se ve nunca. Mismo costo que addSolidBoxT: 30 verts.
//   2) no hay recorte en el plano de la camara: un triangulo con un vertice detras del jugador
//      se DESCARTA ENTERO. Por eso NINGUN nervio es una viga de 21u: van partidos en tramos
//      de 4.2u (transversales) / 5.25u (fajones) / ~3.5u (diagonales).
//
// Winding: copiado literal de addSolidBoxT -> se puede dibujar en el rango de buildSectorWalls
// (cull ON). Textura de piedra, color plano (sin fadeToVoid, igual que buildSectorWalls).

static const float VLT_TOP   = SEC_CEIL;   // 60: los nervios llegan al techo (sin junta visible)
static const float VLT_HALF  = 10.5f;      // media luz del nervio: 9 de pasillo + 1.5 sobre la masa
static const float VLT_TH    =  1.8f;      // espesor del nervio (en el eje del pasillo)
static const float VLT_SPR   = 54.4f;      // ARRANQUE: intrados del nervio junto a la masa
static const int   VLT_SEG   =  5;         // tramos por nervio transversal -> 21/5 = 4.2u
static const int   VLT_FSEG  =  4;         // tramos por arco fajon        -> 21/4 = 5.25u

// intrados por tramo, indexado por distancia al centro: el nervio SUBE hacia la clave, o sea
// cuelga mucho junto a la masa y casi nada en el eje -> desde abajo se lee como arco.
static const float VLT_RIB_Y[3] = { 58.1f, 56.8f, VLT_SPR };   // |j-2| = 0, 1, 2
static const float VLT_FAJ_Y[2] = { 57.6f, VLT_SPR };          // tramo interior / exterior

// recorrido de UNA diagonal del crucero: esquina (9,9) -> clave. {x, z, y del intrados}
static const float VLT_DIAG[4][3] = {
    { 9.0f, 9.0f, VLT_SPR }, { 6.5f, 6.5f, 57.0f }, { 4.0f, 4.0f, 58.1f }, { 1.8f, 1.8f, 58.5f }
};

// --- VIGA vista desde ABAJO: intrados + 4 costados, sin tapa (30 verts, como addSolidBoxT) ---
static void addVaultBeamT(TexVertex *buf, int &i, float cx, float baseY, float cz,
                          float w, float d, float h, unsigned int col) {
    const float x0 = cx - w * 0.5f, x1 = cx + w * 0.5f;
    const float z0 = cz - d * 0.5f, z1 = cz + d * 0.5f;
    const float y0 = baseY, y1 = baseY + h;
    const unsigned int bot = brighten(col, 0.50f);                           // el intrados es lo mas oscuro
    const unsigned int sa  = brighten(col, 0.86f), sb = brighten(col, 0.64f);
    const float uw = w / TILE, ud = d / TILE, uh = h / TILE;
    addQuadT(buf,i, x0,y0,z1, x1,y0,z1, x1,y0,z0, x0,y0,z0, 0,0,uw,ud, bot); // INTRADOS (mira abajo)
    addQuadT(buf,i, x0,y0,z0, x1,y0,z0, x1,y1,z0, x0,y1,z0, 0,uh,uw,0, sa);  // frente
    addQuadT(buf,i, x1,y0,z1, x0,y0,z1, x0,y1,z1, x1,y1,z1, 0,uh,uw,0, sa);  // atras
    addQuadT(buf,i, x0,y0,z1, x0,y0,z0, x0,y1,z0, x0,y1,z1, 0,uh,ud,0, sb);  // izq
    addQuadT(buf,i, x1,y0,z0, x1,y0,z1, x1,y1,z1, x1,y1,z0, 0,uh,ud,0, sb);  // der
}

// --- MENSULA: piramide INVERTIDA (cuadrado arriba, punta abajo) = consola gotica, 12 verts ---
// addPyramidT no sirve: su apice va ARRIBA. Aqui el apice va abajo, asi que el recorrido de la
// base se invierte para que las caras sigan mirando hacia afuera.
static void addCorbelT(TexVertex *buf, int &i, float cx, float topY, float cz,
                       float w, float drop, unsigned int col) {
    const float x0 = cx - w * 0.5f, x1 = cx + w * 0.5f;
    const float z0 = cz - w * 0.5f, z1 = cz + w * 0.5f;
    const float ay = topY - drop;
    const unsigned int a = brighten(col, 0.92f), b = brighten(col, 0.62f);
    const float uw = w / TILE, uh = drop / TILE;
    buf[i++]={uw,uh,a,x1,topY,z0}; buf[i++]={0,uh,a,x0,topY,z0}; buf[i++]={uw*0.5f,0,a,cx,ay,cz};
    buf[i++]={uw,uh,b,x1,topY,z1}; buf[i++]={0,uh,b,x1,topY,z0}; buf[i++]={uw*0.5f,0,b,cx,ay,cz};
    buf[i++]={uw,uh,a,x0,topY,z1}; buf[i++]={0,uh,a,x1,topY,z1}; buf[i++]={uw*0.5f,0,a,cx,ay,cz};
    buf[i++]={uw,uh,b,x0,topY,z0}; buf[i++]={0,uh,b,x0,topY,z1}; buf[i++]={uw*0.5f,0,b,cx,ay,cz};
}

// --- NERVIO DIAGONAL: viga entre dos puntos cualesquiera del plano XZ (no alineada a ejes).
// El INTRADOS sube de A a B y el extrados va plano contra el techo -> cuna, sin junta arriba.
// 18 verts: intrados + los 2 costados (la tapa nunca se ve, esta pegada al techo).
static void addDiagRibT(TexVertex *buf, int &i,
                        float ax, float ay, float az, float bx, float by, float bz,
                        float halfW, float topY, unsigned int col) {
    const float dx = bx - ax, dz = bz - az;
    const float len = sqrtf(dx * dx + dz * dz);
    if (len < 0.001f) return;
    const float px = -dz / len * halfW, pz = dx / len * halfW;   // perpendicular horizontal
    const float ul = len / TILE, uc = (2.0f * halfW) / TILE, uh = (topY - ay) / TILE;
    const unsigned int bot = brighten(col, 0.50f), s = brighten(col, 0.78f);
    addQuadT(buf,i, ax+px,ay,az+pz,  bx+px,by,bz+pz,  bx-px,by,bz-pz,   ax-px,ay,az-pz,
             0,0,ul,uc, bot);                                                     // intrados
    addQuadT(buf,i, bx+px,by,bz+pz,  ax+px,ay,az+pz,  ax+px,topY,az+pz, bx+px,topY,bz+pz,
             0,uh,ul,0, s);                                                       // costado +p
    addQuadT(buf,i, ax-px,ay,az-pz,  bx-px,by,bz-pz,  bx-px,topY,bz-pz, ax-px,topY,az-pz,
             0,uh,ul,0, s);                                                       // costado -p
}

// --- LUCERNARIO: recuadro hundido y mas claro justo bajo el techo (finge luz cenital).
// No se puede hundir HACIA ARRIBA (el plano del techo lo taparia), asi que el panel claro va
// pegado al techo y un labio oscuro cuelga a su alrededor: desde abajo lee como hueco. 30 verts.
static void addSkylightT(TexVertex *buf, int &i, float cx, float cz, float half,
                         unsigned int rim, unsigned int glow) {
    const float x0 = cx - half, x1 = cx + half, z0 = cz - half, z1 = cz + half;
    const float yp = VLT_TOP - 0.25f;   // panel claro, casi al ras del techo
    const float yr = VLT_TOP - 1.80f;   // borde inferior del labio
    const float uw = (2.0f * half) / TILE, uh = (yp - yr) / TILE;
    addQuadT(buf,i, x0,yp,z1, x1,yp,z1, x1,yp,z0, x0,yp,z0, 0,0,uw,uw, glow);  // panel (mira abajo)
    addQuadT(buf,i, x0,yr,z0, x1,yr,z0, x1,yp,z0, x0,yp,z0, 0,uh,uw,0, rim);   // labio -Z
    addQuadT(buf,i, x1,yr,z1, x0,yr,z1, x0,yp,z1, x1,yp,z1, 0,uh,uw,0, rim);   // labio +Z
    addQuadT(buf,i, x0,yr,z1, x0,yr,z0, x0,yp,z0, x0,yp,z1, 0,uh,uw,0, rim);   // labio -X
    addQuadT(buf,i, x1,yr,z0, x1,yr,z1, x1,yp,z1, x1,yp,z0, 0,uh,uw,0, rim);   // labio +X
}

// ================================ LA BOVEDA COMPLETA ================================
// 2418 verts exactos (presupuesto 2500). Determinista, sin rand y sin heap: se hornea una vez.
static void buildVault(TexVertex *buf, int &i) {
    const unsigned int stone = brighten(RGBA(64, 70, 82, 255), 2.1f);   // piedra del sector
    const unsigned int dark  = brighten(RGBA(46, 50, 60, 255), 2.0f);   // piedra oscura
    const unsigned int rib   = brighten(stone, 0.92f);                  // nervios: alla arriba, en penumbra
    const unsigned int key   = brighten(stone, 1.30f);                  // clave: el punto mas claro
    const unsigned int glow  = RGBA(188, 196, 212, 255);                // lucernario

    const float span = 2.0f * VLT_HALF;                 // 21 = luz total del nervio
    const float rs   = span / (float)VLT_SEG;           // 4.2  por tramo transversal
    const float fs   = span / (float)VLT_FSEG;          // 5.25 por tramo de fajon

    // --- 1) NERVIOS TRANSVERSALES + MENSULAS (el gesto principal) --------------------------
    // Un nervio por cada columna de la arcada (cada SEC_COL_STEP = 16u), en los dos pasillos.
    // t = 0 se salta: ese tramo es el crucero y lleva cruceria diagonal.
    // 2 ejes x 4 posiciones x 5 tramos = 40 vigas (1200 v) + 16 mensulas (192 v).
    for (int a = 0; a < 2; ++a) {                       // a=0 pasillo en Z (nervio cruza en X); a=1 al reves
        for (int k = -2; k <= 2; ++k) {
            if (k == 0) continue;
            const float t = (float)k * SEC_COL_STEP;
            for (int j = 0; j < VLT_SEG; ++j) {         // TRAMOS de 4.2u: nunca una viga entera de 21u
                const float o  = ((float)j - 2.0f) * rs;
                const int   lv = (j < 2) ? (2 - j) : (j - 2);
                const float y0 = VLT_RIB_Y[lv];
                const float x  = (a == 0) ? o : t,       z = (a == 0) ? t : o;
                const float w  = (a == 0) ? rs : VLT_TH, d = (a == 0) ? VLT_TH : rs;
                addVaultBeamT(buf, i, x, y0, z, w, d, VLT_TOP - y0,
                              (j & 1) ? rib : brighten(rib, 1.07f));   // dovelas alternas: se ve la junta
            }
            for (int s = -1; s <= 1; s += 2) {          // MENSULA en cada apoyo, ya sobre la masa
                const float x = (a == 0) ? (VLT_HALF * (float)s) : t;
                const float z = (a == 0) ? t : (VLT_HALF * (float)s);
                addCorbelT(buf, i, x, VLT_SPR, z, 3.0f, 2.0f, dark);
            }
        }
    }

    // --- 2) ARCOS FAJONES del CRUCERO: enmarcan el tramo central en |x| = 9 y |z| = 9 -------
    // 2 ejes x 2 lados x 4 tramos de 5.25u = 16 vigas (480 v).
    for (int a = 0; a < 2; ++a) {
        for (int s = -1; s <= 1; s += 2) {
            const float t = SEC_M0 * (float)s;          // +-9 = borde exacto del crucero
            for (int j = 0; j < VLT_FSEG; ++j) {
                const float o  = ((float)j - 1.5f) * fs;
                const float y0 = VLT_FAJ_Y[(j == 0 || j == VLT_FSEG - 1) ? 1 : 0];
                const float x  = (a == 0) ? o : t,       z = (a == 0) ? t : o;
                const float w  = (a == 0) ? fs : VLT_TH, d = (a == 0) ? VLT_TH : fs;
                addVaultBeamT(buf, i, x, y0, z, w, d, VLT_TOP - y0, rib);
            }
        }
    }

    // --- 3) ARRANQUES del crucero: en cada esquina (+-9, +-9) mueren 2 fajones + 1 diagonal --
    // 4 bloques (120 v) + 4 mensulas (48 v).
    for (int q = 0; q < 4; ++q) {
        const float sx = (q & 1) ? -1.0f : 1.0f, sz = (q & 2) ? -1.0f : 1.0f;
        const float x = 9.8f * sx, z = 9.8f * sz;
        addVaultBeamT(buf, i, x, VLT_SPR - 2.2f, z, 3.6f, 3.6f, 2.2f, dark);
        addCorbelT   (buf, i, x, VLT_SPR - 2.2f, z, 3.0f, 2.2f, dark);
    }

    // --- 4) NERVIOS DIAGONALES: de cada arranque a la clave (boveda de cruceria) ------------
    // 4 diagonales x 3 tramos de ~3.5u = 12 vigas (216 v). Partidas igual que las rectas.
    for (int q = 0; q < 4; ++q) {
        const float sx = (q & 1) ? -1.0f : 1.0f, sz = (q & 2) ? -1.0f : 1.0f;
        for (int p = 0; p < 3; ++p)
            addDiagRibT(buf, i,
                        VLT_DIAG[p    ][0] * sx, VLT_DIAG[p    ][2], VLT_DIAG[p    ][1] * sz,
                        VLT_DIAG[p + 1][0] * sx, VLT_DIAG[p + 1][2], VLT_DIAG[p + 1][1] * sz,
                        0.95f, VLT_TOP, rib);
    }

    // --- 5) CLAVE: bloque mayor y mas claro donde concurren las 4 diagonales (30 + 12 v) ----
    addVaultBeamT(buf, i, 0.0f, 57.6f, 0.0f, 5.0f, 5.0f, VLT_TOP - 57.6f, key);
    addCorbelT   (buf, i, 0.0f, 57.6f, 0.0f, 3.2f, 2.4f, brighten(key, 1.10f));

    // --- 6) LUCERNARIOS: uno por plemento del crucero (4 x 30 = 120 v) ----------------------
    // A +-5.8 del eje: no chocan ni con la clave (+-2.5) ni con los fajones (desde 8.1).
    for (int s = -1; s <= 1; s += 2) {
        addSkylightT(buf, i, 0.0f, 5.8f * (float)s, 2.0f, dark, glow);
        addSkylightT(buf, i, 5.8f * (float)s, 0.0f, 2.0f, dark, glow);
    }
}
