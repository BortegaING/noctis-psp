#pragma once
// Sparse gothic quarter: a few tall landmarks over dark open ground, empty plaza at spawn.
static void buildCity() {
    int n = 0;

    // Curated, deterministic layout. Fields: x, z, w, d, h, r, g, b.
    // Centers spread over [-140,140] with wide gaps; center left empty (plaza/spawn).
    // Colors: dark cold gothic stone, bluish-grey (blue channel highest); a couple warmer/lit.
    static const float P[][8] = {
        // --- 3 TALL landmarks (cathedrals / towers) ---
        { -40.f,  60.f, 24.f, 22.f, 200.f, 52.f, 50.f, 56.f }, // great cathedral (lit, warmer)
        {  70.f, -50.f, 16.f, 16.f, 170.f, 40.f, 44.f, 60.f }, // bell tower
        { -90.f, -80.f, 18.f, 18.f, 140.f, 36.f, 40.f, 58.f }, // broken spire

        // --- 8 MID mansions / halls ---
        {  40.f,  40.f, 18.f, 16.f,  70.f, 44.f, 46.f, 58.f },
        { -70.f,  20.f, 16.f, 18.f,  60.f, 38.f, 42.f, 56.f },
        {  20.f, -70.f, 18.f, 18.f,  80.f, 42.f, 44.f, 60.f },
        { -30.f, -40.f, 16.f, 16.f,  55.f, 34.f, 38.f, 52.f },
        { 100.f,  30.f, 18.f, 16.f,  75.f, 46.f, 48.f, 62.f }, // lit hall (warmer)
        {-120.f,  50.f, 16.f, 18.f,  50.f, 32.f, 36.f, 50.f },
        {  60.f, 100.f, 18.f, 18.f,  85.f, 40.f, 42.f, 58.f },
        { 110.f,-100.f, 16.f, 16.f,  65.f, 36.f, 40.f, 54.f },

        // --- 5 LOW ruins / gatehouses ---
        {   0.f, 110.f, 20.f, 14.f,  28.f, 34.f, 36.f, 48.f }, // gatehouse
        {-100.f, -30.f, 14.f, 14.f,  22.f, 30.f, 34.f, 46.f },
        { 130.f, -40.f, 16.f, 12.f,  35.f, 44.f, 42.f, 52.f }, // lit ruin (warmer)
        {  30.f,-120.f, 14.f, 16.f,  20.f, 32.f, 34.f, 44.f },
        { -60.f, 110.f, 16.f, 14.f,  32.f, 38.f, 40.f, 54.f },
    };

    int count = (int)(sizeof(P) / sizeof(P[0]));
    for (int i = 0; i < count; ++i) {
        g_city[n].x = P[i][0];
        g_city[n].z = P[i][1];
        g_city[n].w = P[i][2];
        g_city[n].d = P[i][3];
        g_city[n].h = P[i][4];
        g_city[n].color = RGBA((int)P[i][5], (int)P[i][6], (int)P[i][7], 255);
        ++n;
    }

    g_cityCount = n;
}
