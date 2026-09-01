#pragma once
// PUEBLO gotico sobre un balcon que MIRA el mar de agujas: el frente (-Z) queda
// ABIERTO (sin edificios en |x|<24, z<-18) para ver el bosque de agujas al fondo;
// las catedrales y casonas enmarcan a los LADOS. Aura Bloodborne (referencia).
static void buildCity() {
    int n = 0;
    // --- 2 catedrales landmark, a los lados-frente (enmarcan la vista) ---
    g_city[n++] = {  52.0f,  -66.0f, 30.0f, 30.0f, 164.0f, RGBA(64, 60, 54, 255) }; // catedral derecha
    g_city[n++] = { -56.0f,  -80.0f, 26.0f, 26.0f, 132.0f, RGBA(62, 58, 52, 255) }; // catedral izquierda
    // --- casonas/mansiones a los lados ---
    g_city[n++] = { -44.0f,  -14.0f, 20.0f, 18.0f,  80.0f, RGBA(60, 56, 50, 255) };
    g_city[n++] = {  46.0f,  -16.0f, 18.0f, 20.0f,  68.0f, RGBA(58, 54, 48, 255) };
    g_city[n++] = { -30.0f,   24.0f, 15.0f, 15.0f,  46.0f, RGBA(66, 58, 52, 255) }; // capilla (algo calida)
    g_city[n++] = {  34.0f,   28.0f, 18.0f, 16.0f,  58.0f, RGBA(60, 56, 50, 255) };
    g_city[n++] = { -62.0f,   30.0f, 16.0f, 18.0f,  54.0f, RGBA(58, 54, 48, 255) };
    g_city[n++] = {  72.0f,    8.0f, 18.0f, 18.0f,  62.0f, RGBA(60, 56, 50, 255) };
    g_city[n++] = { -72.0f,  -50.0f, 18.0f, 20.0f,  74.0f, RGBA(59, 55, 49, 255) };
    g_city[n++] = {  76.0f,  -42.0f, 20.0f, 18.0f,  72.0f, RGBA(61, 57, 51, 255) };
    g_cityCount = n;
}
