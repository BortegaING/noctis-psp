#pragma once

// PROJECT NOCTIS - Tablas de datos del juego (ASCII 8x8, sin acentos).
// Header autocontenido: no incluye nada, solo structs y datos.

// ---------------------------------------------------------------------------
// 1) Armas a distancia
// ---------------------------------------------------------------------------
struct RangedWeapon {
    const char* name;
    int damage;
    int fireRateMs;
    int rangeM;
    int energyCost;
    int reloadMs;
    int magazine;
};

// Cada arma es un nicho DISTINTO: cadencia, dano, alcance, energia, recarga, cargador.
// (la cadencia REAL en pantalla la fija fireFrames en weapons_fx.h; rateMs va coherente)
static const RangedWeapon kRanged[] = {
    // name                         dmg  rateMs rangeM ener reloadMs mag
    { "Pistola de Pulsos",           10,   110,    22,   2,     800,  18 }, // 0 sidearm rapido debil
    { "Lanza Gravitacional",         44,   420,    65,  12,    1800,   8 }, // 1 slug pesado violeta
    { "Carabina Energetica",         20,   200,    48,   5,    1100,  24 }, // 2 automatica equilibrada
    { "Canon de Particulas",        100,  1300,    42,  34,    3200,   3 }, // 3 lento enorme, atraviesa
    { "Repetidor del Vacio",          8,    80,    30,   3,    1000,  30 }, // 4 full-auto, el mas rapido
    { "Aguja de Riel",               85,   900,   130,  15,    2200,   5 }, // 5 railgun largo, atraviesa
    { "Haz de Resonancia",           26,   160,    55,   6,    1400,  20 }, // 6 haz sostenido que ensarta
    { "Dispersor de Fragmentos",     14,   700,    15,  10,    1600,   6 }, // 7 escopeta abanico corto
    { "Bobina de Singularidad",      58,   520,    50,  20,    2000,   7 }, // 8 orbe gravitacional pierce
    { "Reliquia del Abismo",        120,  1500,    75,  40,    3500,   2 }, // 9 reliquia, el mas letal
};
static const int kRangedCount = (int)(sizeof(kRanged) / sizeof(kRanged[0]));

// ---------------------------------------------------------------------------
// 2) Armas cuerpo a cuerpo
//    speedMs = tiempo por golpe (menor = mas rapido)
//    reach   = alcance en decimetros
//    weight  = 1..10
// ---------------------------------------------------------------------------
struct MeleeWeapon {
    const char* name;
    int damage;
    int speedMs;
    int reach;
    int weight;
};

// Cada hoja se siente distinta por velocidad/alcance/dano/peso (speedMs y reach mandan).
static const MeleeWeapon kMelee[] = {
    // name                     dmg  speedMs reach weight
    { "Katana",                  28,    260,   12,     3 }, // 0 rapida y equilibrada
    { "Katana Pesada",           48,    460,   13,     6 }, // 1 mas lenta, mas dano
    { "Espada Energetica",       34,    220,   11,     2 }, // 2 la mas rapida, ligera
    { "Hoja Gravitacional",      52,    520,   15,     5 }, // 3 pesada, largo alcance
    { "Espada Industrial",       60,    640,   11,     8 }, // 4 lenta, brutal, corta
    { "Lanza",                   32,    380,   22,     4 }, // 5 el mayor alcance, poco dano
    { "Guadana",                 50,    540,   19,     5 }, // 6 barrido largo medio-alto
    { "Espada de Dos Manos",     74,    820,   17,     9 }, // 7 la mas lenta y demoledora
    { "Hoja Experimental",       44,    340,   13,     3 }, // 8 agil y versatil
    { "Arma Ancestral",          88,    700,   16,     8 }, // 9 rara, letal, largo alcance
};
static const int kMeleeCount = (int)(sizeof(kMelee) / sizeof(kMelee[0]));

// ---------------------------------------------------------------------------
// 3) Materiales (rarity 1 comun .. 5 extremadamente raro)
// ---------------------------------------------------------------------------
struct Material {
    const char* name;
    int rarity;
};

static const Material kMaterials[] = {
    { "Fragmento de Aleacion", 1 },
    { "Cristal Oscuro",        2 },
    { "Circuito Antiguo",      2 },
    { "Nucleo Gravitacional",  4 },
    { "Fragmento de Nucleo",   5 },
    { "Metal de Abismo",       4 },
};
static const int kMaterialCount = (int)(sizeof(kMaterials) / sizeof(kMaterials[0]));

// ---------------------------------------------------------------------------
// 4) Familias de enemigos (dureza creciente; Titans hp enorme)
// ---------------------------------------------------------------------------
struct EnemyFamily {
    const char* name;
    int hp;
    int damage;
    int money;
};

static const EnemyFamily kEnemies[] = {
    // name            hp   dmg  money
    { "Gargoyles",      40,    6,    10 },
    { "Hollows",        70,   10,    18 },
    { "Constructs",    120,   16,    30 },
    { "Aberrations",   200,   24,    55 },
    { "Guardians",     350,   38,    90 },
    { "Titans",       1200,   70,   300 },
};
static const int kEnemyCount = (int)(sizeof(kEnemies) / sizeof(kEnemies[0]));
