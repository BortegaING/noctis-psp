#pragma once
// ============================================================================
// PROJECT NOCTIS - savedata.h
// GUARDADO EN MEMORY STICK. Directiva seccion 42: guardado manual (3 ranuras),
// autoguardado (1), guardado de recuperacion (1), con checksum y respaldo.
//
// FILOSOFIA DE ESTE ARCHIVO
//   1) NUNCA cuelga el juego. Si no hay memory stick, si el directorio no se
//      puede crear, si el archivo esta cortado o corrupto: todas las funciones
//      devuelven false EN SILENCIO y el bucle sigue corriendo. No hay asserts
//      de runtime, ni bucles de reintento, ni excepciones, ni heap.
//   2) Estructura de TAMANO FIJO, POD, sin punteros y SIN PADDING: el archivo
//      es el volcado byte a byte de NoctisSave. Por eso hay static_assert de
//      tamano: si alguien agrega un campo y rompe el layout, NO compila (mejor
//      que sacar partidas corruptas a la calle).
//   3) Todo campo nuevo sale de reserved[] y SUBE SAVE_VERSION. Lo viejo nunca
//      se mueve de sitio: los offsets de abajo son contrato.
//
// E/S: stdio de newlib (fopen/fwrite/fread/fclose/remove/rename) + mkdir() de
//      <sys/stat.h>. Comprobado contra el toolchain: el spec de enlace de
//      psp-gcc ya trae -lcglue (define mkdir, _rename, _unlink sobre sceIo*) y
//      -lpspuser, asi que NO hace falta tocar CMakeLists.txt ni linkear nada
//      nuevo. La alternativa cruda seria sceIoMkdir/sceIoRename/sceIoRemove de
//      <pspiofilemgr.h> (mismas operaciones, un nivel mas abajo); se prefiere
//      stdio por ser la API mas simple que aca funciona igual.
// ============================================================================

#include <stdio.h>      // fopen/fread/fwrite/fclose/remove/rename/snprintf
#include <string.h>     // memset/memcpy
#include <stdint.h>     // uint32_t...
#include <stddef.h>     // offsetof
#include <sys/stat.h>   // mkdir()

// ============================ CONSTANTES =====================================

// 'N','C','T','S' leidos en little endian (el MIPS del PSP es LE) -> 0x5354434E.
// Si el archivo empieza con otra cosa, no es nuestro: se descarta sin tocar nada.
static const uint32_t SAVE_MAGIC   = 0x5354434EU;

// Version del FORMATO (no del juego). Subirla al agregar o mover campos.
//   v1 - formato inicial de 384 bytes descrito abajo.
static const uint16_t SAVE_VERSION = 1;

// Ranuras. 0..2 manuales, 3 autoguardado, 4 recuperacion (copia del auto previo).
static const int SAVE_SLOT_N        = 5;
static const int SAVE_SLOT_MANUAL_N = 3;
static const int SAVE_SLOT_AUTO     = 3;
static const int SAVE_SLOT_RECOVERY = 4;

// Topes FIJOS de los arrays: no pueden cambiar sin subir SAVE_VERSION.
static const int SAVE_RANGED_MAX  = 16;  // main.cpp usa ammoMag[16] (hoy 10 armas)
static const int SAVE_MATKIND_MAX =  8;  // game_data.h tiene 6 materiales
static const int SAVE_RESERVED_N  = 22;  // hueco para crecer sin mover offsets

// Donde viven los archivos. Carpeta propia bajo SAVEDATA: no aparece en el
// gestor de partidas del XMB (eso exigiria un PARAM.SFO), pero se ve por USB.
static const char *SAVE_DIR_ROOT = "ms0:/PSP";
static const char *SAVE_DIR_SAVE = "ms0:/PSP/SAVEDATA";
static const char *SAVE_DIR      = "ms0:/PSP/SAVEDATA/NOCTIS";
static const int   SAVE_PATH_MAX = 64;   // ".../NOCTIS/NOCTIS0.SAV" = 37 chars

