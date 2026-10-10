#ifndef UC_ENTITY_STREAM_H
#define UC_ENTITY_STREAM_H
#include "level_asset.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual1F8BB0 stream branch with221140/160/190/250. cursor aliases the
 * descriptor's current property pointer, initially properties.data. Scalar
 * types1/2/3 copy4 bytes,4 copies12,5 copies8. Types6/8..11 consume a signed
 * link index and publish const char*. Type7 completes without reading/writing.
 * Unknown types complete found0. NULL scalar output completes without advance;
 * string output must exist. Bounds failures reject before effects. Loaded
 * descriptor/links and output/cursor slots remain stable/disjoint. No name
 * lookup, arithmetic conversion, default value or independent cursor owner. */
int uc_entity_stream_get(const UCLevelEntity *,const uint8_t **cursor,
    uint32_t type,void *output,int *found);
#ifdef __cplusplus
}
#endif
#endif
