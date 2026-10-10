#ifndef UC_BRUTE_COMBAT_H
#define UC_BRUTE_COMBAT_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/*1D1240/16C0/1860/1DA0/1B40 states3/4/5/6/7. Fields retain source
 * offsets; task slots own actual callbacks, not elapsed-time substitutes. */
typedef struct UCBruteCombat {
 uint8_t entered,retreat,in_cone,can_throw,can_charge; /*1E0,124,1A1,1F8,218*/
 uint32_t actor_flags; /*198*/
 int32_t target,attack_count,throws_left,route; /*1D8,20C,1F4,11C*/
 float position[3],saved_position[3],aim[3]; /*actor40,128,17C*/
 float distance,facing_dot,shield_time,target_y; /*160,170,1B4,148*/
 float elapsed,since_attack,since_action,attack_cooldown,throw_cooldown,taunt_delay; /*1E4,1E8,110,210,214,114*/
 uintptr_t tasks[4]; /*1FC,200,204,208*/
} UCBruteCombat;
typedef struct UCBruteCombatServices {
 void *owner;
 void (*transition)(void*,int32_t);
 void (*reset_navigation)(void*);
 uint8_t (*attention)(void*);
 void (*clear_attention)(void*);
 void (*look)(void*,const float[3]);
 void (*stop_aim)(void*);
 void (*stop_look)(void*);
 void (*aim)(void*,const float[3]); /*inner18, callback0, mode0*/
 void (*pressure)(void*,float); /*34140*/
 float (*delta)(void*);
 void (*task_release)(void*,uintptr_t);
 int32_t (*task_status)(void*,uintptr_t);
 uintptr_t (*task_create)(void*,uintptr_t*); /*36280 /2A9150*/
 void (*melee_reset)(void*); /*2A220*/
 double (*melee_facing)(void*); /*2A2E0*/
 double (*melee_distance)(void*); /*2A2C0*/
 void (*melee_prepare)(void*,int32_t); /*2A200 target,1,0*/
 void (*melee_action)(void*,uint32_t,uintptr_t); /*bodyAC+8*/
 void (*melee_move)(void*,uint32_t); /*bodyAC+C*/
 void (*sound)(void*,const char*); /*216BD0 tag,0*/
 void (*dialogue)(void*,uint32_t,uint32_t); /*DED50*/
 int32_t (*random_int)(void*,int32_t,int32_t);
 uint8_t (*random_bool)(void*);
 double (*random_float)(void*,float,float);
 double (*difficulty_delay)(void*,float,float,float); /*169C0*/
 uint8_t (*target_is_player)(void*); /*123650 target tracker*/
 void (*throw_clip)(void*,uintptr_t); /*inner2C, saved21C, callback,0,0*/
 void (*action)(void*,const char*,uintptr_t); /*inner30,name,callback,0,0*/
} UCBruteCombatServices;
uint8_t uc_brute_throw_predicate(const UCBruteCombat*,const UCBruteCombatServices*); /*1D17D0*/
uint8_t uc_brute_charge_predicate(const UCBruteCombat*); /*1D1D20*/
int uc_brute_combat_step(UCBruteCombat*,const UCBruteCombatServices*,uint32_t stage);
#ifdef __cplusplus
}
#endif
#endif
