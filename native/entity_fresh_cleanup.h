#ifndef UC_ENTITY_FRESH_CLEANUP_H
#define UC_ENTITY_FRESH_CLEANUP_H
#include "entity_fresh.h"
#include "entity_retirement.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Bounded actual fresh LightFlash base: model74/helper78/audioC0/handleBC/
 * scriptC8/listeners are NULL, CC/CD/CE zero, source flag1000 clear, and aux84
 * is the real live empty owner from entity_fresh. The fields remain borrowed
 * from that SAME entity. No model/audio/script/listener success callbacks.
 * Exclusive access; descriptors/output are disjoint from mutable owned fields.
 * Auxiliary allocator release callbacks also preserve this fresh profile and
 * the entity lifetime until the explicit final outer release.
 * Return0 completed,1 invalid before effects,2 committed service failure.
 * Failure2 is not retryable; effects/lifetimes are not rolled back. */

/*120C60→218C40→218680: completing typed continuation, then actual aux clear.
 * advance may call deactivate below (source removal inside coroutine), but
 * must return with this owner/aux alive and the admitted null profile intact.
 * It must not recursively update/destroy. This is not generic1FB790 guest stack
 * scheduling: concrete uc_light_flash_step and its shared delta remain caller
 * work. Even its final removal/yield is followed by this auxiliary clear. */
int uc_entity_fresh_update(const UCEntityFreshView *,void (*advance)(void *),void *);

/*218C30→218400, actual LightFlash4700C8+7C=46980. If word90 bit0 is clear,
 * add source positive-zero XYZ to the current translation and publish W=1.
 * This is not an omitted no-op: -0 becomes+0 under admitted RN/normal denormals.
 * Finite XYZ required only for that branch. Other matrix elements are untouched.
 * Callback-visible floating exception flags are outside the admitted contract. */
int uc_entity_fresh_late(const UCEntityFreshView *);

/*218B30→216F80→2183C0→20A620: audioNULL publication, aux clear, flags8100,
 * actual207FF0 retirement, then flags80000. Never free the entity here.
 * Retirement's completing record callbacks must preserve this fresh profile
 * and all captured owners. queued retains retirement's truthful stale/full
 * outcomes; queued0 is not successful deferred destruction. Output only on0. */
int uc_entity_fresh_deactivate(const UCEntityFreshView *,const UCEntityRetirement *,uint8_t *queued);

/*176150/176170→218CE0→218180→20A480. Invoke the concrete typed-continuation
 * lifetime finish FIRST, then source handleBC=0's actual22A5B0 no-effect branch,
 * auxiliary clear/destroy, and (deleting_flags&1) final allocation release.
 * finish must complete, retain entity/aux and the null profile, and may release
 * its distinct coroutine holder. It is an explicit native typed replacement
 * boundary, not reconstructed generic1FBA40 stack allocator semantics.
 * The effect's point light must ALREADY be released and its final update/yield
 * completed; this generic destructor does not release it. The world or actual
 * pending drain owns this frontier. release may immediately poison/free the
 * identity, view and entity; no subsequent reads occur. With lowbit0 release
 * may be NULL; outer allocation remains caller-owned, aux84 slot is dangling
 * exactly as source and cannot be used to retry destruction. */
int uc_entity_fresh_destroy(const UCEntityFreshView *,void (*finish)(void *),
    void (*release)(void *,UCEntityEventTarget *),void *,uint32_t deleting_flags);
/* Actual Bullet EB4D0 ->13BE30 ->218CE0 enters the SAME actor cleanup directly,
 * with no LightFlash embedded-coroutine finish. This entry admits the same
 * model-less/null-profile fields above; it does not fake a finish callback.
 * The live cache/world owns the destruction frontier and must already release
 * registry/list/record references. A dead bullet is still cached, not freed by
 * this API automatically. Auxiliary slot remains dangling and final outer free
 * follows its low flag bit, with no reads afterward. No entity class-word ABI
 * is represented by these typed views. */
int uc_entity_fresh_actor_destroy(const UCEntityFreshView *,
    void (*release)(void *,UCEntityEventTarget *),void *,uint32_t deleting_flags);
#ifdef __cplusplus
}
#endif
#endif
