#pragma once
// PROJECT NOCTIS: ciudad gotica procedural, grid 11x11 (spacing 30) sobre [-150,150], plaza central vacia -> 120 edificios dispersos.
static void buildCity() {
    int n = 0;
    const float spacing = 30.0f;

    // Grid centrado en el origen: i,j en [-5,5] => centros base -150..150.
    for (int i = -5; i <= 5; ++i) {
        for (int j = -5; j <= 5; ++j) {
            // Hash entero determinista por celda (sin rand()).
            unsigned int h  = (unsigned int)(i * 73856093) ^ (unsigned int)(j * 19349663);
            unsigned int h2 = h * 2654435761u;                 // mezcla secundaria
            unsigned int h3 = (h ^ 0x9E3779B9u) * 40503u;      // mezcla terciaria

            // Jitter determinista +-6 para romper el grid perfecto.
            float jx = (float)((int)(h  % 13u) - 6);           // -6..6
            float jz = (float)((int)((h2 >> 8) % 13u) - 6);    // -6..6

            float x = (float)i * spacing + jx;
            float z = (float)j * spacing + jz;

            // PLAZA: dejar el centro vacio (spawn). Radio ~22 alrededor del origen.
            if (x * x + z * z < 22.0f * 22.0f) continue;

            // Variedad de skyline por categoria de altura.
            unsigned int cat = h2 % 100u;
            float ht, fw, fd;
            if (cat < 55u) {                 // ~55% medios
                ht = 34.0f + (float)(h3 % 47u);            // 34..80
                fw = 9.0f  + (float)((h  >> 3) % 8u);      // 9..16
                fd = 9.0f  + (float)((h2 >> 3) % 8u);      // 9..16
            } else if (cat < 80u) {          // ~25% bajos
                ht = 16.0f + (float)(h3 % 17u);            // 16..32
                fw = 9.0f  + (float)((h  >> 3) % 7u);      // 9..15
                fd = 9.0f  + (float)((h2 >> 3) % 7u);      // 9..15
            } else if (cat < 95u) {          // ~15% torres altas
                ht = 90.0f + (float)(h3 % 61u);            // 90..150
                fw = 12.0f + (float)((h  >> 3) % 7u);      // 12..18
                fd = 12.0f + (float)((h2 >> 3) % 7u);      // 12..18
            } else {                         // ~5% rascacielos landmark
                ht = 160.0f + (float)(h3 % 71u);           // 160..230
                fw = 14.0f  + (float)((h  >> 3) % 7u);     // 14..20
                fd = 14.0f  + (float)((h2 >> 3) % 7u);     // 14..20
            }

            // Color: piedra gotica oscura y fria, gris azulado (B mas alto = frio).
            unsigned int hc = h ^ (h2 >> 5) ^ (h3 << 3);
            int r = 34 + (int)(hc        % 25u);           // 34..58
            int g = 36 + (int)((hc >> 5) % 25u);           // 36..60
            int b = 44 + (int)((hc >> 10) % 27u);          // 44..70
            // ~9%: unos pocos mas calidos/oscuros para variedad.
            if (((hc >> 20) % 11u) == 0u) {
                r = 40 + (int)(hc        % 14u);           // 40..53
                g = 34 + (int)((hc >> 5) % 12u);           // 34..45
                b = 30 + (int)((hc >> 10) % 12u);          // 30..41 (menos azul)
            }

            g_city[n].x = x;
            g_city[n].z = z;
            g_city[n].w = fw;
            g_city[n].d = fd;
            g_city[n].h = ht;
            g_city[n].color = RGBA(r, g, b, 255);
            ++n;
            if (n >= 256) { g_cityCount = n; return; }   // seguridad: no pasar g_city[256]
        }
    }

    g_cityCount = n;
}
