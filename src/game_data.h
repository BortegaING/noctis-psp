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

static const RangedWeapon kRanged[] = {
    // name                               dmg  rateMs rangeM ener reloadMs mag
    { "Pistola Laser",                     12,   150,    25,   3,     900,  15 },
    { "Rifle Gravitacional",               40,   400,    60,  12,    1800,   8 },
    { "Carabina Energetica",               22,   220,    45,   6,    1200,  20 },
    { "Canion de Particulas",              95,  1200,    40,  30,    3000,   3 },
    { "Pistola de Pulsos",                 16,   120,    20,   4,    1000,  18 },
    { "Rifle de Precision",                80,   900,   120,  15,    2200,   5 },
    { "Lanzador de Energia",               70,  1000,    35,  25,    2800,   4 },
    { "Arma de Fragmentacion",             55,   700,    15,  10,    1600,   6 },
    { "Arma Gravitacional Experimental",   60,   500,    55,  20,    2000,   7 },
    { "Reliquia Antigua",                 110,  1500,    70,  40,    3500,   2 },
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

static const MeleeWeapon kMelee[] = {
    // name                     dmg  speedMs reach weight
    { "Katana",                  30,    300,   12,     3 },
    { "Katana Pesada",           45,    450,   13,     5 },
    { "Espada Energetica",       40,    350,   12,     3 },
    { "Hoja Gravitacional",      50,    500,   14,     4 },
    { "Espada Industrial",       55,    600,   11,     7 },
    { "Lanza",                   35,    400,   20,     4 },
    { "Guadana",                 48,    550,   18,     5 },
    { "Espada de Dos Manos",     70,    800,   16,     9 },
    { "Hoja Experimental",       60,    420,   13,     4 },
    { "Arma Ancestral",          85,    700,   15,     8 },
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
