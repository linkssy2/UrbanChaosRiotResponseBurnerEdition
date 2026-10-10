#ifndef UC_CHARACTER_REACTION_H
#define UC_CHARACTER_REACTION_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/*362D0 retained fields, borrowed by the actor's actual common root.*/
typedef struct UCCharacterReaction {
 uint32_t status,flags;int32_t actor,root,foreground,damage_owner;
 uint8_t enemy;uintptr_t stun_task;
} UCCharacterReaction;
enum UCCharacterReactionSpawn {UC_REACTION_FRIENDLY_DEATH,UC_REACTION_RESTRAINED,UC_REACTION_ELECTROCUTE,UC_REACTION_TASER,UC_REACTION_HIT};
typedef struct UCCharacterReactionDescriptor {
 const float*world;int32_t entity,killer,location,attacker;
 uint8_t incapacitated,obituary,on_floor,player_tasering,enemy,lightning_only,knockback,shield;
 float required_time,speed;
} UCCharacterReactionDescriptor;
typedef struct UCCharacterReactionServices {
 void*owner;const float*world;
 /*scope0 root,1 actor,2 current foreground; actual default is passed.*/
 int(*property)(void*,unsigned,const char*,uint8_t,uint8_t*);
 int(*dead)(void*,uint8_t*); /*DDDA0*/
 int(*health)(void*,float*);int(*set_health)(void*,float);
 int(*task_status)(void*,uintptr_t,int32_t*);int(*task_release)(void*,uintptr_t);
 int(*task_create)(void*,uintptr_t*);int(*clip)(void*,const char*,uintptr_t);
 int(*procedural_hit)(void*,float,float); /*inner7C*/
 int(*random)(void*,uint8_t*); /*2D3800*/
 int(*player)(void*,int32_t*);int(*killer)(void*,int32_t*); /*338B0(0)*/
 int(*taser_time)(void*,double*); /*89A80*/
 int(*unrestrain)(void*); /*2F3D0*/
 int(*spawn)(void*,unsigned,const UCCharacterReactionDescriptor*,int32_t placement,int32_t*);
 int(*queue)(void*,int32_t,uint8_t secondary); /*32D10 /32CB0*/
 int(*session_death)(void*); /*session virtual1C(4,{1},0)*/
 /*site37B52 globalbroadcast,38171electric event,38261actor3F,
  *383E9 globaltaser continuation. Actual payloads retain both Booleans/float.*/
 int(*event)(void*,uint32_t site,int32_t actor,int32_t root,uint8_t,uint8_t,float);
} UCCharacterReactionServices;
/*Whole37AF0 and37D10; required dynamic behaviors/services are never bypassed.
 *Zero completed;1invalid;2committed service failure.*/
int uc_character_hit_response(UCCharacterReaction*,int32_t location,uint8_t knockback,uint8_t shield,const UCCharacterReactionServices*);
int uc_character_reaction_update(UCCharacterReaction*,uint32_t reactions,float damage,const UCCharacterReactionServices*);
#ifdef __cplusplus
}
#endif
#endif
