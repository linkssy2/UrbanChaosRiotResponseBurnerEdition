#ifndef UC_NATIVE_PLAYER_SHIELD_H
#define UC_NATIVE_PLAYER_SHIELD_H
#include "player_equipment_transition.h"
#ifdef __cplusplus
extern "C" {
#endif
/* D0C30's normal hold-mode1 path. Same inventory/control/action fields as the
 * equipment owner. Nonlowered, available current item0/1/2/17; requested and
 * previous may additionally be11 (not read by this input prefix). No pending
 * reload, utility CA9B0 or scripted permission branch. Resolved held input is
 * channel8!=0; the PC adapter owns its button binding. No private return item.
 * Required completing sound and switch update retain all borrowed lifetimes. */
typedef struct UCPlayerShieldServices {
    void (*sound)(void *,uint8_t activate);
    void (*transition)(void *,uint32_t state,uint32_t stage);
    void (*update_switch)(void *);
} UCPlayerShieldServices;
UCMainplayerStatus uc_player_shield_input(UCPlayerEquipmentTransition *,uint8_t held,
    const UCPlayerShieldServices *,void *,uint8_t *requested);
/* C8760 -> C3170, stable actual actor transform at both virtual10 reads.
 * Finite nondegenerate forward/right bases, finite incoming direction and
 * impact amount. Returns eligibility only; does not apply damage/effects. */
UCMainplayerStatus uc_player_shield_blocks(int32_t item,const UCMainplayerEquipControl *,float impact,
    const float body[16],const float incoming[3],uint8_t *blocked);
#ifdef __cplusplus
}
#endif
#endif
