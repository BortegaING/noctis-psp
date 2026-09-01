#pragma once
// PUEBLO gotico chico: SOLO 5 edificios (menos = mas fluido). Frente (-Z) ABIERTO
// para ver el mar de agujas; catedrales y casonas enmarcan a los lados.
static void buildCity() {
    int n = 0;
    g_city[n++] = {  52.0f,  -66.0f, 30.0f, 30.0f, 164.0f, RGBA(60, 57, 52, 255) }; // catedral derecha
    g_city[n++] = { -56.0f,  -80.0f, 26.0f, 26.0f, 132.0f, RGBA(58, 55, 50, 255) }; // catedral izquierda
    g_city[n++] = { -44.0f,  -14.0f, 22.0f, 20.0f,  84.0f, RGBA(56, 53, 48, 255) }; // casona izq
    g_city[n++] = {  46.0f,  -16.0f, 20.0f, 22.0f,  70.0f, RGBA(54, 51, 46, 255) }; // casona der
    g_city[n++] = {  38.0f,   30.0f, 18.0f, 18.0f,  56.0f, RGBA(56, 53, 48, 255) }; // capilla der-atras
    g_cityCount = n;
}
