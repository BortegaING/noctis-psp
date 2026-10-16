// =============================================================================
// cuervo.h - EL CUERVO: el guia de cada piso
// Benjamin (2026-10-16): "un cuervo es el npc que te guia para hacer las cosas de cada piso".
// Esta posado en un tocon junto a donde apareces. TRIANGULO para hablarle: 3 consejos del piso
// (en el cementerio cambian cuando el rey despierta). Se incluye al FINAL de levels.h.
// =============================================================================
#ifndef CUERVO_H
#define CUERVO_H

static float g_cvX = 0.0f, g_cvY = 0.0f, g_cvZ = 0.0f;
static int   g_cvOn = 0, g_cvNear = 0, g_cvTalk = 0, g_cvLine = 0, g_cvCaw = 0;
static LineVertex __attribute__((aligned(16))) g_cvV[700];   // ~470 verts
static int   g_cvN = 0;
static LineVertex __attribute__((aligned(16))) g_cvStumpV[900];   // el tocon (era una caja)
static int   g_cvStumpN = 0;

// 3 consejos por piso (LVF_*)
static const char *kCvLines[LVF_LAST + 1][3] = {
    { "Cra. Al final del camino está el elevador.", "El robot oxidado vende armas. Tráele ecos.", "Los ecos los sueltan los que caen. Cra." },
    { "Cuando el cielo se apague, no corras: apunta.", "Mantén L. Los ojos no se esconden.", "R sin L: esquivas. Sus lágrimas te atraviesan." },
    { "Sigue el camino hasta la capilla. Cra.", "Si oyes ruido en el aire... no te detengas.", "Los ángeles no lloran por ti. Todavía." },   // (cementerio: bcCuervoLine)
    { "Arriba te pedirá que te rindas.", "Cuando brille, no lo mires.", "Si tu cuerpo deja de obedecerte... piensa en Dios." },
    { "Le encanta que le hablen de su perfume.", "Solo siente dolor mientras carga su fuego.", "Y cuando caiga... reza. Hasta el final." },
    { "No lo mires cuando resplandezca.", "Cuando caiga su lanza, busca la luz dorada.", "Él no baja. Tus balas sí suben." },
    { "Sus flechas te buscan donde vas a estar.", "Cambia de rumbo. Y si suena la trompeta...", "...mueve el análogo hasta despejarte." },
    { "Cuando brame, sal de su línea o sáltala.", "Si fallas su tajo, grita. Mueve el análogo.", "La guerra cansa. A él también." },
    { "La balanza castiga al que saca más.", "Salta sus ondas. Y ojo con sus monedas.", "El hambre no espera. Cra." },
    { "Mientras vuelen las moscas, la Muerte no siente.", "Derriba las moscas. Aquí el rosario no cura.", "No le des la espalda a la guadaña." },
};
static const char *cvLine(int k) {
    if (g_lvFloor == LVF_CEMETERY) return bcCuervoLine(k);   // cambian despues de la sirena
    return kCvLines[(g_lvFloor >= 0 && g_lvFloor <= LVF_LAST) ? g_lvFloor : 0][k];
}

