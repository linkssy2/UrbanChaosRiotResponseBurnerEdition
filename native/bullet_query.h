#ifndef UC_NATIVE_BULLET_QUERY_H
#define UC_NATIVE_BULLET_QUERY_H
#include "weapon_bullet.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCBulletPrepareInput {
    uint32_t owner, frame;
    uint16_t group; /* Resolved EBDD0 virtual60 result, not a guessed team. */
    uint8_t progression181;
} UCBulletPrepareInput;
typedef struct UCBulletPrepared {
    uint32_t item, definition_kind, team, actor, owner, frame;
    int32_t sentinel;
    uint32_t query_mask;
    uint16_t group;
    uint8_t flag2c;
    float position[3], previous[3], direction[3], velocity[3];
    float value20, parameters[9], impact_impulse, mass;
} UCBulletPrepared;
typedef struct UCBulletQuery {
    float start[3], end[3];
    uint32_t type, group, mask;
    float impact_impulse;
} UCBulletQuery;
/* EBDD0 payload publication after reset/group resolution, then full EB4F0
 * preparation for item2/17. This is a typed projection of the fields consumed
 * by collision/damage; it does not allocate/register an entity or run physics.
 * owner is EBF10's separate owner ID; frame is manager+0C; group must come from
 * the original owner flag08000000/group94 or manager208C10 selection. Reserved
 * packet bytes are ignored, exactly as EBDD0 does. value20 remains neutral.
 * Finite RN/gradual-underflow, nonzero direction; inputs are immutable/disjoint.
 * Invalid input/result leaves output unchanged. Lifted double-backed x87 scope. */
UCMainplayerStatus uc_bullet_prepare(const UCWeaponBulletRequest *,
    const UCBulletPrepareInput *, UCBulletPrepared *);
/* Actual3F775..3F7F6 query arguments, reached only after entityflags&0800.
 * current_position/current_velocity MUST be actual current physics state.
 * Prepared velocity is before original21D9C0; this API does not invent the
 * intervening physics update. The caller must submit this to the real214230
 * collision owner and retain its returned result. No hit/damage is synthesized.
 * impact_impulse is forwarded only to post-hit body processing; it does not
 * enlarge the world ray into a sphere/capsule query.
 * This leaf performs no query and does not advance history or effect state. */
UCMainplayerStatus uc_bullet_query_prepare(const UCBulletPrepared *,
    const float current_position[3], const float current_velocity[3],
    UCBulletQuery *);
#ifdef __cplusplus
}
#endif
#endif
