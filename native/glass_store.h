#ifndef UC_GLASS_STORE_H
#define UC_GLASS_STORE_H
#include "glass_shards.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCGlassRecord {
    uint8_t active,unwritten[3];
    float triangle[3][3],position[3],velocity[3],floor_triangle[3][2],rotation[4];
} UCGlassRecord;
/* One actual shared50E668 pool:32 blocks of8 records. Init changes ONLY used.
 * Keep this owner alive and at a stable address until every borrowing store is
 * destroyed. Never reset a pool while a store has an allocated block. */
typedef struct UCGlassPool {uint8_t used[32];UCGlassRecord records[32][8];} UCGlassPool;
typedef struct UCGlassBlock {
    struct UCGlassBlock *next,*previous;
    uint32_t index,cursor;
} UCGlassBlock;
typedef struct UCGlassGround {
    void *context;
    /* Actual50A00/526F0 owner services. Packet is3 XY corners then velocityXY.
     * Complete storage/destruction, not a notification or successful no-op. */
    int (*submit)(void *,const float packet[8]);
    void (*destroy)(void *);
} UCGlassGround;
typedef struct UCGlassStore {
    float matrix[16],floor;
    UCGlassPool *pool;
    UCGlassGround *ground;
    UCGlassBlock *sentinel;
    uint32_t blocks;
} UCGlassStore;
typedef struct UCGlassTriangle {struct {float position[3],uv[2];} vertex[3];float alpha;} UCGlassTriangle;
void uc_glass_pool_init(UCGlassPool *);
/* Actual50E10 represented fields. One real circular sentinel allocation;
 * source vtable words/unwritten bytes are not duplicated native metadata.
 * Fresh destination; finite matrix/floor; ground NULL is legal for storage,
 * but update requires complete ground services. Ground lifetime is transferred
 * to store destruction. Its descriptor remains borrowed and must outlive it. */
int uc_glass_store_init(UCGlassStore *,UCGlassPool *,const float matrix[16],float floor,UCGlassGround *);
int uc_glass_store_valid(const UCGlassStore *);
/* Actual52A00/52EE0/52F90, including first-free shared block and single cursor
 * test (NOT a search within a block). Pool exhaustion drops the remaining
 * packets, returns0 with accepted<count. Source signed count<=0 is quiet.
 * RN, DAZ/FTZ off; stable disjoint descriptor/inputs; source IEEE NaN output is
 * preserved. Return1 before effects,2 after possible partial allocation/shard
 * effects; no retry/rollback. Native node malloc failure is defensive only. */
int uc_glass_store_submit(UCGlassStore *,const UCGlassShard *,int32_t count,uint32_t *accepted);
/* Actual51100 gravity, fixed per-update quaternion rotation (not delta-scaled),
 * shared vector RNG bounce and526F0 publication. Removes a block only when it
 * had NO active record on entry, retaining newly emptied blocks until next
 * update. Ground may consume the SAME stream but must keep all storage/list,
 * descriptor, input and arithmetic state stable and complete each publication.
 * Finite delta in[0,1], initialized stream; return0/1/2 as above. */
int uc_glass_store_update(UCGlassStore *,UCVectorRandom *,float delta);
/* Actual50ED0, fixed UV/alpha, current list/slot order. Caller owns one scratch
 * buffer of257 entries, standing in for original shared509B20; live count only
 * is written and stale tail retained. This produces renderer input, not pixels. */
int uc_glass_store_triangles(const UCGlassStore *,UCGlassTriangle scratch[257],uint32_t *count);
/* Actual517C0/50960: destroy ground FIRST, release all pool blocks, then list
 * nodes/sentinel. Stable graph/nonreentrant completing ground destructor.
 * Does not free the enclosing store. Return1 before effects, otherwise0. */
int uc_glass_store_destroy(UCGlassStore *);
#ifdef __cplusplus
}
#endif
#endif
