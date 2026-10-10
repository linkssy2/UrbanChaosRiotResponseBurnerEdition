#ifndef UC_ENTITY_PROPERTIES_H
#define UC_ENTITY_PROPERTIES_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCEntityProperty {
    uint32_t type;
    const char *name;
    uint32_t scalar,reference,vector[3],pair[2];
} UCEntityProperty;
typedef struct UCEntityProperties {
    int32_t source_index;
    const void *definition;
    uint32_t count;
    UCEntityProperty entries[15];
    uint32_t flags,word2A0,placement_mode,word2A8,word2AC;
} UCEntityProperties;
/* Actual159C0 dynamic/NULLstream descriptor projection. All storage already
 * exists; source-unwritten scalar/vector fields keep caller-provided bits.
 * No serialized stream, allocation, ownership of names or general factory. */
int uc_entity_properties_begin(UCEntityProperties *,int32_t source_index);
/* Whole1F8DE0/1F8AE0 with actual temporary-record provenance explicit. Scratch
 * starts with initialized caller-owned bytes and is disjoint from the table.
 * The source initializes reference/pair before reading value, so value may
 * alias scratch payload. Names are stable borrowed NUL strings <=1023 bytes.
 * Full15 capacity rejects before effects (source overflow is not admitted).
 * Payload is opaque source-width bits, including reference words, not a host
 * pointer conversion. Source-unwritten temporary fields are copied unchanged. */
int uc_entity_properties_append(UCEntityProperties *,UCEntityProperty *scratch,
    uint32_t type,const char *name,const void *value);
/* Whole1F8DC0 NULLstream→1F8C90. First case-sensitive name wins, including
 * duplicates. REQUESTED type chooses the source field; stored type is NOT
 * checked. Unwritten payload requires explicit caller provenance as above.
 * Unknown types/7 or missing names yield found0 and preserve output bytes.
 *0 complete,1 invalid before effects (including found unchanged). Exclusive
 * stable table/name lifetimes; output is disjoint from table/name storage and
 * fits the chosen type. Scratch is disjoint from the table and stable names.
 * No stream-backed1F8BB0 or source-default property values are invented. */
int uc_entity_properties_get(const UCEntityProperties *,uint32_t type,
    const char *name,void *value,int *found);
#ifdef __cplusplus
}
#endif
#endif
