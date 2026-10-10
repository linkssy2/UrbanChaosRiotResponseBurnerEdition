#ifndef UC_MEDIC_WEAPON_H
#define UC_MEDIC_WEAPON_H
#include "entity_fresh.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCMedicWeapon {
 const char*asset,*bank_name;void*bank;
 int32_t weapon,bone,variation,blade,tags[4];
 uint8_t collision,has_model,dropped,grounded,fading,landed,skin_applied,paused;
 float time,old_y,far_time,sound_level,sound_target,sound_rate;
} UCMedicWeapon;
typedef struct UCMedicWeaponProperty {int32_t integer;const char*string;}UCMedicWeaponProperty;
typedef struct UCMedicWeaponServices {
 void*owner;
 int(*health_base)(void*);int(*listener_reset)(void*);int(*material_reset)(void*);
 int(*property)(void*,unsigned,const char*,UCMedicWeaponProperty*,unsigned*found);
 int(*bank)(void*,const char*,void**);
 int(*resolve)(void*,int32_t,const UCEntityFreshView**);
 int(*asset)(void*,const char**);int(*model_load)(void*,const char*,int32_t*blade);
 int(*tag)(void*,const UCEntityFreshView*,const char*,int32_t*);
 int(*listener_register)(void*,int32_t);int(*listener_bind)(void*,unsigned);
 int(*priority)(void*,int32_t,unsigned);
 int(*skin)(void*,int32_t);int(*material_dirty)(void*,unsigned*);
 int(*health_update)(void*);
 int(*pose)(void*,const UCEntityFreshView*,int32_t,float[16]);
 int(*set_world)(void*,const float[16]);int(*time)(void*,float*);
 int(*player_count)(void*,int32_t*);int(*player)(void*,int32_t*);
 int(*delta)(void*,float*);int(*remove)(void*);
}UCMedicWeaponServices;
/*1677F0,167150,166390 actual type34/no collision/model/variation0/attached
 * domain. Dropped physics and chainsaw animation are separate required owners.
 *0completed,1invalid/unadmitted,2committed service failure. */
int uc_medic_weapon_construct(UCMedicWeapon*,const UCEntityFreshView*,const UCMedicWeaponServices*);
int uc_medic_weapon_update(UCMedicWeapon*,const UCEntityFreshView*,const UCMedicWeaponServices*);
int uc_medic_weapon_late(UCMedicWeapon*,const UCEntityFreshView*,const UCMedicWeaponServices*);
void uc_medic_weapon_pause(UCMedicWeapon*,uint8_t);
int uc_medic_weapon_property(const UCMedicWeapon*,const char*,int32_t*);
/*2B180 creation matrix: row0/row2 swap, then source SSE grouped composition.*/
int uc_medic_weapon_matrix(const float[16],const float[16],float[16]);
#ifdef __cplusplus
}
#endif
#endif
