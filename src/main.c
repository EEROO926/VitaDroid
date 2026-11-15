#include <psp2/kernel/threadmgr.h>
#include <psp2/ctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "debugScreen.h"

#define printf psvDebugScreenPrintf
#define APK_PATH "ux0:data/VitaDroid/test.apk"

static uint16_t u16(const unsigned char *p) {
    return (uint16_t)(p[0] | (p[1] << 8));
}

static uint32_t u32(const unsigned char *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int starts(const char *s, const char *prefix) {
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

static void analyze_apk(void) {
    FILE *f;
    unsigned char *tail;
    unsigned char h[46];
    long filesize, begin, eocd = -1;
    long cd_offset, cd_size, cd_end;
    size_t n;
    int i, count;
    int manifest = 0, dex = 0;
    int armv7 = 0, arm64 = 0, x86 = 0;
    int unity = 0, native_libs = 0;
    int ok = 1;

    printf("\n--- APK Analysis ---\n");

    f = fopen(APK_PATH, "rb");
    if (!f) {
        printf("APK not found!\n");
        printf("ux0:data/VitaDroid/\n");
        printf("test.apk\n");
        return;
    }

    if (fseek(f, 0, SEEK_END) != 0 ||
        (filesize = ftell(f)) < 22) {
        printf("Invalid file size.\n");
        fclose(f);
        return;
    }

    n = (size_t)(filesize < 65557 ? filesize : 65557);
    begin = filesize - (long)n;
    tail = malloc(n);
    if (!tail) {
        printf("Out of memory.\n");
        fclose(f);
        return;
    }

    if (fseek(f, begin, SEEK_SET) ||
        fread(tail, 1, n, f) != n) {
        printf("Read error.\n");
        free(tail);
        fclose(f);
        return;
    }

    for (long pos = (long)n - 22; pos >= 0; --pos) {
        if (u32(tail + pos) == 0x06054b50 &&
            pos + 22 + u16(tail + pos + 20) == (long)n) {
            eocd = pos;
            break;
        }
    }

    if (eocd < 0) {
        printf("ZIP directory not found.\n");
        free(tail);
        fclose(f);
        return;
    }

    /* Standard single-disk ZIP only; no ZIP64 support yet. */
    if (u16(tail + eocd + 4) != 0 ||
        u16(tail + eocd + 6) != 0 ||
        u16(tail + eocd + 8) != u16(tail + eocd + 10) ||
        u16(tail + eocd + 10) == 65535 ||
        u32(tail + eocd + 12) == 0xffffffffU ||
        u32(tail + eocd + 16) == 0xffffffffU) {
        printf("Unsupported ZIP format.\n");
        free(tail);
        fclose(f);
        return;
    }

    count = u16(tail + eocd + 10);
    cd_size = (long)u32(tail + eocd + 12);
    cd_offset = (long)u32(tail + eocd + 16);
    cd_end = cd_offset + cd_size;

    free(tail);

    if (cd_offset < 0 || cd_end > filesize ||
        cd_end < cd_offset ||
        fseek(f, cd_offset, SEEK_SET) != 0) {
        printf("Invalid ZIP directory.\n");
        fclose(f);
        return;
    }

    for (i = 0; i < count; ++i) {
        char name[256];
        unsigned int len, extra, comment;
        long next;

        if (ftell(f) < 0 ||
            (long)ftell(f) > cd_end - 46 ||
            fread(h, 1, sizeof(h), f) != sizeof(h) ||
            u32(h) != 0x02014b50) {
            ok = 0;
            break;
        }

        len = u16(h + 28);
        extra = u16(h + 30);
        comment = u16(h + 32);

        next = ftell(f);
        if (next < 0 || next > cd_end ||
            (unsigned long)(cd_end - next) <
                (unsigned long)len + extra + comment) {
            ok = 0;
            break;
        }

        /* Read a bounded prefix of each filename. */
        size_t readlen = len < sizeof(name) - 1
                       ? len : sizeof(name) - 1;

        if (fread(name, 1, readlen, f) != readlen) {
            ok = 0;
            break;
        }
        name[readlen] = '\0';

        if (!strcmp(name, "AndroidManifest.xml"))
            manifest = 1;
        if (!strcmp(name, "classes.dex") ||
            starts(name, "classes2.dex"))
            dex = 1;

        if (starts(name, "lib/armeabi-v7a/")) {
            armv7 = 1;
            native_libs = 1;
        }
        if (starts(name, "lib/arm64-v8a/")) {
            arm64 = 1;
            native_libs = 1;
        }
        if (starts(name, "lib/x86/")) {
            x86 = 1;
            native_libs = 1;
        }
        if (strstr(name, "libunity.so") ||
            strstr(name, "assets/bin/Data/"))
            unity = 1;

        if (fseek(f, next + len + extra + comment,
                  SEEK_SET) != 0) {
            ok = 0;
            break;
        }
    }

    fclose(f);

    if (!ok) {
        printf("ZIP entries are invalid.\n");
        return;
    }

    printf("Entries: %d\n", count);
    printf("Manifest: %s\n", manifest ? "YES" : "NO");
    printf("DEX: %s\n", dex ? "YES" : "NO");
    printf("ARMv7: %s\n", armv7 ? "YES" : "NO");
    printf("ARM64: %s\n", arm64 ? "YES" : "NO");
    printf("x86: %s\n", x86 ? "YES" : "NO");
    printf("Native libs: %s\n", native_libs ? "YES" : "NO");
    printf("Unity hints: %s\n", unity ? "YES" : "NO");

    printf("\nAnalysis complete.\n");
    printf("Runtime not implemented.\n");
}

int main(void) {
    SceCtrlData pad;
    unsigned int previous = 0;

    psvDebugScreenInit();
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_DIGITAL);

    printf("========================\n");
    printf("    VitaDroid v0.2\n");
    printf("========================\n\n");
    printf("[X] Analyze test.apk\n");
    printf("[O] Exit\n\n");

    while (1) {
        memset(&pad, 0, sizeof(pad));

        if (sceCtrlPeekBufferPositive(0, &pad, 1) > 0) {
            unsigned int pressed = pad.buttons & ~previous;

            if (pressed & SCE_CTRL_CROSS)
                analyze_apk();

            if (pressed & SCE_CTRL_CIRCLE)
                break;

            previous = pad.buttons;
        }

        sceKernelDelayThread(50000);
    }

    return 0;
}
