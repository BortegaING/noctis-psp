#pragma once
// POCAS CATEDRALES, cada una UNICA (no una plantilla repetida). 4 landmarks
// monumentales rodean una plaza central; entre ellas se ve el VACIO/megaestructura.
// El jugador (1ra persona) esta acotado por un radio (~46) al centro; los muros de
// cada catedral lo frenan al acercarse a su base -> escala imponente al mirar arriba.
// El ORDEN importa: buildSolidWorld despacha por indice (0=Grand 1=Twin 2=Basilica 3=Bell).
static void buildCity() {
    int n = 0;
    // 0 GRAND: catedral principal al NORTE (la mas alta, west-front con torres gemelas)
    g_city[n++] = {   0.0f, -60.0f, 52.0f, 44.0f, 230.0f, RGBA(60, 57, 52, 255) };
    // 1 TWIN: catedral de agujas caladas al ESTE
    g_city[n++] = {  62.0f,   6.0f, 44.0f, 48.0f, 180.0f, RGBA(58, 55, 50, 255) };
    // 2 BASILICA: nave larga con arbotantes al OESTE (footprint alargado)
    g_city[n++] = { -60.0f,  10.0f, 48.0f, 72.0f, 150.0f, RGBA(56, 54, 49, 255) };
    // 3 BELL: campanario/torre del reloj al SUR
    g_city[n++] = {  10.0f,  60.0f, 40.0f, 40.0f, 200.0f, RGBA(59, 56, 51, 255) };
    g_cityCount = n;
}