// cuervo negro: cuerpo, cabeza, pico gris, ojos claros, alas y cola. Mira a -Z, patas en y=0
static void cvInit() {
    int i = 0; LineVertex *b = g_cvV;
    const unsigned int BK = RGBA(34, 34, 44, 255), BK2 = RGBA(56, 58, 74, 255), BEAK = RGBA(120, 118, 110, 255);
    addBall(b, i, 0.0f, 0.42f, 0.04f, 0.26f, 0.24f, 0.36f, 3, 7, BK);                     // cuerpo
    addBall(b, i, 0.0f, 0.70f, -0.22f, 0.16f, 0.16f, 0.17f, 3, 6, BK2);                   // cabeza
    addLimb(b, i, 0.0f, 0.68f, -0.36f, 0.0f, 0.64f, -0.62f, 0.06f, 0.01f, 4, BEAK, BEAK);   // pico
    addBall(b, i, -0.09f, 0.74f, -0.31f, 0.035f, 0.035f, 0.02f, 2, 4, RGBA(210, 200, 170, 255));   // ojos
    addBall(b, i, 0.09f, 0.74f, -0.31f, 0.035f, 0.035f, 0.02f, 2, 4, RGBA(210, 200, 170, 255));
    for (int s = -1; s <= 1; s += 2)                                                      // alas plegadas
        swQuadC(b, i, s * 0.24f, 0.55f, -0.10f, BK2, s * 0.30f, 0.45f, 0.40f, BK, s * 0.22f, 0.25f, 0.42f, BK, s * 0.22f, 0.28f, -0.08f, BK2);
    swQuadC(b, i, -0.10f, 0.36f, 0.34f, BK, 0.10f, 0.36f, 0.34f, BK, 0.14f, 0.30f, 0.72f, BK2, -0.14f, 0.30f, 0.72f, BK2);   // cola
    for (int s = -1; s <= 1; s += 2) addLimb(b, i, s * 0.08f, 0.22f, 0.02f, s * 0.08f, 0.0f, -0.02f, 0.02f, 0.015f, 3, BEAK, BEAK);   // patas
    g_cvN = i;
    // TOCON: corteza (tubo de 10 lados que se ensancha abajo), corte con anillos y raices
    i = 0; b = g_cvStumpV;
    const unsigned int BARK = RGBA(70, 56, 44, 255), BARKL = RGBA(98, 80, 62, 255), WOOD = RGBA(168, 134, 96, 255);
    gpTube(b, i, 0.0f, 0.0f, 0.0f, 0.0f, 1.2f, 0.0f, 0.56f, 0.46f, 10, cp_scale(BARK, 0.8f), BARKL);
    for (int k = 0; k < 10; ++k) {                                                  // el corte: anillos claros y oscuros
        const float a0 = 6.2831853f * k / 10, a1 = 6.2831853f * (k + 1) / 10;
        for (int r = 0; r < 3; ++r) {
            const float r0 = 0.46f * r / 3.0f, r1 = 0.46f * (r + 1) / 3.0f;
            const unsigned int c = (r & 1) ? cp_scale(WOOD, 0.82f) : WOOD;
            gpQuad(b, i, cosf(a0) * r0, 1.2f, sinf(a0) * r0, c, cosf(a1) * r0, 1.2f, sinf(a1) * r0, c,
                         cosf(a1) * r1, 1.2f, sinf(a1) * r1, c, cosf(a0) * r1, 1.2f, sinf(a0) * r1, c);
        }
    }
    for (int k = 0; k < 5; ++k) {                                                   // raices
        const float a = 6.2831853f * k / 5 + 0.4f;
        addLimb(b, i, cosf(a) * 0.38f, 0.32f, sinf(a) * 0.38f, cosf(a) * 0.95f, -0.04f, sinf(a) * 0.95f, 0.16f, 0.04f, 5, BARK, BARKL);
    }
    g_cvStumpN = i;
}
// junto a donde apareces, un poco adelante y del lado CONTRARIO al pilar de guardado (con
// TRIANGULO se pisaban). lvEnter ya puso g_lvSaveX cuando llama a esto.
static void cvEnterFloor(float sx, float sy, float sz) {
    g_cvOn = 1; g_cvTalk = 0; g_cvLine = 0; g_cvCaw = 0;
    const float side = (g_lvSaveX > sx) ? -1.0f : 1.0f;
    g_cvX = sx + side * 3.6f; g_cvY = sy; g_cvZ = sz - 2.4f;
    if (g_lvFloor == LVF_SKY) { g_cvX = sx - 4.5f; g_cvZ = sz - 1.0f; }
}
static int cvNear() { return g_cvOn && g_cvNear && !g_cvTalk && !g_lvNear; }   // guardar/elevador primero
static int cvBlocks() { return g_cvTalk; }
static void cvOpen() { g_cvTalk = 1; g_cvLine = 0; g_cvCaw = 20; }
// devuelve 1 mientras habla (consume los botones)
static int cvUpdate(float px, float py, float pz, int edge) {
    if (!g_cvOn) return 0;
    const float dx = px - g_cvX, dz = pz - g_cvZ;
    g_cvNear = (dx * dx + dz * dz < 3.2f * 3.2f && fabsf(py - g_cvY) < 3.0f);
    if (g_cvCaw > 0) --g_cvCaw;
    if (!g_cvTalk) return 0;
    if (edge & PSP_CTRL_CIRCLE) { g_cvTalk = 0; return 1; }
    if (edge & PSP_CTRL_CROSS) { if (++g_cvLine >= 3) g_cvTalk = 0; else g_cvCaw = 12; }
    return 1;
}
static void cvDraw(float px, float py, float pz) {
    if (!g_cvOn || g_menu) return;
    // el tocon donde se posa
    lvDrawAt(g_cvStumpV, g_cvStumpN, g_cvX, g_cvY, g_cvZ, 0.0f, 0.0f, 1.0f);
    float yaw, pitch; lvLookAngles(g_cvX, g_cvY + 2.0f, g_cvZ, px, py + 1.7f, pz, yaw, pitch);
    const float hop = (g_cvCaw > 0) ? 0.12f * fabsf(sinf(g_cvCaw * 0.6f)) : 0.02f * sinf(g_lvT * 0.05f);
    lvDrawAt(g_cvV, g_cvN, g_cvX, g_cvY + 1.2f + hop, g_cvZ, yaw, 0.0f, 1.7f);
}
static void cvHud() {
    if (!g_cvOn || g_menu || g_cine > 0) return;
    if (g_cvTalk) { bxDialog("EL CUERVO", cvLine(g_cvLine), 1); return; }
    if (cvNear() && !g_lvMenu) { fontTexOn(); drawText(150, 172, 1.0f, RGBA(230, 236, 246, 255), "TRIÁNGULO: HABLAR CON EL CUERVO"); sceGuDisable(GU_TEXTURE_2D); }
}

#endif // CUERVO_H
