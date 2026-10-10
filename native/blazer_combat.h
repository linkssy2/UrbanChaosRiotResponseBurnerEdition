#ifndef UC_BLAZER_COMBAT_H
#define UC_BLAZER_COMBAT_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* State4 (1D5730), with actual 1D4F20/1D4B30 decisions. Only entered,
 * threw, aim_task and cooldown are written by this policy. Task tokens retain
 * the actual callback owner; zero means no callback, not completed work. */
typedef struct UCBlazerCombat {
 uint8_t entered,threw,dont_move,defend,defend_near,visible,in_cone,facing;
 uint8_t position_ready,position_changed,melee_enabled;
 int32_t target;
 float aim[3],distance,facing_dot,near_time,target_y,actor_y,shield_time,cooldown;
 uintptr_t aim_task,wait_task,side_task;
} UCBlazerCombat;
typedef struct UCBlazerCombatServices {
 void *owner;
 void (*stop)(void*); /* inner+90(0) */
 void (*stop_aim)(void*); /* inner+1C */
 void (*reset_navigation)(void*); /* 1E830 */
 void (*task_release)(void*,uintptr_t);
 int32_t (*task_status)(void*,uintptr_t); /* virtual+4: 0 pending */
 uintptr_t (*aim_task_create)(void*,uintptr_t*); /* 36280: replaces slot, returns callback */
 void (*transition)(void*,int32_t); /* 1D3CA0 */
 uint8_t (*attention)(void*); /* DEF80 */
 void (*look)(void*,const float[3]); /* inner+20 */
 void (*cancel_position_search)(void*,uint8_t); /* search+14 */
 uint8_t (*throw_active)(void*); /* 2D7A0 */
 uint8_t (*throw_ready)(void*); /* 2DAB0 */
 double (*throw_elapsed)(void*); /* 2D830 */
 void (*throw_reset)(void*); /* 2E410 */
 void (*throw_target)(void*,int32_t,float,float,uint8_t); /* 2D7B0 */
 void (*publish_shield)(void*,uint8_t); /* 341A0 */
 void (*aim)(void*,const float[3],uintptr_t,uint32_t); /* inner+18 */
 uint8_t (*throw_predicate)(void*); /* 1D4120 */
 uint8_t (*throw_request)(void*,uint8_t close,uint8_t alternate); /* 1D4310 */
 float (*reset_cooldown)(void*); /* whole 1D39F0 retained difficulty/RNG owner */
} UCBlazerCombatServices;
uint8_t uc_blazer_combat_sidestep(const UCBlazerCombat*,const UCBlazerCombatServices*);
uint8_t uc_blazer_combat_melee(const UCBlazerCombat*,const UCBlazerCombatServices*);
int uc_blazer_combat_step(UCBlazerCombat*,const UCBlazerCombatServices*);
#ifdef __cplusplus
}
#endif
#endif