// ============================ EL REGISTRO ====================================
// FORMATO DEL ARCHIVO v1 - 384 bytes exactos, volcado crudo, little endian.
// Columna 1 = offset en bytes desde el inicio del archivo.
//
//   off  tam  campo            significado
//   ---- ---  ---------------  --------------------------------------------
//      0   4  magic            SAVE_MAGIC. Si no coincide: no es una partida.
//      4   2  version          SAVE_VERSION con la que se escribio.
//      6   2  sizeBytes        sizeof(NoctisSave). Red de seguridad extra.
//      8   4  playTimeSec      tiempo jugado ACUMULADO, en segundos.
//     12   4  stampSec         reloj de la PSP al guardar (us/1e6). Informativo.
//     16   4  saveCounter      cuantas veces se guardo. Decide cual ranura es
//                              la mas reciente sin depender del reloj.
//   -- jugador --
//     20  12  px, py, pz       posicion del jugador (main.cpp: playerX/Y/Z)
//     32   8  camYaw, camPitch mirada (para no reaparecer mirando a una pared)
//     40   4  gravG            direccion de gravedad 0..5 (gravity.h)
//   -- vitales --
//     44   4  hp               vida actual
//     48   4  hpMax            vida maxima
//     52   4  en               energia actual (main.cpp: en)
//     56   4  enMax            energia MAXIMA (main.cpp: EN_MAX, sube con cada
//                              material recogido: objEnergyMax())
//   -- mundo y progreso --
//     60   4  district         distrito/sector actual. Hoy solo existe el 0
//                              ("Campanario"/sector cerrado): queda preparado.
//     64   4  objActive        ancla activa = checkpoint (objective.h g_objActive)
//     68   4  objAnchorSeen    bitmask de anclas tocadas       (g_objAnchorSeen)
//     72   4  objMatTaken      bitmask de materiales recogidos (g_objMatTaken)
//     76   4  objGoal          1 = FARO alcanzado              (g_objGoal)
//     80   4  deaths           caidas/respawns (estadistica)
//   -- inventario --
//     84   4  invResLo         bitmask recursos recogidos 0..31  (collected[])
//     88   4  invResHi         bitmask recursos recogidos 32..63
//     92   4  invCount         collectedCount (popcount cacheado)
//     96   4  money            dinero
//   -- materiales por tipo --
//    100  32  matQty[8]        cuantos de cada material (game_data.h kMaterials)
//   -- armas --
//    132   4  curRanged        arma a distancia seleccionada (indice en kRanged)
//    136   4  curMelee         arma melee seleccionada       (indice en kMelee)
//    140  64  ammoMag[16]      municion EN EL CARGADOR por arma a distancia
//    204  64  ammoReserve[16]  municion en reserva por arma (aun sin usar)
//    268   4  ownedRanged      bitmask de armas a distancia desbloqueadas
//    272   4  ownedMelee       bitmask de armas melee desbloqueadas
//   -- mejoras, misiones, eventos --
//    276   4  upgrades         bitmask de mejoras aplicadas
//    280   4  questDone        bitmask de misiones completadas
//    284   4  questActive      bitmask de misiones en curso
//    288   4  eventFlags       bitmask de eventos del mundo ya disparados
//   -- reserva y sello --
//    292  88  reserved[22]     CERO. De aca salen los campos futuros.
//    380   4  checksum         FNV-1a 32 bits de los bytes [0, 380).
//                              SIEMPRE ULTIMO: cubre todo lo anterior.
//   ---- total: 384 ----
//
// Regla de migracion: al agregar un campo se le roba sitio a reserved[] (se
// baja SAVE_RESERVED_N en la misma cantidad de palabras), se sube SAVE_VERSION
// y se acepta la version vieja dejando el campo nuevo en 0 (ya viene en cero
// porque reserved[] siempre se escribio en cero).
struct NoctisSave
{
    /* cabecera */
    uint32_t magic;
    uint16_t version;
    uint16_t sizeBytes;
    uint32_t playTimeSec;
    uint32_t stampSec;
    uint32_t saveCounter;

    /* jugador */
    float    px, py, pz;
    float    camYaw, camPitch;
    int32_t  gravG;

    /* vitales */
    float    hp, hpMax;
    float    en, enMax;

    /* mundo y progreso */
    int32_t  district;
    int32_t  objActive;
    uint32_t objAnchorSeen;
    uint32_t objMatTaken;
    uint32_t objGoal;
    uint32_t deaths;

    /* inventario */
    uint32_t invResLo;
    uint32_t invResHi;
    int32_t  invCount;
    int32_t  money;

