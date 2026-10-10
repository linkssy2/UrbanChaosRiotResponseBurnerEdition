#ifndef UC_NATIVE_CHARACTER_FOLLOW_H
#define UC_NATIVE_CHARACTER_FOLLOW_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCCharacterFollow {
 float *world;const float *actor_world;float last_target[3],stop_range;
 int32_t target,next;uint8_t moving,run,side_step;
} UCCharacterFollow;
typedef struct UCCharacterFollowServices {
 void *owner;
 const float *(*position)(void*,int32_t);
 void (*transition)(void*,int32_t);
 void (*retarget)(void*,const float[3]);
 int32_t (*status)(void*);
 void (*waypoint)(void*,float[3]);
 uint8_t (*near_goal)(void*);
 void (*stop_aim)(void*);
 void (*move)(void*,const float[3],int32_t mode,uint8_t near_goal);
 void (*stop)(void*);
 void (*aim)(void*,const float[3]);
 uint8_t (*property)(void*,const char*,uint8_t fallback);
 void (*side_step)(void*); //actual3BC80 helper, including callback ownership
 int32_t (*current_target)(void*);
 void (*idle_look)(void*); //actualDDE90 / actor194 look-point owner
} UCCharacterFollowServices;
/*194AC0 admitted authored1072 branch: no stop-flag handle, duck-if-shot,
 * duck-if-target, move-back or154 callback. Factory MUST reject these options.
 * Every callback completes its original effect or throws at the PC boundary.
 * No arbitrary arrival/timer: only source range/missing-target calls325A0.
 * Source32680 world-copy runs only on the continuing path. Scheduling and
 * entry194900 (real condition, target resolve, navigator reset, helper7.5)
 * belong to the concrete behavior owner.0 completed,1 invalid before effects. */
int uc_character_follow_update(UCCharacterFollow*,const UCCharacterFollowServices*);
/*193810: dynamic FollowPlayerBehaviour created by169C10 with Wait at start0.
 * The authored wait-at-start option is a separate admission domain. */
typedef struct UCCharacterPlayerFollow {
 float last_target[3],idle_time,delay;
 uint8_t side_step,check_side_step,moving,forced,turned;
} UCCharacterPlayerFollow;
typedef struct UCCharacterPlayerFollowServices {
 void *owner;
 double(*delta)(void*);
 const float*(*player)(void*); /*4A840/4ACD0/1F8FC0; null means no player*/
 const float*(*actor)(void*);
 void(*side_step_tick)(void*); /*3B050, before player-entity null check*/
 void(*retarget)(void*,const float[3]);
 int32_t(*status)(void*);
 void(*waypoint)(void*,float[3]);
 uint8_t(*near_goal)(void*);
 void(*stop_aim)(void*);
 void(*move)(void*,const float[3],int32_t,uint8_t);
 void(*stop)(void*);
 void(*look)(void*,const float[3],uint8_t force);
 uint8_t(*property)(void*,const char*,uint8_t fallback);
 void(*side_step_run)(void*,uint8_t forced); /*3B880/3BC80*/
 uint8_t(*body_aiming)(void*); /*bodyA4 thenC4->8*/
 int32_t(*target)(void*);
 void(*idle_look)(void*);
 const float*(*target_head)(void*);
 int32_t(*random_pair)(void*); /*2D37B0(0,2)*/
 int32_t(*target_kind)(void*); /*34270*/
 uint8_t(*player_shield)(void*);
 int32_t(*character_type)(void*);
 uint8_t(*chance)(void*,float);
 void(*emote)(void*,int32_t,int32_t);
 void(*base)(void*);
} UCCharacterPlayerFollowServices;
int uc_character_player_follow_update(UCCharacterPlayerFollow*,const UCCharacterPlayerFollowServices*);
#ifdef __cplusplus
}
#endif
#endif
