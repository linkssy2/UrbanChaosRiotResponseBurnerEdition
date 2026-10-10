#ifndef UC_COP_ATTACK_H
#define UC_COP_ATTACK_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCCopAttack {
 float *world;const float *actor_world;
 int32_t state,cover;
 float crouch_timer,unseen,no_target;
 uint8_t crouching;
} UCCopAttack;
typedef struct UCCopAttackServices {
 void *owner;double delta;
 uint8_t (*stance_busy)(void*);
 int32_t (*target)(void*);
 const float *(*player_position)(void*);
 int32_t (*dialogue_mode)(void*); //34270 actual named-tag/event traversal
 void (*dialogue)(void*,int32_t type,int32_t priority);
 void (*search_cover)(void*); //1C0CB0, writes same state/cover through owner
 int32_t (*nav_status)(void*);
 void (*waypoint)(void*,float[3]);
 uint8_t (*near_goal)(void*);
 void (*stop_aim)(void*);
 void (*move)(void*,const float[3],int32_t mode,uint8_t near_goal);
 void (*crouch)(void*); //outerBC virtual4(0,0,0), not stand-in transform
 void (*stop)(void*); //inner90(0)
 float (*random)(void*,float minimum,float maximum);
 uint8_t (*gun_target)(void*,int32_t); //GunController virtual24(target,0,0)
 uint8_t (*stand_if_crouching)(void*);
 void (*clear_target)(void*); //32DE0
 int32_t (*stance_mode)(void*);
 void (*idle_look)(void*);
 void (*conditions)(void*); //39190 after32680 copy
} UCCopAttackServices;
/* Whole1C0F10. Expensive(bool) is consumed only by shared32680, which copies
 * actor world; every policy branch and condition runs on cheap frames too.
 * Required services complete actual effects or fail at the C++ boundary.
 * Constructor/entry/exit and the retained1C0CB0 job belong to concrete owner. */
int uc_cop_attack_update(UCCopAttack*,const UCCopAttackServices*);
#ifdef __cplusplus
}
#endif
#endif