    /* materiales por tipo */
    int32_t  matQty[SAVE_MATKIND_MAX];

    /* armas */
    int32_t  curRanged;
    int32_t  curMelee;
    int32_t  ammoMag[SAVE_RANGED_MAX];
    int32_t  ammoReserve[SAVE_RANGED_MAX];
    uint32_t ownedRanged;
    uint32_t ownedMelee;

    /* mejoras, misiones, eventos */
    uint32_t upgrades;
    uint32_t questDone;
    uint32_t questActive;
    uint32_t eventFlags;

    /* reserva + sello */
    uint32_t reserved[SAVE_RESERVED_N];
    uint32_t checksum;          // SIEMPRE el ultimo campo
};

// El archivo ES la estructura: si estas dos fallan, el formato se rompio.
// Todos los campos ocupan 4 bytes salvo version/sizeBytes, que son dos de 2
// pegados en el mismo hueco de 4 -> el compilador no inserta relleno y el
// volcado es identico en disco y en memoria. Esto importa de verdad: el
// checksum se calcula sobre bytes crudos y el padding sin inicializar lo
// volveria aleatorio (adios determinismo).
static_assert(sizeof(NoctisSave) == 384,
              "NoctisSave cambio de tamano: subi SAVE_VERSION y actualiza el mapa de offsets");
static_assert(offsetof(NoctisSave, checksum) == 380,
              "checksum debe ser el ULTIMO campo de NoctisSave");

// Bytes que entran en el checksum: TODO menos el propio checksum.
static const unsigned int SAVE_SUM_BYTES =
    (unsigned int)(sizeof(NoctisSave) - sizeof(uint32_t));

// ============================ CHECKSUM =======================================
// FNV-1a de 32 bits. Se elige sobre una suma de bytes porque la suma es ciega a
// los reordenamientos (A+B == B+A) y a las permutaciones de campos: FNV mezcla
// y MULTIPLICA en cada paso, asi que la POSICION de cada byte cambia el
// resultado. Dos archivos con los mismos bytes en distinto orden dan hashes
// distintos, y un solo bit dado vuelta se propaga a todo el hash. Determinista,
// sin tablas y sin heap.
static inline uint32_t saveFnv1a(const unsigned char *p, unsigned int n)
{
    uint32_t h = 2166136261u;                 // offset basis
    for (unsigned int i = 0; i < n; ++i) {
        h ^= (uint32_t)p[i];
        h *= 16777619u;                       // FNV prime (desborda a proposito)
    }
    return h;
}

static inline uint32_t saveChecksum(const NoctisSave &s)
{
    return saveFnv1a((const unsigned char *)&s, SAVE_SUM_BYTES);
}

// Cuenta bits en 1: sirve para rehidratar g_objMatCount / collectedCount desde
// las mascaras, sin tener que confiar en el contador guardado.
static inline int savePopcount(uint32_t v)
{
    int n = 0;
    while (v) { v &= (v - 1); ++n; }
    return n;
}

// ============================ RUTAS Y DIRECTORIO =============================

// Construye "ms0:/PSP/SAVEDATA/NOCTIS/NOCTISn.SAV" (o .TMP). Nombres 8.3 en
// mayusculas: es lo que mejor digiere el FAT de la memory stick.
static inline bool savePath(int slot, char *buf, int n, bool temporal)
{
    if (!buf || n < SAVE_PATH_MAX) return false;
    if (slot < 0 || slot >= SAVE_SLOT_N) return false;
    const int w = snprintf(buf, (size_t)n, "%s/NOCTIS%d.%s",
                           SAVE_DIR, slot, temporal ? "TMP" : "SAV");
    return (w > 0 && w < n);
}

// Crea el arbol si falta. mkdir() devuelve -1 tanto si el directorio YA existia
// como si no hay medio donde escribir, y sin mirar errno no se pueden
// distinguir: por eso no se consulta el resultado. Si de verdad no hay donde
// escribir, el fopen posterior falla y la operacion entera devuelve false.
// Nunca se aborta el juego por esto.
static inline void saveEnsureDir()
{
    mkdir(SAVE_DIR_ROOT, 0777);
    mkdir(SAVE_DIR_SAVE, 0777);
    mkdir(SAVE_DIR,      0777);
}

// ============================ E/S DE BAJO NIVEL ==============================

