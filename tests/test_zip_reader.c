#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zip_reader.h"

int main(int argc, char **argv) {
    unsigned char *data = NULL;
    size_t size = 0;

    if (argc != 3)
        return 2;

    int rc = zip_read_entry(argv[1], argv[2], &data, &size);

    if (rc != 0) {
        printf("READ FAILED: %d\n", rc);
        return 1;
    }

    printf("READ OK: %zu bytes\n", size);
    free(data);
    return 0;
}
