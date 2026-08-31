// PROJECT NOCTIS - PSP boot minimo.
// Objetivo de este archivo: validar que el pipeline compila y arranca en PSP/PPSSPP.
// A partir de aca crece el motor (GU 3D, camara, gravedad, HUD...).

#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>

PSP_MODULE_INFO("NOCTIS", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static volatile int g_exit = 0;

static int exitCallback(int arg1, int arg2, void *common) {
    g_exit = 1;
    return 0;
}

static int callbackThread(SceSize args, void *argp) {
    int cbid = sceKernelCreateCallback("Exit Callback", exitCallback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

static void setupCallbacks(void) {
    int thid = sceKernelCreateThread("cb_thread", callbackThread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0) sceKernelStartThread(thid, 0, 0);
}

int main(void) {
    pspDebugScreenInit();
    setupCallbacks();

    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    SceCtrlData pad;
    int frame = 0;

    while (!g_exit) {
        sceCtrlReadBufferPositive(&pad, 1);

        pspDebugScreenSetXY(0, 0);
        pspDebugScreenPrintf("PROJECT NOCTIS");
        pspDebugScreenSetXY(0, 2);
        pspDebugScreenPrintf("DISTRITO: Campanario");
        pspDebugScreenSetXY(0, 3);
        pspDebugScreenPrintf("Boot OK - frame %d", frame++);
        pspDebugScreenSetXY(0, 5);
        pspDebugScreenPrintf("HP  1200/1200");
        pspDebugScreenSetXY(0, 6);
        pspDebugScreenPrintf("EN   780/780");
        pspDebugScreenSetXY(0, 7);
        pspDebugScreenPrintf("GRV  100%%");
        pspDebugScreenSetXY(0, 9);
        pspDebugScreenPrintf("START para salir");

        if (pad.Buttons & PSP_CTRL_START) g_exit = 1;

        sceDisplayWaitVblankStart();
    }

    sceKernelExitGame();
    return 0;
}
