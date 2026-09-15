#pragma once
// ============================================================================
// PROJECT NOCTIS - hud.h
// EL HUD, en un solo sitio y diciendo la VERDAD. Reemplaza al bloque suelto
// "---------- HUD (2D) ----------" de main.cpp.
//
// Que cambia respecto del viejo:
//   - El MINIMAPA ya no pinta kStructures[] (el distrito del mundo VIEJO, que
//     ya no existe): ahora dibuja EL SECTOR REAL de sector.h (4 masas + marco)
//     y al jugador como flecha orientada. Ademas mide 56x56 en vez de 106x106.
//   - FUERA la linea de ayuda de 66 caracteres cada frame (66 sprites
//     texturizados + mezcla). Queda una ayuda de 30 caracteres y SOLO los
//     primeros segundos (st.helpTimer).
//   - ENTRA el bucle de juego: contador de materiales con barra de progreso y
//     aviso temporal al recoger uno.
//
// ---------------------------------------------------------------------------
// DEPENDENCIAS (que tiene que estar YA definido donde se incluya)
//   RGBA(), SCR_WIDTH, SCR_HEIGHT, drawRect(), drawText(), fontTexOn()
//   y <pspgu.h> (por sceGuDisable/GU_TEXTURE_2D).
// drawRect/drawText/fontTexOn viven en main.cpp ~linea 770-845, asi que este
// archivo se incluye DESPUES de fontTexOn(), NO arriba con los otros headers.
//
// ESTADO DE GU QUE ESPERA (lo pone el que llama, igual que hoy):
//   sceGuDisable(GU_DEPTH_TEST); sceGuEnable(GU_BLEND);
//   sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
// hudDraw() solo conmuta GU_TEXTURE_2D (apagada para los rectangulos, encendida
// para el texto) y la deja APAGADA al salir. No toca profundidad ni mezcla.
//
// REGLAS DE LA CASA (respetadas): sin heap, sin rand, sin I/O (nada de stdio:
// los numeros se formatean a mano), C++17, <math.h> como unico include.
// ============================================================================
#include <math.h>

// ============================== ESTADO ======================================
// Lo rellena main.cpp cada frame. Todos los campos tienen valor por defecto:
// se puede hacer "HudState st{};" y asignar solo lo que interese.
struct HudState {
    // --- vitales (arriba izquierda) ---
    float hp     = 100.0f, hpMax = 100.0f;   // hp / HP_MAX
    float en     = 100.0f, enMax = 100.0f;   // en / EN_MAX
    float grv    = 1.0f;                     // 0..1; si no hay medidor, dejar 1
    const char *gravName = "ABAJO";          // kGravName[gravG]

    // --- minimapa (arriba derecha) ---
    float px = 0.0f, pz = 0.0f;              // playerX, playerZ (mundo)
    float yaw = 0.0f;                        // heroYaw: adelante = (sin,-cos)
    const char *district = "CAMPANARIO";     // nombre bajo el minimapa

    // --- objetivo (abajo izquierda) ---
    int  matTaken = 0, matTotal = 10;        // objMaterialsTaken()/objMaterialCount()
    int  pickTimer = 0;                      // frames de aviso (0 = sin aviso)
    const char *pickName = nullptr;          // kMaterials[pickedType].name

    // --- arma (abajo derecha) ---
    const char *weaponName = "";             // kRanged[curRanged].name
    int  ammo = 0, ammoMax = 0;              // ammoMag[i] / kRanged[i].magazine
    bool reloading = false;                  // reloadCD > 0

    // --- extras ---
    int  fps = -1;                           // <0 = no dibujarlo
    int  helpTimer = 0;                      // frames de ayuda corta (0 = nada)
    bool aiming = false;                     // mira
    bool paused = false;                     // panel de pausa
};

// ============================ PALETA (sobria) ===============================
static const unsigned int HUD_C_PANEL = RGBA(  9,  10,  14, 150);
static const unsigned int HUD_C_FRAME = RGBA(118, 130, 156, 120);
static const unsigned int HUD_C_DIM   = RGBA(140, 150, 174, 255);
static const unsigned int HUD_C_TEXT  = RGBA(222, 220, 228, 255);
static const unsigned int HUD_C_WARN  = RGBA(236, 176,  86, 255);
static const unsigned int HUD_C_GOOD  = RGBA(126, 214, 150, 255);
static const unsigned int HUD_C_ACC   = RGBA(238, 146,  84, 255);

