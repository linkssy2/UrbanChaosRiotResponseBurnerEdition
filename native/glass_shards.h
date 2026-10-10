#ifndef UC_GLASS_SHARDS_H
#define UC_GLASS_SHARDS_H
#include "vector_random.h"
#include "game_random.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCGlassShard {
    float triangle[3][2],center[2],velocity[3],axis[3],spin;
} UCGlassShard;
typedef struct UCGlassShards {
    UCVectorRandom *vectors;
    UCGameRandom *random;
    void *owner;
    int (*preflight)(void *,const float quad[4][2],const float impact[6]);
    /* Actual53520 owner+4 virtual+4: one complete shard publication per call.
     * Packet is borrowed only through return. Completing service may consume
     * these SAME shared random streams, preserving valid states. It must keep
     * this descriptor, input arrays and arithmetic environment unchanged. */
    int (*submit)(void *,const UCGlassShard *);
} UCGlassShards;
/* Actual53520/53620 ordered subdivision: first triangle012, then230; each uses
 * a temporary LIFO vector, longest-edge tie rules, shared random draws, source
 * chance and60-byte packet. No entity/factory/renderer success is fabricated.
 * Native malloc/realloc/free replaces unobservable temporary vector storage;
 * no guest-layout/allocation-address claim. Positive-base CRT pow is host math.
 * RN, DAZ/FTZ off; finite quad abs<=4096, impact abs<=1e6; disjoint stable inputs
 * and initialized shared streams. Pure preflight guarantees completing sink.
 * Return0 complete,1 before RNG/publication effects,2 after possible committed
 * RNG/shard effects (allocation or sink failure: no rollback/retry).
 */
int uc_glass_shards_emit(const UCGlassShards *,const float quad[4][2],const float impact[6]);
#ifdef __cplusplus
}
#endif
#endif
