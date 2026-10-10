#ifndef UC_CHARACTER_LOOK_DRIVER_H
#define UC_CHARACTER_LOOK_DRIVER_H
#include "character_look.h"
#ifdef __cplusplus
extern "C" {
#endif
/* DDE90's actual property gate and actor194 publication. order_id is the real
 * root property default-1; holding is PLAYER's property defaultfalse. The
 * caller publishes result through inner animation virtual18(point,NULL,4).
 * No synthetic body/animation callback or target position is substituted. */
int uc_character_look_aim_point(UCCharacterLook *,UCGameRandom *,const UCCharacterLookServices *,
    int32_t self,uint32_t frame,float delta,int32_t order_id,uint8_t holding,float output[3]);
#ifdef __cplusplus
}
#endif
#endif
