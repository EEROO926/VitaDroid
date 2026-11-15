#include <stdio.h>
#include "apk_analyzer.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s file.apk\n", argv[0]);
        return 1;
    }
    analyze_apk(argv[1], printf);
    return 0;
}
