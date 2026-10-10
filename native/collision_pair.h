#ifndef UC_NATIVE_COLLISION_PAIR_H
#define UC_NATIVE_COLLISION_PAIR_H
#include "collision_asset.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCCollisionPairProperty { uint32_t key; uint64_t value; } UCCollisionPairProperty;
typedef struct UCCollisionPairAgent { const void *agent,*other_collidable; } UCCollisionPairAgent;
typedef struct UCCollisionPairBody {
    const void *identity,*rigid_owner;
    uint32_t collidable_type,impulse_disabled,shape_present,shape_type,shape_user_data,filter_info;
    const UCCollisionPairProperty *properties;
    uint32_t property_count;
    const UCCollisionPairAgent *agents;
    uint32_t agent_count;
} UCCollisionPairBody;
typedef struct UCCollisionPair { const UCCollisionPairBody *first,*second; } UCCollisionPair;
typedef enum UCCollisionPairFilter { UC_COLLISION_PAIR_GAME_FILTER=1 } UCCollisionPairFilter;
/* Caller must establish that the effective installed pair interface is the
 * game's 29E930/2F0E90 selection (2A8B60), NOT the ray filter2A8860. The enum
 * records that explicit ownership contract; it cannot inspect a live world.
 * Metadata, original-order property/agent arrays and identities are borrowed,
 * immutable and alive through each call. Only class1 nonnull rigid owners are
 * admitted. Property values use their LOW32, first matching key; presence is
 * distinct from zero. At most4096 properties/16383 agent entries per body.
 * The property21-vs4 script lookup and excessive property4 loop are unsupported.
 * No allocation, callbacks, world mutation, mutable getter or alias equivalence.
 * Failed calls preserve outputs; output storage must be disjoint from inputs. */
UCCollisionStatus uc_collision_pair_filter(UCCollisionPairFilter,
    const UCCollisionPairBody *,const UCCollisionPairBody *,int *accepted);
/* Original329660: scan the smaller count, first body on equal counts, in order.
 * OK with NULL means no matching agent (or an explicit matching NULL record).
 * Does not remove an agent or assume an empty list from a missing owner. */
UCCollisionStatus uc_collision_pair_agent(const UCCollisionPairBody *,
    const UCCollisionPairBody *,const void **agent);
/* Pure admission for bounded no-contact-effect kind1/1 lifecycle. Addition
 * requires actual filter rejection. Removal does NOT filter: it performs the
 * ordered lookup and rejects a nonnull matching agent requiring2FE190. Accepted
 * additions needing2FDA50/2FE0C0 are unsupported, even if another dispatch table
 * might suppress creation. Processes pairs in supplied source SAP order. OK
 * qualifies these pair effects only, not body/world registration or listeners.
 * No general callback is silently treated as successful. Count at most16383. */
UCCollisionStatus uc_collision_pair_admit(UCCollisionPairFilter,
    const UCCollisionPair *,uint32_t count,int removing);
#ifdef __cplusplus
}
#endif
#endif
