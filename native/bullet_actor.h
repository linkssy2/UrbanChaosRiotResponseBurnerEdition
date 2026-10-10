#ifndef UC_BULLET_ACTOR_H
#define UC_BULLET_ACTOR_H
#include "bullet_scenery.h"
#include "character_damage.h"
#include "fire_damage_sphere.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCBulletActorServices {
    void *owner;
    int (*boolean)(void *,const char *,uint8_t *); /* actual target20A690 */
    int (*damage)(void *,float,const UCPlayerFireDamage *); /* target+6C E0470 */
    int (*react)(void *,const UCCharacterBullet *,const UCCharacterBulletImpact *); /* target+78 DFC60 */
} UCBulletActorServices;
/*3F803..3F83F: skip the complete contact before impact, while preserving the
 * segment/history suffix. target_id is the live actor ID, never property45.*/
int uc_bullet_actor_ignored(const UCBulletState *,int32_t target_id,int32_t local_player_id);
/*3F981..3FFCA, offline Character recipients whose actual shield-block property
 * is false and whose source parent has no network component. Required callbacks
 * complete real services. Damage callback owns Character+13C (E0470 ordinary
 * bullet), not actor A4 or health. React owns DFC60/body-part/health policy.
 * Mutations by callbacks are observed at the original read frontiers. Owners
 * and views must remain live/disjoint. Finite RN domain; nonzero velocity when
 * damage is reached; kind8/9 melee and shields explicitly reject.0 complete,
 *1 invalid before effects,2 service failure after reads/committed effects.*/
int uc_bullet_actor_contact(const UCEntityFreshView *bullet,UCBulletState *,
    const UCEntityFreshView *target,const UCBulletImpact *,const UCBulletActorServices *);
#ifdef __cplusplus
}
#endif
#endif
