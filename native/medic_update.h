#ifndef UC_MEDIC_UPDATE_H
#define UC_MEDIC_UPDATE_H
#include "medic_root_policy.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCMedicUpdate {
 uint32_t reactions;uint32_t*status;
 int32_t root,order,state;uint8_t finished,can_heal;
} UCMedicUpdate;
typedef struct UCMedicUpdateServices {
 void*owner;
 int(*unrestrain)(void*);
 int(*children_ready)(void*,uint8_t*); /*39190*/
 int(*foreground)(void*,int32_t*);
 int(*complete)(void*,int32_t order,int32_t state,int32_t sender); /*1F90A0*/
 int(*reactions)(void*,uint8_t,uint8_t); /*368E0(1,1), shared status may change*/
 int(*integer)(void*,const char*,int32_t,int32_t*);
 int(*clear_debug)(void*);int(*debug)(void*,const char*); /*E2DD0/E2EE0*/
 int(*body_ready)(void*,uint8_t*); /*body64*/
 int(*drop_allowed)(void*,int32_t foreground,uint8_t*); /*1F7AA0 default1*/
 int(*box_event)(void*,uint8_t down,int32_t sender); /*2EE90->1F90A0*/
 int(*secondary)(void*,int32_t,float); /*323E0*/
 int(*base)(void*,float); /*37160*/
 int(*foreground_busy)(void*,uint8_t*); /*32560: suspended/replacement E0*/
 int(*should_run)(void*,int32_t,uint8_t*); /*secondary "should run", default0*/
} UCMedicUpdateServices;
/* Whole169C10 execution restricted to no assigned order and no death-camera
 * branch. Those domains reject explicitly; no successful order/death stubs.
 * Source callbacks may mutate borrowed actor status. Zero complete,1 domain,
 * 2 missing/failed completing service. */
int uc_medic_unassigned_update(UCMedicUpdate*,UCMedicRootPolicy*,float,const UCMedicUpdateServices*);
/*171190 live/non-death-camera domain; distinct plain-Medic secondary policy. */
int uc_medic_plain_update(UCMedicUpdate*,UCMedicRootPolicy*,int32_t speech,float,const UCMedicUpdateServices*);
typedef struct UCMedicTeamUpdate {
 int32_t quit_count,*quit_flags,quit_behaviour,under_attack;
 uint32_t bullet_by;uint8_t pending_attack;const float *actor_position;
} UCMedicTeamUpdate;
typedef struct UCMedicTeamUpdateServices {
 void *owner;
 int(*flags_ready)(void*,int32_t,const int32_t*,uint8_t*);
 int(*heartbeat_remove)(void*);int(*client_remove)(void*,int32_t);
 int(*replace_authored)(void*,int32_t);int(*clear_flags)(void*);int(*clear_children)(void*);
 int(*weapon_threat)(void*,float,uint8_t*); /*2C720(actor,600,0,0)*/
 int(*bullet_near)(void*,float,uint8_t*); /*36B20(300)*/
 int(*player_position)(void*,float[3],uint8_t*);
 int(*movement_busy)(void*,uint8_t*); /*inner A8+84*/
 int(*attack_event)(void*,int32_t); /*75,0*/
 int(*attack_create)(void*,int32_t*); /*UnderAttack Duck0,Cower1,Time3 ->32D10*/
 int(*emote)(void*,int32_t,int32_t);
 int(*choose)(void*,int32_t,int32_t); /*126*/
 int(*order_integer)(void*,int32_t,const char*,int32_t*,uint8_t*);
 int(*resume_follow)(void*,int32_t); /*17,0*/
 int(*replace_order)(void*,int32_t command,int32_t object); /*source mode1 descriptors*/
 int(*order_complete)(void*,int32_t); /*136,0*/
} UCMedicTeamUpdateServices;
/*Whole169C10 live/non-death-camera domain, including assigned orders.*/
int uc_medic_team_update(UCMedicUpdate*,UCMedicRootPolicy*,UCMedicTeamUpdate*,float,
 const UCMedicUpdateServices*,const UCMedicTeamUpdateServices*);
#ifdef __cplusplus
}
#endif
#endif
