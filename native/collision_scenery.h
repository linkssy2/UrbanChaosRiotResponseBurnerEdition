#ifndef UC_NATIVE_COLLISION_SCENERY_H
#define UC_NATIVE_COLLISION_SCENERY_H
#include "collision_hit.h"
#include "collision_asset.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCCollisionSceneryBody UCCollisionSceneryBody;
/* The decoded asset and its backing resources stay immutable until release.
 * These are real resource-owner operations, not notifications. */
typedef struct UCCollisionSceneryResource {
    const UCCollisionAsset *asset;
    void *context;
    void (*retain)(void *);
    void (*release)(void *);
} UCCollisionSceneryResource;
typedef struct UCCollisionSceneryView {
    const UCCollisionAsset *asset;
    UCCollisionBodyMetadata collidable;
    float transform[4][4]; /* Three rotation columns then translation; all W=0. */
    uint32_t collidable_type,impulse_disabled; /* Original1 and body+92=1. */
    uint64_t property_1; /* Only property assigned by22A120; value0.
                         * Properties0x14/0x2D absent, and no mutators admitted. */
} UCCollisionSceneryView;
typedef struct UCCollisionScenery { UCCollisionSceneryBody *body; } UCCollisionScenery;
typedef enum UCCollisionSceneryWorldResult {
    UC_SCENERY_APPLIED, UC_SCENERY_QUEUED, UC_SCENERY_REJECTED
} UCCollisionSceneryWorldResult;
/* A real world owns references independently of the scene. Both accepted add
 * variants must retain the body. A queued remove must retain its queue record;
 * draining it releases that reference even when the operation is skipped.
 * The world also releases its registered-body reference on actual removal.
 * Original pending drain is recursive, not a simple FIFO; not implemented here.
 * Callbacks must not mutate/reenter this scene or mutate the immutable view. */
typedef struct UCCollisionSceneryWorld {
    void *context;
    UCCollisionSceneryWorldResult (*add)(void *,UCCollisionSceneryBody *,int activate);
    UCCollisionSceneryWorldResult (*remove)(void *,UCCollisionSceneryBody *);
} UCCollisionSceneryWorld;
/* Successful loaded-root tail of22A120. Caller already owns the227640/227140
 * resource projection; failed lookup never enters this function. Original
 * overwrites projection51CA74 before lookup, without destroying an old one.
 * Its archive/projection management is explicitly outside this body API.
 * Incoming resource/callback records and the asset need independent ownership
 * through this call: never borrow them solely from the body being replaced.
 * Removes/releases the previous body before allocating/retaining the new one,
 * publishes the new body, then calls world.add(...,1). OK means the callback
 * accepted (applied OR queued), not that broadphase registration has completed.
 * A rejected callback or allocation failure is explicit and may leave partial
 * state: rejected remove leaves old body; rejected add leaves new current body.
 * No rollback of external world/resource effects is claimed. */
UCCollisionStatus uc_collision_scenery_replace(UCCollisionScenery *,
    const UCCollisionSceneryResource *,const UCCollisionSceneryWorld *);
/*22A440 body phase: world.remove, release scene reference, clear current body.
 * Caller then destroys the2273C0 projection and clears its owner. Empty scene
 * is rejected; original22A440 requires a live current body. */
UCCollisionStatus uc_collision_scenery_remove(UCCollisionScenery *,const UCCollisionSceneryWorld *);
/* Single-threaded live-owner references. Never call with a released pointer.
 * Final release relinquishes the retained resource and frees stable identity. */
UCCollisionStatus uc_collision_scenery_retain(UCCollisionSceneryBody *);
void uc_collision_scenery_release(UCCollisionSceneryBody *);
const UCCollisionSceneryView *uc_collision_scenery_view(const UCCollisionSceneryBody *);
uint32_t uc_collision_scenery_references(const UCCollisionSceneryBody *);
#ifdef __cplusplus
}
#endif
#endif
