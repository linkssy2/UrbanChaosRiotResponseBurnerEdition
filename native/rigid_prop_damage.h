#ifndef UC_RIGID_PROP_DAMAGE_H
#define UC_RIGID_PROP_DAMAGE_H
#include "rigid_prop.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCRigidPropDamage {
    int32_t hits,owner,contacts;
    uint32_t explosions;
    float delay,blast,life;
    uint8_t exploded;
    const char *point_particle,*shot_particle;
} UCRigidPropDamage;
/* Exact ten authored additions of1354F0. Fields absent from that descriptor
 * must retain the actual Explosion constructor defaults, not caller guesses. */
typedef struct UCRigidPropExplosion {
    float position[3],rotation[3];
    const char *sound;
    float scale,radius,power;
    int32_t damage_type,owner,particle_type,parent;
    uint8_t affects_doors,ignore_collision,force_kill;
} UCRigidPropExplosion;
typedef struct UCRigidPropDamageServices {
    void *owner;
    int32_t id;
    const float *position;
    double delta;
    int (*base_update)(void *);
    int (*body)(void *,uint8_t *present);
    int (*motion)(void *,int32_t *type,float velocity[3]);
    int (*fixed)(void *); /*2F1880(body,6,1,0)*/
    int (*wake)(void *);
    int (*value)(void *,uint32_t key,uint32_t *word); /*1354A0 low word*/
    int (*particle)(void *,const char *asset,const char *bone,uint8_t hidden);
    int (*resolve_parent)(void *,int32_t projectile,int32_t *parent,uint8_t *found);
    int (*flags)(void *,uint32_t);
    int (*explosion)(void *,const UCRigidPropExplosion *);
    int (*groups)(void *,int32_t *);
    int (*group_one)(void *); /*21B070(1,1), then actual body rebuild3D350*/
    int (*sound)(void *,const char *);
    int (*destroy)(void *); /*3D150: actual respawn/record lifecycle*/
} UCRigidPropDamageServices;
void uc_rigid_prop_damage_begin(UCRigidPropDamage *,const UCRigidPropParameters *);
/* Whole135840 after completing the explicit3DAA0 base service;135AE0 BF and
 *135BA0 Bullet callbacks, including original no-op2B6230 tail. Actual property
 *21/22 bodies, sound, particles, registry and deferred Explosion remain required
 * typed owners.0 completion,1 invalid,2 committed service failure; never retry2. */
int uc_rigid_prop_damage_step(UCRigidPropDamage *,const UCRigidPropParameters *,const UCRigidPropDamageServices *);
int uc_rigid_prop_damage_event(UCRigidPropDamage *,const UCRigidPropParameters *,const UCRigidPropDamageServices *,int32_t sender);
int uc_rigid_prop_damage_bullet(UCRigidPropDamage *,const UCRigidPropParameters *,const UCRigidPropDamageServices *,int32_t projectile);
int uc_rigid_prop_explode(UCRigidPropDamage *,const UCRigidPropParameters *,const UCRigidPropDamageServices *,int32_t owner);
#ifdef __cplusplus
}
#endif
#endif
