#pragma once
// EDIFICIO SIMPLE "como una imagen": cuerpo de cajas con la FACHADA gotica en la
// TEXTURA (ventanas/arcos pintados, no geometria) + techo empinado + aguja.
// Pocos verts -> rinde. El detalle (ventanas ojivales) vive en la textura.
static void buildGothicBldg(TexVertex *buf, int &i, float cx, float cz,
                            float w, float d, float h, unsigned int baseColor, float dist,
                            int *detailStartOut) {
    const unsigned int stone = fadeToVoid(brighten(baseColor, 2.7f), dist);   // muros (con textura de fachada)
    const unsigned int slate = fadeToVoid(RGBA(30, 28, 26, 255), dist);       // techo/aguja oscuros
    const bool big = (h > 120.0f);
    const float bodyTop = h * 0.72f;

    // cuerpo: 2 tiers con leve talud (perfil escalonado, no un ladrillo)
    const float h1 = bodyTop * 0.62f;
    addSolidBoxT(buf, i, cx, 0.0f, cz, w,        d,        h1,          heightHaze(stone, h1 * 0.5f));         // 30
    addSolidBoxT(buf, i, cx, h1,   cz, w * 0.9f, d * 0.9f, bodyTop - h1, heightHaze(stone, h1 + (bodyTop-h1)*0.5f)); // 30
    if (detailStartOut) *detailStartOut = i;                                   // (sin detalle pesado)

    // techo empinado de pizarra (clave del gotico) + aguja escalonada
    addPyramidT(buf, i, cx, bodyTop, cz, w * 0.92f, d * 0.92f, h * 0.42f, heightHaze(slate, bodyTop));         // 12
    addPyramidT(buf, i, cx, bodyTop + h * 0.14f, cz, w * 0.55f, d * 0.55f, h * 0.34f, heightHaze(brighten(slate,1.2f), bodyTop)); // 12

    // campanario descentrado con su aguja (silueta asimetrica)
    const float bx = cx + w * 0.17f, bz = cz - d * 0.12f, bw = w * 0.26f;
    const float belfryTop = h * (big ? 1.24f : 1.02f);
    addSolidBoxT(buf, i, bx, bodyTop * 0.5f, bz, bw, bw, belfryTop - bodyTop * 0.5f, heightHaze(stone, belfryTop * 0.5f)); // 30
    addPyramidT(buf, i, bx, belfryTop, bz, bw * 1.1f, bw * 1.1f, h * (big ? 0.5f : 0.4f), heightHaze(slate, belfryTop));  // 12

    // catedral (landmark): un par de agujas finas extra para el skyline
    if (big) {
        addPyramidT(buf, i, cx - w * 0.28f, bodyTop, cz + d * 0.05f, w * 0.13f, d * 0.13f, h * 0.42f, heightHaze(slate, bodyTop)); // 12
        addPyramidT(buf, i, cx + w * 0.05f, bodyTop, cz + d * 0.26f, w * 0.12f, d * 0.12f, h * 0.36f, heightHaze(slate, bodyTop)); // 12
    }
}
