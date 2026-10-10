#ifndef UC_NATIVE_PLAYER_FOOTSTEP_H
#define UC_NATIVE_PLAYER_FOOTSTEP_H
#include "collision_result.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCPlayerFootstep {
    float body[16]; /* Actor+10 frame, including actor+40 translation. */
    uint32_t side,material; /* Actor+78C and published+80. */
    uint16_t actor_id; /* Actor+94. */
} UCPlayerFootstep;
typedef struct UCPlayerFootprint {
    const char *shader;
    float point[3],normal[3],tangent[3],size[2];
    uint32_t mask,flags;
} UCPlayerFootprint;
typedef struct UCPlayerFootstepServices {
    void *context;
    int (*preflight)(void *,const UCPlayerFootstep *);
    const UCCollisionResult *(*query)(void *,const float start[3],const float end[3],
        uint32_t type,uint32_t actor_id,uint32_t mask,float impulse);
    void (*footprint)(void *,const UCPlayerFootprint *);
} UCPlayerFootstepServices;
/* FullC3310, concrete3BD10 and its fixed2D3DC0 quarter-turn. Side0/1, finite
 * frame/calculations, RN with DAZ/FTZ off. Caller supplies real completed214230
 * results or genuine miss; footprint is REQUIRED completing2088D0 service.
 * Neither query nor effects are silently replaced. Query may update side0/1
 * (the footprint shader uses its CURRENT value), but frame/id and hit storage
 * remain stable. Preflight is pure; all services must complete synchronously,
 * without reentry or invalidating owners. All spans/owners are disjoint.
 * Returns0 success,1 invalid/unsupported. Pre-query rejection preserves state.
 * Hit material publishes before footprint processing as source. Invalid hit
 * geometry is a completing-service violation after query/material commitment;
 * such an error is NOT retryable. This owns no world, decal pool or rendering. */
int uc_player_footstep(UCPlayerFootstep *,const UCPlayerFootstepServices *);
#ifdef __cplusplus
}
#endif
#endif
