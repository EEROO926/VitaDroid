#include <psp2/kernel/threadmgr.h>
#include <psp2/ctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "debugScreen.h"
#include "apk_analyzer.h"

#define printf psvDebugScreenPrintf
#define APK_PATH "ux0:data/VitaDroid/test.apk"

int main(void) {
    SceCtrlData pad;
    unsigned int previous = 0;

    psvDebugScreenInit();
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_DIGITAL);

    printf("========================\n");
    printf("    VitaDroid v0.3\n");
    printf("========================\n\n");
    printf("[X] Analyze test.apk\n");
    printf("[O] Exit\n\n");

    while (1) {
        memset(&pad, 0, sizeof(pad));

        if (sceCtrlPeekBufferPositive(0, &pad, 1) > 0) {
            unsigned int pressed = pad.buttons & ~previous;

            if (pressed & SCE_CTRL_CROSS)
                analyze_apk(APK_PATH, psvDebugScreenPrintf);

            if (pressed & SCE_CTRL_CIRCLE)
                break;

            previous = pad.buttons;
        }

        sceKernelDelayThread(50000);
    }

    return 0;
}
