#include <zlib.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/ctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "debugScreen.h"
#include "apk_analyzer.h"
#include "zip_reader.h"

#define printf psvDebugScreenPrintf
#define APK_PATH "ux0:data/VitaDroid/test.apk"


static void inspect_manifest(void) {
    unsigned char *data = NULL;
    size_t size = 0;

    int rc = zip_read_entry(
        APK_PATH, "AndroidManifest.xml", &data, &size
    );

    if (rc != 0) {
        printf("Manifest read failed: %d\n", rc);
        return;
    }

    printf("Manifest size: %lu bytes\n",
           (unsigned long)size);

    if (size >= 4) {
        printf("Header: %02X %02X %02X %02X\n",
               data[0], data[1], data[2], data[3]);

        if (data[0] == 0x03 && data[1] == 0x00 &&
            data[2] == 0x08 && data[3] == 0x00) {
            printf("Binary XML signature found\n");
        }
    }

    free(data);
}

int main(void) {
    SceCtrlData pad;
    unsigned int previous = 0;

    psvDebugScreenInit();
    printf("zlib: %s\n", zlibVersion());
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_DIGITAL);

    printf("========================\n");
    printf("    VitaDroid v0.4\n");
    printf("========================\n\n");
    printf("[X] Analyze test.apk\n");
    printf("[O] Exit\n\n");

    while (1) {
        memset(&pad, 0, sizeof(pad));

        if (sceCtrlPeekBufferPositive(0, &pad, 1) > 0) {
            unsigned int pressed = pad.buttons & ~previous;

            if (pressed & SCE_CTRL_CROSS)
                analyze_apk(APK_PATH, psvDebugScreenPrintf);
                inspect_manifest();

            if (pressed & SCE_CTRL_CIRCLE)
                break;

            previous = pad.buttons;
        }

        sceKernelDelayThread(50000);
    }

    return 0;
}
