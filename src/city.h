#pragma once
// PUEBLO gotico: SOLO 4 edificios bien detallados (no una ciudad), mucho suelo
// oscuro entre ellos, agrupados frente al spawn. Aura Bloodborne/MediEvil.
static void buildCity() {
    int n = 0;
    // catedral landmark, justo al frente (-Z) para que reciba al jugador
    g_city[n++] = { 0.0f,  -52.0f, 30.0f, 30.0f, 158.0f, RGBA(42, 44, 60, 255) };
    // casona/salon a la izquierda
    g_city[n++] = { -38.0f, -20.0f, 20.0f, 18.0f,  76.0f, RGBA(44, 46, 62, 255) };
    // mansion a la derecha
    g_city[n++] = {  36.0f, -28.0f, 18.0f, 20.0f,  64.0f, RGBA(40, 42, 56, 255) };
    // capilla/ruina mas cerca, izquierda-frente (un pelo mas calida = "iluminada")
    g_city[n++] = { -18.0f,  -6.0f, 15.0f, 15.0f,  42.0f, RGBA(52, 46, 50, 255) };
    g_cityCount = n;
}
