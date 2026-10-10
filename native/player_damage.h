#ifndef UC_PLAYER_DAMAGE_H
#define UC_PLAYER_DAMAGE_H
#include "fire_damage_sphere.h"
#include "game_difficulty.h"
#include "explosion_sequence.h"
#include "mainplayer_state.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCPlayerDamageState {
 float fire_timer,hit_elapsed,raw_damage,debug_timer;uint8_t last_present;
 UCPlayerFireDamage last;
} UCPlayerDamageState;
typedef struct UCPlayerDamageInput {
 float *health,*maximum;const float *frame;
 UCMainplayerEquipControl shield;int32_t action,item;float shield_impact,scale;
 uint32_t world_flags,selected_level74;uint8_t disabled,locked;
} UCPlayerDamageInput;
typedef struct UCPlayerDamageServices {
 void*owner;const UCGameDifficulty*difficulty;
 int(*multiplayer)(void*);
 int(*sound)(void*,unsigned channel); /*216BD0 GetShot,0 actor2D8,1 actor2D4. */
 int(*pain)(void*); /* actual actor1C8 69150 */
 int(*feedback)(void*,float amplitude,float duration,uint32_t kind);
 int(*enemy_type)(void*,int32_t owner,int32_t*out); /*1F7AF0 default-1 */
 int(*adaptive_hit)(void*,int32_t); /*BC3E0, only nonmissing enemy type */
 double(*delta)(void*);
} UCPlayerDamageServices;
typedef struct UCPlayerImpactState {
 float intensity_x,intensity_y,yaw,pitch; /*64C,650,658,65C*/
} UCPlayerImpactState;
typedef struct UCPlayerImpactServices {
 void *owner;const float *position;uint8_t has_point_push;
 double(*random)(void*,float,float); /*shared2D3780, retains x87 result*/
 int(*statistic)(void*,unsigned offset); /*actual Game+C, increments28/38*/
 int(*shield_sound)(void*,const char*); /*Impacts.amp named event*/
 int(*shield_recoil)(void*,unsigned,float,const float[3]); /*C6A80*/
 int(*shield_point)(void*,float[3]); /*C28F0(selector0), real surface owner*/
 int(*shield_spark)(void*,const float[3],const float[3]); /*FXShieldBulletSpark,scale1,texture0,priority3,parent player*/
 int(*point_push)(void*,unsigned,const float[3],float,float,float); /*actor1C4 virtual14/0C*/
} UCPlayerImpactServices;
void uc_player_damage_fresh(UCPlayerDamageState*);
/* Exact C2150 entry admission.1 means original returns before any effects,
 *0 requires the full blast/concussion response, -1 invalid. */
int uc_player_blast_quiet(const UCExplosionBlast*);
/* WholeC8E90 environmental kinds12/13/15 with ordinary Mainplayer458CA8,
 * absent7E4 optional observer. All original health/cadence/context side effects;
 * feedback, pain owner, actual sound and adaptive history remain completing
 * services. Source C4=2C2B60 is unconditionally NULL in this exact vtable.
 * Same health/maximum borrowed from player; death transition belongs D2AB0.
 * Returns0 complete,1 rejected before effects,2 committed service failure. */
int uc_player_damage_environment(UCPlayerDamageState*,const UCPlayerDamageInput*,float,const UCPlayerFireDamage*,const UCPlayerDamageServices*);
/* WholeC8E90 kind5, with original typed completing effect services. Does not
 * silently remove shield sparks, recoil or either point-based camera impulse. */
int uc_player_damage_impact(UCPlayerDamageState*,UCPlayerImpactState*,const UCPlayerDamageInput*,float,const UCPlayerFireDamage*,const UCPlayerDamageServices*,const UCPlayerImpactServices*);
/* Exact isolated D2C3B..D2C70 and D3053..D3064 fields in D2AB0. This does not
 * claim the whole player update/death state machine or debug-onlyBD6F0 draw. */
int uc_player_damage_age(UCPlayerDamageState*,double delta,uint8_t elapsed_enabled);
/* BC3E0(type2)->16770: borrows the SAME source+0C channel used by Blazer
 * throw decisions; only currently admitted nonmissing enemy type2. */
int uc_player_damage_blazer_hit(uint32_t mode,uint8_t disabled,float *channel);
#ifdef __cplusplus
}
#endif
#endif
