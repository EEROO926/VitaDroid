#ifndef VITADROID_ZIP_READER_H
#define VITADROID_ZIP_READER_H

#include <stddef.h>

/*
 * Returns:
 *  0 = success
 * -1 = missing file or invalid/unsupported ZIP
 * -2 = memory or size limit failure
 *
 * On success, caller must free(*data).
 * Maximum uncompressed entry size: 1 MiB.
 */
int zip_read_entry(const char *archive, const char *entry,
                   unsigned char **data, size_t *size);

#endif
