#ifndef UC_NPC_THROWN_EFFECTS_H
#define UC_NPC_THROWN_EFFECTS_H
#include "npc_thrown_weapon.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCNpcMolotovExplosion {
 uint8_t surface,hit,pad02[2];float point[3],normal[3],direction[3];
 uint8_t airborne,pad29[3];int32_t owner,team,group1,group2;
} UCNpcMolotovExplosion; /*actual60-byte event210 payload*/
typedef struct UCNpcThrownEffectsServices {
 void *owner;
 int(*asset)(void*,void*template_owner,const char*,const char**,unsigned*);
 /*Actual custom45C2BC trigger +219740 FirePos/dead_when_hidden1 for11/12.
  * Retained source/trigger.*/
 int(*particles)(void*,const char*,int32_t entity,unsigned attached_custom);
 int(*time)(void*,float*);int(*camera_position)(void*,float[4]);
 int(*light_destroy)(void*,void*);int(*light_create)(void*,const float[3],void**);
 /*Create is sourcepoint400 thenenabled1/shadows0/colorFFFF8016/
  * multiplier.66/attenuation250/specular0 in that exact order.*/
 int(*sound)(void*,const char*,unsigned stop);
 int(*random)(void*,float low,float high,double*);
 int(*flags)(void*,uint32_t);
 int(*molotov_event)(void*,void*template_owner,const UCNpcMolotovExplosion*);
 int(*pipebomb_event)(void*,void*template_owner,int32_t owner);
 int(*physics_close)(void*);
 /*Actual DAAB0 actor-hit event4F9DA4, payload damage0, flags(1,0),
  * source2, trailing(0,1,1), sender projectile. No damage placeholder.*/
 int(*hit_actor)(void*,int32_t target,int32_t projectile);
} UCNpcThrownEffectsServices;
/*Complete11/12 source branches of DB550/DBA40/DCCD0; other kinds reject.
 * Full ordered callbacks are completing actual owners, not skipped effects.*/
int uc_npc_thrown_ignition_effects(UCNpcThrownWeapon*,int32_t,const float[16],const UCNpcThrownEffectsServices*);
int uc_npc_thrown_release_effects(UCNpcThrownWeapon*,int32_t,const UCNpcThrownEffectsServices*);
int uc_npc_thrown_explosion_effects(UCNpcThrownWeapon*,const UCBulletState*,const UCNpcThrownEffectsServices*);
/*Complete Molotov11 DAAB0 branch. has_actor distinguishes real actor pointer
 * from static collision, even when the actual runtime ID is -1.*/
int uc_npc_thrown_molotov_impact(UCNpcThrownWeapon*,int32_t projectile,unsigned has_actor,int32_t target,const float point[3],const float normal[3],const UCNpcThrownEffectsServices*);
/*Original DA8E0 camera-facing light displacement, reused by DC330.*/
int uc_npc_thrown_light_position(const float world[16],const float camera_position[4],float out[3]);
#ifdef __cplusplus
}
#endif
#endif