// Vuelca el registro tal cual. No sella nada: de eso se ocupa saveWrite().
static inline bool saveWriteRaw(const char *path, const NoctisSave &rec)
{
    FILE *f = fopen(path, "wb");
    if (!f) return false;
    const size_t n  = fwrite(&rec, 1, sizeof(NoctisSave), f);
    const int    fl = fflush(f);
    const int    cl = fclose(f);          // se cierra SIEMPRE, falle o no el write
    return (n == sizeof(NoctisSave)) && (fl == 0) && (cl == 0);
}

// Lee y VALIDA. Deja 'out' intacto si algo no cuadra.
// Rechaza: archivo ausente, corto, largo, magico ajeno, version futura, tamano
// declarado distinto y checksum que no da.
static inline bool saveReadRaw(const char *path, NoctisSave *out)
{
    if (!out) return false;

    FILE *f = fopen(path, "rb");
    if (!f) return false;

    NoctisSave rec;
    memset(&rec, 0, sizeof(rec));
    const size_t n = fread(&rec, 1, sizeof(NoctisSave), f);

    // Un archivo mas LARGO de la cuenta tampoco es nuestro: con un byte de mas
    // se descarta (evita tragarse basura pegada al final o un formato futuro).
    unsigned char extra = 0;
    const size_t tail = fread(&extra, 1, 1, f);
    fclose(f);

    if (n != sizeof(NoctisSave) || tail != 0)           return false;
    if (rec.magic != SAVE_MAGIC)                        return false;
    if (rec.version == 0 || rec.version > SAVE_VERSION) return false;
    if (rec.sizeBytes != (uint16_t)sizeof(NoctisSave))  return false;
    if (rec.checksum != saveChecksum(rec))              return false;

    *out = rec;                               // recien ahora se toca la salida
    return true;
}

// Relee lo escrito y comprueba que el sello sea el que dejamos. Es la
// verificacion de verdad: detecta escrituras a medias y cache mentirosa.
static inline bool saveVerifyFile(const char *path, uint32_t expected)
{
    NoctisSave back;
    if (!saveReadRaw(path, &back)) return false;
    return back.checksum == expected;
}

// ============================ API PUBLICA ====================================

// Deja el registro en un estado de PARTIDA NUEVA coherente. Llamalo antes de
// rellenar campos: garantiza que reserved[] y todo lo no usado queden en CERO
// (el checksum se calcula sobre bytes crudos, no puede haber basura ahi).
static void saveClear(NoctisSave *s)
{
    if (!s) return;
    memset(s, 0, sizeof(NoctisSave));
    s->magic         = SAVE_MAGIC;
    s->version       = SAVE_VERSION;
    s->sizeBytes     = (uint16_t)sizeof(NoctisSave);
    s->hp            = 100.0f;
    s->hpMax         = 100.0f;
    s->en            = 780.0f;   // OBJ_EN_BASE de objective.h
    s->enMax         = 780.0f;
    s->objAnchorSeen = 1u;       // A0 (el spawn) arranca puesta, igual que objReset()
    s->ownedRanged   = 1u;       // solo el arma 0 desbloqueada
    s->ownedMelee    = 1u;
}

