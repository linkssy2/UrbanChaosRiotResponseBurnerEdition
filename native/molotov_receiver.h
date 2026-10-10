#ifndef UC_MOLOTOV_RECEIVER_H
#define UC_MOLOTOV_RECEIVER_H
#include "npc_thrown_effects.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Source F8210's final 48-byte event9. Padding is not read by that source.
 * The event owner must complete actual registered receivers synchronously. */
typedef struct UCMolotovBlast {
 float point[3],radius;int32_t owner;
 uint8_t flag0,flag1,pad22[2];float amount;int32_t kind;
 uint8_t flag32,pad33[3];int32_t team;uint8_t flag40,pad41[3];int32_t projectile;
} UCMolotovBlast;
typedef struct UCMolotovReceiverServices {
 void *owner;
 /* Missing sender is a successful no-effect event. The returned matrix is
  * borrowed only until this callback returns; copy into the supplied output. */
 int(*sender)(void*,int32_t,float matrix[16],unsigned *found);
 int(*particles)(void*,const char*,const float matrix[16]);
 /* Actual dynamic20A230 children, parent is the live template ID. */
 int(*sound)(void*,int32_t parent,const float matrix[16],const char *start,const char *stop,float seconds);
 int(*damage_sphere)(void*,int32_t parent,const float matrix[16],float radius,int32_t owner,float seconds,int32_t session,int32_t unique);
 int(*scorch)(void*,const char*,const float point[3],const float normal[3],float width,float height,uint32_t mask);
 int(*broadcast)(void*,uint32_t event,const UCMolotovBlast*,int32_t sender);
} UCMolotovReceiverServices;
/* Five ground-burst and three direct-hit assets from F8210; NULL past7. */
const char *uc_molotov_particle_asset(unsigned index);
/* Complete F8210 control flow and arithmetic, with concrete particle, Sound,
 * FireDamageSphere, decal and event consumers required for reached branches.
 * world is the actual template's mutable transform; sender is the projectile.
 * A completing callback failure can follow committed effects: do not retry. */
int uc_molotov_receive(float world[16],int32_t template_id,int32_t sender,
 const UCNpcMolotovExplosion*,const UCMolotovReceiverServices*);
#ifdef __cplusplus
}
#endif
#endif
