#ifndef UC_ARCHIVE_LOOKUP_H
#define UC_ARCHIVE_LOOKUP_H
#include "archive_reader/archive_reader.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Borrowed immutable archive, explicit selected language/variant tag. Original
 * 8D670 accepts tag0 or this tag. 8C830 first tries the next accepted entry,
 * then the last case-insensitive match; misses preserve the cursor. Initialize
 * once for each archive owner, retain across requests. ASCII C-locale names
 * only. Slash normalization and resource-cache ordering belong to the caller.
 * Success0, invalid/unsupported1. A successful miss returns SIZE_MAX. */
typedef struct UCArchiveLookup {
    const UCArchive *archive;
    size_t cursor;
    uint16_t selection_tag;
} UCArchiveLookup;
int uc_archive_lookup_init(UCArchiveLookup *,const UCArchive *,uint16_t selection_tag);
int uc_archive_lookup(UCArchiveLookup *,const char *name,size_t *selected);
/*20D630 resource-loader key: first # ends bank path; lower ASCII and collapse
 * either slash to one backslash. Caller keeps the event suffix separately.
 * Bounded PC admission for the original loader's256-byte path storage. */
int uc_resource_name(const char *name,char *out,size_t capacity);
#ifdef __cplusplus
}
#endif
#endif
