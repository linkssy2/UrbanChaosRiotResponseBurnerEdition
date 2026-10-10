#ifndef UC_CHARACTER_HEALTH_H
#define UC_CHARACTER_HEALTH_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCCharacterHealth {
 int32_t order,mode;float elapsed,friendly_hurt_time;
 uint8_t enemy,friendly_hurt,special,headshot_award;
} UCCharacterHealth;
typedef enum UCCharacterHealthBody {
 UC_HEALTH_OUTER94,UC_HEALTH_OUTER88,UC_HEALTH_INNER78,UC_HEALTH_STANCE14,UC_HEALTH_STANCE18
} UCCharacterHealthBody;
typedef struct UCCharacterHealthServices {
 void*owner;int32_t id;float*health;uint32_t*actor_status;int32_t*death_kind;uint8_t gore_disabled;
 int(*boolean)(void*,const char*,uint8_t*); /*actual root property, default0*/
 int(*body)(void*,UCCharacterHealthBody,uint8_t*);
 int(*stat)(void*,const char*,float); /*6BA40*/
 int(*counter)(void*,unsigned byte_offset,int eligible); /*36760*/
 int(*counter_value)(void*,unsigned byte_offset,int32_t*);
 int(*increment_kills)(void*); /*provider statistics+4*/
 int(*difficulty)(void*,uint8_t increase); /*16720/16770,4E5280*/
 int(*player)(void*,int32_t*);
 int(*target)(void*,int32_t*); /*33B40*/
 int(*player_integer)(void*,int32_t,const char*,int32_t*); /*default0*/
 int(*player_scalar)(void*,int32_t,const char*,double*); /*default0*/
 int(*dialogue)(void*,uint32_t,int32_t); /*DED50*/
 int(*headshot_text)(void*,char out[16]); /*262120*/
 int(*headshot_goal)(void*,int32_t*); /*25CE20(1)*/
 int(*message)(void*,int32_t,const char*,const char*,const char*,int32_t); /*73060*/
 int(*sound)(void*,const char*,const char*); /*actor216BD0*/
 int(*effect)(void*,const char*,const char*); /*219740/208C50*/
 int(*set_health)(void*,float); /*actorvirtual64, must update borrowed health*/
 int(*special_actor)(void*,uint8_t*); /*49800(actorID)*/
 int(*fatal_interrupt)(void*); /*39110(actorID,4F9DA4,{?,0,0,3,1,1,0})*/
 int(*indicator)(void*,int32_t,float,uint8_t); /*1998B0,4F9D58*/
 int(*notice)(void*,const char*); /*provider+18*/
 int(*enemy_type)(void*,int32_t*); /*actor20A6D0("dda enemy type",-1)*/
 int(*dda_kill)(void*,int32_t); /*362B0*/
} UCCharacterHealthServices;
/*Whole371B0. Reached services are mandatory; failure2 preserves earlier source
 * side effects. The queued reaction remains owned/cleared by38A70.*/
int uc_character_health(UCCharacterHealth*,uint32_t flags,float damage,const UCCharacterHealthServices*);
/*36760 global one-body-hit-per-frame owner. Counters are provider's18 words;
 * last_frame startsFFFFFFFF,last_offset startsFFFFFFFF (originalNULL pointer).
 * Repeated same-frame headshots upgrade the prior limb/torso counter. */
typedef struct UCCharacterHitStatistics {uint32_t*values;uint32_t last_frame,last_offset;} UCCharacterHitStatistics;
int uc_character_hit_count(UCCharacterHitStatistics*,uint32_t frame,unsigned byte_offset,int eligible);
#ifdef __cplusplus
}
#endif
#endif
