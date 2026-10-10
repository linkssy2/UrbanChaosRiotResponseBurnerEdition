#ifndef UC_NATIVE_WEAPON_SPREAD_H
#define UC_NATIVE_WEAPON_SPREAD_H
#include "mainplayer_state.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCWeaponSpreadInput {
    uint32_t item;
    float definition_spread, accuracy;
    int32_t pellet_count, pellet_index;
    uint8_t alternate;
    int32_t random[4];
} UCWeaponSpreadInput;
/* Original43590 for item2/17, using four ordered original2D3670 results.
 * definition_spread is definition+30; accuracy is item+1C; pellet_count is
 * global4E5598. No RNG state or default definition is invented. Retains wrapped
 * 32-bit pellet arithmetic and signed division. All inputs must be finite;
 * zero/overflowing division, zero random-vector length and nonfinite outputs
 * fail without changing direction. Finite RN, gradual-underflow scope;
 * lifted x87 double/libm, not physical Xbox transcendental/status equivalence.
 * Input and direction are disjoint caller-owned storage; direction is in/out. */
UCMainplayerStatus uc_weapon_spread(const UCWeaponSpreadInput *, float direction[3]);
#ifdef __cplusplus
}
#endif
#endif
