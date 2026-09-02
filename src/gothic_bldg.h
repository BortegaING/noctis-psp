#pragma once
// EDIFICIO BRUTALISTA-GOTICO: POCAS masas MACIZAS (Dracula fusionado con BLAME).
// TODO el detalle fino (ventanas ojivales, encofrado, contrafuertes finos, ornamento)
// vive en la TEXTURA de fachada (genFacade) -> aqui SOLO geometria gruesa y pocos
// verts (clave para el fill-rate de la PSP real). Silueta: mole monolitica de hormigon
// + un unico retranqueo + remate de pizarra casi negro empinado + un contrafuerte romo.
// NO hay geometria de ventanas, NO hay filigrana. Firma intacta (main.cpp la cablea).
static void buildGothicBldg(TexVertex *buf, int &i, float cx, float cz,
                            float w, float d, float h, unsigned int baseColor, float dist,
                            int *detailStartOut) {
    const unsigned int stone = fadeToVoid(brighten(baseColor, 2.7f), dist);  // muro (la fachada la PINTA la textura)
    const unsigned int slate = fadeToVoid(RGBA(24, 22, 20, 255), dist);      // pizarra calida casi negra (remate)
    const bool big = (h > 120.0f);                                           // landmark/catedral

    const float h1      = h * 0.80f;                     // mole principal (la mayor parte de la altura)
    const float bodyTop = big ? h * 0.94f : h * 0.90f;   // tope del cuerpo tras el retranqueo

    // 1) MASA PRINCIPAL: bloque monolitico de hormigon crudo -> 30 verts
    addSolidBoxT(buf, i, cx, 0.0f, cz, w, d, h1, heightHaze(stone, h1 * 0.5f));

    // 2) UN retranqueo (setback): perfil de fortaleza en vez de un ladrillo liso -> 30 verts
    addSolidBoxT(buf, i, cx, h1, cz, w * 0.86f, d * 0.86f, bodyTop - h1,
                 heightHaze(stone, h1 + (bodyTop - h1) * 0.5f));

    // LOD: a distancia main.cpp dibuja SOLO la mole [sStart, sDetail); el remate se salta
    if (detailStartOut) *detailStartOut = i;

    // 3) cornisa/parapeto: losa fina que corona el hormigon antes de la pizarra -> 30 verts
    addSolidBoxT(buf, i, cx, bodyTop, cz, w * 0.92f, d * 0.92f, h * 0.03f,
                 heightHaze(brighten(stone, 0.72f), bodyTop));

    float capBase = bodyTop + h * 0.03f;
    float capW    = w * 0.9f, capD = d * 0.9f;

    // 3b) landmark: torreta-linterna central -> silueta de torre del homenaje -> 30 verts
    if (big) {
        const float lw = w * 0.48f, ld = d * 0.48f, lh = h * 0.12f;
        addSolidBoxT(buf, i, cx, capBase, cz, lw, ld, lh, heightHaze(stone, capBase + lh * 0.5f));
        capBase += lh; capW = lw; capD = ld;
    }

    // 4) remate de PIZARRA empinado y romo (gotico brutalista, casi negro) -> 12 verts
    addPyramidT(buf, i, cx, capBase, cz, capW, capD, h * (big ? 0.22f : 0.16f),
                heightHaze(slate, capBase));

    // 5) UN contrafuerte romo contra el frente (masa pura, sin filigrana) -> 30 verts
    const float btW = w * 0.22f, btD = d * 0.16f, btH = h1 * 0.66f;
    addSolidBoxT(buf, i, cx, 0.0f, cz + d * 0.5f + btD * 0.5f, btW, btD, btH,
                 heightHaze(brighten(stone, 0.9f), btH * 0.5f));
    // Total verts: normal = 132 (30+30+30+12+30); landmark 'big' = 162 (+linterna). <= ~200.
}
