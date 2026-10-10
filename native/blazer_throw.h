#ifndef UC_BLAZER_THROW_H
#define UC_BLAZER_THROW_H
#include <stdint.h>
#include "game_difficulty.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCBlazerThrow {
 int32_t target;uint32_t actor_flags;
 float position[3],target_position[3],last_clear[3],distance,facing_dot,shield_time,cooldown;
 uint8_t near_line,throw_low,melee_enabled;
 uintptr_t aim_task,wait_task,side_task;
} UCBlazerThrow;
typedef struct UCBlazerThrowServices {
 void *owner;
 uint8_t (*active)(void*); /* 2D7A0 */
 uint8_t (*ready)(void*); /* 2DAB0 */
 void (*stop)(void*); /* inner+90(0) */
 int32_t (*task_status)(void*,uintptr_t);
 uintptr_t (*task_create)(void*,uintptr_t*); /* 36280/inline equivalent */
 void (*play)(void*,const char*,uintptr_t,uint8_t overlay); /* inner+30/+40, trailing0,0 */
 void (*prime)(void*); /* 2D790 */
 int32_t (*target)(void*); /* 33B40 */
 void (*set_target)(void*,int32_t,float,float,uint8_t); /* 2D7B0 */
 uint8_t (*can_throw)(void*,int32_t,float); /* 2D8A0 */
 void (*throw_target)(void*,int32_t,float,uint32_t); /* 2D730 */
 void (*throw_point)(void*,const float[3],float,uint32_t); /* 2D750 */
 void (*sound)(void*,const char*); /* 216BD0 actorD0,event,0 */
 void (*speech)(void*,int32_t,int32_t); /* DED50 */
 void (*speech_pair)(void*,int32_t,int32_t,float,int32_t); /* DEDD0 */
 int32_t (*primary_target)(void*); /* 338B0(0), used by121080 */
 double (*random)(void*,float,float); /* shared game2D3780 */
 double (*difficulty)(void*,float,float); /* 168B0, actual throw channel */
 double (*difficulty3)(void*,float,float,float); /* 169C0 */
} UCBlazerThrowServices;
/* Return0/1 is the original decision; -1 reports a missing owner callback. */
int uc_blazer_throw_prepare(UCBlazerThrow*,const UCBlazerThrowServices*);
int uc_blazer_throw_request(UCBlazerThrow*,const UCBlazerThrowServices*,uint8_t close,uint8_t alternate);
int uc_blazer_throw_cooldown(UCBlazerThrow*,const UCBlazerThrowServices*);
/* 16840/169C0. Channel is profile+14C, initialized to.5 by166C0 and
 * maintained independently of adaptive+28. Do not substitute167C0 alone. */
int uc_blazer_throw_difficulty(const UCGameDifficulty*,float channel,double*);
int uc_blazer_throw_curve(const UCGameDifficulty*,float channel,float low,float middle,float high,double*);
/*168D0 is shared by Brute and Blazer; channel is supplied by the caller.*/
int uc_blazer_throw_range(const UCGameDifficulty*,float channel,float low,float high,float lower,float upper,double*);
#ifdef __cplusplus
}
#endif
#endif
