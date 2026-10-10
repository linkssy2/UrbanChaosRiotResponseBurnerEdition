#ifndef UC_WEAPON_BULLET_EMIT_H
#define UC_WEAPON_BULLET_EMIT_H
#include "weapon_bullet.h"
#include "weapon_state.h"
#include "game_random.h"
#include "shot_lightflash.h"
#ifdef __cplusplus
extern "C" {
#endif
/* All fields borrow the existing player/equipment/session authorities. The
 * fixed argument item selects the same equipped object throughout this call. */
typedef struct UCWeaponBulletEmitView {
    uint32_t *entity_id, *spread_disabled;
    float *position, *accuracy, *target, *selected_target;
    int32_t *selected_entity, *pellet_count;
    uint32_t *session_bypass, *session_class;
    const uint8_t *upgrades; /* Actual progression+50, 48 bytes. */
    const uintptr_t *network_owner; /* Actual2918A0 singleton; must be NULL. */
    UCWeaponState *weapon;
} UCWeaponBulletEmitView;
typedef struct UCWeaponBulletEmitServices {
    void *context;
    /* Return1 for pure admission of all subsequent effects and their storage. */
    int (*preflight)(void *,uint32_t,const UCWeaponBulletEmitView *);
    /* Effects return0 only on completion. Actual68A30 spatial effect, before
     * cached count/team/aim publication. */
    int (*feedback)(void *,const float position[3],float radius,float amplitude,
                    float duration,uint32_t kind);
    int (*tag0_origin)(void *,float origin[3]); /* Actual virtual10 tag0. */
    /* Complete actual EBF10, including its cache/factory/initial bullet update.
     * A real post-factory lookup miss is a completed source operation. */
    int (*bullet)(void *,const UCWeaponBulletRequest *,uint32_t owner_id);
    UCShotLightFlashServices flash;
} UCWeaponBulletEmitServices;
typedef enum UCWeaponBulletEmitStep {
    UC_BULLET_EMIT_NONE, UC_BULLET_EMIT_FEEDBACK, UC_BULLET_EMIT_TAG,
    UC_BULLET_EMIT_ACCURACY, UC_BULLET_EMIT_SPREAD, UC_BULLET_EMIT_BULLET,
    UC_BULLET_EMIT_FLASH
} UCWeaponBulletEmitStep;
typedef struct UCWeaponBulletEmitResult {
    uint32_t bullets;
    UCWeaponBulletEmitStep completed,failed;
} UCWeaponBulletEmitResult;
/* Whole ordinary CAB20 control sequence for original initialized item2/17,
 * C4 absent and network singleton NULL. Definitions are immutable original
 * 3A35A0 values, primary or secondary mode; no replacement definition or retuning.
 * Live scalar fields may change in completing callbacks, as may the SAME game
 * random stream. Views/services/storage and FP environment remain valid and
 * unchanged; callbacks must keep this admitted domain. No reentry/aliasing.
 * Initial count may be <=0 (still accuracy and LightFlash); positive count and
 * subsequently read spread denominator are bounded to16384. Source count is
 * cached once, denominator is read again per pellet and must be positive when
 * shotgun spread is enabled. No heap allocation or persistent scratch.
 * RN, DAZ/FTZ off, finite bounded inputs. Callback-visible FP exception status
 * is excluded. Reserved request bytes are unspecified (original unwritten).
 * Return0 completed,1 rejected before effects (result untouched),2 committed
 * failure (result identifies frontier). Never retry2 as a fresh shot. Required
 * services are not simulated by this owner; no world/hit/audio completion claim.
 */
int uc_weapon_bullet_emit(uint32_t,const UCWeaponBulletEmitView *,UCGameRandom *,
    const UCWeaponBulletEmitServices *,UCWeaponBulletEmitResult *);
#ifdef __cplusplus
}
#endif
#endif
