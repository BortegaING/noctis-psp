#pragma once
// PUEBLO minimo: SOLO 3 edificios (borrados varios para el FPS). Frente (-Z)
// abierto al mar de agujas; 2 catedrales + 1 casona enmarcan el balcon.
static void buildCity() {
    int n = 0;
    g_city[n++] = {  50.0f, -64.0f, 30.0f, 30.0f, 150.0f, RGBA(60, 57, 52, 255) }; // catedral derecha
    g_city[n++] = { -54.0f, -76.0f, 26.0f, 26.0f, 120.0f, RGBA(58, 55, 50, 255) }; // catedral izquierda
    g_city[n++] = { -42.0f, -14.0f, 22.0f, 20.0f,  74.0f, RGBA(56, 53, 48, 255) }; // casona izq
    g_cityCount = n;
}
