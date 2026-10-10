#include "archive_reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

struct UCArchive {
    uint8_t *data;
    size_t size, count;
    UCArchiveEntry *entries;
};
static uint16_t u16(const uint8_t *p) { return (uint16_t)(p[0] | (uint16_t)p[1] << 8); }
static uint32_t u32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
void uc_archive_close(UCArchive *a) {
    if (a) { free(a->entries); free(a->data); free(a); }
}
static UCArchiveStatus parse_owned(uint8_t *data, size_t size, UCArchive **out) {
    UCArchive *a = NULL;
    UCArchiveStatus error = UC_AR_FORMAT;
    if (size < 0xC8 || u32(data) != 0x1C) goto malformed;
    uint32_t directory = u32(data + 0xC0), count = u32(data + 0xC4);
    if (directory < 4 || directory > size - 0xC4) goto malformed;
    size_t end = 0xC4 + (size_t)directory, cursor = 0xC8;
    if (count > (end - cursor) / 18 || count > SIZE_MAX / sizeof(UCArchiveEntry)) goto malformed;
    a = (UCArchive *)calloc(1, sizeof(*a));
    if (!a) { free(data); return UC_AR_MEMORY; }
    a->data = data; a->size = size; a->count = count;
    if (count) {
        a->entries = (UCArchiveEntry *)calloc(count, sizeof(*a->entries));
        if (!a->entries) { uc_archive_close(a); return UC_AR_MEMORY; }
    }
    for (size_t i = 0; i < count; ++i) {
        if (end - cursor < 18) goto malformed;
        uint16_t name_bytes = u16(data + cursor); cursor += 2;
        if (!name_bytes || name_bytes >= 0x400 || name_bytes > end - cursor - 16 || data[cursor + name_bytes - 1]) goto malformed;
        UCArchiveEntry *e = &a->entries[i];
        e->name = data + cursor; e->name_bytes = name_bytes; cursor += name_bytes;
        e->offset = u32(data + cursor); e->unpacked_bytes = u32(data + cursor + 4);
        e->packed_bytes = u32(data + cursor + 8); e->selection_tag = u32(data + cursor + 12); cursor += 16;
        if (!e->packed_bytes && e->unpacked_bytes) { error = UC_AR_UNSUPPORTED_LAYOUT; goto malformed; }
        if (e->offset > size || e->packed_bytes > size - e->offset ||
            (e->packed_bytes && e->offset < end)) goto malformed;
    }
    if (cursor != end) goto malformed;
    *out = a; return UC_AR_OK;
malformed:
    if (a) uc_archive_close(a); else free(data);
    return error;
}
UCArchiveStatus uc_archive_copy(const void *bytes, size_t size, UCArchive **out) {
    if (!out) return UC_AR_ARGUMENT;
    *out = NULL;
    if (!bytes && size) return UC_AR_ARGUMENT;
    if (size > UC_AR_MAX_FILE_BYTES) return UC_AR_LIMIT;
    uint8_t *data = (uint8_t *)malloc(size ? size : 1);
    if (!data) return UC_AR_MEMORY;
    if (size) memcpy(data, bytes, size);
    return parse_owned(data, size, out);
}
UCArchiveStatus uc_archive_open(const char *path, UCArchive **out) {
    if (!out) return UC_AR_ARGUMENT;
    *out = NULL;
    if (!path) return UC_AR_ARGUMENT;
    FILE *f = fopen(path, "rb");
    if (!f) return UC_AR_IO;
    if (fseek(f, 0, SEEK_END)) { fclose(f); return UC_AR_IO; }
    long length = ftell(f);
    if (length < 0 || fseek(f, 0, SEEK_SET)) { fclose(f); return UC_AR_IO; }
    if ((unsigned long)length > UC_AR_MAX_FILE_BYTES) { fclose(f); return UC_AR_LIMIT; }
    size_t size = (size_t)length;
    uint8_t *data = (uint8_t *)malloc(size ? size : 1);
    if (!data) { fclose(f); return UC_AR_MEMORY; }
    int okay = fread(data, 1, size, f) == size && !ferror(f);
    if (fclose(f)) okay = 0;
    if (!okay) { free(data); return UC_AR_IO; }
    return parse_owned(data, size, out);
}
size_t uc_archive_count(const UCArchive *a) { return a ? a->count : 0; }
const UCArchiveEntry *uc_archive_entry(const UCArchive *a, size_t index) {
    return a && index < a->count ? &a->entries[index] : NULL;
}
uint16_t uc_archive_external_stream_id(const UCArchive *a) { return a ? u16(a->data + 0x16) : 0; }
void uc_archive_bytes_free(UCArchiveBytes *b) {
    if (b) { free(b->data); b->data = NULL; b->size = 0; }
}
UCArchiveStatus uc_archive_read(const UCArchive *a, size_t index, UCArchiveSource source_mode, UCArchiveBytes *out) {
    if (!out) return UC_AR_ARGUMENT;
    *out = (UCArchiveBytes){0};
    const UCArchiveEntry *e = uc_archive_entry(a, index);
    if (!e) return UC_AR_ARGUMENT;
    if (source_mode == UC_AR_EXTERNAL_STREAM) return UC_AR_UNSUPPORTED_SOURCE;
    if (source_mode != UC_AR_LOCAL_PAYLOAD) return UC_AR_ARGUMENT;
    if (e->unpacked_bytes > UC_AR_MAX_ENTRY_BYTES) return UC_AR_LIMIT;
    uint8_t *data = (uint8_t *)malloc(e->unpacked_bytes ? e->unpacked_bytes : 1);
    if (!data) return UC_AR_MEMORY;
    const uint8_t *source = a->data + e->offset;
    /* Original 8DCC0 constructs an inflate stream for every packed payload.
     * Equal encoded/decoded lengths do not identify an uncompressed entry. */
    if (e->packed_bytes || e->unpacked_bytes) {
        uLongf decoded = e->unpacked_bytes ? e->unpacked_bytes : 1;
        uLong consumed = e->packed_bytes;
        int result = uncompress2(data, &decoded, source, &consumed);
        if (result != Z_OK || decoded != e->unpacked_bytes || consumed != e->packed_bytes) {
            free(data); return UC_AR_DECOMPRESSION;
        }
    }
    out->data = data; out->size = e->unpacked_bytes; return UC_AR_OK;
}
const char *uc_archive_status(UCArchiveStatus s) {
    static const char *names[] = {"ok", "argument", "io", "format", "limit", "memory", "unsupported-source", "decompression", "unsupported-layout"};
    return (unsigned)s < sizeof(names) / sizeof(names[0]) ? names[s] : "unknown";
}