// Escribe la ranura. slot 0..2 = manuales, 3 = autoguardado, 4 = recuperacion.
//
// ESCRITURA SEGURA (por que existe el .TMP): un apagon a mitad del fwrite
// dejaria la ranura mutilada y perderias la partida. Secuencia:
//   1) volcar a NOCTISn.TMP
//   2) releer el .TMP y verificar el checksum -> si no da, se borra y chau
//      (el .SAV bueno sigue intacto: nunca se destruye antes de tener un
//       temporal VALIDADO)
//   3) borrar el .SAV viejo (el rename del FAT del PSP falla si el destino ya
//      existe) y rename(.TMP -> .SAV): cambiar el nombre es una operacion de
//      directorio, muchisimo mas corta que el volcado -> ventana de riesgo
//      minima
//   4) plan B: si el rename no esta disponible o el resultado no verifica, se
//      escribe directo al .SAV y se comprueba RELEYENDO. El .TMP validado
//      sigue en disco durante esa reescritura, asi que tampoco aca hay un
//      instante sin respaldo.
static bool saveWrite(int slot, const NoctisSave &s)
{
    if (slot < 0 || slot >= SAVE_SLOT_N) return false;

    saveEnsureDir();

    // Copia en pila (384 B, sin heap): aca se sella la cabecera y el checksum,
    // asi el llamador no tiene que acordarse de nada.
    NoctisSave rec = s;
    rec.magic     = SAVE_MAGIC;
    rec.version   = SAVE_VERSION;
    rec.sizeBytes = (uint16_t)sizeof(NoctisSave);
    rec.checksum  = saveChecksum(rec);

    char fin[SAVE_PATH_MAX], tmp[SAVE_PATH_MAX];
    if (!savePath(slot, fin, SAVE_PATH_MAX, false)) return false;
    if (!savePath(slot, tmp, SAVE_PATH_MAX, true))  return false;

    if (!saveWriteRaw(tmp, rec))            { remove(tmp); return false; }
    if (!saveVerifyFile(tmp, rec.checksum)) { remove(tmp); return false; }

    remove(fin);
    if (rename(tmp, fin) == 0 && saveVerifyFile(fin, rec.checksum)) return true;

    // Plan B: sin rename utilizable -> escribir y verificar releyendo.
    const bool ok = saveWriteRaw(fin, rec) && saveVerifyFile(fin, rec.checksum);
    remove(tmp);
    return ok;
}

// Lee la ranura. Valida magico, version y checksum. Si algo falla devuelve
// false y NO toca 'out': podes reintentar con otra ranura sin perder lo que ya
// tenias cargado.
static bool saveRead(int slot, NoctisSave *out)
{
    if (slot < 0 || slot >= SAVE_SLOT_N || !out) return false;
    char fin[SAVE_PATH_MAX];
    if (!savePath(slot, fin, SAVE_PATH_MAX, false)) return false;
    return saveReadRaw(fin, out);
}

// "Existe" = hay una partida LEGIBLE Y SANA en esa ranura. Un archivo presente
// pero corrupto cuenta como inexistente: es lo util para pintar el menu de
// ranuras (si no se va a poder cargar, no se ofrece).
static bool saveExists(int slot)
{
    NoctisSave scratch;
    return saveRead(slot, &scratch);
}

// Borra la ranura (y cualquier .TMP huerfano que haya quedado). Devuelve true
// si al terminar ya no hay nada valido ahi, incluso si no habia nada que borrar.
static bool saveDelete(int slot)
{
    if (slot < 0 || slot >= SAVE_SLOT_N) return false;
    char fin[SAVE_PATH_MAX], tmp[SAVE_PATH_MAX];
    if (!savePath(slot, fin, SAVE_PATH_MAX, false)) return false;
    if (savePath(slot, tmp, SAVE_PATH_MAX, true)) remove(tmp);
    remove(fin);
    return !saveExists(slot);
}

// AUTOGUARDADO con red. ANTES de pisar la ranura 3 se copia la 3 anterior a la
// 4: esa es la RECUPERACION. Si el autoguardado se corrompe (apagon justo ahi,
// memory stick sacada en caliente), el estado previo sigue entero en la 4 y se
// pierde como mucho un tramo de juego, nunca la partida.
// Solo se propaga a la 4 si la 3 estaba SANA: asi la red nunca se llena de
// basura. No devuelve nada a proposito: es fuego y olvido desde el bucle
// principal, y si el medio no esta simplemente no pasa nada.
static void saveAuto(const NoctisSave &s)
{
    NoctisSave prev;
    if (saveRead(SAVE_SLOT_AUTO, &prev))
        saveWrite(SAVE_SLOT_RECOVERY, prev);
    saveWrite(SAVE_SLOT_AUTO, s);
}

// Carga al arrancar: intenta el autoguardado y, si esta roto o no esta, cae a
// la ranura de recuperacion. Devuelve false si no hay nada cargable (partida
// nueva) sin haber tocado 'out'.
static bool saveLoadAutoOrRecovery(NoctisSave *out)
{
    if (!out) return false;
    if (saveRead(SAVE_SLOT_AUTO, out))     return true;
    if (saveRead(SAVE_SLOT_RECOVERY, out)) return true;
    return false;
}

