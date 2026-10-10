#ifndef UC_NATIVE_FOOTPRINT_H
#define UC_NATIVE_FOOTPRINT_H
#include "player_footstep.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCFootprintServices {
    void *context;
    int (*preflight)(void *,const UCPlayerFootprint *,float fade);
    const UCCollisionResult *(*query)(void *,const float start[3],const float end[3],
        uint32_t type,uint32_t actor_id,uint32_t mask,float impulse);
    void *(*manager)(void *); /* Current graphics virtual+4C; genuinely nullable. */
    void *(*material_cache)(void *); /* Actual22B210/2E0E00 cache, required nonnull. */
    void (*publish)(void *,void *manager,void *cache,const char *shader,
        const float corners[4][3],uint32_t flags,float fade);
} UCFootprintServices;
/* Original223450 supplied-tangent branch; no random orientation substitute.
 * RN/DAZ-FTZ off, finite inputs/intermediates, tangent not parallel to normal.
 * Returns0 success,1 invalid/unsupported; failure preserves corners. Inputs,
 * output and services are valid/disjoint and immutable for each call. */
int uc_footprint_quad(const UCPlayerFootprint *,float corners[4][3]);
/* Full2088D0/223BC0 policy with the supplied-tangent geometry above. shaderNULL
 * is the original no-request branch. Otherwise pure preflight must admit every
 * completing query/cache/publish effect and stable borrowed owner lifetime.
 * Four rays occur in original corner order unless flags mask0x04 skips them;
 * the first miss returns normally. Manager is reloaded AFTER queries, then
 * captured before resolving the cache and invoking actual publication.
 * submitted means the required publish service was invoked, not pixels shown.
 * Returns0 success,1 invalid/unsupported. CacheNULL after queries is a violated
 * completing contract, not a retryable untouched request. No world, cache,
 * material, ring, lighting or rendering effects are fabricated here. */
int uc_footprint_submit(const UCPlayerFootprint *,float fade,const UCFootprintServices *,uint8_t *submitted);
#ifdef __cplusplus
}
#endif
#endif
