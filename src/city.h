#pragma once
// PUEBLO gotico caminable: ~12 edificios BIEN detallados repartidos sobre suelo
// solido. Pocos a la vista a la vez (culling), pero al CAMINAR van apareciendo.
// Plaza al centro (spawn). Aura Bloodborne (gotico), sin vacio.
static void buildCity() {
    int n = 0;
    // --- landmark: catedral, justo al frente (-Z) ---
    g_city[n++] = {   0.0f,  -56.0f, 30.0f, 30.0f, 162.0f, RGBA(42, 44, 60, 255) };
    // --- anillo interior alrededor de la plaza ---
    g_city[n++] = { -40.0f,  -20.0f, 20.0f, 18.0f,  78.0f, RGBA(44, 46, 62, 255) };
    g_city[n++] = {  42.0f,  -30.0f, 18.0f, 20.0f,  66.0f, RGBA(40, 42, 56, 255) };
    g_city[n++] = { -20.0f,   -6.0f, 15.0f, 15.0f,  44.0f, RGBA(52, 46, 50, 255) }; // capilla (algo calida)
    g_city[n++] = {  40.0f,   22.0f, 18.0f, 16.0f,  58.0f, RGBA(43, 45, 60, 255) };
    g_city[n++] = { -46.0f,   26.0f, 16.0f, 18.0f,  54.0f, RGBA(41, 43, 58, 255) };
    // --- anillo exterior (aparecen al caminar) ---
    g_city[n++] = {  62.0f,  -60.0f, 22.0f, 20.0f,  96.0f, RGBA(44, 46, 62, 255) }; // torre alta
    g_city[n++] = { -66.0f,  -48.0f, 18.0f, 20.0f,  72.0f, RGBA(40, 42, 56, 255) };
    g_city[n++] = {  20.0f,   62.0f, 18.0f, 18.0f,  50.0f, RGBA(43, 45, 60, 255) };
    g_city[n++] = { -34.0f,   64.0f, 20.0f, 18.0f,  74.0f, RGBA(42, 44, 60, 255) };
    g_city[n++] = {  78.0f,   12.0f, 18.0f, 18.0f,  60.0f, RGBA(41, 43, 58, 255) };
    g_city[n++] = { -12.0f, -104.0f, 26.0f, 26.0f, 128.0f, RGBA(45, 47, 64, 255) }; // segundo landmark, mas lejos
    g_cityCount = n;
}
