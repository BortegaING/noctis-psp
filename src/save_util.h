// save_util.h -- guardar/cargar partida con la utilidad de sistema de la PSP
// (psputility_savedata). La partida aparece en el XMB con ICON0 (144x80) y PIC1 (480x272).
// Modo AUTOSAVE / AUTOLOAD (sin dialogo). Enlazar con: -lpsputility
// IMPORTANTE: frame() debe dibujar un cuadro COMPLETO (sceGuStart..sceGuFinish, sceGuSync,
// vblank, sceGuSwapBuffers); la utilidad dibuja encima durante sceUtilitySavedataUpdate.
#pragma once
#include <pspkernel.h>
#include <psputility.h>
#include <psputility_savedata.h>
#include <psputility_sysparam.h>
#include <malloc.h>
#include <string.h>

#define SAVEUTIL_GAME_NAME   "NOCTIS000"
#define SAVEUTIL_SAVE_NAME   "0000"
#define SAVEUTIL_FILE_NAME   "DATA.BIN"
#define SAVEUTIL_TIMEOUT_US  (15u * 1000000u)   // maximo 15 s de espera

// Copia segura de texto a un buffer fijo
static inline void saveUtilCopyStr(char *dst, unsigned int cap, const char *src)
{
    unsigned int i = 0;
    if (src) { for (; i + 1 < cap && src[i]; i++) dst[i] = src[i]; }
    dst[i] = 0;
}

// Prepara la estructura base comun (size, idioma, hilos, nombres)
static inline void saveUtilInitParam(SceUtilitySavedataParam *p)
{
    int lang = PSP_SYSTEMPARAM_LANGUAGE_ENGLISH, swap = 0;
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE, &lang);
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_BUTTON_SWAP, &swap);

    memset(p, 0, sizeof(*p));               // deja tambien key[] en ceros
    p->base.size         = sizeof(*p);
    p->base.language     = lang;
    p->base.buttonSwap   = swap;
    p->base.graphicsThread = 0x11;
    p->base.accessThread   = 0x13;
    p->base.fontThread     = 0x12;
    p->base.soundThread    = 0x10;

    p->overwrite = 1;
    saveUtilCopyStr(p->gameName, sizeof(p->gameName), SAVEUTIL_GAME_NAME);
    saveUtilCopyStr(p->saveName, sizeof(p->saveName), SAVEUTIL_SAVE_NAME);
    saveUtilCopyStr(p->fileName, sizeof(p->fileName), SAVEUTIL_FILE_NAME);
}

// Bucle de espera. Devuelve true si termino a tiempo; resultado en p->base.result.
static inline bool saveUtilRun(SceUtilitySavedataParam *p, void (*frame)(void))
{
    if (sceUtilitySavedataInitStart(p) < 0) return false;
    u64 t0 = sceKernelGetSystemTimeWide();
    bool ok = false;
    for (;;) {
        if (frame) frame();
        int st = sceUtilitySavedataGetStatus();
        if (st == PSP_UTILITY_DIALOG_VISIBLE)      sceUtilitySavedataUpdate(1);
        else if (st == PSP_UTILITY_DIALOG_QUIT)    sceUtilitySavedataShutdownStart();
        else if (st == PSP_UTILITY_DIALOG_NONE)  { ok = true; break; }
        // FINISHED: seguir llamando GetStatus hasta NONE
        if (!frame) sceKernelDelayThread(16000);
        if (sceKernelGetSystemTimeWide() - t0 > SAVEUTIL_TIMEOUT_US) {
            sceUtilitySavedataShutdownStart();   // abortar; no esperar mas
            break;
        }
    }
    return ok;
}

static bool saveUtilWrite(const void *data, unsigned int size,
                          const unsigned char *icon0, unsigned int icon0Size,
                          const unsigned char *pic1,  unsigned int pic1Size,
                          const char *title, const char *savedataTitle, const char *detail,
                          void (*frame)(void))
{
    if (!data || size == 0) return false;
    // El buffer debe estar alineado: copia a memoria alineada a 64 bytes
    unsigned int cap = (size + 63u) & ~63u;
    void *buf = memalign(64, cap);
    if (!buf) return false;
    memset(buf, 0, cap);
    memcpy(buf, data, size);

    static SceUtilitySavedataParam p;   // estatica: la utilidad la usa mientras trabaja
    saveUtilInitParam(&p);
    p.mode        = PSP_UTILITY_SAVEDATA_AUTOSAVE;
    p.dataBuf     = buf;
    p.dataBufSize = size;
    p.dataSize    = size;

    saveUtilCopyStr(p.sfoParam.title,        sizeof(p.sfoParam.title),        title);
    saveUtilCopyStr(p.sfoParam.savedataTitle, sizeof(p.sfoParam.savedataTitle), savedataTitle);
    saveUtilCopyStr(p.sfoParam.detail,       sizeof(p.sfoParam.detail),       detail);
    p.sfoParam.parentalLevel = 1;

    if (icon0 && icon0Size) {
        p.icon0FileData.buf     = (void *)icon0;
        p.icon0FileData.bufSize = icon0Size;
        p.icon0FileData.size    = icon0Size;
    }
    if (pic1 && pic1Size) {
        p.pic1FileData.buf     = (void *)pic1;
        p.pic1FileData.bufSize = pic1Size;
        p.pic1FileData.size    = pic1Size;
    }

    bool done = saveUtilRun(&p, frame);
    bool ok = done && p.base.result == 0;
    if (done) free(buf);   // si hubo timeout se filtra a proposito (la utilidad aun podria usarlo)
    return ok;
}

static bool saveUtilRead(void *data, unsigned int size, void (*frame)(void))
{
    if (!data || size == 0) return false;
    unsigned int cap = (size + 63u) & ~63u;
    void *buf = memalign(64, cap);
    if (!buf) return false;
    memset(buf, 0, cap);

    static SceUtilitySavedataParam p;
    saveUtilInitParam(&p);
    p.mode        = PSP_UTILITY_SAVEDATA_AUTOLOAD;
    p.dataBuf     = buf;
    p.dataBufSize = size;
    p.dataSize    = size;

    bool done = saveUtilRun(&p, frame);
    // result 0 = leido; 0x80110306 = no existe (cualquier otro valor tambien da false)
    bool ok = done && p.base.result == 0;
    if (ok) memcpy(data, buf, size);
    if (done) free(buf);
    return ok;
}