// =========================== GEOMETRIA DEL HUD ==============================
static const int HUD_BAR_H  = 6;
static const int HUD_SP_X   = 6,  HUD_SP_Y = 6, HUD_SP_W = 152, HUD_SP_H = 40;
static const int HUD_BAR_X  = 36, HUD_BAR_W = 68, HUD_VAL_X = 108;
static const int HUD_MM     = 56;                              // lado del minimapa
static const int HUD_MM_X   = SCR_WIDTH - 8 - HUD_MM;          // 416
static const int HUD_MM_Y   = 8;
static const int HUD_WP_W   = 148, HUD_WP_H = 20;
static const int HUD_WP_X   = SCR_WIDTH  - 8 - HUD_WP_W;       // 324
static const int HUD_WP_Y   = SCR_HEIGHT - 8 - HUD_WP_H;       // 244

// --- el SECTOR, replicado de sector.h (si se tocan los SEC_*, actualizar) ---
// recinto: muros exteriores en |x|,|z| = 56 ; 4 masas de 36x36 entre 9 y 45.
static const float HUD_W_HALF = 56.0f;                 // media anchura del mundo
static const float HUD_W_M0   =  9.0f, HUD_W_M1 = 45.0f;

// ============================ UTILIDADES ====================================
// Entero -> texto sin stdio. Devuelve cuantos caracteres escribio.
static int hudUInt(char *d, int v) {
    if (v < 0) v = 0;
    char t[12]; int n = 0;
    do { t[n++] = (char)('0' + (v % 10)); v /= 10; } while (v > 0 && n < 11);
    for (int k = 0; k < n; ++k) d[k] = t[n - 1 - k];
    d[n] = 0;
    return n;
}
// Copia acotada (trunca nombres largos: cada caracter es un sprite mezclado).
static int hudPut(char *d, int at, const char *s, int maxc) {
    int n = 0;
    if (s) while (s[n] && n < maxc) { d[at + n] = s[n]; ++n; }
    d[at + n] = 0;
    return at + n;
}
static int hudLen(const char *s, int maxc) {
    int n = 0;
    if (s) while (s[n] && n < maxc) ++n;
    return n;
}
// Texto pegado a la derecha de un borde (para la municion).
static void hudTextR(int right, int y, unsigned int c, const char *s) {
    drawText(right - hudLen(s, 64) * 8, y, 1.0f, c, s);
}
// Mundo -> pixel del minimapa. escala = 56 px / 112 u = 0.5 px por unidad.
static int hudMM(int origin, float v) {
    float t = (v + HUD_W_HALF) * ((float)HUD_MM / (2.0f * HUD_W_HALF));
    if (t < 0.0f) t = 0.0f;
    if (t > (float)HUD_MM) t = (float)HUD_MM;
    return origin + (int)t;
}
// Barra con marco: 3 rectangulos (marco = un rect 1px mas grande por detras).
static void hudBar(int y, float frac, unsigned int fill, unsigned int back) {
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;
    drawRect(HUD_BAR_X - 1, y - 1, HUD_BAR_W + 2, HUD_BAR_H + 2, HUD_C_FRAME);
    drawRect(HUD_BAR_X, y, HUD_BAR_W, HUD_BAR_H, back);
    int w = (int)((float)HUD_BAR_W * frac + 0.5f);
    if (w > 0) drawRect(HUD_BAR_X, y, w, HUD_BAR_H, fill);
}
// Desvanecido de los avisos temporales: mantiene el alfa base y solo lo apaga
// en los ultimos 16 frames (asi el panel no se vuelve opaco de golpe).
static unsigned int hudFade(unsigned int col, int timer) {
    int a = (int)((col >> 24) & 0xFFu);
    if (timer < 0)  timer = 0;
    if (timer < 16) a = a * timer / 16;
    return (col & 0x00FFFFFFu) | ((unsigned int)a << 24);
}

