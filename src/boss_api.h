// ============================================================================
// boss_api.h -- CONTRATO de los pisos-jefe 3..7 (Baphomet y los 4 jinetes) + utilidades de UI
//
// levels.h incluye este archivo y DESPUES los modulos boss_*.h. Cada modulo define SUS
// funciones con su prefijo (bp_, bw_, br_, bk_, bv_) y levels.h las llama:
//
//   void  XX_Enter(BossIO &io, LineVertex *geo, int geoCap, int *geoN,
//                  CityBldg *boxes, int *nBoxes, int boxCap, float *sx, float *sy, float *sz);
//         arma el escenario (geo: se dibuja con niebla XX_Fog) + colision (CityBldg = columna
//         desde y=0 hasta h; el SUELO tambien es una caja) + spawn del jugador. Reinicia estado.
//   void  XX_Update(BossIO &io);     // logica por frame (dialogos, ataques, minijuegos)
//   void  XX_Draw(float px, float py, float pz);   // 3D extra: jefe, enemigos, efectos
//   void  XX_Hud(BossIO &io);        // 2D: dialogos, menus, barras (usa los bx* de abajo)
//   void  XX_Backdrop();             // cielo 2D (gradQuad / lvFan2D); se llama sin Z
//   unsigned int XX_Fog(float *nearF, float *farF);   // niebla del escenario
//   int   XX_AimTarget(float out[3]);                 // blanco del autoapuntado (0 = no hay)
//   bool  XX_ShotHit(float x, float y, float z, int dmg);   // la bala toco algo suyo?
//   int   XX_Blocks();               // 1 = el jugador no se mueve ni dispara (dialogo/menu)
//   int   XX_Done();                 // 1 = jefe vencido: levels.h abre el elevador
//   float XX_Shake();                // temblor de camara (0 = nada)
//
// REGLAS:
//  - Todo se dibuja SIN cull. LINE_FLAGS + LineVertex {color,x,y,z}. Sin texturas.
//  - Si g_lvTimeStop > 0 (poder de Baphomet) los enemigos y sus proyectiles NO se mueven.
//  - Danio al jugador: io.hp -= n; (levels.h maneja la muerte y el respawn en el piso).
//  - DASH: si g_lvDashInv > 0 el jugador esta en pleno dash. Los golpes que se ESQUIVAN
//    (proyectiles, ondas, embestidas, zonas, contacto) no le hacen nada y el proyectil lo
//    atraviesa. El danio de MECANICA (minijuegos, quemaduras, estados) si le llega.
//  - Mensajes grandes: lvMessage(texto, frames). Destello blanco: g_lvFlash = frames.
//  - Proyectil enemigo generico: lvFireOrb(x,y,z, tx,ty,tz, velocidad, color) (5 de danio,
//    lo mueve/dibuja levels.h, ya respeta la parada del tiempo).
//  - Textos en UTF-8: drawText dibuja tildes y enie (a, e, i, o, u, n y mayusculas, signos).
//  - El jugador mide 3.45; salta ~8.6 (gravedad lunar). Pantalla 480x272.
// ============================================================================

// ---------------------------------------------------------------- contexto por frame
struct BossIO {
    float &px, &py, &pz, &vy, &hp;   // jugador (se puede mover/herir/curar)
    float hpMax;
    int   grounded;
    float &camYaw;                   // fijar camara (yaw: adelante = (sin, -cos))
    int   btn, edge;                 // botones crudos y FLANCOS de este frame (PSP_CTRL_*)
    float stickX, stickY;            // -1..1 (stickY + = abajo, como el Ly de la PSP)
    int   aiming;                    // L apretado
};

// ---------------------------------------------------------------- estado comun
static int g_lvTimeStop = 0;         // frames de tiempo detenido (poder de Baphomet)
static int g_lvPowerTime = 0;        // 1 = el jugador tiene el poder de detener el tiempo
static int g_lvGiveTimePower = 0;    // un modulo pone 1 para otorgarlo (levels.h lo guarda)
static int g_lvNoHeal = 0;           // 1 = el rosario no cura en este piso (jinete palido)
static int g_lvDashInv = 0;          // frames de DASH que le quedan al jugador (main lo pone)
static int g_lvNoFallDeath = 0;      // 1 = el jugador puede caer bajo el piso sin "morir" (caidas guionadas)

