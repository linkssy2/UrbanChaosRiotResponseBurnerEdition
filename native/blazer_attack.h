#ifndef UC_BLAZER_ATTACK_H
#define UC_BLAZER_ATTACK_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Actual1D4730 state1 with source772's no-defend/no-route properties. The
 * target owner remains real: acquisition switches state instead of suppressing
 * enemies to keep this admitted branch running. */
typedef struct UCBlazerAttackIdle { uint8_t entered,route;int32_t route_entity; } UCBlazerAttackIdle;
typedef struct UCBlazerAttackIdleServices {
 void *owner;
 int32_t (*target)(void*);
 void (*transition)(void*,int32_t);
 void (*reset_navigation)(void*);
 void (*stop)(void*);
 int32_t (*nav_status)(void*);
 void (*waypoint)(void*,float[3]);
 uint8_t (*near_goal)(void*);
 void (*move)(void*,const float[3],int32_t,uint8_t);
 void (*stop_aim)(void*);
} UCBlazerAttackIdleServices;
int uc_blazer_attack_idle(UCBlazerAttackIdle*,const UCBlazerAttackIdleServices*);
/*1D48D0. Completion0 remains pending; absent/finished/cancelled progresses.*/
int uc_blazer_attack_new_target(UCBlazerAttackIdle*,uint8_t*first,int completion,const UCBlazerAttackIdleServices*);
typedef struct UCBlazerAttackMove {
 uint8_t entered,defend,dont_move,in_cone,position_ready,position_changed;
 int32_t target;float distance,elapsed,position[3],target_position[3],defend_position[3];
} UCBlazerAttackMove;
typedef struct UCBlazerAttackMoveServices {
 void *owner;
 int32_t (*player)(void*); /*338B0(0)*/
 void (*dialogue)(void*,int32_t,int32_t); /*DED50*/
 void (*paired_dialogue)(void*,int32_t,int32_t,float,int32_t); /*DEDD0*/
 void (*retarget)(void*,const float[3]); /*21D20*/
 int32_t (*run)(void*); /*33020(0,1,0,1,0)*/
 void (*transition)(void*,int32_t);
} UCBlazerAttackMoveServices;
int uc_blazer_attack_move(UCBlazerAttackMove*,const UCBlazerAttackMoveServices*);
typedef struct UCBlazerTargetCue {float remaining;int32_t target;} UCBlazerTargetCue;
typedef struct UCBlazerTargetCueServices {
 void *owner;uint8_t (*weapon_ready)(void*);uint8_t (*valid)(void*,int32_t);int32_t (*player)(void*);
 uint8_t (*say)(void*,int32_t,int32_t);uint8_t (*pair)(void*,int32_t,int32_t,float,int32_t);
 double (*health)(void*);double (*maximum)(void*);double (*random)(void*,float,float);
} UCBlazerTargetCueServices;
int uc_blazer_target_cue(UCBlazerTargetCue*,const UCBlazerTargetCueServices*,uint8_t shield);
typedef struct UCBlazerEvade {
 uint8_t entered,near_line;int32_t target;float position[3],direction[3],aim[3];uintptr_t task;
} UCBlazerEvade;
typedef struct UCBlazerEvadeServices {
 void *owner;uint8_t (*random_side)(void*);void (*right)(void*,float[3]);
 int (*clear)(void*,const float[3]);uint8_t (*inside_defend)(void*,const float[3]);
 int32_t (*player)(void*);void (*pair)(void*,int32_t,int32_t,float,int32_t);
 uintptr_t (*task_create)(void*,uintptr_t*);
 void (*approach)(void*,const float[3],const float[3],uintptr_t,float);
 void (*aim)(void*,const float[3]);uint8_t (*attention)(void*);void (*look)(void*,const float[3]);
 int (*task_status)(void*,uintptr_t);void (*transition)(void*,int32_t);
} UCBlazerEvadeServices;
/* Whole1D4BA0: actual right-vector clearance, callback-owned sidestep. */
int uc_blazer_evade(UCBlazerEvade*,const UCBlazerEvadeServices*);
typedef struct UCBlazerDuck {
 uint8_t entered,thrown,near_line;int32_t target;float delay,quiet;
 uintptr_t throw_task,duck_task,stop_task;
} UCBlazerDuck;
typedef struct UCBlazerDuckServices {
 void *owner;double delta;
 int (*status)(void*,uintptr_t);void (*release)(void*,uintptr_t);
 uintptr_t (*create)(void*,unsigned slot,uintptr_t*);
 void (*duck)(void*,uintptr_t);void (*stop)(void*,uintptr_t);
 int32_t (*player)(void*);void (*pair)(void*,int32_t,int32_t,float,int32_t);
 void (*health_cue)(void*);uint32_t (*bullet_by)(void*);
 double (*difficulty)(void*,float,float);void (*transition)(void*,int32_t);
} UCBlazerDuckServices;
int uc_blazer_duck(UCBlazerDuck*,const UCBlazerDuckServices*);
#ifdef __cplusplus
}
#endif
#endif
