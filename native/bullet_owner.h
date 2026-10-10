#ifndef UC_BULLET_OWNER_H
#define UC_BULLET_OWNER_H
#include "entity_registration.h"
#include "weapon_bullet.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct UCBulletOwner {
    UCEntityEventRegistry *registry;
    int32_t *cached_id; /*actual shared4FA77C generation-bearing ID */
    UCEntityCreation *creation; /*same current world creation context */
    void *context;
    /* Pure admission for every reachable factory/initial-update effect. */
    int (*preflight)(void *,const UCWeaponBulletRequest *,uint32_t owner_id);
    /* Complete1FBC50 with empty dynamic159C0(NULL,-1) definition, class"Bullet".
     * The real shared context is already parent-1, relative0/priority1, as207CD0.
     * Publish the actual returned entity+54 into
     * out, even if-1. Source factory returns a real live object, not NULL.
     * This is a required actual factory service, not implemented by this cache. */
    int (*create)(void *,int32_t *out);
    /* CompleteEBDD0 reset/prepare/first-update on the SAME resolved target.
     * Request and separate owner ID retain source order and lifetime. No false
     * success for missing physics, actor update or hit/effect services. */
    int (*initialize)(void *,UCEntityEventTarget *,const UCWeaponBulletRequest *,uint32_t);
} UCBulletOwner;

/* WholeEBF10 cached entity route, using actual1F8FC0/207D70 lookup. The factory
 * runs only when the first lookup misses; its returned ID replaces the current
 * cache, then a SECOND lookup decides whether initialization is reached.
 * Creation may change registry/cache (the return publication overwrites cache).
 * Initialization may mutate cache/registry as its real owner does; the selected
 * target remains alive throughout the synchronous callback. No reentry, no
 * descriptor replacement, immutable borrowed request and separate output.
 * The empty temporary definition's source-only class words are not represented.
 * Return0 completed,1 invalid/preflight rejection before effects,2 committed
 * service failure; do not retry2. submitted is published only on0: zero means
 * actual post-factory lookup missed, NOT a fabricated successful bullet.
 * No per-shot allocation, new ID map, pooled bullet array or standalone physics
 * is introduced. This component does not itself complete create/initialize. */
int uc_bullet_owner_submit(const UCBulletOwner *,const UCWeaponBulletRequest *,
    uint32_t owner_id,uint8_t *submitted);
#ifdef __cplusplus
}
#endif
#endif