// ---------------------------------------------------------------- helpers de levels.h
// (definidos mas abajo en levels.h; aqui solo declarados para que los modulos los usen)
static void  lvMessage(const char *m, int frames);
// cinematica de jefe a medida: la camara gira alrededor de (fx,fy,fz); rk escala la distancia,
// zm cuanto se acerca (1 = a la mitad), h0 sube/baja la camara. Mientras dura, XX_Update NO corre
// (animar con g_lvT, que si avanza). Titulo y subtitulo en la tarjeta.
static void  lvCineBoss(float fx, float fy, float fz, float rk, float zm, float h0, const char *title, const char *sub);
static void  lvFireOrb(float x, float y, float z, float tx, float ty, float tz, float spd, unsigned int col);
static void  lvDrawAt(LineVertex *v, int n, float x, float y, float z, float yaw, float pitch, float scale, float roll);
static void  lvLookAngles(float x, float y, float z, float tx, float ty, float tz, float &yaw, float &pitch);
static void  lvBox(float x, float y, float z, float w, float h, unsigned int col);          // caja iluminada
static void  lvRing3D(float cx, float y, float cz, float R, float w, unsigned int col);     // anillo plano
static void  lvBeam(float cx, float y, float cz, float a, float len, float h, unsigned int col); // rayo (x=sin a, z=cos a)
static void  lvFan2D(int cx, int cy, int r, unsigned int cIn, unsigned int cOut, int seg);  // abanico 2D (resplandor)
static int   lvProject(float x, float y, float z, int &sx, int &sy);                      // 3D -> pantalla
static unsigned int lvMix(unsigned int a, unsigned int b, float t);
// globales de levels.h disponibles: g_lvT (frames en el piso), g_lvFlash, g_lvEye[3]/g_lvFwd[3]
// (camara: para "lo estas mirando?"), g_lvRight[3]/g_lvUp[3].

// ---------------------------------------------------------------- UI comun (2D)
// caja de dialogo inferior: nombre (dorado) + linea; "X >" si next
static void bxDialog(const char *who, const char *line, int next) {
    sceGuDisable(GU_TEXTURE_2D);
    drawRect(30, 186, 420, 66, RGBA(8, 10, 16, 215));
    drawRect(30, 186, 420, 1, RGBA(200, 176, 120, 200));
    fontTexOn();
    drawText(40, 194, 1.0f, RGBA(236, 206, 120, 255), who);
    drawText(40, 212, 1.0f, RGBA(240, 240, 246, 255), line);
    if (next) drawText(392, 236, 1.0f, RGBA(160, 170, 190, 255), "X >");
    sceGuDisable(GU_TEXTURE_2D);
}
// lista de opciones (hasta 8) en una caja; sel resaltada
static void bxChoices(const char *title, const char **opts, int n, int sel) {
    const int h = 26 + n * 14, y0 = 136 - h / 2;
    sceGuDisable(GU_TEXTURE_2D);
    drawRect(70, y0, 340, h, RGBA(8, 10, 16, 225));
    drawRect(70, y0, 340, 1, RGBA(200, 176, 120, 200));
    fontTexOn();
    if (title) drawText(80, y0 + 6, 1.0f, RGBA(236, 206, 120, 255), title);
    for (int k = 0; k < n; ++k)
        drawText(84, y0 + 22 + k * 14, 1.0f, k == sel ? RGBA(255, 255, 255, 255) : RGBA(140, 146, 160, 255), opts[k]);
    if (sel >= 0 && sel < n) drawText(74, y0 + 22 + sel * 14, 1.0f, RGBA(255, 220, 140, 255), ">");
    sceGuDisable(GU_TEXTURE_2D);
}
// barra de vida del jefe (arriba al centro)
static void bxBossBar(const char *name, int hp, int hpMax) {
    if (hpMax <= 0) return;
    if (hp < 0) hp = 0;
    sceGuDisable(GU_TEXTURE_2D);
    drawRect(120, 10, 240, 16, RGBA(8, 10, 16, 200));
    drawRect(124, 21, 232, 3, RGBA(40, 20, 24, 255));
    drawRect(124, 21, 232 * hp / hpMax, 3, RGBA(210, 60, 60, 255));
    fontTexOn();
    drawText(240 - textLen(name) * 4, 11, 1.0f, RGBA(236, 230, 220, 255), name);
    sceGuDisable(GU_TEXTURE_2D);
}
// "MUEVE EL ANALOGO": se llena con el recorrido del stick
struct BxMash { float acc, lx, ly; int frames; };
static void bxMashStart(BxMash &m, int frames) { m.acc = 0.0f; m.lx = m.ly = 0.0f; m.frames = frames; }
// devuelve 1 cuando se libero (acc >= need), -1 si se acabo el tiempo, 0 si sigue
static int bxMashUpdate(BxMash &m, float sx, float sy, float need) {
    m.acc += sqrtf((sx - m.lx) * (sx - m.lx) + (sy - m.ly) * (sy - m.ly));
    m.lx = sx; m.ly = sy;
    if (m.acc >= need) { m.frames = 0; return 1; }
    if (m.frames > 0 && --m.frames == 0) return -1;
    return 0;
}
static void bxMashHud(const BxMash &m, float need, const char *label) {
    if (m.frames <= 0) return;
    float f = m.acc / need; if (f > 1.0f) f = 1.0f;
    sceGuDisable(GU_TEXTURE_2D);
    drawRect(140, 150, 200, 8, RGBA(30, 30, 40, 220));
    drawRect(140, 150, (int)(200.0f * f), 8, RGBA(170, 140, 255, 240));
    fontTexOn();
    drawText(240 - textLen(label) * 4, 136, 1.0f, RGBA(230, 220, 255, 255), label);
    sceGuDisable(GU_TEXTURE_2D);
}
// velo de color a pantalla completa (tintes, destellos)
static void bxTint(unsigned int col) { sceGuDisable(GU_TEXTURE_2D); drawRect(0, 0, SCR_WIDTH, SCR_HEIGHT, col); }
