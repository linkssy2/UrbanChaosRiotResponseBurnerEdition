#ifndef UC_NATIVE_COLLISION_FIXED_ISLAND_H
#define UC_NATIVE_COLLISION_FIXED_ISLAND_H
#include "collision_asset.h"

#define UC_COLLISION_FIXED_DETACHED UINT16_MAX
#define UC_COLLISION_FIXED_MAX_MEMBERS 65535u
typedef struct UCCollisionFixedIsland UCCollisionFixedIsland;
typedef struct UCCollisionFixedMember {
    const void *identity;
    const void *world;
    UCCollisionFixedIsland *island;
    uint16_t index;
    uint8_t fixed;
} UCCollisionFixedMember;
typedef struct UCCollisionFixedIslandView {
    const void *world;
    UCCollisionFixedMember *const *members;
    uint32_t count, accumulator;
    uint16_t storage_index, config_1bf8, config_1bfc;
    uint8_t dirty;
} UCCollisionFixedIslandView;

/* Fixed 2FD0E0 + 2EE310 designation, then fixed 2F6730/2F6F50 membership.
 * Create is EMPTY, not an initialized world: the world owner must construct
 * and register its original shape-null built-in body through its ordinary
 * add/ref lifecycle before external bodies. Source configuration values are
 * truncated to uint16 exactly as the constructor; they are not guessed here.
 * Each identity has one stable member; it and its owner are BORROWED until removal. Initialize
 * detached members as {identity,NULL,NULL,UINT16_MAX,1}. Attached fields and view
 * are read-only to callers. World ownership, references, SAP, pair/listener
 * dispatch, pending work and dynamic islands belong to the outer world owner.
 * Add appends; remove moves the last member into the gap and sets sticky dirty.
 * All capacity is allocated at create (1..65535). Failure changes nothing.
 * Free requires an empty island, so it cannot silently bypass world removal.
 * No concurrent mutation; views remain valid until free. */
#ifdef __cplusplus
extern "C" {
#endif
UCCollisionStatus uc_collision_fixed_island_create(const void *world,
    uint32_t capacity, uint32_t config_1bf8, uint32_t config_1bfc,
    UCCollisionFixedIsland **out);
UCCollisionStatus uc_collision_fixed_island_free(UCCollisionFixedIsland *island);
const UCCollisionFixedIslandView *uc_collision_fixed_island_view(
    const UCCollisionFixedIsland *island);
UCCollisionStatus uc_collision_fixed_island_add(UCCollisionFixedIsland *island,
    UCCollisionFixedMember *member);
UCCollisionStatus uc_collision_fixed_island_remove(UCCollisionFixedIsland *island,
    UCCollisionFixedMember *member);
#ifdef __cplusplus
}
#endif
#endif