// ============================== DIBUJO ======================================
// Dos fases a proposito: PRIMERO todos los rectangulos (textura apagada) y
// DESPUES todo el texto (textura de fuente encendida). Un solo cambio de
// estado de textura por frame en vez de ir alternando.
static void hudDraw(const HudState &st) {
    const int r0 = 11, r1 = 22, r2 = 33;              // filas de las 3 barras
    const float hpF = (st.hpMax > 0.0f) ? st.hp / st.hpMax : 0.0f;
    const float enF = (st.enMax > 0.0f) ? st.en / st.enMax : 0.0f;
    const int   matN = (st.matTotal > 0) ? st.matTotal : 1;
    const int   pickN = 2 + hudLen(st.pickName, 18);  // "+ " + nombre truncado
    char b[48];

    // ------------------------- FASE 1: rectangulos -------------------------
    sceGuDisable(GU_TEXTURE_2D);

    // --- vitales (arriba izquierda) ---
    drawRect(HUD_SP_X, HUD_SP_Y, HUD_SP_W, HUD_SP_H, HUD_C_PANEL);
    hudBar(r0, hpF,    RGBA(198,  62,  64, 255), RGBA(44, 18, 20, 190));
    hudBar(r1, enF,    RGBA( 72, 142, 208, 255), RGBA(16, 28, 44, 190));
    hudBar(r2, st.grv, RGBA(158, 120, 210, 255), RGBA(28, 20, 42, 190));

    // --- minimapa REAL del sector (arriba derecha) ---
    // fondo + marco + las 4 masas de 36x36. El pasillo en cruz, el perimetral
    // y los ventanales son el HUECO: no cuestan un solo pixel.
    drawRect(HUD_MM_X, HUD_MM_Y, HUD_MM, HUD_MM, HUD_C_PANEL);
    drawRect(HUD_MM_X, HUD_MM_Y, HUD_MM, 1, HUD_C_FRAME);                 // marco N
    drawRect(HUD_MM_X, HUD_MM_Y + HUD_MM - 1, HUD_MM, 1, HUD_C_FRAME);    // marco S
    drawRect(HUD_MM_X, HUD_MM_Y + 1, 1, HUD_MM - 2, HUD_C_FRAME);         // marco O
    drawRect(HUD_MM_X + HUD_MM - 1, HUD_MM_Y + 1, 1, HUD_MM - 2, HUD_C_FRAME); // marco E
    {
        const unsigned int mass = RGBA(74, 82, 100, 210);
        for (int q = 0; q < 4; ++q) {
            const bool nx = (q & 1) != 0, nz = (q & 2) != 0;   // cuadrante
            const int x0 = hudMM(HUD_MM_X, nx ? -HUD_W_M1 : HUD_W_M0);
            const int z0 = hudMM(HUD_MM_Y, nz ? -HUD_W_M1 : HUD_W_M0);
            const int x1 = hudMM(HUD_MM_X, nx ? -HUD_W_M0 : HUD_W_M1);
            const int z1 = hudMM(HUD_MM_Y, nz ? -HUD_W_M0 : HUD_W_M1);
            drawRect(x0, z0, x1 - x0, z1 - z0, mass);                     // 18x18 px
        }
        // jugador: cuerpo + morro en la direccion de la mirada (adelante en
        // mundo = (sin yaw, -cos yaw); +z del mundo baja en pantalla).
        const int  bx = hudMM(HUD_MM_X, st.px), bz = hudMM(HUD_MM_Y, st.pz);
        const float dx = sinf(st.yaw), dz = -cosf(st.yaw);
        drawRect(bx - 1, bz - 1, 3, 3, HUD_C_ACC);
        drawRect(bx + (int)(dx * 4.0f) - 1, bz + (int)(dz * 4.0f) - 1, 2, 2,
                 RGBA(255, 244, 232, 255));
    }

    // --- objetivo (abajo izquierda) + arma (abajo derecha) ---
    // 80 px de ancho = "MAT 10/10" (9 caracteres) + margen.
    drawRect(HUD_SP_X, HUD_WP_Y, 80, HUD_WP_H, HUD_C_PANEL);
    drawRect(10, HUD_WP_Y + 13, 72, 3, RGBA(22, 34, 28, 210));            // progreso
    {
        int w = (int)(72.0f * (float)st.matTaken / (float)matN + 0.5f);
        if (w > 0) drawRect(10, HUD_WP_Y + 13, w, 3, HUD_C_GOOD);
    }
    drawRect(HUD_WP_X, HUD_WP_Y, HUD_WP_W, HUD_WP_H, HUD_C_PANEL);

    // --- aviso temporal de objeto obtenido (solo mientras dura) ---
    if (st.pickTimer > 0 && st.pickName)
        drawRect(HUD_SP_X, 224, 8 + pickN * 8, 14, hudFade(HUD_C_PANEL, st.pickTimer));

    // --- mira (apuntando con L) ---
    if (st.aiming) {
        const unsigned int rc = RGBA(255, 90, 80, 235);
        drawRect(239, 127, 2, 7, rc);
        drawRect(239, 139, 2, 7, rc);
        drawRect(231, 135, 7, 2, rc);
        drawRect(243, 135, 7, 2, rc);
        drawRect(239, 135, 2, 2, RGBA(255, 255, 255, 255));
    }
    if (st.paused) drawRect(150, 88, 180, 64, RGBA(10, 10, 16, 205));

    // ---------------------------- FASE 2: texto ----------------------------
    fontTexOn();

    // --- vitales: etiqueta + valor numerico ---
    drawText(10, r0 - 1, 1.0f, RGBA(216, 116, 122, 255), "HP");
    drawText(10, r1 - 1, 1.0f, RGBA(124, 168, 226, 255), "EN");
    drawText(10, r2 - 1, 1.0f, RGBA(180, 148, 226, 255), "GRV");
    hudUInt(b, (int)(st.hp + 0.5f));
    drawText(HUD_VAL_X, r0 - 1, 1.0f, HUD_C_TEXT, b);
    hudUInt(b, (int)(st.en + 0.5f));
    drawText(HUD_VAL_X, r1 - 1, 1.0f, HUD_C_TEXT, b);
    hudPut(b, 0, st.gravName ? st.gravName : "-", 6);
    drawText(HUD_VAL_X, r2 - 1, 1.0f, HUD_C_TEXT, b);
    if (st.fps >= 0) {                                 // discreto, bajo el panel
        int n = hudPut(b, 0, "FPS ", 4);
        hudUInt(b + n, st.fps);
        drawText(HUD_SP_X + 2, 50, 1.0f, RGBA(96, 106, 128, 255), b);
    }

    // --- nombre del distrito, bajo el minimapa y alineado a su borde ---
    hudPut(b, 0, st.district ? st.district : "", 12);
    hudTextR(HUD_MM_X + HUD_MM, HUD_MM_Y + HUD_MM + 4, HUD_C_DIM, b);

    // --- materiales: "MAT 3/10" ---
    {
        int n = hudPut(b, 0, "MAT ", 4);
        n += hudUInt(b + n, st.matTaken);
        n = hudPut(b, n, "/", 1);
        hudUInt(b + n, st.matTotal);
        drawText(10, HUD_WP_Y + 3, 1.0f, HUD_C_GOOD, b);
    }

    // --- arma + municion (nombre truncado: los hay de 23 caracteres) ---
    hudPut(b, 0, st.weaponName, 17);
    drawText(HUD_WP_X + 4, HUD_WP_Y + 2, 1.0f, RGBA(206, 198, 180, 255), b);
    if (st.reloading) {
        hudTextR(HUD_WP_X + HUD_WP_W - 4, HUD_WP_Y + 11, HUD_C_WARN, "RECARGANDO");
    } else {
        int n = hudUInt(b, st.ammo);
        n = hudPut(b, n, "/", 1);
        hudUInt(b + n, st.ammoMax);
        const bool low = (st.ammoMax > 0 && st.ammo * 4 <= st.ammoMax);  // cargador al 25%
        hudTextR(HUD_WP_X + HUD_WP_W - 4, HUD_WP_Y + 11,
                 low ? RGBA(236, 120, 104, 255) : HUD_C_TEXT, b);
    }

    // --- aviso de objeto obtenido ---
    if (st.pickTimer > 0 && st.pickName) {
        int n = hudPut(b, 0, "+ ", 2);
        hudPut(b, n, st.pickName, 18);
        drawText(10, 227, 1.0f, hudFade(HUD_C_GOOD, st.pickTimer), b);
    }

    // --- ayuda CORTA y solo los primeros segundos (antes: 66 chars/frame) ---
    if (st.helpTimer > 0)
        drawText(120, 208, 1.0f, hudFade(HUD_C_DIM, st.helpTimer),
                 "TRI gravedad  X salto  O melee");

    if (st.paused) {
        drawText(206, 104, 2.0f, RGBA(232, 222, 242, 255), "PAUSA");
        drawText(163, 130, 1.0f, RGBA(165, 175, 205, 255), "START continuar   HOME salir");
    }

    sceGuDisable(GU_TEXTURE_2D);
}

// ============================================================================
// NOTA DE COSTE (por frame, en pixeles MEZCLADOS; el fill es el cuello de la PSP)
//   arriba izq  panel 152x40 + 3 barras con marco .......... ~10.200 px
//   arriba der  minimapa 56x56 + marco + 4 masas + jugador .. ~4.700 px
//   abajo       panel objetivo 80x20 + panel arma 148x20 ..... ~4.600 px
//   texto       ~55 caracteres (era ~176) = 55 * 64 ......... ~3.500 px
//   TOTAL ~22.800 px  (el viejo: ~50.000 px, incluida la banda gradQuad(0,32)
//   de 15.360 px y el minimapa opaco de ~21.000 px) => algo menos de la MITAD.
// Al cablear: borrar el gradQuad(0,32,...) del bloque HUD viejo (los paneles ya
// dan contraste local) y la linea de ayuda de 66 caracteres.
// ============================================================================
