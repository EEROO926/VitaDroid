#include <psp2/kernel/threadmgr.h>
#include <psp2/ctrl.h>
#include <stdio.h>
#include <string.h>

#include "debugScreen.h"

#define printf psvDebugScreenPrintf

int main(int argc, char *argv[]) {
    SceCtrlData pad;
    unsigned int previous = 0;

    (void)argc;
    (void)argv;

    psvDebugScreenInit();
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_DIGITAL);

    printf("============================\n");
    printf("        VitaDroid v0.1\n");
    printf("============================\n\n");

    printf("Android APK Compatibility\n");
    printf("Experimental Project\n\n");

    printf("[X] APK Analyzer\n");
    printf("[O] Exit\n\n");

    printf("Status: Initial prototype\n");
    printf("Android runtime: Not implemented\n\n");

    while (1) {
        memset(&pad, 0, sizeof(pad));

        if (sceCtrlPeekBufferPositive(0, &pad, 1) > 0) {
            unsigned int pressed = pad.buttons & ~previous;

            if (pressed & SCE_CTRL_CROSS) {
                printf("\nAPK Analyzer\n");
                printf("Coming in a future version.\n");
            }

            if (pressed & SCE_CTRL_CIRCLE) {
                break;
            }

            previous = pad.buttons;
        }

        sceKernelDelayThread(50000);
    }

    return 0;
}
