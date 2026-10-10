#ifndef UC_NATIVE_WEAPON_BULLET_H
#define UC_NATIVE_WEAPON_BULLET_H
#include "weapon_spread.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCWeaponBulletSource {
    uint32_t item, definition_kind, session_bypass, session_class;
    int32_t pellet_count, selected_entity;
    float origin[3], target[3], selected_target[3], accuracy, accuracy_factor;
} UCWeaponBulletSource;
typedef struct UCWeaponBulletPrefix {
    uint32_t item, definition_kind, team;
    int32_t count;
    float origin[3], direction[3], accuracy_after;
} UCWeaponBulletPrefix;
typedef struct UCWeaponBulletPellet {
    uint32_t actor, spread_disabled;
    float definition_spread, weapon_accuracy, value20, value28;
    uint8_t alternate;
    int32_t random[4];
} UCWeaponBulletPellet;
/* Original60-byte EBF10 input. Names remain neutral where the consumer's
 * meaning is unresolved. CAB20 does not write reserved; EBDD0 does not read it. */
typedef struct UCWeaponBulletRequest {
    uint32_t item, definition_kind, team, actor;
    float direction[3];
    int32_t sentinel;
    float value20, scale24, value28;
    uint8_t flag2c, reserved[3];
    float origin[3];
} UCWeaponBulletRequest;
/* Ordinary actual Mainplayer (virtualC4 absent), item2/17 only. Source origin
 * is the resolved virtual10(tag0) frame translation; selected_entity==-1 uses
 * target(+460), otherwise selected_target(+454). Session/team and definition
 * fields are supplied actual owners. Returns the once-per-shot accuracy+360
 * update but does not publish it or perform the preceding feedback callback.
 * count<=0 means no per-pellet work, exactly as the original signed loop gate. */
UCMainplayerStatus uc_weapon_bullet_prefix(const UCWeaponBulletSource *, UCWeaponBulletPrefix *);
/* Call once per positive prefix count, in order, with freshly resolved inputs,
 * and submit each successful request through the real bullet owner before the
 * next. Original43590 receives TOTAL count, not the loop ordinal. No callbacks,
 * RNG generation, hit or effect success is synthesized. value20 is the actual
 * rounded44010 result; value28 is actual virtual9C(item), after44010. Inputs
 * represent a stable ordinary transaction, not callback mutation equivalence.
 * Preserves output reserved[3]; other57 bytes are published on success only.
 * Finite RN/gradual-underflow domain and disjoint caller storage are required;
 * no physical Xbox x87 status/transcendental-equivalence claim. */
UCMainplayerStatus uc_weapon_bullet_request(const UCWeaponBulletPrefix *,
    const UCWeaponBulletPellet *, UCWeaponBulletRequest *);
#ifdef __cplusplus
}
#endif
#endif
