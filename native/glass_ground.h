#ifndef UC_GLASS_GROUND_H
#define UC_GLASS_GROUND_H
#include "glass_store.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCGlassGroundRecord {uint8_t active,unwritten[3];float triangle[3][2],velocity[2],alpha,fade;} UCGlassGroundRecord;
typedef struct UCGlassGroundPool {uint8_t used[16];UCGlassGroundRecord records[16][4];} UCGlassGroundPool;
typedef struct UCGlassGroundStore {
    UCGlassGroundPool *pool;
    UCVectorRandom *random;
    UCGlassBlock *sentinel;
    uint32_t blocks;
    float floor;
    UCGlassGround service;
} UCGlassGroundStore;
/* Exact shared50780/50DB20 allocator:16 blocks x4 records. Init changes only
 * used; pool and shared RNG must remain alive/stable until all stores die. */
void uc_glass_ground_pool_init(UCGlassGroundPool *);
/* Actual50A00 fields plus real native enclosing/sentinel allocations. Returned
 * owner.service binds actual submit/destruction for UCGlassStore; destroying
 * an airborne owner therefore destroys this ground owner FIRST, as original.
 * No duplicate record/RNG state. One owner per service; never double-destroy. */
UCGlassGroundStore *uc_glass_ground_create(UCGlassGroundPool *,UCVectorRandom *,float floor);
int uc_glass_ground_valid(const UCGlassGroundStore *);
/*526F0 consumes THREE shared uniform-vector draws per accepted triangle, even
 * though only the final fade value survives. Fixed-pool exhaustion stops input
 * without RNG draws for unaccepted packets. Signed count<=0 is source-quiet.
 * Return0complete,1before effects,2possible partial effects/native allocation
 * failure. accepted reports actual stored packets. Stable disjoint inputs,
 * valid constructed graphs, RN/gradual underflow; no callbacks/reentrancy. */
int uc_glass_ground_submit(UCGlassGroundStore *,const float (*packets)[8],int32_t count,uint32_t *accepted);
/* Whole actual entry50C20 (preceding INT3 padding is not executable method):
 * alpha below0 retires; alpha==0 still moves. Friction min(delta*4,.99).
 * A block emptied by this update is freed only on the following update.
 * Finite delta[0,1]. No render/simulation scheduling claim. */
int uc_glass_ground_update(UCGlassGroundStore *,float delta);
/* Whole50A90, same caller-owned shared scratch as airborne50ED0; draw consumer
 * must finish before another owner overwrites it. Only live prefix is written. */
int uc_glass_ground_triangles(const UCGlassGroundStore *,UCGlassTriangle scratch[256],uint32_t *count);
/* Actual50DB0/50960 + deleting50D90 lifetime; frees this owner last. */
int uc_glass_ground_destroy(UCGlassGroundStore *);
#ifdef __cplusplus
}
#endif
#endif
