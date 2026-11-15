#include "zip_reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <zlib.h>

#define MAX_OUTPUT (1024U * 1024U)
#define MAX_INPUT  (8U * 1024U * 1024U)

static uint16_t rd16(const unsigned char *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t rd32(const unsigned char *p) {
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static int read_at(FILE *f, long offset, void *buf, size_t len) {
    return offset >= 0 &&
           fseek(f, offset, SEEK_SET) == 0 &&
           fread(buf, 1, len, f) == len;
}

int zip_read_entry(const char *archive, const char *entry,
                   unsigned char **data, size_t *size) {
    FILE *f = NULL;
    unsigned char *tail = NULL, *compressed = NULL, *output = NULL;
    unsigned char h[46], local[30];
    long file_size, tail_start, eocd = -1;
    long cd_start, cd_end, pos;
    size_t tail_size;
    uint32_t offset = 0, csize = 0, usize = 0, expected_crc = 0;
    uint16_t flags = 0, method = 0, entries = 0;
    int found = 0, rc = -1;

    if (!archive || !entry || !data || !size)
        return -1;

    *data = NULL;
    *size = 0;

    f = fopen(archive, "rb");
    if (!f)
        return -1;

    if (fseek(f, 0, SEEK_END) ||
        (file_size = ftell(f)) < 22)
        goto done;

    tail_size = (size_t)(file_size < 65557 ? file_size : 65557);
    tail_start = file_size - (long)tail_size;
    tail = malloc(tail_size);
    if (!tail) {
        rc = -2;
        goto done;
    }

    if (!read_at(f, tail_start, tail, tail_size))
        goto done;

    for (long i = (long)tail_size - 22; i >= 0; --i) {
        if (rd32(tail + i) == 0x06054b50 &&
            (size_t)i + 22 + rd16(tail + i + 20) == tail_size) {
            eocd = i;
            break;
        }
    }
    if (eocd < 0)
        goto done;

    const unsigned char *e = tail + eocd;

    /* Reject multi-disk ZIP and ZIP64. */
    if (rd16(e + 4) || rd16(e + 6) ||
        rd16(e + 8) != rd16(e + 10) ||
        rd16(e + 10) == 65535 ||
        rd32(e + 12) == UINT32_MAX ||
        rd32(e + 16) == UINT32_MAX)
        goto done;

    entries = rd16(e + 10);
    cd_start = (long)rd32(e + 16);

    if ((uint64_t)cd_start + rd32(e + 12) >
        (uint64_t)tail_start + (uint64_t)eocd)
        goto done;

    cd_end = cd_start + (long)rd32(e + 12);
    pos = cd_start;

    for (unsigned int i = 0; i < entries; ++i) {
        unsigned int name_len, extra_len, comment_len;
        long next;
        char *name;

        if (pos < cd_start || pos > cd_end - 46 ||
            !read_at(f, pos, h, sizeof(h)) ||
            rd32(h) != 0x02014b50)
            goto done;

        name_len = rd16(h + 28);
        extra_len = rd16(h + 30);
        comment_len = rd16(h + 32);

        next = pos + 46L + name_len + extra_len + comment_len;
        if (next > cd_end || next < pos)
            goto done;

        name = malloc((size_t)name_len + 1);
        if (!name) {
            rc = -2;
            goto done;
        }

        if (!read_at(f, pos + 46, name, name_len)) {
            free(name);
            goto done;
        }
        name[name_len] = 0;

        int matches = strlen(name) == name_len &&
                      strcmp(name, entry) == 0;
        free(name);

        if (matches) {
            if (found)
                goto done; /* Reject duplicate target entries. */

            found = 1;
            flags = rd16(h + 8);
            method = rd16(h + 10);
            expected_crc = rd32(h + 16);
            csize = rd32(h + 20);
            usize = rd32(h + 24);
            offset = rd32(h + 42);

            if (rd16(h + 34) != 0 ||
                rd16(h + 36) != 0 ||
                (flags & (1U | 0x40U | 0x2000U)) ||
                (method != 0 && method != 8) ||
                csize == UINT32_MAX ||
                usize == UINT32_MAX)
                goto done;
        }

        pos = next;
    }

    if (!found || pos != cd_end)
        goto done;

    if (usize > MAX_OUTPUT || csize > MAX_INPUT) {
        rc = -2;
        goto done;
    }

    if ((uint64_t)offset + 30 > (uint64_t)file_size ||
        !read_at(f, (long)offset, local, sizeof(local)) ||
        rd32(local) != 0x04034b50 ||
        rd16(local + 8) != method ||
        rd16(local + 6) != flags)
        goto done;

    /* Validate local filename against central directory. */
    unsigned int local_name_len = rd16(local + 26);
    unsigned int local_extra_len = rd16(local + 28);

    uint64_t data_pos = (uint64_t)offset + 30 +
                        local_name_len + local_extra_len;

    if (local_name_len != strlen(entry) ||
        data_pos > (uint64_t)cd_start ||
        data_pos + csize > (uint64_t)cd_start)
        goto done;

    char *local_name = malloc((size_t)local_name_len + 1);
    if (!local_name) {
        rc = -2;
        goto done;
    }

    int name_ok = read_at(f, (long)offset + 30,
                          local_name, local_name_len) &&
                  memcmp(local_name, entry, local_name_len) == 0;

    free(local_name);

    if (!name_ok)
        goto done;

    if (data_pos + csize > (uint64_t)file_size ||
        data_pos > (uint64_t)cd_start)
        goto done;

    compressed = malloc(csize ? csize : 1);
    output = malloc(usize ? usize : 1);

    if (!compressed || !output) {
        rc = -2;
        goto done;
    }

    if (!read_at(f, (long)data_pos, compressed, csize))
        goto done;

    if (method == 0) {
        if (csize != usize)
            goto done;
        memcpy(output, compressed, usize);
    } else {
        z_stream zs;
        memset(&zs, 0, sizeof(zs));

        zs.next_in = compressed;
        zs.avail_in = csize;
        zs.next_out = output;
        zs.avail_out = usize ? usize : 1;

        if (inflateInit2(&zs, -MAX_WBITS) != Z_OK)
            goto done;

        int zrc = inflate(&zs, Z_FINISH);
        int valid = zrc == Z_STREAM_END &&
                    zs.total_out == usize &&
                    zs.total_in == csize;
        inflateEnd(&zs);

        if (!valid)
            goto done;
    }

    if ((uint32_t)crc32(0L, output, usize) != expected_crc)
        goto done;

    *data = output;
    *size = usize;
    output = NULL;
    rc = 0;

done:
    free(tail);
    free(compressed);
    free(output);
    if (f)
        fclose(f);
    return rc;
}
