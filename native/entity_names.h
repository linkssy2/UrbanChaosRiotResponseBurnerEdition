#ifndef UC_ENTITY_NAMES_H
#define UC_ENTITY_NAMES_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCEntityNames UCEntityNames;
/* Actual1F88E0/1F89D0 case-sensitive name -> insertion-ordered unique IDs.
 * Names are borrowed immutable NUL-terminated strings, valid until close.
 * Equal spellings retain the original borrowed key. Strings<=1023 bytes,
 * at most4096 names and65536 IDs/name; all int32 IDs, including-1, are legal.
 * Allocation effects are unobserved and owners disjoint/exclusive.0 complete,
 *1 invalid/allocation failure before adding an ID. A failed first insertion
 * may leave an empty name entry; no exception escapes this C interface. */
int uc_entity_names_create(UCEntityNames **);
int uc_entity_names_bind(UCEntityNames *,const char *,int32_t);
/* Actual1F7E20: remove first matching ID stably, retain name/capacity, miss is
 * complete. Native live-vector projection does not expose original stale tail. */
int uc_entity_names_remove(UCEntityNames *,const char *,int32_t);
/* Borrowed ID view valid until next bind/remove/close; missing name returnsNULL/count0.
 * This is a projection of the actual stored vector, not a second identity map. */
int uc_entity_names_view(const UCEntityNames *,const char *,const int32_t **,uint32_t *);
void uc_entity_names_close(UCEntityNames *);
#ifdef __cplusplus
}
#endif
#endif
