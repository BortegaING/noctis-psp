#pragma once
// PROJECT NOCTIS - firma visual/comportamiento de las 10 armas a distancia (que NO se sientan iguales).
// RGBA(r,g,b,a) ya esta definido antes de este include. NO lo redefinas, NO agregues #include.
struct WeaponFX {
    float speed;      // velocidad de la bala en unidades de mundo por frame (60fps). rango util ~0.9 .. 2.8
    float size;       // tamano del tracer (lado del cubo). ~0.08 .. 0.45
    unsigned int col; // color EMISIVO brillante del tracer (RGBA)
    int   spread;     // nro de balas por disparo en abanico (1 = una; 3/5 = escopeta/fragmentacion)
    int   pierce;     // 0 = se destruye al impactar; 1 = atraviesa (sigue tras derribar)
    int   fireFrames; // frames entre disparos a 60fps (cadencia). rapido ~5, canon lento ~42
};

static WeaponFX weaponFX(int idx) {
    WeaponFX f;
    // valores por defecto (fallback seguro = Pistola Laser)
    f.speed = 2.40f; f.size = 0.10f; f.col = RGBA(120,240,255,255); f.spread = 1; f.pierce = 0; f.fireFrames = 6;
    switch(idx) {
        case 0: // Pistola Laser        - rapida, tracer fino cian, 1 bala, cadencia alta
            f.speed = 2.40f; f.size = 0.10f; f.col = RGBA(120,240,255,255); f.spread = 1; f.pierce = 0; f.fireFrames = 6;  break;
        case 1: // Rifle Gravitacional  - media, bala violeta gruesa, potente, cadencia media
            f.speed = 1.60f; f.size = 0.30f; f.col = RGBA(170, 90,255,255); f.spread = 1; f.pierce = 0; f.fireFrames = 20; break;
        case 2: // Carabina Energetica  - rapida, verde, cadencia alta, 1 bala
            f.speed = 2.20f; f.size = 0.14f; f.col = RGBA(120,255,140,255); f.spread = 1; f.pierce = 0; f.fireFrames = 7;  break;
        case 3: // Canion de Particulas - LENTA y GRANDE, naranja, mucho dano, cadencia muy baja, pierce 1
            f.speed = 0.95f; f.size = 0.45f; f.col = RGBA(255,150, 40,255); f.spread = 1; f.pierce = 1; f.fireFrames = 42; break;
        case 4: // Pistola de Pulsos    - media-rapida, azul, 1 bala, cadencia alta
            f.speed = 2.00f; f.size = 0.16f; f.col = RGBA( 80,140,255,255); f.spread = 1; f.pierce = 0; f.fireFrames = 8;  break;
        case 5: // Rifle de Precision   - MUY rapida, tracer finisimo blanco, cadencia baja, pierce 1
            f.speed = 2.80f; f.size = 0.08f; f.col = RGBA(255,255,255,255); f.spread = 1; f.pierce = 1; f.fireFrames = 30; break;
        case 6: // Lanzador de Energia  - lenta, bola teal grande, cadencia baja
            f.speed = 1.10f; f.size = 0.38f; f.col = RGBA( 60,230,220,255); f.spread = 1; f.pierce = 0; f.fireFrames = 34; break;
        case 7: // Arma de Fragmentacion- corta, naranja, SPREAD 5 en abanico, cadencia media
            f.speed = 1.40f; f.size = 0.18f; f.col = RGBA(255,120, 30,255); f.spread = 5; f.pierce = 0; f.fireFrames = 18; break;
        case 8: // Arma Gravitacional Exp.- media, purpura grande, cadencia media, pierce 1
            f.speed = 1.70f; f.size = 0.34f; f.col = RGBA(200, 60,230,255); f.spread = 1; f.pierce = 1; f.fireFrames = 22; break;
        case 9: // Reliquia Antigua     - lenta, dorada grande, PIERCE 1, cadencia muy baja, look raro/antiguo
            f.speed = 1.00f; f.size = 0.40f; f.col = RGBA(255,205, 90,255); f.spread = 1; f.pierce = 1; f.fireFrames = 40; break;
        default: break;
    }
    return f;
}

// Tinte del fogonazo (muzzle-flash) por arma: color brillante cercano al del tracer, empujado hacia el blanco.
static unsigned int weaponMuzzle(int idx) {
    switch(idx) {
        case 0: return RGBA(200,252,255,255); // cian palido
        case 1: return RGBA(220,170,255,255); // violeta claro
        case 2: return RGBA(200,255,205,255); // verde claro
        case 3: return RGBA(255,205,130,255); // naranja incandescente
        case 4: return RGBA(180,210,255,255); // azul claro
        case 5: return RGBA(255,255,255,255); // blanco puro
        case 6: return RGBA(170,250,245,255); // teal palido
        case 7: return RGBA(255,190,120,255); // naranja fragmentacion
        case 8: return RGBA(240,160,255,255); // purpura claro
        case 9: return RGBA(255,235,170,255); // dorado palido
        default: return RGBA(255,255,255,255);
    }
}
