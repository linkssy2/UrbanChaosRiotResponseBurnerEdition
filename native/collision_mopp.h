#ifndef UC_NATIVE_COLLISION_MOPP_H
#define UC_NATIVE_COLLISION_MOPP_H
#include "collision_asset.h"
#ifdef __cplusplus
extern "C" {
#endif

/* MOPP program view. For prepare/cast, asset code/info must remain immutable and
 * alive through every cast; zero-initialize, then prepare once per asset.
 * Preparation also certifies whole-DAG expanded work, both-branch call depth
 * and accumulated rescale shift. The public layout is unchanged; constructing
 * a view by hand or mutating its bytes/info bypasses that required contract. */
typedef struct UCCollisionMopp {
    const uint8_t *bytes;
    uint32_t size;
    float info[4];
} UCCollisionMopp;
UCCollisionStatus uc_collision_mopp_prepare(const UCCollisionAsset *,UCCollisionMopp *);

/* Ordered terminal key from32C1C0. The context owns the ORIGINAL full ray and
 * scratch hit collector: terminal filter -> mesh child -> triangle -> closest.
 * Code/info and both borrowed endpoint arrays must remain unchanged, and the
 * required FP arithmetic mode must remain in force, including across callbacks.
 * May retain/decrease fraction in [0,1]; no callback rollback is implied.
 * Return a failure to stop traversal without publishing *fraction. Commit the
 * scratch collector only after the whole cast succeeds. */
typedef UCCollisionStatus (*UCCollisionMoppTerminal)(void *context,uint32_t key,float *fraction);
/* Original32D060 -> enabled32C1C0 collector path (context+58==0). Physics
 * float4 endpoints retain W. No broadphase, body transform, filtering, triangle
 * query, profiler or34B850 enable-gate implementation is hidden here. Internal
 * branch clipping does not change the full ray supplied to terminal owners.
 * Preparation rejects malformed/unsupported bytecode and excessive complete
 * program depth/work/rescale before any callback, including shared subgraphs.
 * Query limits and callback/numerical failure leave *fraction unchanged;
 * callback context must be caller-owned scratch. View/output must not alias.
 * Native bounds:64MiB code,256 nested calls,4194304 expanded instructions,
 * accumulated rescale shift32. Entry requires |from/to/info|<=2^32 and
 * info.w>=2^-32, before any callback, to prevent internal floating overflow.
 * Finite forward fraction domain; rejects non-RN arithmetic or DAZ/FTZ at entry.
 * These arithmetic/input preconditions apply throughout the call; entry checks
 * do not authorize callbacks to mutate borrowed inputs or the FP environment.
 * These guards do NOT make arbitrary callbacks transactional: their error or
 * invalid fraction may follow effects. No original-query replay is authorized. */
UCCollisionStatus uc_collision_mopp_cast(const UCCollisionMopp *,
    const float from[4],const float to[4],float *fraction,
    UCCollisionMoppTerminal,void *context);

/* Separate speculative entry for stable bytes borrowed only during this call.
 * Does NOT require prepare or certify unvisited instructions. It bounds size,
 * every visited decode/target, original call depth, expanded steps and each
 * path's rescale sum; unsupported/malformed/over-limit execution may fail
 * AFTER terminals. Both target addresses are checked even if one is unvisited.
 * Terminals MUST be pure with respect to guest/shared state: update exclusively
 * private caller scratch, never invoke guest callbacks, pump or yield. Keep
 * code/info/full ray and FP mode stable until return; discard all scratch on
 * failure. Publish only after success and after checking read/output aliases.
 * No guest pointer is retained. This contract authorizes no replay after any
 * externally visible effect. The prepared entry's contract remains unchanged. */
UCCollisionStatus uc_collision_mopp_cast_borrowed_staged(const UCCollisionMopp *,
    const float from[4],const float to[4],float *fraction,
    UCCollisionMoppTerminal,void *context);

#ifdef __cplusplus
}
#endif
#endif
