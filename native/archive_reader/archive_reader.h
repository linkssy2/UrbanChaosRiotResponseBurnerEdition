#ifndef UC_NATIVE_ARCHIVE_READER_H
#define UC_NATIVE_ARCHIVE_READER_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Native .lst/.ent directory reader. No guest addresses, runtime, or extraction
 * to the filesystem. Names are exact byte strings, not normalized host paths.
 * The archive owns its input snapshot and entries until uc_archive_close(). */
typedef struct UCArchive UCArchive;
typedef struct UCArchiveEntry {
    const uint8_t *name;
    uint16_t name_bytes;
    uint32_t offset, unpacked_bytes, packed_bytes, selection_tag;
} UCArchiveEntry;
typedef struct UCArchiveBytes { uint8_t *data; size_t size; } UCArchiveBytes;
typedef enum UCArchiveStatus {
    UC_AR_OK, UC_AR_ARGUMENT, UC_AR_IO, UC_AR_FORMAT, UC_AR_LIMIT,
    UC_AR_MEMORY, UC_AR_UNSUPPORTED_SOURCE, UC_AR_DECOMPRESSION, UC_AR_UNSUPPORTED_LAYOUT
} UCArchiveStatus;
typedef enum UCArchiveSource { UC_AR_LOCAL_PAYLOAD, UC_AR_EXTERNAL_STREAM } UCArchiveSource;

/* Deliberate input/resource limits, not claims about every original variant.
 * Packed-zero/nonempty entries have unresolved original storage semantics and
 * return UNSUPPORTED_LAYOUT. These are not asserted to be malformed assets. */
#define UC_AR_MAX_FILE_BYTES (512u * 1024u * 1024u)
#define UC_AR_MAX_ENTRY_BYTES (256u * 1024u * 1024u)
UCArchiveStatus uc_archive_open(const char *path, UCArchive **out);
UCArchiveStatus uc_archive_copy(const void *bytes, size_t size, UCArchive **out);
void uc_archive_close(UCArchive *archive);
size_t uc_archive_count(const UCArchive *archive);
const UCArchiveEntry *uc_archive_entry(const UCArchive *archive, size_t index);
uint16_t uc_archive_external_stream_id(const UCArchive *archive);
/* Entry names include their original trailing NUL; returned metadata pointers
 * expire at close. selection_tag is preserved without language filtering.
 * Caller must explicitly choose LOCAL_PAYLOAD; external .saf routing is not
 * implemented and returns UNSUPPORTED_SOURCE. A nonzero header stream id alone does not
 * select .saf: the original constructor also receives a separate routing flag.
 * Pass an empty output (or free a previous result first). Output owns an
 * independent allocation; failures leave {NULL,0}. */
UCArchiveStatus uc_archive_read(const UCArchive *archive, size_t index, UCArchiveSource source, UCArchiveBytes *out);
void uc_archive_bytes_free(UCArchiveBytes *bytes);
const char *uc_archive_status(UCArchiveStatus status);
#ifdef __cplusplus
}
#endif
#endif
