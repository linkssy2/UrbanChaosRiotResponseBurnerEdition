#ifndef UC_GLASS_ENTITY_H
#define UC_GLASS_ENTITY_H
#include "glass_proxy.h"
#include "glass_container.h"
#include "bullet_observers.h"
#include "level_asset.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCGlassEntityProperties {
    float left,right,up,down,maximum,collision_height;
    uint32_t auto_extents,on_broken[2];
    uint8_t npc_smash;
    const char *shader;
} UCGlassEntityProperties;
/* Actual18A560 ordered stream fields; borrows the SAME qualified level owner.
 * Exactly44 property bytes and an in-range Shader link (or original-1 empty
 * string) are admitted. No invented defaults or dynamic descriptor parsing.
 *0complete,1invalid, with output unchanged on rejection. */
int uc_glass_entity_properties(const UCLevelEntity *,UCGlassEntityProperties *);
/* Embed once in the actual entity. The observer is the native interface identity
 * for its container, not another simulation owner. Stable/nonmovable until all
 * observer/group borrows end. The base entity owns the transform and name. */
typedef struct UCGlassEntity {
    UCGlassProxy *model74;
    UCGlassContainer *container11C;
    void *bank114;
    uint32_t state118;
    UCBulletObserver observer;
} UCGlassEntity;
typedef struct UCGlassEntityStart {
    const UCGlassModelFactory *factory;
    const UCModelSupportMap *collision,*entity;
    UCGlassPool *air_pool;
    UCGlassGroundPool *ground_pool;
    UCVectorRandom *vectors;
    UCGameRandom *random;
    UCGlassPaneUV *uv;
    void *context;
    UCMaterialResources *(*current_resources)(void *);
    UCBulletObservers *(*current_observers)(void *);
    /* Pure admission promises the actual216020/208980 bank lookup completes.
     * load_bank executes AFTER observer registration. It must construct/resolve
     * real Banks\\Glass.amp resource ownership, not report a fabricated NULL success.
     * It may returnNULL only for a genuine source failure/absent asset branch.
     * ctx identifies the actual entity; output becomes its actual+114 field. */
    int (*bank_preflight)(void *,const char *);
    int (*load_bank)(void *,const char *,void **);
} UCGlassEntityStart;
/* Whole189AE0 scheduling with real proxy/container/observer owners. Borrowed
 * transform/name/properties/maps remain stable, disjoint and live; descriptor
 * is fresh with model/containerNULL. Allocations and three wrapper slots must
 * be admitted; callbacks may change external current resource/observer owners,
 * but preserve this graph and input values. Native malloc has no game-visible
 * allocator callbacks. Current observer is re-read AFTER container publication.
 *0complete,1pre-effect rejection,2committed failure: never retry; fields retain
 * actual partial ownership for caller cleanup. No base/property factory,
 * coroutine, break-sound/event update, generic model/actor deletion or main.
 * Already registered entity provenance belongs to the caller. */
int uc_glass_entity_start(UCGlassEntity *,const UCGlassEntityStart *,
    const char *name,const float world[16],const UCGlassEntityProperties *);
#ifdef __cplusplus
}
#endif
#endif
