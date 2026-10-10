#ifndef UC_NATIVE_WEAPON_STATE_H
#define UC_NATIVE_WEAPON_STATE_H
#include "mainplayer_state.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCWeaponDefinition {
    float interval, decay, recoil_limit;
} UCWeaponDefinition;
typedef struct UCWeaponState {
    uint32_t shot_count;
    float decay, deadline, recoil[2], spring[3], recoil_amount, last_shot, heat;
    uint8_t firing, alternate, pending_reload;
} UCWeaponState;
typedef struct UCWeaponCadenceResult { uint8_t ready, reset_fire_input; } UCWeaponCadenceResult;
/* Fixed original definitions for item2/17 only. Outputs remain unchanged on
 * failure. All pointers are disjoint, initialized caller-owned storage. */
UCMainplayerStatus uc_weapon_definition(uint32_t item, UCWeaponDefinition *out);
/* Original431C0 projection: initializes the modeled fields but leaves shot_count
 * untouched, exactly as the constructor does. Inventory grants and item binding
 * are separate. Use reset_shots at the original43310 action boundary. */
UCMainplayerStatus uc_weapon_initialize(UCWeaponState *);
UCMainplayerStatus uc_weapon_reset_shots(UCWeaponState *);
UCMainplayerStatus uc_weapon_fire_mode(uint32_t item, uint8_t alternate, uint32_t *out);
UCMainplayerStatus uc_weapon_can_fire(uint32_t item, uint32_t clip, uint8_t *out);
UCMainplayerStatus uc_weapon_ready(const UCWeaponState *, float clock, uint8_t *out);
/* Every admitted call follows439D0, including calls returning ready=0.
 * reset_fire_input represents2084E0(11)->210840, which precedes the scalar
 * mutation. The caller must honor that effect; this is not a pure readiness
 * query. clock/delta are original manager time and1F9010, not invented clocks. */
UCMainplayerStatus uc_weapon_cadence(uint32_t item, uint32_t reserve,
    float clock, float delta, UCWeaponState *, UCWeaponCadenceResult *out);
/* Uses resolved44160/44210 values (uc_mainplayer_item_spec). The original
 * commit always transfers its amount and clears pending; it does NOT test a
 * trigger or pending flag. The owning caller performs that gate first. */
UCMainplayerStatus uc_weapon_reload_amount(uint32_t item,
    const UCMainplayerItemSpec *, uint32_t clip, uint32_t reserve, uint32_t *out);
UCMainplayerStatus uc_weapon_reload_commit(uint32_t item,
    const UCMainplayerItemSpec *, uint32_t *clip, uint32_t *reserve, UCWeaponState *);
/* Full43CE0 scalar transaction for the initialized2/17 definitions, whose
 * recoil_limit is zero: the random44290 branch is never entered. Mutable
 * definition overrides are unsupported. Projectile, audio and world effects
 * remain caller-owned. Empty clip returns
 * fired=0 with state/clip unchanged. No cadence/deadline gate is added here.
 * Finite RN arithmetic, nonnegative clock/delta/clip/reserve and ordinary
 * finite state only; reject nonfinite results transactionally. No physical-x87
 * claim. RN is round-to-nearest with DAZ/FTZ disabled. */
UCMainplayerStatus uc_weapon_shot(uint32_t item, float clock, float delta,
    uint32_t *clip, UCWeaponState *, uint8_t *fired);
#ifdef __cplusplus
}
#endif
#endif