// Ranura valida con el saveCounter mas alto (la partida mas avanzada), o -1 si
// no hay ninguna. Util para un "CONTINUAR" que mire tambien las manuales.
static int saveLatestSlot()
{
    int best = -1;
    uint32_t bestCount = 0;
    NoctisSave rec;
    for (int i = 0; i < SAVE_SLOT_N; ++i) {
        if (!saveRead(i, &rec)) continue;
        if (best < 0 || rec.saveCounter >= bestCount) { best = i; bestCount = rec.saveCounter; }
    }
    return best;
}

// ============================ COMO CABLEARLO (main.cpp) ======================
// 1) #include "savedata.h" junto a los demas headers del bloque de includes.
//
// 2) ARRANQUE: justo despues de declarar gravG (main.cpp ~linea 1034) y ANTES
//    del while:
//        NoctisSave sv; saveClear(&sv);
//        unsigned int playSec = 0;
//        if (saveLoadAutoOrRecovery(&sv)) {
//            playerX = sv.px; playerY = sv.py; playerZ = sv.pz;
//            camYaw  = sv.camYaw; camPitch = sv.camPitch; gravG = sv.gravG;
//            en = sv.en; EN_MAX = sv.enMax;
//            g_objActive     = sv.objActive;
//            g_objAnchorSeen = (int)sv.objAnchorSeen;
//            g_objMatTaken   = (int)sv.objMatTaken;
//            g_objMatCount   = savePopcount(sv.objMatTaken);
//            g_objGoal       = (int)sv.objGoal;
//            curRanged = sv.curRanged; curMelee = sv.curMelee;
//            for (int k = 0; k < kRangedCount && k < SAVE_RANGED_MAX; ++k)
//                ammoMag[k] = sv.ammoMag[k];
//            collectedCount = sv.invCount;
//            for (int r = 0; r < kResourceCount && r < 32; ++r)
//                collected[r] = (int)((sv.invResLo >> r) & 1u);
//            playSec = sv.playTimeSec;
//        }
//
// 3) SNAPSHOT: lo unico que main.cpp tiene que escribir de su lado.
//        auto snap = [&]() {
//            sv.px = playerX; sv.py = playerY; sv.pz = playerZ;
//            sv.camYaw = camYaw; sv.camPitch = camPitch; sv.gravG = gravG;
//            sv.en = en; sv.enMax = EN_MAX;
//            sv.objActive     = g_objActive;
//            sv.objAnchorSeen = (uint32_t)g_objAnchorSeen;
//            sv.objMatTaken   = (uint32_t)g_objMatTaken;
//            sv.objGoal       = (uint32_t)g_objGoal;
//            sv.curRanged = curRanged; sv.curMelee = curMelee;
//            for (int k = 0; k < kRangedCount && k < SAVE_RANGED_MAX; ++k)
//                sv.ammoMag[k] = ammoMag[k];
//            sv.invResLo = 0;
//            for (int r = 0; r < kResourceCount && r < 32; ++r)
//                if (collected[r]) sv.invResLo |= (1u << r);
//            sv.invCount    = collectedCount;
//            sv.playTimeSec = playSec;
//            sv.stampSec    = (uint32_t)(sceKernelGetSystemTimeWide() / 1000000LL);
//            sv.saveCounter++;
//        };
//
// 4) AUTOGUARDADO AL TOCAR UN ANCLA. En main.cpp ~linea 1261 hoy dice:
//        objAnchorTouch(playerX, playerY, playerZ);
//    cambiarlo por:
//        if (objAnchorTouch(playerX, playerY, playerZ) >= 0) { snap(); saveAuto(sv); }
//    objAnchorTouch ya devuelve el indice del ancla RECIEN activada (o -1), asi
//    que el autoguardado cae UNA sola vez por ancla, nunca cada frame: el
//    hipo de E/S coincide con un momento en que el jugador ya esta parado y a
//    salvo en el checkpoint.
//
// 5) GUARDADO DE EMERGENCIA AL SALIR. Al final de main(), entre el cierre del
//    while y sceGuTerm():
//        snap(); saveAuto(sv);
//    HOME pone g_exit=1 desde el hilo de callbacks; el volcado se hace aca, en
//    el hilo principal, que es el unico que ve estas variables locales.
//
// 6) GUARDADO MANUAL (menu de pausa): snap(); saveWrite(0..2, sv);
//    Menu de carga: saveExists(i) para pintar la ranura, saveRead(i, &sv) para
//    cargarla y saveDelete(i) para borrarla.
// =============================================================================
