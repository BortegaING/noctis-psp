#pragma once
// BLAME brutalista: POCOS monolitos ENORMES y muy SEPARADOS (r 150..320) -> cada uno ocupa
// poca pantalla (fill barato) y se ven de lejos en la niebla. Cajas simples (industrial tex).
static void buildCity() {
    int n = 0;
    g_city[n++] = {    0.0f, -220.0f, 70.0f, 70.0f, 380.0f, RGBA(64, 70, 82, 255) };
    g_city[n++] = {  190.0f, -120.0f, 50.0f, 50.0f, 260.0f, RGBA(62, 68, 80, 255) };
    g_city[n++] = {  240.0f,   90.0f, 80.0f, 40.0f, 320.0f, RGBA(60, 66, 78, 255) };
    g_city[n++] = {   60.0f,  230.0f, 45.0f, 45.0f, 200.0f, RGBA(64, 70, 82, 255) };
    g_city[n++] = { -170.0f,  200.0f, 60.0f, 60.0f, 300.0f, RGBA(62, 68, 80, 255) };
    g_city[n++] = { -250.0f,  -40.0f, 40.0f, 90.0f, 240.0f, RGBA(60, 66, 78, 255) };
    g_city[n++] = { -150.0f, -200.0f, 55.0f, 55.0f, 350.0f, RGBA(64, 70, 82, 255) };
    g_city[n++] = {  120.0f, -260.0f, 35.0f, 35.0f, 180.0f, RGBA(62, 68, 80, 255) };
    g_cityCount = n;
}
