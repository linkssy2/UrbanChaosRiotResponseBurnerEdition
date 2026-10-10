#ifndef UC_PLAYER_DEATH_H
#define UC_PLAYER_DEATH_H
#include "player_damage.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCPlayerDeath {
 uint32_t action,stage,view;int32_t id,animation,killer;
 uint8_t lowered,input_present,move_dirty,look_dirty;
 uint32_t input_timer;float input_range;uintptr_t interaction;
 float heartbeat,second_heartbeat,heartbeat_interval,remaining;
 float position[3];
} UCPlayerDeath;
typedef struct UCPlayerDeathServices {
 void*owner;
 int(*flashlight_off)(void*); /*actor23C virtual4(0), even already dead.*/
 int(*cleanup)(void*); /*C2740: retained animation, Idle sound and old action.*/
 int(*health_zero)(void*); /*actual virtual64(0).*/
 int(*stop_input)(void*); /*actual1B0 virtual10.*/
 int(*lower)(void*); /*C1E60(1,0,0).*/
 int(*input_velocity)(void*,float[3]); /*2A42E0; queried even with NULL ragdoll.*/
 int(*sequence_count)(void*,uint32_t*); /*actual DEATH_SEQUENCE name owner.*/
 int(*sequence)(void*,uint32_t,int32_t*);
 int(*create_sequence)(void*); /*actual fallback descriptor below,20A260 child attach.*/
 int(*character_type)(void*,int32_t,int32_t*); /*1F7AF0 default1.*/
 int(*npc_count)(void*,uint32_t*); /*actual AI registry2F090.*/
 int(*npc)(void*,uint32_t,int32_t*); /*2F570.*/
 int(*position)(void*,int32_t,float[3],uint8_t*); /*real1F8FC0,0 missing.*/
 int(*sequence_event)(void*,int32_t target,uint32_t view,int32_t sender); /*eventB8,size4.*/
 int(*broadcast)(void*,uint32_t view,int32_t owner,int32_t sender); /*event90,size8.*/
 int(*notify)(void*,int32_t player,int32_t killer,const UCPlayerFireDamage*); /*Gamevirtual1C(0,{player,killer,damage},0).*/
 int(*profile_died)(void*); /*actual fresh profile3BC=1.*/
 int(*adaptive_death)(void*); /*16A10, offline only.*/
 int(*transition)(void*,uint32_t,uint32_t); /*wholeBFE00 old-action cleanup.*/
 int(*notice)(void*,const char*,float); /*6BA40("Died",1).*/
} UCPlayerDeathServices;
/* C7F40 ordinary Mainplayer domain: exact C0/C4 getters are NULL and network
 * singleton2918A0 is absent. Live providers/names/AI queries remain completing
 * services in original order. No replacement death camera or fake event.
 * Fallback descriptor: PlayerDeathSequence, Camera Start Anim=
 * Assets_CharacterAnimation/Generic/GCA_Post_Death_Events/gca_post_death_falling_death_cam_ALT_1.kfa;
 * Character Anim/Camera Anim=NULL, Static Shader=NULL, Static Sound Bank=NULL,
 * SoundBank=Banks/Setpiece/PlayerDeath/PlayerDeathBrute.amp; attached to player.
 * Runtime must register source-immediate Brute/MedicDeathSequence before this
 * callback can truthfully report an empty list. Query owners are bounded4096.
 *0 complete,1 invalid before effects,2 committed failure; never retry2. */
int uc_player_death_begin(UCPlayerDeath*,int32_t killer,const UCPlayerFireDamage*,const UCPlayerDeathServices*);
typedef struct UCPlayerDeathStepServices {
 void*owner;int(*motion)(void*); /*21D8B0 then218C40 actual completed body.*/
 double(*delta)(void*);int(*heartbeat)(void*); /*Player bank HeartBeat.*/
 int(*feedback)(void*,uint32_t view,float,float,uint32_t); /*55E00 virtual18.*/
} UCPlayerDeathStepServices;
int uc_player_death_step(UCPlayerDeath*,const UCPlayerDeathStepServices*); /*wholeBDBC0*/
#ifdef __cplusplus
}
#endif
#endif
