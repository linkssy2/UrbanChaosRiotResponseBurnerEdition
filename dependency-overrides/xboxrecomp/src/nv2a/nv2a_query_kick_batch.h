#ifndef NV2A_QUERY_KICK_BATCH_H
#define NV2A_QUERY_KICK_BATCH_H
#include <stdint.h>
#include "nv2a_pgraph_d3d11.h"

/* Owner-thread submission cadence, not query completion or a background timer.
 * Call only for an unsubmitted query whose real GetData returned not-ready.
 * Decode/GET publication and resource ownership remain synchronous. Hard guest
 * progress, capacity waits and unknown callbacks must never be throttled.
 * Budget counts gate attempts (both queues can ask), not guest kicks. */
typedef struct NvQueryKickBatch {
    uint64_t last_submit;
    unsigned deferred,valid;
} NvQueryKickBatch;
enum { NV_QUERY_KICK_MAX_DEFERRED = 8 };

static int nv_query_kick_allow(NvQueryKickBatch *s,int enabled,int cpu_only,
                              unsigned reason,uint64_t now,uint64_t max_ticks){
    if(!enabled || !cpu_only || reason!=NV_WAIT_KICK || !max_ticks ||
       !s->valid || now<s->last_submit || now-s->last_submit>=max_ticks ||
       s->deferred>=NV_QUERY_KICK_MAX_DEFERRED)return 1;
    ++s->deferred;
    return 0;
}
/* Only AFTER a real Flush on the owning immediate context, never on allow or
 * deferred polls. Queries ended later are not covered by that submission. */
static void nv_query_kick_submitted(NvQueryKickBatch *s,uint64_t now){
    s->last_submit=now;s->deferred=0;s->valid=1;
}
#endif
