#pragma once
// EL POZO: las catedrales son HITOS contra los muros lejanos del pozo (radio ~160),
// cada una sobre su propia plataforma, AL OTRO LADO DEL VACIO respecto del balcon
// del jugador (que esta en el muro +Z, z~+118, mirando a -Z).
// El ORDEN importa: buildSolidWorld despacha por indice (0=Grand 1=Twin 2=Basilica 3=Bell).
static void buildCity() {
    int n = 0;
    // 0 GRAND: justo ENFRENTE del balcon, contra el muro -Z, a 236u a traves del abismo
    g_city[n++] = {   0.0f, -118.0f, 52.0f, 44.0f, 230.0f, RGBA(60, 57, 52, 255) };
    // 1 TWIN: muro +X (a la izquierda del jugador que mira -Z... derecha en pantalla)
    g_city[n++] = { 118.0f,  -10.0f, 44.0f, 48.0f, 180.0f, RGBA(58, 55, 50, 255) };
    // 2 BASILICA: muro -X (alargada a lo largo del muro)
    g_city[n++] = {-116.0f,    8.0f, 48.0f, 72.0f, 150.0f, RGBA(56, 54, 49, 255) };
    // 3 BELL: en el MISMO muro +Z que el balcon, justo al lado -> escala brutal de cerca
    g_city[n++] = {  58.0f,  118.0f, 40.0f, 40.0f, 200.0f, RGBA(59, 56, 51, 255) };
    g_cityCount = n;
}
